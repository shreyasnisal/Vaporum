#pragma once

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include <string>
#include <map>

class Texture;

class Particle
{
public:
	~Particle() = default;
	Particle(
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
				float endSpeedTime
	);

	void Update(float deltaSeconds);
	void Render(Camera const& camera) const;

	static void InitializeParticleTextures();

public:
	Vec3 m_position = Vec3::ZERO;
	Vec3 m_velocity = Vec3::ZERO;
	float m_size = 0.f;
	Rgba8 m_color = Rgba8::WHITE;
	unsigned char m_opacity = 255;
	Texture* m_texture = nullptr;
	Stopwatch m_lifetimeTimer;
	bool m_isDestroyed = false;
	float m_rotation = 0.f;
	float m_rotationSpeed = 0.f;
	float m_startAlpha = 1.f;
	float m_endAlpha = 0.f;
	float m_startAlphaTime = 0.f;
	float m_endAlphaTime = 1.f;
	float m_startScale = 1.f;
	float m_endScale = 1.f;
	float m_startScaleTime = 0.f;
	float m_endScaleTime = 1.f;
	float m_startSpeedMultiplier = 1.f;
	float m_endSpeedMultiplier = 1.f;
	float m_startSpeedTime = 0.f;
	float m_endSpeedTime = 1.f;
	float m_scale = 1.f;
	float m_speedMultiplier = 1.f;

	static std::map<std::string, Texture*> s_particleTextures;
};
