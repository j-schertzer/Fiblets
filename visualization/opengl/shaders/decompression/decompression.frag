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

#if (OCCLUDER_FRAGMENT_TEST == 0 || MESH_CUT_TEST==0)
	layout(early_fragment_tests) in;//cannot work if fragment are discarded
#endif

layout (location = 0) out uvec2 gBuffer;// -> x = fiber id, y = packtan
layout (binding = 5) uniform sampler2D fragOccluderTex;//rg
out float gl_FragDepth;// ->gBufferDepthTex

flat in uint fiberID;//no interpolation
in vec4 tangent;//all coordinate already in [0,1]
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

void main(void)
{

	#if (OCCLUDER_FRAGMENT_TEST == 1 && MESH_CUT_TEST==1)
	vec2 zoccluder  = texelFetch(fragOccluderTex,ivec2(gl_FragCoord.xy),0).xy;
	// zoccluder.x=ZOFB    zoccluder.y=ZOFF
		#if (OCCLUDER_CULL_OUTSIDE == 1)
		    bool t = gl_FragCoord.z>zoccluder.x || gl_FragCoord.z<zoccluder.y;
		#else
	    	bool t = gl_FragCoord.z<zoccluder.x && gl_FragCoord.z>zoccluder.y;
	 	#endif
 	gl_FragDepth = gl_FragCoord.z + float(t);
   	#endif

	gBuffer.x = fiberID;
	gBuffer.y = packSnorm4x8(vec4(normalize(tangent.xyz),tangent.w));
}
