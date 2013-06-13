//#include "stdafx.h"
#if defined(WIN32)
#include <vld/vld.h>
#endif
#include "swl/machine_vision/KinectSensor.h"
#include "gslic_lib/FastImgSeg.h"
#define CV_NO_BACKWARD_COMPATIBILITY
#include <opencv2/opencv.hpp>
#include <boost/smart_ptr.hpp>
#include <algorithm>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstdlib>


#define __USE_RECTIFIED_IMAGE 1

namespace {
namespace local {

// [ref] canny() in ${CPP_RND_HOME}/test/machine_vision/opencv/opencv_edge_detection.cpp
void canny(const cv::Mat &gray, cv::Mat &edge)
{
#if 0
	// down-scale and up-scale the image to filter out the noise
	cv::Mat blurred;
	cv::pyrDown(gray, blurred);
	cv::pyrUp(blurred, edge);
#else
	cv::blur(gray, edge, cv::Size(3, 3));
#endif

	// run the edge detector on grayscale
	const int lowerEdgeThreshold = 5, upperEdgeThreshold = 50;
	const bool useL2 = true;
	cv::Canny(edge, edge, lowerEdgeThreshold, upperEdgeThreshold, 3, useL2);
}

// [ref] load_kinect_sensor_parameters_from_IR_to_RGB() in ${CPP_RND_HOME}/test/machine_vision/opencv/opencv_image_rectification.cpp
void load_kinect_sensor_parameters_from_IR_to_RGB(
	cv::Mat &K_ir, cv::Mat &distCoeffs_ir, cv::Mat &K_rgb, cv::Mat &distCoeffs_rgb,
	cv::Mat &R_ir_to_rgb, cv::Mat &T_ir_to_rgb
)
{
	// [ref]
	//	Camera Calibration Toolbox for Matlab: http://www.vision.caltech.edu/bouguetj/calib_doc/
	//	http://docs.opencv.org/doc/tutorials/calib3d/camera_calibration/camera_calibration.html

	// Caution:
	//	In order to use the calibration results from Camera Calibration Toolbox for Matlab in OpenCV,
	//	a parameter for radial distrtortion, kc(5) has to be active, est_dist(5) = 1.

	// IR (left) to RGB (right)
#if 1
	// the 5th distortion parameter, kc(5) is activated.

	const double fc_ir[] = { 5.865281297534211e+02, 5.866623900166177e+02 };  // [pixel]
	const double cc_ir[] = { 3.371860463542209e+02, 2.485298169373497e+02 };  // [pixel]
	const double alpha_c_ir = 0.0;
	//const double kc_ir[] = { -1.227084070414958e-01, 5.027511830344261e-01, -2.562850607972214e-03, 6.916249031489476e-03, -5.507709925923052e-01 };  // 5x1 vector
	const double kc_ir[] = { -1.227084070414958e-01, 5.027511830344261e-01, -2.562850607972214e-03, 6.916249031489476e-03, -5.507709925923052e-01, 0.0, 0.0, 0.0 };  // 8x1 vector

	const double fc_rgb[] = { 5.248648751941851e+02, 5.268281060449414e+02 };  // [pixel]
	const double cc_rgb[] = { 3.267484107269922e+02, 2.618261807606497e+02 };  // [pixel]
	const double alpha_c_rgb = 0.0;
	//const double kc_rgb[] = { 2.796770514235670e-01, -1.112507253647945e+00, 9.265501548915561e-04, 2.428229310663184e-03, 1.744019737212440e+00 };  // 5x1 vector
	const double kc_rgb[] = { 2.796770514235670e-01, -1.112507253647945e+00, 9.265501548915561e-04, 2.428229310663184e-03, 1.744019737212440e+00, 0.0, 0.0, 0.0 };  // 8x1 vector

	const double rotVec[] = { -1.936270295074452e-03, 1.331596538715070e-02, 3.404073398703758e-03 };
	const double transVec[] = { 2.515260082139980e+01, 4.059127243511693e+00, -5.588303932036697e+00 };  // [mm]
#else
	// the 5th distortion parameter, kc(5) is deactivated.

	const double fc_ir[] = { 5.864902565580264e+02, 5.867305900503998e+02 };  // [pixel]
	const double cc_ir[] = { 3.376088045224677e+02, 2.480083390372575e+02 };  // [pixel]
	const double alpha_c_ir = 0.0;
	//const double kc_ir[] = { -1.123867977947529e-01, 3.552017514491446e-01, -2.823972305243438e-03, 7.246763414437084e-03, 0.0 };  // 5x1 vector
	const double kc_ir[] = { -1.123867977947529e-01, 3.552017514491446e-01, -2.823972305243438e-03, 7.246763414437084e-03, 0.0, 0.0, 0.0, 0.0 };  // 8x1 vector

	const double fc_rgb[] = { 5.256215953836251e+02, 5.278165866956751e+02 };  // [pixel]
	const double cc_rgb[] = { 3.260532981578608e+02, 2.630788286947369e+02 };  // [pixel]
	const double alpha_c_rgb = 0.0;
	//const double kc_rgb[] = { 2.394862387380747e-01, -5.840355691714197e-01, 2.567740590187774e-03, 2.044179978023951e-03, 0.0 };  // 5x1 vector
	const double kc_rgb[] = { 2.394862387380747e-01, -5.840355691714197e-01, 2.567740590187774e-03, 2.044179978023951e-03, 0.0, 0.0, 0.0, 0.0 };  // 8x1 vector

	const double rotVec[] = { 1.121432126402549e-03, 1.535221550916760e-02, 3.701648572107407e-03 };
	const double transVec[] = { 2.512732389978993e+01, 3.724869927389498e+00, -4.534758982979088e+00 };  // [mm]
#endif

	//
	cv::Mat(3, 3, CV_64FC1, cv::Scalar::all(0)).copyTo(K_ir);
	K_ir.at<double>(0, 0) = fc_ir[0];
	K_ir.at<double>(0, 1) = alpha_c_ir * fc_ir[0];
	K_ir.at<double>(0, 2) = cc_ir[0];
	K_ir.at<double>(1, 1) = fc_ir[1];
	K_ir.at<double>(1, 2) = cc_ir[1];
	K_ir.at<double>(2, 2) = 1.0;
	cv::Mat(3, 3, CV_64FC1, cv::Scalar::all(0)).copyTo(K_rgb);
	K_rgb.at<double>(0, 0) = fc_rgb[0];
	K_rgb.at<double>(0, 1) = alpha_c_rgb * fc_rgb[0];
	K_rgb.at<double>(0, 2) = cc_rgb[0];
	K_rgb.at<double>(1, 1) = fc_rgb[1];
	K_rgb.at<double>(1, 2) = cc_rgb[1];
	K_rgb.at<double>(2, 2) = 1.0;

	cv::Mat(8, 1, CV_64FC1, (void *)kc_ir).copyTo(distCoeffs_ir);
	cv::Mat(8, 1, CV_64FC1, (void *)kc_rgb).copyTo(distCoeffs_rgb);

    cv::Rodrigues(cv::Mat(3, 1, CV_64FC1, (void *)rotVec), R_ir_to_rgb);
	cv::Mat(3, 1, CV_64FC1, (void *)transVec).copyTo(T_ir_to_rgb);
}

// [ref] load_kinect_sensor_parameters_from_RGB_to_IR() in ${CPP_RND_HOME}/test/machine_vision/opencv/opencv_image_rectification.cpp
void load_kinect_sensor_parameters_from_RGB_to_IR(
	cv::Mat &K_rgb, cv::Mat &distCoeffs_rgb, cv::Mat &K_ir, cv::Mat &distCoeffs_ir,
	cv::Mat &R_rgb_to_ir, cv::Mat &T_rgb_to_ir
)
{
	// [ref]
	//	Camera Calibration Toolbox for Matlab: http://www.vision.caltech.edu/bouguetj/calib_doc/
	//	http://docs.opencv.org/doc/tutorials/calib3d/camera_calibration/camera_calibration.html

	// Caution:
	//	In order to use the calibration results from Camera Calibration Toolbox for Matlab in OpenCV,
	//	a parameter for radial distrtortion, kc(5) has to be active, est_dist(5) = 1.

	// RGB (left) to IR (right)
#if 1
	// the 5th distortion parameter, kc(5) is activated.

	const double fc_rgb[] = { 5.248648079874888e+02, 5.268280486062615e+02 };  // [pixel]
	const double cc_rgb[] = { 3.267487100838014e+02, 2.618261169946102e+02 };  // [pixel]
	const double alpha_c_rgb = 0.0;
	//const double kc_rgb[] = { 2.796764337988712e-01, -1.112497355183840e+00, 9.264749543097661e-04, 2.428507887293728e-03, 1.743975665436613e+00 };  // 5x1 vector
	const double kc_rgb[] = { 2.796764337988712e-01, -1.112497355183840e+00, 9.264749543097661e-04, 2.428507887293728e-03, 1.743975665436613e+00, 0.0, 0.0, 0.0 };  // 8x1 vector

	const double fc_ir[] = { 5.865282023957649e+02, 5.866624209441105e+02 };  // [pixel]
	const double cc_ir[] = { 3.371875014947813e+02, 2.485295493095561e+02 };  // [pixel]
	const double alpha_c_ir = 0.0;
	//const double kc_ir[] = { -1.227176734054719e-01, 5.028746725848668e-01, -2.563029340202278e-03, 6.916996280663117e-03, -5.512162545452755e-01 };  // 5x1 vector
	const double kc_ir[] = { -1.227176734054719e-01, 5.028746725848668e-01, -2.563029340202278e-03, 6.916996280663117e-03, -5.512162545452755e-01, 0.0, 0.0, 0.0 };  // 8x1 vector

	const double rotVec[] = { 1.935939237060295e-03, -1.331788958930441e-02, -3.404128236480992e-03 };
	const double transVec[] = { -2.515262012891160e+01, -4.059118899373607e+00, 5.588237589014362e+00 };  // [mm]
#else
	// the 5th distortion parameter, kc(5) is deactivated.

	const double fc_rgb[] = { 5.256217798767822e+02, 5.278167798992870e+02 };  // [pixel]
	const double cc_rgb[] = { 3.260534767468189e+02, 2.630800669346188e+02 };  // [pixel]
	const double alpha_c_rgb = 0.0;
	//const double kc_rgb[] = { 2.394861400525463e-01, -5.840298777969020e-01, 2.568959896208732e-03, 2.044336479083819e-03, 0.0 };  // 5x1 vector
	const double kc_rgb[] = { 2.394861400525463e-01, -5.840298777969020e-01, 2.568959896208732e-03, 2.044336479083819e-03, 0.0, 0.0, 0.0, 0.0 };  // 8x1 vector

	const double fc_ir[] = { 5.864904832545356e+02, 5.867308191567271e+02 };  // [pixel]
	const double cc_ir[] = { 3.376079004969836e+02, 2.480098376453992e+02 };  // [pixel]
	const double alpha_c_ir = 0.0;
	//const double kc_ir[] = { -1.123902857373373e-01, 3.552211727724343e-01, -2.823183218548772e-03, 7.246270574438420e-03, 0.0 };  // 5x1 vector
	const double kc_ir[] = { -1.123902857373373e-01, 3.552211727724343e-01, -2.823183218548772e-03, 7.246270574438420e-03, 0.0, 0.0, 0.0, 0.0 };  // 8x1 vector

	const double rotVec[] = { -1.121214964017936e-03, -1.535031632771925e-02, -3.701579055761772e-03 };
	const double transVec[] = { -2.512730902761022e+01, -3.724884753207001e+00, 4.534776794502955e+00 };  // [mm]
#endif

	//
	cv::Mat(3, 3, CV_64FC1, cv::Scalar::all(0)).copyTo(K_rgb);
	K_rgb.at<double>(0, 0) = fc_rgb[0];
	K_rgb.at<double>(0, 1) = alpha_c_rgb * fc_rgb[0];
	K_rgb.at<double>(0, 2) = cc_rgb[0];
	K_rgb.at<double>(1, 1) = fc_rgb[1];
	K_rgb.at<double>(1, 2) = cc_rgb[1];
	K_rgb.at<double>(2, 2) = 1.0;
	cv::Mat(3, 3, CV_64FC1, cv::Scalar::all(0)).copyTo(K_ir);
	K_ir.at<double>(0, 0) = fc_ir[0];
	K_ir.at<double>(0, 1) = alpha_c_ir * fc_ir[0];
	K_ir.at<double>(0, 2) = cc_ir[0];
	K_ir.at<double>(1, 1) = fc_ir[1];
	K_ir.at<double>(1, 2) = cc_ir[1];
	K_ir.at<double>(2, 2) = 1.0;

	cv::Mat(8, 1, CV_64FC1, (void *)kc_rgb).copyTo(distCoeffs_rgb);
	cv::Mat(8, 1, CV_64FC1, (void *)kc_ir).copyTo(distCoeffs_ir);

    cv::Rodrigues(cv::Mat(3, 1, CV_64FC1, (void *)rotVec), R_rgb_to_ir);
	cv::Mat(3, 1, CV_64FC1, (void *)transVec).copyTo(T_rgb_to_ir);
}

cv::Rect get_bounding_rect(const cv::Mat &img)
{
	std::vector<cv::Point> pts;
	pts.reserve(img.rows * img.cols);
	for (int i = 0; i < img.rows; ++i)
		for (int j = 0; j < img.cols; ++j)
			if (!img.at<unsigned char>(i, j))
				pts.push_back(cv::Point(j, i));

	return cv::boundingRect(pts);
}

}  // namespace local
}  // unnamed namespace

namespace swl {

void create_superpixel_by_gSLIC(const cv::Mat &input_image, cv::Mat &superpixel_mask, const SEGMETHOD seg_method, const double seg_weight, const int num_segments);
void create_superpixel_boundary(const cv::Mat &superpixel_mask, cv::Mat &superpixel_boundary);

void construct_depth_guided_mask_using_superpixel(
	const cv::Size &imageSize_rgb, const cv::Mat &rgb_input_image, const cv::Mat &depth_boundary_image, const cv::Mat &depth_validity_mask, cv::Mat &depth_guided_mask,
	const int num_segments, const SEGMETHOD seg_method, const double seg_weight
)
{
	cv::Mat rgb_superpixel_mask;
	cv::Mat filtered_superpixel_mask(imageSize_rgb, CV_8UC1, cv::Scalar::all(255)), filtered_superpixel_indexes(imageSize_rgb, CV_32SC1, cv::Scalar::all(0)); 
	double minVal = 0.0, maxVal = 0.0;
	cv::Mat tmp_image;

	// PPP [] >>
	//	1. run superpixel.

	// superpixel mask consists of segment indexes.
	create_superpixel_by_gSLIC(rgb_input_image, rgb_superpixel_mask, seg_method, seg_weight, num_segments);

#if 0
	// show superpixel mask.
	cv::minMaxLoc(rgb_superpixel_mask, &minVal, &maxVal);
	rgb_superpixel_mask.convertTo(tmp_image, CV_32FC1, 1.0 / maxVal, 0.0);

	cv::imshow("superpixels by gSLIC - mask", tmp_image);
#endif

#if 0
	// show superpixel boundary.
	cv::Mat rgb_superpixel_boundary;
	swl::create_superpixel_boundary(rgb_superpixel_mask, rgb_superpixel_boundary);

	rgb_input_image.copyTo(tmp_image);
	tmp_image.setTo(cv::Scalar(0, 0, 255), rgb_superpixel_boundary);

	cv::imshow("superpixels by gSLIC - boundary", tmp_image);
#endif

	// PPP [] >>
	//	2. depth info�κ��� ���� ������ boundary�� ����.
	//		Depth histogram�� �̿��� depth region�� ���� => ��ü�� ��迡 ���ؼ��� �ƴ϶� depth range�� ���ؼ� ������ ����. ��ü������ ����� �� ���� ū blob�� ������.
	//		Depth image�� edge �����κ��� boundary ���� => �ٸ� �� ��ü�� �´�� �ִ� ���, depth image�� boundary info�κ��� ���˸��� �ĺ��ϱ� �����.

	// FIXME [enhance] >> too slow. speed up.
	{
		// PPP [] >>
		//	3. Depth boundary�� ��ġ�� superpixel�� index�� ����.
		//		Depth boundary�� mask�� ����ϸ� ���� index�� ������ �� ����.

		//filtered_superpixel_indexes.setTo(cv::Scalar::all(0));
		rgb_superpixel_mask.copyTo(filtered_superpixel_indexes, depth_boundary_image);
		cv::MatIterator_<int> itBegin = filtered_superpixel_indexes.begin<int>(), itEnd = filtered_superpixel_indexes.end<int>();
		std::sort(itBegin, itEnd);
		cv::MatIterator_<int> itEndNew = std::unique(itBegin, itEnd);
		//std::size_t count = 0;
		//for (cv::MatIterator_<int> it = itBegin; it != itEndNew; ++it, ++count)
		//	std::cout << *it << std::endl;

		// PPP [] >>
		//	4. ����� superpixel index�鿡 �ش��ϴ� superpixel ������ 0, �׿� ������ 1�� ����.

		//filtered_superpixel_mask.setTo(cv::Scalar::all(255));
		for (cv::MatIterator_<int> it = itBegin; it != itEndNew; ++it)
			// FIXME [check] >> why is 0 contained in index list?
			if (*it)
				filtered_superpixel_mask.setTo(cv::Scalar::all(0), rgb_superpixel_mask == *it);

#if 1
		// show filtered superpixel index mask.
		cv::imshow("mask of superpixels on depth boundary", filtered_superpixel_mask);
#endif
	}

	// construct depth guided mask.
	depth_guided_mask.setTo(cv::Scalar::all(127));  // depth boundary region.
	depth_guided_mask.setTo(cv::Scalar::all(255), depth_validity_mask & filtered_superpixel_mask);  // valid depth region (foreground).
	depth_guided_mask.setTo(cv::Scalar::all(0), ~depth_validity_mask & filtered_superpixel_mask);  // invalid depth region (background).
}

void construct_depth_guided_mask_using_morphological_operation_of_depth_boundary(const cv::Size &imageSize_rgb, const cv::Mat &rgb_input_image, const cv::Mat &depth_boundary_image, const cv::Mat &depth_validity_mask, cv::Mat &depth_guided_mask)
{
	const cv::Mat &selement = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3), cv::Point(-1, -1));
	//const cv::Mat &selement = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5), cv::Point(-1, -1));
	//const cv::Mat &selement = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7), cv::Point(-1, -1));

	cv::Mat dilated_depth_boundary_image;
	cv::dilate(depth_boundary_image, dilated_depth_boundary_image, selement, cv::Point(-1, -1), 3);

#if 1
		// show dilated depth boundary mask.
		cv::imshow("dilated depth boundary mask", dilated_depth_boundary_image);
#endif

	// construct depth guided mask.
	depth_guided_mask.setTo(cv::Scalar::all(127));  // depth boundary region.
	depth_guided_mask.setTo(cv::Scalar::all(255), depth_validity_mask & ~dilated_depth_boundary_image);  // valid depth region (foreground).
	depth_guided_mask.setTo(cv::Scalar::all(0), ~depth_validity_mask & ~dilated_depth_boundary_image);  // invalid depth region (background).
}

void run_grabcut(const cv::Size &imageSize_rgb, const cv::Mat &rgb_input_image, const cv::Mat &depth_guided_mask)
{
	// PPP [] >>
	//	5. ����� superpixel index��κ��� foreground & background ������ ����.
	//		���õ� depth range�κ��� ���� ������ 1��, �׿� ������ 0���� ������ ��, ����� superpixel index�� bit-and operation.
	//		1�� ������ ������ boundary�� GrabCut�� foreground seed�� ���.
	//		���õ� depth range�κ��� ���� ������ 0��, �׿� ������ 1���� ������ ��, ����� superpixel index�� bit-and operation.
	//		1�� ������ ������ boundary�� GrabCut�� background seed�� ���.

	cv::Mat grabCut_mask(imageSize_rgb, CV_8UC1);
	cv::Mat grabCut_bgModel, grabCut_fgModel;

#if 1
	// GC_BGD, GC_FGD, GC_PR_BGD, GC_PR_FGD
	//grabCut_mask.setTo(cv::Scalar::all(cv::GC_PR_BGD));
	grabCut_mask.setTo(cv::Scalar::all(cv::GC_PR_FGD));
	grabCut_mask.setTo(cv::Scalar::all(cv::GC_FGD), 255 == depth_guided_mask);  // foreground.
	grabCut_mask.setTo(cv::Scalar::all(cv::GC_BGD), 0 == depth_guided_mask);  // background.

	cv::grabCut(rgb_input_image, grabCut_mask, cv::Rect(), grabCut_bgModel, grabCut_fgModel, 1, cv::GC_INIT_WITH_MASK);
#else
	// FIXME [enhance] >> too slow. speed up.
	const cv::Rect grabCut_rect(local::get_bounding_rect(depth_guided_mask > 0));

	cv::grabCut(rgb_input_image, grabCut_mask, grabCut_rect, grabCut_bgModel, grabCut_fgModel, 1, cv::GC_INIT_WITH_RECT);
#endif

	cv::Mat tmp_image;
#if 0
	// show foreground & background masks.
	//cv::imshow("foreground mask", 255 == depth_guided_mask);  // foreground.
	//cv::imshow("background mask", 0 == depth_guided_mask);  // background.

	// show GrabCut mask.
	grabCut_mask.convertTo(tmp_image, CV_8UC1, 255.0 / cv::GC_PR_FGD, 0.0);
	//cv::rectangle(tmp_image, grabCut_rect, cv::Scalar::all(255), 2);
	cv::imshow("GrabCut mask", tmp_image);
#endif

	cv::grabCut(rgb_input_image, grabCut_mask, cv::Rect(), grabCut_bgModel, grabCut_fgModel, 1, cv::GC_EVAL);

#if 1
	rgb_input_image.copyTo(tmp_image, cv::Mat(grabCut_mask & 1));
	cv::imshow("GrabCut result", tmp_image);
#endif
}

void run_efficient_graph_based_image_segmentation(const cv::Size &imageSize_rgb, const cv::Mat &rgb_input_image, const cv::Mat &depth_guided_mask)
{
}

}  // namespace swl

int main(int argc, char *argv[])
{
	int retval = EXIT_SUCCESS;
	try
	{
		//
		const std::size_t num_images = 4;
		const cv::Size imageSize_ir(640, 480), imageSize_rgb(640, 480);

		std::vector<std::string> rgb_input_file_list, depth_input_file_list;
		rgb_input_file_list.reserve(num_images);
		depth_input_file_list.reserve(num_images);
		rgb_input_file_list.push_back("../data/kinect_segmentation/kinect_rgba_20130530T103805.png");
		rgb_input_file_list.push_back("../data/kinect_segmentation/kinect_rgba_20130531T023152.png");
		rgb_input_file_list.push_back("../data/kinect_segmentation/kinect_rgba_20130531T023346.png");
		rgb_input_file_list.push_back("../data/kinect_segmentation/kinect_rgba_20130531T023359.png");
		depth_input_file_list.push_back("../data/kinect_segmentation/kinect_depth_20130530T103805.png");
		depth_input_file_list.push_back("../data/kinect_segmentation/kinect_depth_20130531T023152.png");
		depth_input_file_list.push_back("../data/kinect_segmentation/kinect_depth_20130531T023346.png");
		depth_input_file_list.push_back("../data/kinect_segmentation/kinect_depth_20130531T023359.png");

		const bool use_depth_range_filtering = false;
		std::vector<cv::Range> depth_range_list;
		{
			depth_range_list.reserve(num_images);
#if 0
			depth_range_list.push_back(cv::Range(500, 3420));
			depth_range_list.push_back(cv::Range(500, 3120));
			depth_range_list.push_back(cv::Range(500, 1700));
			depth_range_list.push_back(cv::Range(500, 1000));
#else
			const int min_depth = 100, max_depth = 3000;
			depth_range_list.push_back(cv::Range(min_depth, max_depth));
			depth_range_list.push_back(cv::Range(min_depth, max_depth));
			depth_range_list.push_back(cv::Range(min_depth, max_depth));
			depth_range_list.push_back(cv::Range(min_depth, max_depth));
#endif
		}

		//
		boost::scoped_ptr<swl::KinectSensor> kinect;
		{
			const bool useIRtoRGB = true;
			cv::Mat K_ir, K_rgb;
			cv::Mat distCoeffs_ir, distCoeffs_rgb;
			cv::Mat R, T;

			// load the camera parameters of a Kinect sensor.
			if (useIRtoRGB)
				local::load_kinect_sensor_parameters_from_IR_to_RGB(K_ir, distCoeffs_ir, K_rgb, distCoeffs_rgb, R, T);
			else
				local::load_kinect_sensor_parameters_from_RGB_to_IR(K_rgb, distCoeffs_rgb, K_ir, distCoeffs_ir, R, T);

			kinect.reset(new swl::KinectSensor(useIRtoRGB, imageSize_ir, K_ir, distCoeffs_ir, imageSize_rgb, K_rgb, distCoeffs_rgb, R, T));
			kinect->initialize();
		}

		const cv::Mat &selement = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3), cv::Point(-1, -1));
		//const cv::Mat &selement = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5), cv::Point(-1, -1));
		//const cv::Mat &selement = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7), cv::Point(-1, -1));

		//
		cv::Mat rectified_depth_image, rectified_rgb_image;
		cv::Mat depth_validity_mask(imageSize_rgb, CV_8UC1), depth_boundary_image, depth_guided_mask(imageSize_rgb, CV_8UC1);
		double minVal = 0.0, maxVal = 0.0;
		cv::Mat tmp_image;
		for (std::size_t i = 0; i < num_images; ++i)
		{
			// load images.
			const cv::Mat rgb_input_image(cv::imread(rgb_input_file_list[i], CV_LOAD_IMAGE_COLOR));
			if (rgb_input_image.empty())
			{
				std::cout << "fail to load image file: " << rgb_input_file_list[i] << std::endl;
				continue;
			}
			const cv::Mat depth_input_image(cv::imread(depth_input_file_list[i], CV_LOAD_IMAGE_UNCHANGED));
			if (depth_input_image.empty())
			{
				std::cout << "fail to load image file: " << depth_input_file_list[i] << std::endl;
				continue;
			}

			const int64 start = cv::getTickCount();

			// rectify Kinect images.
			{
				kinect->rectifyImagePair(depth_input_image, rgb_input_image, rectified_depth_image, rectified_rgb_image);

#if 1
				// show rectified images
				cv::imshow("rectified RGB image", rectified_rgb_image);

				cv::minMaxLoc(rectified_depth_image, &minVal, &maxVal);
				rectified_depth_image.convertTo(tmp_image, CV_32FC1, 1.0 / maxVal, 0.0);
				cv::imshow("rectified depth image", tmp_image);
#endif

#if 0
				std::ostringstream strm1, strm2;
				strm1 << "../data/kinect_segmentation/rectified_image_depth_" << i << ".png";
				cv::imwrite(strm1.str(), rectified_depth_image);
				strm2 << "../data/kinect_segmentation/rectified_image_rgb_" << i << ".png";
				cv::imwrite(strm2.str(), rectified_rgb_image);
#endif
			}

			// make depth validity mask.
			{
#if __USE_RECTIFIED_IMAGE
				if (use_depth_range_filtering)
					cv::inRange(rectified_depth_image, cv::Scalar::all(depth_range_list[i].start), cv::Scalar::all(depth_range_list[i].end), depth_validity_mask);
				else
					cv::Mat(rectified_depth_image > 0).copyTo(depth_validity_mask);
#else
				if (use_depth_range_filtering)
					cv::inRange(depth_input_image, cv::Scalar::all(valid_depth_range.start), cv::Scalar::all(valid_depth_range.end), depth_validity_mask);
				else
					cv::Mat(depth_input_image > 0).copyTo(depth_validity_mask);
#endif

				cv::erode(depth_validity_mask, depth_validity_mask, selement, cv::Point(-1, -1), 3);
				cv::dilate(depth_validity_mask, depth_validity_mask, selement, cv::Point(-1, -1), 3);

#if 1
				// show depth validity mask.
				cv::imshow("depth validity mask", depth_validity_mask);
#endif
			}

			// extract boundary from depth image by edge detector.
			{
#if __USE_RECTIFIED_IMAGE
				rectified_depth_image.copyTo(tmp_image, depth_validity_mask);
#else
				depth_input_image.copyTo(tmp_image, depth_validity_mask);
#endif

				cv::minMaxLoc(tmp_image, &minVal, &maxVal);
				tmp_image.convertTo(tmp_image, CV_8UC1, 255.0 / maxVal, 0.0);

				//const double low = 1.0, high = 255.0;
				//const double alpha = (high - low) / (depth_range_list[i].end - depth_range_list[i].start), beta = low - alpha * depth_range_list[i].start;
				//tmp_image.convertTo(tmp_image, CV_8UC1, alpha, beta);
				
				local::canny(tmp_image, depth_boundary_image);


#if 1
				// show depth boundary image.
				cv::imshow("depth boundary by Canny", depth_boundary_image);
#endif
			}

#if 0
			// construct depth guided mask using superpixel.
			{
				const int num_segments = 2500;
				const SEGMETHOD seg_method = XYZ_SLIC;  // SLIC, RGB_SLIC, XYZ_SLIC
				const double seg_weight = 0.3;

				//cv::dilate(depth_boundary_image, depth_boundary_image, selement, cv::Point(-1, -1), 3);

#if __USE_RECTIFIED_IMAGE
				swl::construct_depth_guided_mask_using_superpixel(imageSize_rgb, rectified_rgb_image, depth_boundary_image, depth_validity_mask, depth_guided_mask, num_segments, seg_method, seg_weight);
#else
				swl::construct_depth_guided_mask_using_superpixel(imageSize_rgb, rgb_input_image, depth_boundary_image, depth_validity_mask, depth_guided_mask, num_segments, seg_method, seg_weight);
#endif
			}
#elif 1
			// construct depth guided mask using morphological operation of depth boundary.
			{
#if __USE_RECTIFIED_IMAGE
				swl::construct_depth_guided_mask_using_morphological_operation_of_depth_boundary(imageSize_rgb, rectified_rgb_image, depth_boundary_image, depth_validity_mask, depth_guided_mask);
#else
				swl::construct_depth_guided_mask_using_morphological_operation_of_depth_boundary(imageSize_rgb, rgb_input_image, depth_boundary_image, depth_validity_mask, depth_guided_mask);
#endif
			}
#endif

#if 1
			// show depth guided mask.
			cv::imshow("depth guided mask", depth_guided_mask);
#endif

#if 0
			// segment image by GrabCut algorithm.
			{
#if __USE_RECTIFIED_IMAGE
				swl::run_grabcut(imageSize_rgb, rectified_rgb_image, depth_guided_mask);
#else
				swl::run_grabcut(imageSize_rgb, rgb_input_image, depth_guided_mask);
#endif
			}
#else
			// segment image by efficient graph-based image segmentation algorithm.
			{
#if __USE_RECTIFIED_IMAGE
				swl::run_efficient_graph_based_image_segmentation(imageSize_rgb, rectified_rgb_image, depth_guided_mask);
#else
				swl::run_efficient_graph_based_image_segmentation(imageSize_rgb, rgb_input_image, depth_guided_mask);
#endif
			}
#endif

			const int64 elapsed = cv::getTickCount() - start;
			const double freq = cv::getTickFrequency();
			const double etime = elapsed * 1000.0 / freq;
			const double fps = freq / elapsed;
			std::cout << std::setprecision(4) << "elapsed time: " << etime <<  ", FPS: " << fps << std::endl;

			const unsigned char key = cv::waitKey(0);
			if (27 == key)
				break;
		}

		cv::destroyAllWindows();
	}
	catch (const cv::Exception &e)
	{
		//std::cout << "OpenCV exception caught: " << e.what() << std::endl;
		//std::cout << "OpenCV exception caught: " << cvErrorStr(e.code) << std::endl;
		std::cout << "OpenCV exception caught: " << std::endl
			<< "\tdescription: " << e.err << std::endl
			<< "\tline:        " << e.line << std::endl
			<< "\tfunction:    " << e.func << std::endl
			<< "\tfile:        " << e.file << std::endl;
		retval = EXIT_FAILURE;
	}
    catch (const std::bad_alloc &e)
	{
		std::cout << "std::bad_alloc caught: " << e.what() << std::endl;
		retval = EXIT_FAILURE;
	}
	catch (const std::exception &e)
	{
		std::cout << "std::exception caught: " << e.what() << std::endl;
		retval = EXIT_FAILURE;
	}
	catch (...)
	{
		std::cout << "unknown exception caught" << std::endl;
		retval = EXIT_FAILURE;
	}

	std::cout << "press any key to exit ..." << std::endl;
	std::cin.get();

	return retval;
}
