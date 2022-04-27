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

#include "widgetopengl.h"

WidgetOpenGL::WidgetOpenGL(const QGLFormat& format, QGLWidget *parent) :
	QGLWidget(format, parent)
{
	QWidget::setFocusPolicy(Qt::WheelFocus);
	m_mainTimer = startTimer(0);
	stopTimer();//We need to open a file first

	m_translationMat[0][0] = 40.0f;
	m_translationMat[1][1] = m_translationMat[0][0];
	m_translationMat[2][2] = m_translationMat[0][0];
	m_translationMat[3][0] = 80;
	m_translationMat[3][1] = 80;
	m_translationMat[3][2] = 80;

	makeCurrent();
}

WidgetOpenGL::~WidgetOpenGL()
{

}

void WidgetOpenGL::open(string filename, bool uncompressed)
{
	stopTimer();
	if (filename == "")
	{
		QFileDialog fileDialog;
		fileDialog.setOption(QFileDialog::DontUseNativeDialog,true);
		string filePath=".";
		fstream recordedPath;
		recordedPath.open("recordedPath.txt", ios_base::in);
		if (recordedPath.is_open())
			recordedPath >> filePath;
		recordedPath.close();
		if (uncompressed)
			m_filename = fileDialog.getOpenFileName(this, "Open a file containing fibers", QString(filePath.c_str()), "tck files (*.tck)").toStdString();
		else
			m_filename = fileDialog.getOpenFileName(this, "Open a file containing fibers", QString(filePath.c_str()), "tck or fft files (*.tck *.fft)").toStdString();
		if (m_filename == "")
		{
			cout << endl << "No file selected, canceling" << endl;
			return;
		}

		std::size_t endPath = m_filename.find_last_of('/');
		recordedPath.open("recordedPath.txt", ios_base::out);
		recordedPath << m_filename.substr(0, endPath);
		recordedPath.close();

		auto hasEnding = [](const string & path, const string & extension)
		{
			if (path.length() < extension.length()) return false;
			return (path.compare(path.length() - extension.length(), extension.length(), extension) == 0);
		};
		if (hasEnding(m_filename, "tck"))
		{
			m_scene.setNbPtsPerBlock(60);
			cout << endl;
			cout << "Number of points per block: " << m_scene.getNbPtsPerBlock() << endl;
		}
	}
	else
	{
		m_filename = filename;
		m_scene.setNbPtsPerBlock(60);
	}

	m_scene.load_scene(m_filename, uncompressed);
	restartTimer();
}

void WidgetOpenGL::initializeGL()
{
	//Init OpenGL
	setup_opengl();

	//Init Camera
	cam.setupCamera();

	//Init Scene 3D
	m_scene.set_widget(this);
	m_scene.init();
	//Activate depth buffer
	glEnable(GL_DEPTH_TEST); PRINT_OPENGL_ERROR();
	//glClearColor (1.0f, 1.0f, 1.0f, 1.0f);                      PRINT_OPENGL_ERROR();
}

void WidgetOpenGL::paintGL()
{
	//Compute current cameras
	cam.setupCamera();

	//clear screen
	if (m_initialized)
	{
		double frameTime = m_scene.draw_scene();
		std::ostringstream out, out2;
		out.precision(2);
		out << std::fixed << frameTime * 1000.0;
		double fps = 1.0 / frameTime;
		out2.precision(2);
		out2 << std::fixed << fps;
		std::string title = "Fibers visualization -- " + out2.str() + " fps     " + out.str() + " ms";
		this->window()->setWindowTitle(QString::fromStdString(title));

		if (m_captureNextFrame)
		{
			m_nbFrames++;
			if (m_nbFrames > 50)
			{
				m_scene.captureImage();
				exit(EXIT_SUCCESS);
			}
		}
	}
}


void WidgetOpenGL::resizeGL(int const width, int const height)
{
	m_scene.SetSizeScreen(width, height);
	m_scene.resizeWindow();//scene.h will resize the camera
}

void WidgetOpenGL::setup_opengl()
{
	if(!gladLoadGL()) {
			printf("Something went wrong!\n");
			exit(-1);
		}

	print_current_opengl_context();
}

void WidgetOpenGL::print_current_opengl_context() const
{
	std::cout << "OpenGl informations: VENDOR:       " << glGetString(GL_VENDOR)<<std::endl;
	std::cout << "                     RENDERDER:    " << glGetString(GL_RENDERER)<<std::endl;
	std::cout << "                     VERSION:      " << glGetString(GL_VERSION)<<std::endl;
	std::cout << "                     GLSL VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION)<<std::endl;
	std::cout<<"Current OpenGL context: "<< context()->format().majorVersion() << "." << context()->format().minorVersion()<<std::endl;
}

scene& WidgetOpenGL::get_scene()
{
	return m_scene;
}

void WidgetOpenGL::keyPressEvent(QKeyEvent *event)
{
	int current=event->key();
	Qt::KeyboardModifiers mod=event->modifiers();

	// We can quit the scene with 'Q'
	if( (mod&Qt::ShiftModifier)!=0 && (current==Qt::Key_Q) )
	{
		std::cout<<"\n[EXIT OK]\n\n"<<std::endl;
		this->window()->close();
	}
	if (current==Qt::Key_S)
	{
		fstream file;
		file.open("Camera.txt" , ios_base::out);
		glm::fquat q = cam.getQuat();
		file << q.x << " " << q.y << " " << q.z << " " << q.w << endl;
		glm::vec3 t=cam.getTranslation();
		file << t[0] << " " << t[1] << " " << t[2] << endl;
		file << cam.getDist() << endl;
		file.close();
		cout << "Camera position saved" << endl;
	}
	if (current==Qt::Key_C)
	{
		fstream file;
		file.open("Camera.txt", ios_base::in);
		if (!file.is_open())
		{
			std::cout << "No camera recorded" << std::endl;
			return;
		}
		float x, y, z, w;
		file >> x; file >> y; file >> z; file >> w;
		glm::fquat q(w, x, y, z);
		cam.setQuaternion(q);
		file >> x; file >> y; file >> z;
		cam.setTranslation(glm::vec3(x, y, z));
		file >> x;
		cam.setDist(x);
		m_scene.draw_scene();
		updateGL(); PRINT_OPENGL_ERROR();
		cout << "Camera position loaded" << endl;
	}
	if (current==Qt::Key_X)
	{
		cam.alignX();
		updateGL(); PRINT_OPENGL_ERROR();
	}
	if (current==Qt::Key_Y)
	{
		cam.alignY();
		updateGL(); PRINT_OPENGL_ERROR();
	}
	if (current==Qt::Key_Z)
	{
		cam.alignZ();
		updateGL(); PRINT_OPENGL_ERROR();
	}
	if (current==Qt::Key_A)
	{
		m_turnCam = ! m_turnCam;
	}
	if (current==Qt::Key_T)//Record frame rate in a file, with a fixed camera rotation
	{
		startRecordingFrameRate();
	}
	if (current==Qt::Key_W)//Reset camera
	{
		cam.reset();
	}
	if (current == Qt::Key_F)
	{
		emit  actionFullScreen();
	}
	if (current == Qt::Key_P)
	{
		m_scene.captureImage();
	}

	QGLWidget::keyPressEvent(event);
}

void WidgetOpenGL::startRecordingFrameRate()
{
	cam.reset();
	resize(1920, 1080);
	m_recordingFrameRate = true;
	m_turnCam = true;
	m_frameNumber = 0;
	m_startingTime = std::chrono::high_resolution_clock::now();
}

void WidgetOpenGL::timerEvent(QTimerEvent *event)
{
	event->accept();
	if (m_turnCam)
	{
		cam.rotateAlongY(0.01f);
		if (m_recordingFrameRate)
		{
			m_frameNumber++;
			if (m_frameNumber > m_nbOfFrame)
			{
				m_turnCam = false;
				m_recordingFrameRate = false;
				//Record file
				emit recordFrameRate(std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - m_startingTime).count(), m_nbOfFrame);
			}
		}
	}
	update();
}


void WidgetOpenGL::mousePressEvent(QMouseEvent *event)
{
	cam.xPrevious()=event->x();
	cam.yPrevious()=event->y();
	int const ctrl_pressed  = (event->modifiers() & Qt::ControlModifier);
	int const alt_pressed  = (event->modifiers() & Qt::AltModifier);

	if (alt_pressed && (event->buttons() & Qt::LeftButton))
		m_scene.setZOIVariables(true, glm::ivec2(event->x(), cam.getScreenSizeY() - 1 - event->y()));

	if (ctrl_pressed && (event->buttons() & Qt::LeftButton))
		m_scene.setSelectedRenderingVariables(true, glm::ivec2(event->x(), cam.getScreenSizeY() - 1 - event->y()), false);

	else if (ctrl_pressed && (event->buttons() & Qt::RightButton))
		m_scene.setSelectedRenderingVariables(false, glm::ivec2(event->x(), cam.getScreenSizeY() - 1 - event->y()), true);
}

void WidgetOpenGL::mouseMoveEvent(QMouseEvent *event)
{
	int const x=event->x();
	int const y=event->y();

	int const ctrl_pressed  = (event->modifiers() & Qt::ControlModifier);
	int const shift_pressed = (event->modifiers() & Qt::ShiftModifier);

	if (m_occluderMode) //We move the occluder
	{
		if(!ctrl_pressed && !shift_pressed && (event->buttons() & Qt::LeftButton))
		{
			float const dL=0.01f;
			glm::vec3 movement = glm::vec3(-(x-cam.xPrevious()), (y-cam.yPrevious()), 0);
			float angle = glm::length(movement);
			if (angle == 0.f)
				return;
			glm::vec3 movementAxis = movement / angle;
			glm::vec3 axis = glm::cross(movementAxis, glm::vec3(0.f, 0.f, 1.f));
			m_rotationMat = glm::rotate(m_rotationMat, angle * dL, glm::vec3(glm::inverse(glm::mat3(m_rotationMat)) * glm::inverse(glm::mat3(cam.getWorldview())) * axis));
		}
		if( (!ctrl_pressed && shift_pressed && (event->buttons() & Qt::LeftButton)) || (event->buttons() & Qt::MiddleButton))
		{
			float const dL=0.00001f*(1+10*fabs(cam.getDist()));
			glm::vec3 translationVector = glm::inverse(glm::mat3(cam.getWorldview())) * glm::vec3(+dL * (x-cam.xPrevious()), -dL * (y-cam.yPrevious()), 0);
			glm::mat4 translationMat = glm::mat4(1.f);
			translationMat[3][0] = translationVector[0];
			translationMat[3][1] = translationVector[1];
			translationMat[3][2] = translationVector[2];
			m_translationMat = m_translationMat * translationMat;
		}
		m_scene.getOccluderTransfo() = m_translationMat * m_rotationMat;
	}
	else // We move the camera
	{
		if(!ctrl_pressed && !shift_pressed && (event->buttons() & Qt::LeftButton))
			cam.rotation(x, y);

		// Shift+Left button controls the window translation (left/right, bottom/up)
		float const dL=0.0001f*(1+10*fabs(cam.getDist()));
		if( (!ctrl_pressed && shift_pressed && (event->buttons() & Qt::LeftButton)) || (event->buttons() & Qt::MiddleButton))
		{
			cam.goUp(dL*(y-cam.yPrevious()));
			cam.goRight(-dL*(x-cam.xPrevious()));
		}

		// Shift+Right button enables to translate forward/backward
		if( !ctrl_pressed && shift_pressed && (event->buttons() & Qt::RightButton) )
			cam.goForward(5.0f*dL*(y-cam.yPrevious()));

	}
	cam.xPrevious()=x;
	cam.yPrevious()=y;
}

void WidgetOpenGL::wheelEvent(QWheelEvent * event)
{
	int const shift_pressed = (event->modifiers() & Qt::ShiftModifier);
	if (m_occluderMode  && shift_pressed) //We move the occluder
	{
		float const dL=0.0000001f*(1+10*fabs(cam.getDist()));
		glm::vec3 translationVector = glm::inverse(glm::mat3(cam.getWorldview())) * glm::vec3(0, 0, dL * event->angleDelta().y());
		glm::mat4 translationMat = glm::mat4(1.f);
		translationMat[3][0] = translationVector[0];
		translationMat[3][1] = translationVector[1];
		translationMat[3][2] = translationVector[2];
		m_translationMat = m_translationMat * translationMat;
		m_scene.getOccluderTransfo() = m_translationMat * m_rotationMat;
	}
	else
	{
		int numSteps = event->angleDelta().y() / 8;
		m_steps += numSteps;
		cam.zoom(m_steps);
		event->accept();
	}
}

