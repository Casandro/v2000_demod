#include <stdio.h>
#include <math.h>

int main(int argc, char **argv)
{
	double t=0;
	double x[2];
	double i=0;
	double i1=0;
	double i2=0;
	double q=0;
	double q1=0;
	double q2=0;
	int n=0;
	while (scanf("%lf %lf", &t, &(x[n%2]))==2) {
		if (n%2==1) {
			double f=1;
			if ((n/2)==1) f=-1;
			i2=i1; i1=i;
			q2=q1; q1=q;
			i=x[0]*f;
			q=x[1]*f;
			double i_=i2-i;
			double q_=q2-q;
			double p=i1*i1+q1*q1;
			double o=(i_*q1-q_*i1)/p;
			printf("%lf %lf %lf\n", t, o*0.1, p);
		}	
		n=(n+1)%4;
	}
}
