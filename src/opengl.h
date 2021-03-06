#pragma once

#ifdef EMSCRIPTEN
#	include <GLES2/gl2.h>
#	include <GLES2/gl2ext.h>
#	define OP_GL_MAJOR_VERSION 2
#	define OP_GL_MINOR_VERSION 0
#	define OP_GL_USE_ES
#	define FORMAT_GREYSCALE GL_ALPHA
#	define INTERNAL_FORMAT_GREYSCALE GL_ALPHA
#	define GL_RGB8 GL_RGB
#	define GL_RGBA8 GL_RGBA
#else
#	include <GL/glew.h>
#	define OP_GL_MAJOR_VERSION 3
#	define OP_GL_MINOR_VERSION 0
#	define FORMAT_GREYSCALE GL_LUMINANCE
#	define INTERNAL_FORMAT_GREYSCALE GL_LUMINANCE8
#endif


