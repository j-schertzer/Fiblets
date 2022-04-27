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

#pragma once

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWindow>
#include <QScreen>
#include <QActionGroup>
#include "widgetopengl.h"
#include <sstream>
#include <QSlider>
#include <functional>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(bool analysisMode = false, string filename = "", bool capture_image = false, QWidget *parent = 0);
	~MainWindow();

private slots:

	/** Quit the application */
	void action_quit();

	//Menus
	void open();
	void openUncompressed();
	void save();
	void exit();
	void change_mode();
	void change_background();
	void draw_uncompressed();
	void capture_image();

	//Interface
	void action_slider();
	void action_post_process();
	void action_clear_selection();
	void action_cut();
	void action_optimisation();
	void action_color();
	void action_pipeline();
	void action_criterion();

	void action_culling();
	void action_min_culling();
	void action_max_culling();


	void keyPressEvent(QKeyEvent *event);

	void record_frame_rate(double elapsedTime, unsigned nbOfFrames);
	void action_fullscreen();

	void triggerMenu(QAction* action, QMenu* &menu, QActionGroup* &group, bool cheched = false);

private:

	/** Layout for the Window */
	Ui::MainWindow *ui;
	/** The OpenGL Widget */
	WidgetOpenGL *glWidget;

	QMainWindow *m_fullScreenWidget = NULL;

	QMenu *openMenu;
	QAction *openFile;
	QAction *openUncompressedFile;
	QAction *saveFile;
	QAction *exitMenu;

	QMenu *modesMenu;
	QAction *normalMode;
	QAction *spectator;
	QAction *occluder;
	QActionGroup *groupMode = new QActionGroup(this);//For modes to be mutually exclusive

	QMenu *visualizationMenu;
	QAction *background;
	QAction *drawUncompressed;
	QAction *captureImage;

	QMenu *shadingMenu;
	QAction *randomColor;
	QAction *blackAndWhiteColor;
	QAction *orientationColor;
	QAction *colormap1;
	QAction *colormap2;
	QAction *colormap3;
	QActionGroup *groupColor = new QActionGroup(this);//For colors to be mutually exclusive

	QMenu *criterionMenu;
	QAction *noCriterion;
	QAction *lengthCriterion;
	QAction *zoiCriterion;
	QAction *lateralizationCriterion;
	QActionGroup *groupCriterion = new QActionGroup(this);//For criterions to be mutually exclusive

	string m_filename = "";
	bool m_isFullScreen = false;

	/** Function called in a timer loop */
	void timerEvent(QTimerEvent *event);
	int m_timer;
	bool m_analysisMode = false;
	bool m_autoCapture = false;
	bool m_waitingForNextFrame = false;
};

#endif // MAINWINDOW_H
