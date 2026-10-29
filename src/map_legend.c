#include "map_legend.h"
#include <string.h>
/* Android 2.6.1 enlarges mobile legend text beyond the fixed legend panel.
 * Keep title at normal scale and item text at 0.9 to fit the French elite
 * label inside the Vita panel. Leave icons, hitboxes and input intact.
 * Validate both instruction blocks before changing either one. */
int map_legend_configure(ElfImage *im){
 static const char *names[]={
 "_ZN3com8megacrit9cardcrawl3map19Legend$_render_VoidEPNS2_6LegendEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchE",
 "_ZN3com8megacrit9cardcrawl3map23LegendItem$_render_VoidEPNS2_10LegendItemEPNS_8badlogic3gdx8graphics3g2d11SpriteBatchEPNS7_5ColorE"};
 static const uint32_t starts[]={0x1fe7f15,0x1fe9171},sizes[]={2096,1576},sites[]={0x1fe8198,0x1fe9490};
 static const unsigned lengths[]={8,10};
 static const unsigned char before[2][10]={
 {0x43,0xf2,0x33,0x30,0xc3,0xf6,0xb3,0x70},
 {0x49,0xf6,0x9a,0x11,0x10,0x44,0xc3,0xf6,0x99,0x71}};
 static const unsigned char after[2][10]={
 {0x40,0xf2,0,0,0xc3,0xf6,0x80,0x70},
 {0x46,0xf2,0x66,0x61,0x10,0x44,0xc3,0xf6,0x66,0x71}};
 if(!im||!im->relocated)return -1;
 for(unsigned i=0;i<2;i++){
  uint32_t value,size;
  if(elf_image_symbol(im,names[i],&value,&size)||value!=starts[i]||size!=sizes[i]||
     !elf_image_contains(im,sites[i],lengths[i],5)||memcmp(im->base+sites[i],before[i],lengths[i]))return -1;
 }
 for(unsigned i=0;i<2;i++)memcpy(im->base+sites[i],after[i],lengths[i]);
 return 0;
}
