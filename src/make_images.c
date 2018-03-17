#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define HEIGHT (625/2)
#define WIDTH (512)

#define BLACK (0)
#define WHITE (1.22)

#define IMGSIZE (WIDTH*HEIGHT)

uint8_t image[IMGSIZE];

void clear_image()
{
	int x;
	int y;
	for (y=0; y<HEIGHT; y++) 
	for (x=0; x<WIDTH; x++) {
		int p=y*WIDTH+x;
		int m=((x/4)%2+(y/4)%2)%2;
		image[p]=m*255;
	}
}

void write_image_to_file(const char *fn)
{
	FILE *f=fopen(fn, "w");
	fprintf(f,"P5 %d %d 255\n", WIDTH, HEIGHT);
	int n;
	for (n=0;n<IMGSIZE;n++) {
		fprintf(f,"%c", image[n]);
	}
	fclose(f);
}


void write_line(const int line, double *d)
{
	if (line>=HEIGHT) return;
	fprintf(stderr, "write_line %d\n", line);
	int n;
	for (n=0;n<WIDTH;n++) {
		int p=line*WIDTH+n;
		int x=((double)(d[n]-BLACK))*256.0/(WHITE-BLACK);
		if (x<0) x=0;
		if (x>255) x=255;
		image[p]=x;
	}
}

double *data=NULL;
int datalen=-1;


void load_data(const char *fn)
{
	fprintf(stderr, "load_data %s\n",fn);
	FILE *f=fopen(fn, "r");
	int cnt=0;
	double x;
	while (fscanf(f,"%lf\n",&x)==1) cnt=cnt+1;
	fclose(f);
	fprintf(stderr, "%d values\n",cnt);
	data=malloc(cnt*sizeof(data[0]));
	if (data==NULL) return;
	datalen=cnt;

	f=fopen(fn, "r");
	int n=0;
	while (fscanf(f,"%lf",&x)==1) {
		data[n]=x;
		n=n+1;
		if (n>=cnt) break;
	}
	fclose(f);
	fprintf(stderr, "in memory\n");
}


void flushimage(char *bn, int frame)
{
	char buffer[256];
	snprintf(buffer,255,"%s-%05d.pgm",bn, frame);
	fprintf(stderr, "writing %s\n", buffer);
	write_image_to_file(buffer);
	clear_image();
}

int main(int argc, char **argv)
{
	if (argc!=2) return 1;
	load_data(argv[1]);
	int line=0;
	double pos=0;
	double len=0;
	int field=0;
	int aframe=0;
	clear_image();
	while (scanf("%d %lf %lf %d", &line, &pos, &len, &field)==4) {
		if (field!=aframe) {
			flushimage(argv[1],aframe);
			aframe=field;
		}
		int p=pos+0.5;
		if (p+WIDTH<datalen) {
			write_line(line/2, &(data[p]));
		}
	}
	flushimage(argv[1],aframe);
}
