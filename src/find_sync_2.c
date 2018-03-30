#include<stdio.h>
#include<math.h>

#define KHZ (1000)
#define MHZ (1000*KHZ)
#define SRATE (8*MHZ)

#define SECOND (SRATE)
#define MSEC (MHZ/1000)
#define USEC (MSEC/1000)

//A field is 20 ms long
#define FIELDLEN (20*MSEC)

//A line is 64µs long
#define LINELEN (64*USEC)



//The timing error is +-32µs per full field
#define TIMING_MAX (32.0*USEC/(double)FIELDLEN)
#define TIMING_MIN (-32.0*USEC/(double)FIELDLEN)
#define TIMING_CNT (256)


//This is where the last 20ms are stored
//This is a circular buffer
#define BUFFERLEN (FIELDLEN+8*LINELEN)
double field[BUFFERLEN];
int field_p; //Pointer to the next value to be written


//Gets the sample offset*error before the current one
double before(double offset, double error)
{	
	int p=((int)((field_p+BUFFERLEN)-roundl(offset*error)))%BUFFERLEN;
	return field[p];
}

double timing_to_error(int timing)
{
	return 1+((double)timing)/TIMING_CNT*(TIMING_MAX-TIMING_MIN)+TIMING_MIN;
}

double before_impulse(double offset, double length, double error)
{
	return before(offset,error)-before(offset+length,error);
}

//calculate the amount we need to change the integral for normal sync pulses
double diff_normal_sync(double error, double offset)
{
	double sum=0;
	int n;
	for (n=0;n<300;n++) {
		double ls=n*LINELEN+offset;
		sum=sum+before_impulse(ls,5*USEC,error);
	}
	return sum;
}


double diff_field_sync(double error, double offset)
{
	double sum=0;
	int n;
	//equalistion pulses after sync
	for (n=0;n<5;n++) {
		sum=sum+before_impulse(n*32*USEC+offset, 2.5*USEC, error);
	}	
	//Sync pulses
	for (n=0;n<5;n++) {
		sum=sum+before_impulse(2.5*LINELEN+n*32*USEC+offset, 27*USEC, error);
	}
	//equalisation pulses before sync
	for (n=0;n<5;n++) {
		sum=sum+before_impulse(5*LINELEN+n*32*USEC+offset, 2.5*USEC, error);
	}
}


int main(int argc, char **argv)
{
	int n;
	for (n=0;n<BUFFERLEN;n++) field[n]=0; 
	double t=0;
	double x=0;
	double p=0;
	field_p=0;
	while (scanf("%lf %lf %lf", &t, &x, &p)==3) {
		field[field_p]=x;
		field_p=(field_p)%BUFFERLEN;
	}
	return 0;	
}

