
#include<iostream>
#include<string>

#include "MyImage.h"

std::ostream& operator<<(std::ostream& os, const MyImage::ImageInfo &img)
{
	os << "ImageInfo: {" << img.imageBaseName << ", " << img.classType <<
		", " << img.cx << ", " << img.cy << ", " << img.radius << "}";
	return os;
}

