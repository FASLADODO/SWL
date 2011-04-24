#include "HistogramAccumulator.h"
#include "HistogramMatcher.h"


namespace swl {

//-----------------------------------------------------------------------------
//

HistogramAccumulator::HistogramAccumulator(const size_t histogramNum)
: histogramNum_(histogramNum), weights_(), histograms_(histogramNum_)
{
}

HistogramAccumulator::HistogramAccumulator(const std::vector<float> &weights)
: histogramNum_(weights.size()), weights_(weights), histograms_(histogramNum_)
{
	if (weights_.size() != histogramNum_)
		throw std::runtime_error("the size of the 2nd parameter is mismatched");
}

cv::MatND HistogramAccumulator::createAccumulatedHistogram() const
{
	cv::MatND accumulatedHistogram;

	if (weights_.empty())
	{
		// simple running averaging
		for (boost::circular_buffer<const cv::MatND>::const_iterator it = histograms_.begin(); it != histograms_.end(); ++it)
		{
			if (accumulatedHistogram.empty()) accumulatedHistogram = *it;
			else accumulatedHistogram += *it;
		}
	}
	else
	{
		// weighted averaging
		size_t step = 0;
		for (boost::circular_buffer<const cv::MatND>::const_reverse_iterator rit = histograms_.rbegin(); rit != histograms_.rend(); ++rit, ++step)
		{
			if (accumulatedHistogram.empty()) accumulatedHistogram = (*rit) * weights_[step];
			else accumulatedHistogram += (*rit) * weights_[step];
		}
	}

	return accumulatedHistogram;
}

cv::MatND HistogramAccumulator::createTemporalHistogram() const
{
	if (histograms_.empty()) return cv::MatND();

	cv::MatND temporalHistogram(cv::Mat::zeros(histograms_.front().rows, histograms_.size(), histograms_.front().type()));

	size_t k = 0;
	for (boost::circular_buffer<const cv::MatND>::const_iterator it = histograms_.begin(); it != histograms_.end(); ++it, ++k)
	{
		//temporalHistogram.col(k) = *it;  // not working
		it->copyTo(temporalHistogram.col(k));
	}

	return temporalHistogram;
}

cv::MatND HistogramAccumulator::createTemporalHistogram(const std::vector<const cv::MatND> &refHistograms, const double histDistThreshold) const
{
	if (histograms_.empty() || refHistograms.empty()) return cv::MatND();

	cv::MatND temporalHistogram(cv::Mat::zeros(histograms_.front().rows, histograms_.size(), histograms_.front().type()));

	size_t k = 0;
	for (boost::circular_buffer<const cv::MatND>::const_iterator it = histograms_.begin(); it != histograms_.end(); ++it, ++k)
	{
		// match histogram
		double minHistDist = std::numeric_limits<double>::max();
		const size_t &matchedIdx = HistogramMatcher::match(refHistograms, *it, minHistDist);

		if (minHistDist < histDistThreshold)
		{
			//temporalHistogram.col(k) = refHistograms[matchedIdx];  // not working
			refHistograms[matchedIdx].copyTo(temporalHistogram.col(k));
		}
	}

	return temporalHistogram;
}

}  // namespace swl