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

#if NB_PTS_PER_BLOCK > 39
	#define COMPRESSION_NEEDED
#endif

layout (isolines, equal_spacing/*fractional_even_spacing*//*, point_mode*/, ccw) in;

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




#ifdef COMPRESSION_NEEDED
	patch in uvec4 decompressedValues[25];// 192 half floats -> 96 floats -> 24 vec4 + first point of next pack
#else
	//BE CAREFUL, ALIGNEMENT IS DONE SO VEC3 BECOME VEC4...
	patch in vec4 decompressedValues[int(ceil((NB_PTS_PER_BLOCK + 1) * 3 / 4.0))];// NB_PTS_PER_BLOCK points + first point of next pack, max 40 points so 120 floats
#endif
flat in uint fiberIDtcs[];
in vec3 nextDir[];
flat in uint blockStatus[];
flat out uint fiberID;
out vec4 tangent;


vec3 getPos(in uint ptNberx3)
{
	vec3 position;
#ifdef COMPRESSION_NEEDED
	const uint ptNberx3Div2 = ptNberx3>>1;//Division by 2

	vec2 part1 = unpackUnorm2x16(decompressedValues[(ptNberx3Div2)>>2][(ptNberx3Div2)&0x03]);
	vec2 part2 = unpackUnorm2x16(decompressedValues[(ptNberx3Div2 + 1)>>2][(ptNberx3Div2 + 1)&0x03]);
	const uint ptNberx3Mod2 = (ptNberx3)&0x01;
	if (ptNberx3Mod2 == 0)//Modulo 2
	{
		position = vec3(part1.x, part1.y, part2.x);
	}
	else
	{
		position = vec3(part1.y, part2.x, part2.y);
	}
#else
	const uint ptNberx3Div4 = ptNberx3>>2;//Division by 4
	vec4 part1 = decompressedValues[ptNberx3Div4];
	vec4 part2 = decompressedValues[ptNberx3Div4 + 1];
	const uint ptNberx3Mod4 = (ptNberx3)&0x03;//Modulo 4
	if (ptNberx3Mod4 < 2)//To avoid branching
	{
		if (ptNberx3Mod4 == 0)
		{
			position = vec3(part1.x, part1.y, part1.z);
		}
		else
		{
			position = vec3(part1.y, part1.z, part1.w);
		}
	}
	else
	{
		if (ptNberx3Mod4 == 2)
		{
			position = vec3(part1.z, part1.w, part2.x);
		}
		else
		{
			position = vec3(part1.w, part2.x, part2.y);
		}
	}
#endif
return position;
}




void main(void)
{
	float u = gl_TessCoord.x;
	fiberID = fiberIDtcs[0];
	tangent = vec4(1.0,0.0,0.0,0.0);

	const uint ptID = uint(u * ((blockStatus[0]&0x3f)));
	const uint ptNberx3 =  ptID * 3;

	float bid = float(blockStatus[0]&0xC0 + ptID)/127.0-1.0;
	vec3 position = getPos(ptNberx3);

	if (u == 1.0)
		tangent = vec4(nextDir[0],bid);
	else
		tangent = vec4(getPos(ptNberx3+3)-position,bid);

	#if (SPECTATOR_MODE==0)
	gl_Position = projCamMultMV * vec4(position, 1.0);
	#else
	gl_Position = projSpecMultMV * vec4(position, 1.0);
	#endif
}
