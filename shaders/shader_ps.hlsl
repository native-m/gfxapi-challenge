struct PSInput
{
    float4 pos : SV_POSITION;
    float3 col : COLOR0;
};

float4 main(PSInput input) : SV_Target0
{
    return float4(input.col, 1.0f);
}
