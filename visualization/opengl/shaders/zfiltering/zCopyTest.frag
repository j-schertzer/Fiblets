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
layout (binding = 8) uniform sampler2D ZB;//r = depth
layout (binding = 9) uniform sampler2D ZOB;//r = depth
layout (binding = 10) uniform sampler2D ZOF;//r = depth
in vec4 gl_FragCoord;
layout (location = 0) out vec3 FiberZtest;
void main()
{
	ivec2 ic = ivec2(gl_FragCoord.xy);
	FiberZtest.x = texelFetch(ZB,ic,0).x;
	FiberZtest.y = texelFetch(ZOB,ic,0).x;
	float zof = texelFetch(ZOF,ic,0).x;	
	FiberZtest.z = zof*step(zof,FiberZtest.y);
}