SamplerState smp : register(s0);

cbuffer mat : register(b0)
{
    float4x4 world;
    float4x4 view;
    float4x4 projection;
    float4x4 lvp;
    float3 eye;
};

cbuffer bones : register(b2)
{
    float4x4 boneMats[512];
}

struct Out
{
    float4 svpos : SV_POSITION;
    float4 pos : POSITION;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
    min16uint2 boneno : BONENO;
    min16uint weight : WEIGHT;
};

//頂点シェーダ
Out ShadowVS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONENO, min16uint weight : WEIGHT)
{
    Out o;

    float w = weight / 100.f;
    matrix m = boneMats[boneno.x] * w + boneMats[boneno.y] * (1 - w);
    pos = mul(m, pos);
    
    float4 wPos = mul(world, pos);
    float4 lvpPos = mul(lvp, wPos);

    o.svpos = lvpPos;
    o.normal = mul(world, normal);
    o.uv = uv;
    return o;
}
//ピクセルシェーダ
float4 ShadowPS(Out o) : SV_Target
{
    return float4(1, 1, 1, 1);
}