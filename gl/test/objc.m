
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

	/*

	const GLubyte* strExt = glGetString(GL_EXTENSIONS); 
	GLboolean fboSupported = gluCheckExtension((const GLubyte*)"GL_EXT_framebuffer_object", strExt); 

	GLuint  renderBuffer = 0; 
	GLuint  depthBuffer = 0; 
	int     img_width = 128, img_height = 128; 

	glGenRenderbuffersEXT(1, &depthBuffer); 
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthBuffer); 
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24,img_width, img_height); 
	REPORTGLERROR("creating depth render buffer");

	glGenRenderbuffersEXT(1, &renderBuffer); 
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderBuffer); 
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, img_width,img_height); 
	REPORTGLERROR("creating color render buffer"); 

	GLuint  fbo = 0; 
	glGenFramebuffersEXT(1, &fbo); 

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo); 
	REPORTGLERROR("binding framebuffer");

	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, renderBuffer); 
	REPORTGLERROR("specifying color render buffer");

	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) !=GL_FRAMEBUFFER_COMPLETE_EXT)
		REPORT_ERROR_AND_EXIT("Problem with OpenGL framebuffer afterspecifying color render buffer.");

	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depthBuffer); 
	REPORTGLERROR("specifying depth render buffer");

	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) !=GL_FRAMEBUFFER_COMPLETE_EXT)
		REPORT_ERROR_AND_EXIT("Problem with OpenGL framebuffer afterspecifying depth render buffer.");

	glEnable(GL_DEPTH_TEST); 
	REPORTGLERROR("enabling depth testing");

	glClearColor(0.0, 0.0, 0.0, 1.0); 
	REPORTGLERROR("specifying clear color");

	glViewport(0, 0, img_width, img_height); 
	REPORTGLERROR("specifying viewport"); 

	glMatrixMode(GL_PROJECTION); 
	glLoadIdentity(); 
	glOrtho(-img_width / 2, img_width / 2, -img_height / 2, img_height /2, -1, 1);

	glMatrixMode(GL_MODELVIEW); 
	glLoadIdentity(); 
	REPORTGLERROR("setting up view/model matrices");

	glClear(GL_COLOR_BUFFER_BIT); 
	REPORTGLERROR("clearing color buffer"); 

	glClear(GL_DEPTH_BUFFER_BIT); 
	REPORTGLERROR("clearing depth buffer");

	glBegin(GL_TRIANGLES); 
	glColor3f(1.0, 0.0, 0.0); 
	glVertex3f(0.0, 60.0, 0.0); 
	glColor3f(0.0, 1.0, 0.0); 
	glVertex3f(40.0, -40.0, 0.0); 
	glColor3f(0.0, 0.0, 1.0); 
	glVertex3f(-40.0, -40.0, 0.0); 
	glEnd(); 
	REPORTGLERROR("rendering scene"); 

	int samplesPerPixel = 4; // R, G, B and A 
	int rowBytes = samplesPerPixel * img_width; 
	char* bufferData = (char*)malloc(rowBytes * img_height); 
	NULL_ERROR_EXIT(bufferData, "Unable to allocate buffer for imageextraction."); 

	glReadPixels(0, 0, img_width, img_height, GL_BGRA, GL_UNSIGNED_BYTE,bufferData); 
	REPORTGLERROR("reading pixels from framebuffer");

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); 

	CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB); 
	CGBitmapInfo bitmapInfo = kCGImageAlphaNoneSkipFirst |kCGBitmapByteOrder32Little;    // XRGB Little Endian 
	int bitsPerComponent = 8; 
	CGContextRef contextRef = CGBitmapContextCreate(bufferData, 
			img_width, img_height, bitsPerComponent, rowBytes,colorSpace, bitmapInfo); 
	NULL_ERROR_EXIT(contextRef, "Unable to create CGContextRef.");

	CGImageRef imageRef = CGBitmapContextCreateImage(contextRef); 
	NULL_ERROR_EXIT(imageRef, "Unable to create CGImageRef."); 

	Boolean isDirectory = false; 
	CFURLRef fileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, 
			CFSTR("/tmp/k.jpg"), kCFURLPOSIXPathStyle, isDirectory); 
	NULL_ERROR_EXIT(fileURL, "Unable to create file URL ref."); 

	CFIndex                 fileImageIndex = 1; 
	CFMutableDictionaryRef  fileDict       = NULL; 
	CFStringRef             fileUTType     = kUTTypeJPEG; 

	CGImageDestinationRef imageDest =CGImageDestinationCreateWithURL(fileURL, 
			fileUTType, 
			fileImageIndex, 
			fileDict); 

	NULL_ERROR_EXIT(imageDest, "Unable to createCGImageDestinationRef."); 

	CFIndex capacity = 1; 
	CFMutableDictionaryRef imageProps =CFDictionaryCreateMutable(kCFAllocatorDefault, 
			capacity, 
			&kCFTypeDictionaryKeyCallBacks, 
			&kCFTypeDictionaryValueCallBacks); 

	CGImageDestinationAddImage(imageDest, imageRef, imageProps); 
	CGImageDestinationFinalize(imageDest); 

	free(bufferData); 

	CFRelease(imageDest); 
	CFRelease(fileURL); 
	CFRelease(imageProps); 
	CGColorSpaceRelease( colorSpace ); 
	CGImageRelease(imageRef); 

	*/

	[openGLContext clearDrawable]; 
	[openGLContext release]; 
	[pool release];

	return 0;
}

