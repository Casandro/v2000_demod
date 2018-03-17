#include <stdio.h>
#include <stdlib.h>

//Samplingrate in MHz
#define SFRQ (8)

//Length of a line in Pixels
#define LINELEN (64*SFRQ)

//Length of the sync pulse
#define SPLEN (38)

int *data=NULL;
int datasize=0;


int *read_image(const char *fn)
{
	FILE *f=fopen(fn, "r");
	int cnt=0;
	double min=1000;
	double max=-1000;
	double v;
	while (fscanf(f, "%lf", &v)==1) {
		cnt=cnt+1;
		if (v<min) min=v;
		if (v>max) max=v;
	}
	fclose(f);
	int *x=malloc(cnt*sizeof(int));
	max=1.22;
	min=0;
	if (x==NULL) return NULL;
	f=fopen(fn, "r");
	int n=0;
	while (fscanf(f, "%lf", &v)==1) {
		x[n]=((v-min)/(max-min))*256;
		if (x[n]>255) x[n]=255;
		if (x[n]<0) x[n]=0;
		n=n+1;
		if (n>=cnt) break;
	}
	data=x;
	datasize=cnt;
	return x;
}



int main(int argc, char **argv)
{
	if (argc!=2) {
		printf("%c filename", argv[0]);
		return 1;
	}
	read_image(argv[1]);
	int linecnt=(datasize/LINELEN-1);
	printf("P5 %d %d 255\n", LINELEN*2, linecnt);
	int y;
	for (y=0; y<linecnt; y++) {
		int x;
		int mx=SPLEN;
		int min=1000000;
		int sum=0;
		for (x=0; x<SPLEN; x++) sum=sum+data[y*LINELEN+x];
		mx=SPLEN; min=sum;
		for (x=SPLEN; x<LINELEN+SPLEN; x++) {
			if (sum<min) {
				mx=x;
				min=sum;
			}
			sum=sum-data[y*LINELEN+x-SPLEN]+data[y*LINELEN+x];
		}
//		printf("%d %d\n", min, mx);
		for (x=0; x<LINELEN*2; x++) {
			printf("%c", data[y*LINELEN+x]);
		}
	}
	return 0;
}
