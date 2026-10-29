#include "grid_batch_contract.h"
#include <assert.h>
#include <string.h>
static void put(unsigned char *p,unsigned at,uint32_t v){memcpy(p+at,&v,4);}
static uint32_t get(const unsigned char *p,unsigned at){uint32_t v;memcpy(&v,p+at,4);return v;}
int main(void){
 unsigned char a[92]={0},b[92],c[92],original[92];GridBatchPatch patch;
 put(a,0,1);put(a,8,2);put(a,12,3);a[32]=1;put(a,80,20);put(a,84,100);
 memcpy(b,a,92);put(b,20,4);put(b,24,0x3c000000);put(b,28,0x3d000000);put(b,72,0xfeffffff);put(b,80,192);put(b,84,272);
 memcpy(c,b,92);put(c,20,9);put(c,24,0x3f800000);put(c,72,0xfe000000);put(c,80,193);put(c,84,273);memcpy(original,c,92);
 assert(grid_batch_prepare(&patch,a,b,c));assert(!memcmp(c,original,92));grid_batch_apply(c,&patch);
 assert(get(c,20)==4&&get(c,24)==0x3c000000&&get(c,28)==0x3d000000&&get(c,72)==0xfeffffff&&get(c,80)==365&&get(c,84)==445);
 for(unsigned at=0;at<92;at++){
  int allowed=(at>=20&&at<32)||(at>=72&&at<76)||(at>=80&&at<88);
  if(allowed)continue;
  unsigned char x[92];memcpy(x,b,92);x[at]^=1;assert(!grid_batch_prepare(&patch,a,x,original));
  memcpy(x,original,92);x[at]^=1;assert(!grid_batch_prepare(&patch,a,b,x));
 }
 put(b,84,273);assert(!grid_batch_prepare(&patch,a,b,original));put(b,84,272);
 put(b,80,5020);put(b,84,5100);assert(!grid_batch_prepare(&patch,a,b,original));
 /* begin() reset semantics use the captured final renderCalls, while the
  * lifetime counter still increases by total submissions. */
 put(a,80,20);put(a,84,100);put(b,80,3);put(b,84,272);put(original,80,4);put(original,84,273);
 assert(grid_batch_prepare(&patch,a,b,original));assert(patch.words[4]==3&&patch.words[5]==445);
 /* A wrapped per-begin counter is ambiguous and must fall back. */
 put(a,80,UINT32_MAX-2);put(a,84,UINT32_MAX-4);put(b,80,2);put(b,84,0);put(original,80,UINT32_MAX);put(original,84,3);
 assert(!grid_batch_prepare(&patch,a,b,original));
 assert(!grid_batch_prepare(NULL,a,b,c));return 0;
}
