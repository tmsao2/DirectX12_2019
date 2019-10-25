Texture2D<float4> tex:register(t0);
Texture2D<float4> sph : register(t1);
Texture2D<float4> spa : register(t2);
Texture2D<float4> toon : register(t3);

SamplerState smp : register(s0);
SamplerState smpToon : register(s1);

cbuffer mat : register(b0)
{
    float4x4 world;
    float4x4 view;
    float4x4 projection;
    float3 eye;
};

cbuffer material : register(b1)
{
    float4 diffuse;
    float power;
    float3 specular;
    float3 ambient;
}

//cbuffer bones : register(b2)
//{
//    float4x4 boneMats[512];
//}

struct Out {
	float4 svpos    : SV_POSITION;
	float4 pos      : POSITION;
    float4 normal   : NORMAL;
	float2 uv       :TEXCOORD;
    //min16uint2 boneno : BONENO;
    //min16uint weight : WEIGHT;
};
//���_�V�F�[�_
Out BasicVS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD
            /*min16uint2 boneno : BONENO,min16uint weight:WEIGHT*/)
{
	Out o;

    //float w = weight / 100.f;
    //matrix m = boneMats[boneno.x] * w + boneMats[boneno.y] * (1 - w);
    //pos = mul(m, pos);
    float4 worldPos     = mul(world, pos);
    float4 viewPos      = mul(view, worldPos);
    float4 projPos      = mul(projection, viewPos);

    o.svpos     = projPos;
    o.pos       = worldPos;
    o.normal    = mul(world, normal);
	o.uv        = uv;
    //o.boneno    = boneno;
	return o;
}
//�s�N�Z���V�F�[�_
float4 BasicPS(Out o) :SV_Target
{
    float3 light = normalize(float3(-1, 1, -1));
    float3 eyeray = normalize(o.pos.xyz - eye);
    float brightness = dot(light, o.normal.xyz);
    float4 toonBright = toon.Sample(smpToon, float2(0, 1.0 - brightness));
    float3 rlight = reflect(light, o.normal.xyz);

    float spec = 0.0f;
    if(spec!=0)
    {
        spec = pow(saturate(dot(rlight, -eyeray)), power);
    }
    float2 normalUV = (o.normal.xy + float2(1, -1)) * float2(0.5, -0.5);
    float4 color = tex.Sample(smp, o.uv);
    return saturate(toonBright
                * diffuse
                * color
                * sph.Sample(smp, normalUV)
                + saturate(spa.Sample(smp, normalUV) * color)
                + float4(specular * spec, 1)
                + float4(ambient * color.rgb * 0.2, 1));

    //�e�N�X�`��
    return tex.Sample(smp, o.uv);
    //����
	float t =  tex.Sample(smp,o.uv).r;
	return float4(t,t,t,1);
}