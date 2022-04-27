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
layout (location = 0) out vec3 colorBuffer;
out float gl_FragDepth;
in vec2 TexCoords;


layout (binding = 1) uniform usampler2D gBufferTex;//R = fiberID, G = vec4 (tan.xyz,unused.w)
layout (binding = 2) uniform sampler2D gBufferDepthTex;//r = depth

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

layout(std430, binding = 3)readonly buffer colorSSBO
{
    uint Criterion[];
};

const uint dataz2 = floatBitsToUint(data2.z);
const uint colorMode = (dataz2&0xff)>>2;
const uint clearColor = (dataz2>>1)%2;
const uint shadingMode = dataz2%2;

#define K_AMBIANT 0.5
#define K_DIFFUSE 0.4
#define K_SPECULAR 0.4
#define ALPHA_SPECULAR 2.0



vec3 plasmaColorMap(float t);
vec3 viridisColorMap(float t);
vec3 magmaColorMap(float t);
vec3 colorHash(float p);
float getCriterionfromFiberID(uint fid);
vec3 getColor(uint fid, vec4 tanv);
float depthOffset(ivec2 offset);

int kernelSize = 8; 
ivec2 iFragCoords = ivec2(gl_FragCoord.xy);


const int nbDir = 16;
void main()
{
    uvec2 fid = texelFetch(gBufferTex, iFragCoords, 0).rg;
    vec4 t = unpackSnorm4x8(fid.g);

    vec3 albedo = getColor(fid.x,t);
    gl_FragDepth = texelFetch(gBufferDepthTex, iFragCoords, 0).r;


    float occlusion = 1.0;

    if (gl_FragDepth < 1.0)
    {

        float currentDepth = 1.0/(invProjSpec[2][3]*(2.0*gl_FragDepth-1.0)+invProjSpec[3][3]);
        for (int i=-kernelSize; i<=kernelSize; i++)
            for (int c=-kernelSize; c<=kernelSize; c++)
            {
                float lPlan= length(vec2(i*2.0,c*2.0));
                float depthDiff = max(0.0,currentDepth-depthOffset(ivec2(i*2,c*2)))*data1.w*0.125/(data1.y*lPlan*currentDepth);
                occlusion += min(1.0,depthDiff*depthDiff);
            }
        occlusion /= kernelSize * kernelSize * 4 + 2 * kernelSize + 1;
        occlusion = 1.0- occlusion;
        if (shadingMode==0)//diffuse only
        {
        colorBuffer = albedo*occlusion;
        }
        else// illuminated lines
        {
        vec3 Specular = vec3(1.0,1.0,1.0);
        float occlusion2 = occlusion*occlusion;

        vec4 fragPos = invProjSpec*vec4(gl_FragCoord.xy*data0.zw-1.0,gl_FragDepth*2.0-1.0,1.0);
        fragPos.xyz/=fragPos.w;
        vec3 tcam = mat3(ModelViewCam)*t.xyz/data1.w;

        vec3 posToCam = normalize(-fragPos.xyz);
        vec3 binormalCam = normalize(cross(tcam,posToCam));
        vec3 normalCam = cross(binormalCam,tcam);
        vec3 cambitan = normalize(cross(normalCam,posToCam));
        float cylindercoefCam =(abs(dot(cambitan,binormalCam))+0.2)/1.2;//approximate version of the texturelookup
        colorBuffer = K_AMBIANT*albedo*(occlusion+1.0)*0.5;//AMBIANT : small occlusion effect at best
        for(int i=0;i<4;i++)//4 lights
        {
            float lightStrength = 1.0;
            vec3 posToLight = normalize(lights[i].xyz-fragPos.xyz);
            vec3 normal = normalize(cross(cross(posToLight,posToCam),tcam));
            normal*=sign(dot(normal,posToCam));
            float cylindercoef = max(0.0,(dot(normal,normalCam)+0.1)/1.1);//approximate version of the texturelookup
            vec3 lightbitan = normalize(cross(normalCam,posToLight));
            cylindercoef*=(abs(dot(lightbitan,binormalCam))+0.2)/1.2 * cylindercoefCam;//approximate version of the texturelookup
            float lambertian = dot(posToLight,normal);
            vec3 reflected = 2*lambertian*normal-posToLight;
            colorBuffer+= K_DIFFUSE*albedo*max(0.0,lambertian)*occlusion*cylindercoef;
            colorBuffer+= K_SPECULAR*Specular*pow(max(0.0,dot(posToCam,reflected)),ALPHA_SPECULAR)*occlusion2*cylindercoef;
        }
        }
    }
    else
        colorBuffer = vec3(1.0,1.0,1.0)*float(clearColor);
}


vec3 colorHash(float p)
{
    vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
   p3 += dot(p3, p3.yzx+33.33);
   return fract((p3.xxy+p3.yzz)*p3.zyx); 
}



vec3 getColor(uint fid, vec4 tanv)
{
    float critv = getCriterionfromFiberID(fid);
    if (colorMode==0)
    {
        return colorHash(float(fid));
    }
    else if (colorMode==1)
    {
        return vec3(0.5);
    }
    else if (colorMode==2)//tancolor
    {
        return abs(tanv.xyz);
    }
    else if (colorMode==3)//tancolor
    {
        return plasmaColorMap(critv);
    }
    else if (colorMode==4)//tancolor
    {
        return viridisColorMap(critv);
    }
    else if (colorMode==5)//tancolor
    {
        return magmaColorMap(critv);
    }
    else
    {
        return vec3(1.0,0.0,0.0);
    }

}



float getCriterionfromFiberID(uint fid)
{
    uint gid = fid>>2;
    uint lid = (fid%4)<<3;
    return float((Criterion[gid]>>lid)%256)/255.0;
}




vec3 plasmaColorMap(float t) {

    const vec3 c0 = vec3(0.05873234392399702, 0.02333670892565664, 0.5433401826748754);
    const vec3 c1 = vec3(2.176514634195958, 0.2383834171260182, 0.7539604599784036);
    const vec3 c2 = vec3(-2.689460476458034, -7.455851135738909, 3.110799939717086);
    const vec3 c3 = vec3(6.130348345893603, 42.3461881477227, -28.51885465332158);
    const vec3 c4 = vec3(-11.10743619062271, -82.66631109428045, 60.13984767418263);
    const vec3 c5 = vec3(10.02306557647065, 71.41361770095349, -54.07218655560067);
    const vec3 c6 = vec3(-3.658713842777788, -22.93153465461149, 18.19190778539828);

    return c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6)))));

}

vec3 viridisColorMap(float t) {

    const vec3 c0 = vec3(0.2777273272234177, 0.005407344544966578, 0.3340998053353061);
    const vec3 c1 = vec3(0.1050930431085774, 1.404613529898575, 1.384590162594685);
    const vec3 c2 = vec3(-0.3308618287255563, 0.214847559468213, 0.09509516302823659);
    const vec3 c3 = vec3(-4.634230498983486, -5.799100973351585, -19.33244095627987);
    const vec3 c4 = vec3(6.228269936347081, 14.17993336680509, 56.69055260068105);
    const vec3 c5 = vec3(4.776384997670288, -13.74514537774601, -65.35303263337234);
    const vec3 c6 = vec3(-5.435455855934631, 4.645852612178535, 26.3124352495832);

    return c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6)))));

}

vec3 magmaColorMap(float t) {

    const vec3 c0 = vec3(-0.002136485053939582, -0.000749655052795221, -0.005386127855323933);
    const vec3 c1 = vec3(0.2516605407371642, 0.6775232436837668, 2.494026599312351);
    const vec3 c2 = vec3(8.353717279216625, -3.577719514958484, 0.3144679030132573);
    const vec3 c3 = vec3(-27.66873308576866, 14.26473078096533, -13.64921318813922);
    const vec3 c4 = vec3(52.17613981234068, -27.94360607168351, 12.94416944238394);
    const vec3 c5 = vec3(-50.76852536473588, 29.04658282127291, 4.23415299384598);
    const vec3 c6 = vec3(18.65570506591883, -11.48977351997711, -5.601961508734096);
    float t1 = t*0.9+0.1;
    return c0+t1*(c1+t1*(c2+t1*(c3+t1*(c4+t1*(c5+t1*c6)))));
}

float depthOffset(ivec2 offset)
{
    ivec2 coord = iFragCoords + offset; 
    return 1.0/(invProjSpec[2][3]*(texelFetch(gBufferDepthTex, coord, 0).r * 2.0 - 1.0)+invProjSpec[3][3]);
}