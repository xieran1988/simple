
#include "a.h"
#include <stdio.h>

void gl_draw_quads_revtex(float x, float y, float z, float w, float h)
{
	glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f(x, y+h, z);
		glTexCoord2f(0, 0); glVertex3f(x, y, z);
		glTexCoord2f(1, 0); glVertex3f(x+w, y, z);
		glTexCoord2f(1, 1); glVertex3f(x+w, y+h, z);
	glEnd();
}

void gl_draw_quads(float x, float y, float z, float w, float h)
{
	glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex3f(x, y+h, z);
		glTexCoord2f(0, 1); glVertex3f(x, y, z);
		glTexCoord2f(1, 1); glVertex3f(x+w, y, z);
		glTexCoord2f(1, 0); glVertex3f(x+w, y+h, z);
	glEnd();
}

void gl_checkerr(char *msg) 
{
	GLenum errCode;
	const GLubyte *errString;

	if ((errCode = glGetError()) != GL_NO_ERROR) {
		errString = gluErrorString(errCode);
		fprintf (stderr, "OpenGL Error: %s: %s\n", msg, errString);
	}
}


