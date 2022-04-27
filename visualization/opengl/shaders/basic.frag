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
in vec3 n;
out vec4 Color;
void main()
{
vec3 c = abs(cos(n*n.yzx*20.0 + n.zxy*n.yzx*10.0)*0.6)*0.4+0.3;

Color = vec4(vec3(dot(abs(n), vec3(1.0))/3.0*0.8+0.1), 0.0);
}