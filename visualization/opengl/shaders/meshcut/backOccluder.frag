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
in vec4 gl_FragCoord;
out float gl_FragDepth;
void main()
{
gl_FragDepth = gl_FragCoord.z;	
}