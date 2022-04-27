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

#version 460

#define TEX_CHANNEL 5

out vec4 colorBuffer;
in vec2 TexCoords;
layout (binding = TEX_CHANNEL) uniform sampler2D gtestzTex;//rgb

layout(binding = 1, shared) uniform UBO_STATIC
{
    vec4 data0;//xy=m_sizeZtestViewPort.xy, zw=2.0/(m_sizeFBO.xy-1.0)
    vec4 data1;//x = guardBand, y = angularCorrection, z = ratio, w = scaling
    vec4 data2;//x = unused, y = tesselationGain, z = colormode & backgroundflag, w = m_sizeSubsampledZBuffers.x
    vec4 data3;//xyz = frustrum test constants, w = stepsize
    vec4 sliders;//x = percent of fiber drawn, y = selected fiber Radius size, z = alpha selected fiber, w = size of selection
};

layout(binding = 2, shared) uniform UBO_DYNAMIC
{
    mat4 projCam;
    mat4 ZFilterinvCamProjMultTexShift;
    mat4 ProjMultCurrentPoseMultInvPreviousProjPoseMultTexShift;
    mat4 projCamMultMV;
    mat4 ModelViewCam;
    mat4 ModelViewOccluder;

    mat4 projSpec;
    mat4 invProjSpec;
    mat4 ModelViewSpec;
    mat4 projSpecMultMV;

    mat4 lights;//4xvec4(xLight,yLigth,zLight,1.0) in specCamSpace
};


void main()
{
	const float zn = 82.0;
	const float zf = 180.0;	
	float d = texture(gtestzTex, TexCoords, 0).g;
	float wd = 1.0/(invProjSpec[2][3]*(2.0*d-1.0)+invProjSpec[3][3]);
	float sd = clamp((wd-zn)/(zf-zn),0.0,1.0);

    colorBuffer = vec4(sd.xxx,1.0);
}


