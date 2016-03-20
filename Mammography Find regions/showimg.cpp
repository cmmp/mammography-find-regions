#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>

int main3() {
	cv::Mat image;
	image = cv::imread("C://Users//Cássio//datasets//all-mias//region-pngs//mdb058.png", cv::IMREAD_COLOR);

	cv::Mat imgray;
	cv::cvtColor(image, imgray, CV_RGB2GRAY);

	cv::imshow("Gray scale", imgray);
	cv::waitKey(0);
	
	return 0;
}