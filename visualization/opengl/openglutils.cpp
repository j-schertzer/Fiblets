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

#include "openglutils.h"

static const GLchar* readShader(const char* filename)
{
	FILE* file=fopen(filename, "rb");
	if (!file)
		return NULL;
	fseek( file, 0, SEEK_END );
	int len = ftell( file );
	fseek( file, 0, SEEK_SET );
	GLchar* content=new GLchar[len+1];
	if (fread(content, 1, len, file)==0)
	{
		cerr << "Error while reading shaders" << endl;
	}
	fclose(file);
	content[len]=0;
	return const_cast<const GLchar*>(content);
}


GLint loadShaders(ShaderInfo *shaders)
{
	if (shaders == NULL)
	{
		cerr << "Pas de shaders trouvés" << endl;
		return 0;
	}
	GLuint program = glCreateProgram();

	ShaderInfo* entry = shaders;
	while(entry->type!=GL_NONE)
	{
		GLuint shader=glCreateShader(entry->type);
		const GLchar* source=readShader(entry->filename);
		if (source==NULL)
		{
			cerr << "No content in shader " << entry->filename << endl;
			for (entry=shaders; entry->type!=GL_NONE; ++entry)
			{
				glDeleteShader(entry->shader);
				entry->shader=0;
			}
			return 0;
		}
		glShaderSource(shader, 1, &source, NULL);
		delete[] source;
		glCompileShader(shader);
		GLint compiled;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if(!compiled)
		{
			GLsizei len;
			glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &len );

			GLchar* log = new GLchar[len+1];
			glGetShaderInfoLog( shader, len, &len, log );
			std::cerr << "Shader compilation failed: " << log << std::endl;
			delete [] log;
			return 0;
		}
		glAttachShader(program, shader);
		++entry;
	}
	glLinkProgram(program);
	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if(!linked)
	{
		GLsizei len;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
		GLchar* log = new GLchar[len+1];
		glGetProgramInfoLog( program, len, &len, log );
		std::cerr << "Program linkage failed: " << log << std::endl;
		delete [] log;

		for ( entry = shaders; entry->type != GL_NONE; ++entry ) {
			glDeleteShader( entry->shader );
			entry->shader = 0;
		}
		cerr << "Linkage error of the shader program" << endl;
		return 0;
	}
	return program;
}

/*****************************************************************************\
 * print_opengl_error                                                        *
\*****************************************************************************/
bool print_opengl_error(const char *file, int line)
{
	// Returns true if an OpenGL error occurred, false otherwise.
	GLenum error;
	bool   ret_code = false;

	error = glGetError();
	while (error != GL_NO_ERROR)
	{
		cerr << "glError in file " << file << ", line " << line << ": " << gluErrorString(error) << endl;
		ret_code = true;
		error = glGetError();
	}
	return ret_code;
}

GLint get_uni_loc(GLuint program, const GLchar *name)
{
  GLint loc;

  loc = glGetUniformLocation(program, name); PRINT_OPENGL_ERROR();
  if (loc == -1)
	std::cerr << "No such uniform named \"" << name << "\"" << std::endl;

  return loc;
}
