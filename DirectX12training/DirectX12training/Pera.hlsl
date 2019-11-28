Texture2D<float4>tex : register(t0); // 通常テクスチャ
Texture2D<float4> depth : register(t1);
Texture2D<float4> shadow : register(t2);

SamplerState smp : register(s0);

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

//頂点シェーダ
Output vs(float4 pos : POSITION, float2 uv : TEXCOORD)
{
    Output output;
    output.svpos = pos;
    output.uv = uv;
    return output;
}

//ピクセルシェーダ
float4 ps(Output o) : SV_Target
{
    float dep = depth.Sample(smp, o.uv * 5);
    dep = pow(dep, 100);
    if (o.uv.x <= 0.2 && o.uv.y <= 0.2)
    {
        return 1 - float4(dep, dep, dep, 1);
    }

    float sha = shadow.Sample(smp, o.uv * 5);
    sha = pow(sha, 100);
    if (o.uv.x <= 0.2 && o.uv.y <= 0.4)
    {
        return 1 - float4(sha, sha, sha, 1);
    }

    return tex.Sample(smp, o.uv);

    float w, h, miplevel;
    tex.GetDimensions(0, w, h, miplevel);
    float dx = 1.0f / w;
    float dy = 1.0f / h;
    float4 color = tex.Sample(smp, o.uv);
    float4 ret = tex.Sample(smp, o.uv);
    ret = ret * 4 -
                tex.Sample(smp, o.uv + float2(-dx, 0)) -
                tex.Sample(smp, o.uv + float2(dx, 0)) -
                tex.Sample(smp, o.uv + float2(0, -dy)) -
                tex.Sample(smp, o.uv + float2(0, dy));

    float b = dot(float3(0.298912f, 0.586611f, 0.114478f), 1-ret.rgb);
    b = pow(b, 4);

    return float4(b, b, b, 1);

    return float4(color.r - fmod(color.r, 0.2f), color.g - fmod(color.g, 0.5f), color.b - fmod(color.b, 0.2f), 1.0f);
}
