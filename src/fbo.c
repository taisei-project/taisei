
#include "fbo.h"
#include "global.h"
#include "taisei_err.h"

void init_fbo(FBO *fbo) {
	if(!GLEW_ARB_framebuffer_object) {
		warnx("missing FBO support. seriously.");
		tconfig.intval[NO_SHADER] = 1;
		return;
	}
	
	glGenTextures(1, &fbo->tex);
	glBindTexture(GL_TEXTURE_2D, fbo->tex);
	
	fbo->nw = 2;
	fbo->nh = 2;
	
	while(fbo->nw < VIEWPORT_W) fbo->nw *= 2;
	while(fbo->nh < VIEWPORT_H) fbo->nh *= 2;
		
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fbo->nw, fbo->nh, 0, GL_BGR, GL_UNSIGNED_BYTE, NULL);
	
	glGenFramebuffersEXT(1,&fbo->fbo);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo->fbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fbo->tex, 0);
	
	glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
	
	glGenRenderbuffersEXT(1, &fbo->depth);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, fbo->depth);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, fbo->nw, fbo->nh);

	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, fbo->depth);
	
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void draw_fbo_viewport(FBO *fbo) {
	glPushMatrix();
	glTranslatef(-fbo->nw+VIEWPORT_W,-fbo->nh+VIEWPORT_H+VIEWPORT_Y,0);
		
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, fbo->tex);
	glBegin(GL_QUADS);
		glTexCoord2f(0,1); glVertex3f(0, 0, 0);
		glTexCoord2f(0,0); glVertex3f(0, fbo->nh, 0);
		glTexCoord2f(1,0); glVertex3f(fbo->nw, fbo->nh, 0);
		glTexCoord2f(1,1); glVertex3f(fbo->nw, 0, 0);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	
	glUseProgramObjectARB(0);
	glPopMatrix();
}