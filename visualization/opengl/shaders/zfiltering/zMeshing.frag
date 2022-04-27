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
out float gl_FragDepth;

//layout (binding = 2) uniform sampler2D DepthTex;//r = depth
in vec4 gl_FragCoord;
in vec3 posFrag;//last coord is scale
in vec4 circlePos;//
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

float x2 = dot(circlePos.xy,circlePos.xy);
float r2 = circlePos.z*circlePos.z;
if (x2>r2)
discard;
else
{
float d2 = circlePos.w*circlePos.w;
float xpd2 = x2+d2;
float l = posFrag.z + (sqrt(r2*xpd2-d2*x2)-x2)/sqrt(xpd2) * dot(normalize(posFrag),vec3(0.0,0.0,1.0));
gl_FragDepth = (-projCam[2][2]-projCam[3][2]/l)*0.5+0.5;
}
}