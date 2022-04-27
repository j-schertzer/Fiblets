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
layout (binding = 7) uniform sampler2D DepthTex;//r = depth
in vec4 gl_FragCoord;
void main()
{
	ivec2 ic = ivec2(gl_FragCoord.xy)*2;
	float d[4];
	d[0] = texelFetch(DepthTex,ic,0).x;
	d[1] = texelFetch(DepthTex,ic+ivec2(0,1),0).x;
	d[2] = texelFetch(DepthTex,ic+ivec2(1,0),0).x;
	d[3] = texelFetch(DepthTex,ic+ivec2(1,1),0).x;
/*
	d[0] -= step(1.0,d[0]);//because we don't want background to "absorb" filter
	d[1] -= step(1.0,d[1]);
	d[2] -= step(1.0,d[2]);
	d[3] -= step(1.0,d[3]);*/
gl_FragDepth = max(max(d[0],d[1]),max(d[2],d[3]));
//gl_FragDepth = 0.5;
}