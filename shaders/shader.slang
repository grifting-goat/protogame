struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD0;
};

struct VertexOutput
{
    float2 texCoord    : TEXCOORD0;
    float4 sv_position : SV_Position;
};

cbuffer Transforms : register(b0)
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

cbuffer Material : register(b1)
{
    float3 objectColor;
    bool   useTexture;
};

Texture2D    texSampler      : register(t0);
SamplerState texSamplerState : register(s0);

[shader("vertex")]
VertexOutput vertMain(VertexInput input)
{
    VertexOutput output;
    output.sv_position = mul(projection, mul(view, mul(model, float4(input.position, 1.0))));
    output.texCoord    = input.texCoord;
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target
{
    if (useTexture)
    {
        float4 texColor = texSampler.Sample(texSamplerState, input.texCoord);
        return texColor * float4(objectColor, 1.0);
    }
    else
    {
        return float4(objectColor, 1.0);
    }
}