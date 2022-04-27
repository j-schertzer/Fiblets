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
layout (quads, equal_spacing/*fractional_even_spacing*/, cw) in;
layout (binding = 8+3) uniform sampler2D zBuffer;
in int index[];
int GID = index[0];

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


void main(void)
{

	int sx = int(data0.x+0.2-2.0)/64 +1;
	ivec2 pix = ivec2(round(gl_TessCoord.xy*64.0));
	ivec2 tile;
	tile.x = (GID%sx);
	tile.y = (GID/sx);
	pix += 64*tile;


	float z = texelFetch(zBuffer,pix,0).x;
	vec2 fpix = vec2(pix);
	vec2 b = sign(fpix-(data0.xy-1.5))+sign(fpix-0.5);
	gl_Position = ProjMultCurrentPoseMultInvPreviousProjPoseMultTexShift*vec4((fpix+b*64.0)+0.5,z,1.0);
	gl_Position.z = min(gl_Position.w,gl_Position.z);

}
