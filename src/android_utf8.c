#include "android_utf8.h"
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
_Static_assert(sizeof(AndroidMbState)==4,"Android LP32 mbstate ABI");
static pthread_key_t key;static pthread_once_t once=PTHREAD_ONCE_INIT;static int key_error;
static void init_key(void){key_error=pthread_key_create(&key,free);}
static AndroidMbState *private_state(unsigned index){
 pthread_once(&once,init_key);if(key_error)return NULL;
 AndroidMbState *s=pthread_getspecific(key);
 if(!s){s=calloc(5,sizeof(*s));if(!s)return NULL;if(pthread_setspecific(key,s)){free(s);return NULL;}}
 return s+index;
}
static size_t invalid(AndroidMbState *s){memset(s,0,sizeof(*s));errno=84;return (size_t)-1;}
int android_mbsinit(const AndroidMbState *s){return !s||!(s->bytes[0]|s->bytes[1]|s->bytes[2]|s->bytes[3]);}
static size_t decode(uint32_t *out,const char *input,size_t n,AndroidMbState *s){
 if(!input){input="";n=1;out=NULL;}
 if(!n)return (size_t)-2;
 unsigned char seq[4];memcpy(seq,s->bytes,4);unsigned have=0;
 while(have<4&&seq[have])have++;
 if(have==4)return invalid(s);
 size_t used=0;
 if(!have){seq[0]=(unsigned char)input[used++];have=1;}
 unsigned first=seq[0],need;uint32_t value;
 if(first<0x80){need=1;value=first;}
 else if(first>=0xc2&&first<=0xdf){need=2;value=first&31;}
 else if(first>=0xe0&&first<=0xef){need=3;value=first&15;}
 else if(first>=0xf0&&first<=0xf4){need=4;value=first&7;}
 else return invalid(s);
 if(have>need)return invalid(s);
 for(unsigned i=1;i<need;i++){
  if(i>=have){
   if(used==n){memset(s,0,sizeof(*s));memcpy(s->bytes,seq,i);return (size_t)-2;}
   seq[i]=(unsigned char)input[used++];have++;
  }
  unsigned c=seq[i];if(c<0x80||c>0xbf)return invalid(s);
  if(i==1&&((first==0xe0&&c<0xa0)||(first==0xed&&c>=0xa0)||(first==0xf0&&c<0x90)||(first==0xf4&&c>=0x90)))return invalid(s);
  value=(value<<6)|(c&63);
 }
 memset(s,0,sizeof(*s));if(out)*out=value;return value?used:0;
}
size_t android_mb_cur_max(void){return 4;}
size_t android_mbrtoc16(uint16_t *out,const char *input,size_t n,AndroidMbState *state){
 if(!state)state=private_state(4);
 if(!state){errno=12;return (size_t)-1;}
 if(!input)out=NULL;
 /* 0xff cannot begin a valid pending UTF-8 prefix. This state is opaque. */
 if(state->bytes[0]==0xff){
  uint16_t low=state->bytes[1]|((uint16_t)state->bytes[2]<<8);
  if(low<0xdc00||low>0xdfff||state->bytes[3])return invalid(state);
  memset(state,0,sizeof(*state));if(out)*out=low;return (size_t)-3;
 }
 uint32_t value=0;size_t used=decode(&value,input,n,state);
 if(used==(size_t)-1||used==(size_t)-2)return used;
 if(value<0x10000){if(out)*out=(uint16_t)value;return used;}
 value-=0x10000;
 if(out)*out=(uint16_t)(0xd800+(value>>10));
 uint16_t low=(uint16_t)(0xdc00+(value&1023));
 state->bytes[0]=0xff;state->bytes[1]=(unsigned char)low;state->bytes[2]=low>>8;state->bytes[3]=0;
 return used;
}
size_t android_mbrtowc(uint32_t *out,const char *s,size_t n,AndroidMbState *state){
 if(!state)state=private_state(0);
 if(!state){errno=12;return (size_t)-1;}return decode(out,s,n,state);
}
size_t android_mbrlen(const char *s,size_t n,AndroidMbState *state){
 if(!state)state=private_state(1);
 if(!state){errno=12;return (size_t)-1;}return decode(NULL,s,n,state);
}
size_t android_c32rtomb(char *out,uint32_t value,AndroidMbState *state){
 if(!state)state=private_state(2);
 if(!state){errno=12;return (size_t)-1;}
 if(!out||!value){memset(state,0,sizeof(*state));if(out)*out=0;return 1;}
 if(!android_mbsinit(state)||value>0x10ffff||(value>=0xd800&&value<=0xdfff))return invalid(state);
 unsigned count=value<0x80?1:value<0x800?2:value<0x10000?3:4;
 static const unsigned char lead[]={0,0,0xc0,0xe0,0xf0};
 for(unsigned i=count-1;i>0;i--){out[i]=0x80|(value&63);value>>=6;}
 out[0]=lead[count]|value;return count;
}
size_t android_c16rtomb(char *out,uint16_t value,AndroidMbState *state){
 if(!state)state=private_state(3);
 if(!state){errno=12;return (size_t)-1;}
 if(!out){memset(state,0,sizeof(*state));return 1;}
 uint32_t high=0;memcpy(&high,state,4);
 if(high){
  memset(state,0,sizeof(*state));
  if(high<0xd800||high>0xdbff||value<0xdc00||value>0xdfff){errno=22;return (size_t)-1;}
  return android_c32rtomb(out,0x10000+((high-0xd800)<<10)+(value-0xdc00),state);
 }
 if(value>=0xd800&&value<=0xdbff){high=value;memcpy(state,&high,4);return 0;}
 if(value>=0xdc00&&value<=0xdfff){errno=22;return (size_t)-1;}
 return android_c32rtomb(out,value,state);
}
uintptr_t android_utf8_lookup(const char *name){
 if(!strcmp(name,"__ctype_get_mb_cur_max"))return (uintptr_t)android_mb_cur_max;
 if(!strcmp(name,"mbrtoc16"))return (uintptr_t)android_mbrtoc16;
 if(!strcmp(name,"c16rtomb"))return (uintptr_t)android_c16rtomb;
 if(!strcmp(name,"c32rtomb"))return (uintptr_t)android_c32rtomb;
 if(!strcmp(name,"mbrlen"))return (uintptr_t)android_mbrlen;
 if(!strcmp(name,"mbrtowc"))return (uintptr_t)android_mbrtowc;
 if(!strcmp(name,"mbsinit"))return (uintptr_t)android_mbsinit;
 return 0;
}
