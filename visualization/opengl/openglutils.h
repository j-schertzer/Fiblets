// --------------------------------------------------------------------------
// This file is part of the reference implementation for the paper
//    Fiblets for Real-Time Rendering of Massive Brain Tractograms.
//    J. Schertzer, C. Mercier, S. Rousseau and T. Boubekeur
//    Computer Graphics Forum 2022
//    DOI: 10.1111/cgf.14486
//
// All rights reserved. Use of this source code is governed by a
// MIT license that can be found in the LICENSE file.
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
