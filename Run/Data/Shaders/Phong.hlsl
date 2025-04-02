//------------------------------------------------------------------------------------------------
struct vs_input_t
{
	float3 localPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float3 localTangent : TANGENT;
	float3 localBitangent : BITANGENT;
	float3 localNormal : NORMAL;
};

//------------------------------------------------------------------------------------------------
struct v2p_t
{
	float4 position : SV_Position;
	float4 worldPosition : WORLD_POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float4 tangent : TANGENT;
	float4 bitangent : BITANGENT;
	float4 normal : NORMAL;
};

struct ps_output_t
{
	float4 colorRenderTarget : SV_Target0;
	float4 emissiveRenderTarget : SV_Target1;
};

//------------------------------------------------------------------------------------------------
cbuffer LightConstants : register(b1)
{
	float3 SunDirection;
	float SunIntensity;
	float AmbientIntensity;
	float3 padding0;
	float4x4 LightViewMatrix;
	float4x4 LightProjectionMatrix;
	float3 worldEyePosition;
	float minimumFalloff;
	float maximumFalloff;
	float minimumFalloffMultiplier;
	float maximumFalloffMultiplier;
	float padding1;

	int renderAmbientDebugFlag;
	int renderDiffuseFlag;
	int renderSpecularDebugFlag;
	int renderEmissiveDebugFlag;
	int useDiffuseMapDebugFlag;
	int useNormalMapDebugFlag;
	int useSpecularMapDebugFlag;
	int useGlossinessMapDebugFlag;
	int useEmissiveMapDebugFlag;
	float3 padding2;
};

//------------------------------------------------------------------------------------------------
cbuffer CameraConstants : register(b2)
{
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
};

//------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b3)
{
	float4x4 ModelMatrix;
	float4 ModelColor;
};

//------------------------------------------------------------------------------------------------
Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D specGlosEmitTexture : register(t2);

//------------------------------------------------------------------------------------------------
SamplerState diffuseSampler : register(s0);

//------------------------------------------------------------------------------------------------
v2p_t VertexMain(vs_input_t input)
{
	float4 localPosition = float4(input.localPosition, 1);
	float4 worldPosition = mul(ModelMatrix, localPosition);
	float4 viewPosition = mul(ViewMatrix, worldPosition);
	float4 clipPosition = mul(ProjectionMatrix, viewPosition);

	float4 localTangent = float4(input.localTangent, 0);
	float4 worldTangent = mul(ModelMatrix, localTangent);

	float4 localBitangent = float4(input.localBitangent, 0);
	float4 worldBitangent = mul(ModelMatrix, localBitangent);

	float4 localNormal = float4(input.localNormal, 0);
	float4 worldNormal = mul(ModelMatrix, localNormal);

	v2p_t v2p;
	v2p.position = clipPosition;
	v2p.worldPosition = worldPosition;
	v2p.color = input.color;
	v2p.uv = input.uv;
	v2p.tangent = worldTangent;
	v2p.bitangent = worldBitangent;
	v2p.normal = worldNormal;
	return v2p;
}

//------------------------------------------------------------------------------------------------
ps_output_t PixelMain(v2p_t input)
{
	float ambient = AmbientIntensity * renderAmbientDebugFlag;
	float4 vertexColor = input.color;
	float4 modelColor = ModelColor;
	float3 vertexWorldNormal = normalize(input.normal.xyz);
	float3 pixelWorldNormal = vertexWorldNormal;

	if (useNormalMapDebugFlag)
	{
		float3 tangentNormal = 2.f * normalTexture.Sample(diffuseSampler, input.uv).rgb - 1.f;
		float3x3 tbnMatrix = float3x3(normalize(input.tangent.xyz), normalize(input.bitangent.xyz), normalize(input.normal.xyz));
		pixelWorldNormal = mul(tangentNormal, tbnMatrix);
	}

	float vnDotL = dot(vertexWorldNormal, -SunDirection);
	float nDotL = dot(pixelWorldNormal, -SunDirection);

	float falloff = clamp(vnDotL, minimumFalloff, maximumFalloff);
	float falloffT = (falloff - minimumFalloff) / (maximumFalloff - minimumFalloff);
	float falloffMultiplier = lerp(minimumFalloffMultiplier, maximumFalloffMultiplier, falloffT);

	float4 textureColor = float4(1.f, 1.f, 1.f, 1.f);
	if (useDiffuseMapDebugFlag)
	{
		textureColor = diffuseTexture.Sample(diffuseSampler, input.uv);
	}
	float diffuse = SunIntensity * falloffMultiplier * saturate(nDotL) * renderDiffuseFlag;
	
	// Specular
	float specularIntensity = 0.f;
	float specularPower = 1.f;
	float emissiveIntensity = 0.f;
	float4 specGlosEmit = float4(0.f, 0.f, 0.f, 1.f);
	if (useSpecularMapDebugFlag)
	{
		specGlosEmit = specGlosEmitTexture.Sample(diffuseSampler, input.uv);
		specularIntensity = specGlosEmit.r;
		specularPower = 1.f + (specGlosEmit.g * 31.f);
	}
	float3 worldViewDirection = normalize(worldEyePosition - input.worldPosition.xyz);
	float3 worldHalfVector = normalize(-SunDirection + worldViewDirection);
	float nDotH = saturate(dot(pixelWorldNormal, worldHalfVector));
	float specular = specularIntensity * falloffMultiplier * pow(nDotH, specularPower) * renderSpecularDebugFlag;

	if (useEmissiveMapDebugFlag)
	{
		emissiveIntensity = specGlosEmit.b;
	}
	float emissive = emissiveIntensity * renderEmissiveDebugFlag;

	float4 direct = float4((ambient + diffuse + specular + emissive).xxx, 1);
	
	ps_output_t psOut;
	psOut.colorRenderTarget = direct * textureColor * vertexColor * modelColor;
	clip(psOut.colorRenderTarget.a - 0.01f);
	psOut.emissiveRenderTarget = float4(emissive.xxx, 1.f) * textureColor * vertexColor * modelColor;

	return psOut;
}
