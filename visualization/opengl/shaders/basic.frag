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
in vec3 n;
out vec4 Color;
void main()
{
vec3 c = abs(cos(n*n.yzx*20.0 + n.zxy*n.yzx*10.0)*0.6)*0.4+0.3;

Color = vec4(vec3(dot(abs(n), vec3(1.0))/3.0*0.8+0.1), 0.0);
}