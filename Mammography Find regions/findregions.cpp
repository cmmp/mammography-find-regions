/*
 *	Program to find selected regions of mammograms from a 
 *	supervision file.
 *
 *	Author: Cássio M. M. Pereira
 *	Date: sábado, 9 de janeiro de 2016 12:16:02
 *
*/

#define IMAGE_HEIGHT 1024

#include<iostream>
#include<string>
#include<vector>
#include<fstream>
#include<cmath>

// for command line arguments parsing
#include<boost/program_options/options_description.hpp>
#include<boost/program_options/variables_map.hpp>
#include<boost/program_options/parsers.hpp>

// for csv parsing
#include<boost/tokenizer.hpp> 

// for image manipulation
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// basic image information
#include "MyImage.h"

// auxiliary functions
#include <MyCppUtils.h>

bool criterion1(int count) {
	std::cout << "> Criterion 1: count = " << count << std::endl;
	// mass area has to be in this range
	if (count >= 900 && count <= 5000)
		return true;
	return false;
}

bool criterion2(cv::Mat contMat, cv::Mat lblImg, int selectedComponent) {
	// check if third order moment (skewness) is
	// negative.

	// see https://en.wikipedia.org/wiki/Image_moment
	// http://answers.opencv.org/question/87432/compute-moments-after-connected-component-labeling/?answer=87467#post-id-87467
	// for a rough explanation

	cv::Rect r = cv::boundingRect(lblImg == selectedComponent);

	cv::Moments mom = cv::moments(contMat(r), false);

	// see equations on the wiki page

	// centroid:
	int x = (mom.m10 / mom.m00);
	int y = (mom.m01 / mom.m00);

	int MU00 = mom.m00;
	int MU30 = mom.m30 - 3 * x * mom.m20 + 2 * x * x * mom.m10;
	int MU03 = mom.m03 - 3 * y * mom.m02 + 2 * y * mom.m01;
	int MU12 = mom.m12 - 2 * y * mom.m11 - x * mom.m02 + 2 * y * y * mom.m10;
	int MU21 = mom.m21 - 2 * x * mom.m11 - y * mom.m20 + 2 * x * mom.m01;

	double n30 = MU30 / (std::pow(MU00, 1. + (3./2.)));
	double n03 = MU03 / (std::pow(MU00, 1. + (3. / 2.)));
	double n12 = MU12 / (std::pow(MU00, 1. + (3. / 2.)));
	double n21 = MU21 / (std::pow(MU00, 1. + (3. / 2.)));

	double I3 = std::pow(n30 - 3 * n12, 2) + std::pow(3 * n21 - n03, 2);
	
	// or we do it the opencv way:
	//cv::Vec<double, 7> humom;
	//cv::HuMoments(mom, humom);
	//std::cout << "opencv humom[2] = " << humom[2] << std::endl;

	//std::cout << "third order moments: MU30: " << MU30  << " MU03: " << MU03 << " I3 = " << I3 << std::endl;
	
	std::cout << "> Criterion 2: I3 = " << I3 << std::endl;

	return (I3 < 0) ? true : false;
}

bool criterion3(cv::Mat contMat, std::vector<int> xpos,
	std::vector<int> ypos, int count) {
	// compute if mean intensity is higher than 
	// a threshold value
	double T = 160.;
	double sum = 0;

	for (int i = 0; i < count; i++) {
		sum += contMat.at<uchar>(ypos[i], xpos[i]);
	}

	double mean = sum / count;

	std::cout << "> Criterion 3: mean intensity: " << mean << std::endl;

	return (mean >= T) ? true : false;
}

void process_image(const MyImage::ImageInfo &img_info,
	const std::string &png_in_dir,
	const std::string &out_dir, 
	const std::string &png_out_dir) {

	std::cout << "Processing " << img_info << std::endl;

	cv::Mat image;
	image = cv::imread(png_in_dir + img_info.imageBaseName + ".png", cv::IMREAD_COLOR);

	if (!image.data) {
		std::cerr << "Could not open or find the image" << std::endl;
		exit(EXIT_FAILURE);
	}

	cv::Mat imgray;
	cv::cvtColor(image, imgray, CV_RGB2GRAY);
	image = imgray;
	
	// now we have to crop to the region indicated 
	// in cx and cy, considering also the radius
	
	int upperX, upperY, width, height;
	upperX = img_info.cx - img_info.radius;
	upperY = img_info.cy - img_info.radius;
	width = img_info.cx + img_info.radius - upperX + 1;
	height = img_info.cy + img_info.radius - upperY + 1;

	cv::Rect myCrop(upperX, upperY, width, height);
	
	cv::Mat cropped = image(myCrop);

	// apply 3x3 median filter
	cv::Mat mBlur;
	cv::medianBlur(cropped, mBlur, 3);

	// apply 9x9 average filter
	cv::Mat avgBlur;
	cv::boxFilter(mBlur, avgBlur, -1, cv::Size(9,9));

	// gray scale transformation - adjusting contrast:
	double min_pval, max_pval;
	cv::minMaxLoc(avgBlur, &min_pval, &max_pval);
	//std::cout << "min value: " << min_pval << " max pval: " << max_pval << std::endl;
	
	cv::Mat contMat = avgBlur.clone();
	
	for (int i = 0; i < avgBlur.rows; i++) {
		for (int j = 0; j < avgBlur.cols; j++) {
			int val = avgBlur.at<uchar>(cv::Point(i, j));
			contMat.at<uchar>(cv::Point(i, j)) = (val * val * val) / (max_pval * max_pval);
		}
	}

	// thresholding:
	std::vector<uchar> array(contMat.rows * contMat.cols);
	for (int i = 0; i < contMat.rows; i++)
		array.insert(array.end(), contMat.ptr<uchar>(i), contMat.ptr<uchar>(i) + contMat.cols);

	double threshold = MyCppUtils::Utils::sampleQuantile(array, 0.95);

	//std::cout << "got a threshold of: " << threshold << std::endl;

	// binarize the image:
	cv::Mat binImg = contMat.clone();

	for (int i = 0; i < binImg.rows; i++) {
		for (int j = 0; j < binImg.cols; j++) {
			int val = binImg.at<uchar>(cv::Point(i, j));
			binImg.at<uchar>(cv::Point(i, j)) = (val >= (int) threshold) ? 255 : 0;
		}
	}

	// save the binarized image
	//cv::imwrite(png_out_dir + img_info.imageBaseName + ".png", binImg);	

	// find connected components
	cv::Mat lblImg;
	int num_lbls = cv::connectedComponents(binImg, lblImg, 8);
	
	
	cv::imshow("cropped window", cropped);
	cv::imshow("median filter", mBlur);
	cv::imshow("avg filter after median", avgBlur);
	cv::imshow("contrast adj after avg", contMat);
	cv::imshow("binarized img", binImg);
	std::cout << "number of components: " << num_lbls << std::endl;
	cv::Mat seeMyLabels;
	cv::normalize(lblImg, seeMyLabels, 0, 255, cv::NORM_MINMAX, CV_8U);
	cv::imshow("Connected components", seeMyLabels);
	cv::waitKey(0);
	exit(1);
	

	// apply the 3 criteria to find a region of 
	// interest
	std::vector<int> xpos(lblImg.rows * lblImg.cols);
	std::vector<int> ypos(lblImg.rows * lblImg.cols);

	// in case no connected component fits any of the 
	// three criteria, we select the one with the 
	// largest area.
	std::vector<int> bestXpos(lblImg.rows * lblImg.cols);
	std::vector<int> bestYpos(lblImg.rows * lblImg.cols);
	int bestCount = -1;
	int bestComponent = -1;

	int selectedComponent = -1; // which components indicates the ROI

	int count;

	for (int i = 1; i < num_lbls; i++) {
		// we dont use 0 because label 0 is used for background.
		
		count = 0;

		for (int j = 0; j < lblImg.rows; j++) {
			for (int k = 0; k < lblImg.cols; k++) {
				int lbl = lblImg.at<int>(cv::Point(j, k));
				if (lbl == i) {
					xpos[count] = k; // position in the column
					ypos[count] = j; // position in the row
					count++;
				}
			}
		}

		//std::cout << "calculei " << count << "pixels" << std::endl;
		
		// now check the three criteria for this selected region.
		// We pick the first accepted region as the region of the tumor,
		// since we selected only images with one tumor.

		// 1st criterion - mass area has to be in range of [900,5000] pixels.
		// 2nd criterion - negative third order moment (skewness)
		// 3rd criterion - mean intensity higher than a threshold

		std::cout << "testing criteria for connected component " << i << "/" << num_lbls - 1 << std::endl;
		
		if (count < 10) {
			std::cout << "very few points on this component..." << std::endl;
			continue;
		}

		if (criterion1(count) ||
			criterion2(contMat, lblImg, i) ||
			criterion3(contMat, xpos, ypos, count)) {
			std::cout << "selected component " << i << " stoping search..." << std::endl;
			selectedComponent = i;
			break;
		}

		if (count > bestCount) {
			bestXpos.clear(); bestYpos.clear();
			bestXpos.assign(xpos.begin(), xpos.end());
			bestYpos.assign(ypos.begin(), ypos.end());
			bestCount = count;
			bestComponent = i;
		}
	} // end for -> search for suspicious connected component 

	// save positions of selected component to 
	// a csv file
	std::ofstream file;
	file.open(out_dir + img_info.imageBaseName + ".csv");

	if (!file.is_open()) {
		std::cerr << "could not open output file to write positions..." << std::endl;
		exit(EXIT_FAILURE);
	}

	
	if (selectedComponent == -1) {
		std::cout << "Found no suitable selected component... Picking the one with largest area -> " << bestComponent << std::endl;
		xpos = bestXpos;
		ypos = bestYpos;
		count = bestCount;
	}

	for (int i = 0; i < count; i++)
		file << ypos[i] << "," << xpos[i] << "\n";
	
	file.close();
}

int mainxx2(int argc, char **argv) {

	namespace po = boost::program_options;

	std::string orig_path, out_path, sup_path, png_out_path;
	int threshold;
	
	po::options_description desc("Supported options");
	desc.add_options()
		("help,h", "print this help message")
		("supervision-csv,S",
			po::value<std::string>(&sup_path)->default_value("C:\\Users\\Cássio\\datasets\\all-mias\\supervisao.csv"),
			"path for supervision csv file")
		("images-dir,I", 
			po::value<std::string>(&orig_path)->default_value("C:\\Users\\Cássio\\datasets\\all-mias\\orig-pngs\\"),
			"path for original png files")
		("output-dir,O",
			po::value<std::string>(&out_path)->default_value("C:\\Users\\Cássio\\datasets\\all-mias\\region-data\\"),
			"directory for output data files")
		("png-output-dir,P", 
			po::value<std::string>(&png_out_path)->default_value("C:\\Users\\Cássio\\datasets\\all-mias\\region-pngs\\"),
			"directory for output png files")
		/*("threshold,T",
			po::value<int>(&threshold)->default_value(200),
			"threshold value for generating data files from regions")*/
		;
	
	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
	po::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << std::endl;
		return 1;
	}

	// open the csv file to read the image file paths
	std::ifstream csvin(sup_path);

	if (!csvin.is_open()) {
		std::cerr << "Could not open supervision csv file..." << std::endl;
		return 1;
	}

	typedef boost::tokenizer< boost::escaped_list_separator<char> > Tokenizer;

	std::string line;
	std::vector<std::string> vec;

	std::getline(csvin, line); // throw away the header line

	while (std::getline(csvin, line)) {
		// process an image file
		Tokenizer tok(line);
		vec.assign(tok.begin(), tok.end());

		MyImage::ImageInfo img_info{vec[0], // file name
			vec[1], // class type
			std::stoi(vec[2]), // cx
			IMAGE_HEIGHT - std::stoi(vec[3]), // cy: we do this because the supervision considers origin as bottom left, not upper left.
			std::stoi(vec[4]) // radius
		};

		process_image(img_info, orig_path, out_path, png_out_path);
	}
	
	csvin.close();

	return 0;
}
