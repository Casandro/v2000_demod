#include <stdio.h>
#include <math.h>
#include <fftw3.h>

#define KHZ (1000)
#define MHZ (1000*KHZ)
#define SRATE (16*MHZ)

#define SEC (SRATE)
#define MSEC (SEC/1000)
#define USEC (MSEC/1000)

#define LINELEN (64*USEC)

#define CORRLEN (8192)

#define SYNCMID (CORRLEN/2)

#define BLEN (256*1024)

fftw_complex *vsync_f=NULL;

double *input_t=NULL;
fftw_complex *input_f=NULL;
fftw_plan input_to_f;

fftw_complex *corr_f=NULL;
double *corr_t=NULL;
fftw_plan corr_to_t;

double buffer[BLEN];
int bpos=0;

double fwindow(int x)
{
	double d=(x-SYNCMID)/CORRLEN*2; //-1..1
	d=fabs(d);
	return 1+cos(x*M_PI);
}

void fillvalue(int x, int w, double *d, double v)
{
	int n;
	for (n=x; n<x+w; n++) d[n]=v;
}

void create_sync_and_plans()
{
	fprintf(stderr, "create_sync_and_plans");
	double *ref=(double*) fftw_malloc(sizeof(double)*CORRLEN); 
	fprintf(stderr, ".");
	int n;
	fillvalue(0,CORRLEN, ref, 0);
	for (n=0; n<5; n++) {
		fillvalue(SYNCMID+n*32*USEC, 27*USEC, ref, -1);
	}
	for (n=0; n<5; n++) {
		fillvalue(SYNCMID+160+n*32*USEC, 2.5*USEC, ref, -1);
		fillvalue(SYNCMID-160+n*32*USEC, 2.5*USEC, ref, -1);
	}
	for (n=0; n<CORRLEN; n++) ref[n]=ref[n]*fwindow(n);
	fprintf(stderr, ".");
	vsync_f=(fftw_complex*) fftw_malloc(sizeof(fftw_complex) * CORRLEN);
	fprintf(stderr, ".");
	fftw_plan p=fftw_plan_dft_r2c_1d(CORRLEN, ref, vsync_f, FFTW_ESTIMATE);
	fprintf(stderr, ".");
	fftw_execute(p);
	fprintf(stderr, ".");
	fftw_destroy_plan(p);
	fftw_free(ref);
	
	fprintf(stderr, ".");
	input_t=(double*) fftw_malloc(sizeof(double)*CORRLEN);
	input_f=(fftw_complex*) fftw_malloc(sizeof(fftw_complex)*CORRLEN);
	input_to_f=fftw_plan_dft_r2c_1d(CORRLEN, input_t, input_f, FFTW_MEASURE|FFTW_DESTROY_INPUT);
	
	fprintf(stderr, ".");
	corr_f=(fftw_complex*) fftw_malloc(sizeof(fftw_complex)*CORRLEN);
	corr_t=(double*) fftw_malloc(sizeof(double)*CORRLEN);
	corr_to_t=fftw_plan_dft_c2r_1d(CORRLEN, corr_f, corr_t, FFTW_MEASURE|FFTW_DESTROY_INPUT);
	fprintf(stderr, "done\n");
}

void destroy_plans_and_data()
{
	fftw_free(vsync_f);
	fftw_free(input_t);
	fftw_free(input_f);
	fftw_destroy_plan(input_to_f);
	fftw_free(corr_f);
	fftw_free(corr_t);
	fftw_destroy_plan(corr_to_t);
}


/*Calculates a cross correlation between vsync_f and input_t
output in corr_f
*/
int correlate()
{
	fftw_execute(input_to_f);
	int n;
	for (n=0; n<CORRLEN; n++) {
		double a=vsync_f[n][0];
		double b=vsync_f[n][1]*-1;
		double c=input_f[n][0];
		double d=input_f[n][1];
		corr_f[n][0]=(a*c-b*d)/CORRLEN;
		corr_f[n][1]=(a*d+b*c)/CORRLEN;
	}
	fftw_execute(corr_to_t);
	int maxn=-1;
	double max=-1e50;
	for (n=0; n<CORRLEN; n++) {
		if (corr_t[n]>max) {
			max=corr_t[n];
			maxn=n;
		}
	}
	return maxn;
}

int read_values(int cnt)
{
	int n=0;
	for (n=0; n<cnt; n++) {
		double t=0;
		double x=0;
		double p=0;
		if (scanf("%lf%lf%lf",&t, &x, &p)!=3) return n;
		if (isnan(x)) x=0;
		buffer[bpos]=x;
		bpos=(bpos+1)%BLEN;
	}
	return cnt;
}

int main(int argc, char **argv) 
{
	create_sync_and_plans();
	while (read_values(CORRLEN/8)!=0) {
		int n;
		for (n=0; n<CORRLEN; n++) input_t[n]=buffer[(n+bpos-CORRLEN+BLEN)%BLEN];
		int max=correlate();
		printf("%d %lf\n", max, corr_t[max]/CORRLEN);
	}
	destroy_plans_and_data();
	return 0;
}


