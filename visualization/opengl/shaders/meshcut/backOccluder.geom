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

layout (triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 nv[];
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
	vec3 p0 = gl_in[0].gl_Position.xyz;
	vec3 p1 = gl_in[1].gl_Position.xyz;
	vec3 p2 = gl_in[2].gl_Position.xyz;

	#if (OCCLUDER_FRAGMENT_TEST == 1)

		#if (OCCLUDER_CULL_OUTSIDE == 1 )
			float d=-data1.x*1.6;//secure threshold
		#else
			float d=data1.x*1.6;//secure threshold
		#endif
	#else
		#if (OCCLUDER_CULL_OUTSIDE == 1 )
			float d=data1.x*0.8;//secure threshold
		#else
			float d=-data1.x*0.8;//secure threshold
		#endif
	#endif


	p0-= nv[0]*d;
	p1-= nv[1]*d;
	p2-= nv[2]*d;


	gl_Position = projCam*vec4(p0,1.0);
	EmitVertex();
	gl_Position = projCam*vec4(p1,1.0);
	EmitVertex();
	gl_Position = projCam*vec4(p2,1.0);
	EmitVertex();
	EndPrimitive();
}
