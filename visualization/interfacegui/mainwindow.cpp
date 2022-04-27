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

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(bool analysisMode, string filename, bool capture_image, QWidget *parent) :
	QMainWindow(parent), ui(new Ui::MainWindow)
{
	//Setup window layout
	ui->setupUi(this);

	this->resize(1920*2/3,1280*2/3);

	//Create openGL context
	QGLFormat qglFormat;
	qglFormat.setVersion(4,6);
	qglFormat.setDoubleBuffer(true);
	qglFormat.setSwapInterval(0);
	qglFormat.setSampleBuffers(false);
	qglFormat.setDirectRendering(true);
	//Create OpenGL Widget renderer
	glWidget=new WidgetOpenGL(qglFormat);

	//Add the OpenGL Widget into the layout
	ui->layout_scene->addWidget(glWidget);

	//Connect slot and signals

	//Menus
	//File Menu
	openMenu = menuBar()->addMenu(tr("&File"));
	openFile = new QAction(tr("&Open"), this);
	openMenu->addAction(openFile);
	connect(openFile,SIGNAL(triggered()), this, SLOT(open()));
	openUncompressedFile = new QAction(tr("Open &uncompressed"), this);
	openMenu->addAction(openUncompressedFile);
	connect(openUncompressedFile,SIGNAL(triggered()), this, SLOT(openUncompressed()));
	saveFile = new QAction(tr("&Save"), this);
	openMenu->addAction(saveFile);
	connect(saveFile, SIGNAL(triggered()), this, SLOT(save()));
	exitMenu = new QAction(tr("&Exit"), this);
	openMenu->addAction(exitMenu);
	connect(exitMenu, SIGNAL(triggered()), this, SLOT(exit()));

	//Modes menu
	modesMenu = menuBar()->addMenu(tr("&Modes"));
	triggerMenu(normalMode = new QAction(tr("&Normal"), this), modesMenu, groupMode, true);
	connect(normalMode,SIGNAL(triggered()), this, SLOT(change_mode()));
	triggerMenu(spectator = new QAction(tr("&Spectator"), this), modesMenu, groupMode);
	connect(spectator,SIGNAL(triggered()), this, SLOT(change_mode()));
	triggerMenu(occluder = new QAction(tr("&Occluder"), this), modesMenu, groupMode);
	connect(occluder,SIGNAL(triggered()), this, SLOT(change_mode()));

	//Visualization Menu
	visualizationMenu = menuBar()->addMenu(tr("&Visualization"));
	background = new QAction(tr("&White background"), this);
	background->setCheckable(true);
	background->setChecked(false);
	visualizationMenu->addAction(background);
	connect(background,SIGNAL(triggered()), this, SLOT(change_background()));
	drawUncompressed = new QAction(tr("&Draw uncompressed fibers"), this);
	drawUncompressed->setCheckable(false);
	drawUncompressed->setChecked(false);
	visualizationMenu->addAction(drawUncompressed);
	connect(drawUncompressed,SIGNAL(triggered()), this, SLOT(draw_uncompressed()));
	captureImage = new QAction(tr("&Screenshot"), this);
	visualizationMenu->addAction(captureImage);
	connect(captureImage,SIGNAL(triggered()), this, SLOT(capture_image()));

	//Shading Menu
	shadingMenu = menuBar()->addMenu(tr("&Shading"));
	triggerMenu(randomColor = new QAction(tr("&Random"), this), shadingMenu, groupColor);
	connect(randomColor, SIGNAL(triggered()), this, SLOT(action_color()));
	triggerMenu(blackAndWhiteColor = new QAction(tr("&Black & White"), this), shadingMenu, groupColor);
	connect(blackAndWhiteColor, SIGNAL(triggered()), this, SLOT(action_color()));
	triggerMenu(orientationColor = new QAction(tr("&Orientation"), this), shadingMenu, groupColor, true);
	connect(orientationColor, SIGNAL(triggered()), this, SLOT(action_color()));
	triggerMenu(colormap1 = new QAction(tr("ColorMap 1"), this), shadingMenu, groupColor);
	connect(colormap1, SIGNAL(triggered()), this, SLOT(action_color()));
	triggerMenu(colormap2 = new QAction(tr("ColorMap 2"), this), shadingMenu, groupColor);
	connect(colormap2, SIGNAL(triggered()), this, SLOT(action_color()));
	triggerMenu(colormap3 = new QAction(tr("ColorMap 3"), this), shadingMenu, groupColor);
	connect(colormap3, SIGNAL(triggered()), this, SLOT(action_color()));

	//Criterion Menu
	criterionMenu = menuBar()->addMenu(tr("&Criterion"));
	triggerMenu(noCriterion = new QAction(tr("&None"), this), criterionMenu, groupCriterion, true);
	connect(noCriterion, SIGNAL(triggered()), this, SLOT(action_criterion()));
	triggerMenu(lengthCriterion = new QAction(tr("&Length"), this), criterionMenu, groupCriterion);
	connect(lengthCriterion, SIGNAL(triggered()), this, SLOT(action_criterion()));
	triggerMenu(zoiCriterion = new QAction(tr("&Zone of interest"), this), criterionMenu, groupCriterion);
	connect(zoiCriterion, SIGNAL(triggered()), this, SLOT(action_criterion()));
	triggerMenu(lateralizationCriterion = new QAction(tr("Lateralization"), this), criterionMenu, groupCriterion);
	connect(lateralizationCriterion, SIGNAL(triggered()), this, SLOT(action_criterion()));

	connect(ui->slider0, SIGNAL(sliderMoved(int)), this, SLOT(action_slider()));//Percentage of fibers to draw

	//Fiber selection
	connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(action_clear_selection()));
	connect(ui->slider3, SIGNAL(sliderMoved(int)), this, SLOT(action_slider()));//Size of the selection
	connect(ui->slider1, SIGNAL(sliderMoved(int)), this, SLOT(action_slider()));//Radius of the selected fibers
	connect(ui->slider2, SIGNAL(sliderMoved(int)), this, SLOT(action_slider()));//Transparency of the brain when fibers are selected

	//HD post process
	connect(ui->ssao, SIGNAL(toggled(bool)), this, SLOT(action_post_process()));
	//connect(ui->volumetry, SIGNAL(toggled(bool)), this, SLOT(action_post_process()));
	connect(ui->illuminatedLines, SIGNAL(toggled(bool)), this, SLOT(action_post_process()));

	//Mesh cut
	connect(ui->useMeshCut, SIGNAL(toggled(bool)), this, SLOT(action_cut()));
	connect(ui->drawMeshCut, SIGNAL(toggled(bool)), this, SLOT(action_cut()));
	connect(ui->fragmentPrecision, SIGNAL(toggled(bool)), this, SLOT(action_cut()));
	connect(ui->cullInside, SIGNAL(toggled(bool)), this, SLOT(action_cut()));
	connect(ui->cullOustide, SIGNAL(toggled(bool)), this, SLOT(action_cut()));

	//Optimizations
	connect(ui->optimisationBox, SIGNAL(toggled(bool)), this, SLOT(action_optimisation()));
	connect(ui->aggressiveBox, SIGNAL(toggled(bool)), this, SLOT(action_optimisation()));

	//Culling criterion
	connect(ui->cullingCritMin, SIGNAL(valueChanged(int)), this, SLOT(action_min_culling()));
	connect(ui->cullingCritMax, SIGNAL(valueChanged(int)), this, SLOT(action_max_culling()));

	//Pipeline
	connect(ui->tessellation, SIGNAL(toggled(bool)), this, SLOT(action_pipeline()));
	connect(ui->geometry, SIGNAL(toggled(bool)), this, SLOT(action_pipeline()));
	connect(ui->mesh, SIGNAL(toggled(bool)), this, SLOT(action_pipeline()));

	connect(glWidget, SIGNAL(recordFrameRate(double, unsigned)), this, SLOT(record_frame_rate(double, unsigned)));
	connect(glWidget, SIGNAL(actionFullScreen()), this, SLOT(action_fullscreen()));

	//Load a tractogram, displays it, make it turn and record the framerate before closing
	m_analysisMode = analysisMode;
	m_autoCapture = capture_image;
	if (analysisMode || capture_image)
	{
		if (analysisMode)
			cout << "Starting analysis mode" << endl;
		m_filename = filename;
		m_timer = startTimer(25, Qt::TimerType::PreciseTimer);
	}
}

MainWindow::~MainWindow()
{
	delete ui;
}

//Function only called in analysisMode
void MainWindow::timerEvent(QTimerEvent *event)
{
	killTimer(m_timer);
	event->accept();
	glWidget->open(m_filename);
	ui->slider0->setValue(1023);
	ui->slider0->sliderMoved(1023);
	if (m_autoCapture)
	{
		background->setChecked(true);
		change_background();
		this->resize(1919, 1200);//To obtain an image as close as possible to 1920x1080
		orientationColor->setChecked(true);
		ui->illuminatedLines->setChecked(true);
		ui->mesh->setChecked(true);
		//Wait for next frame to be drawn to capture
		glWidget->captureNextFrame();
	}
	if(m_analysisMode)
	{
		ui->tessellation->setChecked(true);
		glWidget->startRecordingFrameRate();
	}
}

void MainWindow::action_quit()
{
	close();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
	int current=event->key();
	if (current == Qt::Key_R)
	{
		glWidget->get_scene().reloadShaders();
	}
}

void MainWindow::action_fullscreen()
{
	if (m_isFullScreen)
	{
		m_fullScreenWidget->close();
		ui->layout_scene->addWidget(glWidget);
		m_isFullScreen = false;
	}
	else
	{
		if (!m_fullScreenWidget)
			m_fullScreenWidget = new QMainWindow();
		m_fullScreenWidget->setCentralWidget(glWidget);
		m_isFullScreen = true;
		m_fullScreenWidget->show();
		m_fullScreenWidget->windowHandle()->setScreen(this->windowHandle()->screen());//qApp->screens()[1]);
		m_fullScreenWidget->windowHandle()->setFramePosition(this->windowHandle()->screen()->geometry().topLeft());
		m_fullScreenWidget->showFullScreen();
		ui->layout_scene->removeWidget(glWidget);
	}
}

void MainWindow::open()
{
	glWidget->open();
	drawUncompressed->setCheckable(false);
	drawUncompressed->setChecked(false);
}

void MainWindow::openUncompressed()
{
	glWidget->open("", true);
	drawUncompressed->setCheckable(true);
}

void MainWindow::save()
{
	if (!glWidget->get_scene().thereIsATracto())
		return;
	glWidget->stopTimer();
	string filename = QFileDialog::getSaveFileName(this, "Select a file to save fibers", "../../compressor/build", "fft files (*.fft)").toStdString();
	glWidget->get_scene().saveTracto(filename);
	glWidget->restartTimer();
}

void MainWindow::exit()
{
	std::exit(EXIT_SUCCESS);
}

void MainWindow::change_mode()
{
	cout << "Mode changed" << endl;
	if (spectator->isChecked())
	{
		ui->fragmentPrecision->setChecked(false);
		ui->fragmentPrecision->setDisabled(true);
	}
	else
		ui->fragmentPrecision->setDisabled(false);
	glWidget->get_scene().setSpectatorMode(spectator->isChecked());
	glWidget->setOccluderMode(occluder->isChecked());
}

void MainWindow::change_background()
{
	glWidget->get_scene().setWhiteBackground(background->isChecked());
}

void MainWindow::draw_uncompressed()
{
	glWidget->get_scene().drawUncompressedFibers(drawUncompressed->isChecked());
}

void MainWindow::capture_image()
{
	glWidget->get_scene().captureImage();
}

void MainWindow::action_slider()
{
	glm::vec4 val;
	val.x = float(ui->slider0->sliderPosition() ) / 1023;
	ui->fiberPercentage->setText(QString::fromStdString(to_string(int(val.x * 100)) + "% fibers"));
	val.y = float(ui->slider1->sliderPosition()) / 1023;
	val.z = float(ui->slider2->sliderPosition()) / 1023;
	val.w = float(ui->slider3->sliderPosition()) / 1023;
	glWidget->get_scene().updateSliderValue(val);
}

void MainWindow::action_post_process()
{
	unsigned postProcess;
	if (ui->ssao->isChecked())
		postProcess = 0;
	else if (ui->illuminatedLines->isChecked())
		postProcess = 1;
	else
		postProcess = 2;
	glWidget->get_scene().activatePostProcess(postProcess);
}

void MainWindow::action_clear_selection()
{
	glWidget->get_scene().ClearSelection();
}

void MainWindow::action_cut()
{
	if (!ui->useMeshCut->isChecked())
	{
		ui->drawMeshCut->setDisabled(true);
		ui->fragmentPrecision->setDisabled(true);
		ui->cullInside->setDisabled(true);
		ui->cullOustide->setDisabled(true);
	}
	else
	{
		ui->drawMeshCut->setDisabled(false);
		ui->fragmentPrecision->setDisabled(false);
		ui->cullInside->setDisabled(false);
		ui->cullOustide->setDisabled(false);
	}
	glWidget->get_scene().setMeshCut(ui->useMeshCut->isChecked(), ui->drawMeshCut->isChecked(),
									 ui->fragmentPrecision->isChecked(), ui->cullInside->isChecked());
}

void MainWindow::action_optimisation()
{
	if (!ui->optimisationBox->isChecked())
	{
		ui->aggressiveBox->setChecked(false);
		ui->aggressiveBox->setCheckable(false);
		ui->aggressiveBox->setDisabled(true);
	}
	else
	{
		ui->aggressiveBox->setCheckable(true);
		ui->aggressiveBox->setDisabled(false);
	}
	glWidget->stopTimer();
	glWidget->get_scene().setOptimisation(ui->optimisationBox->isChecked(), ui->aggressiveBox->isChecked());
	glWidget->restartTimer();
}

void MainWindow::action_color()
{
	unsigned color = 0;
	if (randomColor->isChecked())
		color = 0;
	else if (blackAndWhiteColor->isChecked())
		color = 1;
	else if (orientationColor->isChecked())
		color = 2;
	else if (colormap1->isChecked())
		color = 3;
	else if (colormap2->isChecked())
		color = 4;
	else if (colormap3->isChecked())
		color = 5;
	glWidget->get_scene().setColor(color);
}

void MainWindow::action_pipeline()
{
	glWidget->get_scene().setPipeline(ui->tessellation->isChecked(), ui->geometry->isChecked(), ui->mesh->isChecked());
}

void MainWindow::action_criterion()
{
	unsigned criterion = 0;
	if (noCriterion->isChecked())
		criterion = 0;
	else if (lengthCriterion->isChecked())
		criterion = 1;
	else if (zoiCriterion->isChecked())
		criterion = 2;
	else if (lateralizationCriterion->isChecked())
		criterion = 3;
	glWidget->get_scene().setCriterion(criterion);
}

void MainWindow::action_culling()
{
	glWidget->get_scene().setCullingCriterion(ui->cullingCritMin->value(), ui->cullingCritMax->value());
}

void MainWindow::action_min_culling()
{
	int minVal = ui->cullingCritMin->value();
	int maxVal = ui->cullingCritMax->value();
	if (maxVal < minVal)
		ui->cullingCritMax->setValue(minVal);
	action_culling();
}

void MainWindow::action_max_culling()
{
	int minVal = ui->cullingCritMin->value();
	int maxVal = ui->cullingCritMax->value();
	if (maxVal < minVal)
		ui->cullingCritMin->setValue(maxVal);
	action_culling();
}

void MainWindow::record_frame_rate(double elapsedTime, unsigned nbOfFrames)
{
	double frameRate = nbOfFrames / elapsedTime;
	fstream file;
	file.open("FrameRates.txt", ios_base::out | ios_base::app);
	file << glWidget->getFilename() << " ";
	if (ui->tessellation->isChecked())
		file << "Tessellation ";
	else if (ui->mesh->isChecked())
		file << "Mesh ";
	else
		file << "Geometry ";
	if (ui->optimisationBox->isChecked())
		file << "true ";
	else
		file << "false ";
	file << glWidget->get_scene().getNbPtsPerBlock() << " ";
	file << frameRate << " ";
	file << 1.0/frameRate;
	file << "\n";
	file.close();
	cout << "Frame rate recorded" << endl;

	if (ui->mesh->isChecked() && !ui->optimisationBox->isChecked())
		std::exit(EXIT_SUCCESS);

	if (ui->tessellation->isChecked())
		ui->geometry->setChecked(true);
	else if (ui->geometry->isChecked())
		ui->mesh->setChecked(true);
	else if (ui->optimisationBox->isChecked())
	{
		ui->optimisationBox->setChecked(false);
		ui->tessellation->setChecked(true);
	}
	glWidget->startRecordingFrameRate();
}

void MainWindow::triggerMenu(QAction* action, QMenu* &menu, QActionGroup* &group, bool checked)
{
	action->setCheckable(true);
	action->setChecked(checked);
	action->setActionGroup(group);
	menu->addAction(action);
}
