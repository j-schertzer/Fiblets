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