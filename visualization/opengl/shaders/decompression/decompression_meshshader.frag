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

#extension GL_NV_fragment_shader_barycentric : require


#if (OCCLUDER_FRAGMENT_TEST == 0 || MESH_CUT_TEST==0)
	layout(early_fragment_tests) in;//cannot work if fragment are discarded
#endif

layout (location = 0) out uvec2 gBuffer;// -> x = fiber id, y = packtan
layout (binding = 5) uniform sampler2D fragOccluderTex ;//rg



 pervertexNV in PerVertexData
{
	uint fid;
	uint packedTan;
} v_out[];

out float gl_FragDepth;

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


float mapToSnorm(uint val)
{
	return float(val)/127.0-1.0;
}

void main(void)
{
	#if (OCCLUDER_FRAGMENT_TEST == 1 && MESH_CUT_TEST==1)
	vec2 zoccluder  = texelFetch(fragOccluderTex,ivec2(gl_FragCoord.xy),0).xy;
	#if (OCCLUDER_CULL_OUTSIDE == 1)
	    bool t = gl_FragCoord.z>=zoccluder.x || gl_FragCoord.z<zoccluder.y;
	#else
    	bool t = gl_FragCoord.z<zoccluder.x && gl_FragCoord.z>zoccluder.y;
 	#endif
 	gl_FragDepth = gl_FragCoord.z + float(t);
   	#endif
    

 	vec4 interpoledTan = unpackSnorm4x8(v_out[0].packedTan)*gl_BaryCoordNV.x + unpackSnorm4x8(v_out[1].packedTan)*gl_BaryCoordNV.y;
	gBuffer.x = uint(v_out[0].fid);
	gBuffer.y = packSnorm4x8(vec4(normalize(interpoledTan.xyz),interpoledTan.w));
}
