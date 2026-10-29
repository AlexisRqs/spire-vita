/* Small crosshair; preserve all GL state touched by the overlay. */
static void graphics_draw_pointer(void){
 float x,y;if(!vita_frame_pointer(&x,&y))return;
 GLint framebuffer,box[4];GLfloat clear[4];GLboolean mask[4];
 GLboolean scissor=glIsEnabled(GL_SCISSOR_TEST);
 glGetIntegerv(GL_FRAMEBUFFER_BINDING,&framebuffer);glGetIntegerv(GL_SCISSOR_BOX,box);
 glGetFloatv(GL_COLOR_CLEAR_VALUE,clear);glGetBooleanv(GL_COLOR_WRITEMASK,mask);
 glBindFramebuffer(GL_FRAMEBUFFER,0);glEnable(GL_SCISSOR_TEST);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
 int px=(int)x,py=543-(int)y;
 glClearColor(0,0,0,1);
 glScissor(px-8,py-2,17,5);glClear(GL_COLOR_BUFFER_BIT);
 glScissor(px-2,py-8,5,17);glClear(GL_COLOR_BUFFER_BIT);
 glClearColor(1,1,1,1);
 glScissor(px-7,py-1,15,3);glClear(GL_COLOR_BUFFER_BIT);
 glScissor(px-1,py-7,3,15);glClear(GL_COLOR_BUFFER_BIT);
 glClearColor(clear[0],clear[1],clear[2],clear[3]);glColorMask(mask[0],mask[1],mask[2],mask[3]);
 glScissor(box[0],box[1],box[2],box[3]);if(!scissor)glDisable(GL_SCISSOR_TEST);
 glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
}
