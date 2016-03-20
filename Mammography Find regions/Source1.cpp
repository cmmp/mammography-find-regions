#include <boost/math/distributions/negative_binomial.hpp>
#include <iostream>


using boost::math::negative_binomial_distribution;

using namespace boost::math::policies;

typedef negative_binomial_distribution<
	double,
	policy<discrete_quantile<integer_round_nearest> >
> dist_type;

int main4() {

	double vals[] = { -1.7690062,  1.8395634,  0.1471367 , 0.9166575, -1.3735382 ,-1.2922401, -0.8019666, -2.0271023,  0.3679942, -0.2203661,
		 - 0.6833688 ,-0.4724519 ,-1.1511077, -0.5498765, -0.3319151, -1.4608938, -1.6329929 ,-0.4759542,  0.5382961,  0.9091339,
		- 0.1960731  ,1.9935024, -0.9178812 , 2.3128425 , 0.4423744,  0.6392177,  0.3017261 , 0.4612829 ,-0.5414020  ,1.4440446 };

	// Lower quantile rounded (down) to nearest:
	double x = quantile(dist_type(20, 0.3), 0.05); // 27
												   // Upper quantile rounded (down) to nearest:
	double y = quantile(complement(dist_type(20, 0.3), 0.05)); // 68

	double z = quantile(vals, 0.25);

	std::cout << x << std::endl;
	std::cout << y << std::endl;
}
