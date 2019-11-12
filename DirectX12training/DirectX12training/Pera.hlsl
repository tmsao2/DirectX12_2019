Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);
struct Out
{
    float4 svpos : SV_POSITION;
    float4 pos : POSITION;
    float2 uv : TEXCOORD;
};

//���_�V�F�[�_
Out BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD)
{
    Out o;
    o.svpos = pos;
    o.pos = pos;
    o.uv = uv;
    return o;
}
//�s�N�Z���V�F�[�_
float4 BasicPS(Out o) : SV_Target
{
    //�e�N�X�`��
    return tex.Sample(smp, o.uv);
    //����
    float t = tex.Sample(smp, o.uv).r;
    return float4(t, t, t, 1);
}