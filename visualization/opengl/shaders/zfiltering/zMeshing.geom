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
layout (points) in;
layout(triangle_strip, max_vertices = 4) out;

in int index[];
int GID = index[gl_InvocationID];
layout (binding = 7) uniform sampler2D zBuffer;

layout(binding = 1, shared) uniform UBO_STATIC
{
    vec4 data0;//xy=m_sizeZtestViewPort.xy, zw=2.0/(m_sizeFBO.xy-1.0)
    vec4 data1;//x = guardBand, y = angularCorrection, z = ratio, w = scaling
    vec4 data2;//x = unused, y = tesselationGain, z = colormode & backgroundflag, w = m_sizeSubsampledZBuffers.x
    vec4 data3;//xyz = frustrum test constants, w = stepsize
    vec4 sliders;//x = percent of fiber drawn, y = selected fiber Radius size, z = alpha selected fiber, w = size of selection
};

layout(binding = 2, shared) uniform UBO_DYNAMIC
{
    mat4 projCam;
    mat4 ZFilterinvCamProjMultTexShift;
    mat4 ProjMultCurrentPoseMultInvPreviousProjPoseMultTexShift;
    mat4 projCamMultMV;
    mat4 ModelViewCam;
    mat4 ModelViewOccluder;

    mat4 projSpec;
    mat4 invProjSpec;
    mat4 ModelViewSpec;
    mat4 projSpecMultMV;

    mat4 lights;//4xvec4(xLight,yLigth,zLight,1.0) in specCamSpace
};


out vec3 posFrag;//last coord is scale
out vec4 circlePos;

void main(void)
{
	int sx = int(data2.w+0.2)>>2;
	ivec2 pix;
	pix.x = GID&0x00000003;
	pix.y = (GID>>2)&0x00000003;
	ivec2 tile;
	int pt = GID>>4;
	tile.x = (pt%sx);
	tile.y = (pt/sx);
	pix += 4*tile;

	float z = texelFetch(zBuffer,pix,0).x;
	vec4 center3dPos = ZFilterinvCamProjMultTexShift*vec4(vec2(pix)+0.5,z,1.0);
	center3dPos/=center3dPos.w;

	float l = length(center3dPos.xyz);
	vec3 dir = center3dPos.xyz/l;
	vec3 right = normalize(cross(dir,vec3(1.0,0.0,0.0)));
	vec3 up = cross(dir,right);

	float sc = (data1.x-data1.y*center3dPos.z);
	float scsqrt2 = 1.42*sc;//sqrt2
	circlePos.z = sc;
	circlePos.w = l;

	circlePos.xy = vec2(-scsqrt2,-scsqrt2);
	posFrag.xyz = center3dPos.xyz - scsqrt2*right;
	gl_Position = projCam*vec4(posFrag.xyz,1.0);
	gl_Position.w = max(gl_Position.w,gl_Position.z);
	EmitVertex();
	circlePos.xy = vec2(-scsqrt2,scsqrt2);
	posFrag.xyz = center3dPos.xyz - scsqrt2*up;
	gl_Position = projCam*vec4(posFrag.xyz,1.0);
	gl_Position.w = max(gl_Position.w,gl_Position.z);
	EmitVertex();
	circlePos.xy = vec2(scsqrt2,-scsqrt2);
	posFrag.xyz = center3dPos.xyz + scsqrt2*up;
	gl_Position = projCam*vec4(posFrag.xyz,1.0);
	gl_Position.w = max(gl_Position.w,gl_Position.z);
	EmitVertex();
	circlePos.xy = vec2(scsqrt2,scsqrt2);
	posFrag.xyz = center3dPos.xyz + scsqrt2*right;
	gl_Position = projCam*vec4(posFrag.xyz,1.0);
	gl_Position.w = max(gl_Position.w,gl_Position.z);
	EmitVertex();	
	EndPrimitive();
}
