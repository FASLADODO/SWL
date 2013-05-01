//#include "stdafx.h"
#include "swl/Config.h"
#include "swl/rnd_util/VonMisesMixtureModel.h"
#include <boost/smart_ptr.hpp>
#include <fstream>
#include <iostream>


#if defined(_DEBUG) && defined(__SWL_CONFIG__USE_DEBUG_NEW)
#include "swl/ResourceLeakageCheck.h"
#define new DEBUG_NEW
#endif


//#define __TEST_HMM_MODEL 1
#define __TEST_HMM_MODEL 2
#define __USE_SPECIFIED_VALUE_FOR_RANDOM_SEED 1


namespace {
namespace local {

void model_reading_and_writing()
{
	// reading a model
	{
		boost::scoped_ptr<swl::ContinuousDensityMixtureModel> cdmm;

#if __TEST_HMM_MODEL == 1
		const size_t K = 3;  // the number of mixture components
		//const size_t D = 1;  // the dimension of observation symbols

		//
		std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test1.cdmm");
#elif __TEST_HMM_MODEL == 2
		const size_t K = 3;  // the number of mixture components
		//const size_t D = 1;  // the dimension of observation symbols

		//
		std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test2.cdmm");
#endif
		if (!stream)
		{
			std::ostringstream stream;
			stream << "file not found at " << __LINE__ << " in " << __FILE__;
			throw std::runtime_error(stream.str().c_str());
			return;
		}

		cdmm.reset(new swl::VonMisesMixtureModel(K));

		const bool retval = cdmm->readModel(stream);
		if (!retval)
		{
			std::ostringstream stream;
			stream << "model reading error at " << __LINE__ << " in " << __FILE__;
			throw std::runtime_error(stream.str().c_str());
			return;
		}

		// normalize pi
		cdmm->normalizeModelParameters();

		cdmm->writeModel(std::cout);
	}

	// writing a model
	{
		boost::scoped_ptr<swl::ContinuousDensityMixtureModel> cdmm;

#if __TEST_HMM_MODEL == 1
		const size_t K = 3;  // the number of mixture components
		//const size_t D = 1;  // the dimension of observation symbols

		const double arrPi[] = {
			0.30, 0.40, 0.30
		};
		const double arrMu[] = {
			0.0, 3.0, 5.0
		};
		const double arrKappa[] = {
			50.0, 100.0, 85.0
		};

		//
		std::ofstream stream("..\\data\\mixture_model\\von_mises_mixture_test1_writing.cdmm");
#elif __TEST_HMM_MODEL == 2
		const size_t K = 3;  // the number of mixture components
		//const size_t D = 1;  // the dimension of observation symbols

		const double arrPi[] = {
			0.20, 0.45, 0.35
		};
		const double arrMu[] = {
			1.2, 3.2, 5.2
		};
		const double arrKappa[] = {
			40.0, 55.0, 80.0
		};

		//
		std::ofstream stream("..\\data\\mixture_model\\von_mises_mixture_test2_writing.cdmm");
#endif
		if (!stream)
		{
			std::ostringstream stream;
			stream << "file not found at " << __LINE__ << " in " << __FILE__;
			throw std::runtime_error(stream.str().c_str());
			return;
		}

		std::vector<double> pi(arrPi, arrPi + K);
		swl::VonMisesMixtureModel::dvector_type mus(boost::numeric::ublas::vector<double, std::vector<double> >(K, std::vector<double>(arrMu, arrMu + K)));
		swl::VonMisesMixtureModel::dvector_type kappas(boost::numeric::ublas::vector<double, std::vector<double> >(K, std::vector<double>(arrKappa, arrKappa + K)));
		cdmm.reset(new swl::VonMisesMixtureModel(K, pi, mus, kappas));

		const bool retval = cdmm->writeModel(stream);
		if (!retval)
		{
			std::ostringstream stream;
			stream << "model writing error at " << __LINE__ << " in " << __FILE__;
			throw std::runtime_error(stream.str().c_str());
			return;
		}
	}
}

void observation_sequence_generation(const bool outputToFile)
{
	boost::scoped_ptr<swl::ContinuousDensityMixtureModel> cdmm;

	// read a model
	{
#if __TEST_HMM_MODEL == 1
		const size_t K = 3;  // the number of mixture components
		//const size_t D = 1;  // the dimension of observation symbols

		//
		std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test1.cdmm");
#elif __TEST_HMM_MODEL == 2
		const size_t K = 3;  // the number of mixture components
		//const size_t D = 1;  // the dimension of observation symbols

		//
		std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test2.cdmm");
#endif
		if (!stream)
		{
			std::ostringstream stream;
			stream << "file not found at " << __LINE__ << " in " << __FILE__;
			throw std::runtime_error(stream.str().c_str());
			return;
		}

		cdmm.reset(new swl::VonMisesMixtureModel(K));

		const bool retval = cdmm->readModel(stream);
		if (!retval)
		{
			std::ostringstream stream;
			stream << "model reading error at " << __LINE__ << " in " << __FILE__;
			throw std::runtime_error(stream.str().c_str());
			return;
		}

		// normalize pi
		cdmm->normalizeModelParameters();

		//cdmm->writeModel(std::cout);
	}

	// generate a sample sequence
	{
#if defined(__USE_SPECIFIED_VALUE_FOR_RANDOM_SEED)
		const unsigned int seed = 34586u;
#else
		const unsigned int seed = (unsigned int)std::time(NULL);
#endif
		std::srand(seed);
		std::cout << "random seed: " << seed << std::endl;

		if (outputToFile)
		{
#if __TEST_HMM_MODEL == 1

#if 1
			const size_t N = 50;
			std::ofstream stream("..\\data\\mixture_model\\von_mises_mixture_test1_50.seq");
#elif 0
			const size_t N = 100;
			std::ofstream stream("..\\data\\mixture_model\\von_mises_mixture_test1_100.seq");
#elif 0
			const size_t N = 1500;
			std::ofstream stream("..\\data\\mixture_model\\von_mises_mixture_test1_1500.seq");
#endif

#elif __TEST_HMM_MODEL == 2

#if 1
			const size_t N = 50;
			std::ofstream stream("..\\data\\mixture_model\\von_mises_mixture_test2_50.seq");
#elif 0
			const size_t N = 100;
			std::ofstream stream("..\\data\\mixture_model\\von_mises_mixture_test2_100.seq");
#elif 0
			const size_t N = 1500;
			std::ofstream stream("..\\data\\mixture_model\\von_mises_mixture_test2_1500.seq");
#endif

#endif
			if (!stream)
			{
				std::ostringstream stream;
				stream << "file not found at " << __LINE__ << " in " << __FILE__;
				throw std::runtime_error(stream.str().c_str());
				return;
			}

			swl::ContinuousDensityMixtureModel::dmatrix_type observations(N, cdmm->getObservationDim(), 0.0);
			std::vector<unsigned int> states(N, (unsigned int)-1);
			cdmm->generateSample(N, observations, states, seed);

#if 0
			// output states
			for (size_t n = 0; n < N; ++n)
				std::cout << states[n] << ' ';
			std::cout << std::endl;
#endif

			// write a sample sequence
			swl::ContinuousDensityMixtureModel::writeSequence(stream, observations);
		}
		else
		{
			const size_t N = 100;

			swl::ContinuousDensityMixtureModel::dmatrix_type observations(N, cdmm->getObservationDim(), 0.0);
			std::vector<unsigned int> states(N, (unsigned int)-1);
			cdmm->generateSample(N, observations, states, seed);

#if 0
			// output states
			for (size_t n = 0; n < N; ++n)
				std::cout << states[n] << ' ';
			std::cout << std::endl;
#endif

			// write a sample sequence
			swl::ContinuousDensityMixtureModel::writeSequence(std::cout, observations);
		}
	}
}

void observation_sequence_reading_and_writing()
{
	swl::ContinuousDensityMixtureModel::dmatrix_type observations;
	size_t N = 0;  // length of observation sequence, N

#if __TEST_HMM_MODEL == 1

#if 1
	std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test1_50.seq");
#elif 0
	std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test1_100.seq");
#elif 0
	std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test1_1500.seq");
#else
	std::istream stream = std::cin;
#endif

#elif __TEST_HMM_MODEL == 2

#if 1
	std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test2_50.seq");
#elif 0
	std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test2_100.seq");
#elif 0
	std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test2_1500.seq");
#else
	std::istream stream = std::cin;
#endif

#endif
	if (!stream)
	{
		std::ostringstream stream;
		stream << "file not found at " << __LINE__ << " in " << __FILE__;
		throw std::runtime_error(stream.str().c_str());
		return;
	}

	// read a observation sequence
	size_t D = 0;
	const bool retval = swl::ContinuousDensityMixtureModel::readSequence(stream, N, D, observations);
	if (!retval)
	{
		std::ostringstream stream;
		stream << "sample sequence reading error at " << __LINE__ << " in " << __FILE__;
		throw std::runtime_error(stream.str().c_str());
		return;
	}

	// write a observation sequence
	swl::ContinuousDensityMixtureModel::writeSequence(std::cout, observations);
}

void em_learning_by_mle()
{
	boost::scoped_ptr<swl::ContinuousDensityMixtureModel> cdmm;

/*
	you can initialize the hmm model three ways:
		1) with a model, which also sets the number of states N and number of symbols M.
		2) with a random model by just specifyin N and M.
		3) with a specific random model by specifying N, M and seed.
*/

	// initialize a model
	const int initialization_mode = 1;
	if (1 == initialization_mode)
	{
#if __TEST_HMM_MODEL == 1
		const size_t K = 3;  // the number of mixture components
		//const size_t D = 1;  // the dimension of observation symbols

		//
		std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test1.cdmm");
#elif __TEST_HMM_MODEL == 2
		const size_t K = 3;  // the number of mixture components
		//const size_t D = 1;  // the dimension of observation symbols

		//
		std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test2.cdmm");
#endif
		if (!stream)
		{
			std::ostringstream stream;
			stream << "file not found at " << __LINE__ << " in " << __FILE__;
			throw std::runtime_error(stream.str().c_str());
			return;
		}

		cdmm.reset(new swl::VonMisesMixtureModel(K));

		const bool retval = cdmm->readModel(stream);
		if (!retval)
		{
			std::ostringstream stream;
			stream << "model reading error at " << __LINE__ << " in " << __FILE__;
			throw std::runtime_error(stream.str().c_str());
			return;
		}

		// normalize pi
		cdmm->normalizeModelParameters();

		//cdmm->writeModel(std::cout);
	}
	else if (2 == initialization_mode)
	{
#if defined(__USE_SPECIFIED_VALUE_FOR_RANDOM_SEED)
		const unsigned int seed = 34586u;
#else
		const unsigned int seed = (unsigned int)std::time(NULL);
#endif
		std::srand(seed);
		std::cout << "random seed: " << seed << std::endl;

		const size_t K = 3;  // the number of mixture components
		//const size_t D = 1;  // the dimension of observation symbols

		cdmm.reset(new swl::VonMisesMixtureModel(K));

		// the total number of parameters of observation density = K * D * 2
		std::vector<double> lowerBounds, upperBounds;
		const size_t numParameters = K * 1 * 2;
		lowerBounds.reserve(numParameters);
		upperBounds.reserve(numParameters);
		// means
		for (size_t i = 0; i < K; ++i)
		{
			lowerBounds.push_back(-10000.0);
			upperBounds.push_back(10000.0);
		}
		// standard deviations: sigma > 0
		const double small = 1.0e-10;
		for (size_t i = K; i < numParameters; ++i)
		{
			lowerBounds.push_back(small);
			upperBounds.push_back(10000.0);
		}
		cdmm->initializeModel(lowerBounds, upperBounds);
	}
	else
		throw std::runtime_error("incorrect initialization mode");

	//
	{
		// read a observation sequence
		swl::ContinuousDensityMixtureModel::dmatrix_type observations;
		size_t N = 0;  // length of observation sequence, N
		{
#if __TEST_HMM_MODEL == 1

#if 0
			std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test1_50.seq");
#elif 0
			std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test1_100.seq");
#elif 1
			std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test1_1500.seq");
#else
			std::istream stream = std::cin;
#endif

#elif __TEST_HMM_MODEL == 2

#if 0
			std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test2_50.seq");
#elif 0
			std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test2_100.seq");
#elif 1
			std::ifstream stream("..\\data\\mixture_model\\von_mises_mixture_test2_1500.seq");
#else
			std::istream stream = std::cin;
#endif

#endif
			if (!stream)
			{
				std::ostringstream stream;
				stream << "file not found at " << __LINE__ << " in " << __FILE__;
				throw std::runtime_error(stream.str().c_str());
				return;
			}

			size_t D = 0;
			const bool retval = swl::ContinuousDensityMixtureModel::readSequence(stream, N, D, observations);
			if (!retval || cdmm->getObservationDim() != D)
			{
				std::ostringstream stream;
				stream << "sample sequence reading error at " << __LINE__ << " in " << __FILE__;
				throw std::runtime_error(stream.str().c_str());
				return;
			}
		}

		// EM algorithm
		{
			const double terminationTolerance = 0.001;
			const size_t maxIteration = 1000;
			size_t numIteration = (size_t)-1;
			double initLogProbability = 0.0, finalLogProbability = 0.0;
			cdmm->estimateParametersByML(N, observations, terminationTolerance, maxIteration, numIteration, initLogProbability, finalLogProbability);

			// normalize pi
			//cdmm->normalizeModelParameters();

			//
			std::cout << "------------------------------------" << std::endl;
			std::cout << "EM algorithm" << std::endl;
			std::cout << "\tnumber of iterations = " << numIteration << std::endl;
			std::cout << "\tlog prob(observations | initial model) = " << std::scientific << initLogProbability << std::endl;
			std::cout << "\tlog prob(observations | estimated model) = " << std::scientific << finalLogProbability << std::endl;
			std::cout << "\testimated model:" << std::endl;
			cdmm->writeModel(std::cout);
		}
	}
}

}  // namespace local
}  // unnamed namespace

void von_mises_mixture_model()
{
	std::cout << "===== von Mises mixture model =====" << std::endl;

	//local::model_reading_and_writing();
	//const bool outputToFile = false;
	//local::observation_sequence_generation(outputToFile);
	//local::observation_sequence_reading_and_writing();

	local::em_learning_by_mle();
}