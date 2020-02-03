#define FXAA_GRAY_AS_LUMA 1
#define FXAA_PC 1
#define FXAA_HLSL_5 1
#include "FXAA.hlsl"

Texture2D<float4> tex : register(t0);           // 通常テクスチャ
Texture2D<float4> normal : register(t1);        //法線
Texture2D<float4> bright : register(t2);        //高輝度成分
Texture2D<float4> cameraDepth : register(t4);   //カメラ深度値
Texture2D<float4> lightDepth : register(t5);    //ライト深度値
Texture2D<float4> bloom : register(t6);         //高輝度縮小バッファ
Texture2D<float4> dof : register(t7);           //通常縮小バッファ

SamplerState smp : register(s0);

cbuffer status : register(b0)
{
    uint debug;
    uint cameraOutLine;
    uint normalOutLine;
    uint bloomFlag;
    uint dofFlag;
    float time;
    float3 col1;
    float3 col2;
    float3 col3;
};

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 tpos : POSITION;
};

//頂点シェーダ
Output vs(float4 pos : POSITION, float2 uv : TEXCOORD)
{
    Output output;
    output.svpos = pos;
    output.uv = uv;
    output.tpos = pos;
    return output;
}

float4 GaussianFilter(Texture2D<float4> t, SamplerState s, float2 uv, float dx, float dy)
{
    float4 center = t.Sample(s, uv);
    float4 ret =
    t.Sample(s, uv + float2(-2 * dx, 2 * dy)) * 1 +
    t.Sample(s, uv + float2(-1 * dx, 2 * dy)) * 4 +
    t.Sample(s, uv + float2(0 * dx, 2 * dy)) * 6 +
    t.Sample(s, uv + float2(1 * dx, 2 * dy)) * 4 +
    t.Sample(s, uv + float2(2 * dx, 2 * dy)) * 1 +
    
    t.Sample(s, uv + float2(-2 * dx, 1 * dy)) * 4 +
    t.Sample(s, uv + float2(-1 * dx, 1 * dy)) * 16 +
    t.Sample(s, uv + float2(0 * dx, 1 * dy)) * 24 +
    t.Sample(s, uv + float2(1 * dx, 1 * dy)) * 16 +
    t.Sample(s, uv + float2(2 * dx, 1 * dy)) * 4 +

    t.Sample(s, uv + float2(-2 * dx, 0 * dy)) * 6 +
    t.Sample(s, uv + float2(-1 * dx, 0 * dy)) * 24 +
    center * 36 +
    t.Sample(s, uv + float2(1 * dx, 0 * dy)) * 24 +
    t.Sample(s, uv + float2(2 * dx, 0 * dy)) * 6 +
    
    t.Sample(s, uv + float2(-2 * dx, -1 * dy)) * 4 +
    t.Sample(s, uv + float2(-1 * dx, -1 * dy)) * 16 +
    t.Sample(s, uv + float2(0 * dx, -1 * dy)) * 24 +
    t.Sample(s, uv + float2(1 * dx, -1 * dy)) * 16 +
    t.Sample(s, uv + float2(2 * dx, -1 * dy)) * 4 +
    
    t.Sample(s, uv + float2(-2 * dx, -2 * dy)) * 1 +
    t.Sample(s, uv + float2(-1 * dx, -2 * dy)) * 4 +
    t.Sample(s, uv + float2(0 * dx, -2 * dy)) * 6 +
    t.Sample(s, uv + float2(1 * dx, -2 * dy)) * 4 +
    t.Sample(s, uv + float2(2 * dx, -2 * dy)) * 1;

    ret = ret / 256;
    
    return float4(ret.rgb, ret.a);
}

float2 Rotate(float2 pos, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return mul(float2x2(c, -s, s, c), pos);
}

float2 InvRotate(float2 pos, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return mul(float2x2(c, s, -s, c), pos);
}

float3 mod(float3 x, float y)
{
    return x - y * floor(x / y);
}

float3 ModPos(float3 pos, float divider)
{
    return mod(pos, divider) - divider / 2;
}

float3 RGBtoHSV(float3 rgb)
{
    float3 hsv;
    // RGB 2 HSV    
    float maxNum = max(rgb.r, max(rgb.g, rgb.b));
    float minNum = min(rgb.r, min(rgb.g, rgb.b));
    float delta = maxNum - minNum;
    hsv.z = maxNum; // v
    if (maxNum != 0.0)
    {  
        hsv.y = delta / maxNum; //s        
    }
    else
    {
        hsv.y = 0.0; //s
    }
    if (rgb.r == maxNum)
    {
        hsv.x = (rgb.g - rgb.b) / delta; // h
    }
    else if (rgb.g == maxNum)
    {
        hsv.x = 2 + (rgb.b - rgb.r) / delta; // h
    }
    else
    {
        hsv.x = 4 + (rgb.r - rgb.g) / delta; // h
    }
    hsv.x /= 6.0;
    if (hsv.x < 0)
    {
        hsv.x += 1.0;
    }
    return hsv;
}

float3 HSVtoRGB(float3 hsv)
{
    float3 ret;
    // HSV 2 RGB
    if (hsv.y == 0)
    { /* Grayscale */
        ret.r = ret.g = ret.b = hsv.z; // v
    }
    else
    {
        if (1.0 <= hsv.x)
            hsv.x -= 1.0;
        hsv.x *= 6.0;
        float i = floor(hsv.x);
        float f = hsv.x - i;
        float aa = hsv.z * (1 - hsv.y);
        float bb = hsv.z * (1 - (hsv.y * f));
        float cc = hsv.z * (1 - (hsv.y * (1 - f)));
        if (i < 1)
        {
            ret.r = hsv.z;
            ret.g = cc;
            ret.b = aa;
        }
        else if (i < 2)
        {
            ret.r = bb;
            ret.g = hsv.z;
            ret.b = aa;
        }
        else if (i < 3)
        {
            ret.r = aa;
            ret.g = hsv.z;
            ret.b = cc;
        }
        else if (i < 4)
        {
            ret.r = aa;
            ret.g = bb;
            ret.b = hsv.z;
        }
        else if (i < 5)
        {
            ret.r = cc;
            ret.g = aa;
            ret.b = hsv.z;
        }
        else
        {
            ret.r = hsv.z;
            ret.g = aa;
            ret.b = bb;
        }
    }
    return ret;
}

//////////////////////////////////距離関数////////////////////////////////////////////////
float SDFSphere3D(float3 p, float r)
{
    return length(p) - r;
}

float SDFBox(float3 p, float3 b)
{
    float3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.y, max(q.x, q.z)), 0.0);
}

float SDFRoundBox(float3 p, float3 b, float r)
{
    float3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0) - r;
}

float SDFHex3d(float3 p, float2 h)
{
    const float3 k = float3(-0.8660254, 0.5, 0.57735);
    p = abs(p);
    p.xy -= 2.0 * min(dot(k.xy, p.xy), 0.0) * k.xy;
    float2 d = float2(length(p.xy - float2(clamp(p.x, -k.z * h.x, k.z * h.x), h.x)) * sign(p.y - h.x), p.z - h.y);
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

float SDFOctahedron(float3 p, float s)
{
    p = abs(p);
    float m = p.x + p.y + p.z - s;
    float3 q;
    if (3.0 * p.x < m)
        q = p.xyz;
    else if (3.0 * p.y < m)
        q = p.yzx;
    else if (3.0 * p.z < m)
        q = p.zxy;
    else
        return m * 0.57735027;
    
    float k = clamp(0.5 * (q.z - q.y + s), 0.0, s);
    return length(float3(q.x, q.y - s + k, q.z - k));
}
/////////////////////////////////////////////////////////////////////////////////////////
float SceneDist(float3 p)
{
    float r = 0.2f;
    float3 size = float3(0.4, 0.4, 0.4);
    float div = 3;
    
    float3 p1 = ModPos(p, div);
    p1.xz = Rotate(p1.xz, time);

    //float len = SDFSphere3D(p1, r);
    float len = SDFRoundBox(p1, size, 0.1f);
    //float len = SDFOctahedron(p1, 0.3f);
    //float len = SDFBox(p1, size);
    //float len = min(SDFOctahedron(p1, 0.3f), SDFOctahedron(p2, 0.3f));
    
    return len;
}

float4 SceneColor(float3 p)
{
    float div = 3;
    float3 p1 = ModPos(p, div);
    p1.xz = Rotate(p1.xz, time);
    return float4(HSVtoRGB(float3((p.x + p.y + p.z) / 30.0, 1.0, 1.0)), 1);
}

float3 GetNormal(float3 pos)
{
    const float epsilon = 0.0001;
    return normalize(float3(SceneDist(pos + float3(epsilon, 0, 0)) - SceneDist(pos + float3(-epsilon, 0, 0)),
                            SceneDist(pos + float3(0, epsilon, 0)) - SceneDist(pos + float3(0, -epsilon, 0)),
                            SceneDist(pos + float3(0, 0, epsilon)) - SceneDist(pos + float3(0, 0, -epsilon))));
}

float4 RayMarching(float3 pos,float3 ray,float3 light,uint maxNum)
{
    const int cnt = 64;
    
    float3 p = pos;
    float depth = 0;
    float len;
    for (int i = 0; i < cnt; ++i)
    {
        len = SceneDist(p);
        depth += len;
        p = depth * ray + pos;

        if (len < 0.001f)
        {
            float3 norm = GetNormal(p);
            float3 rlight = normalize(reflect(light, norm));
            float diffuse = clamp(dot(norm, light), 0.1, 1.0);
            float3 spec = pow(saturate(dot(rlight, ray)), 50);
            float3 col = SceneColor(p).rgb * diffuse + spec;
            return float4(col - pow(clamp(0.05 * depth, 0.0, 0.6), 2.0), 1);
        }
    }
    return float4(0, 0, 0, 1);
}

struct ShrinkOut
{
    float4 hightLight : SV_Target0;
    float4 col : SV_Target1;
};

ShrinkOut BloomPS(Output o) : SV_Target
{
    float w, h, miplevels;
    tex.GetDimensions(0, w, h, miplevels);
    float dx = 1.0 / w;
    float dy = 1.0 / h;
    ShrinkOut ret;
    ret.col = GaussianFilter(tex, smp, o.uv, dx, dy);
    ret.hightLight = GaussianFilter(bright, smp, o.uv, dx, dy);
    return ret;
}


//ピクセルシェーダ
float4 ps(Output o) : SV_Target
{    
    ///////////////////////////////////////デバッグ用/////////////////////////////////////////////
    if(debug == 1)
    {
        float dep = cameraDepth.Sample(smp, o.uv * 5);
        dep = pow(dep, 100);
        if (o.uv.x <= 0.2 && o.uv.y <= 0.2)
        {
            return 1 - float4(dep, dep, dep, 1);
        }
        float sha = lightDepth.Sample(smp, o.uv * 5);
        sha = pow(sha, 100);
        if (o.uv.x <= 0.2 && o.uv.y <= 0.4)
        {
            return 1 - float4(sha, sha, sha, 1);
        }
        float4 no = normal.Sample(smp, o.uv * 5);
        if (o.uv.x <= 0.2 && o.uv.y <= 0.6)
        {
            return no;
        }
        float4 br = bright.Sample(smp, o.uv * 5);
        if (o.uv.x <= 0.2 && o.uv.y <= 0.8)
        {
            return br;
        }
        float4 bl = bloom.Sample(smp, o.uv * 5);
        if (o.uv.x <= 0.2 && o.uv.y <= 1.0)
        {
            return bl;
        }
        float4 sh = dof.Sample(smp, o.uv * 5);
        if (o.uv.x <= 0.4 && o.uv.y <= 0.2)
        {
            return sh;
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    float4 color = tex.Sample(smp, o.uv);
    float4 basenorm = normal.Sample(smp, o.uv);
    float w, h, miplevel;
    tex.GetDimensions(0, w, h, miplevel);
    float dx = 1.0f / w;
    float dy = 1.0f / h;
    
    //FXAA
    FxaaTex
        InputFXAATex =
    {
        smp, tex
    };
    float3 aa = FxaaPixelShader(
		o.uv, // FxaaFloat2 pos,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f), // FxaaFloat4 fxaaConsolePosPos,
		InputFXAATex, // FxaaTex tex,
		InputFXAATex, // FxaaTex fxaaConsole360TexExpBiasNegOne,
		InputFXAATex, // FxaaTex fxaaConsole360TexExpBiasNegTwo,
		float2(dx, dy), // FxaaFloat2 fxaaQualityRcpFrame,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f), // FxaaFloat4 fxaaConsoleRcpFrameOpt,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f), // FxaaFloat4 fxaaConsoleRcpFrameOpt2,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f), // FxaaFloat4 fxaaConsole360RcpFrameOpt2,
		0.75f, // FxaaFloat fxaaQualitySubpix,
		0.166f, // FxaaFloat fxaaQualityEdgeThreshold,
		0.0833f, // FxaaFloat fxaaQualityEdgeThresholdMin,
		0.0f, // FxaaFloat fxaaConsoleEdgeSharpness,
		0.0f, // FxaaFloat fxaaConsoleEdgeThreshold,
		0.0f, // FxaaFloat fxaaConsoleEdgeThresholdMin,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f) // FxaaFloat fxaaConsole360ConstDir,
	).rgb;
    
    color.rgb = aa;
    
    //カメラ深度による輪郭線
    float edge = 1;
    if(cameraOutLine==1)
    {
        float lineWeight = 2;
        edge = cameraDepth.Sample(smp, o.uv).r * 4 -
                    cameraDepth.Sample(smp, o.uv + float2(-dx * lineWeight, 0)).r -
                    cameraDepth.Sample(smp, o.uv + float2(dx * lineWeight, 0)).r -
                    cameraDepth.Sample(smp, o.uv + float2(0, -dy * lineWeight)).r -
                    cameraDepth.Sample(smp, o.uv + float2(0, dy * lineWeight)).r;
        edge = 1.0f - step(0.00025, edge);
    }
    //法線による輪郭線
    float b = 1;
    if (normalOutLine == 1)
    {
        float weight = 4;
        float4 ret = normal.Sample(smp, o.uv);
        ret = ret * 4 -
                normal.Sample(smp, o.uv + float2(-dx / weight, 0)) -
                normal.Sample(smp, o.uv + float2(dx / weight, 0)) -
                normal.Sample(smp, o.uv + float2(0, -dy / weight)) -
                normal.Sample(smp, o.uv + float2(0, dy / weight));
        b = dot(float3(0.298912f, 0.586611f, 0.114478f), 1 - ret.rgb);
        b = pow(b, 2);
    }
    
    //ブルーム
    float4 bloomAccum = GaussianFilter(bright,smp,o.uv,dx,dy);
    float2 uvSize = float2(1.0f, 0.5f);
    float2 uvOffset = float2(0, 0);
    for (int i = 0; i < 4;++i)
    {
        bloomAccum += GaussianFilter(bloom, smp, o.uv * uvSize + uvOffset, dx, dy);
        uvOffset.y += uvSize.y;
        uvSize /= 2;
    }
    if(bloomFlag!=1)
    {
        bloomAccum = float4(0, 0, 0, 0);
    }
    
    //被写界深度
    float depthDiff = abs(cameraDepth.Sample(smp, float2(0.5, 0.5)) - cameraDepth.Sample(smp, o.uv));
    depthDiff = pow(depthDiff, 0.5f);
    uvSize = float2(1, 0.5);
    uvOffset = float2(0, 0);
    float t = depthDiff * 8;
    float num;
    t = modf(t, num);
    float4 retColor[2];
    retColor[0] = color; // 通常テクスチャ
    if(num==0.0f)
    {
        retColor[1] = GaussianFilter(dof, smp, o.uv * uvSize + uvOffset, dx, dy);
    }
    else
    {
        for (int i = 1; i <= 8;++i)
        {
            if(i-num<0)
            {
                continue;
            }
            retColor[i - num] = GaussianFilter(dof, smp, o.uv * uvSize + uvOffset, dx, dy);
            uvOffset.y += uvSize.y;
            uvSize /= 2;
            if(i-num>1)
            {
                break;
            }
        }
    }
    if(dofFlag==1)
    {
        color = lerp(retColor[0], retColor[1], t);
    }
    
    
    //レイマーチング
    if(basenorm.a==0)
    {
        float2 aspect = float2(w / h, 1);
        float rate = 0.8;
        float3 eye = float3(0, 0, -2.5 /*- rate * time*/);
        float2 tmp = o.tpos.xy * aspect;
        float3 tpos = float3(tmp.x, tmp.y, 0/*-rate * time*/);
        float3 ray = normalize(tpos - eye);
        float3 light = float3(1, 1, -1);
        return RayMarching(eye, ray, light, 2);
    }
    
    return float4(color.rgb * edge * b + bloomAccum.rgb, color.a);
}
