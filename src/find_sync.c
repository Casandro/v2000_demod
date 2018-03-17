//Compile with -O3 for tail recursion optimisation

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <strings.h>

//Samplingrate in MHz
#define SFRQ (8)

//Length of a line in Pixels
#define LINELEN (64*SFRQ)

//Length of the sync pulse
#define SPLEN (38)
#define VSPLEN (218)

#define BLEN (1024*8)

double buffer[BLEN];

typedef struct sync_s {
	struct sync_s *prev;
	struct sync_s *next;
	double v; //Position
	double x; //Wert
	int number;
	int field;
} sync_t;


sync_t *add_sync_after(sync_t *p, sync_t *s)
{
//	fprintf(stderr, "add_sync_after %p %p\n", p,s);
	s->prev=p;
	if (p!=NULL) {
		s->next=p->next;
		s->prev=p;
		p->next=s;
		if (s->next!=NULL) {
			s->next->prev=s;
		}
	}
	return s;
}

sync_t *add_sync_before(sync_t *n, sync_t *s)
{
//	fprintf(stderr, "add_sync_before %p %p\n", n,s);
	if (s==NULL) return NULL;
	s->next=n;
	if (n!=NULL) {
		s->prev=n->prev;
		if (s->prev!=NULL) {
			s->prev->next=s;
		}
	}
	return s;
}

//Inserts a sync pulse at the right position in the sorted list
sync_t *insert_sync(sync_t *p, sync_t *s)
{
	if (p==NULL) return s;
	double pv=p->v;
	double sv=s->v;
	if (pv>sv) { //add somewhere before
		if (p->prev==NULL) return add_sync_before(p,s);
		if (p->prev->v<pv) return add_sync_before(p,s);
		return insert_sync(p->prev,s);
	}
	//Add it somewhere after p
	if (p->next==NULL) return add_sync_after(p,s);
	if (p->next->v<pv) return add_sync_after(p,s);
	return insert_sync(p->next,s);
}

sync_t *add_sync(sync_t *p,const double v, const double x)
{
//	fprintf(stderr,"add_sync %p %lf %lf\n", p, v, x);
	sync_t *s=malloc(sizeof(sync_t));
	memset (s,0,sizeof(sync_t));
	if (s==NULL) return p;
	s->v=v;
	s->x=x;
	s->number=-1;
	s->field=-1;
	if (p==NULL) return s;
	return insert_sync(p,s);
}

sync_t *delete_sync(sync_t *p)
{
	sync_t *n=p->prev;
	if (p->prev!=NULL) {
		p->prev->next=p->next;
	}
	if (p->next!=NULL) {
		p->next->prev=p->prev;
		n=p->next;
	}
	free(p);
	return n;
}


sync_t *cleanup_syncs(sync_t *s, const double len)
{
//	fprintf(stderr,"cleanup_sync %p %p\n", s, s->next);
	if (s==NULL) return NULL;
	if (s->next==NULL) return s;
	sync_t *next=s->next;
	double d=fabs(s->v - s->next->v);
	if (d<len/2) { //to many sync pulses, pick best one
		double a=fabs(s->x);
		double b=fabs(s->next->x);
//		fprintf(stderr, "cleanup_sync delete %lf %lf %lf\n", d, a, b);
		if (a>b) {
//		fprintf(stderr, "cleanup_sync delete b %p\n", s->next);
			delete_sync(s->next);
			next=s;
		} else {
//		fprintf(stderr, "cleanup_sync delete a %p\n", s);
			next=delete_sync(s);
		}
	} else if (d>len*1.5) { //to few sync pulses, add one
		double dc=d/len;
		double r=dc-floor(dc);
		fprintf(stderr, "cleanup_sync %lf\n", dc);
		if ( (r<0.3) || (r>0.7) ) {
			int cnt=floor(dc);
			double dist=d/cnt;
			next=add_sync(s,s->v+dist, 0);
		}
		
	}
	return cleanup_syncs(next, len);
}

sync_t *first_sync(sync_t *s)
{
//	fprintf(stderr,"first_sync\n");
	if (s==NULL) return NULL;
	if (s->prev==NULL) return s;
	return first_sync(s->prev);
}
sync_t *cleanup_rewind(sync_t *s, const int len)
{
	sync_t *a=first_sync(s);
	return first_sync(cleanup_syncs(a,len));
}

sync_t *smooth_sync(sync_t *s)
{
	if (s->next==NULL) return s;
	if (s->prev==NULL) return smooth_sync(s->next);
	double d=s->next->v - s->prev->v;
	double f=(d/LINELEN)-floor(d/LINELEN);
	if ((f<0.2) || (f>0.8) ) {
		double m=(s->next->v + s->prev->v)/2;
		s->v=s->v*0.5+m*0.5;
	}
	return smooth_sync(s->next);
}

double phase(const double x)
{
	double d=x/LINELEN;
	return (d-floor(d))*LINELEN;
}

double avg_sync_phase(const sync_t *s, const double sum, const int cnt)
{
	if (s==NULL) return sum/cnt;
	double ph=phase(s->v);
	return avg_sync_phase(s->next, ph+sum, cnt+1);
}

void count_field(sync_t *v, const double avgphase, const int frame)
{
	if (v==NULL) return;
	double ph=phase(v->v+avgphase);
	int field=(ph/(LINELEN/2)+0.5);
	field=(field)%2;
	v->number=frame*2+field;
//	fprintf(stderr, "count_field %p, %d, %d %lf\n", v, v->number,field, ph);
	return count_field(v->next, avgphase, frame+field);
}


void count_fields(sync_t *v, sync_t *h)
{
	double avgph=avg_sync_phase(first_sync(h), 0,0);
//	fprintf(stderr, "count_fields avg_phase=%lf\n", avgph);
	count_field(first_sync(v), avgph, 0);
}

void count_line(sync_t *h, const sync_t *v)
{
	if (h==NULL) return;
	double fs=v->v-(v->number%2)*LINELEN/2; //calculate Field start
	if ( (fs<h->v) && (v->next!=NULL) && ((v->next->v) < (h->v)) ) {
		//fprintf(stderr, "count_line next field %lf\n", fs);
		return count_line(h, v->next);
	}
	int lnum=((h->v-fs)/LINELEN+0.5);
	int field=v->number;
	if (lnum<0) field=field-1;
	while (lnum<0) lnum=lnum+312;
//	while (lnum>312) lnum=lnum-312;
	h->number=lnum*2+(v->number%2);
	h->field=field;
	return count_line(h->next, v);
}

void count_lines(sync_t *v, const sync_t *h)
{
	sync_t *hf=first_sync(h);
	sync_t *vf=first_sync(v);
	count_line(hf, vf);
}

void print_syncs(const sync_t *s, const int cnt)
{
	if (s==NULL) return;
	double d=0;
	if (s->next!=NULL) d=s->next->v - s->v;
	printf("%d %lf %lf %d\n", s->number, s->v, d, s->field);
	return print_syncs(s->next, cnt+LINELEN);
}

int count_syncs(const sync_t *s, const int cnt)
{
	if (s==NULL) return cnt;
	return count_syncs(s->next, cnt+1);
}

double getb(const int n)
{
	return buffer[(n+BLEN)%BLEN];
}

int main(int argc, char **argv)
{
	int n;
	for (n=0;n<BLEN;n++) buffer[n]=0;
	double x=0;
	double h_sum=0;
	double v_sum=0;
	int cnt=0;
	sync_t *h_syncs=NULL;
	sync_t *v_syncs=NULL;

	while (scanf("%lf", &x)==1) {
		buffer[cnt%BLEN]=x;
		h_sum=h_sum+x-getb(cnt-SPLEN);
		if (h_sum<-0.05*SPLEN) h_syncs=add_sync(h_syncs,cnt-SPLEN,h_sum);
	
		int n;
		for (n=0; n<5; n++) {
			int p=cnt-n*LINELEN/2;
			v_sum=v_sum+getb(p)-getb(p-VSPLEN);
		}
		if (v_sum<-0.05*VSPLEN*5) v_syncs=add_sync(v_syncs,cnt-LINELEN/2*5,v_sum);
		cnt=(cnt+1);
	}
	h_syncs=cleanup_rewind(h_syncs,LINELEN);
	v_syncs=cleanup_rewind(v_syncs,LINELEN*312.5);
	h_syncs=first_sync(h_syncs);

	count_fields(v_syncs, h_syncs);
	count_lines (v_syncs, h_syncs);
	int cnt1=count_syncs(h_syncs,0);
	
	int cnt2=count_syncs(v_syncs,0);
	smooth_sync(h_syncs);
	fprintf(stderr, "%d %d\n", cnt1, cnt2);
	print_syncs(h_syncs, 0);
	return 0;
}
