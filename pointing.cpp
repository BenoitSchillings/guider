#include <stdio.h>
#include <math.h>

#define RD 57.2958;


class bpoint {

public:	
	bpoint();
	~bpoint();
	double distance(double az1, double ele1, double az2, double ele2);
};


	bpoint::bpoint()
{
}

	bpoint::~bpoint()
{
}


double bpoint::distance(double az1, double ele1, double az2, double ele2)
{
	double	phi1 = ele1/RD;
	double	phi2 = ele2/RD;

	double	delta_phi = (ele2 - ele1) / RD;
	double	delta_lam = (az2 - az1) /RD;

	double a = sin(delta_phi/2);
	a = a * a;

	a = a + cos(phi1) * cos(phi2) * sin(delta_lam/2) * sin(delta_lam/2);

	double c = 2 * atan2(sqrt(a), sqrt(1-a));

	c = c * RD;
	
	printf("%f\n", c);
}




int main()
{
	bpoint  b;

	b.distance(-20, 20, 83, 23);
	return 0;
}

