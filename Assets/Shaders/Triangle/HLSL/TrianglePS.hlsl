
Texture2D<float4> Texture1 : register(t1, space0);
SamplerState Sampler1 : register(s1, space0);

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

float4 Main(VSOutput Input) : SV_TARGET
{
    float4 Color = Texture1.Sample(Sampler1, Input.TexCoord);
    return Color;
}