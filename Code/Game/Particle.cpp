#include "Particle.hpp"

#include "Game/GameCommon.hpp"

#include "Engine/Renderer/VertexBuffer.hpp"


std::map<std::string, Texture*> Particle::s_particleTextures;


Particle::Particle(
	Vec3 const& startPos,
	Vec3 const& velocity,
	float rotation,
	float rotationSpeed,
	float size,
	Clock* parentClock,
	float lifetime,
	std::string const& textureName,
	Rgba8 const& color,
	float startAlpha,
	float endAlpha,
	float startAlphaTime,
	float endAlphaTime,
	float startScale,
	float endScale,
	float startScaleTime,
	float endScaleTime,
	float startSpeed,
	float endSpeed,
	float startSpeedTime,
	float endSpeedTime)
	: m_position(startPos)
	, m_velocity(velocity)
	, m_size(size)
	, m_lifetimeTimer(parentClock, lifetime)
	, m_color(color)
	, m_rotation(rotation)
	, m_rotationSpeed(rotationSpeed)
	, m_startAlpha(startAlpha)
	, m_endAlpha(endAlpha)
	, m_startAlphaTime(startAlphaTime)
	, m_endAlphaTime(endAlphaTime)
	, m_startScale(startScale)
	, m_endScale(endScale)
	, m_startScaleTime(startScaleTime)
	, m_endScaleTime(endScaleTime)
	, m_startSpeedMultiplier(startSpeed)
	, m_endSpeedMultiplier(endSpeed)
	, m_startSpeedTime(startSpeedTime)
	, m_endSpeedTime(endSpeedTime)
{
	m_lifetimeTimer.Start();

	auto particleTexturesIter = s_particleTextures.find(textureName);
	if (particleTexturesIter != s_particleTextures.end())
	{
		m_texture = particleTexturesIter->second;
	}
}

void Particle::Update(float deltaSeconds)
{
	if (m_lifetimeTimer.HasDurationElapsed())
	{
		m_isDestroyed = true;
		return;
	}

	//m_opacity = DenormalizeByte(Interpolate(1.f, 0.f, m_lifetimeTimer.GetElapsedFraction()));
	m_opacity = DenormalizeByte(RangeMapClamped(m_lifetimeTimer.GetElapsedFraction(), m_startAlphaTime, m_endAlphaTime, m_startAlpha, m_endAlpha));
	m_scale = RangeMapClamped(m_lifetimeTimer.GetElapsedFraction(), m_startScaleTime, m_endScaleTime, m_startScale, m_endScale);
	m_speedMultiplier = RangeMapClamped(m_lifetimeTimer.GetElapsedFraction(), m_startSpeedTime, m_endSpeedTime, m_startSpeedMultiplier, m_endSpeedMultiplier);

	m_position += m_velocity * m_speedMultiplier * deltaSeconds;
	m_rotation += m_rotationSpeed * deltaSeconds;
}

void Particle::Render(Camera const& camera) const
{
	std::vector<Vertex_PCU> particleVerts;
	AddVertsForQuad3D(particleVerts, Vec3(0.f, -m_size * m_scale * 0.5f, -m_size * m_scale * 0.5f), Vec3(0.f, m_size * m_scale * 0.5f, -m_size * m_scale * 0.5f), Vec3(0.f, m_size * m_scale * 0.5f, m_size * m_scale * 0.5f), Vec3(0.f, -m_size * m_scale * 0.5f, m_size * m_scale * 0.5f), Rgba8::WHITE);

	Mat44 billboardMatrix = GetBillboardMatrix(BillboardType::FULL_OPPOSING, camera.GetModelMatrix(), m_position);
	billboardMatrix.AppendXRotation(m_rotation);

	g_renderer->SetDepthMode(DepthMode::DISABLED);
	//g_theRenderer->SetDepthMode(DepthMode::READ_ONLY_LESS_EQUAL);
	g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_BACK);
	g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
	g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_renderer->SetBlendMode(BlendMode::ALPHA);
	g_renderer->BindTexture(m_texture);
	g_renderer->BindShader(nullptr);
	g_renderer->SetModelConstants(billboardMatrix, Rgba8(m_color.r, m_color.g, m_color.b, m_opacity));
	g_renderer->DrawVertexArray(particleVerts);
}

void Particle::InitializeParticleTextures()
{
	Texture* texture = g_renderer->CreateOrGetTextureFromFile("Data/Images/Particles/Smoke01.png");
	s_particleTextures["Light Smoke"] = texture;

	texture = g_renderer->CreateOrGetTextureFromFile("Data/Images/Particles/Smoke02.png");
	s_particleTextures["Dark Smoke"] = texture;

	texture = g_renderer->CreateOrGetTextureFromFile("Data/Images/Particles/Fire01.png");
	s_particleTextures["Muzzle Flash 1"] = texture;

	texture = g_renderer->CreateOrGetTextureFromFile("Data/Images/Particles/Fire02.png");
	s_particleTextures["Muzzle Flash 2"] = texture;
}
