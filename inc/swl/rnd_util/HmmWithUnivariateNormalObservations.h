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
	typedef boost::numeric::ublas::vector<double> dvector_type;
	typedef boost::numeric::ublas::matrix<double> dmatrix_type;
	typedef boost::numeric::ublas::vector<unsigned int> uivector_type;
	typedef boost::numeric::ublas::matrix<unsigned int> uimatrix_type;

public:
	HmmWithUnivariateNormalObservations(const size_t K);
	HmmWithUnivariateNormalObservations(const size_t K, const dvector_type &pi, const dmatrix_type &A, const dvector_type &mus, const dvector_type &sigmas);
	virtual ~HmmWithUnivariateNormalObservations();

private:
	HmmWithUnivariateNormalObservations(const HmmWithUnivariateNormalObservations &rhs);
	HmmWithUnivariateNormalObservations & operator=(const HmmWithUnivariateNormalObservations &rhs);

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
	/*virtual*/ void doEstimateObservationDensityParametersInMStep(const size_t N, const unsigned int state, const dmatrix_type &observations, dmatrix_type &gamma, const double denominatorA);
	// for multiple independent observation sequences
	/*virtual*/ void doEstimateObservationDensityParametersInMStep(const std::vector<size_t> &Ns, const unsigned int state, const std::vector<dmatrix_type> &observationSequences, const std::vector<dmatrix_type> &gammas, const size_t R, const double denominatorA);

	//
	/*virtual*/ bool doReadObservationDensity(std::istream &stream);
	/*virtual*/ bool doWriteObservationDensity(std::ostream &stream) const;
	/*virtual*/ void doInitializeObservationDensity();
	/*virtual*/ void doNormalizeObservationDensityParameters()
	{
		// do nothing
	}

private:
	dvector_type mus_;  // the means of the univariate normal distribution
	dvector_type sigmas_;  // the standard deviations of the univariate normal distribution

	typedef boost::minstd_rand base_generator_type;
	mutable base_generator_type baseGenerator_;
};

}  // namespace swl


#endif  // __SWL_RND_UTIL__HMM_WITH_UNIVARIATE_NORMAL_OBSERVATIONS__H_
