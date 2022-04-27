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

#pragma once

#ifndef WIDGETOPENGL_H
#define WIDGETOPENGL_H

#include <glad/glad.h>

#include <GL/gl.h>

#include <QtOpenGL/QGLWidget>
#include <QMouseEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QDialogButtonBox>

#include <cstring>
#include <iostream>
#include <sstream>
#include <chrono>

#include "../opengl/openglutils.h"
#include "../scene/scene.h"
#include "../opengl/camera.h"

using namespace std;

/** Qt Widget to render OpenGL scene */
class WidgetOpenGL : public QGLWidget
{
	Q_OBJECT

public:

	WidgetOpenGL(const QGLFormat& format, QGLWidget *parent = 0);
	~WidgetOpenGL();

	void open(string filename = "", bool uncompressed = false);

	scene& get_scene();
	//Camera
	camera cam;

	void setInitialized(bool val = true){m_initialized = val;}
	bool isInitialized(){return m_initialized;}

	void stopTimer(){killTimer(m_mainTimer);}
	void restartTimer(){m_mainTimer = startTimer(0);}

	string getFilename(){return m_filename;}
	/** Record frame rate statistics */
	void startRecordingFrameRate();

	void setOccluderMode(const bool b){m_occluderMode = b;}

	void captureNextFrame(){m_captureNextFrame = true;}

signals:
	void recordFrameRate(double elapsedTime, unsigned nbOfFrames);
	void actionFullScreen();

protected:
	/** Setup the OpenGL rendering mode */
	void initializeGL();
	/** The actual rendering function */
	void paintGL();
	/** Function called when the window is resized */
	void resizeGL(const int width, const int height);

	/** Function called a button of the mouse is pressed */
	void mousePressEvent(QMouseEvent *event);
	/** Function called when the mouse is moved */
	void mouseMoveEvent(QMouseEvent *event);
	/** Function called when the wheel of the mouse is moved **/
	void wheelEvent(QWheelEvent *event);
	/** Function called in a timer loop */
	void timerEvent(QTimerEvent *event);
	/** Function called when keyboard is pressed */
	void keyPressEvent(QKeyEvent *event);

private:
	/** Init the OpenGL rendering mode once at the beginning */
	void setup_opengl();
	/** Init Glew once at the beginning */
	void setup_glew();
	/** Print on the command line the actual version of the OpenGL Context */
	void print_current_opengl_context() const;
	/** All the content of the 3D scene */
	scene m_scene;

	string m_filename;

	bool m_initialized = false;
	int m_steps = -1325;
	float increment = 1;

	bool m_turnCam = false;
	bool m_recordingFrameRate = false;
	unsigned m_frameNumber = 0;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_startingTime;
	unsigned m_nbOfFrame = 314;

	//Occluder
	bool m_occluderMode = false;//Mode to move the occluder
	glm::mat4 m_rotationMat = glm::mat4(1.0f);
	glm::mat4 m_translationMat = glm::mat4(1.f);

	int m_mainTimer;
	bool m_captureNextFrame = false;
	unsigned m_nbFrames = 0;
};

#endif // WIDGETOPENGL_H
