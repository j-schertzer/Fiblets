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
layout (binding = 12) uniform sampler2D ZOFB;//r = depth
layout (binding = 13) uniform sampler2D ZOFF;//r = depth
in vec4 gl_FragCoord;
layout (location = 0) out vec2 FragZtest;
void main()
{
	ivec2 ic = ivec2(gl_FragCoord.xy);
	FragZtest.x = texelFetch(ZOFB,ic,0).x;	
	float zoff = texelFetch(ZOFF,ic,0).x;	
	FragZtest.y = zoff*step(zoff,FragZtest.x);	
}