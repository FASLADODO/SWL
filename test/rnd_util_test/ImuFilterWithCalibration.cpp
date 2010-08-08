#include "stdafx.h"
#include "swl/Config.h"
#include "ImuFilterRunner.h"
#include "AdisUsbz.h"
#include "swl/rnd_util/ExtendedKalmanFilter.h"
#include "swl/rnd_util/DiscreteNonlinearStochasticSystem.h"
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_statistics.h>
#include <boost/math/constants/constants.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <functional>
#include <vector>
#include <string>
#include <numeric>
#include <cmath>
#include <cassert>


#if defined(_DEBUG) && defined(__SWL_CONFIG__USE_DEBUG_NEW)
#include "swl/ResourceLeakageCheck.h"
#define new DEBUG_NEW
#endif

//#define __USE_ADIS16350_DATA 1


const char ADIS16350_SUPPLY_OUT =	0x02;
const char ADIS16350_XGYRO_OUT =	0x04;
const char ADIS16350_YGYRO_OUT =	0x06;
const char ADIS16350_ZGYRO_OUT =	0x08;
const char ADIS16350_XACCL_OUT =	0x0A;
const char ADIS16350_YACCL_OUT =	0x0C;
const char ADIS16350_ZACCL_OUT =	0x0E;
const char ADIS16350_XTEMP_OUT =	0x10;
const char ADIS16350_YTEMP_OUT =	0x12;
const char ADIS16350_ZTEMP_OUT =	0x14;
const char ADIS16350_AUX_ADC =		0x16;

const double ADIS16350_SUPLY_SCALE_FACTOR =		1.8315e-1;  // binary, [V]
const double ADIS16350_GYRO_SCALE_FACTOR =		0.07326;  // 2's complement, [deg/sec]
//const double ADIS16350_GYRO_SCALE_FACTOR =	0.07326 * boost::math::constants::pi<double>() / 180.0;  // 2's complement, [rad/sec]
const double ADIS16350_ACCL_SCALE_FACTOR =		2.522e-3;  // 2's complement, [g]
const double ADIS16350_TEMP_SCALE_FACTOR =		0.1453;  // 2's complement, [deg]
const double ADIS16350_ADC_SCALE_FACTOR =		0.6105e-3;  // binary, [V]


namespace {

class ImuSystem: public swl::DiscreteNonlinearStochasticSystem
{
public:
	typedef swl::DiscreteNonlinearStochasticSystem base_type;

public:
	ImuSystem(const double Ts, const size_t stateDim, const size_t inputDim, const size_t outputDim, const gsl_vector *initial_gravity, const gsl_matrix *Qd, const gsl_matrix *Rd)
	: base_type(stateDim, inputDim, outputDim, (size_t)-1, (size_t)-1),
	  Ts_(Ts), Phi_(NULL), A_(NULL), B_(NULL), Bd_(NULL), Bu_(NULL), Cd_(NULL), Qd_(NULL), Rd_(NULL), f_eval_(NULL), h_eval_(NULL), initial_gravity_(NULL),
	  A_tmp_(NULL)
	{
		// Phi = exp(A * Ts) -> I + A * Ts where A = df/dx
		//	the EKF approximation for Phi is I + A * Ts
		Phi_ = gsl_matrix_alloc(stateDim_, stateDim_);
		gsl_matrix_set_zero(Phi_);

		A_ = gsl_matrix_alloc(stateDim_, stateDim_);
		gsl_matrix_set_zero(A_);

		// B = [ 0 0 0 ; 0 0 0 ; 0 0 0 ; 1 0 0 ; 0 1 0 ; 0 0 1 ; 0 0 0 ; ... ; 0 0 0 ]
		B_ = gsl_matrix_alloc(stateDim_, inputDim_);
		gsl_matrix_set_zero(B_);
		gsl_matrix_set(B_, 3, 0, 1.0);
		gsl_matrix_set(B_, 4, 1, 1.0);
		gsl_matrix_set(B_, 5, 2, 1.0);

		Bd_ = gsl_matrix_alloc(stateDim_, inputDim_);
		Bu_ = gsl_vector_alloc(stateDim_);
		gsl_vector_set_zero(Bu_);

		// C = [
		//	0 0 0  0 0 0  1 0 0   0 0 0 0  0 0 0  1 0 0  0 0 0
		//	0 0 0  0 0 0  0 1 0   0 0 0 0  0 0 0  0 1 0  0 0 0
		//	0 0 0  0 0 0  0 0 1   0 0 0 0  0 0 0  0 0 1  0 0 0
		//	0 0 0  0 0 0  0 0 0   0 0 0 0  1 0 0  0 0 0  1 0 0
		//	0 0 0  0 0 0  0 0 0   0 0 0 0  0 1 0  0 0 0  0 1 0
		//	0 0 0  0 0 0  0 0 0   0 0 0 0  0 0 1  0 0 0  0 0 1
		// ]
		Cd_ = gsl_matrix_alloc(outputDim_, stateDim_);
		gsl_matrix_set_zero(Cd_);
		gsl_matrix_set(Cd_, 0, 6, 1.0);  gsl_matrix_set(Cd_, 0, 16, 1.0);
		gsl_matrix_set(Cd_, 1, 7, 1.0);  gsl_matrix_set(Cd_, 1, 17, 1.0);
		gsl_matrix_set(Cd_, 2, 8, 1.0);  gsl_matrix_set(Cd_, 2, 18, 1.0);
		gsl_matrix_set(Cd_, 3, 13, 1.0);  gsl_matrix_set(Cd_, 3, 19, 1.0);
		gsl_matrix_set(Cd_, 4, 14, 1.0);  gsl_matrix_set(Cd_, 4, 20, 1.0);
		gsl_matrix_set(Cd_, 5, 15, 1.0);  gsl_matrix_set(Cd_, 5, 21, 1.0);

		// Qd = W * Q * W^T 
		//	the EKF approximation of Qd will be W * [ Q * Ts ] * W^T
		Qd_ = gsl_matrix_alloc(stateDim_, stateDim_);
		gsl_matrix_memcpy(Qd_, Qd);

		// Rd = V * R * V^T
		Rd_ = gsl_matrix_alloc(outputDim_, outputDim_);
		gsl_matrix_memcpy(Rd_, Rd);

		//
		f_eval_ = gsl_vector_alloc(stateDim_);
		gsl_vector_set_zero(f_eval_);

		//
		h_eval_ = gsl_vector_alloc(outputDim_);
		gsl_vector_set_zero(h_eval_);

		// initial gravity
		initial_gravity_ = gsl_vector_alloc(initial_gravity->size);
		gsl_vector_memcpy(initial_gravity_, initial_gravity);

		//
		A_tmp_ = gsl_matrix_alloc(stateDim_, stateDim_);
	}
	~ImuSystem()
	{
		gsl_matrix_free(Phi_);  Phi_ = NULL;
		gsl_matrix_free(A_);  A_ = NULL;
		gsl_matrix_free(B_);  B_ = NULL;
		gsl_matrix_free(Bd_);  Bd_ = NULL;
		gsl_vector_free(Bu_);  Bu_ = NULL;
		gsl_matrix_free(Cd_);  Cd_ = NULL;
		gsl_matrix_free(Qd_);  Qd_ = NULL;
		gsl_matrix_free(Rd_);  Rd_ = NULL;

		gsl_vector_free(f_eval_);  f_eval_ = NULL;
		gsl_vector_free(h_eval_);  h_eval_ = NULL;

		gsl_vector_free(initial_gravity_);  initial_gravity_ = NULL;

		gsl_matrix_free(A_tmp_);  A_tmp_ = NULL;
	}

private:
	ImuSystem(const ImuSystem &rhs);
	ImuSystem & operator=(const ImuSystem &rhs);

public:
	// the stochastic differential equation: f = f(k, x(k), u(k), v(k))
	/*virtual*/ gsl_vector * evaluatePlantEquation(const size_t step, const gsl_vector *state, const gsl_vector *input, const gsl_vector *noise) const
	{
		const gsl_matrix *Phi = getStateTransitionMatrix(step, state);
		const gsl_vector *Bu = getControlInput(step, state);
		gsl_vector_memcpy(f_eval_, Bu);
		gsl_blas_dgemv(CblasNoTrans, 1.0, Phi, state, 1.0, f_eval_);

		return f_eval_;
	}
	/*virtual*/ gsl_matrix * getStateTransitionMatrix(const size_t step, const gsl_vector *state) const  // Phi(k) = exp(A(k) * Ts) where A(k) = df(k, x(k), u(k), 0)/dx
	{
		const double &Px = gsl_vector_get(state, 0);
		const double &Py = gsl_vector_get(state, 1);
		const double &Pz = gsl_vector_get(state, 2);
		const double &Vx = gsl_vector_get(state, 3);
		const double &Vy = gsl_vector_get(state, 4);
		const double &Vz = gsl_vector_get(state, 5);
		const double &Ap = gsl_vector_get(state, 6);
		const double &Aq = gsl_vector_get(state, 7);
		const double &Ar = gsl_vector_get(state, 8);
		const double &E0 = gsl_vector_get(state, 9);
		const double &E1 = gsl_vector_get(state, 10);
		const double &E2 = gsl_vector_get(state, 11);
		const double &E3 = gsl_vector_get(state, 12);
		const double &Wp = gsl_vector_get(state, 13);
		const double &Wq = gsl_vector_get(state, 14);
		const double &Wr = gsl_vector_get(state, 15);
		const double &Abp = gsl_vector_get(state, 16);
		const double &Abq = gsl_vector_get(state, 17);
		const double &Abr = gsl_vector_get(state, 18);
		const double &Wbp = gsl_vector_get(state, 19);
		const double &Wbq = gsl_vector_get(state, 20);
		const double &Wbr = gsl_vector_get(state, 21);

		gsl_matrix_set(A_, 0, 3, 1.0);
		gsl_matrix_set(A_, 1, 4, 1.0);
		gsl_matrix_set(A_, 2, 5, 1.0);

		gsl_matrix_set(A_, 3, 6, 2.0 * (0.5 - E2*E2 - E3*E3));
		gsl_matrix_set(A_, 3, 7, 2.0 * (E1*E2 - E0*E3));
		gsl_matrix_set(A_, 3, 8, 2.0 * (E1*E3 + E0*E2));
		gsl_matrix_set(A_, 3, 9, 2.0 * (-E3*Aq + E2*Ar));
		gsl_matrix_set(A_, 3, 10, 2.0 * (E2*Aq + E3*Ar));
		gsl_matrix_set(A_, 3, 11, 2.0 * (-2.0*E2*Ap + E1*Aq + E0*Ar));
		gsl_matrix_set(A_, 3, 12, 2.0 * (-2.0*E3*Ap - E0*Aq + E1*Ar));

		gsl_matrix_set(A_, 4, 6, 2.0 * (E1*E2 + E0*E3));
		gsl_matrix_set(A_, 4, 7, 2.0 * (0.5 - E1*E1 - E3*E3));
		gsl_matrix_set(A_, 4, 8, 2.0 * (E2*E3 - E0*E1));
		gsl_matrix_set(A_, 4, 9, 2.0 * (E3*Ap - E1*Ar));
		gsl_matrix_set(A_, 4, 10, 2.0 * (E2*Ap - 2.0*E1*Aq - E0*Ar));
		gsl_matrix_set(A_, 4, 11, 2.0 * (E1*Ap + E3*Ar));
		gsl_matrix_set(A_, 4, 12, 2.0 * (E0*Ap - 2.0*E3*Aq + E2*Ar));

		gsl_matrix_set(A_, 5, 6, 2.0 * (E1*E3 - E0*E2));
		gsl_matrix_set(A_, 5, 7, 2.0 * (E2*E3 + E0*E1));
		gsl_matrix_set(A_, 5, 8, 2.0 * (0.5 - E1*E1 - E2*E2));
		gsl_matrix_set(A_, 5, 9, 2.0 * (-E2*Ap + E1*Aq));
		gsl_matrix_set(A_, 5, 10, 2.0 * (E3*Ap + E0*Aq - 2.0*E1*Ar));
		gsl_matrix_set(A_, 5, 11, 2.0 * (-E0*Ap + E3*Aq - 2.0*E2*Ar));
		gsl_matrix_set(A_, 5, 12, 2.0 * (E1*Ap + E2*Aq));

		gsl_matrix_set(A_, 9, 9, 0.0);
		gsl_matrix_set(A_, 9, 10, -0.5 * Wp);
		gsl_matrix_set(A_, 9, 11, -0.5 * Wq);
		gsl_matrix_set(A_, 9, 12, -0.5 * Wr);
		gsl_matrix_set(A_, 9, 13, -0.5 * E1);
		gsl_matrix_set(A_, 9, 14, -0.5 * E2);
		gsl_matrix_set(A_, 9, 15, -0.5 * E3);

		gsl_matrix_set(A_, 10, 9, -0.5 * -Wp);
		gsl_matrix_set(A_, 10, 10, 0.0);
		gsl_matrix_set(A_, 10, 11, -0.5 * -Wr);
		gsl_matrix_set(A_, 10, 12, -0.5 * Wq);
		gsl_matrix_set(A_, 10, 13, -0.5 * -E0);
		gsl_matrix_set(A_, 10, 14, -0.5 * E3);
		gsl_matrix_set(A_, 10, 15, -0.5 * -E2);

		gsl_matrix_set(A_, 11, 9, -0.5 * -Wq);
		gsl_matrix_set(A_, 11, 10, -0.5 * Wr);
		gsl_matrix_set(A_, 11, 11, 0.0);
		gsl_matrix_set(A_, 11, 12, -0.5 * -Wp);
		gsl_matrix_set(A_, 11, 13, -0.5 * -E3);
		gsl_matrix_set(A_, 11, 14, -0.5 * -E0);
		gsl_matrix_set(A_, 11, 15, -0.5 * E1);

		gsl_matrix_set(A_, 12, 9, -0.5 * -Wr);
		gsl_matrix_set(A_, 12, 10, -0.5 * -Wq);
		gsl_matrix_set(A_, 12, 11, -0.5 * Wp);
		gsl_matrix_set(A_, 12, 12, 0.0);
		gsl_matrix_set(A_, 12, 13, -0.5 * E2);
		gsl_matrix_set(A_, 12, 14, -0.5 * -E1);
		gsl_matrix_set(A_, 12, 15, -0.5 * -E0);

		// Phi = exp(A * Ts) -> I + A * Ts where A = df/dx
		//	the EKF approximation for Phi is I + A * Ts

		gsl_matrix_memcpy(Phi_, A_);
		gsl_matrix_scale(Phi_, Ts_);
		for (size_t i = 0; i < stateDim_; ++i)
			gsl_matrix_set(Phi_, i, i, gsl_matrix_get(Phi_, i, i) + 1.0);

		return Phi_;
	}
	/*virtual*/ gsl_vector * getControlInput(const size_t step, const gsl_vector *state) const  // Bu(k) = Bd(k) * u(k)
	{
		// Bu = Bd * u where u(t) = initial gravity
		//	Bd = integrate(exp(A * t), {t, 0, Ts}) * B or A^-1 * (Ad - I) * B if A is nonsingular
		//	Ad = Phi = exp(A * Ts) -> I + A * Ts where A = df/dx

		// TODO [check] >>
		//	integrate(exp(A * t), {t, 0, Ts}) -> integrate(I + A * t, {t, 0, Ts}) -> I * Ts + A * Ts^2 / 2
		//	Bd = integrate(exp(A * t), {t, 0, Ts}) * B -> integrate(I + A * t, {t, 0, Ts}) * B -> (I * Ts + A * Ts^2 / 2) * B ???
		gsl_matrix_memcpy(A_tmp_, A_);
		gsl_matrix_scale(A_tmp_, 0.5 * Ts_*Ts_);
		for (size_t i = 0; i < stateDim_; ++i)
			gsl_matrix_set(A_tmp_, i, i, gsl_matrix_get(A_tmp_, i, i) + Ts_);

		gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, A_tmp_, B_, 0.0, Bd_);
		gsl_blas_dgemv(CblasNoTrans, 1.0, Bd_, initial_gravity_, 0.0, Bu_);

		return Bu_;
	}
	///*virtual*/ gsl_matrix * getProcessNoiseCouplingMatrix(const size_t step) const  {  return W_;  }  // W(k) = df(k, x(k), u(k), 0)/dw

	// the observation equation: h = h(k, x(k), u(k), v(k))
	/*virtual*/ gsl_vector * evaluateMeasurementEquation(const size_t step, const gsl_vector *state, const gsl_vector *input, const gsl_vector *noise) const 
	{
		gsl_blas_dgemv(CblasNoTrans, 1.0, Cd_, state, 0.0, h_eval_);
		return h_eval_;
	}
	/*virtual*/ gsl_matrix * getOutputMatrix(const size_t step, const gsl_vector *state) const  // Cd(k) = dh(k, x(k), u(k), 0)/dx
	{  return Cd_;  }
	/*virtual*/ gsl_vector * getMeasurementInput(const size_t step, const gsl_vector *state) const  // Du(k) = D(k) * u(k) (D == Dd)
	{  throw std::runtime_error("this function doesn't have to be called");  }
	///*virtual*/ gsl_matrix * getMeasurementNoiseCouplingMatrix(const size_t step) const  {  return V_;  }  // V(k) = dh(k, x(k), u(k), 0)/dv

	// noise covariance matrices
	/*virtual*/ gsl_matrix * getProcessNoiseCovarianceMatrix(const size_t step) const  {  return Qd_;  }  // Qd = W * Q * W^T, but not Q
	/*virtual*/ gsl_matrix * getMeasurementNoiseCovarianceMatrix(const size_t step) const  {  return Rd_;  }  // Rd = V * R * V^T, but not R

private:
	const double Ts_;

	gsl_matrix *Phi_;
	gsl_matrix *A_;  // A = df/dx
	gsl_matrix *B_;  // B = df/du
	gsl_matrix *Bd_;  // Bd = integrate(exp(A * t), {t, 0, Ts}) * B or A^-1 * (Ad - I) * B if A is nonsingular
	gsl_vector *Bu_;  // Bu = Bd * u
	gsl_matrix *Cd_;  // Cd = C
	gsl_matrix *Qd_;
	gsl_matrix *Rd_;

	// evalution of the plant equation: f = f(k, x(k), u(k), 0)
	gsl_vector *f_eval_;
	// evalution of the measurement equation: h = h(k, x(k), u(k), 0)
	gsl_vector *h_eval_;

	// initial gravity
	gsl_vector *initial_gravity_;

	gsl_matrix *A_tmp_;
};

}  // unnamed namespace

// [ref] wikipedia
// (latitude, longitude, altitude) = (phi, lambda, h) = (36.36800, 127.35532, ?)
// g(phi, h) = 9.780327 * (1 + 0.0053024 * sin(phi)^2 - 0.0000058 * sin(2 * phi)^2) - 3.086 * 10^-6 * h
const double deg2rad = boost::math::constants::pi<double>() / 180.0;
const double phi = 36.36800 * deg2rad;  // latitude [rad]
const double lambda = 127.35532 * deg2rad;  // longitude [rad]
const double h = 0.0;  // altitude: unknown [m]
const double sin_phi = std::sin(phi);
const double sin_2phi = std::sin(2 * phi);
/*static*/ const double ImuFilterRunner::REF_GRAVITY_ = 9.780327 * (1 + 0.0053024 * sin_phi*sin_phi - 0.0000058 * sin_2phi*sin_2phi) - 3.086e-6 * h;  // [m/sec^2]

// [ref] "The Global Positioning System and Inertial Navigation", Jay Farrell & Mattthew Barth, pp. 22
/*static*/ const double ImuFilterRunner::REF_ANGULAR_VEL_ = 7.292115e-5;  // [rad/sec]

ImuFilterRunner::ImuFilterRunner(const double Ts, const size_t stateDim, const size_t inputDim, const size_t outputDim, AdisUsbz *adis)
: numAccelParam_(9), numGyroParam_(3), dim_(3), Ts_(Ts), adis_(adis),
  initialGravity_(NULL), accelCalibrationParam_(NULL), accelCalibrationCovariance_(NULL), gyroCalibrationParam_(NULL), gyroCalibrationCovariance_(NULL),
  measuredAccel_(NULL), measuredAngularVel_(NULL), calibratedAccel_(NULL), calibratedAngularVel_(NULL), currAccel_(NULL), currAngularVel_(NULL),
  actualMeasurement_(NULL), currPos_(NULL), prevPos_(NULL), currVel_(NULL), prevVel_(NULL), currAngle_(NULL), prevAngle_(NULL)
{
	initialGravity_ = gsl_vector_alloc(dim_);
	//initialAngularVel_ = gsl_vector_alloc(dim_);

	accelCalibrationParam_ = gsl_vector_alloc(numAccelParam_);
	accelCalibrationCovariance_ = gsl_matrix_alloc(numAccelParam_, numAccelParam_);
	gyroCalibrationParam_ = gsl_vector_alloc(numGyroParam_);
	gyroCalibrationCovariance_ = gsl_matrix_alloc(numGyroParam_, numGyroParam_);

	measuredAccel_ = gsl_vector_alloc(dim_);
	measuredAngularVel_ = gsl_vector_alloc(dim_);
	calibratedAccel_ = gsl_vector_alloc(dim_);
	calibratedAngularVel_ = gsl_vector_alloc(dim_);

	actualMeasurement_ = gsl_vector_alloc(outputDim);

	currPos_ = gsl_vector_alloc(dim_);
	prevPos_ = gsl_vector_alloc(dim_);
	currVel_ = gsl_vector_alloc(dim_);
	prevVel_ = gsl_vector_alloc(dim_);
	currAngle_ = gsl_vector_alloc(dim_);
	prevAngle_ = gsl_vector_alloc(dim_);

	gsl_vector_set_zero(currPos_);  // initially stationary
	gsl_vector_set_zero(prevPos_);  // initially stationary
	gsl_vector_set_zero(currVel_);  // initially stationary
	gsl_vector_set_zero(prevVel_);  // initially stationary
	gsl_vector_set_zero(currAngle_);  // initially stationary
	gsl_vector_set_zero(prevAngle_);  // initially stationary

	currAccel_ = gsl_vector_alloc(dim_);
	currAngularVel_ = gsl_vector_alloc(dim_);
}

ImuFilterRunner::~ImuFilterRunner()
{
	gsl_vector_free(initialGravity_);  initialGravity_ = NULL;
	//gsl_vector_free(initialAngularVel_);  initialAngularVel_ = NULL;

	gsl_vector_free(accelCalibrationParam_);  accelCalibrationParam_ = NULL;
	gsl_matrix_free(accelCalibrationCovariance_);  accelCalibrationCovariance_ = NULL;
	gsl_vector_free(gyroCalibrationParam_);  gyroCalibrationParam_ = NULL;
	gsl_matrix_free(gyroCalibrationCovariance_);  gyroCalibrationCovariance_ = NULL;

	gsl_vector_free(measuredAccel_);  measuredAccel_ = NULL;
	gsl_vector_free(measuredAngularVel_);  measuredAngularVel_ = NULL;
	gsl_vector_free(calibratedAccel_);  calibratedAccel_ = NULL;
	gsl_vector_free(calibratedAngularVel_);  calibratedAngularVel_ = NULL;

	gsl_vector_free(actualMeasurement_);  actualMeasurement_ = NULL;

	gsl_vector_free(currPos_);  currPos_ = NULL;
	gsl_vector_free(prevPos_);  prevPos_ = NULL;
	gsl_vector_free(currVel_);  currVel_ = NULL;
	gsl_vector_free(prevVel_);  prevVel_ = NULL;
	gsl_vector_free(currAngle_);  currAngle_ = NULL;
	gsl_vector_free(prevAngle_);  prevAngle_ = NULL;

	gsl_vector_free(currAccel_);  currAccel_ = NULL;
	gsl_vector_free(currAngularVel_);  currAngularVel_ = NULL;
}

void ImuFilterRunner::calculateCalibratedAcceleration(const gsl_vector *lg, gsl_vector *a_calibrated) const
{
	const double &b_gx = gsl_vector_get(accelCalibrationParam_, 0);
	const double &b_gy = gsl_vector_get(accelCalibrationParam_, 1);
	const double &b_gz = gsl_vector_get(accelCalibrationParam_, 2);
	const double &s_gx = gsl_vector_get(accelCalibrationParam_, 3);
	const double &s_gy = gsl_vector_get(accelCalibrationParam_, 4);
	const double &s_gz = gsl_vector_get(accelCalibrationParam_, 5);
	const double &theta_gyz = gsl_vector_get(accelCalibrationParam_, 6);
	const double &theta_gzx = gsl_vector_get(accelCalibrationParam_, 7);
	const double &theta_gzy = gsl_vector_get(accelCalibrationParam_, 8);

	const double &l_gx = gsl_vector_get(lg, 0);
	const double &l_gy = gsl_vector_get(lg, 1);
	const double &l_gz = gsl_vector_get(lg, 2);

	const double tan_gyz = std::tan(theta_gyz);
	const double tan_gzx = std::tan(theta_gzx);
	const double tan_gzy = std::tan(theta_gzy);
	const double cos_gyz = std::cos(theta_gyz);
	const double cos_gzx = std::cos(theta_gzx);
	const double cos_gzy = std::cos(theta_gzy);

	const double g_x = (l_gx - b_gx) / (1.0 + s_gx);
	const double g_y = tan_gyz * (l_gx - b_gx) / (1.0 + s_gx) + (l_gy - b_gy) / ((1.0 + s_gy) * cos_gyz);
	const double g_z = (tan_gzx * tan_gyz - tan_gzy / cos_gzx) * (l_gx - b_gx) / (1.0 + s_gx) +
		((l_gy - b_gy) * tan_gzx) / ((1.0 + s_gy) * cos_gyz) + (l_gz - b_gz) / ((1.0 + s_gz) * cos_gzx * cos_gzy);

	gsl_vector_set(a_calibrated, 0, g_x);
	gsl_vector_set(a_calibrated, 1, g_y);
	gsl_vector_set(a_calibrated, 2, g_z);
}

void ImuFilterRunner::calculateCalibratedAngularRate(const gsl_vector *lw, gsl_vector *w_calibrated) const
{
	const double &b_wx = gsl_vector_get(gyroCalibrationParam_, 0);
	const double &b_wy = gsl_vector_get(gyroCalibrationParam_, 1);
	const double &b_wz = gsl_vector_get(gyroCalibrationParam_, 2);

	const double &l_wx = gsl_vector_get(lw, 0);
	const double &l_wy = gsl_vector_get(lw, 1);
	const double &l_wz = gsl_vector_get(lw, 2);

	const double w_x = l_wx - b_wx;
	const double w_y = l_wy - b_wy;
	const double w_z = l_wz - b_wz;

	gsl_vector_set(w_calibrated, 0, w_x);
	gsl_vector_set(w_calibrated, 1, w_y);
	gsl_vector_set(w_calibrated, 2, w_z);
}

bool ImuFilterRunner::loadCalibrationParam(const std::string &filename)
{
	std::ifstream stream(filename.c_str());
	if (!stream)
	{
		std::cout << "file not found !!!" << std::endl;
		return false;
	}

	std::string line_str;
	double val;

	// load acceleration parameters
	if (!stream.eof()) std::getline(stream, line_str);
	else
	{
		std::cout << "file format error !!!" << std::endl;
		return false;
	}

	if (!stream.eof())
	{
		std::getline(stream, line_str);

		std::istringstream sstream(line_str);
		for (size_t i = 0; i < numAccelParam_; ++i)
		{
			sstream >> val;
			gsl_vector_set(accelCalibrationParam_, i, val);
		}
	}
	else
	{
		std::cout << "file format error !!!" << std::endl;
		return false;
	}

	if (!stream.eof()) std::getline(stream, line_str);
	else
	{
		std::cout << "file format error !!!" << std::endl;
		return false;
	}

	// load covariance of acceleration parameters
	if (!stream.eof()) std::getline(stream, line_str);
	else
	{
		std::cout << "file format error !!!" << std::endl;
		return false;
	}

	if (!stream.eof())
	{
		for (size_t i = 0; i < numAccelParam_; ++i)
		{
			std::getline(stream, line_str);

			std::istringstream sstream(line_str);
			for (size_t j = 0; j < numAccelParam_; ++j)
			{
				sstream >> val;
				gsl_matrix_set(accelCalibrationCovariance_, i, j, val);
			}
		}
	}
	else
	{
		std::cout << "file format error !!!" << std::endl;
		return false;
	}

	if (!stream.eof()) std::getline(stream, line_str);
	else
	{
		std::cout << "file format error !!!" << std::endl;
		return false;
	}

	// load gyro parameters
	if (!stream.eof()) std::getline(stream, line_str);
	else
	{
		std::cout << "file format error !!!" << std::endl;
		return false;
	}

	if (!stream.eof())
	{
		std::getline(stream, line_str);

		std::istringstream sstream(line_str);
		for (size_t i = 0; i < numGyroParam_; ++i)
		{
			sstream >> val;
			gsl_vector_set(gyroCalibrationParam_, i, val);
		}
	}
	else
	{
		std::cout << "file format error !!!" << std::endl;
		return false;
	}

	if (!stream.eof()) std::getline(stream, line_str);
	else
	{
		std::cout << "file format error !!!" << std::endl;
		return false;
	}

	// load covariance of gyro parameters
	if (!stream.eof()) std::getline(stream, line_str);
	else
	{
		std::cout << "file format error !!!" << std::endl;
		return false;
	}

	if (!stream.eof())
	{
		for (size_t i = 0; i < numGyroParam_; ++i)
		{
			std::getline(stream, line_str);

			std::istringstream sstream(line_str);
			for (size_t j = 0; j < numGyroParam_; ++j)
			{
				sstream >> val;
				gsl_matrix_set(gyroCalibrationCovariance_, i, j, val);
			}
		}
	}
	else
	{
		std::cout << "file format error !!!" << std::endl;
		return false;
	}

	if (!stream.eof()) std::getline(stream, line_str);
	else
	{
		std::cout << "file format error !!!" << std::endl;
		return false;
	}

	stream.close();

	return true;
}

bool ImuFilterRunner::readAdisData(gsl_vector *accel, gsl_vector *gyro) const
{
	if (!adis_) return false;

	const short rawXGyro = adis_->ReadInt14(ADIS16350_XGYRO_OUT) & 0x3FFF;
	const short rawYGyro = adis_->ReadInt14(ADIS16350_YGYRO_OUT) & 0x3FFF;
	const short rawZGyro = adis_->ReadInt14(ADIS16350_ZGYRO_OUT) & 0x3FFF;
	const short rawXAccel = adis_->ReadInt14(ADIS16350_XACCL_OUT) & 0x3FFF;
	const short rawYAccel = adis_->ReadInt14(ADIS16350_YACCL_OUT) & 0x3FFF;
	const short rawZAccel = adis_->ReadInt14(ADIS16350_ZACCL_OUT) & 0x3FFF;

	gsl_vector_set(accel, 0, ((rawXAccel & 0x2000) == 0x2000 ? (rawXAccel - 0x4000) : rawXAccel) * ADIS16350_ACCL_SCALE_FACTOR);
	gsl_vector_set(accel, 1, ((rawYAccel & 0x2000) == 0x2000 ? (rawYAccel - 0x4000) : rawYAccel) * ADIS16350_ACCL_SCALE_FACTOR);
	gsl_vector_set(accel, 2, ((rawZAccel & 0x2000) == 0x2000 ? (rawZAccel - 0x4000) : rawZAccel) * ADIS16350_ACCL_SCALE_FACTOR);

	gsl_vector_set(gyro, 0, ((rawXGyro & 0x2000) == 0x2000 ? (rawXGyro - 0x4000) : rawXGyro) * ADIS16350_GYRO_SCALE_FACTOR);
	gsl_vector_set(gyro, 1, ((rawYGyro & 0x2000) == 0x2000 ? (rawYGyro - 0x4000) : rawYGyro) * ADIS16350_GYRO_SCALE_FACTOR);
	gsl_vector_set(gyro, 2, ((rawZGyro & 0x2000) == 0x2000 ? (rawZGyro - 0x4000) : rawZGyro) * ADIS16350_GYRO_SCALE_FACTOR);

	return true;
}

void ImuFilterRunner::initializeGravity(const size_t Ninitial)
{
	double accel_x_sum = 0.0, accel_y_sum = 0.0, accel_z_sum = 0.0;
	double gyro_x_sum = 0.0, gyro_y_sum = 0.0, gyro_z_sum = 0.0;

	for (size_t i = 0; i < Ninitial; ++i)
	{
#if defined(__USE_ADIS16350_DATA)
		ImuFilterRunner::readAdisData(measuredAccel_, measuredAngularVel_);
#endif

		calculateCalibratedAcceleration(measuredAccel_, calibratedAccel_);
		//calculateCalibratedAngularRate(measuredAngularVel_, calibratedAngularVel_);

		accel_x_sum += gsl_vector_get(calibratedAccel_, 0);
		accel_y_sum += gsl_vector_get(calibratedAccel_, 1);
		accel_z_sum += gsl_vector_get(calibratedAccel_, 2);
		//gyro_x_sum += gsl_vector_get(calibratedAngularVel_, 0);
		//gyro_y_sum += gsl_vector_get(calibratedAngularVel_, 1);
		//gyro_z_sum += gsl_vector_get(calibratedAngularVel_, 2);
	}

	gsl_vector_set(initialGravity_, 0, accel_x_sum / Ninitial);
	gsl_vector_set(initialGravity_, 1, accel_y_sum / Ninitial);
	gsl_vector_set(initialGravity_, 2, accel_z_sum / Ninitial);
	//gsl_vector_set(initialAngularVel, 0, gyro_x_sum / Ninitial);
	//gsl_vector_set(initialAngularVel, 1, gyro_y_sum / Ninitial);
	//gsl_vector_set(initialAngularVel, 2, gyro_z_sum / Ninitial);
}

void ImuFilterRunner::initializeGravity(const size_t Ninitial, const std::vector<Acceleration> &accels, const std::vector<Gyro> &gyros)
{
	double accel_x_sum = 0.0, accel_y_sum = 0.0, accel_z_sum = 0.0;
	double gyro_x_sum = 0.0, gyro_y_sum = 0.0, gyro_z_sum = 0.0;

	for (size_t i = 0; i < Ninitial; ++i)
	{
		gsl_vector_set(measuredAccel_, 0, accels[i].x);
		gsl_vector_set(measuredAccel_, 1, accels[i].y);
		gsl_vector_set(measuredAccel_, 2, accels[i].z);
		//gsl_vector_set(measuredAngularVel_, 0, gyros[i].x);
		//gsl_vector_set(measuredAngularVel_, 1, gyros[i].y);
		//gsl_vector_set(measuredAngularVel_, 2, gyros[i].z);

		calculateCalibratedAcceleration(measuredAccel_, calibratedAccel_);
		//calculateCalibratedAngularRate(measuredAngularVel_, calibratedAngularVel_);

		accel_x_sum += gsl_vector_get(calibratedAccel_, 0);
		accel_y_sum += gsl_vector_get(calibratedAccel_, 1);
		accel_z_sum += gsl_vector_get(calibratedAccel_, 2);
		//gyro_x_sum += gsl_vector_get(calibratedAngularVel_, 0);
		//gyro_y_sum += gsl_vector_get(calibratedAngularVel_, 1);
		//gyro_z_sum += gsl_vector_get(calibratedAngularVel_, 2);
	}

	gsl_vector_set(initialGravity_, 0, accel_x_sum / Ninitial);
	gsl_vector_set(initialGravity_, 1, accel_y_sum / Ninitial);
	gsl_vector_set(initialGravity_, 2, accel_z_sum / Ninitial);
	//gsl_vector_set(initialAngularVel, 0, gyro_x_sum / Ninitial);
	//gsl_vector_set(initialAngularVel, 1, gyro_y_sum / Ninitial);
	//gsl_vector_set(initialAngularVel, 2, gyro_z_sum / Ninitial);
}

bool ImuFilterRunner::testAdisUsbz(const size_t loopCount)
{
	size_t loop = 0;
	while (loop++ < loopCount)
	{
		if (!readAdisData(measuredAccel_, measuredAngularVel_))
			return false;

		std::cout << gsl_vector_get(measuredAccel_, 0) << ", " << gsl_vector_get(measuredAccel_, 1) << ", " << gsl_vector_get(measuredAccel_, 2) << " ; " <<
			gsl_vector_get(measuredAngularVel_, 0) << ", " << gsl_vector_get(measuredAngularVel_, 1) << ", " << gsl_vector_get(measuredAngularVel_, 2) << std::endl;
	}

	return true;
}

/*static*/ bool ImuFilterRunner::loadSavedImuData(const std::string &filename, std::vector<Acceleration> &accels, std::vector<Gyro> &gyros)
{
	std::ifstream stream(filename.c_str());

	// data format:
	//	Sample #,Time (sec),XgND,X Gryo,YgND,Y Gyro,ZgND,Z Gyro,XaND,X acc,YaND,Y acc,ZaND,Z acc,

	if (!stream.is_open())
	{
		std::cout << "file open error !!!" << std::endl;
		return false;
	}

	// eliminate the 1st 7 lines
	{
		std::string str;
		for (int i = 0; i < 7; ++i)
		{
			if (!stream.eof())
				std::getline(stream, str);
			else
			{
				std::cout << "file format error !!!" << std::endl;
				return false;
			}
		}
	}

	//
	double xAccel, yAccel, zAccel, xGyro, yGyro, zGyro;

	const double deg2rad = boost::math::constants::pi<double>() / 180.0;
	int dummy;
	double dummy1;
	char comma;
	while (!stream.eof())
	{
		stream >> dummy >> comma >> dummy1 >> comma >>
			dummy >> comma >> xGyro >> comma >>
			dummy >> comma >> yGyro >> comma >>
			dummy >> comma >> zGyro >> comma >>
			dummy >> comma >> xAccel >> comma >>
			dummy >> comma >> yAccel >> comma >>
			dummy >> comma >> zAccel >> comma;
		if (stream)
		{
			accels.push_back(Acceleration(xAccel * REF_GRAVITY_, yAccel * REF_GRAVITY_, zAccel * REF_GRAVITY_));  // [m/sec^2]
			gyros.push_back(Gyro(xGyro * deg2rad, yGyro * deg2rad, zGyro * deg2rad));  // [rad/sec]
		}
	}

	if (accels.empty() || gyros.empty())
	{
		std::cout << "data error !!!" << std::endl;
		return false;
	}

	stream.close();

	return true;
}

bool ImuFilterRunner::runImuFilter(swl::DiscreteExtendedKalmanFilter &filter, const size_t step, const gsl_vector *measuredAccel, const gsl_vector *measuredAngularVel)
{
	size_t step2 = step;

	// method #1
	// 1-based time step. 0-th time step is initial

	// 0. initial estimates: x(0) & P(0)

	// 1. time update (prediction): x(k) & P(k)  ==>  x-(k+1) & P-(k+1)
	if (!filter.updateTime(step2, NULL)) return false;

	// save x-(k+1) & P-(k+1)
	{
		const gsl_vector *x_hat = filter.getEstimatedState();
		const gsl_matrix *P = filter.getStateErrorCovarianceMatrix();

		const double Ax = gsl_vector_get(x_hat, 6);
		const double Ay = gsl_vector_get(x_hat, 7);
		const double Az = gsl_vector_get(x_hat, 8);
		const double Wx = gsl_vector_get(x_hat, 13);
		const double Wy = gsl_vector_get(x_hat, 14);
		const double Wz = gsl_vector_get(x_hat, 15);
		//gsl_matrix_get(P, 6, 6);
		//gsl_matrix_get(P, 7, 7);
		//gsl_matrix_get(P, 8, 8);
	}

	//
	calculateCalibratedAcceleration(measuredAccel, calibratedAccel_);
	calculateCalibratedAngularRate(measuredAngularVel, calibratedAngularVel_);

	gsl_vector_set(actualMeasurement_, 0, gsl_vector_get(calibratedAccel_, 0));
	gsl_vector_set(actualMeasurement_, 1, gsl_vector_get(calibratedAccel_, 1));
	gsl_vector_set(actualMeasurement_, 2, gsl_vector_get(calibratedAccel_, 2));
	gsl_vector_set(actualMeasurement_, 3, gsl_vector_get(calibratedAngularVel_, 0));
	gsl_vector_set(actualMeasurement_, 4, gsl_vector_get(calibratedAngularVel_, 1));
	gsl_vector_set(actualMeasurement_, 5, gsl_vector_get(calibratedAngularVel_, 2));

	// advance time step
	++step2;

	// 2. measurement update (correction): x-(k), P-(k) & y_tilde(k)  ==>  K(k), x(k) & P(k)
	if (!filter.updateMeasurement(step2, actualMeasurement_, NULL)) return false;

	// save K(k), x(k) & P(k)
	{
		const gsl_vector *x_hat = filter.getEstimatedState();
		const gsl_matrix *K = filter.getKalmanGain();
		const gsl_matrix *P = filter.getStateErrorCovarianceMatrix();

		const double Ax = gsl_vector_get(x_hat, 6);
		const double Ay = gsl_vector_get(x_hat, 7);
		const double Az = gsl_vector_get(x_hat, 8);
		const double Wx = gsl_vector_get(x_hat, 13);
		const double Wy = gsl_vector_get(x_hat, 14);
		const double Wz = gsl_vector_get(x_hat, 15);
		//gsl_matrix_get(K, 6, 6);
		//gsl_matrix_get(K, 7, 7);
		//gsl_matrix_get(K, 8, 8);
		//gsl_matrix_get(P, 6, 6);
		//gsl_matrix_get(P, 7, 7);
		//gsl_matrix_get(P, 8, 8);

		gsl_vector_set(currAccel_, 0, Ax);
		gsl_vector_set(currAccel_, 1, Ay);
		gsl_vector_set(currAccel_, 2, Az);
		gsl_vector_set(currAngularVel_, 0, Wx);
		gsl_vector_set(currAngularVel_, 1, Wy);
		gsl_vector_set(currAngularVel_, 2, Wz);

		gsl_vector_set(currVel_, 0, gsl_vector_get(prevVel_, 0) + Ax * Ts_);
		gsl_vector_set(currPos_, 0, gsl_vector_get(prevPos_, 0) + gsl_vector_get(prevVel_, 0) * Ts_ + 0.5 * Ax * Ts_*Ts_);
		gsl_vector_set(currVel_, 1, gsl_vector_get(prevVel_, 1) + Ay * Ts_);
		gsl_vector_set(currPos_, 1, gsl_vector_get(prevPos_, 1) + gsl_vector_get(prevVel_, 1) * Ts_ + 0.5 * Ay * Ts_*Ts_);
		gsl_vector_set(currVel_, 2, gsl_vector_get(prevVel_, 2) + Az * Ts_);
		gsl_vector_set(currPos_, 2, gsl_vector_get(prevPos_, 2) + gsl_vector_get(prevVel_, 2) * Ts_ + 0.5 * Az * Ts_*Ts_);
		gsl_vector_set(currAngle_, 0, gsl_vector_get(prevAngle_, 0) + Wx * Ts_);
		gsl_vector_set(currAngle_, 1, gsl_vector_get(prevAngle_, 1) + Wy * Ts_);
		gsl_vector_set(currAngle_, 2, gsl_vector_get(prevAngle_, 2) + Wz * Ts_);

		gsl_vector_memcpy(prevPos_, currPos_);
		gsl_vector_memcpy(prevVel_, currVel_);
		gsl_vector_memcpy(prevAngle_, currAngle_);
	}

	return true;
}

// "Sigma-Point Kalman Filters for Integrated Navigation", R. van der Merwe and Eric A. Wan,
//	Annual Meeting of The Institute of Navigation, 2004
// "A new multi-position calibration method for MEMS inertial navigation systems", Z. F. Syed, P. Aggarwal, C. Goodall, X. Niu, and N. El-Sheimy,
//	Measurement Science and Technology, vol. 18, pp. 1897-1907, 2007

void imu_filter_with_calibration()
{
	const size_t stateDim = 22;
	const size_t inputDim = 3;
	const size_t outputDim = 6;

	// sampling interval
	//const double Ts = 0.01;  // [sec]
	const double Ts = 0.016 / 5;  // [sec]

	const size_t Ninitial = 10000;

	//
#if defined(__USE_ADIS16350_DATA)
	AdisUsbz adis;

#if defined(UNICODE) || defined(_UNICODE)
	if (!adis.Initialize(L"\\\\.\\Ezusb-0"))
#else
	if (!adis.Initialize("\\\\.\\Ezusb-0"))
#endif
	{
		std::cout << "fail to initialize ADISUSBZ" << std::endl;
		return;
	}

	ImuFilterRunner runner(Ts, stateDim, inputDim, outputDim, &adis);
#else
	ImuFilterRunner runner(Ts, stateDim, inputDim, outputDim, NULL);
#endif

	// load calibration parameters
	std::cout << "load calibration parameters ..." << std::endl;
	const std::string calibration_filename("..\\data\\adis16350_data_20100801\\imu_calibration_result.txt");
	runner.loadCalibrationParam(calibration_filename);

#if defined(__USE_ADIS16350_DATA)
	// test ADISUSBZ
	//ImuFilterRunner::testAdisUsbz(Ntest);

	std::cout << "set an initial gravity ..." << std::endl;
	// set an initial gravity
	runner.initializeGravity(Ninitial);
#else
	std::vector<ImuFilterRunner::Acceleration> accels;
	std::vector<ImuFilterRunner::Gyro> gyros;
	accels.reserve(Ninitial);
	gyros.reserve(Ninitial);

	// load validation data
	ImuFilterRunner::loadSavedImuData("..\\data\\adis16350_data_20100801\\01_z_pos.csv", accels, gyros);
	//ImuFilterRunner::loadSavedImuData("..\\data\\adis16350_data_20100801\\02_z_neg.csv", accels, gyros);

	std::cout << "set an initial gravity ..." << std::endl;
	// set an initial gravity
	runner.initializeGravity(Ninitial, accels, gyros);
#endif

	const gsl_vector *initialGravity = runner.getInitialGravity();

	//
	gsl_vector *x0 = gsl_vector_alloc(stateDim);
	gsl_vector_set_zero(x0);
	gsl_vector_set(x0, 6, -gsl_vector_get(initialGravity, 0));  // a_p = g_initial_x
	gsl_vector_set(x0, 7, -gsl_vector_get(initialGravity, 1));  // a_q = g_initial_y
	gsl_vector_set(x0, 8, -gsl_vector_get(initialGravity, 2));  // a_r = g_initial_z
	gsl_vector_set(x0, 9, 1.0);  // e0 = 1.0
	gsl_matrix *P0 = gsl_matrix_alloc(stateDim, stateDim);
	gsl_matrix_set_identity(P0);
	gsl_matrix_scale(P0, 1.0e-8);  // the initial estimate is completely unknown

	// Qd = W * Q * W^T where W = I
	//	the EKF approximation of Qd will be W * [ Q * Ts ] * W^T
	gsl_matrix *Qd = gsl_matrix_alloc(stateDim, stateDim);
	gsl_matrix_set_zero(Qd);
	// FIXME [modify] >>
	const double QQ = 1.0e-8;
	gsl_matrix_set(Qd, 0, 0, QQ);
	gsl_matrix_set(Qd, 1, 1, QQ);
	gsl_matrix_set(Qd, 2, 2, QQ);
	gsl_matrix_set(Qd, 3, 3, QQ);
	gsl_matrix_set(Qd, 4, 4, QQ);
	gsl_matrix_set(Qd, 5, 5, QQ);
	gsl_matrix_set(Qd, 6, 6, QQ);
	gsl_matrix_set(Qd, 7, 7, QQ);
	gsl_matrix_set(Qd, 8, 8, QQ);
	gsl_matrix_set(Qd, 9, 9, QQ);
	gsl_matrix_set(Qd, 10, 10, QQ);
	gsl_matrix_set(Qd, 11, 11, QQ);
	gsl_matrix_set(Qd, 12, 12, QQ);
	gsl_matrix_set(Qd, 13, 13, QQ);
	gsl_matrix_set(Qd, 14, 14, QQ);
	gsl_matrix_set(Qd, 15, 15, QQ);
	gsl_matrix_set(Qd, 16, 16, QQ);
	gsl_matrix_set(Qd, 17, 17, QQ);
	gsl_matrix_set(Qd, 18, 18, QQ);
	gsl_matrix_set(Qd, 19, 19, QQ);
	gsl_matrix_set(Qd, 20, 20, QQ);
	gsl_matrix_set(Qd, 21, 21, QQ);
	gsl_matrix_scale(Qd, Ts);

	// Rd = V * R * V^T where V = I
	gsl_matrix *Rd = gsl_matrix_alloc(outputDim, outputDim);
	gsl_matrix_set_zero(Rd);
	// FIXME [modify] >>
	const double RR = 1.0e-8;
	gsl_matrix_set(Rd, 0, 0, RR);
	gsl_matrix_set(Rd, 1, 1, RR);
	gsl_matrix_set(Rd, 2, 2, RR);
	gsl_matrix_set(Rd, 3, 3, RR);
	gsl_matrix_set(Rd, 4, 4, RR);
	gsl_matrix_set(Rd, 5, 5, RR);

	const ImuSystem system(Ts, stateDim, inputDim, outputDim, initialGravity, Qd, Rd);
	swl::DiscreteExtendedKalmanFilter filter(system, x0, P0);

	gsl_vector_free(x0);  x0 = NULL;
	gsl_matrix_free(P0);  P0 = NULL;
	gsl_matrix_free(Qd);  Qd = NULL;
	gsl_matrix_free(Rd);  Rd = NULL;

	// extended Kalman filtering
	std::cout << "start extended Kalman filtering ..." << std::endl;

#if defined(__USE_ADIS16350_DATA)
	const size_t Nstep = 10000;
#else
	const size_t Nstep = Ninitial;
#endif

	gsl_vector *measuredAccel = gsl_vector_alloc(3);
	gsl_vector *measuredAngularVel = gsl_vector_alloc(3);

	size_t step = 0;
	while (step < Nstep)
	{
#if defined(__USE_ADIS16350_DATA)
		runner.readAdisData(measuredAccel, measuredAngularVel);
#else
		gsl_vector_set(measuredAccel, 0, accels[step].x);
		gsl_vector_set(measuredAccel, 1, accels[step].y);
		gsl_vector_set(measuredAccel, 2, accels[step].z);
		gsl_vector_set(measuredAngularVel, 0, gyros[step].x);
		gsl_vector_set(measuredAngularVel, 1, gyros[step].y);
		gsl_vector_set(measuredAngularVel, 2, gyros[step].z);
#endif

		if (!runner.runImuFilter(filter, step, measuredAccel, measuredAngularVel))
		{
			std::cout << "IMU filtering error !!!" << std::endl;
			return;
		}

		const gsl_vector *pos = runner.getFilteredPos();
		const gsl_vector *vel = runner.getFilteredVel();
		const gsl_vector *accel = runner.getFilteredAccel();
		const gsl_vector *ang = runner.getFilteredAngle();
		const gsl_vector *angVel = runner.getFilteredAngularVel();

		std::cout << (step + 1) << " : " << gsl_vector_get(pos, 0) << ", " << gsl_vector_get(pos, 1) << ", " << gsl_vector_get(pos, 2) << " ; " <<
			gsl_vector_get(ang, 0) << ", " << gsl_vector_get(ang, 1) << ", " << gsl_vector_get(ang, 2) << std::endl;

		++step;
	}

	gsl_vector_free(measuredAccel);  measuredAccel = NULL;
	gsl_vector_free(measuredAngularVel);  measuredAngularVel = NULL;
}
