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

#include "camera.h"

camera::camera()
{
	reset();
}

void camera::reset()
{
	m_quat = glm::identity<glm::quat>();
	m_quat = glm::angleAxis((float)PI/2.f, glm::vec3(0, 1, 0)) * m_quat;
	m_quat = glm::angleAxis((float)PI/2.f, glm::vec3(0, 0, 1)) * m_quat;
	m_translation=glm::vec3(-80.f, -80.f, -80.f);
	m_fovy=55.0f*PI/180.0f;
	m_dist=-200.0f;
}

void camera::setupCamera()
{
	computeProjection();
	computeWorldview();
}

void camera::computeProjection()
{
	float ratio=static_cast<float>((float)m_screenSizeX/m_screenSizeY);
	const float f=1.0f/tan(m_fovy/2.0f);
	const float fp=f/ratio;
	m_fovx = 2.0f*atan(1.0f / fp);
	const float L=m_znear-m_zfar;

	const float C=(m_zfar+m_znear)/L;
	const float D=(2.0f*m_zfar*m_znear)/L;

	m_projection = glm::transpose(glm::mat4 (fp, 0,  0, 0,
					 0, f,  0, 0,
					 0, 0,  C, D,
					 0, 0, -1, 0));
}



void camera::computeWorldview()
{
	glm::mat4 worldMatrixZoom;
	worldMatrixZoom = glm::transpose(glm::mat4(1, 0, 0, 0,
					   0, 1, 0, 0,
					   0, 0, 1, m_dist,
					   0, 0, 0, 1));
	glm::mat4 worldMatrixTranslation;
	worldMatrixTranslation = glm::transpose(glm::mat4(1, 0, 0, m_translation[0],
							  0, 1, 0, m_translation[1],
							  0, 0, 1, m_translation[2],
							  0, 0, 0, 1));
	glm::mat4 worldMatrixRotation=glm::identity<glm::mat4>();
	worldMatrixRotation = glm::toMat4(m_quat);
	m_worldview=worldMatrixZoom*worldMatrixRotation*worldMatrixTranslation;
	m_normal = glm::transpose(glm::inverse(m_worldview));
}

void camera::goUp(float dL)
{
	glm::vec3 const y(0,-1,0);
	m_translation += dL*(glm::conjugate(m_quat)*y);
}

void camera::goRight(float dL)
{
	glm::vec3 const x(-1,0,0);
	m_translation += dL*(glm::conjugate(m_quat)*x);
}

void camera::goForward(float dL)
{
	glm::vec3 const z(0,0,1);
	m_translation += dL*(glm::conjugate(m_quat)*z);
}

float camera::projectToDisc(float const x, float const y)
{
	float const n=sqrt(x*x+y*y);
	if(n<m_discRadius*0.707107f)
		return sqrt(pow(m_discRadius,2)-n*n);
	else
		return pow(m_discRadius,2)/(2.0f*n);
}

void camera::rotation(int const x, int const y)
{
	float const xOld=m_xPrevious;
	float const yOld=m_yPrevious;

	float const x0=(2.0f*xOld-m_screenSizeX)/m_screenSizeX;
	float const y0=(m_screenSizeY-2.0f*yOld)/m_screenSizeY;
	float const x1=(2.0f*x-m_screenSizeX)/m_screenSizeX;
	float const y1=(m_screenSizeY-2.0f*y)/m_screenSizeY;
	if(sqrt(pow(x0-x1,2)+pow(y0-y1,2)>1e-6))
	{
		glm::vec3 const p1=glm::vec3(x0, y0, projectToDisc(x0, y0));
		glm::vec3 const p2=glm::vec3(x1, y1, projectToDisc(x1, y1));
		glm::vec3 const axis=glm::normalize(glm::cross(p1,p2));
		glm::vec3 const u=p1-p2;
		float t=glm::length(u)/(2.0f*m_discRadius);
		t=std::min(std::max(t,-1.0f),1.0f); //clamp
		float const phi = 2.0f*asin(t);
		//compute quaternion to apply
		m_quat = glm::angleAxis(phi, axis) * m_quat;
	}
	m_xPrevious=x;
	m_yPrevious=y;
}

void camera::rotateAlongY(float x)
{
	float const phi = 2.0f*asin(x);
	//compute quaternion to apply
	m_quat = glm::angleAxis(phi, glm::vec3(0.0f, 1.0f, 0.0f)) * m_quat;
}

void camera::alignX()
{
	m_quat = glm::angleAxis(float(PI / 2.f), glm::vec3(0.f, -1.f, 0.f)) * glm::identity<glm::quat>();
}

void camera::alignY()
{
	m_quat = glm::angleAxis(float(PI / 2.f), glm::vec3(-1.f, 0.f, 0.f)) * glm::identity<glm::quat>();
}

void camera::alignZ()
{
	m_quat = glm::identity<glm::quat>();
}

void camera::zoom(int const y)
{
	m_dist += (fabs(m_dist)+1.0f)*(y-m_zoomPrevious)/500.0f;
	m_dist = min(m_dist,0.0f);
	m_zoomPrevious=y;
}

glm::vec3 camera::getUpVector()
{
	return glm::rotate(glm::inverse(m_quat), glm::vec3(0, 1, 0));
}

glm::vec3 camera::getLookAtVector()
{
	return glm::rotate(glm::inverse(m_quat), glm::vec3(0, 0, -1));
}

glm::vec3 camera::getRightVector()
{
	return glm::rotate(glm::inverse(m_quat), glm::vec3(1, 0, 0));
}
