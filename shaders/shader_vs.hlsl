struct VSInput
{
    float4 pos : POSITION0;
    float3 col : COLOR0;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 col : COLOR0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos = input.pos;
    output.col = input.col;
    return output;
}
