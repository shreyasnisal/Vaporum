#include "Game/Unit.hpp"

#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/Player.hpp"
#include "Game/UnitDefinition.hpp"

#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Models/Model.hpp"
#include "Engine/Core/Models/ModelLoader.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"


Unit::Unit(UnitDefinition const& definition, Map* map, IntVec2 const& tileCoords, Vec3 const& position, EulerAngles const& orientation, Player* owner)
	: m_definition(definition)
	, m_map(map)
	, m_tileCoords(tileCoords)
	, m_previousTileCoords(tileCoords)
	, m_currentTileCoords(tileCoords)
	, m_position(position)
	, m_orientation(orientation)
	, m_defaultOrientation(orientation)
	, m_owner(owner)
	, m_health(m_definition.m_maxHealth)
{
	m_heatMap = new TileHeatMap(m_map->m_definition.m_dimensions);
	std::vector<float> distanceField;
	m_map->PopuplateDistanceField(distanceField, m_tileCoords);
	m_heatMap->SetAllValues(distanceField);
}

void Unit::Update()
{
	float deltaSeconds = m_map->m_game->m_gameClock.GetDeltaSeconds();

	if (this->m_isSelected && (m_owner->m_turnState == TurnState::UNIT_SELECTED_MOVE))
	{
		IntVec2 const& hoveredTileCoords = m_map->m_hoveredTile;
		Vec2 const& hoveredTilePosition = m_map->GetTileWorldPositionFromCoordinates(hoveredTileCoords);
		Vec2 directionToHoveredTile = hoveredTilePosition - m_position.GetXY();
		m_orientation.m_yawDegrees = GetTurnedTowardDegrees(m_orientation.m_yawDegrees, directionToHoveredTile != Vec2::ZERO ? directionToHoveredTile.GetOrientationDegrees() : m_defaultOrientation.m_yawDegrees, deltaSeconds * TURN_SPEED_PER_SECOND);
	}
	else if (m_pathSpline)
	{
		Vec3 newPosition = m_pathSpline->EvaluateAtParametric(m_movementTimer->GetElapsedTime() * m_pathLength).ToVec3();
		if (newPosition != m_position)
		{
			m_orientation.m_yawDegrees = (newPosition - m_position).GetAngleAboutZDegrees();
		}
		m_position = newPosition;
		if (m_movementTimer->HasDurationElapsed())
		{
			m_movementTimer->Stop();
			delete m_movementTimer;
			m_movementTimer = nullptr;
			delete m_pathSpline;
			m_pathSpline = nullptr;

			m_owner->m_game->m_isAnimationPlaying = false;
		}
	}
	else if (this->m_isSelected && m_owner->m_turnState == TurnState::UNIT_SELECTED_ATTACK)
	{
		IntVec2 const& hoveredTileCoords = m_map->m_hoveredTile;
		Vec2 const& hoveredTilePosition = m_map->GetTileWorldPositionFromCoordinates(hoveredTileCoords);
		Vec2 directionToHoveredTile = hoveredTilePosition - m_position.GetXY();
		m_orientation.m_yawDegrees = GetTurnedTowardDegrees(m_orientation.m_yawDegrees, directionToHoveredTile != Vec2::ZERO ? directionToHoveredTile.GetOrientationDegrees() : m_defaultOrientation.m_yawDegrees, deltaSeconds * TURN_SPEED_PER_SECOND);

		Player const* waitingPlayer = m_owner->m_game->GetWaitingPlayer();
		if (waitingPlayer)
		{
			Unit* targetUnit = waitingPlayer->GetUnitFromTileCoords(hoveredTileCoords);
			if (targetUnit)
			{
				Vec3 directionToSelectedUnit = m_position - targetUnit->m_position;
				targetUnit->m_orientation.m_yawDegrees = GetTurnedTowardDegrees(targetUnit->m_orientation.m_yawDegrees, directionToSelectedUnit.GetAngleAboutZDegrees(), TURN_SPEED_PER_SECOND * deltaSeconds);
			}
		}
	}
	//else
	//{
	//	m_orientation.m_yawDegrees = GetTurnedTowardDegrees(m_orientation.m_yawDegrees, m_defaultOrientation.m_yawDegrees, TURN_SPEED_PER_SECOND * deltaSeconds);
	//}

	if (m_floatingDamageTimer && m_floatingDamageTimer->HasDurationElapsed())
	{
		m_floatingDamageStr = "";
		delete m_floatingDamageTimer;
		m_floatingDamageTimer = nullptr;
	}
}

void Unit::Render() const
{
	Mat44 transform = Mat44::CreateTranslation3D(m_position);
	transform.Append(m_orientation.GetAsMatrix_iFwd_jLeft_kUp());

	Rgba8 modelColor = m_owner->GetTeamColor();
	if (m_isSelected)
	{
		modelColor.MultiplyRGBScaled(Rgba8::WHITE, 0.75f);
	}
	if (m_ordersIssued)
	{
		modelColor.MultiplyRGBScaled(Rgba8::WHITE, 0.5f);
	}

	g_renderer->SetBlendMode(BlendMode::OPAQUE);
	g_renderer->SetDepthMode(DepthMode::ENABLED);
	g_renderer->SetModelConstants(transform, modelColor);
	g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_BACK);
	g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
	g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_renderer->BindTexture(nullptr);
	g_renderer->BindShader(m_map->m_definition.m_shader);
	g_renderer->DrawVertexBuffer(m_definition.m_model->GetVertexBuffer(), m_definition.m_model->GetVertexCount());

	Vec3 sunDirection = m_owner->m_game->m_sunOrientation.GetAsMatrix_iFwd_jLeft_kUp().GetIBasis3D();
	if (sunDirection.z < 0.f)
	{
		Vec3 groundIBasis(1.f, 0.f, 0.f);
		Vec3 groundJBasis(0.f, 1.f, 0.f);
		Vec3 groundKBasis(-(sunDirection.x / sunDirection.z), -(sunDirection.y / sunDirection.z), 0.f);
		Vec3 groundTranslation(0.f, 0.f, 0.001f);
		Mat44 shadowMatrix(groundIBasis, groundJBasis, groundKBasis, groundTranslation);
		shadowMatrix.Append(transform);

		Rgba8 shadowColor(0, 0, 0, 195);

		g_renderer->SetBlendMode(BlendMode::ALPHA);
		g_renderer->SetDepthMode(DepthMode::READ_ONLY_LESS_EQUAL);
		g_renderer->SetModelConstants(shadowMatrix, shadowColor);
		g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_BACK);
		g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
		g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_renderer->BindTexture(nullptr);
		g_renderer->BindShader(m_map->m_definition.m_shader);
		g_renderer->DrawVertexBuffer(m_definition.m_model->GetVertexBuffer(), m_definition.m_model->GetVertexCount());
	}

	if (m_floatingDamageTimer && !m_floatingDamageTimer->HasDurationElapsed())
	{
		std::vector<Vertex_PCU> floatingDamageVerts;
		Vec3 floatingDamagePosition = Interpolate(m_position + Vec3::SKYWARD * 0.4f, m_position + Vec3::SKYWARD * 0.8f, m_floatingDamageTimer->GetElapsedFraction());
		Rgba8 floatingDamageColor = Interpolate(Rgba8::RED, Rgba8(255, 0, 0, 0), m_floatingDamageTimer->GetElapsedFraction());
		g_butlerFont->AddVertsForText3D(floatingDamageVerts, Vec2::ZERO, 0.4f, m_floatingDamageStr, Rgba8::WHITE, 0.5f);
		Mat44 floatingDamageTransform = GetBillboardMatrix(BillboardType::FULL_OPPOSING, m_map->m_game->m_worldCamera.GetModelMatrix(), floatingDamagePosition);

		g_renderer->SetBlendMode(BlendMode::ALPHA);
		g_renderer->SetDepthMode(DepthMode::DISABLED);
		g_renderer->SetModelConstants(floatingDamageTransform, floatingDamageColor);
		g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_NONE);
		g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
		g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_renderer->BindTexture(g_butlerFont->GetTexture());
		g_renderer->BindShader(nullptr);
		g_renderer->DrawVertexArray(floatingDamageVerts);
	}
}

void Unit::Move(IntVec2 const& newTileCoords)
{
	if (m_didMove)
	{
		return;
	}
	
	m_didMove = true;
	m_tileCoords = newTileCoords;

	std::vector<Vec2> path;
	m_map->GenerateHeatMapPath(path, m_tileCoords, m_previousTileCoords, m_heatMap);
	m_pathLength = (int)path.size();

	m_pathSpline = new CatmullRomSpline(path);
	m_movementTimer = new Stopwatch(&m_map->m_game->m_gameClock, 1.f);
	m_movementTimer->Start();
	m_owner->m_game->m_isAnimationPlaying = true;

	std::vector<float> newDistanceField;
	m_map->PopuplateDistanceField(newDistanceField, m_tileCoords);
	m_heatMap->SetAllValues(newDistanceField);
	//m_position = m_map->GetTileWorldPositionFromCoordinates(newTileCoords).ToVec3();
}

void Unit::Attack(Unit* targetUnit)
{
	int damage = (int)(2.f * m_definition.m_attackDamage / targetUnit->m_definition.m_defense);
	Vec3 hitDirection = (targetUnit->m_position - m_position).GetNormalized();


	Vec3 unitFwd, unitLeft, unitUp;
	m_orientation.GetAsVectors_iFwd_jLeft_kUp(unitFwd, unitLeft, unitUp);
	//---------------------------------------------------------------------------------------
	// Muzzle Flash Effect
	//---------------------------------------------------------------------------------------
	// Spawn 12 light smoke particles
	for (int particleIndex = 0; particleIndex < 6; particleIndex++)
	{
		float particleSize = g_RNG->RollRandomFloatInRange(0.25f, 0.75f);
		float particleLifetime = g_RNG->RollRandomFloatInRange(1.5f, 2.5f);
		float particleRotation = g_RNG->RollRandomFloatInRange(0.f, 360.f);
		float particleRotationSpeed = g_RNG->RollRandomFloatInRange(15.f, 45.f);
		Rgba8 const minColor(127, 127, 127, 255);
		Rgba8 const maxColor(255, 255, 255, 255);
		float colorLerpFraction = g_RNG->RollRandomFloatZeroToOne();
		Rgba8 particleColor = Interpolate(minColor, maxColor, colorLerpFraction);
		float spread = 30.f * 0.5f;
		EulerAngles randomDeviationAngles(g_RNG->RollRandomFloatInRange(-spread, spread), g_RNG->RollRandomFloatInRange(-spread, spread), 0.f);
		Vec3 particleVelocity = (hitDirection + randomDeviationAngles.GetAsMatrix_iFwd_jLeft_kUp().GetIBasis3D()) * g_RNG->RollRandomFloatInRange(0.25f, 0.5f);
		Vec3 particleOffset = m_definition.m_muzzleOffset;

		float particleStartAlpha = 1.f;
		float particleEndAlpha = 0.f;
		float startAlphaTime = 0.25f;
		float endAlphaTime = 1.f;
		float startScale = 1.f;
		float endScale = 4.f;
		float startScaleTime = 0.f;
		float endScaleTime = 1.f;
		float startSpeedMultiplier = 1.f;
		float endSpeedMultiplier = 0.25f;
		float startSpeedTime = 0.f;
		float endSpeedTime = 1.f;

		g_app->m_game->SpawnParticle(m_position + unitFwd * particleOffset.x + unitLeft * particleOffset.y + unitUp * particleOffset.z, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Light Smoke", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
	}
	// Spawn 12 dark smoke particles
	for (int particleIndex = 0; particleIndex < 12; particleIndex++)
	{
		float particleSize = g_RNG->RollRandomFloatInRange(0.25f, 0.5f);
		float particleLifetime = g_RNG->RollRandomFloatInRange(1.f, 2.f);
		float particleRotation = g_RNG->RollRandomFloatInRange(0.f, 360.f);
		float particleRotationSpeed = g_RNG->RollRandomFloatInRange(15.f, 45.f);
		Rgba8 const minColor(31, 31, 31, 255);
		Rgba8 const maxColor(63, 63, 63, 255);
		float colorLerpFraction = g_RNG->RollRandomFloatZeroToOne();
		Rgba8 particleColor = Interpolate(minColor, maxColor, colorLerpFraction);
		float spread = 10.f * 0.5f;
		EulerAngles randomDeviationAngles(g_RNG->RollRandomFloatInRange(-spread, spread), g_RNG->RollRandomFloatInRange(-spread, spread), 0.f);
		Vec3 particleVelocity =  (hitDirection + randomDeviationAngles.GetAsMatrix_iFwd_jLeft_kUp().GetIBasis3D()).GetNormalized() * g_RNG->RollRandomFloatInRange(0.25f, 0.5f);
		Vec3 particleOffset = m_definition.m_muzzleOffset;

		float particleStartAlpha = 1.f;
		float particleEndAlpha = 0.f;
		float startAlphaTime = 0.5f;
		float endAlphaTime = 1.f;
		float startScale = 1.f;
		float endScale = 1.25f;
		float startScaleTime = 0.f;
		float endScaleTime = 1.f;
		float startSpeedMultiplier = 1.f;
		float endSpeedMultiplier = 0.75f;
		float startSpeedTime = 0.f;
		float endSpeedTime = 1.f;

		g_app->m_game->SpawnParticle(m_position + unitFwd * particleOffset.x + unitLeft * particleOffset.y + unitUp * particleOffset.z, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Dark Smoke", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
	}
	// Spawn 3 type 1 fire particles
	for (int particleIndex = 0; particleIndex < 3; particleIndex++)
	{
		float particleSize = g_RNG->RollRandomFloatInRange(0.5f, 0.75f);
		float particleLifetime = g_RNG->RollRandomFloatInRange(1.f, 2.f);
		float particleRotation = 0.f;
		float particleRotationSpeed = 0.f;
		Rgba8 const minColor(200, 31, 31, 255);
		Rgba8 const maxColor(255, 127, 127, 255);
		float colorLerpFraction = g_RNG->RollRandomFloatZeroToOne();
		Rgba8 particleColor = Interpolate(minColor, maxColor, colorLerpFraction);
		Vec3 particleVelocity =  hitDirection * g_RNG->RollRandomFloatInRange(0.1f, 0.2f);
		Vec3 particleOffset = m_definition.m_muzzleOffset;

		float particleStartAlpha = 1.f;
		float particleEndAlpha = 0.f;
		float startAlphaTime = 0.75f;
		float endAlphaTime = 1.f;
		float startScale = 1.f;
		float endScale = 1.5f;
		float startScaleTime = 0.f;
		float endScaleTime = 1.f;
		float startSpeedMultiplier = 1.f;
		float endSpeedMultiplier = 1.f;
		float startSpeedTime = 0.f;
		float endSpeedTime = 1.f;

		g_app->m_game->SpawnParticle(m_position + unitFwd * particleOffset.x + unitLeft * particleOffset.y + unitUp * particleOffset.z, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Muzzle Flash 1", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
	}
	// Spawn 3 type 2 fire particles
	for (int particleIndex = 0; particleIndex < 3; particleIndex++)
	{
		float particleSize = g_RNG->RollRandomFloatInRange(0.5f, 0.75f);
		float particleLifetime = g_RNG->RollRandomFloatInRange(1.f, 2.f);
		float particleRotation = 0.f;
		float particleRotationSpeed = 0.f;
		Rgba8 const minColor(200, 31, 31, 255);
		Rgba8 const maxColor(255, 127, 127, 255);
		float colorLerpFraction = g_RNG->RollRandomFloatZeroToOne();
		Rgba8 particleColor = Interpolate(minColor, maxColor, colorLerpFraction);
		Vec3 particleVelocity = hitDirection * g_RNG->RollRandomFloatInRange(0.1f, 0.2f);
		Vec3 particleOffset = m_definition.m_muzzleOffset;

		float particleStartAlpha = 1.f;
		float particleEndAlpha = 0.f;
		float startAlphaTime = 0.75f;
		float endAlphaTime = 1.f;
		float startScale = 1.f;
		float endScale = 1.5f;
		float startScaleTime = 0.f;
		float endScaleTime = 1.f;
		float startSpeedMultiplier = 1.f;
		float endSpeedMultiplier = 1.f;
		float startSpeedTime = 0.f;
		float endSpeedTime = 1.f;

		g_app->m_game->SpawnParticle(m_position + unitFwd * particleOffset.x + unitLeft * particleOffset.y + unitUp * particleOffset.z, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Muzzle Flash 2", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
	}

	g_audio->StartSound(m_definition.m_fireSFX);
	m_previousTileCoords = m_tileCoords;
	targetUnit->TakeDamage(damage, hitDirection);
	m_isSelected = false;
	m_ordersIssued = true;

	// Target unit attacks back if possible
	int attackDistance = m_map->GetHexTaxicabDistance(targetUnit->m_tileCoords, m_tileCoords);
	if (attackDistance < targetUnit->m_definition.m_attackRange.m_min || attackDistance > targetUnit->m_definition.m_attackRange.m_max)
	{
		return;
	}
	int receivedDamage = (int)(2.f * targetUnit->m_definition.m_attackDamage / m_definition.m_defense);
	TakeDamage(receivedDamage, -hitDirection);

	Vec3 targetUnitFwd, targetUnitLeft, targetUnitUp;
	targetUnit->m_orientation.GetAsVectors_iFwd_jLeft_kUp(targetUnitFwd, targetUnitLeft, targetUnitUp);
	//---------------------------------------------------------------------------------------
	// Target Unit Muzzle Flash Effect
	//---------------------------------------------------------------------------------------
	// Spawn 12 light smoke particles
	for (int particleIndex = 0; particleIndex < 6; particleIndex++)
	{
		float particleSize = g_RNG->RollRandomFloatInRange(0.25f, 0.75f);
		float particleLifetime = g_RNG->RollRandomFloatInRange(1.5f, 2.5f);
		float particleRotation = g_RNG->RollRandomFloatInRange(0.f, 360.f);
		float particleRotationSpeed = g_RNG->RollRandomFloatInRange(15.f, 45.f);
		Rgba8 const minColor(127, 127, 127, 255);
		Rgba8 const maxColor(255, 255, 255, 255);
		float colorLerpFraction = g_RNG->RollRandomFloatZeroToOne();
		Rgba8 particleColor = Interpolate(minColor, maxColor, colorLerpFraction);
		float spread = 30.f * 0.5f;
		EulerAngles randomDeviationAngles(g_RNG->RollRandomFloatInRange(-spread, spread), g_RNG->RollRandomFloatInRange(-spread, spread), 0.f);
		Vec3 particleVelocity = (-hitDirection + randomDeviationAngles.GetAsMatrix_iFwd_jLeft_kUp().GetIBasis3D()).GetNormalized() * g_RNG->RollRandomFloatInRange(0.25f, 0.5f);
		Vec3 particleOffset = targetUnit->m_definition.m_muzzleOffset;

		float particleStartAlpha = 1.f;
		float particleEndAlpha = 0.f;
		float startAlphaTime = 0.25f;
		float endAlphaTime = 1.f;
		float startScale = 1.f;
		float endScale = 4.f;
		float startScaleTime = 0.f;
		float endScaleTime = 1.f;
		float startSpeedMultiplier = 1.f;
		float endSpeedMultiplier = 0.25f;
		float startSpeedTime = 0.f;
		float endSpeedTime = 1.f;

		g_app->m_game->SpawnParticle(targetUnit->m_position + targetUnitFwd * particleOffset.x + targetUnitLeft * particleOffset.y + targetUnitUp * particleOffset.z, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Light Smoke", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
	}
	// Spawn 12 dark smoke particles
	for (int particleIndex = 0; particleIndex < 12; particleIndex++)
	{
		float particleSize = g_RNG->RollRandomFloatInRange(0.25f, 0.5f);
		float particleLifetime = g_RNG->RollRandomFloatInRange(1.f, 2.f);
		float particleRotation = g_RNG->RollRandomFloatInRange(0.f, 360.f);
		float particleRotationSpeed = g_RNG->RollRandomFloatInRange(15.f, 45.f);
		Rgba8 const minColor(31, 31, 31, 255);
		Rgba8 const maxColor(63, 63, 63, 255);
		float colorLerpFraction = g_RNG->RollRandomFloatZeroToOne();
		Rgba8 particleColor = Interpolate(minColor, maxColor, colorLerpFraction);
		float spread = 10.f * 0.5f;
		EulerAngles randomDeviationAngles(g_RNG->RollRandomFloatInRange(-spread, spread), g_RNG->RollRandomFloatInRange(-spread, spread), 0.f);
		Vec3 particleVelocity = (-hitDirection + g_RNG->RollRandomVec3InRadius(Vec3::ZERO, 1.f)).GetNormalized() * g_RNG->RollRandomFloatInRange(0.25f, 0.5f);
		Vec3 particleOffset = targetUnit->m_definition.m_muzzleOffset;

		float particleStartAlpha = 1.f;
		float particleEndAlpha = 0.f;
		float startAlphaTime = 0.5f;
		float endAlphaTime = 1.f;
		float startScale = 1.f;
		float endScale = 1.25f;
		float startScaleTime = 0.f;
		float endScaleTime = 1.f;
		float startSpeedMultiplier = 1.f;
		float endSpeedMultiplier = 0.75f;
		float startSpeedTime = 0.f;
		float endSpeedTime = 1.f;

		g_app->m_game->SpawnParticle(targetUnit->m_position + targetUnitFwd * particleOffset.x + targetUnitLeft * particleOffset.y + targetUnitUp * particleOffset.z, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Dark Smoke", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
	}
	// Spawn 3 type 1 fire particles
	for (int particleIndex = 0; particleIndex < 3; particleIndex++)
	{
		float particleSize = g_RNG->RollRandomFloatInRange(0.5f, 0.75f);
		float particleLifetime = g_RNG->RollRandomFloatInRange(1.f, 2.f);
		float particleRotation = 0.f;
		float particleRotationSpeed = 0.f;
		Rgba8 const minColor(200, 31, 31, 255);
		Rgba8 const maxColor(255, 127, 127, 255);
		float colorLerpFraction = g_RNG->RollRandomFloatZeroToOne();
		Rgba8 particleColor = Interpolate(minColor, maxColor, colorLerpFraction);
		Vec3 particleVelocity = -hitDirection * g_RNG->RollRandomFloatInRange(0.1f, 0.2f);
		Vec3 particleOffset = targetUnit->m_definition.m_muzzleOffset;

		float particleStartAlpha = 1.f;
		float particleEndAlpha = 0.f;
		float startAlphaTime = 0.75f;
		float endAlphaTime = 1.f;
		float startScale = 1.f;
		float endScale = 1.5f;
		float startScaleTime = 0.f;
		float endScaleTime = 1.f;
		float startSpeedMultiplier = 1.f;
		float endSpeedMultiplier = 1.f;
		float startSpeedTime = 0.f;
		float endSpeedTime = 1.f;

		g_app->m_game->SpawnParticle(targetUnit->m_position + targetUnitFwd * particleOffset.x + targetUnitLeft * particleOffset.y + targetUnitUp * particleOffset.z, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Muzzle Flash 1", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
	}
	// Spawn 3 type 2 fire particles
	for (int particleIndex = 0; particleIndex < 3; particleIndex++)
	{
		float particleSize = g_RNG->RollRandomFloatInRange(0.5f, 0.75f);
		float particleLifetime = g_RNG->RollRandomFloatInRange(1.f, 2.f);
		float particleRotation = 0.f;
		float particleRotationSpeed = 0.f;
		Rgba8 const minColor(200, 31, 31, 255);
		Rgba8 const maxColor(255, 127, 127, 255);
		float colorLerpFraction = g_RNG->RollRandomFloatZeroToOne();
		Rgba8 particleColor = Interpolate(minColor, maxColor, colorLerpFraction);
		Vec3 particleVelocity = -hitDirection.GetNormalized() * g_RNG->RollRandomFloatInRange(0.1f, 0.2f);
		Vec3 particleOffset = targetUnit->m_definition.m_muzzleOffset;

		float particleStartAlpha = 1.f;
		float particleEndAlpha = 0.f;
		float startAlphaTime = 0.75f;
		float endAlphaTime = 1.f;
		float startScale = 1.f;
		float endScale = 1.5f;
		float startScaleTime = 0.f;
		float endScaleTime = 1.f;
		float startSpeedMultiplier = 1.f;
		float endSpeedMultiplier = 1.f;
		float startSpeedTime = 0.f;
		float endSpeedTime = 1.f;

		g_app->m_game->SpawnParticle(targetUnit->m_position + targetUnitFwd * particleOffset.x + targetUnitLeft * particleOffset.y + targetUnitUp * particleOffset.z, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Muzzle Flash 2", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
	}
}

void Unit::HoldFire()
{
	m_previousTileCoords = m_tileCoords;
	m_isSelected = false;
	m_ordersIssued = true;
}

void Unit::Cancel()
{
	if (m_didMove)
	{
		m_didMove = false;
	}

	delete m_pathSpline;
	m_pathSpline = nullptr;
	m_pathLength = -1;
	delete m_movementTimer;
	m_movementTimer = nullptr;

	m_tileCoords = m_previousTileCoords;
	m_position = m_map->GetTileWorldPositionFromCoordinates(m_previousTileCoords).ToVec3();

	std::vector<float> newDistanceField;
	m_map->PopuplateDistanceField(newDistanceField, m_tileCoords);
	m_heatMap->SetAllValues(newDistanceField);
}

void Unit::TakeDamage(int damage, Vec3 const& hitDirection)
{
	m_floatingDamageStr = Stringf("-%d", damage);
	m_floatingDamageTimer = new Stopwatch(&m_map->m_game->m_gameClock, 1.f);
	m_floatingDamageTimer->Start();

	m_health -= damage;
	if (m_health <= 0)
	{
		Die();
		return;
	}

	g_audio->StartSound(m_definition.m_hitSFX);

	//---------------------------------------------------------------------------------------
	// Hit Effect
	//---------------------------------------------------------------------------------------
	// Spawn 6 light smoke particles
	for (int particleIndex = 0; particleIndex < 6; particleIndex++)
	{
		float particleSize = g_RNG->RollRandomFloatInRange(0.15f, 0.3f);
		float particleLifetime = g_RNG->RollRandomFloatInRange(0.5f, 1.f);
		float particleRotation = g_RNG->RollRandomFloatInRange(0.f, 360.f);
		float particleRotationSpeed = g_RNG->RollRandomFloatInRange(15.f, 45.f);
		Rgba8 const minColor(127, 127, 127, 255);
		Rgba8 const maxColor(255, 255, 255, 255);
		float colorLerpFraction = g_RNG->RollRandomFloatZeroToOne();
		Rgba8 particleColor = Interpolate(minColor, maxColor, colorLerpFraction);
		float spread = 45.f * 0.5f;
		EulerAngles randomDeviationAngles(g_RNG->RollRandomFloatInRange(-spread, spread), g_RNG->RollRandomFloatInRange(-spread, spread), 0.f);
		Vec3 particleVelocity = (hitDirection + randomDeviationAngles.GetAsMatrix_iFwd_jLeft_kUp().GetIBasis3D()).GetNormalized() * g_RNG->RollRandomFloatInRange(0.125f, 0.25f);

		float particleStartAlpha = 1.f;
		float particleEndAlpha = 0.f;
		float startAlphaTime = 0.25f;
		float endAlphaTime = 1.f;
		float startScale = 1.f;
		float endScale = 4.f;
		float startScaleTime = 0.f;
		float endScaleTime = 1.f;
		float startSpeedMultiplier = 1.f;
		float endSpeedMultiplier = 0.25f;
		float startSpeedTime = 0.f;
		float endSpeedTime = 1.f;

		g_app->m_game->SpawnParticle(m_position - hitDirection * 0.2f, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Light Smoke", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
	}
	// Spawn 6 dark smoke particles
	for (int particleIndex = 0; particleIndex < 6; particleIndex++)
	{
		float particleSize = g_RNG->RollRandomFloatInRange(1.f, 1.25f);
		float particleLifetime = g_RNG->RollRandomFloatInRange(0.25f, 0.5f);
		float particleRotation = g_RNG->RollRandomFloatInRange(0.f, 360.f);
		float particleRotationSpeed = g_RNG->RollRandomFloatInRange(15.f, 45.f);
		Rgba8 const minColor(31, 31, 31, 255);
		Rgba8 const maxColor(63, 63, 63, 255);
		float colorLerpFraction = g_RNG->RollRandomFloatZeroToOne();
		Rgba8 particleColor = Interpolate(minColor, maxColor, colorLerpFraction);
		float spread = 180.f * 0.5f;
		EulerAngles randomDeviationAngles(g_RNG->RollRandomFloatInRange(-spread, spread), g_RNG->RollRandomFloatInRange(-spread, spread), 0.f);
		Vec3 particleVelocity = (hitDirection + randomDeviationAngles.GetAsMatrix_iFwd_jLeft_kUp().GetIBasis3D()).GetNormalized() * g_RNG->RollRandomFloatInRange(0.25f, 0.5f);

		float particleStartAlpha = 1.f;
		float particleEndAlpha = 0.f;
		float startAlphaTime = 0.5f;
		float endAlphaTime = 1.f;
		float startScale = 1.f;
		float endScale = 1.25f;
		float startScaleTime = 0.f;
		float endScaleTime = 1.f;
		float startSpeedMultiplier = 1.f;
		float endSpeedMultiplier = 0.75f;
		float startSpeedTime = 0.f;
		float endSpeedTime = 1.f;

		g_app->m_game->SpawnParticle(m_position - hitDirection * 0.2f, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Dark Smoke", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
	}
	// Spawn 6 fire particles (Sparks)
	for (int particleIndex = 0; particleIndex < 6; particleIndex++)
	{
		float particleSize = g_RNG->RollRandomFloatInRange(0.05f, 0.1f);
		float particleLifetime = g_RNG->RollRandomFloatInRange(0.125f, 0.25f);
		float particleRotation = 0.f;
		float particleRotationSpeed = 0.f;
		Rgba8 const minColor(200, 31, 31, 255);
		Rgba8 const maxColor(255, 127, 127, 255);
		float colorLerpFraction = g_RNG->RollRandomFloatZeroToOne();
		Rgba8 particleColor = Interpolate(minColor, maxColor, colorLerpFraction);
		float spread = 180.f * 0.5f;
		EulerAngles randomDeviationAngles(g_RNG->RollRandomFloatInRange(-spread, spread), g_RNG->RollRandomFloatInRange(-spread, spread), 0.f);
		Vec3 particleVelocity = (hitDirection + randomDeviationAngles.GetAsMatrix_iFwd_jLeft_kUp().GetIBasis3D()).GetNormalized() * g_RNG->RollRandomFloatInRange(0.5f, 0.75f);

		float particleStartAlpha = 1.f;
		float particleEndAlpha = 0.f;
		float startAlphaTime = 0.75f;
		float endAlphaTime = 1.f;
		float startScale = 1.f;
		float endScale = 1.5f;
		float startScaleTime = 0.f;
		float endScaleTime = 1.f;
		float startSpeedMultiplier = 1.f;
		float endSpeedMultiplier = 1.f;
		float startSpeedTime = 0.f;
		float endSpeedTime = 1.f;

		g_app->m_game->SpawnParticle(m_position - hitDirection * 0.2f, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Muzzle Flash 1", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
	}
}

void Unit::Die()
{
	g_audio->StartSound(m_definition.m_deathSFX);
	m_isDead = true;
	m_isGarbage = true;

	//---------------------------------------------------------------------------------------
	// Death Effect
	//---------------------------------------------------------------------------------------
	// Spawn 12 light smoke particles
	for (int particleIndex = 0; particleIndex < 6; particleIndex++)
	{
		float particleSize = g_RNG->RollRandomFloatInRange(0.25f, 0.75f);
		float particleLifetime = g_RNG->RollRandomFloatInRange(1.5f, 2.5f);
		float particleRotation = g_RNG->RollRandomFloatInRange(0.f, 360.f);
		float particleRotationSpeed = g_RNG->RollRandomFloatInRange(15.f, 45.f);
		Rgba8 const minColor(127, 127, 127, 255);
		Rgba8 const maxColor(255, 255, 255, 255);
		float colorLerpFraction = g_RNG->RollRandomFloatZeroToOne();
		Rgba8 particleColor = Interpolate(minColor, maxColor, colorLerpFraction);
		Vec3 particleVelocity = g_RNG->RollRandomVec3InRadius(Vec3::ZERO, 1.f).GetNormalized() * g_RNG->RollRandomFloatInRange(0.25f, 0.5f);
		Vec3 particleOffset = g_RNG->RollRandomVec3InRadius(Vec3::ZERO, 0.1f);

		float particleStartAlpha = 1.f;
		float particleEndAlpha = 0.f;
		float startAlphaTime = 0.25f;
		float endAlphaTime = 1.f;
		float startScale = 1.f;
		float endScale = 4.f;
		float startScaleTime = 0.f;
		float endScaleTime = 1.f;
		float startSpeedMultiplier = 1.f;
		float endSpeedMultiplier = 0.25f;
		float startSpeedTime = 0.f;
		float endSpeedTime = 1.f;

		g_app->m_game->SpawnParticle(m_position + particleOffset, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Light Smoke", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
	}
	// Spawn 6 dark smoke particles
	for (int particleIndex = 0; particleIndex < 6; particleIndex++)
	{
		float particleSize = g_RNG->RollRandomFloatInRange(0.25f, 0.5f);
		float particleLifetime = g_RNG->RollRandomFloatInRange(1.f, 2.f);
		float particleRotation = g_RNG->RollRandomFloatInRange(0.f, 360.f);
		float particleRotationSpeed = g_RNG->RollRandomFloatInRange(15.f, 45.f);
		Rgba8 const minColor(31, 31, 31, 255);
		Rgba8 const maxColor(63, 63, 63, 255);
		float colorLerpFraction = g_RNG->RollRandomFloatZeroToOne();
		Rgba8 particleColor = Interpolate(minColor, maxColor, colorLerpFraction);
		Vec3 particleVelocity = g_RNG->RollRandomVec3InRadius(Vec3::ZERO, 1.f).GetNormalized() * g_RNG->RollRandomFloatInRange(0.25f, 0.5f);
		Vec3 particleOffset = g_RNG->RollRandomVec3InRadius(Vec3::ZERO, 0.1f);

		float particleStartAlpha = 1.f;
		float particleEndAlpha = 0.f;
		float startAlphaTime = 0.5f;
		float endAlphaTime = 1.f;
		float startScale = 1.f;
		float endScale = 1.25f;
		float startScaleTime = 0.f;
		float endScaleTime = 1.f;
		float startSpeedMultiplier = 1.f;
		float endSpeedMultiplier = 0.75f;
		float startSpeedTime = 0.f;
		float endSpeedTime = 1.f;

		g_app->m_game->SpawnParticle(m_position + particleOffset, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Dark Smoke", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
	}
	// Spawn 6 type 1 fire particles
	for (int particleIndex = 0; particleIndex < 6; particleIndex++)
	{
		float particleSize = g_RNG->RollRandomFloatInRange(0.5f, 0.75f);
		float particleLifetime = g_RNG->RollRandomFloatInRange(1.f, 2.f);
		float particleRotation = 0.f;
		float particleRotationSpeed = 0.f;
		Rgba8 const minColor(200, 31, 31, 255);
		Rgba8 const maxColor(255, 127, 127, 255);
		float colorLerpFraction = g_RNG->RollRandomFloatZeroToOne();
		Rgba8 particleColor = Interpolate(minColor, maxColor, colorLerpFraction);
		Vec3 particleVelocity = g_RNG->RollRandomVec3InRadius(Vec3::ZERO, 1.f).GetNormalized() * g_RNG->RollRandomFloatInRange(0.1f, 0.2f);
		Vec3 particleOffset = g_RNG->RollRandomVec3InRadius(Vec3::ZERO, 0.01f);

		float particleStartAlpha = 1.f;
		float particleEndAlpha = 0.f;
		float startAlphaTime = 0.75f;
		float endAlphaTime = 1.f;
		float startScale = 1.f;
		float endScale = 1.5f;
		float startScaleTime = 0.f;
		float endScaleTime = 1.f;
		float startSpeedMultiplier = 1.f;
		float endSpeedMultiplier = 1.f;
		float startSpeedTime = 0.f;
		float endSpeedTime = 1.f;

		g_app->m_game->SpawnParticle(m_position + particleOffset, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Muzzle Flash 1", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
	}
	// Spawn 6 type 2 fire particles
	for (int particleIndex = 0; particleIndex < 6; particleIndex++)
	{
		float particleSize = g_RNG->RollRandomFloatInRange(0.5f, 0.75f);
		float particleLifetime = g_RNG->RollRandomFloatInRange(1.f, 2.f);
		float particleRotation = 0.f;
		float particleRotationSpeed = 0.f;
		Rgba8 const minColor(200, 31, 31, 255);
		Rgba8 const maxColor(255, 127, 127, 255);
		float colorLerpFraction = g_RNG->RollRandomFloatZeroToOne();
		Rgba8 particleColor = Interpolate(minColor, maxColor, colorLerpFraction);
		Vec3 particleVelocity = g_RNG->RollRandomVec3InRadius(Vec3::ZERO, 1.f).GetNormalized() * g_RNG->RollRandomFloatInRange(0.1f, 0.2f);
		Vec3 particleOffset = g_RNG->RollRandomVec3InRadius(Vec3::ZERO, 0.01f);

		float particleStartAlpha = 1.f;
		float particleEndAlpha = 0.f;
		float startAlphaTime = 0.75f;
		float endAlphaTime = 1.f;
		float startScale = 1.f;
		float endScale = 1.5f;
		float startScaleTime = 0.f;
		float endScaleTime = 1.f;
		float startSpeedMultiplier = 1.f;
		float endSpeedMultiplier = 1.f;
		float startSpeedTime = 0.f;
		float endSpeedTime = 1.f;

		g_app->m_game->SpawnParticle(m_position + particleOffset, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Muzzle Flash 2", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
	}
}
