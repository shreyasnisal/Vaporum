struct vs_input_t
{
	float3 localPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct v2p_t
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD;
};

struct BlurSample
{
	float2 Offset;
	float Weight;
	float padding;
};

static const int MaxSamples = 64;

cbuffer BlurConstants : register(b5)
{
	float2 TexelSize;
	float LerpT;
	int NumSamples;
	BlurSample Samples[MaxSamples];
};


Texture2D downTexture : register(t0);
Texture2D upTexture : register(t1);
SamplerState sourceSampler : register(s0);

v2p_t VertexMain(vs_input_t input)
{
	v2p_t v2p;
	v2p.position = float4(input.localPosition, 1);
	v2p.uv = input.uv;

	return v2p;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
    float4 downColor = downTexture.Sample(sourceSampler, input.uv);
    float4 upColor = float4(0.f, 0.f, 0.f, 1.f);
    for (int sampleIndex = 0; sampleIndex < NumSamples; sampleIndex++)
    {
        upColor.rgb += upTexture.Sample(sourceSampler, input.uv + Samples[sampleIndex].Offset * TexelSize).rgb * Samples[sampleIndex].Weight;
    }
    float4 color = lerp(downColor, upColor, LerpT);
	return color;
}