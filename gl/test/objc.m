
#include <av/encdec/a.h>
#include "a.h"

#include <stdio.h>         // for error output 
#import <OpenGL/OpenGL.h> 
#import <OpenGL/glu.h>      // for gluCheckExtension 
#import <AppKit/AppKit.h>   // for NSOpenGL...

#define REPORTGLERROR(task) { \
	GLenum tGLErr = glGetError(); \
	if (tGLErr!= GL_NO_ERROR) { \
		printf("GLErr: %s\n", task);	\
	}	\
} 
#define REPORT_ERROR_AND_EXIT(desc) { printf("%s\n", desc); return 1; } 
#define NULL_ERROR_EXIT(test, desc) { if (!test) REPORT_ERROR_AND_EXIT(desc); }

int main (int argc, char * const argv[]) 
{
	NSAutoreleasePool *pool = [NSAutoreleasePool new];
	NSOpenGLPixelFormatAttribute attributes[] = { 
		NSOpenGLPFAPixelBuffer, 
		NSOpenGLPFANoRecovery, 
		NSOpenGLPFAAccelerated, 
		NSOpenGLPFADepthSize, 24, 
		(NSOpenGLPixelFormatAttribute) 0 
	}; 
	NSOpenGLPixelFormat *pixFormat = [[[NSOpenGLPixelFormat alloc] 
		initWithAttributes:attributes]autorelease]; 

	NSOpenGLContext *openGLContext; 
  openGLContext = [[NSOpenGLContext alloc] 
		initWithFormat: pixFormat
		shareContext: nil
		]; 
	NULL_ERROR_EXIT(openGLContext, "Unable to create NSOpenGLContext");

	[openGLContext makeCurrentContext];

	all_init(640, 360);

	all_ctrl("t");
	while (1) 
		all_render();

	[openGLContext clearDrawable]; 
	[openGLContext release]; 
	[pool release];

	return 0;
}

