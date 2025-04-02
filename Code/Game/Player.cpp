#include "Game/Player.hpp"

#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"
#include "Game/UnitDefinition.hpp"
#include "Game/Unit.hpp"

#include "Engine/Networking/NetSystem.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"


Player::Player(Game* game, int playerIndex, NetState netState)
	: m_game(game)
	, m_playerIndex(playerIndex)
	, m_netState(netState)
{
	m_diffuseShader = g_renderer->CreateOrGetShader("Data/Shaders/Diffuse", VertexType::VERTEX_PCUTBN);
}

void Player::InitializeUnits(Map* map)
{
	auto unitsDataIter = map->m_definition.m_unitsDataByPlayer.find(m_playerIndex);
	if (unitsDataIter == map->m_definition.m_unitsDataByPlayer.end())
	{
		ERROR_AND_DIE("Attempted to initialize units for undefined player!");
	}

	int numTiles = map->m_definition.m_dimensions.x * map->m_definition.m_dimensions.y;
	std::string unitsData = unitsDataIter->second;
	EulerAngles unitOrientation = EulerAngles::ZERO;
	if(m_playerIndex == 1)
	{
		unitOrientation.m_yawDegrees = 180.f;
	}

	for (int tileIndex = 0; tileIndex < numTiles; tileIndex++)
	{
		Vec2 tilePosition = map->GetTileWorldPositionFromIndex(tileIndex);
		IntVec2 tileCoords = map->GetTileCoordsFromIndex(tileIndex);
		char tileSymbol = unitsData[tileIndex];

		for (auto unitDefIter = UnitDefinition::s_definitions.begin(); unitDefIter != UnitDefinition::s_definitions.end(); ++unitDefIter)
		{
			if (unitDefIter->second.m_symbol == tileSymbol)
			{
				Unit* newUnit = new Unit(unitDefIter->second, map, tileCoords, tilePosition.ToVec3(), unitOrientation, this);
				m_units.push_back(newUnit);
			}
		}
	}
}

void Player::Update()
{
	for (int unitIndex = 0; unitIndex < (int)m_units.size(); unitIndex++)
	{
		m_units[unitIndex]->Update();
	}

	if (m_turnState == TurnState::WAITING_FOR_TURN)
	{
		return;
	}
	if (m_game->m_gameType == GameType::NETWORK && m_netState == NetState::REMOTE)
	{
		return;
	}

	RaycastResult3D raycastResult = m_game->m_currentMap->RaycastCursorVsMap();
	int tileIndexForClosestTile = -1;
	float closestTileDistance = FLT_MAX;
	for (int tileIndex = 0; tileIndex < (int)m_game->m_currentMap->m_tiles.size(); tileIndex++)
	{
		Vec2 tilePosition = m_game->m_currentMap->GetTileWorldPositionFromIndex(tileIndex);

		if (!IsPointInsideAABB2(tilePosition, AABB2(m_game->m_currentMap->m_definition.m_bounds.m_mins.GetXY(), m_game->m_currentMap->m_definition.m_bounds.m_maxs.GetXY())))
		{
			continue;
		}

		float tileDistanceFromCursor = GetDistanceSquared2D(raycastResult.m_impactPosition.GetXY(), tilePosition);
		if (tileDistanceFromCursor < closestTileDistance)
		{
			closestTileDistance = tileDistanceFromCursor;
			tileIndexForClosestTile = tileIndex;
		}

		if (IsPointInsideHex(raycastResult.m_impactPosition.GetXY(), tilePosition, Tile::HEX_RADIUS))
		{
			IntVec2 tileCoords = m_game->m_currentMap->GetTileCoordsFromIndex(tileIndex);

			if (!m_game->m_currentMap->m_tiles[tileIndex].m_isHovered)
			{
				m_game->SetFocusedHex(tileCoords);

				if (m_game->m_gameType == GameType::NETWORK)
				{
					g_netSystem->QueueMessageForSend(Stringf("SetFocusedHex hexCoords=\"%d,%d\"", tileCoords.x, tileCoords.y));
				}
			}
		}
	}

	if (!m_game->m_hasGameEnded && !m_game->m_isAnimationPlaying && g_input->WasKeyJustPressed(KEYCODE_LMB))
	{
		if (m_turnState == TurnState::NO_SELECTION)
		{
			m_game->SelectFocusedUnit();
			if (m_game->m_gameType == GameType::NETWORK)
			{
				g_netSystem->QueueMessageForSend(Stringf("SelectFocusedUnit"));
			}
		}
		else if (m_turnState == TurnState::UNIT_SELECTED_MOVE)
		{
			IntVec2& targetTileCoords = m_game->m_currentMap->m_hoveredTile;

			if (targetTileCoords == m_selectedUnit->m_tileCoords)
			{
				m_game->Stay();
				if (m_game->m_gameType == GameType::NETWORK)
				{
					g_netSystem->QueueMessageForSend(Stringf("Stay"));
				}
			}
			else
			{
				m_game->Move(targetTileCoords);
				if (m_game->m_gameType == GameType::NETWORK)
				{
					g_netSystem->QueueMessageForSend(Stringf("Move hexCoords=\"%d,%d\"", targetTileCoords.x, targetTileCoords.y));
				}
			}
		}
		else if (m_turnState == TurnState::UNIT_SELECTED_ATTACK)
		{
			IntVec2& targetTileCoords = m_game->m_currentMap->m_hoveredTile;

			if (targetTileCoords == m_selectedUnit->m_tileCoords)
			{
				m_game->HoldFire();
				if (m_game->m_gameType == GameType::NETWORK)
				{
					g_netSystem->QueueMessageForSend(Stringf("HoldFire"));
				}
			}
			else
			{
				m_game->Attack();
				if (m_game->m_gameType == GameType::NETWORK)
				{
					g_netSystem->QueueMessageForSend(Stringf("Attack"));
				}
			}
		}
	}

	if (g_input->WasKeyJustPressed(KEYCODE_RMB) && !m_game->m_isAnimationPlaying)
	{
		m_game->Cancel();
		if (m_game->m_gameType == GameType::NETWORK)
		{
			g_netSystem->QueueMessageForSend(Stringf("Cancel"));
		}
	}

	if (g_input->WasKeyJustPressed('Y') && m_turnState == TurnState::NO_SELECTION && !m_game->m_isAnimationPlaying)
	{
		m_game->EndTurn();
		if (m_game->m_gameType == GameType::NETWORK)
		{
			g_netSystem->QueueMessageForSend(Stringf("EndTurn"));
		}
	}

	if (g_input->WasKeyJustPressed('P') && m_turnState == TurnState::UNIT_SELECTED_ATTACK && !m_game->m_isAnimationPlaying)
	{
		Vec3 unitFwd, unitLeft, unitUp;
		m_selectedUnit->m_orientation.GetAsVectors_iFwd_jLeft_kUp(unitFwd, unitLeft, unitUp);

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
			Vec3 particleVelocity = (unitFwd + randomDeviationAngles.GetAsMatrix_iFwd_jLeft_kUp().GetIBasis3D()).GetNormalized() * g_RNG->RollRandomFloatInRange(0.25f, 0.5f);
			Vec3 particleOffset = m_selectedUnit->m_definition.m_muzzleOffset;

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

			g_app->m_game->SpawnParticle(m_selectedUnit->m_position + unitFwd * particleOffset.x + unitLeft * particleOffset.y + unitUp * particleOffset.z, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Light Smoke", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
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
			Vec3 particleVelocity = (unitFwd + randomDeviationAngles.GetAsMatrix_iFwd_jLeft_kUp().GetIBasis3D()).GetNormalized() * g_RNG->RollRandomFloatInRange(0.25f, 0.5f);
			Vec3 particleOffset = m_selectedUnit->m_definition.m_muzzleOffset;

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

			g_app->m_game->SpawnParticle(m_selectedUnit->m_position + unitFwd * particleOffset.x + unitLeft * particleOffset.y + unitUp * particleOffset.z, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Dark Smoke", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
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
			Vec3 particleVelocity = unitFwd * g_RNG->RollRandomFloatInRange(0.1f, 0.2f);
			Vec3 particleOffset = m_selectedUnit->m_definition.m_muzzleOffset;

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

			g_app->m_game->SpawnParticle(m_selectedUnit->m_position + unitFwd * particleOffset.x + unitLeft * particleOffset.y + unitUp * particleOffset.z, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Muzzle Flash 1", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
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
			Vec3 particleVelocity = unitFwd * g_RNG->RollRandomFloatInRange(0.1f, 0.2f);
			Vec3 particleOffset = m_selectedUnit->m_definition.m_muzzleOffset;

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

			g_app->m_game->SpawnParticle(m_selectedUnit->m_position + unitFwd * particleOffset.x + unitLeft * particleOffset.y + unitUp * particleOffset.z, particleVelocity, particleRotation, particleRotationSpeed, particleSize, particleLifetime, "Muzzle Flash 2", particleColor, particleStartAlpha, particleEndAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
		}
	}
}

void Player::Render() const
{
	g_renderer->BeginCamera(m_game->m_worldCamera);
	{
		g_renderer->BindShader(m_diffuseShader);
		for (int unitIndex = 0; unitIndex < (int)m_units.size(); unitIndex++)
		{
			m_units[unitIndex]->Render();
		}
	}
	g_renderer->EndCamera(m_game->m_worldCamera);
}

void Player::DebugRender() const
{
	g_renderer->BeginCamera(m_game->m_worldCamera);
	{
		if (m_selectedUnit)
		{
			TileHeatMap const* heatMap = m_selectedUnit->m_heatMap;
			m_game->m_currentMap->DebugRenderDistanceField(heatMap);
		}

		if (m_selectedUnit && m_selectedUnit->m_pathSpline)
		{
			std::vector<Vertex_PCU> splineDebugVerts;
			m_selectedUnit->m_pathSpline->AddVertsForDebugDraw(splineDebugVerts, Rgba8::MAGENTA, Rgba8::YELLOW, false, Rgba8::WHITE, 64, 0.02f, 0.05f);
			g_renderer->SetBlendMode(BlendMode::ALPHA);
			g_renderer->SetDepthMode(DepthMode::DISABLED);
			g_renderer->SetModelConstants();
			g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_BACK);
			g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
			g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
			g_renderer->BindTexture(nullptr);
			g_renderer->BindShader(nullptr);
			g_renderer->DrawVertexArray(splineDebugVerts);
		}
	}
	g_renderer->EndCamera(m_game->m_worldCamera);
}

Rgba8 const Player::GetTeamColor()
{
	if (m_playerIndex != 0)
	{
		return Rgba8(255, 127, 127, 255);
	}
	return Rgba8(127, 127, 255, 255);
}

Unit* Player::GetUnitFromTileCoords(IntVec2 const& tileCoords) const
{
	for (int unitIndex = 0; unitIndex < (int)m_units.size(); unitIndex++)
	{
		if (m_units[unitIndex]->m_tileCoords == tileCoords)
		{
			return m_units[unitIndex];
		}
	}

	return nullptr;
}

void Player::DeleteGarbageUnits()
{
	for (int unitIndex = 0; unitIndex < (int)m_units.size(); unitIndex++)
	{
		if (m_units[unitIndex]->m_isGarbage)
		{
			delete m_units[unitIndex];
			m_units.erase(m_units.begin() + unitIndex);
			unitIndex--;
		}
	}
}

void Player::EndTurn()
{
	m_turnState = TurnState::WAITING_FOR_TURN;
	for (int unitIndex = 0; unitIndex < (int)m_units.size(); unitIndex++)
	{
		m_units[unitIndex]->m_didMove = false;
		m_units[unitIndex]->m_isSelected = false;
		m_units[unitIndex]->m_ordersIssued = false;
	}
}
