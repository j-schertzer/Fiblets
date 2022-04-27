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

#version 460
//layout(early_fragment_tests) in;
out vec3 color;// -> gBufferColorTex
layout (binding = 3) uniform sampler2DMS gBuffer;//multisampled
layout (binding = 4) uniform sampler2DMS zBuffer;//multisampled
out float gl_FragDepth;// ->gBufferDepthTex

in vec3 pos;
in vec3 albedo;
in vec3 normal;
in vec3 tangeant;
in vec4 gl_FragCoord;

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

mat2 zProjInv = inverse(mat2(projCam[2].zw,projCam[3].zw));








void main(void)
{
	float zb = -1.0/(invProjSpec[2][3]*(texelFetch(zBuffer,ivec2(gl_FragCoord.xy),0).r*2.0-1.0)+invProjSpec[3][3]);
	float zs = -1.0/(invProjSpec[2][3]*(gl_FragCoord.z*2.0-1.0)+invProjSpec[3][3]);
	gl_FragDepth = gl_FragCoord.z;

	vec3 brainColor = texelFetch(gBuffer,ivec2(gl_FragCoord.xy),0).rgb;


	vec3 c2p = normalize(-pos);
	float relDepth = (-zs+zb)/(data1.w*1.2);
	float ztest =step(zs,zb)*clamp(0.2 + relDepth*2.0,0.0,min(0.6,sliders.z));//transparency augments with depth inside brain

	vec3 n = normalize(normal);
	vec3 t = normalize(tangeant);
	vec3 colorFS = albedo * (0.1+1.2*dot(c2p,n));

	color = mix(mix(colorFS,brainColor,step(zs,zb)*sliders.z),brainColor,ztest);

}
