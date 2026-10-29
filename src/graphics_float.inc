/* Generated from vitaGL headers: base AAPCS guest to native VFP ABI. */
static ANDROID_ABI void bridge_glAlphaFunc(GLenum func, GLfloat ref){glAlphaFunc(func,ref);}
static ANDROID_ABI void bridge_glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha){glClearColor(red,green,blue,alpha);}
static ANDROID_ABI void bridge_glClearDepth(GLdouble depth){glClearDepth(depth);}
static ANDROID_ABI void bridge_glClearDepthf(GLclampf depth){glClearDepthf(depth);}
static ANDROID_ABI void bridge_glColor3f(GLfloat red, GLfloat green, GLfloat blue){glColor3f(red,green,blue);}
static ANDROID_ABI void bridge_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha){glColor4f(red,green,blue,alpha);}
static ANDROID_ABI void bridge_glDepthRange(GLdouble nearVal, GLdouble farVal){glDepthRange(nearVal,farVal);}
static ANDROID_ABI void bridge_glDepthRangef(GLfloat nearVal, GLfloat farVal){glDepthRangef(nearVal,farVal);}
static ANDROID_ABI void bridge_glFogf(GLenum pname, GLfloat param){glFogf(pname,param);}
static ANDROID_ABI void bridge_glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal){glFrustum(left,right,bottom,top,nearVal,farVal);}
static ANDROID_ABI void bridge_glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal){glFrustumf(left,right,bottom,top,nearVal,farVal);}
static ANDROID_ABI void bridge_glLineWidth(GLfloat width){glLineWidth(width);}
static ANDROID_ABI void bridge_glMaterialf(GLenum face, GLenum pname, GLfloat param){glMaterialf(face,pname,param);}
static ANDROID_ABI void bridge_glMatrixFrustum(GLenum matrixMode, GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f){glMatrixFrustum(matrixMode,l,r,b,t,n,f);}
static ANDROID_ABI void bridge_glMatrixOrtho(GLenum matrixMode, GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f){glMatrixOrtho(matrixMode,l,r,b,t,n,f);}
static ANDROID_ABI void bridge_glMatrixRotated(GLenum matrixMode, GLdouble angle, GLdouble x, GLdouble y, GLdouble z){glMatrixRotated(matrixMode,angle,x,y,z);}
static ANDROID_ABI void bridge_glMatrixRotatef(GLenum matrixMode, GLfloat angle, GLfloat x, GLfloat y, GLfloat z){glMatrixRotatef(matrixMode,angle,x,y,z);}
static ANDROID_ABI void bridge_glMatrixScaled(GLenum matrixMode, GLdouble x, GLdouble y, GLdouble z){glMatrixScaled(matrixMode,x,y,z);}
static ANDROID_ABI void bridge_glMatrixScalef(GLenum matrixMode, GLfloat x, GLfloat y, GLfloat z){glMatrixScalef(matrixMode,x,y,z);}
static ANDROID_ABI void bridge_glMatrixTranslated(GLenum matrixMode, GLdouble x, GLdouble y, GLdouble z){glMatrixTranslated(matrixMode,x,y,z);}
static ANDROID_ABI void bridge_glMatrixTranslatef(GLenum matrixMode, GLfloat x, GLfloat y, GLfloat z){glMatrixTranslatef(matrixMode,x,y,z);}
static ANDROID_ABI void bridge_glMultiTexCoord2f(GLenum target, GLfloat s, GLfloat t){glMultiTexCoord2f(target,s,t);}
static ANDROID_ABI void bridge_glNormal3f(GLfloat x, GLfloat y, GLfloat z){glNormal3f(x,y,z);}
static ANDROID_ABI void bridge_glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal){glOrtho(left,right,bottom,top,nearVal,farVal);}
static ANDROID_ABI void bridge_glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal){glOrthof(left,right,bottom,top,nearVal,farVal);}
static ANDROID_ABI void bridge_glPointSize(GLfloat size){glPointSize(size);}
static ANDROID_ABI void bridge_glPolygonOffset(GLfloat factor, GLfloat units){glPolygonOffset(factor,units);}
static ANDROID_ABI void bridge_glProgramUniform1f(GLuint program, GLint location, GLfloat v0){glProgramUniform1f(program,location,v0);}
static ANDROID_ABI void bridge_glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1){glProgramUniform2f(program,location,v0,v1);}
static ANDROID_ABI void bridge_glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2){glProgramUniform3f(program,location,v0,v1,v2);}
static ANDROID_ABI void bridge_glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3){glProgramUniform4f(program,location,v0,v1,v2,v3);}
static ANDROID_ABI void bridge_glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2){glRectf(x1,y1,x2,y2);}
static ANDROID_ABI void bridge_glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z){glRotated(angle,x,y,z);}
static ANDROID_ABI void bridge_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z){glRotatef(angle,x,y,z);}
static ANDROID_ABI void bridge_glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param){glSamplerParameterf(sampler,pname,param);}
static ANDROID_ABI void bridge_glScaled(GLdouble x, GLdouble y, GLdouble z){glScaled(x,y,z);}
static ANDROID_ABI void bridge_glScalef(GLfloat x, GLfloat y, GLfloat z){glScalef(x,y,z);}
static ANDROID_ABI void bridge_glTexCoord2f(GLfloat s, GLfloat t){glTexCoord2f(s,t);}
static ANDROID_ABI void bridge_glTexEnvf(GLenum target, GLenum pname, GLfloat param){glTexEnvf(target,pname,param);}
static ANDROID_ABI void bridge_glTexParameterf(GLenum target, GLenum pname, GLfloat param){glTexParameterf(target,pname,param);}
static ANDROID_ABI void bridge_glTextureParameterf(GLuint texture, GLenum pname, GLfloat param){glTextureParameterf(texture,pname,param);}
static ANDROID_ABI void bridge_glTranslated(GLdouble x, GLdouble y, GLdouble z){glTranslated(x,y,z);}
static ANDROID_ABI void bridge_glTranslatef(GLfloat x, GLfloat y, GLfloat z){glTranslatef(x,y,z);}
static ANDROID_ABI void bridge_glUniform1f(GLint location, GLfloat v0){glUniform1f(location,v0);}
static ANDROID_ABI void bridge_glUniform2f(GLint location, GLfloat v0, GLfloat v1){glUniform2f(location,v0,v1);}
static ANDROID_ABI void bridge_glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2){glUniform3f(location,v0,v1,v2);}
static ANDROID_ABI void bridge_glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3){glUniform4f(location,v0,v1,v2,v3);}
static ANDROID_ABI void bridge_glVertex2d(GLdouble x, GLdouble y){glVertex2d(x,y);}
static ANDROID_ABI void bridge_glVertex2f(GLfloat x, GLfloat y){glVertex2f(x,y);}
static ANDROID_ABI void bridge_glVertex3d(GLdouble x, GLdouble y, GLdouble z){glVertex3d(x,y,z);}
static ANDROID_ABI void bridge_glVertex3f(GLfloat x, GLfloat y, GLfloat z){glVertex3f(x,y,z);}
static ANDROID_ABI void bridge_glVertexAttrib1f(GLuint index, GLfloat v0){glVertexAttrib1f(index,v0);}
static ANDROID_ABI void bridge_glVertexAttrib2f(GLuint index, GLfloat v0, GLfloat v1){glVertexAttrib2f(index,v0,v1);}
static ANDROID_ABI void bridge_glVertexAttrib3f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2){glVertexAttrib3f(index,v0,v1,v2);}
static ANDROID_ABI void bridge_glVertexAttrib4f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3){glVertexAttrib4f(index,v0,v1,v2,v3);}
static ANDROID_ABI void bridge_gluLookAt(GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ, GLdouble centerX, GLdouble centerY, GLdouble centerZ, GLdouble upX, GLdouble upY, GLdouble upZ){gluLookAt(eyeX,eyeY,eyeZ,centerX,centerY,centerZ,upX,upY,upZ);}
static ANDROID_ABI void bridge_gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar){gluPerspective(fovy,aspect,zNear,zFar);}
