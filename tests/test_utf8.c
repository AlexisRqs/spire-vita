#include "android_utf8.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
static unsigned encode(unsigned v,char *s){
 if(v<128){s[0]=v;return 1;}if(v<2048){s[0]=0xc0|(v>>6);s[1]=0x80|(v&63);return 2;}
 if(v<65536){s[0]=0xe0|(v>>12);s[1]=0x80|((v>>6)&63);s[2]=0x80|(v&63);return 3;}
 s[0]=0xf0|(v>>18);s[1]=0x80|((v>>12)&63);s[2]=0x80|((v>>6)&63);s[3]=0x80|(v&63);return 4;
}
static void *thread(void *p){(void)p;for(unsigned i=0;i<1000;i++){assert(android_mbrlen("\xe2",1,NULL)==(size_t)-2);assert(android_mbrlen("\x82\xac",2,NULL)==2);}return NULL;}
int main(void){
 struct {AndroidMbState state;unsigned guard;} s={{{0}},0x12345678};uint32_t out;
 for(unsigned v=0;v<=0x10ffff;v++){
  if(v>=0xd800&&v<=0xdfff)continue;char b[4];unsigned n=encode(v,b);char encoded[4];
  assert(android_c32rtomb(encoded,v,&s.state)==n);for(unsigned j=0;j<n;j++)assert(encoded[j]==b[j]);
  if(v>=65536){assert(android_c16rtomb(encoded,0xd800+((v-65536)>>10),&s.state)==0);assert(android_c16rtomb(encoded,0xdc00+((v-65536)&1023),&s.state)==4);}
  else assert(android_c16rtomb(encoded,v,&s.state)==n);
  for(unsigned j=0;j<n;j++)assert(encoded[j]==b[j]);
  uint16_t c16=0;
  assert(android_mbrtoc16(&c16,b,n,&s.state)==(v?n:0));
  if(v<65536)assert(c16==v);
  else {assert(c16==0xd800+((v-65536)>>10));assert(!android_mbsinit(&s.state));assert(android_mbrtoc16(&c16,"ignored",0,&s.state)==(size_t)-3);assert(c16==0xdc00+((v-65536)&1023));}
  assert(android_mbsinit(&s.state));
  assert(android_mbrtowc(&out,b,n,&s.state)==(v?n:0));assert(out==v);assert(android_mbsinit(&s.state));
  for(unsigned split=1;split<n;split++){
   assert(android_mbrtowc(&out,b,split,&s.state)==(size_t)-2);
   assert(!android_mbsinit(&s.state));assert(android_mbrtowc(&out,b+split,n-split,&s.state)==n-split);assert(out==v);
  }
 }
 const char *bad[]={"\xc0\x80","\xed\xa0\x80","\xf4\x90\x80\x80","\xe0\x80","\x80","\xff"};
 for(unsigned i=0;i<6;i++){assert(android_mbrlen(bad[i],2,&s.state)==(size_t)-1);assert(errno==84);assert(android_mbsinit(&s.state));}
 assert(android_mbrlen(NULL,0,&s.state)==0);assert(android_mbrlen("",0,&s.state)==(size_t)-2);
 assert(android_mbrlen("\xe2",1,&s.state)==(size_t)-2);assert(android_mbrlen(NULL,0,&s.state)==(size_t)-1);
 char output[4];
 assert(android_c16rtomb(output,0xdc00,&s.state)==(size_t)-1);assert(errno==22);
 assert(android_c16rtomb(output,0xd800,&s.state)==0);assert(!android_mbsinit(&s.state));
 assert(android_c16rtomb(output,'a',&s.state)==(size_t)-1);assert(errno==22);assert(android_mbsinit(&s.state));
 assert(android_c16rtomb(output,0xd800,&s.state)==0);assert(android_c16rtomb(NULL,0,&s.state)==1);assert(android_mbsinit(&s.state));
 assert(android_c32rtomb(output,0x110000,&s.state)==(size_t)-1);assert(errno==84);
 assert(android_c32rtomb(output,0xd800,&s.state)==(size_t)-1);assert(android_mbsinit(&s.state));
 assert(s.guard==0x12345678);
 pthread_t t[4];for(int i=0;i<4;i++)assert(!pthread_create(t+i,NULL,thread,NULL));for(int i=0;i<4;i++)assert(!pthread_join(t[i],NULL));
 return 0;
}
