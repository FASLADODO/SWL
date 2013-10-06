#if !defined(__SWL_RND_UTIL__HMM_WITH_UNIVARIATE_NORMAL_OBSERVATIONS__H_)
#define __SWL_RND_UTIL__HMM_WITH_UNIVARIATE_NORMAL_OBSERVATIONS__H_ 1


#include "swl/rnd_util/CDHMM.h"
#include <boost/random/linear_congruential.hpp>


namespace swl {

//--------------------------------------------------------------------------
// continuous density HMM with univariate normal observation densities

class SWL_RND_UTIL_API HmmWithUnivariateNormalObservations: public CDHMM
{
public:
	typedef CDHMM base_type;

public:
	HmmWithUnivariateNormalObservations(const size_t K);  // for ML learning.
	HmmWithUnivariateNormalObservations(const size_t K, const dvector_type &pi, const dmatrix_type &A, const dvector_type &mus, const dvector_type &sigmas);
	HmmWithUnivariateNormalObservations(const size_t K, const dvector_type *pi_conj, const dmatrix_type *A_conj, const dvector_type *mus_conj, const dvector_type *betas_conj, const dvector_type *sigmas_conj, const dvector_type *nus_conj);  // for MAP learning using conjugate prior.
	virtual ~HmmWithUnivariateNormalObservations();

private:
	HmmWithUnivariateNormalObservations(const HmmWithUnivariateNormalObservations &rhs);  // not implemented
	HmmWithUnivariateNormalObservations & operator=(const HmmWithUnivariateNormalObservations &rhs);  // not implemented

public:
	//
	dvector_type & getMean()  {  return mus_;  }
	const dvector_type & getMean() const  {  return mus_;  }
	dvector_type & getStandardDeviation()  {  return  sigmas_;  }
	const dvector_type & getStandardDeviation() const  {  return  sigmas_;  }

protected:
	// if state == 0, hidden state = [ 1 0 0 ... 0 0 ]
	// if state == 1, hidden state = [ 0 1 0 ... 0 0 ]
	// ...
	// if state == N-1, hidden state = [ 0 0 0 ... 0 1 ]
	/*virtual*/ double doEvaluateEmissionProbability(const unsigned int state, const boost::numeric::ublas::matrix_row<const dmatrix_type> &observation) const;
	// if seed != -1, the seed value is set
	/*virtual*/ void doGenerateObservationsSymbol(const unsigned int state, boost::numeric::ublas::matrix_row<dmatrix_type> &observation, const unsigned int seed = (unsigned int)-1) const;

	// for a single independent observation sequence
	/*virtual*/ void doEstimateObservationDensityParametersByML(const size_t N, const unsigned int state, const dmatrix_type &observations, dmatrix_type &gamma, const double denominatorA);
	// for multiple independent observation sequences
	/*virtual*/ void doEstimateObservationDensityParametersByML(const std::vector<size_t> &Ns, const unsigned int state, const std::vector<dmatrix_type> &observationSequences, const std::vector<dmatrix_type> &gammas, const size_t R, const double denominatorA);

	// for a single independent observation sequence
	/*virtual*/ void doEstimateObservationDensityParametersByMAP(const size_t N, const unsigned int state, const dmatrix_type &observations, dmatrix_type &gamma, const double denominatorA);
	// for multiple independent observation sequences
	/*virtual*/ void doEstimateObservationDensityParametersByMAP(const std::vector<size_t> &Ns, const unsigned int state, const std::vector<dmatrix_type> &observationSequences, const std::vector<dmatrix_type> &gammas, const size_t R, const double denominatorA);

	//
	/*virtual*/ bool doReadObservationDensity(std::istream &stream);
	/*virtual*/ bool doWriteObservationDensity(std::ostream &stream) const;
	/*virtual*/ void doInitializeObservationDensity(const std::vector<double> &lowerBoundsOfObservationDensity, const std::vector<double> &upperBoundsOfObservationDensity);
	/*virtual*/ void doNormalizeObservationDensityParameters()
	{
		// do nothing
	}

	/*virtual*/ bool doDoHyperparametersOfConjugatePriorExist() const
	{
		return base_type::doDoHyperparametersOfConjugatePriorExist() &&
			NULL != mus_conj_.get() && NULL != betas_conj_.get() && NULL != sigmas_conj_.get() && NULL != nus_conj_.get();
	}

private:
	dvector_type mus_;  // the means of the univariate normal distribution.
	dvector_type sigmas_;  // the standard deviations of the univariate normal distribution.

	// hyperparameters for the conjugate prior.
	//	[ref] "EM Algorithm 3 - THE EM Algorithm for MAP Estimates of HMM", personal note.
	//	[ref] "Pattern Recognition and Machine Learning", C. M. Bishop, Springer, 2006.
	boost::scoped_ptr<const dvector_type> mus_conj_;  // m.
	boost::scoped_ptr<const dvector_type> betas_conj_;  // beta. beta > 0.
	boost::scoped_ptr<const dvector_type> sigmas_conj_;  // inv(W).
	boost::scoped_ptr<const dvector_type> nus_conj_;  // nu. nu > D - 1.

	typedef boost::minstd_rand base_generator_type;
	mutable base_generator_type baseGenerator_;
};

}  // namespace swl


#endif  // __SWL_RND_UTIL__HMM_WITH_UNIVARIATE_NORMAL_OBSERVATIONS__H_
