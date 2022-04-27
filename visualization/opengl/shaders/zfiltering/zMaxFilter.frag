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