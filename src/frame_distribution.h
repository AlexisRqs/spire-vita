#ifndef FRAME_DISTRIBUTION_H
#define FRAME_DISTRIBUTION_H
#include <stdint.h>
#include <stdlib.h>
static int frame_compare(const void *a,const void *b){uint32_t x=*(const uint32_t*)a,y=*(const uint32_t*)b;return (x>y)-(x<y);}
/* Call only on a complete, nonempty window. Nearest-rank percentiles. */
static void frame_distribution(uint32_t *values,unsigned n,uint32_t *p95,uint32_t *p99,uint32_t *max,unsigned *slow33,unsigned *slow50){
 *slow33=*slow50=0;
 for(unsigned i=0;i<n;i++){*slow33+=values[i]>33333;*slow50+=values[i]>50000;}
 qsort(values,n,sizeof(*values),frame_compare);
 *p95=values[(95*n+99)/100-1];*p99=values[(99*n+99)/100-1];*max=values[n-1];
}
#endif
