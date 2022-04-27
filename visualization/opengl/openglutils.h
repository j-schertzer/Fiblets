// --------------------------------------------------------------------------
// Source code provided FOR REVIEW ONLY, as part of the submission entitled
// "Fiblets for Real-Time Rendering of Massive Brain Tractograms".
//
// A proper version of this code will be released if the paper is accepted
// with the proper licence, documentation and bug fix.
// Currently, this material has to be considered confidential and shall not
// be used outside the review process.
//
// All right reserved. The Authors
// --------------------------------------------------------------------------

#ifndef OPENGLUTILS_H
#define OPENGLUTILS_H

#ifdef WIN32
#include <windows.h>
#define M_PI_2 1.57079632679 //M_PI_2 doesnt exist on windows
#endif
#include <glad/glad.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include <string>
#include <iostream>
#include <fstream>
#include <streambuf>


using namespace std;

/** Macro indicating OpenGL errors (file and line) */
#define PRINT_OPENGL_ERROR() print_opengl_error(__FILE__, __LINE__)

typedef struct
{
	GLenum type;
	const char* filename;
	GLuint shader;
}ShaderInfo;

//Function returning the ID of the program using the shaders included in ShaderInfo
GLint loadShaders(ShaderInfo* shaders);

/** Print OpenGL Error given the information of the file and the line
 *  Function called by the macro PRINT_OPENGL_ERROR */
bool print_opengl_error(const char *file, int line);

GLint get_uni_loc(GLuint program, const GLchar *name);


#endif // OPENGLUTILS_H
