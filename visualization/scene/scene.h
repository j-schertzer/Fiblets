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

#ifndef SCENE_H
#define SCENE_H
#define IDENT(x) x
#define XSTR(x) #x
#define STR(x) XSTR(x)



//On windows, if due to a custom development environment settings, the folder where Visual Studio launch the application is not \bin,
//the relative path access for ressource will fail. The #define USE_EXPLICIT_FILENAME_SYSTEM allows to force the specific path for ressources
//#define USE_EXPLICIT_FILENAME_SYSTEM

#ifdef USE_EXPLICIT_FILENAME_SYSTEM
#define PATHINC(y) STR(IDENT(E:/Code/fiber-visualization/visualization/)IDENT(y)) //hard access
#else
#define PATHINC(y) STR(IDENT(../visualization/)IDENT(y))
#endif




#include <string>
#include <set>
#include <iostream>
#include <chrono>
#include <sys/stat.h>
#include <limits>
#include <random>

#ifndef linux
	#include <io.h>
	#define F_OK 0
#else
	#include <unistd.h>
#endif

#include "../lib/stb_image_write.h"
#include PATHINC(../lib/tiny_obj_loader.h)
#include PATHINC(opengl/openglutils.h)

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <qfib.hpp>

#define RESTART_PRIMITIVE (0xFFFFFFF0)

#define FILTER_PASSES 3 //MAX filtering zbuffer (final size is screensize/pow(2,FILTER_PASSES))
#define SUBSAMBLE_ZBUFFER_FACTOR 8 // fraction of screen size for depthtesting textures
#define MS_SAMPLES 8 // MSAA for final renderbuffer
#define DEPTH_BUFFER_TYPE GL_DEPTH_COMPONENT24 // precision of zbuffer
//AGGRESSIVE OPTIM CONSTANTS
	#define FRAGMENTS_PER_LINE 4 //for adaptative tesselation and fiber clamping
	// a factor is calulated within shaders according to the constant defined up above. that factor is the size in texel of a stepsize (at depth z) divided by FRAGMENTS_PER_LINE
	// factor >=1.0 rendering of all compressed point is necessary [ close to camera]
	// factor close to 0.0. fibers are seen from far, therefore compressed points can be discarded (in tesselation pipeline)
	#define RENDER_SINGLE_SEGMENT_RATIO 1 // the define is the decimal part, in shader it will appear as 0.1
	//uint threshold = uint(mix(900.0,0.0,0.5*tessL/RENDER_SINGLE_SEGMENT_RATIO));
	//if ((fiberID%1000) < threshold)
	 //for adaptative tesselation and fiber clamping
	//if factor < RENDER_SINGLE_SEGMENT_RATIO the primitive generated is simply the segment of the first point and last point (for a non end block)



//GL texture slot constants
#define FREE_UNBOUND_SLOT_TEXTURE 0
#define GBUFFER_COLOR_SLOT_TEXTURE 1
#define GBUFFER_DEPTH_SLOT_TEXTURE 2
#define GBUFFER_COLOR_SLOT_TEXTURE_MS 3
#define GBUFFER_DEPTH_SLOT_TEXTURE_MS 4
#define FRAGMENT_CULLING_SLOT_TEXTURE 5
#define FIBER_CULLING_SLOT_TEXTURE 6
#define FILTERING_SLOT_TEXTURE 7

#define OCCLUDER_SLOT_TEXTURE 8
#define NB_OCCLUDER_SLOTS 6
#define ZB 0//Zback
#define ZOB 1//ZOccluderBack
#define ZOF 2//ZOccluderFront
#define ZBC 3//ZBack Cache
#define ZOFB 0//ZOccluderFragmentBack
#define ZOFF 1//ZOccluderFragmentFront



#define UBO_STATIC_BINDING_POINT 1
typedef struct staticUboDataFormat //elements must be 128 bits aligned
{
	glm::vec4 data0;//xy=m_sizeZtestViewPort.xy, zw=2.0/(m_sizeFBO.xy-1.0)
	glm::vec4 data1;//x = guardBand, y = angularCorrection, z = ratio, w = scaling
	glm::vec4 data2;//x = unused, y = tesselationGain, z = colormode & backgroundflag, w = m_sizeSubsampledZBuffers.x
	glm::vec4 data3;//xyz = frustrum test constants, w = stepsize
	glm::vec4 sliders;//x = percent of fiber drawn, y = selected fiber Radius size, z = alpha selected fiber, w = size of selection
}staticUboDataFormat;

#define UBO_DYNAMIC_BINDING_POINT 2
typedef struct dynamicUboDataFormat //elements must be 128 bits aligned
{
	glm::mat4 projCam;//used
	glm::mat4 ZFilterinvCamProjMultTexShift;//used
	glm::mat4 ProjMultCurrentPoseMultInvPreviousProjPoseMultTexShift;//used
	glm::mat4 projCamMultMV;//used
	glm::mat4 ModelViewCam;//used
	glm::mat4 ModelViewOccluder;//used

	glm::mat4 projSpec;//used
	glm::mat4 invProjSpec;//used
	glm::mat4 ModelViewSpec;//used
	glm::mat4 projSpecMultMV;//used

	glm::mat4 lights;//4xvec4(xLight,yLigth,zLight,1.0) in camera space

}dynamicUboDataFormat;
class WidgetOpenGL;

class scene
{
public:
	scene();
	void init();

	/**  Method called only once at the beginning (load off files ...) */
	bool load_scene(string filename, bool uncompressedFile = false);
	bool close_scene();

	/**  Method called at every frame */
	double draw_scene();

	void captureImage();

	void ProcessZMinMaxFiltering();
	void RenderZcullingBuffer();
	/** Set the pointer to the parent Widget */
	void set_widget(WidgetOpenGL* widget_param);

	void valueChanged(){m_valueChange=true;}

	void resizeWindow();
	//void RebuildIBO();

	void reloadShaders();
	void updateSliderValue(glm::vec4 val) {m_sliderValue = val;}

	bool thereIsATracto() { return m_tractoLoaded; }
	void saveTracto(const string filename);

	unsigned getNbPtsPerBlock(){return m_tracto.nbPtsPerBlock;}
	unsigned getPackSize(){return m_tracto.packSize;}

	void setNbPtsPerBlock(unsigned val);

	//Fiber selection
	void setSelectedRenderingVariables(bool click, glm::ivec2 mouse, bool forceReset) { m_clickSelected = click; m_mousePos = mouse + glm::ivec2(0,m_sizeScreen.y-m_sizeFBO.y); m_forceSelectedReset = forceReset; }
	void setZOIVariables(bool click, glm::ivec2 mouse) { m_clickZOI = click; m_mousePos = mouse + glm::ivec2(0, m_sizeScreen.y - m_sizeFBO.y); }
	void ClearSelection();

	//HD post process
	void activatePostProcess(const unsigned postProcess){m_postProcess = postProcess;}

	//Mesh cut
	void setMeshCut(const bool useMeshCut, const bool drawMeshCut, const bool fragmentPrecision, const bool cullInside)
	{m_useMeshCut = useMeshCut; m_drawMeshCut = drawMeshCut; m_fragmentTestOccluder = fragmentPrecision; m_cullOutsideMesh = !cullInside; reloadShaders();}

	//Optimizations
	void setOptimisation(bool optim, bool aggressive);

	//Colors
	void setColor(unsigned c = 2){m_color = c;}//0 = random; 1 = black and white; 2 = orientation; 3 = colormap1; 4 = colormap2; 5 = colormap3

	//Criterions
	void setCriterion(unsigned c = 0){m_criterion = c;}//0 = none; 1 = length; 2 = zoi; 3 = lateralization

	//Culling criterion
	void setCullingCriterion(unsigned minVal, unsigned maxVal){m_minCullingCriterion = minVal; m_maxCullingCriterion = maxVal;}

	//Pipeline
	void setPipeline(bool t, bool g, bool m){m_tessellation = t; m_geometry = g; m_mesh = m;}

	//Spectator
	void setSpectatorMode(const bool b){m_spectatorMode = b;}

	//Change occluder transformation
	glm::mat4 & getOccluderTransfo(){return m_occluderTransfoM;}

	//Change background color
	void setWhiteBackground(bool white);
	void SetSizeScreen(int const width, int const height) { m_sizeScreen.x = width, m_sizeScreen.y = height; }

	//Change to uncompressed mode
	void drawUncompressedFibers(bool doIt){m_drawUncompressed = doIt;}
private:
	void flushStaticUBO();
	void flushDynamicUBO();
	void renderQuad(void);
	void loadTracto(const string filename, const bool fillUncompressedVBO);
	bool hasEnding(const string & path, const string & extension);
	//Access to the parent object
	WidgetOpenGL* m_pwidget;


	///////////////////////BEGIN SHADERS
	void decompressionCurrentShaders(bool optimisation);
	//decompression FOLDER
	GLuint m_dummyShader;
	//rendering of uncompressed tractogram
	GLuint m_UncompressedRenderingShader;
	//Tessellation
	GLuint m_TessDecompressShader;
	GLuint m_TessOptimsProgram;
	GLuint m_TessNoOptimsProgram;
	//Geometry
	GLuint m_GeomDecompressShader;
	GLuint m_GeomOptimsProgram;
	GLuint m_GeomNoOptimsProgram;
	//Geometry
	GLuint m_MeshDecompressShader;
	GLuint m_MeshOptimsProgram;
	GLuint m_MeshNoOptimsProgram;
	//zfiltering FOLDER
	GLuint m_zMaxFilterProgram;
	GLuint m_zMeshProgram;
	GLuint m_zWrapProgram;
	GLuint m_zcopyTestProgram;
	GLuint m_zcopyFragmentProgram;
	//postprocess FOLDER
	GLuint m_SSAOShadingProgram;
	//meshcut FOLDER
	GLuint m_BackOccluderProgram;
	GLuint m_BackFragmentOccluderProgram;
	//selectedRendering FOLDER
	GLuint  m_selectedRenderingProgram;//using tesselation without any optimisation
	//ROOT FOLDER
	GLuint m_basicProgram;
	GLuint m_viewTextureProgram;
	/////////////////////////////////END SHADERS

	//Fibers
	//Compressed tractogram
	CompressedTractogram m_tracto;
	string m_filename = "";//Name of the opened tractogram

	//Storage for the VBO associated to the fibers seen as lines
	GLuint m_vaoDummy;
	//uncompressed tck
	void drawUncompressed(void);
	std::vector<GLuint> m_vaoUncompressed;
	std::vector<GLuint> m_vboUncompressed;//rgba32f (.w = fiberID)
	bool m_FillTck = false;
	bool m_drawUncompressed=false;
		//multidraw buffers
		std::vector<std::vector<uint32_t>> m_uncompressedFIRST;
		std::vector<std::vector<GLsizei >> m_uncompressedCOUNT;
		std::vector<GLuint> m_primcountUncompressed;

	glm::ivec2 m_sizeScreen;
	glm::ivec2 m_sizeZtestViewPort;// m_sizeScreen / SUBSAMBLE_ZBUFFER_FACTOR rounded to be divided by 4
	glm::ivec2 m_sizeFBO;//THIS IS THE RESOLUTION OF FBOs (can be divided by 4*pow(2,FILTER_PASSES), >= m_sizeScreen)
	glm::ivec2 m_sizeSubsampledZBuffers;//=m_sizeFBO/pow(2,FILTER_PASSES) //can be divided by 4




	void MergeToZTextures();

	GLuint m_vaoQuad;
	GLuint m_vboQuad;

	//Occluder
	void loadOccluder(const string fileName);
	bool m_meshCutLoaded = false;
	void ProcessZoccluder();
	GLuint m_vaoOccluder;
	GLuint m_vboOccluder;
	unsigned int m_nbVertexOccluder;
	bool m_pstate = false;//previous state
	glm::mat4 m_occluderTransfoM;

	//Compressed fibers
	uint64_t m_numberOfPacks = 0;
	glm::mat4 m_modelWorld;//agregating scaling and offset
	float m_guardBand = 1.0f;
	void SetGuardBand(float stepLength, uint32_t NbPoint);

	//Slider
	int m_value_of_slider=100;
	bool m_valueChange=false;

	//FBO
	void initFBOrelated();
	GLuint gBufferFBO;//FBO (1 color attachment (r = FiberID, g = tangent+ 1 depth attachment)
		GLuint gBufferTex;//fiberid + tangent
		GLuint gBufferZTex;//framebufferZbuffer
	GLuint copyRenderFBOMS;// dummy fbo for blitting color buffer when selective rendering is enabled
	GLuint tractoRenderFBOMS;//multisampled FBO (1 color attachment rgba,1 depth buffer)
		GLuint tractorRenderColorBufferMS;//this is not a texture but  a MultisampledRenderbuffer
		GLuint tractorRenderZBufferMS;//this is not a texture but  a MultisampledRenderbuffer
		GLuint tractorRenderColorTexMS;//attachement changes if selected rendering is enabled
		GLuint tractorRenderZTexMS;//attachement changes if selected rendering is enabled
	GLuint zFilterFBO;//fbo used for processing depth culling
		GLuint zFilterBackTex[FILTER_PASSES+1];


	GLuint zTestFragmentFBO;
		GLuint zChannelFragmentTex[2];
	GLuint zTestFBO[4];
		GLuint zChannelTex[4];
	GLuint zCopyFragmentFBO;
		GLuint zFragmentTestTex;
	GLuint zCopyTestFBO;
		GLuint zTestTex;
	//SSBO
	std::vector<GLuint> ssboCompressedFibers;

	GLuint ssboCriterion;//ssbo storing one constant per fiber, used for color map shading/culling purpose
	#define SSBO_CRITERION_SLOT (3)
	void FlushCriterionSSBO(void);
	//UBO
	GLuint staticUBO;
	GLuint dynamicUBO;
	staticUboDataFormat staticDataUbo;
	dynamicUboDataFormat dynamicDataUbo;
	//Sliders value
	glm::vec4 m_sliderValue = glm::vec4(0.1f, 0.5f, 0.33f, 0.2f);

	bool m_firstOpenFile = true;
	bool m_tractoLoaded = false;
	unsigned int m_clearColor = 0;

	//Optimizations
	bool m_optimisation = true;
	bool m_aggressiveOptim = false;

	//Colors
	unsigned m_color = 2;//0 = random; 1 = black and white; 2 = orientation; 3 = colormap1; 4 = colormap2; 5 = colormap3

	//Criterions
	unsigned m_criterion = 0;//0 = none; 1 = length; 2 = zoi; 3 = lateralization

	bool m_clickZOI = false;
	//Culling criterion
	unsigned m_minCullingCriterion = 0;
	unsigned m_maxCullingCriterion = 255;

	//Post Process
	unsigned m_postProcess = 0;//0 Diffuse; 1 Illuminated lines;

	//Mesh cut variables
	bool m_drawMeshCut = false;
	bool m_useMeshCut = false;
	bool m_fragmentTestOccluder = true;
	bool m_cullOutsideMesh = false;

	//Pipeline
	bool m_tessellation = true;
	bool m_geometry = false;
	bool m_mesh = false;

	//Spectator
	bool m_spectatorMode = false;

	//SELECTED RENDERING
	bool m_clickSelected;
	glm::ivec2 m_mousePos;
	bool m_forceSelectedReset;
	#define VOID_FIBER_ID 0xffffffff
	#define TRACTO_TRANSPARENCY_VALUE (0.45f)
	float m_tractoTransparency=0.0f;//is also used as test value for selected rendering
	//array of IBO (one per SSBO) is required to prepare draw call with selected fibers
	GLuint* m_iboSelectedFibers;
	GLuint* m_iboSelectedElementNumber;
	std::set<GLuint> m_selectedFibersToRender;
	void BuildSelectedRenderingBuffers(bool clicked,glm::ivec2 mouseCoord,int selectSize,bool forceReset);

	void renderSelectedFibers();
	void DeleteSelectedBuffers();
	void CreateSelectedIBO();


};



#endif // SCENE_H
