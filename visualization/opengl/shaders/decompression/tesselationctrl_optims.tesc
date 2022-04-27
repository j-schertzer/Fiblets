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

#if (NB_PTS_PER_BLOCK > 39)
	#define COMPRESSION_NEEDED
#endif
#if ((NB_PTS_PER_BLOCK + 16) % 4 == 0)
	#define PACK_SIZE (NB_PTS_PER_BLOCK + 16)
#else
	#define PACK_SIZE ((NB_PTS_PER_BLOCK + 16) + (4 - (NB_PTS_PER_BLOCK + 16) % 4))// (4 - (NB_PTS_PER_BLOCK + 16) % 4))
#endif

layout (vertices = 1) out;

in int index[];
layout (binding = 6) uniform sampler2D zDepthTestTex;//r = depth


#define ratio (data1.z)
#define stepsize (data3.w)
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


struct pack
{
	uint data[PACK_SIZE / 4];
};
layout(std430, binding = 2)readonly buffer blockSSBO
{
	pack listOfPoints[];
	//data[0] = first.x, first.y
	//data[1] = first.z, second.x
	//data[2] = second.y, second.z
	//data[3] = FiberID
	//data[4] = state(2bits), number of compressed points(6bits) -  block ID (1 byte) - 2 first points (2 bytes)
	//data[5 - 19] = 60 compressedPoints
};

layout(std430, binding = 3)readonly buffer colorSSBO
{
    uint criterion[];
};
uint getCriterionfromFiberID(uint fid);
const uint dataz2 = floatBitsToUint(data2.z);
const uint minCritCull = (dataz2)>>24;
const uint maxCritCull = ((dataz2)>>16)&0xff;


#ifdef COMPRESSION_NEEDED
	patch out uvec4 decompressedValues[25];// 192 half floats -> 96 floats -> 24 vec4 + first point of next pack
#else
//BE CAREFUL, ALIGNEMENT IS DONE SO VEC3 BECOME VEC4...
	patch out vec4 decompressedValues[int(ceil((NB_PTS_PER_BLOCK + 1) * 3 / 4.0))];// NB_PTS_PER_BLOCK points + first point of next pack, max 40 points so 120 floats
#endif

flat out uint fiberIDtcs[];
out vec3 nextDir[];
flat out uint blockStatus[];

/////////////////
float mapToSnorm(uint val)
{
	return float(val)/127.0-1.0;
}

vec3 unquantize(in uint point)
{
	float qsize = 16.0;

	float q2 = float(point & 0x0f) / (qsize-1.0f)*2.0f-1.0f;
	float q1 = float(point>>4) / (qsize-1.0f)*2.0f-1.0f;

	vec3 dir;
	dir.y = (q1 + q2)*0.5f;
	dir.z = (q1 - q2)*0.5f;
	dir.x = 1.0f - abs(dir.y) - abs(dir.z);

	return normalize(dir);
}

vec3 uniformMappingAxisX(in vec3 v)
{
	//v has been normalized previously
	vec3 ret;
	ret.x = mix(1.0,v.x,ratio);
	ret.yz = v.yz * sqrt((1.0000001-ret.x*ret.x)/(1.0000001-v.x*v.x));
	return ret;
}

mat4 retreiveFirstTransform(in vec3 points[2])
{
	//the first transform matrix encode the transformation from the global frame
	//to the first local frame
	mat4 transform;
	transform[0].xyz = normalize(points[1] - points[0]);
	transform[0].w =0.0;
	transform[1].xyz = normalize(cross(transform[0].xyz,C));
	transform[1].w =0.0;
	transform[2].xyz = cross(transform[0].xyz,transform[1].xyz);
	transform[2].w =0.0;
	transform[3] = vec4(points[0],1.0);
	return transform;
}

mat4 retreiveTransform(in uint point)
{
	//transform matrix encode local rotation from current local frame
	//to next local frame, it also encode the translation of a stepsize distance
	//note that frame propagation has to be consistent with the way it was built
	//within the compressed block. The local propagation using the custom vector C
	//and the CPU-side algorithm ensure that property
	mat4 transform;
	transform[0].xyz = uniformMappingAxisX(unquantize(point));
	transform[0].w =0.0;
	transform[1].xyz = normalize(cross(transform[0].xyz,C));//local transof
	transform[1].w =0.0;
	transform[2].xyz = cross(transform[0].xyz,transform[1].xyz);
	transform[2].w =0.0;
	transform[3] = vec4(stepsize,0.0,0.0,1.0);
	return transform;
}

bool testInGuardFrustrum(const in vec4 v)
{
	float s = v.x/(v.w+data3.x);
	bool ret  = (-1.0< s) && (s < 1.0);
	s = v.y/(v.w+data3.y);
	return ret && (-1.0< s) && (s < 1.0) && (v.w>data3.z);
}

bool testRenderZmask(const in vec4 v)
{
	vec3 p = v.xyz/v.w*0.5+0.5;// in [0,1]
	vec3 zm = textureLod(zDepthTestTex,p.xy,0).xyz;//.x=ZB .y = ZOB .z = ZOF

	#if (MESH_CUT_TEST == 1)
		#if (OCCLUDER_CULL_OUTSIDE==1)
			return (p.z<zm.x) && ( (p.z<zm.y) && (p.z>zm.z) );
		#else
			return (p.z<zm.x) && ( (p.z>zm.y) || (p.z<zm.z) );
		#endif
	#else
		return (p.z<zm.x);
	#endif

}

uint getCriterionfromFiberID(uint fid)
{
    uint gid = fid>>2;
    uint lid = (fid%4)<<3;
    return (criterion[gid]>>lid)%256;
}


float rejectFiber(const in vec3 pt, const uint fid)
{
	vec4 ptest = projCamMultMV * vec4(pt,1.0);
	if (testInGuardFrustrum(ptest)==false)
		return -1.0;
	if (testRenderZmask(ptest)==false)
		return -1.0;

	float tessL = clamp(data2.y/max(0.00001,ptest.w-data1.x),0.0,1.0);

	#if (AGGRESSIVE_OPTIM == 1)
	//we delete 900/1000 of fibers if tessL = 0.0 (asymptotic case)
	uint threshold = uint(mix(900.0,0.0,2.0*tessL/RENDER_SINGLE_SEGMENT_RATIO));

	if ((fid%1000) < threshold)
		return -1.0;
	#endif

	return tessL;
}

#ifdef COMPRESSION_NEEDED
void fill(inout uint indice,in vec3 tab[2])
{
	uvec2 val = uvec2(indice>>2,indice&0x03);
	decompressedValues[val.x][val.y] = packUnorm2x16(tab[0].xy);
	indice++;
	val = uvec2(indice>>2,indice&0x03);
	decompressedValues[val.x][val.y] = packUnorm2x16(vec2(tab[0].z,tab[1].x));
	indice++;
	val = uvec2(indice>>2,indice&0x03);
	decompressedValues[val.x][val.y] = packUnorm2x16(tab[1].yz);
	indice++;
	return;
}
#else
void fill(inout uint indice, in vec3 pt)
{
	uvec2 val = uvec2(indice>>2,indice&0x03);
	decompressedValues[val.x][val.y] = pt.x;
	indice++;
	val = uvec2(indice>>2,indice&0x03);
	decompressedValues[val.x][val.y] = pt.y;
	indice++;
	val = uvec2(indice>>2,indice&0x03);
	decompressedValues[val.x][val.y] = pt.z;
	indice++;
	return;
}
#endif

void main(void)
{
	vec2 a = unpackUnorm2x16(listOfPoints[index[gl_InvocationID]].data[0]);
	vec2 b = unpackUnorm2x16(listOfPoints[index[gl_InvocationID]].data[1]);
	vec2 c = unpackUnorm2x16(listOfPoints[index[gl_InvocationID]].data[2]);

	fiberIDtcs[gl_InvocationID] = listOfPoints[index[gl_InvocationID]].data[3];

	uint nbPtPack = (listOfPoints[index[gl_InvocationID]].data[4] >> 24);// & 0x3f;
	uint notEnd = uint((nbPtPack & 0x40)!=0);
	uint notBegin =  uint((nbPtPack & 0x80)!=0);//looks like it is inverted
	uint tessLevelm2 = (nbPtPack & 0x3f) - 1 + notEnd;//nbseg
	blockStatus[gl_InvocationID] = tessLevelm2 + 0x40*notEnd + 0x80*notBegin;
	nbPtPack = nbPtPack & 0x3f;
	nbPtPack = min(min((nbPtPack), (nbPtPack) - 2), (nbPtPack) - 1);

	//Decompression
	vec3 nextTan;
	vec3 decompressedBuffer[2];
	decompressedBuffer[0] = vec3(a.x, a.y, b.x);//point0
	decompressedBuffer[1] = vec3(b.y, c.x, c.y);//point1

	uint crit = getCriterionfromFiberID(listOfPoints[index[gl_InvocationID]].data[3]) ;
	bool critRender = ( crit >= minCritCull ) && ( crit <= maxCritCull ); 
	float levelT = rejectFiber(decompressedBuffer[0], fiberIDtcs[gl_InvocationID])*float(critRender);
	if (levelT<=0.0)//culling fibers
	{
		gl_TessLevelOuter[0] = 0;
		gl_TessLevelOuter[1] = 0;
		return ;
	}
	gl_TessLevelOuter[0] = 1;
	#if (AGGRESSIVE_OPTIM == 1)
	gl_TessLevelOuter[1] = max(1.0,levelT*tessLevelm2);
	#else
	gl_TessLevelOuter[1] = tessLevelm2;//levelT;
	#endif
	//Directly pass to evaluation shader the two first points
#ifdef COMPRESSION_NEEDED
		decompressedValues[0][0] = listOfPoints[index[gl_InvocationID]].data[0];
		decompressedValues[0][1] = listOfPoints[index[gl_InvocationID]].data[1];
		decompressedValues[0][2] = listOfPoints[index[gl_InvocationID]].data[2];

		uint indice = 3;
#else
		uint indice = 0;
		fill(indice, decompressedBuffer[0]);
		fill(indice, decompressedBuffer[1]);
#endif

	//First two matrices are needed for next points
	mat4 Mi = retreiveFirstTransform(decompressedBuffer);
	const uint current32bitsVal = listOfPoints[index[gl_InvocationID]].data[4];
	const uint decalage = 8;
	const uint nextPoint = bitfieldExtract(current32bitsVal, int(decalage), 8);
	Mi = Mi * retreiveTransform(nextPoint);

		//Loop on compressed points
	for (uint i=0; i<ceil(float(nbPtPack) / 2.0); i++)
	{
		for(uint j=0;j<2;j++)
		{
			const uint id = 2*i+j + 19;
			const uint current32bitsVal = listOfPoints[index[gl_InvocationID]].data[(id)>>2];
			const uint decalage = ((id&0x03)^0x03)<<3;
			const uint nextPoint = bitfieldExtract(current32bitsVal, int(decalage), 8);

			if (id-18 > tessLevelm2)
				nextTan = nextTan;
			else
				nextTan = vec3(Mi[0]);
			Mi = Mi * retreiveTransform(nextPoint);
			decompressedBuffer[j] = vec3(Mi[3]);
	#ifndef COMPRESSION_NEEDED
			fill(indice, decompressedBuffer[j]);
	#endif
		}
	#ifdef COMPRESSION_NEEDED
		fill(indice,decompressedBuffer);
	#endif
	}

	vec2 an = unpackUnorm2x16(listOfPoints[index[gl_InvocationID]+1].data[0]);
	vec2 bn = unpackUnorm2x16(listOfPoints[index[gl_InvocationID]+1].data[1]);
	vec2 cn = unpackUnorm2x16(listOfPoints[index[gl_InvocationID]+1].data[2]);

	if (notEnd==1)
	{
		vec2 an = unpackUnorm2x16(listOfPoints[index[gl_InvocationID]+1].data[0]);
		vec2 bn = unpackUnorm2x16(listOfPoints[index[gl_InvocationID]+1].data[1]);
		vec2 cn = unpackUnorm2x16(listOfPoints[index[gl_InvocationID]+1].data[2]);
		nextDir[gl_InvocationID] = vec3(bn.y, cn.x, cn.y)-vec3(an.x, an.y, bn.x);
		#ifdef COMPRESSION_NEEDED
			uvec2 val = uvec2(indice>>2,indice&0x03);
			decompressedValues[val.x][val.y] = listOfPoints[index[gl_InvocationID] + 1].data[0];
			indice++;
			val = uvec2(indice>>2,indice&0x03);
			decompressedValues[val.x][val.y] = listOfPoints[index[gl_InvocationID] + 1].data[1];
		#else
			a = unpackUnorm2x16(listOfPoints[index[gl_InvocationID] + 1].datas[0]);
			b = unpackUnorm2x16(listOfPoints[index[gl_InvocationID] + 1].data[1]);
			fill(indice, vec3(a.x, a.y, b.x));
		#endif
	}
	else //fiber end
	{
		nextDir[gl_InvocationID] = nextTan*stepsize;
	}
}
