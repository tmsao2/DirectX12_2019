Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);
struct Out
{
    float4 svpos : SV_POSITION;
    float4 pos : POSITION;
    float2 uv : TEXCOORD;
};

//頂点シェーダ
Out BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD)
{
    Out o;
    o.svpos = pos;
    o.pos = pos;
    o.uv = uv;
    return o;
}
//ピクセルシェーダ
float4 BasicPS(Out o) : SV_Target
{
    //テクスチャ
    return tex.Sample(smp, o.uv);
    //白黒
    float t = tex.Sample(smp, o.uv).r;
    return float4(t, t, t, 1);
}