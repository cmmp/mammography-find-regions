#pragma once

#include<iostream>

namespace MyImage {
	struct ImageInfo {
		std::string imageBaseName;
		std::string classType;
		// cx and cy are the centers of the mass
		int cx, cy, radius;
	};	
}

std::ostream& operator<<(std::ostream& os, const MyImage::ImageInfo &img);


