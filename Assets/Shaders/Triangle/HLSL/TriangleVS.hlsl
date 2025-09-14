
struct VSInput
{
    float3 Pos : POSITION;
    float3 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

// MVP变换矩阵
cbuffer MVPBuffer : register(b0, space0)
{
    float4x4 Model;
    float4x4 View;
    float4x4 Projection;
}

VSOutput Main(VSInput Input)
{
    VSOutput output;
    float4 WorldPos = mul(Model, float4(Input.Pos, 1.0));
    float4 ViewPos = mul(View, WorldPos);
    float4 HomogeneousPos = mul(Projection, ViewPos); // 齐次坐标
    output.Pos = HomogeneousPos;
    output.Color = Input.Color;
    output.TexCoord = Input.TexCoord;
    return output;
}