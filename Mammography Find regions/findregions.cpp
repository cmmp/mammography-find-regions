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
	
	/*
	cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);
	cv::imshow("Display window", image);
	cv::namedWindow("cropped window", cv::WINDOW_AUTOSIZE);
	cv::imshow("cropped window", cropped);
	cv::waitKey(0);
	*/

	// save the cropped image
	cv::imwrite(png_out_dir + img_info.imageBaseName + ".png", cropped);

	// apply threshold to cropped region
	// and save positions with intensity >= threshold to 
	// a csv file
	std::ofstream file;
	file.open(out_dir + img_info.imageBaseName + ".csv");

	if (!file.is_open()) {
		std::cerr << "could not open output file to write positions..." << std::endl;
		exit(EXIT_FAILURE);
	}

	std::vector<uchar> array(cropped.rows * cropped.cols);
	for (int i = 0; i < cropped.rows; i++)
		array.insert(array.end(), cropped.ptr<uchar>(i), cropped.ptr<uchar>(i) + cropped.cols);
	
	double threshold = MyCppUtils::Utils::sampleQuantile(array, 0.995);

	std::cout << "got a threshold of: " << threshold << std::endl;

	for (int i = 0; i < cropped.rows; i++) {
		for (int j = 0; j < cropped.cols; j++) {
			int val = cropped.at<uchar>(cv::Point(i, j));
			//cout << "val(" << i << "," << j << ") = " << val << endl;
			if (val >= threshold) {				
				file << i << "," << j << "\n";
			}
		}
	}

	file.close();

}

int main(int argc, char **argv) {

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
