Texture2D<float> depth : register(t0);

SamplerState smp : register(s0);

cbuffer mat : register(b0)
{
    float4x4 world;
    float4x4 view;
    float4x4 projection;
    float4x4 lvp;
    float3 eye;
};

struct Output
{
    float4 svpos : SV_POSITION;
    float4 pos : POSITION;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};

Output PrimitiveVS(float4 pos : POSITION, float4 normal : NORMAL,float2 uv : TEXCOORD)
{
    Output o;
    float4 wPos = mul(world, pos);
    float4 wvPos = mul(view, wPos);
    float4 wvpPos = mul(projection, wvPos);
    o.svpos = wvpPos;
    o.pos = wPos;
    o.normal = mul(world, normal);
    o.uv = uv;
    return o;
}

float4 PrimitivePS(Output o) : SV_Target
{
    float4 color=float4(1, 1, 1, 1);
    float4 tpos = mul(lvp, o.pos);
    float2 uv = (float2(1, -1) + tpos.xy) * float2(0.5, -0.5);

    if (tpos.z > depth.Sample(smp, uv))
    {
        color.rgb *= 0.5f;
    }
    return color;
}