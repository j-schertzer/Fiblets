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

#ifndef CAMERA_H
#define CAMERA_H

#define PI 3.14159265f
#ifdef WIN32
//#define M_PI_2 1.57079632679f //M_PI_2 doesnt exist on windows
#endif
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <algorithm>

using namespace std;

class camera
{
public:
	camera();
	void reset();

	const glm::mat4 getWorldview() {return m_worldview;}
	const glm::mat4 getProjection() {return m_projection;}


	const glm::mat2 getSubZProjection()
	{
		glm::mat2 submatrix = glm::mat2(m_projection[2][2], m_projection[2][3], m_projection[3][2], m_projection[3][3]);
		return submatrix;
	}
	const glm::mat2 getProjectionInverse()
	{
		glm::mat2 submatrix = glm::mat2(m_projection[2][2], m_projection[2][3], m_projection[3][2], m_projection[3][3]);
		return glm::inverse(submatrix);
	}
	const glm::mat4 getNormal() {return m_normal;}

	void setWorldview(glm::mat4 worldview) { m_worldview=worldview;}
	void setProjection(glm::mat4 projection) {m_projection=projection;}
	void setNormal(glm::mat4 normal) {m_normal=normal;}

	void setupCamera();
	void computeProjection();
	void computeWorldview();

	void setScreenSize(unsigned int width, unsigned int height){m_screenSizeX=width; m_screenSizeY=height;}
	void setDist(float dist){m_dist=dist;}
	void setQuaternion(glm::fquat q){m_quat=q;}
	void setTranslation(glm::vec3 t){m_translation=t;}

	float getFovyRad() { return m_fovy; }
	float getFovxRad() { return m_fovx; }
	float getNear() { return m_znear; }
	unsigned int getScreenSizeX(){return m_screenSizeX;}
	unsigned int getScreenSizeY(){return m_screenSizeY;}
	float getDist(){return m_dist;}
	glm::fquat getQuat(){return m_quat;}
	glm::vec3 getTranslation(){return m_translation;}

	int &xPrevious(){return m_xPrevious;}
	int &yPrevious(){return m_yPrevious;}

	//Movements of camera
	void goUp(float dL);
	void goRight(float dL);
	void goForward(float dL);

	void rotation(const int x, const int y);
	void rotateAlongY(float x);
	void zoom(int const y);

	void alignX();
	void alignY();
	void alignZ();

	glm::vec3 getUpVector();
	glm::vec3 getLookAtVector();
	glm::vec3 getRightVector();

private:
	float projectToDisc(float const x, float const y);

	//Matrices for 3D transformations
	glm::mat4 m_worldview;
	glm::mat4 m_projection;
	glm::mat4 m_normal;

	//Camera's parameters
	float m_fovy=55.0f*PI/180.0f;//fovy
	float m_fovx;//calculated thanks to ratio
	float m_znear=2.f;
	float m_zfar=600.0f;
	float m_dist=-200.0f;
	unsigned int m_screenSizeX=800;
	unsigned int m_screenSizeY=800;
	glm::vec3 m_translation=glm::vec3(-80.f, -80.f, -80.f);

	//Rotations
	glm::fquat m_quat;
	float m_discRadius=0.8f;

	//Mouse positions
	int m_xPrevious;
	int m_yPrevious;
	int m_zoomPrevious = -1325;
};

#endif // CAMERA_H
