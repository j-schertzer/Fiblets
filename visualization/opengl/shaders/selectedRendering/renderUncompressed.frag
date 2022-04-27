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

layout (location = 0) out uvec2 gBuffer;// -> x = fiber id, y = packtan
out float gl_FragDepth;// ->gBufferDepthTex

flat in uint fiberID;//no interpolation
in vec4 gl_FragCoord;
void main(void)
{

    gBuffer.x = fiberID;
    gBuffer.y = packSnorm4x8(vec4(1.0,0.0,0.0,0.0));
    gl_FragDepth = gl_FragCoord.z;

}