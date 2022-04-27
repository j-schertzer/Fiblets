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