
//struct VSInput
//{
//    float3 Pos : POSITION0;
//    float3 Color : COLOR0;
//};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Color : COLOR0;
};

static const float2 Position[3] = {
    float2(0.0f, -0.5f),
    float2(0.5f, 0.5f),
    float2(-0.5f, 0.5f)
};

static const float3 Color[3] = {
    float3(1.0f, 0.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 0.0f, 1.0f)
};

VSOutput Main(uint index : SV_VertexID)
{
    VSOutput output = (VSOutput)0;
    output.Pos = float4(Position[index], 0.0f, 1.0f);
    output.Color = Color[index];
    return output;
}