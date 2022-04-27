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

#if ((NB_PTS_PER_BLOCK + 16) % 4 == 0)
	#define PACK_SIZE (NB_PTS_PER_BLOCK + 16)
#else
	#define PACK_SIZE ((NB_PTS_PER_BLOCK + 16) + (4 - (NB_PTS_PER_BLOCK + 16) % 4))
#endif

layout (points) in;
#if (NB_PTS_PER_BLOCK < 64)
	layout(line_strip, max_vertices = NB_PTS_PER_BLOCK + 1) out;
#else
	layout(line_strip, max_vertices = 64) out;//Max 64, here just for not crashing
#endif

in int index[];
layout (binding = 6) uniform sampler2D zDepthTestTex;//r = depth

flat out uint fiberID;
out vec4 tangent;

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

	#if (SPECTATOR_MODE==0)
		#define PROJ_MAT (projCamMultMV)
	#else
		#define PROJ_MAT (projSpecMultMV)
	#endif

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

/////////////////
uint getCriterionfromFiberID(uint fid)
{
    uint gid = fid>>2;
    uint lid = (fid%4)<<3;
    return (criterion[gid]>>lid)%256;
}
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
	ret.yz = v.yz * sqrt((1.00000001-ret.x*ret.x)/(1.00000001-v.x*v.x));
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
void main(void)
{
	//Decompression
	vec3 decompressedPoints[2];
	vec2 a = unpackUnorm2x16(listOfPoints[index[0]].data[0]);
	vec2 b = unpackUnorm2x16(listOfPoints[index[0]].data[1]);
	vec2 c = unpackUnorm2x16(listOfPoints[index[0]].data[2]);
	decompressedPoints[0] = vec3(a.x, a.y, b.x);
	decompressedPoints[1] = vec3(b.y, c.x, c.y);
	fiberID = listOfPoints[index[0]].data[3];

	uint nbPtPack = (listOfPoints[index[0]].data[4] >> 24);
	uint notEnd = uint((nbPtPack & 0x40)!=0);
	uint notBegin =  uint((nbPtPack & 0x80)!=0);
	uint mask = 0x40*notEnd + 0x80*notBegin;
	nbPtPack = nbPtPack & 0x3f;
	nbPtPack = min(min((nbPtPack), (nbPtPack) - 2), (nbPtPack) - 1);
	uint pid=0;
	
	uint crit = getCriterionfromFiberID(listOfPoints[index[gl_InvocationID]].data[3]) ;
	bool critRender = ( crit >= minCritCull ) && ( crit <= maxCritCull ); 
	float fiberRes = rejectFiber(decompressedPoints[0], fiberID)*float(critRender);
	if (fiberRes<=0.0 || nbPtPack == 0)
		return;

#if (AGGRESSIVE_OPTIM == 1)
	if (fiberRes<RENDER_SINGLE_SEGMENT_RATIO && notEnd==1)
	{
		tangent = vec4(normalize(decompressedPoints[1]-decompressedPoints[0]),mapToSnorm(mask+pid));pid++;
		gl_Position = PROJ_MAT * vec4(decompressedPoints[0],1.0);
		EmitVertex();
		vec3 decompressedPointsB[2];
		vec2 aB = unpackUnorm2x16(listOfPoints[index[0]+1].data[0]);
		vec2 bB = unpackUnorm2x16(listOfPoints[index[0]+1].data[1]);
		vec2 cB = unpackUnorm2x16(listOfPoints[index[0]+1].data[2]);
		decompressedPointsB[0] = vec3(aB.x, aB.y, bB.x);
		decompressedPointsB[1] = vec3(bB.y, cB.x, cB.y);
		tangent = vec4(normalize(decompressedPointsB[1]-decompressedPointsB[0]),mapToSnorm(mask+pid));pid++;
		gl_Position = PROJ_MAT * vec4(decompressedPointsB[0],1.0);
		EmitVertex();
		EndPrimitive();
		return;
	}
#endif

	mat4 Mi = retreiveFirstTransform(decompressedPoints);
	gl_Position = PROJ_MAT * Mi[3];

	tangent = vec4(Mi[0].xyz,mapToSnorm(mask+pid));pid++;
	EmitVertex();

	const uint current32bitsVal = listOfPoints[index[0]].data[4];
	const uint decalage = 8;
	const uint nextPoint = bitfieldExtract(current32bitsVal, int(decalage), 8);
	Mi = Mi * retreiveTransform(nextPoint);
	gl_Position = PROJ_MAT * Mi[3];
	tangent = vec4(Mi[0].xyz,mapToSnorm(mask+pid));pid++;
	EmitVertex();

	for (uint i=0; i<ceil(float(nbPtPack) / 2.0); i++)
	{
		for (uint j=0; j<2; j++)
		{
			const uint id = 2*i+j + 19;
#if (NB_PTS_PER_BLOCK % 2 != 0)
			if ((id - 19) >= NB_PTS_PER_BLOCK - 2) break;
#endif
			const uint current32bitsVal = listOfPoints[index[0]].data[(id)>>2];
			const uint decalage = ((id&0x03)^0x03)<<3;
			const uint nextPoint = bitfieldExtract(current32bitsVal, int(decalage), 8);

			Mi = Mi * retreiveTransform(nextPoint);
			if ((id - 19) < nbPtPack)//Odd nbPtPack will need one less vertex
			{
				gl_Position = PROJ_MAT * Mi[3];
				tangent = vec4(Mi[0].xyz,mapToSnorm(mask+pid));pid++;
				EmitVertex();
			}
		}
	}

#if (NB_PTS_PER_BLOCK < 64)//We can add the endpoint in this case
	if (notEnd==1)
	{
		a = unpackUnorm2x16(listOfPoints[index[0] + 1].data[0]);
		b = unpackUnorm2x16(listOfPoints[index[0] + 1].data[1]);
		c = unpackUnorm2x16(listOfPoints[index[0] + 1].data[2]);
		vec3 pn0 = vec3(a.x,a.y,b.x);
		vec3 pn1 = vec3(b.y,c.x,c.y);
		decompressedPoints[0] = pn0;
		decompressedPoints[1] = pn1;

		Mi = retreiveFirstTransform(decompressedPoints);
		gl_Position = PROJ_MAT * Mi[3];
		tangent = vec4(Mi[0].xyz,mapToSnorm(mask+pid));pid++;
		EmitVertex();
	}
#endif
	EndPrimitive();
}
