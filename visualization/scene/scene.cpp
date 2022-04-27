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

#include "scene.h"
#include PATHINC(interfacegui/widgetopengl.h)

float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

scene::scene()
{

}

void scene::resizeWindow()
{
	uint32_t f = 4 << FILTER_PASSES;//8*pow(2,FILTER_PASSES);
//m_sizeScreen has been updated before call
	m_sizeZtestViewPort.x = (m_sizeScreen.x/(SUBSAMBLE_ZBUFFER_FACTOR*4)+1)*4;
	m_sizeZtestViewPort.y = (m_sizeScreen.y/(SUBSAMBLE_ZBUFFER_FACTOR*4)+1)*4;
	m_sizeFBO.x = (m_sizeScreen.x/f+1)*f;
	m_sizeFBO.y = (m_sizeScreen.y/f+1)*f;
	m_sizeSubsampledZBuffers.x = m_sizeFBO.x / f * 4;
	m_sizeSubsampledZBuffers.y = m_sizeFBO.y / f * 4;
	m_pwidget->cam.setScreenSize(m_sizeFBO.x, m_sizeFBO.y);
	//ibo
	/*
	cout << " m_sizeScreen : " << m_sizeScreen.x << "X" << m_sizeScreen.y << endl;
	cout << " m_sizeZtestViewPort : " << m_sizeZtestViewPort.x << "X" << m_sizeZtestViewPort.y << endl;
	cout << " m_sizeFBO : " << m_sizeFBO.x << "X" << m_sizeFBO.y << endl;
	cout << " m_sizeSubsampledZBuffers : " << m_sizeSubsampledZBuffers.x << "X" << m_sizeSubsampledZBuffers.y << endl <<endl;
	*/

	flushStaticUBO();
	//making sure that textures/renderbuffers are not bound
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	//function called automatically free previous allocated memory !
	glTextureImage2DEXT(gBufferTex, GL_TEXTURE_2D, 0, GL_RG32UI, m_sizeFBO.x, m_sizeFBO.y, 0, GL_RG_INTEGER, GL_UNSIGNED_INT, NULL);
	glTextureImage2DEXT(gBufferZTex, GL_TEXTURE_2D, 0, DEPTH_BUFFER_TYPE, m_sizeFBO.x, m_sizeFBO.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glNamedRenderbufferStorageMultisample(tractorRenderColorBufferMS, MS_SAMPLES, GL_RGB8, m_sizeFBO.x, m_sizeFBO.y);
	glNamedRenderbufferStorageMultisample(tractorRenderZBufferMS, MS_SAMPLES, DEPTH_BUFFER_TYPE, m_sizeFBO.x, m_sizeFBO.y);

	//as glTextureImage2DMultisample does not seem to exist, let's allocate the bad old way...
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tractorRenderColorTexMS);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, MS_SAMPLES, GL_RGB8, m_sizeFBO.x, m_sizeFBO.y, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tractorRenderZTexMS);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, MS_SAMPLES, DEPTH_BUFFER_TYPE, m_sizeFBO.x, m_sizeFBO.y, GL_TRUE);
	glm::ivec2 ts = m_sizeFBO;
	for (uint32_t i = 0; i <= FILTER_PASSES; i++)
	{
		glTextureImage2DEXT(zFilterBackTex[i], GL_TEXTURE_2D, 0, DEPTH_BUFFER_TYPE, ts.x, ts.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		ts /= 2;
	}
	for (uint32_t i = 0; i < 4; i++)
	{
		glTextureImage2DEXT(zChannelTex[i], GL_TEXTURE_2D, 0, DEPTH_BUFFER_TYPE, m_sizeZtestViewPort.x, m_sizeZtestViewPort.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}
	for (uint32_t i = 0; i < 2; i++)
	{
		glTextureImage2DEXT(zChannelFragmentTex[i], GL_TEXTURE_2D, 0, DEPTH_BUFFER_TYPE, m_sizeFBO.x, m_sizeFBO.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}

	glTextureImage2DEXT(zFragmentTestTex, GL_TEXTURE_2D, 0, GL_RG16, m_sizeFBO.x, m_sizeFBO.y, 0, GL_RG, GL_FLOAT, NULL);
	glTextureImage2DEXT(zTestTex, GL_TEXTURE_2D, 0, GL_RGB16, m_sizeZtestViewPort.x, m_sizeZtestViewPort.y, 0, GL_RGB, GL_FLOAT, NULL);
}

void scene::captureImage()
{
	if (m_filename == "")
		return;
	int width = int(m_pwidget->cam.getScreenSizeX());
	int height = int(m_pwidget->cam.getScreenSizeY());

	char* data = (char*)calloc(width * height * 3, sizeof(char));
	char* dataCorrected = (char*)calloc(width * height * 3, sizeof(char));

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE,  data); PRINT_OPENGL_ERROR();

	for (int i=0; i<height; i++)
	{
		for (int j=0; j<width*3; j++)
		{
			dataCorrected[i * width * 3 + j] = data[(height - i - 1) * width * 3 + j];
		}
	}

	size_t beginName = m_filename.find_last_of("/") + 1;
	string filename = m_filename.substr(beginName, m_filename.find_last_of(".") - beginName);
	string imageName = "Screenshot-" + filename + "-" + to_string(0) + ".png";;
	unsigned nb = 0;
	while(access( imageName.c_str(), F_OK ) != -1)
	{
		nb++;
		imageName = "Screenshot-" + filename + "-" + to_string(nb) + ".png";
	}
	stbi_write_png(imageName.c_str(), width, height, 3, dataCorrected, 3 * width * sizeof(uint8_t));
	cout << "Image " << imageName << " recorded" << endl;
}


void scene::initFBOrelated()
{
	//making sure that textures/renderbuffers are not bound
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glEnable(GL_MULTISAMPLE);
	//creating FBO and textures
	glCreateFramebuffers(1, &gBufferFBO);
	glCreateTextures(GL_TEXTURE_2D, 1, &gBufferTex);
	glCreateTextures(GL_TEXTURE_2D, 1, &gBufferZTex);
	glCreateFramebuffers(1, &copyRenderFBOMS);
	glCreateFramebuffers(1, &tractoRenderFBOMS);
	glCreateRenderbuffers(1, &tractorRenderColorBufferMS);
	glCreateRenderbuffers(1, &tractorRenderZBufferMS);
	glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &tractorRenderColorTexMS);
	glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &tractorRenderZTexMS);
	glCreateFramebuffers(1, &zFilterFBO);
	for (uint32_t i = 0; i <= FILTER_PASSES; i++)
	{
		glCreateTextures(GL_TEXTURE_2D, 1, &zFilterBackTex[i]);
	}
	glCreateFramebuffers(1, &zTestFragmentFBO);
	for (uint32_t i = 0; i < 2; i++)
		glCreateTextures(GL_TEXTURE_2D, 1, &zChannelFragmentTex[i]);


	for (uint32_t i = 0; i < 4; i++)
		glCreateFramebuffers(1, &zTestFBO[i]);
	for (uint32_t i = 0; i < 4; i++)
		glCreateTextures(GL_TEXTURE_2D, 1, &zChannelTex[i]);

	glCreateFramebuffers(1, &zCopyFragmentFBO);
	glCreateTextures(GL_TEXTURE_2D, 1, &zFragmentTestTex);
	glCreateFramebuffers(1, &zCopyTestFBO);
	glCreateTextures(GL_TEXTURE_2D, 1, &zTestTex);


	// binding FBO texture that won't change in the rendering pipeline

	glNamedFramebufferDrawBuffer(gBufferFBO, GL_COLOR_ATTACHMENT0);//color buffer, depth is implicitly bound
	glNamedFramebufferReadBuffer(gBufferFBO, GL_COLOR_ATTACHMENT0);
	glNamedFramebufferTexture(gBufferFBO, GL_COLOR_ATTACHMENT0, gBufferTex, 0);
	glNamedFramebufferTexture(gBufferFBO, GL_DEPTH_ATTACHMENT, gBufferZTex, 0);

	glNamedFramebufferReadBuffer(copyRenderFBOMS, GL_COLOR_ATTACHMENT0);
	glNamedFramebufferDrawBuffer(tractoRenderFBOMS, GL_COLOR_ATTACHMENT0);
	glNamedFramebufferReadBuffer(tractoRenderFBOMS, GL_COLOR_ATTACHMENT0);



	glNamedFramebufferDrawBuffer(zFilterFBO, GL_NONE);
	glNamedFramebufferReadBuffer(zFilterFBO, GL_NONE);


	for (uint32_t i = 0; i < 4; i++)
	{

	glNamedFramebufferReadBuffer(zTestFBO[i], GL_NONE);
	glNamedFramebufferDrawBuffer(zTestFBO[i], GL_NONE);
		glNamedFramebufferTexture(zTestFBO[i], GL_DEPTH_ATTACHMENT, zChannelTex[i], 0);
	}



	glNamedFramebufferReadBuffer(zTestFragmentFBO, GL_NONE);
	glNamedFramebufferDrawBuffer(zTestFragmentFBO, GL_NONE);

	glNamedFramebufferDrawBuffer(zCopyFragmentFBO,GL_COLOR_ATTACHMENT0);
	glNamedFramebufferReadBuffer(zCopyFragmentFBO, GL_NONE);
		glNamedFramebufferTexture(zCopyFragmentFBO, GL_COLOR_ATTACHMENT0, zFragmentTestTex, 0);//RG

	glNamedFramebufferDrawBuffer(zCopyTestFBO,GL_COLOR_ATTACHMENT0);
	glNamedFramebufferReadBuffer(zCopyTestFBO, GL_NONE);
		glNamedFramebufferTexture(zCopyTestFBO, GL_COLOR_ATTACHMENT0, zTestTex, 0);//RGB



	//defining texture / renderbuffer object flags
	float colorzf[] = { 1.0f, 1.0f, 1.0f, 0.0f };

	glTextureParameteri(gBufferTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(gBufferTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTextureUnit(GBUFFER_COLOR_SLOT_TEXTURE, gBufferTex);//static binding
	glTextureParameterfv(gBufferZTex, GL_TEXTURE_BORDER_COLOR, colorzf);
		glTextureParameteri(gBufferZTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTextureParameteri(gBufferZTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTextureParameteri(gBufferZTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(gBufferZTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);PRINT_OPENGL_ERROR();
	GLint mipLevMax = 0;
	glTextureParameteriv(tractorRenderZTexMS, GL_TEXTURE_MAX_LEVEL, &mipLevMax);
	glTextureParameteriv(tractorRenderColorTexMS, GL_TEXTURE_MAX_LEVEL, &mipLevMax);

	for (uint32_t i = 0; i <= FILTER_PASSES; i++)
	{
		glTextureParameterfv(zFilterBackTex[i], GL_TEXTURE_BORDER_COLOR, colorzf); PRINT_OPENGL_ERROR();
			glTextureParameteri(zFilterBackTex[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTextureParameteri(zFilterBackTex[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTextureParameteri(zFilterBackTex[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(zFilterBackTex[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	glTextureParameteri(zFragmentTestTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(zFragmentTestTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(zFragmentTestTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(zFragmentTestTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	for (uint32_t i = 0; i < 4; i++)
	{
		glTextureParameterfv(zChannelTex[i], GL_TEXTURE_BORDER_COLOR, colorzf); PRINT_OPENGL_ERROR();
			glTextureParameteri(zChannelTex[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTextureParameteri(zChannelTex[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTextureParameteri(zChannelTex[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(zChannelTex[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
		for (uint32_t i = 0; i < 2; i++)
	{
		glTextureParameterfv(zChannelFragmentTex[i], GL_TEXTURE_BORDER_COLOR, colorzf); PRINT_OPENGL_ERROR();
			glTextureParameteri(zChannelFragmentTex[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTextureParameteri(zChannelFragmentTex[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTextureParameteri(zChannelFragmentTex[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(zChannelFragmentTex[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	glTextureParameteri(zTestTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);//Edge is important
		glTextureParameteri(zTestTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(zTestTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(zTestTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void scene::init()
{
	m_sizeScreen = glm::ivec2(800, 600);
	m_vaoQuad = 0;
	int maxTessComponents;
	glGetIntegerv( GL_MAX_TESS_PATCH_COMPONENTS, &maxTessComponents);
	int maxTessTotalComponents;
	glGetIntegerv( GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS, &maxTessTotalComponents);
	GLint64 maxSSBOSize;
	glGetInteger64v( GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxSSBOSize);
	GLint64 maxBufferAddress;
	glGetInteger64v( GL_MAX_SHADER_BUFFER_ADDRESS_NV, &maxBufferAddress);
//	std::cout << maxTessComponents << " " << maxTessTotalComponents << " " << maxSSBOSize << " " << maxBufferAddress << std::endl; PRINT_OPENGL_ERROR();
	//*****************************************//
	// Preload default structure               //
	//*****************************************//
	//
	glCreateVertexArrays(1, &m_vaoDummy);

	//ubos
	glCreateBuffers(1, &staticUBO);
	glCreateBuffers(1, &dynamicUBO);

	glNamedBufferStorage(staticUBO, sizeof(staticUboDataFormat), nullptr, GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferStorage(dynamicUBO, sizeof(dynamicUboDataFormat), nullptr, GL_DYNAMIC_STORAGE_BIT);

	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_STATIC_BINDING_POINT, staticUBO);
	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_DYNAMIC_BINDING_POINT, dynamicUBO); PRINT_OPENGL_ERROR();


	glEnable(GL_PROGRAM_POINT_SIZE);//point size changed within shader
	glFrontFace(GL_CW);
	glEnable(GL_DEPTH_TEST);
	glPatchParameteri(GL_PATCH_VERTICES, 1);


	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	GLfloat vals[4];
	vals[0] = 64.0f;vals[1] = 64.0f;vals[2] = 64.0f;vals[3] = 64.0f;
	glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, (const GLfloat*)(vals));
	glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, (const GLfloat*)(vals));
	//Texture Creation
	initFBOrelated();



	loadOccluder(PATHINC(scene/cube.obj));

	//texture unit binding
	glBindTextureUnit(GBUFFER_COLOR_SLOT_TEXTURE, gBufferTex);
	glBindTextureUnit(GBUFFER_DEPTH_SLOT_TEXTURE, gBufferZTex);
	glBindTextureUnit(GBUFFER_COLOR_SLOT_TEXTURE_MS, tractorRenderColorTexMS);
	glBindTextureUnit(GBUFFER_DEPTH_SLOT_TEXTURE_MS, tractorRenderZTexMS);
	glBindTextureUnit(FRAGMENT_CULLING_SLOT_TEXTURE, zFragmentTestTex);
	glBindTextureUnit(FIBER_CULLING_SLOT_TEXTURE, zTestTex);
	glBindTextureUnit(FILTERING_SLOT_TEXTURE, zFilterBackTex[0]);
	//FILTERING_SLOT_TEXTURE is dynamic because of multipass filtering zFilterBackTex[FILTER_PASSES]
	for(int i=0;i<4;i++)
	glBindTextureUnit(OCCLUDER_SLOT_TEXTURE+i, zChannelTex[i]);
	for(int i=0;i<2;i++)
	glBindTextureUnit(OCCLUDER_SLOT_TEXTURE+4+i, zChannelFragmentTex[i]);

	reloadShaders(); PRINT_OPENGL_ERROR();
	resizeWindow(); PRINT_OPENGL_ERROR();
}

void scene::setWhiteBackground(bool white)
{
	if (white)
	{
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		m_clearColor = 1;
	}
	else
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		m_clearColor = 0;
	}
}

void scene::loadOccluder(const string fileName)
{
	m_occluderTransfoM = glm::mat4(1.f);
	m_occluderTransfoM[0][0] = 40.0f;
	m_occluderTransfoM[1][1] = m_occluderTransfoM[0][0];
	m_occluderTransfoM[2][2] = m_occluderTransfoM[0][0];
	m_occluderTransfoM[3][0] = 80;
	m_occluderTransfoM[3][1] = 80;
	m_occluderTransfoM[3][2] = 80;
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fileName.c_str());
	if (err != "")
	{
		cout << "  TINY OBJ LOADER ERROR " << err.c_str() << " ... " << endl;
		m_meshCutLoaded  =false;
		return;
	}
	std::cout << "# of vertices  : " << (attrib.vertices.size() / 3) << std::endl;
	std::cout << "# of normals   : " << (attrib.normals.size() / 3) << std::endl;



	if (attrib.normals.size() != attrib.vertices.size())
	{
		cout << " OCCLUDER REQUIRE PER VERTEX NORMALS !" << endl;
		m_meshCutLoaded  =false;
		return;
	}

	int nbmesh = (int)shapes.size();
	m_nbVertexOccluder = 0;
	for (int i = 0; i<nbmesh; i++)
	{
		m_nbVertexOccluder += (int)shapes[i].mesh.indices.size();
	}
	std::cout << "# of tris : " << m_nbVertexOccluder<< std::endl;
	float* OccluderData = new float[m_nbVertexOccluder*2*3];//vertice + normal

	int  currVertex = 0;
	tinyobj::index_t s[3];
	for (int i = 0; i<nbmesh; i++)
	{
		for (unsigned int j = 0; j<shapes[i].mesh.indices.size() / 3; j++)
		{
			s[0] = shapes[i].mesh.indices[j * 3];
			s[2]= shapes[i].mesh.indices[j * 3 + 1];
			s[1] = shapes[i].mesh.indices[j * 3 + 2];

			for (unsigned int k = 0; k < 3; k++)
			{
				for (unsigned int u = 0; u < 3; u++)
				{
					OccluderData[3*currVertex+u] = attrib.vertices[3 * s[k].vertex_index + u];
					OccluderData[3*(currVertex+1)+u] = attrib.normals[3 * s[k].normal_index + u];
				}
				currVertex +=2;
			}
		}
	}


	glGenVertexArrays(1, &m_vaoOccluder);
	glGenBuffers(1, &m_vboOccluder);
	glBindVertexArray(m_vaoOccluder);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboOccluder);
	glBufferData(GL_ARRAY_BUFFER, m_nbVertexOccluder *3*2*sizeof(float), OccluderData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); PRINT_OPENGL_ERROR();
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); PRINT_OPENGL_ERROR();
	delete[] OccluderData;
	m_meshCutLoaded  = true ;
}


void scene::SetGuardBand(float stepLength, uint32_t NbPoint)
{
	m_guardBand = stepLength*1.01f*NbPoint * m_tracto.scaling;
}


bool scene::load_scene(string filename, bool uncompressedFile)
{
	if (!m_firstOpenFile)
		close_scene();
	m_firstOpenFile = false;
	m_filename = filename;
	m_FillTck = uncompressedFile;

	//*****************************************//
	// Load fibers                             //
	//*****************************************//

	auto start = chrono::steady_clock::now();

	// Load fiber data
	cout << "  Load data from file " << filename << " ... " << endl;
	loadTracto(filename,m_FillTck);
	cout << "   [OK] " << endl;

	glm::mat4 scaleMat = glm::mat4(0.0f);
	scaleMat[0][0] = m_tracto.scaling;
	scaleMat[1][1] = m_tracto.scaling;
	scaleMat[2][2] = m_tracto.scaling;
	scaleMat[3][3] = 1.0f;
	glm::mat4 offsetMat = glm::mat4(0.0f);
	offsetMat[0][0] = 1.0f;
	offsetMat[1][1] = 1.0f;
	offsetMat[2][2] = 1.0f;
	offsetMat[3] = glm::vec4(-m_tracto.offset.x, -m_tracto.offset.y, -m_tracto.offset.z, 1.0f);

	m_modelWorld = offsetMat * scaleMat;
	//cout << "Scaling : " << m_tracto.scaling << endl;
	flushStaticUBO();

	//*****************************************//
	// OPENGL initialization                   //
	//*****************************************//
	{
		cout << "  Loading SSBOs ... " << endl;

		//SSBO containing compressed fibers
		const unsigned SSBOBlockSize = m_tracto.packSize / 4;
		//cout << "Size of buffer requested: " << SSBOBlockSize * sizeof(GLuint) * m_tracto.packedCompressedFibers.size() << " " << m_tracto.packedCompressedFibers.size() << endl;
		ssboCompressedFibers.resize(m_tracto.startPositionOfSSBOs.size());
		uint64_t pos = 0;
		for (unsigned int numOfSSBO = 0; numOfSSBO < m_tracto.startPositionOfSSBOs.size(); numOfSSBO++)// splitting  along ssbo
		{
			glCreateBuffers(1, &ssboCompressedFibers[numOfSSBO]); PRINT_OPENGL_ERROR();
			glNamedBufferData(ssboCompressedFibers[numOfSSBO], SSBOBlockSize * sizeof(GLuint) * (m_tracto.startPositionOfSSBOs[numOfSSBO] - pos), NULL, GL_STATIC_DRAW);PRINT_OPENGL_ERROR();
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboCompressedFibers[numOfSSBO]);PRINT_OPENGL_ERROR();
			GLuint* datassbo = (GLuint*)(glMapNamedBuffer(ssboCompressedFibers[numOfSSBO], GL_WRITE_ONLY));PRINT_OPENGL_ERROR();
			for (uint64_t i=pos; i<m_tracto.startPositionOfSSBOs[numOfSSBO]; i++)
			{
				for (unsigned int j=0; j<SSBOBlockSize; j++)//raw copy of a block
				{
					datassbo[(i - pos) * SSBOBlockSize + j] = (m_tracto.packedCompressedFibers[i][j * 4] << 24 | m_tracto.packedCompressedFibers[i][j * 4 + 1] << 16 | m_tracto.packedCompressedFibers[i][j * 4 + 2] << 8 | m_tracto.packedCompressedFibers[i][j * 4 + 3]);
				}
			}
			pos = m_tracto.startPositionOfSSBOs[numOfSSBO];
			glUnmapNamedBuffer(ssboCompressedFibers[numOfSSBO]);PRINT_OPENGL_ERROR();
		}
		m_numberOfPacks = uint64_t(m_tracto.packedCompressedFibers.size());
		//cout << "number of packs : " << m_numberOfPacks << endl;
		//cout << "number of SSBOs: " << m_tracto.startPositionOfSSBOs.size() << endl;

		glCreateBuffers(1, &ssboCriterion);
		glNamedBufferData(ssboCriterion, ceil(m_tracto.nbOfFibers / 4)*4, NULL, GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_CRITERION_SLOT, ssboCriterion);


		//cout << "   [OK] " << endl;

	}
	CreateSelectedIBO();

	//cout << "Run drawing" << endl;

	auto end = chrono::steady_clock::now();
	auto diff = end-start;
	cout << "Total computational time : " << chrono::duration <double, milli> (diff).count() << " ms" << endl;
	m_tractoLoaded = true;
	PRINT_OPENGL_ERROR();
	resizeWindow();

	return true;
}

void scene::FlushCriterionSSBO(void) //slow function when flushing
{
	static unsigned prev_criterion = 0;
	//m_criterion
	if (m_criterion == 0)
		return;
	if (m_criterion == prev_criterion && m_criterion!=2)//since zone of interest if flushed on picking 
		return;
	if (m_criterion == 2 && m_clickZOI)//zone of interest
	{
		m_clickZOI = false;
		//retrieve position of ZOI
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBufferFBO);
		float pixelDepth[1];
		glReadPixels(m_mousePos.x, m_mousePos.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, (void*)(pixelDepth));

		if (pixelDepth[0]==1.0f)
			cout <<"Zone of interest updated [background selected] "<< endl;
		else
			cout <<"Zone of interest updated"<< endl;

		glm::mat4 imat = glm::inverse(dynamicDataUbo.projCamMultMV);
		glm::vec4 sic4 = imat * glm::vec4(m_mousePos.x/float(m_sizeFBO.x)*2.0f-1.0f,m_mousePos.y/float(m_sizeFBO.y)*2.0f-1.0f,pixelDepth[0]*2.0f-1.0f,1.0f);//sphere of interest center
		glm::vec3 sic = glm::vec3(sic4)/sic4.w;
		//cout << sic.x << " " << sic.y << " " << sic.z << endl;
		GLuint* dataccritssbo = (GLuint*)(glMapNamedBuffer(ssboCriterion, GL_WRITE_ONLY));
		std::vector<float> criterion;
		criterion.resize(m_tracto.nbOfFibers + 4);
		for (int i = 0; i < m_tracto.nbOfFibers + 4; i++)
		{
			criterion[i]=5.0e15f;
		}

		for (uint64_t i = 0; i < m_tracto.packedCompressedFibers.size(); i++)
		{
			uint32_t fid = m_tracto.packedCompressedFibers[i][12] << 24 | m_tracto.packedCompressedFibers[i][13] << 16 | m_tracto.packedCompressedFibers[i][14] << 8 | m_tracto.packedCompressedFibers[i][15];
			glm::vec2 xy = glm::unpackUnorm2x16(m_tracto.packedCompressedFibers[i][0] << 24 | m_tracto.packedCompressedFibers[i][1] << 16 | m_tracto.packedCompressedFibers[i][2] << 8 | m_tracto.packedCompressedFibers[i][3]);
			glm::vec2 zw = glm::unpackUnorm2x16(m_tracto.packedCompressedFibers[i][4] << 24 | m_tracto.packedCompressedFibers[i][5] << 16 | m_tracto.packedCompressedFibers[i][6] << 8 | m_tracto.packedCompressedFibers[i][7]);
			glm::vec3 xyz = glm::vec3(xy.x, xy.y, zw.x);
			//sphere criterion
			criterion[fid] = glm::min(criterion[fid],glm::dot(xyz - sic, xyz - sic));
		}
		for (int i = 0; i < m_tracto.nbOfFibers; i++)
		{
			criterion[i] = sqrtf(criterion[i]);
		}

		float minL = criterion[0];
		float maxL = criterion[0];
		for (int i = 1; i < m_tracto.nbOfFibers ; i++)
		{
			if (criterion[i] < minL)
				minL = criterion[i];
			else if (criterion[i] > maxL)
				maxL = criterion[i];
		}

		float df = 255.0f / float(maxL - minL);
		for (int i = 0; i < m_tracto.nbOfFibers; i++)
		{
			criterion[i] = 255.0f-float(criterion[i] - minL)*df;//between 0.0f-255.0f
		}



		for (int i = 0; i < ceil(m_tracto.nbOfFibers / 4); i++)
		{
			GLuint v = 0;
			for (int j = 0; j < 4; j++)
			{
				uint32_t id = 4 * i + j;
				v += uint32_t(criterion[id]) << (8 * j);
			}
			dataccritssbo[i] = v;
		}

	glUnmapNamedBuffer(ssboCriterion);
	}
	else if (m_criterion == 1) //length
	{
		GLuint* dataccritssbo = (GLuint*)(glMapNamedBuffer(ssboCriterion, GL_WRITE_ONLY));
		std::vector<uint32_t> criterion;
		criterion.resize(m_tracto.nbOfFibers + 4);

		for (int i = 0; i < m_tracto.nbOfFibers + 4; i++)
		{
			criterion[i]=0;
		}
		for (uint64_t i = 0; i < m_tracto.packedCompressedFibers.size(); i++)
		{
			uint32_t fid = m_tracto.packedCompressedFibers[i][12] << 24 | m_tracto.packedCompressedFibers[i][13] << 16 | m_tracto.packedCompressedFibers[i][14] << 8 | m_tracto.packedCompressedFibers[i][15];
			uint32_t nbPoint = m_tracto.packedCompressedFibers[i][16]&0x3F;
			criterion[fid]+= nbPoint;
		}

		uint32_t minL = criterion[0];
		uint32_t maxL = criterion[0];
		for (int i = 1; i < m_tracto.nbOfFibers ; i++)
		{
			if (criterion[i] < minL)
				minL = criterion[i];
			else if (criterion[i] > maxL)
				maxL = criterion[i];
		}

		float df = 255.0f / float(maxL - minL);
		for (int i = 0; i < m_tracto.nbOfFibers; i++)
		{
			criterion[i] = uint32_t(float(criterion[i] - minL)*df);//between 0-255
		}

		for (int i = 0; i < ceil(m_tracto.nbOfFibers / 4); i++)
		{
			GLuint v = 0;
			for (int j = 0; j < 4; j++)
			{
				uint32_t id = 4 * i + j;
				v += criterion[id] << (8 * j);
			}
			dataccritssbo[i] = v;
		}

	glUnmapNamedBuffer(ssboCriterion);
	}
	else if (m_criterion == 3) // lateralization
	{
		GLuint* dataccritssbo = (GLuint*)(glMapNamedBuffer(ssboCriterion, GL_WRITE_ONLY));
		std::vector<float> criterion;
		criterion.resize(m_tracto.nbOfFibers + 4);
		std::vector<uint32_t> criterionFreq;
		criterionFreq.resize(m_tracto.nbOfFibers + 4);
		for (int i = 0; i < m_tracto.nbOfFibers + 4; i++)
		{
			criterion[i]=0.0f;
			criterionFreq[i] = 0;

		}
		for (uint64_t i = 0; i < m_tracto.packedCompressedFibers.size(); i++)
		{
			uint32_t fid = m_tracto.packedCompressedFibers[i][12] << 24 | m_tracto.packedCompressedFibers[i][13] << 16 | m_tracto.packedCompressedFibers[i][14] << 8 | m_tracto.packedCompressedFibers[i][15];
			glm::vec2 xy = glm::unpackUnorm2x16(m_tracto.packedCompressedFibers[i][0] << 24 | m_tracto.packedCompressedFibers[i][1] << 16 | m_tracto.packedCompressedFibers[i][2] << 8 | m_tracto.packedCompressedFibers[i][3]);
			criterionFreq[fid]++;
			criterion[fid]+=xy.x;
		}
		for (int i = 0; i < m_tracto.nbOfFibers; i++)
		{
			criterion[i]/= float(criterionFreq[i]);
		}

		float minL = criterion[0];
		float maxL = criterion[0];
		for (int i = 1; i < m_tracto.nbOfFibers ; i++)
		{
			if (criterion[i] < minL)
				minL = criterion[i];
			else if (criterion[i] > maxL)
				maxL = criterion[i];
		}

		float df = 255.0f / float(maxL - minL);
		for (int i = 0; i < m_tracto.nbOfFibers; i++)
		{
			criterion[i] = float(criterion[i] - minL)*df;//between 0.0f-255.0f
		}

		for (int i = 0; i < ceil(m_tracto.nbOfFibers / 4); i++)
		{
			GLuint v = 0;
			for (int j = 0; j < 4; j++)
			{
				uint32_t id = 4 * i + j;
				v += uint32_t(criterion[id]) << (8 * j);
			}
			dataccritssbo[i] = v;
		}


		glUnmapNamedBuffer(ssboCriterion);
	}


	prev_criterion = m_criterion;
}




void scene::renderQuad()
{
	if (m_vaoQuad == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &m_vaoQuad);
		glGenBuffers(1, &m_vboQuad);
		glBindVertexArray(m_vaoQuad);
		glBindBuffer(GL_ARRAY_BUFFER, m_vboQuad);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(m_vaoQuad);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void scene::flushStaticUBO()
{

	staticDataUbo.data0.x = m_sizeZtestViewPort.x;
	staticDataUbo.data0.y = m_sizeZtestViewPort.y;
	staticDataUbo.data0.z = 2.0f/(m_sizeFBO.x-1.0f);
	staticDataUbo.data0.w = 2.0f/(m_sizeFBO.y-1.0f);

	staticDataUbo.data1.x = m_guardBand;
	staticDataUbo.data1.y = 1.45f*tan(m_pwidget->cam.getFovyRad()*0.5f)*(1<<FILTER_PASSES)/m_sizeFBO.y;//due to resolution //1.5 = abit more than sqrt2
	staticDataUbo.data1.z = m_tracto.ratio;
	staticDataUbo.data1.w = m_tracto.scaling;

	staticDataUbo.data2.x = 0.0f;//unused
	staticDataUbo.data2.y = m_tracto.stepsize * m_tracto.scaling*m_sizeFBO.y*0.5f/(FRAGMENTS_PER_LINE*tan(m_pwidget->cam.getFovyRad()*0.5f));
	uint32_t params = 0;
	params += glm::clamp(m_minCullingCriterion * uint32_t(m_criterion>0), uint32_t(0), uint32_t(255)) << 24;
	params += glm::clamp(m_maxCullingCriterion, uint32_t(0), uint32_t(255)) << 16;
	params += glm::clamp(m_criterion, uint32_t(0), uint32_t(255)) << 8;
	params += glm::clamp(m_color*4+m_clearColor*2+m_postProcess, uint32_t(0), uint32_t(255));
	staticDataUbo.data2.z = glm::uintBitsToFloat(params);
	staticDataUbo.data2.w = m_sizeSubsampledZBuffers.x;

	staticDataUbo.data3.x = m_guardBand/sin(m_pwidget->cam.getFovxRad()*0.5f);
	staticDataUbo.data3.y = m_guardBand/sin(m_pwidget->cam.getFovyRad()*0.5f);
	staticDataUbo.data3.z = m_pwidget->cam.getNear() + m_guardBand;
	staticDataUbo.data3.w = m_tracto.stepsize;

	staticDataUbo.sliders = m_sliderValue;

	glNamedBufferSubData(staticUBO, 0, sizeof(staticUboDataFormat), &staticDataUbo);
}

void scene::flushDynamicUBO()
{
	glm::mat4 texelShift = glm::mat4(0.0f);//maps [0,m_sizeZtestViewPort-1] to [-1,1] and [0,1]z to [-1,1z]
	texelShift[0][0] = 2.0f/(m_sizeZtestViewPort.x);
	texelShift[1][1] = 2.0f/(m_sizeZtestViewPort.y);
	texelShift[2][2] = 2.0f;
	texelShift[3] = glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f);

	dynamicDataUbo.ProjMultCurrentPoseMultInvPreviousProjPoseMultTexShift = glm::inverse(dynamicDataUbo.projCamMultMV)*texelShift;//inverse previous projection

	//update current projection
	glm::mat4 ProjSpec = m_pwidget->cam.getProjection();

	if (m_spectatorMode==0) //we flush params
	dynamicDataUbo.projCam = ProjSpec;

	glm::mat4 iproj = glm::inverse(dynamicDataUbo.projCam);


	texelShift = glm::mat4(0.0f);//maps [0,m_sizeSubsampledZBuffers-1] to [-1,1] and [0,1]z to [-1,1]z
	texelShift[0][0] = 2.0f/(m_sizeSubsampledZBuffers.x);//size of gbuffer
	texelShift[1][1] = 2.0f/(m_sizeSubsampledZBuffers.y);
	texelShift[2][2] = 2.0f;
	texelShift[3] = glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f);
	dynamicDataUbo.ZFilterinvCamProjMultTexShift = iproj*texelShift;

	static glm::mat4 WV = glm::mat4(1.0f);
	glm::mat4 WVspec = m_pwidget->cam.getWorldview();
	if (m_spectatorMode == 0) //we flush params
		WV = WVspec;

	dynamicDataUbo.ModelViewCam = WV * m_modelWorld;
	dynamicDataUbo.projCamMultMV = dynamicDataUbo.projCam * dynamicDataUbo.ModelViewCam;

	dynamicDataUbo.projSpec = ProjSpec;
	dynamicDataUbo.invProjSpec = glm::inverse(ProjSpec);
	dynamicDataUbo.ModelViewSpec = WVspec*m_modelWorld;
	dynamicDataUbo.projSpecMultMV = dynamicDataUbo.projSpec*dynamicDataUbo.ModelViewSpec;
	dynamicDataUbo.ModelViewOccluder = WV* m_occluderTransfoM;

	dynamicDataUbo.ProjMultCurrentPoseMultInvPreviousProjPoseMultTexShift =
		dynamicDataUbo.projCamMultMV * dynamicDataUbo.ProjMultCurrentPoseMultInvPreviousProjPoseMultTexShift;


	//light management
	glm::mat4 lights;
	float distLight = 0.8f;//actually this is the double of the distance
						   //4 lights are made as a tetrahedron centered in 0
	lights[0][0] = distLight + 0.5f;
	lights[0][1] = distLight + 0.5f;
	lights[0][2] = distLight + 0.5f;
	lights[1][0] = distLight + 0.5f;
	lights[1][1] = -distLight + 0.5f;
	lights[1][2] = -distLight + 0.5f;
	lights[2][0] = -distLight + 0.5f;
	lights[2][1] = distLight + 0.5f;
	lights[2][2] = -distLight + 0.5f;
	lights[3][0] = -distLight + 0.5f;
	lights[3][1] = -distLight + 0.5f;
	lights[3][2] = distLight + 0.5f;
	lights[0][3] = 1.0f;
	lights[1][3] = 1.0f;
	lights[2][3] = 1.0f;
	lights[3][3] = 1.0f;
	dynamicDataUbo.lights = dynamicDataUbo.ModelViewSpec * lights;



	glNamedBufferSubData(dynamicUBO, 0, sizeof(dynamicUboDataFormat), &dynamicDataUbo);
}


void scene::ProcessZoccluder()
{

	bool cstate = m_meshCutLoaded && (m_drawMeshCut || m_useMeshCut);//current
	if (cstate)
	{
		glBindVertexArray(m_vaoOccluder);
		glDepthFunc(GL_LESS);
		glClearDepth(1.0);
		glEnable(GL_CULL_FACE);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, zTestFBO[ZOB]);
		glUseProgram(m_BackOccluderProgram);//fiberOccluder
		glClear(GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_FRONT);
		glDrawArrays(GL_TRIANGLES, 0, m_nbVertexOccluder);//back
		glFlush();
		glCullFace(GL_BACK);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, zTestFBO[ZOF]);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, m_nbVertexOccluder);//front
		glFlush();
		if (m_fragmentTestOccluder)
		{
			glViewport(0, 0, m_sizeFBO.x, m_sizeFBO.y);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, zTestFragmentFBO);
			glUseProgram(m_BackFragmentOccluderProgram);//fragmentOccluder
			glNamedFramebufferTexture(zTestFragmentFBO, GL_DEPTH_ATTACHMENT, zChannelFragmentTex[ZOFB], 0);
			glClear(GL_DEPTH_BUFFER_BIT);
			glCullFace(GL_FRONT);
			glDrawArrays(GL_TRIANGLES, 0, m_nbVertexOccluder);//back
			glFlush();
			glCullFace(GL_BACK);
			glNamedFramebufferTexture(zTestFragmentFBO, GL_DEPTH_ATTACHMENT, zChannelFragmentTex[ZOFF], 0);
			glClear(GL_DEPTH_BUFFER_BIT);
			glDrawArrays(GL_TRIANGLES, 0, m_nbVertexOccluder);//front
			glFlush();

			glViewport(0, 0, m_sizeZtestViewPort.x, m_sizeZtestViewPort.y);
		}
		glDisable(GL_CULL_FACE);
	}
	else if ((!cstate) && m_pstate) //occluder has just been turned off
	{
		GLfloat c = 0.0f;
		glClearNamedFramebufferfv(zTestFBO[ZOB], GL_DEPTH, 0, &c);
		if (m_fragmentTestOccluder)
		{
			glNamedFramebufferTexture(zTestFragmentFBO, GL_DEPTH_ATTACHMENT, zChannelFragmentTex[ZOFB], 0);
			glClearNamedFramebufferfv(zTestFragmentFBO, GL_DEPTH, 0, &c);
		}
	}
	m_pstate = cstate;
}

void scene::ProcessZMinMaxFiltering()
{
	if (FILTER_PASSES == 0)//
	{
		glNamedFramebufferTexture(zFilterFBO, GL_DEPTH_ATTACHMENT, zFilterBackTex[0], 0);
		glBlitNamedFramebuffer(gBufferFBO,zFilterFBO, 0, 0, m_sizeFBO.x, m_sizeFBO.y,0,0, m_sizeFBO.x, m_sizeFBO.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	}
	else
	{
		glDepthFunc(GL_ALWAYS);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, zFilterFBO);
		//processing back
		glBindTextureUnit(FILTERING_SLOT_TEXTURE, gBufferZTex);
		glm::ivec2 ts = m_sizeFBO;
		glUseProgram(m_zMaxFilterProgram);
		for (uint32_t i = 1; i <= FILTER_PASSES; i++)
		{
			glNamedFramebufferTexture(zFilterFBO, GL_DEPTH_ATTACHMENT, zFilterBackTex[i], 0);
			ts /= 2;
			glViewport(0, 0, ts.x, ts.y);
			renderQuad();
			glFlush();
			glBindTextureUnit(FILTERING_SLOT_TEXTURE, zFilterBackTex[i]);
		}
		glBindTextureUnit(GBUFFER_DEPTH_SLOT_TEXTURE, gBufferZTex);
	}

	glBindTextureUnit(16, zFilterBackTex[3]);
	glBindTextureUnit(15, zFilterBackTex[2]);
	glBindTextureUnit(14, zFilterBackTex[1]);
}

void scene::RenderZcullingBuffer()
{
	glDepthFunc(GL_GREATER);
	glClearDepth(0.0);
	glBindVertexArray(m_vaoDummy);
	glUseProgram(m_zMeshProgram);//render
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, zTestFBO[ZBC]);
	glClear(GL_DEPTH_BUFFER_BIT);

	glDrawArrays(GL_POINTS, 0, m_sizeSubsampledZBuffers.x*m_sizeSubsampledZBuffers.y);

	glFlush();

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, zTestFBO[ZB]);
	glClear(GL_DEPTH_BUFFER_BIT);
	glUseProgram(m_zWrapProgram);//wrapReprojection tesselation level is 64 by default (no TCS)
	glDrawArrays(GL_PATCHES, 0, ((m_sizeZtestViewPort.x-2)/64 +1)*((m_sizeZtestViewPort.y-2)/64 +1));//64=MaxTessLevel
	glFlush();
}



void scene::MergeToZTextures()
{
	glViewport(0, 0, m_sizeZtestViewPort.x, m_sizeZtestViewPort.y);
	glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, zCopyTestFBO);
	glUseProgram(m_zcopyTestProgram);
	renderQuad();
	glFlush();
	if (m_fragmentTestOccluder)
	{
		glViewport(0, 0, m_sizeFBO.x, m_sizeFBO.y);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, zCopyFragmentFBO);
		glUseProgram(m_zcopyFragmentProgram);
		renderQuad();
		glFlush();
	}
	glEnable(GL_DEPTH_TEST);
}



double scene::draw_scene()
{
	static bool PreviousSpectatorMode = false;
	bool spectatorTurnedOn = m_spectatorMode && !PreviousSpectatorMode;
	bool spectatorTurnedOff = !m_spectatorMode && PreviousSpectatorMode;
	PreviousSpectatorMode = m_spectatorMode;
	if (spectatorTurnedOn || spectatorTurnedOff)//we just changed spectator mode
	{
		reloadShaders();
	}

	std::chrono::time_point<std::chrono::high_resolution_clock> beginFrameTime = std::chrono::high_resolution_clock::now();

	decompressionCurrentShaders(m_optimisation);
	flushDynamicUBO();
	flushStaticUBO();

	PRINT_OPENGL_ERROR();
	if (m_spectatorMode == 0 && !(m_drawUncompressed && m_FillTck))
	{
		ProcessZMinMaxFiltering(); PRINT_OPENGL_ERROR();//subsampling Zbuffer
		glViewport(0, 0, m_sizeZtestViewPort.x, m_sizeZtestViewPort.y);
		ProcessZoccluder();	PRINT_OPENGL_ERROR();//filling ZOB and ZOF
		RenderZcullingBuffer();	PRINT_OPENGL_ERROR();//filling ZB
		MergeToZTextures();	PRINT_OPENGL_ERROR();//
	}
	//draw the brain

	glDepthFunc(GL_LESS);
	glClearDepth(1.0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gBufferFBO);
	glViewport(0, 0, m_sizeFBO.x, m_sizeFBO.y);
	static GLuint clearColor[] = { VOID_FIBER_ID,glm::packSnorm4x8(glm::vec4(0.0f,0.0f,0.0f,-1.0f)),0,0 };
	glClearNamedFramebufferuiv(gBufferFBO, GL_COLOR, 0, clearColor);//clear GL_COLOR_ATTACHMENT0

	glClear(GL_DEPTH_BUFFER_BIT);
	PRINT_OPENGL_ERROR();


	if (m_drawUncompressed && m_FillTck)
	{
		glUseProgram(m_UncompressedRenderingShader); PRINT_OPENGL_ERROR();
		drawUncompressed();
	}
	else if (m_tessellation)
	{
		//Decompression with tessellation
		glBindVertexArray(m_vaoDummy);
		glUseProgram(m_TessDecompressShader);
		uint64_t previousPosition = 0;
		for (unsigned numOfSSBO = 0; numOfSSBO < m_tracto.startPositionOfSSBOs.size(); numOfSSBO++)
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboCompressedFibers[numOfSSBO]); PRINT_OPENGL_ERROR();
			glDrawArrays(GL_PATCHES, 0, (m_tracto.startPositionOfSSBOs[numOfSSBO] - previousPosition)* m_sliderValue.x); PRINT_OPENGL_ERROR();
			previousPosition = m_tracto.startPositionOfSSBOs[numOfSSBO];
			glFlush();
		}
	}
	else if (m_geometry)
	{
		//Decompression with geometry shader
		glBindVertexArray(m_vaoDummy);
		glUseProgram(m_GeomDecompressShader);
		uint64_t previousPosition = 0;
		for (unsigned numOfSSBO = 0; numOfSSBO < m_tracto.startPositionOfSSBOs.size(); numOfSSBO++)
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboCompressedFibers[numOfSSBO]); PRINT_OPENGL_ERROR();
			glDrawArrays(GL_POINTS, 0, (m_tracto.startPositionOfSSBOs[numOfSSBO] - previousPosition)* m_sliderValue.x); PRINT_OPENGL_ERROR();
			previousPosition = m_tracto.startPositionOfSSBOs[numOfSSBO];
			glFlush();
		}
	}
	else if (m_mesh)
	{
		//Decompression with mesh shader
		glUseProgram(m_MeshDecompressShader);
		glBindVertexArray(m_vaoDummy);
		uint64_t previousPosition = 0;
		for (unsigned numOfSSBO = 0; numOfSSBO < m_tracto.startPositionOfSSBOs.size(); numOfSSBO++)
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboCompressedFibers[numOfSSBO]);
			glDrawMeshTasksNV(0, int(std::ceil(((m_tracto.startPositionOfSSBOs[numOfSSBO] - previousPosition)* m_sliderValue.x) / 32.0) + 0.5f));
			previousPosition = m_tracto.startPositionOfSSBOs[numOfSSBO];
			glFlush();
		}
	}
	PRINT_OPENGL_ERROR();//visible fibers ID should be visible in gbuffer

	BuildSelectedRenderingBuffers(m_clickSelected, m_mousePos, int(10.0f*m_sliderValue.w), m_forceSelectedReset);
	FlushCriterionSSBO();
	//SSAO+shading drawcall
	glNamedFramebufferTexture(tractoRenderFBOMS, GL_COLOR_ATTACHMENT0, tractorRenderColorTexMS, 0);
	glNamedFramebufferTexture(tractoRenderFBOMS, GL_DEPTH_ATTACHMENT, tractorRenderZTexMS, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tractoRenderFBOMS);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDepthFunc(GL_ALWAYS);
	glUseProgram(m_SSAOShadingProgram);
	renderQuad();
	glFlush();
	glDepthFunc(GL_LESS);

	PRINT_OPENGL_ERROR();
	if (m_drawMeshCut && m_meshCutLoaded && m_useMeshCut)//render occluder
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glUseProgram(m_basicProgram);
		glBindVertexArray(m_vaoOccluder);
		glDrawArrays(GL_TRIANGLES, 0, m_nbVertexOccluder);
		glFlush();
		glDisable(GL_CULL_FACE);
	}
	if (m_tractoTransparency != 0.0f)//render selected fiber
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glNamedFramebufferRenderbuffer(tractoRenderFBOMS, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, tractorRenderColorBufferMS);
		glNamedFramebufferRenderbuffer(tractoRenderFBOMS, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, tractorRenderZBufferMS);
		glNamedFramebufferTexture(copyRenderFBOMS, GL_COLOR_ATTACHMENT0, tractorRenderColorTexMS, 0);
		glBlitNamedFramebuffer(copyRenderFBOMS, tractoRenderFBOMS, 0, 0, m_sizeFBO.x, m_sizeFBO.y, 0, 0, m_sizeFBO.x, m_sizeFBO.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glClear(GL_DEPTH_BUFFER_BIT);//only Zbuffer as Color buffer will be used for transparency
		glUseProgram(m_selectedRenderingProgram);

		renderSelectedFibers();
		glFlush();
		glDisable(GL_CULL_FACE);
	}
	PRINT_OPENGL_ERROR();
	glViewport(0, 0, m_sizeFBO.x, m_sizeFBO.y);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	PRINT_OPENGL_ERROR();
	glFinish();
	glBlitNamedFramebuffer(tractoRenderFBOMS, 0, 0, 0, m_sizeFBO.x, m_sizeFBO.y, 0, 0, m_sizeFBO.x, m_sizeFBO.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	//Draw depth
	/*
	{
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		glDepthFunc(GL_ALWAYS);
		glUseProgram(m_viewTextureProgram);
		renderQuad();
	}*/
	PRINT_OPENGL_ERROR();
	glFlush();
	glFinish();

	std::chrono::time_point<std::chrono::high_resolution_clock> currentTime = std::chrono::high_resolution_clock::now();
	double frameTime = std::chrono::duration<double>(currentTime - beginFrameTime).count();

	return frameTime;
}

void scene::set_widget(WidgetOpenGL *widget_param)
{
	m_pwidget=widget_param;
}

void scene::saveTracto(const string filename)
{
	cout << "Saving file " << filename << " ..." << endl;
	saveFft(filename, m_tracto);
	cout << "Done" << endl;
}

bool scene::hasEnding(const string & path, const string & extension)
{
	if (path.length() < extension.length()) return false;
	return (path.compare(path.length() - extension.length(), extension.length(), extension) == 0);
}


void scene::drawUncompressed(void)
{
	if (m_primcountUncompressed.size() > 0)
	{
		for (unsigned i=0; i<m_uncompressedFIRST.size(); i++)
		{
			glBindVertexArray(m_vaoUncompressed[i]);
			glBindBuffer(GL_ARRAY_BUFFER, m_vboUncompressed[i]);
			glMultiDrawArrays(GL_LINE_STRIP, (const GLint*)(m_uncompressedFIRST[i].data()), (const GLint*)(m_uncompressedCOUNT[i].data()), GLuint64(m_sliderValue.x * m_primcountUncompressed[i]));
			glFlush();
		}
	}
}


void scene::loadTracto(const string filename,const bool fillUncompressedVBO)
{
	if (hasEnding(filename, "tck"))
	{
		compress(filename, m_tracto);
		cout << "Number of points per block: " << m_tracto.nbPtsPerBlock << endl;

		if (fillUncompressedVBO)
		{
			std::vector<std::vector<glm::vec4>> vTab;
			cout << "UNCOMPRESS TCK DATA COPIED" << endl;
			loadTckFibers(filename, vTab, m_uncompressedFIRST);
			cout << "UNCOMPRESS TCK DATA COPIED" << endl;
			unsigned nbOfVBOs = unsigned(m_uncompressedFIRST.size());
			m_primcountUncompressed.resize(nbOfVBOs);
			m_uncompressedCOUNT.resize(nbOfVBOs);
			for (unsigned i=0; i<nbOfVBOs; i++)
			{
				m_primcountUncompressed[i] = unsigned(m_uncompressedFIRST[i].size() - 1);
				m_uncompressedCOUNT[i].resize(m_primcountUncompressed[i]);
			}

			for (unsigned i = 0; i < unsigned(vTab.size()); i++)
			{
				for (unsigned j=0; j < unsigned(vTab[i].size()); j++)
				{
					vTab[i][j].x = (vTab[i][j].x + m_tracto.offset.x)  * 1.0f / m_tracto.scaling + 0.5f;
					vTab[i][j].y = (vTab[i][j].y + m_tracto.offset.y)  * 1.0f / m_tracto.scaling + 0.5f;
					vTab[i][j].z = (vTab[i][j].z + m_tracto.offset.z)  * 1.0f / m_tracto.scaling + 0.5f;
				}
			}
			for (unsigned i = 0; i < m_primcountUncompressed.size(); i++)
			{
				for (unsigned j=0; j < m_primcountUncompressed[i]; j++)
				{
					m_uncompressedCOUNT[i][j] = m_uncompressedFIRST[i][j + 1] - m_uncompressedFIRST[i][j];
				}
			}

			m_vaoUncompressed.resize(nbOfVBOs);
			glGenVertexArrays(nbOfVBOs, m_vaoUncompressed.data());PRINT_OPENGL_ERROR();
			m_vboUncompressed.resize(nbOfVBOs);
			glCreateBuffers(nbOfVBOs, m_vboUncompressed.data());PRINT_OPENGL_ERROR();
			for (unsigned i=0; i<nbOfVBOs; i++)
			{
				glBindVertexArray(m_vaoUncompressed[i]);PRINT_OPENGL_ERROR();
				glBindBuffer(GL_ARRAY_BUFFER, m_vboUncompressed[i]);PRINT_OPENGL_ERROR();
				glBufferData(GL_ARRAY_BUFFER, vTab[i].size() * 4*sizeof(float), vTab[i].data(), GL_STATIC_DRAW);
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*)0); PRINT_OPENGL_ERROR();
			}
		}

		reloadShaders();
	}
	else if (hasEnding(filename, "fft"))
	{
		if (!loadFft(filename, m_tracto))
		{
			cerr << "Unable to read file " << filename << endl;
			exit(EXIT_FAILURE);
		}
		cout << "Number of points per block: " << m_tracto.nbPtsPerBlock << endl;
		reloadShaders();
	}
	else
	{
		cerr << "Error. Supported formats are tck and fft" << endl;
		exit(EXIT_FAILURE);
	}



	SetGuardBand(m_tracto.stepsize, m_tracto.nbPtsPerBlock);
}

void scene::setNbPtsPerBlock(unsigned val)
{
	m_tracto.nbPtsPerBlock = val;
	m_tracto.computePackSize();
}

void scene::DeleteSelectedBuffers(void)
{
	glDeleteBuffers(1, &ssboCriterion);
	for (int i = 0; i < int(m_tracto.startPositionOfSSBOs.size()); i++)
		glDeleteBuffers(1, &m_iboSelectedFibers[i]);
	for (int i = 0; i< int(ssboCompressedFibers.size()); i++)
		glDeleteBuffers(1, &ssboCompressedFibers[i]);
	ssboCompressedFibers.clear();
	m_tracto.fiberBlockLut.clear();
	m_selectedFibersToRender.clear();
	delete[] m_iboSelectedFibers;
	delete[] m_iboSelectedElementNumber;
}

void scene::CreateSelectedIBO(void)
{
	uint32_t SSBOdim = uint32_t(m_tracto.startPositionOfSSBOs.size());
	m_iboSelectedFibers = new GLuint[SSBOdim];
	m_iboSelectedElementNumber = new GLuint[SSBOdim];
	for (int i = 0; i < int(SSBOdim); i++)//copy ibo content to GPU
	{
		glCreateBuffers(1, &m_iboSelectedFibers[i]);
		m_iboSelectedElementNumber[i] = 0;
	}
}

void scene::ClearSelection(void)
{
	cout << "Selection cleared" << endl;
	m_tractoTransparency = 0.0f;
	for (uint32_t i = 0; i < m_tracto.startPositionOfSSBOs.size(); i++)
		m_iboSelectedElementNumber[i] = 0;
	m_selectedFibersToRender.clear();
}

void scene::BuildSelectedRenderingBuffers(bool clicked, glm::ivec2 mouseCoord, int selectSize, bool forceReset)
{
	if (forceReset)
	{
		m_forceSelectedReset = false;
		ClearSelection();
	}
	if (clicked)
	{
		m_clickSelected = false;

		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBufferFBO);//clear color (RED) should be VOID_FIBER_ID !!
		uint s = 2 * selectSize + 1;
		GLuint* dataPick = new GLuint[s*s];
		for (unsigned i = 0; i < s*s; i++)
			dataPick[i] = VOID_FIBER_ID;

		PRINT_OPENGL_ERROR();
		glReadPixels(glm::max(mouseCoord.x - selectSize,0), glm::max(mouseCoord.y - selectSize,0)
		, glm::min(selectSize,m_sizeScreen.x-mouseCoord.x)+1+selectSize, glm::min(selectSize,m_sizeScreen.y-mouseCoord.y)+1+selectSize, GL_RED_INTEGER, GL_UNSIGNED_INT,(void*)dataPick);

		PRINT_OPENGL_ERROR();

		vector<GLuint> fibers;
		bool reset = true;
		for (unsigned i = 0; i < s*s; i++)
		{
			if (dataPick[i] != VOID_FIBER_ID)
			{
				reset = false;
				fibers.push_back(dataPick[i]);
			}
		}
		delete[]dataPick;

		if (reset)
			ClearSelection();
		else// building IBO rendering command
		{
			cout << "Selection updated" << endl;
			m_tractoTransparency = TRACTO_TRANSPARENCY_VALUE;
			m_selectedFibersToRender.insert(fibers.begin(), fibers.end());
			uint32_t SSBOdim = uint32_t(m_tracto.startPositionOfSSBOs.size());

			vector<GLuint> *iboBuffer;
			iboBuffer = new vector<GLuint>[SSBOdim];

			for (auto f : m_selectedFibersToRender)
			{
				uint32_t blockID = m_tracto.fiberBlockLut[f].y;
				uint32_t flength = m_tracto.fiberBlockLut[f].x % 65536;//2^16
				uint32_t ssboId = m_tracto.fiberBlockLut[f].x >> 16;

				for (unsigned i = blockID; i < blockID + flength; i++)
					iboBuffer[ssboId].push_back(i);
			}

			for (uint32_t i = 0; i < SSBOdim; i++)//copy ibo content to GPU
			{
				m_iboSelectedElementNumber[i] = GLuint(iboBuffer[i].size());
				if (m_iboSelectedElementNumber[i] != 0)
				{
					GLuint* data = new GLuint[iboBuffer[i].size()];//for the data to be packed
					for (unsigned j = 0; j < iboBuffer[i].size(); j++)
						data[j] = iboBuffer[i][j];
					glNamedBufferData(m_iboSelectedFibers[i], iboBuffer[i].size() * sizeof(GLuint), (const void*)(data),  GL_STATIC_DRAW);
					delete[] data;
				}
			}
			for (unsigned i = 0; i < SSBOdim; i++)
				iboBuffer[i].clear();
			delete[]iboBuffer;
		}
		fibers.clear();
	}
}

void scene::renderSelectedFibers()
{

	glBindVertexArray(m_vaoDummy);
	for (int i = 0; i < int(m_tracto.startPositionOfSSBOs.size()); i++)//copy ibo content to GPU
	{
		if (m_iboSelectedElementNumber[i] != 0)// render if there are fibers within SSBO
		{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboCompressedFibers[i]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iboSelectedFibers[i]);
		glDrawElements(GL_PATCHES, m_iboSelectedElementNumber[i], GL_UNSIGNED_INT, 0);
		}
	}

}

bool scene::close_scene()
{
	DeleteSelectedBuffers();
	return true;
}

void editShaders(string & shader, unsigned nbPtsPerBlock,  bool aggressiveOptim, bool useMeshCut,bool cullOutsideMesh, bool fragmentTestOccluder, int fragmentsPerLine,int specMode)
{
	string oldName = shader;
	size_t pos = shader.find_last_of("/");
	shader = shader.substr(0, pos+1) + "GENERATED_"+ shader.substr(pos+1, shader.length());
	fstream oldFile, newFile;
	oldFile.open(oldName, ios_base::in);
	newFile.open(shader, ios_base::out | ios_base::trunc);
	newFile << "#version 460\n#define NB_PTS_PER_BLOCK " << nbPtsPerBlock
			<< "\n#define AGGRESSIVE_OPTIM " << aggressiveOptim
			<< "\n#define MESH_CUT_TEST " << useMeshCut
			<< "\n#define OCCLUDER_CULL_OUTSIDE " << cullOutsideMesh
			<< "\n#define OCCLUDER_FRAGMENT_TEST " << fragmentTestOccluder
			<< "\n#define RENDER_SINGLE_SEGMENT_RATIO 0." << fragmentsPerLine
			<< "\n#define SPECTATOR_MODE " << specMode
			<< "\n#define C vec3(-0.703023156622, 0.039959276793, 0.71004344758)"
			<< "\n\n";

	while (!oldFile.eof())
	{
		string line;
		getline(oldFile, line);
		newFile << line << "\n";
	}

	oldFile.close();
	newFile.close();
}

void scene::reloadShaders()
{
	bool isCompiled = true;
	ShaderInfo  s0a[] =
	{
		{ GL_VERTEX_SHADER,PATHINC(opengl/shaders/basic.vert), 0 },
		{ GL_FRAGMENT_SHADER,PATHINC(opengl/shaders/basic.frag), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_dummyShader);
	m_dummyShader = loadShaders(s0a);
	if (m_dummyShader == 0)
	{
		cerr << "Error due to m_dummyShader" << endl;
		isCompiled = false;
	}

	ShaderInfo  s0b[] =
	{
		{ GL_VERTEX_SHADER,PATHINC(opengl/shaders/selectedRendering/renderUncompressed.vert), 0 },
		{ GL_FRAGMENT_SHADER,PATHINC(opengl/shaders/selectedRendering/renderUncompressed.frag), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_UncompressedRenderingShader);
	m_UncompressedRenderingShader = loadShaders(s0b);
	if (m_UncompressedRenderingShader == 0)
	{
		cerr << "Error due to m_UncompressedRenderingShader" << endl;
		isCompiled = false;
	}

	//////////////////////////////////////////////////////////////////////
	string teval = PATHINC(opengl/shaders/decompression/tesselationeval.tese);
	string toptimctrl = PATHINC(opengl/shaders/decompression/tesselationctrl_optims.tesc);
	string tnooptimctrl = PATHINC(opengl/shaders/decompression/tesselationctrl_no_optims.tesc);
	string decompfragment = PATHINC(opengl/shaders/decompression/decompression.frag);
	editShaders(teval, m_tracto.nbPtsPerBlock, m_aggressiveOptim,m_useMeshCut,m_cullOutsideMesh,m_fragmentTestOccluder,RENDER_SINGLE_SEGMENT_RATIO,m_spectatorMode);
	editShaders(toptimctrl, m_tracto.nbPtsPerBlock, m_aggressiveOptim,m_useMeshCut,m_cullOutsideMesh,m_fragmentTestOccluder,RENDER_SINGLE_SEGMENT_RATIO, m_spectatorMode);
	editShaders(tnooptimctrl, m_tracto.nbPtsPerBlock, m_aggressiveOptim,m_useMeshCut,m_cullOutsideMesh,m_fragmentTestOccluder,RENDER_SINGLE_SEGMENT_RATIO, m_spectatorMode);
	editShaders(decompfragment, m_tracto.nbPtsPerBlock, m_aggressiveOptim,m_useMeshCut,m_cullOutsideMesh,m_fragmentTestOccluder,RENDER_SINGLE_SEGMENT_RATIO, m_spectatorMode);

	ShaderInfo  s0[] =
	{
		{ GL_VERTEX_SHADER,  PATHINC(opengl/shaders/decompression/decompression.vert), 0 },
		{ GL_TESS_CONTROL_SHADER, toptimctrl.c_str(), 0 },
		{ GL_TESS_EVALUATION_SHADER, teval.c_str(), 0 },
		{ GL_FRAGMENT_SHADER,  decompfragment.c_str(), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_TessOptimsProgram);
	m_TessOptimsProgram = loadShaders(s0);
	if (m_TessOptimsProgram == 0)
	{
		cerr << "Error due to m_TessOptimsProgram" << endl;
		isCompiled = false;
	}
	ShaderInfo  s1[] =
	{
		{ GL_VERTEX_SHADER,  PATHINC(opengl/shaders/decompression/decompression.vert), 0 },
		{ GL_TESS_CONTROL_SHADER, tnooptimctrl.c_str(), 0 },
		{ GL_TESS_EVALUATION_SHADER, teval.c_str(), 0 },
		{ GL_FRAGMENT_SHADER,decompfragment.c_str(), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_TessNoOptimsProgram);
	m_TessNoOptimsProgram = loadShaders(s1);
	if (m_TessNoOptimsProgram == 0)
	{
		cerr << "Error due to m_TessNoOptimsProgram" << endl;
		isCompiled = false;
	}
/////////////////////////////////////////////////////////////////////////
	string goptim = PATHINC(opengl/shaders/decompression/geometry_optims.geom);
	string gnooptim = PATHINC(opengl/shaders/decompression/geometry_no_optims.geom);
	editShaders(goptim, m_tracto.nbPtsPerBlock, m_aggressiveOptim,m_useMeshCut,m_cullOutsideMesh,m_fragmentTestOccluder,RENDER_SINGLE_SEGMENT_RATIO, m_spectatorMode);
	editShaders(gnooptim, m_tracto.nbPtsPerBlock, m_aggressiveOptim,m_useMeshCut,m_cullOutsideMesh,m_fragmentTestOccluder,RENDER_SINGLE_SEGMENT_RATIO, m_spectatorMode);
	ShaderInfo  s2[] =
	{
		{ GL_VERTEX_SHADER, PATHINC(opengl/shaders/decompression/decompression.vert), 0 },
		{ GL_GEOMETRY_SHADER, goptim.c_str(), 0 },
		{ GL_FRAGMENT_SHADER, decompfragment.c_str(), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_GeomOptimsProgram);
	m_GeomOptimsProgram = loadShaders(s2);
	if (m_GeomOptimsProgram == 0)
	{
		cerr << "Error due to m_GeomOptimsProgram" << endl;
		isCompiled = false;
	}
	ShaderInfo  s3[] =
	{
		{ GL_VERTEX_SHADER, PATHINC(opengl/shaders/decompression/decompression.vert), 0 },
		{ GL_GEOMETRY_SHADER, gnooptim.c_str(), 0 },
		{ GL_FRAGMENT_SHADER, decompfragment.c_str(), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_GeomNoOptimsProgram);
	m_GeomNoOptimsProgram = loadShaders(s3);
	if (m_GeomNoOptimsProgram == 0)
	{
		cerr << "Error due to m_GeomNoOptimsProgram" << endl;
		isCompiled = false;
	}
///////////////////////////////////////////////////////////////////////////////////////
	string tsoptim = PATHINC(opengl/shaders/decompression/task_optims.task);
	string tsnooptim = PATHINC(opengl/shaders/decompression/task_no_optims.task);
	string msoptim = PATHINC(opengl/shaders/decompression/mesh_optims.mesh);
	string msnooptim = PATHINC(opengl/shaders/decompression/mesh_no_optims.mesh);
	string msfragment = PATHINC(opengl/shaders/decompression/decompression_meshshader.frag);
	editShaders(tsoptim, m_tracto.nbPtsPerBlock, m_aggressiveOptim,m_useMeshCut,m_cullOutsideMesh,m_fragmentTestOccluder,RENDER_SINGLE_SEGMENT_RATIO, m_spectatorMode);
	editShaders(tsnooptim, m_tracto.nbPtsPerBlock, m_aggressiveOptim,m_useMeshCut,m_cullOutsideMesh,m_fragmentTestOccluder,RENDER_SINGLE_SEGMENT_RATIO, m_spectatorMode);
	editShaders(msoptim, m_tracto.nbPtsPerBlock, m_aggressiveOptim,m_useMeshCut,m_cullOutsideMesh,m_fragmentTestOccluder,RENDER_SINGLE_SEGMENT_RATIO, m_spectatorMode);
	editShaders(msnooptim, m_tracto.nbPtsPerBlock, m_aggressiveOptim,m_useMeshCut,m_cullOutsideMesh,m_fragmentTestOccluder,RENDER_SINGLE_SEGMENT_RATIO, m_spectatorMode);
	editShaders(msfragment, m_tracto.nbPtsPerBlock, m_aggressiveOptim,m_useMeshCut,m_cullOutsideMesh,m_fragmentTestOccluder,RENDER_SINGLE_SEGMENT_RATIO, m_spectatorMode);
	ShaderInfo  s4[] =
	{
		{ GL_TASK_SHADER_NV, tsoptim.c_str(), 0 },
		{ GL_MESH_SHADER_NV, msoptim.c_str(), 0 },
		{ GL_FRAGMENT_SHADER, msfragment.c_str(), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_MeshOptimsProgram);
	m_MeshOptimsProgram = loadShaders(s4);
	if (m_MeshOptimsProgram == 0)
	{
		cerr << "Error due to m_MeshOptimsProgram" << endl;
		//isCompiled = false;
	}
	ShaderInfo  s5[] =
	{
		{ GL_TASK_SHADER_NV, tsnooptim.c_str(), 0 },
		{ GL_MESH_SHADER_NV, msnooptim.c_str(), 0 },
		{ GL_FRAGMENT_SHADER, msfragment.c_str(), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_MeshNoOptimsProgram);
	m_MeshNoOptimsProgram = loadShaders(s5);
	if (m_MeshNoOptimsProgram == 0)
	{
		cerr << "Error due to m_MeshNoOptimsProgram" << endl;
		//isCompiled = false;
	}
////////////////////////////////////////////////////////
	ShaderInfo s6[] =
	{
		{ GL_VERTEX_SHADER, PATHINC(opengl/shaders/postprocess/SSAOShading.vert), 0 },
		{ GL_FRAGMENT_SHADER, PATHINC(opengl/shaders/postprocess/SSAOShading.frag), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_SSAOShadingProgram);
	m_SSAOShadingProgram = loadShaders(s6);
	if (m_SSAOShadingProgram == 0)
	{
		cerr << "Error due to m_SSAOProgram" << endl;
		isCompiled = false;
	}

///////////////////////////////////////////////////////
	ShaderInfo s7[] =
	{
		{ GL_VERTEX_SHADER, PATHINC(opengl/shaders/zfiltering/zMaxFilter.vert), 0 },
		{ GL_FRAGMENT_SHADER, PATHINC(opengl/shaders/zfiltering/zMaxFilter.frag), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_zMaxFilterProgram);
	m_zMaxFilterProgram = loadShaders(s7);
	if (m_zMaxFilterProgram == 0)
	{
		cerr << "Error due to m_zMaxFilterProgram" << endl;
		isCompiled = false;
	}
	ShaderInfo s8[] =
	{
		{ GL_VERTEX_SHADER, PATHINC(opengl/shaders/zfiltering/zMeshing.vert), 0 },
		{ GL_GEOMETRY_SHADER, PATHINC(opengl/shaders/zfiltering/zMeshing.geom), 0 },
		{ GL_FRAGMENT_SHADER, PATHINC(opengl/shaders/zfiltering/zMeshing.frag), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_zMeshProgram);
	m_zMeshProgram = loadShaders(s8);
	if (m_zMeshProgram == 0)
	{
		cerr << "Error due to m_zMeshProgram" << endl;
		isCompiled = false;
	}
	ShaderInfo s8b[] =
	{
		{ GL_VERTEX_SHADER, PATHINC(opengl/shaders/zfiltering/zCopy.vert), 0 },
		{ GL_FRAGMENT_SHADER, PATHINC(opengl/shaders/zfiltering/zCopyTest.frag), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_zcopyTestProgram);
	m_zcopyTestProgram = loadShaders(s8b);
	if (m_zcopyTestProgram == 0)
	{
		cerr << "Error due to m_zcopyProgram" << endl;
		isCompiled = false;
	}

	ShaderInfo s8c[] =
	{
		{ GL_VERTEX_SHADER, PATHINC(opengl/shaders/zfiltering/zWrap.vert), 0 },
		{ GL_TESS_EVALUATION_SHADER, PATHINC(opengl/shaders/zfiltering/zWrap.tese), 0 },
		{ GL_FRAGMENT_SHADER, PATHINC(opengl/shaders/zfiltering/zWrap.frag), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_zWrapProgram);
	m_zWrapProgram = loadShaders(s8c);
	if (m_zWrapProgram == 0)
	{
		cerr << "Error due to m_zWrapProgram" << endl;
		isCompiled = false;
	}

	ShaderInfo s8d[] =
	{
		{ GL_VERTEX_SHADER, PATHINC(opengl/shaders/zfiltering/zCopy.vert), 0 },
		{ GL_FRAGMENT_SHADER, PATHINC(opengl/shaders/zfiltering/zCopyFragment.frag), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_zcopyFragmentProgram);
	m_zcopyFragmentProgram = loadShaders(s8d);
	if (m_zcopyFragmentProgram == 0)
	{
		cerr << "Error due to m_zWrapProgram" << endl;
		isCompiled = false;
	}

//////////////////////////////////////////////////////
	string sOccluderGeometry = PATHINC(opengl/shaders/meshcut/backOccluder.geom);
	editShaders(sOccluderGeometry, m_tracto.nbPtsPerBlock, m_aggressiveOptim,m_useMeshCut,m_cullOutsideMesh,m_fragmentTestOccluder,RENDER_SINGLE_SEGMENT_RATIO, m_spectatorMode);
	ShaderInfo s9[] =
	{
		{ GL_VERTEX_SHADER, PATHINC(opengl/shaders/meshcut/backOccluder.vert), 0 },
		{ GL_GEOMETRY_SHADER, sOccluderGeometry.c_str(), 0 },
		{ GL_FRAGMENT_SHADER, PATHINC(opengl/shaders/meshcut/backOccluder.frag), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_BackOccluderProgram);
	m_BackOccluderProgram = loadShaders(s9);
	if (m_BackOccluderProgram == 0)
	{
		cerr << "Error due to m_BackOccluderProgram" << endl;
		isCompiled = false;
	}

	ShaderInfo s10b[] =
	{
		{ GL_VERTEX_SHADER,PATHINC(opengl/shaders/meshcut/backOccluderFragment.vert), 0 },
		{ GL_FRAGMENT_SHADER,PATHINC(opengl/shaders/meshcut/backOccluder.frag), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_BackFragmentOccluderProgram);
	m_BackFragmentOccluderProgram = loadShaders(s10b);
	if (m_BackFragmentOccluderProgram == 0)
	{
		cerr << "Error due to m_BackFragmentOccluderProgram" << endl;
		isCompiled = false;
	}
///////////////////////////////////////////////////
	string rendertcs = PATHINC(opengl/shaders/selectedRendering/render3d.tesc);
	string rendertes = PATHINC(opengl/shaders/selectedRendering/render3d.tese);
	editShaders(rendertcs,  m_tracto.nbPtsPerBlock, m_aggressiveOptim,m_useMeshCut,m_cullOutsideMesh,m_fragmentTestOccluder,RENDER_SINGLE_SEGMENT_RATIO, m_spectatorMode);
	editShaders(rendertes,  m_tracto.nbPtsPerBlock, m_aggressiveOptim,m_useMeshCut,m_cullOutsideMesh,m_fragmentTestOccluder,RENDER_SINGLE_SEGMENT_RATIO, m_spectatorMode);
	ShaderInfo  s11[] =
	{
		{ GL_VERTEX_SHADER,  PATHINC(opengl/shaders/selectedRendering/render3d.vert), 0 },
		{ GL_TESS_CONTROL_SHADER,rendertcs.c_str() , 0 },
		{ GL_TESS_EVALUATION_SHADER, rendertes.c_str(), 0 },
		{ GL_FRAGMENT_SHADER,  PATHINC(opengl/shaders/selectedRendering/render3d.frag), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_selectedRenderingProgram);
	m_selectedRenderingProgram = loadShaders(s11);
	if (m_selectedRenderingProgram == 0)
	{
		cerr << "Error due to m_selectedRenderingProgram" << endl;
		isCompiled = false;
	}


	//////////////////////////////////////////////////////////////
	ShaderInfo s12[] =
	{
		{ GL_VERTEX_SHADER,PATHINC(opengl/shaders/basic.vert), 0 },
		{ GL_FRAGMENT_SHADER,PATHINC(opengl/shaders/basic.frag), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_basicProgram);
	m_basicProgram = loadShaders(s12);
	if (m_basicProgram == 0)
	{
		cerr << "Error due to m_basicProgram" << endl;
		isCompiled = false;
	}
	ShaderInfo s13[] =
	{
		{ GL_VERTEX_SHADER,PATHINC(opengl/shaders/viewTexture.vert), 0 },
		{ GL_FRAGMENT_SHADER,PATHINC(opengl/shaders/viewTexture.frag), 0 },
		{ GL_NONE, NULL, 0 }
	};
	glDeleteProgram(m_viewTextureProgram);
	m_viewTextureProgram = loadShaders(s13);
	if (m_viewTextureProgram == 0)
	{
		cerr << "Error due to m_viewTextureProgram" << endl;
		isCompiled = false;
	}


	printf("\n Shaders compiled\n");
	m_pwidget->setInitialized(isCompiled);
}



void scene::decompressionCurrentShaders(bool optimisation)
{
	if (optimisation)
	{
		m_TessDecompressShader = m_TessOptimsProgram;
		m_GeomDecompressShader = m_GeomOptimsProgram;
		m_MeshDecompressShader = m_MeshOptimsProgram;
	}
	else
	{
		m_TessDecompressShader = m_TessNoOptimsProgram;
		m_GeomDecompressShader = m_GeomNoOptimsProgram;
		m_MeshDecompressShader = m_MeshNoOptimsProgram;
	}
}

void scene::setOptimisation(bool optim, bool aggressive)
{
	m_optimisation = optim;
	if (m_aggressiveOptim != aggressive)
	{
		m_aggressiveOptim = aggressive;
		reloadShaders();
	}
}
