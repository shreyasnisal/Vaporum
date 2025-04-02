#include "Game/Map.hpp"

#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Player.hpp"
#include "Game/Unit.hpp"
#include "Game/UnitDefinition.hpp"

#include "Engine/Core/Models/ModelLoader.hpp"
#include "Engine/Networking/NetSystem.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include <queue>


Map::~Map()
{
	delete m_mapVBO;
	m_mapVBO = nullptr;

	delete m_tilesVBO;
	m_tilesVBO = nullptr;

	delete m_game->m_player1;
	m_game->m_player1 = nullptr;

	delete m_game->m_player2;
	m_game->m_player2 = nullptr;
}

Map::Map(Game* game, MapDefinition const& definition)
	: m_game(game)
	, m_definition(definition)
{
	LoadAssets();

	int numTiles = m_definition.m_dimensions.x * m_definition.m_dimensions.y;

	std::vector<Vertex_PCUTBN> mapVertexes;
	AddVertsForQuad3D(mapVertexes, Vec3(-40.f, 80.f, 0.f), Vec3(-40.f, -80.f, 0.f), Vec3(40.f, -80.f, 0.f), Vec3(40.f, 80.f, 0.f));
	m_mapVBO = g_renderer->CreateVertexBuffer(mapVertexes.size() * sizeof(Vertex_PCUTBN), VertexType::VERTEX_PCUTBN);
	g_renderer->CopyCPUToGPU(mapVertexes.data(), mapVertexes.size() * sizeof(Vertex_PCUTBN), m_mapVBO);

	std::vector<Vertex_PCU> tileVertexes;
	for (int tileIndex = 0; tileIndex < numTiles; tileIndex++)
	{
		Vec2 tilePosition = GetTileWorldPositionFromIndex(tileIndex);

		char tileSymbol = m_definition.m_tilesData[tileIndex];
		TileDefinition* tileDef = TileDefinition::GetTileDefinitionFromSymbol(tileSymbol);

		m_tiles.push_back(Tile(*tileDef));

		if (IsPointInsideAABB2(tilePosition, AABB2(m_definition.m_bounds.m_mins.GetXY(), m_definition.m_bounds.m_maxs.GetXY())))
		{
			m_tiles[tileIndex].AddVerts(tileVertexes, tilePosition.ToVec3());
		}
	}
	m_tilesVBO = g_renderer->CreateVertexBuffer(tileVertexes.size() * sizeof(Vertex_PCU));
	g_renderer->CopyCPUToGPU(tileVertexes.data(), tileVertexes.size() * sizeof(Vertex_PCU), m_tilesVBO);

	// Initialize Players
	if (m_game->m_gameType == GameType::LOCAL)
	{
		m_game->m_player1 = new Player(m_game, 0, NetState::LOCAL);
		m_game->m_player1->InitializeUnits(this);

		m_game->m_player2 = new Player(m_game, 1, NetState::LOCAL);
		m_game->m_player2->InitializeUnits(this);

		if (m_game->m_gameState != GameState::GAME)
		{
			m_game->m_nextGameState = GameState::GAME;
		}
		else
		{
			m_game->m_player1->m_turnState = TurnState::NO_SELECTION;
		}

		m_game->m_isLocalPlayerReady = true;
	}
	else if (g_netSystem->GetNetworkMode() == NetworkMode::SERVER)
	{
		m_game->m_player1 = new Player(m_game, 0, NetState::LOCAL);
		m_game->m_player1->InitializeUnits(this);

		m_game->m_player2 = new Player(m_game, 1, NetState::REMOTE);
		m_game->m_player2->InitializeUnits(this);

		g_netSystem->QueueMessageForSend("PlayerReady");
		m_game->m_isLocalPlayerReady = true;
	}
	else
	{
		m_game->m_player1 = new Player(m_game, 0, NetState::REMOTE);
		m_game->m_player1->InitializeUnits(this);

		m_game->m_player2 = new Player(m_game, 1, NetState::LOCAL);
		m_game->m_player2->InitializeUnits(this);

		g_netSystem->QueueMessageForSend("PlayerReady");
		m_game->m_isLocalPlayerReady = true;
	}

	m_game->m_player1->m_turnState = TurnState::NO_SELECTION;
}

void Map::LoadAssets()
{
	XmlDocument xmlDoc;
	XmlResult result = xmlDoc.LoadFile("Data/Materials/Moon_8k.xml");
	if (result != XmlResult::XML_SUCCESS)
	{
		ERROR_AND_DIE(Stringf("Could not open or read XML file \"%s\"", "Data/Materials/Moon_8k.xml"));
	}
	XmlElement* moonMaterialXmlElement = xmlDoc.RootElement();
	m_moonMaterial = g_modelLoader->CreateMaterialFromXml(moonMaterialXmlElement);
}

void Map::Update()
{
	constexpr float MOVEMENT_SPEED = 10.f;
	constexpr float CAMERA_MIN_ELEVATION = 1.f;
	
	float deltaSeconds = m_game->m_gameClock.GetDeltaSeconds();

	if (g_input->IsKeyDown('W'))
	{
		m_game->m_playerPosition += Vec3::NORTH * MOVEMENT_SPEED * deltaSeconds;
	}
	if (g_input->IsKeyDown('A'))
	{
		m_game->m_playerPosition += Vec3::WEST * MOVEMENT_SPEED * deltaSeconds;
	}
	if (g_input->IsKeyDown('S'))
	{
		m_game->m_playerPosition += Vec3::SOUTH * MOVEMENT_SPEED * deltaSeconds;
	}
	if (g_input->IsKeyDown('D'))
	{
		m_game->m_playerPosition += Vec3::EAST * MOVEMENT_SPEED * deltaSeconds;
	}
	if (g_input->IsKeyDown('Q'))
	{
		m_game->m_playerPosition += Vec3::GROUNDWARD * MOVEMENT_SPEED * deltaSeconds;
	}
	if (g_input->IsKeyDown('E'))
	{
		m_game->m_playerPosition += Vec3::SKYWARD * MOVEMENT_SPEED * deltaSeconds;
	}

	if (g_input->WasKeyJustPressed(KEYCODE_F1))
	{
		m_debugDraw = !m_debugDraw;
	}

	m_game->m_playerPosition.x = GetClamped(m_game->m_playerPosition.x, m_definition.m_bounds.m_mins.x, m_definition.m_bounds.m_maxs.x - m_definition.m_bounds.m_mins.x);
	m_game->m_playerPosition.y = GetClamped(m_game->m_playerPosition.y, m_definition.m_bounds.m_mins.y - m_game->m_playerPosition.z / TanDegrees(Game::FIXED_CAMERA_ANGLE.m_pitchDegrees), m_definition.m_bounds.m_maxs.y - m_game->m_playerPosition.z / TanDegrees(Game::FIXED_CAMERA_ANGLE.m_pitchDegrees));
	m_game->m_playerPosition.z = GetClamped(m_game->m_playerPosition.z, CAMERA_MIN_ELEVATION, m_definition.m_bounds.m_maxs.z);

	Player* const& player1 = m_game->m_player1;
	Player* const& player2 = m_game->m_player2;
	Unit* hoveredUnit = nullptr;
	if (player1)
	{
		for (int unitIndex = 0; unitIndex < (int)player1->m_units.size(); unitIndex++)
		{
			if (player1->m_units[unitIndex]->m_tileCoords == m_hoveredTile)
			{
				hoveredUnit = player1->m_units[unitIndex];
				break;
			}
		}
	}
	if (player2 && !hoveredUnit)
	{
		for (int unitIndex = 0; unitIndex < (int)player2->m_units.size(); unitIndex++)
		{
			if (player2->m_units[unitIndex]->m_tileCoords == m_hoveredTile)
			{
				hoveredUnit = player2->m_units[unitIndex];
				break;
			}
		}
	}

	if (hoveredUnit)
	{
		std::string unitInfo = Stringf("Name: %s\n", hoveredUnit->m_definition.m_name.c_str());
		unitInfo += Stringf("Attack: %d\n", hoveredUnit->m_definition.m_attackDamage);
		unitInfo += Stringf("Attack Range: %d - %d\n", hoveredUnit->m_definition.m_attackRange.m_min, hoveredUnit->m_definition.m_attackRange.m_max);
		unitInfo += Stringf("Defense: %d\n", hoveredUnit->m_definition.m_defense);
		unitInfo += Stringf("Health: %d", hoveredUnit->m_health);

		if (hoveredUnit->m_owner == player1)
		{
			m_game->m_p1UnitInfoWidget->SetText(unitInfo);
			m_game->m_p1UnitImageWidget->SetImage(hoveredUnit->m_definition.m_imagePath);
			m_game->m_p1UnitInfoContainerWidget->SetVisible(true);
			m_game->m_p1UnitInfoWidget->SetVisible(true);
			m_game->m_p1UnitImageWidget->SetVisible(true);
		}
		else
		{
			m_game->m_p2UnitInfoWidget->SetText(unitInfo);
			m_game->m_p2UnitImageWidget->SetImage(hoveredUnit->m_definition.m_imagePath);
			m_game->m_p2UnitInfoContainerWidget->SetVisible(true);
			m_game->m_p2UnitInfoWidget->SetVisible(true);
			m_game->m_p2UnitImageWidget->SetVisible(true);
		}
	}
	else
	{
		m_game->m_p1UnitInfoContainerWidget->SetVisible(false);
		m_game->m_p1UnitInfoWidget->SetVisible(false);
		m_game->m_p1UnitImageWidget->SetVisible(false);
		m_game->m_p2UnitInfoContainerWidget->SetVisible(false);
		m_game->m_p2UnitInfoWidget->SetVisible(false);
		m_game->m_p2UnitImageWidget->SetVisible(false);
	}
}

void Map::Render() const
{
	g_renderer->BeginCamera(m_game->m_worldCamera);
	{
		g_renderer->SetBlendMode(BlendMode::OPAQUE);
		g_renderer->SetDepthMode(DepthMode::ENABLED);
		g_renderer->SetModelConstants();
		g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_NONE);
		g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
		g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);

		g_renderer->BindTexture(m_moonMaterial.m_diffuseTexture, 0);
		g_renderer->BindTexture(m_moonMaterial.m_normalTexture, 1);
		g_renderer->BindTexture(m_moonMaterial.m_specGlosEmitTexture, 2);
		g_renderer->BindShader(m_moonMaterial.m_shader);
		g_renderer->SetLightConstants(m_game->m_sunOrientation.GetAsMatrix_iFwd_jLeft_kUp().GetIBasis3D(), m_game->m_sunIntensity, 1.f - m_game->m_sunIntensity, m_game->m_playerPosition);
		g_renderer->DrawVertexBuffer(m_mapVBO, (int)m_mapVBO->m_size / sizeof(Vertex_PCUTBN));

		g_renderer->SetBlendMode(BlendMode::ALPHA);
		g_renderer->SetDepthMode(DepthMode::DISABLED);
		g_renderer->BindTexture(nullptr);
		g_renderer->BindShader(nullptr);
		g_renderer->DrawVertexBuffer(m_tilesVBO, (int)m_tilesVBO->m_size / (int)sizeof(Vertex_PCU));

		Player* const& player1 = m_game->m_player1;
		Player* const& player2 = m_game->m_player2;

		std::vector<Vertex_PCU> tileHighlightVerts;
		Player* currentPlayer = m_game->GetCurrentPlayer();
		Player* waitingPlayer = m_game->GetWaitingPlayer();
		if (currentPlayer && currentPlayer->m_turnState == TurnState::UNIT_SELECTED_MOVE)
		{
			Unit* selectedUnit = currentPlayer->m_selectedUnit;
			if (selectedUnit && !selectedUnit->m_didMove)
			{
				for (int tileIndex = 0; tileIndex < (int)m_tiles.size(); tileIndex++)
				{
					IntVec2 tileCoords = GetTileCoordsFromIndex(tileIndex);

					if (GetHexTaxicabDistance(tileCoords, selectedUnit->m_tileCoords) > selectedUnit->m_definition.m_movementRange)
					{
						continue;
					}

					Vec3 tilePosition = GetTileWorldPositionFromIndex(tileIndex).ToVec3();
					if (!IsPointInsideAABB2(tilePosition.GetXY(), AABB2(m_definition.m_bounds.m_mins.GetXY(), m_definition.m_bounds.m_maxs.GetXY())))
					{
						continue;
					}
					if (player1->GetUnitFromTileCoords(tileCoords) || player2->GetUnitFromTileCoords(tileCoords))
					{
						continue;
					}

					m_tiles[tileIndex].AddVertsForHighlight(tileHighlightVerts, tilePosition);
				}

				if (GetHexTaxicabDistance(m_hoveredTile, selectedUnit->m_tileCoords) <= selectedUnit->m_definition.m_movementRange)
				{
					std::vector<IntVec2> tilesPath;
					GenerateHeatMapPath(tilesPath, m_hoveredTile, selectedUnit->m_tileCoords, selectedUnit->m_heatMap);
					for (int tilePathIndex = 0; tilePathIndex < (int)tilesPath.size(); tilePathIndex++)
					{
						IntVec2 const& pathTileCoords = tilesPath[tilePathIndex];
						int tileIndex = GetTileIndexFromCoords(pathTileCoords);
						Vec2 tilePosition = GetTileWorldPositionFromCoordinates(pathTileCoords);
						m_tiles[tileIndex].AddVertsForPathHighlight(tileHighlightVerts, tilePosition.ToVec3());
					}
				}
			}
		}

		std::vector<Vertex_PCU> tileHoverVertexes;
		for (int tileIndex = 0; tileIndex < (int)m_tiles.size(); tileIndex++)
		{
			Vec3 tilePosition = GetTileWorldPositionFromIndex(tileIndex).ToVec3();
			IntVec2 tileCoords = GetTileCoordsFromIndex(tileIndex);
			if (currentPlayer && currentPlayer->m_turnState == TurnState::UNIT_SELECTED_ATTACK && waitingPlayer->GetUnitFromTileCoords(tileCoords))
			{
				m_tiles[tileIndex].AddVertsForAttackHighlight(tileHighlightVerts, tilePosition);
			}
			else
			{
				m_tiles[tileIndex].AddVertsForHover(tileHoverVertexes, tilePosition);
			}
		}

		Rgba8 hoverColor = Rgba8::LIME;
		Unit* hoveredUnit = nullptr;
		if (player1)
		{
			for (int unitIndex = 0; unitIndex < (int)player1->m_units.size(); unitIndex++)
			{
				if (player1->m_units[unitIndex]->m_tileCoords == m_hoveredTile)
				{
					hoveredUnit = player1->m_units[unitIndex];
					break;
				}
			}
		}
		if (player2 && !hoveredUnit)
		{
			for (int unitIndex = 0; unitIndex < (int)player2->m_units.size(); unitIndex++)
			{
				if (player2->m_units[unitIndex]->m_tileCoords == m_hoveredTile)
				{
					hoveredUnit = player2->m_units[unitIndex];
					break;
				}
			}
		}

		if (hoveredUnit)
		{
			if (hoveredUnit->m_owner != m_game->GetCurrentPlayer())
			{
				hoverColor = Rgba8::RED;
			}
			else
			{
				hoverColor = Rgba8::BLUE;
			}
		}

		g_renderer->SetModelConstants();
		g_renderer->DrawVertexArray(tileHighlightVerts);

		g_renderer->SetModelConstants(Mat44::IDENTITY, hoverColor);
		g_renderer->DrawVertexArray(tileHoverVertexes);
	}
	g_renderer->EndCamera(m_game->m_worldCamera);
}

RaycastResult3D Map::RaycastCursorVsMap()
{
	// Raycast vs World
	Vec2 normalizedCursorPosition = g_input->GetCursorNormalizedPosition();
	Vec2 windowDimensions = g_window->GetClientDimensions().GetAsVec2();
	float cursorScreenX = RangeMap(normalizedCursorPosition.x, 0.f, 1.f, -0.5f, 0.5f);
	float cursorScreenY = RangeMap(normalizedCursorPosition.y, 0.f, 1.f, -0.5f, 0.5f);

	float nearClipPlaneDistance = m_game->m_worldCamera.m_perspectiveNear;
	float halfFov = m_game->m_worldCamera.m_perspectiveFov * 0.5f;
	float nearClipPlaneWorldHeight = nearClipPlaneDistance * TanDegrees(halfFov) * 2.f;
	float nearClipPlaneWorldWidth = nearClipPlaneWorldHeight * m_game->m_worldCamera.m_perspectiveAspect;

	Vec3 cameraFwd, cameraLeft, cameraUp;
	m_game->m_worldCamera.GetOrientation().GetAsVectors_iFwd_jLeft_kUp(cameraFwd, cameraLeft, cameraUp);

	Vec3 cursorWorldPosition = m_game->m_worldCamera.GetPosition() + cameraFwd * nearClipPlaneDistance;
	cursorWorldPosition -= cameraLeft * cursorScreenX * nearClipPlaneWorldWidth;
	cursorWorldPosition += cameraUp * cursorScreenY * nearClipPlaneWorldHeight;

	Vec3 rayStart = cursorWorldPosition;
	Vec3 rayDirection = (cursorWorldPosition - m_game->m_worldCamera.GetPosition()).GetNormalized();
	float rayMaxDistance = m_game->m_worldCamera.m_perspectiveFar;

	Plane3 groundPlane = Plane3(Vec3::SKYWARD, 0.f);
	RaycastResult3D groundRaycastResult = RaycastVsPlane3(rayStart, rayDirection, rayMaxDistance, groundPlane);

	return groundRaycastResult;
}

IntVec2 Map::GetTileCoordsFromIndex(int tileIndex) const
{
	return IntVec2(tileIndex % m_definition.m_dimensions.x, tileIndex / m_definition.m_dimensions.y);
}

int Map::GetTileIndexFromCoords(IntVec2 const& tileCoords) const
{
	return m_definition.m_dimensions.x * tileCoords.y + tileCoords.x;
}

Vec2 Map::GetTileWorldPositionFromCoordinates(IntVec2 const& tileCoords) const
{
	return GRID_TO_WORLD_TRANSFORM.TransformPosition2D(tileCoords.GetAsVec2());
}

Vec2 Map::GetTileWorldPositionFromIndex(int tileIndex) const
{
	IntVec2 tileCoords = GetTileCoordsFromIndex(tileIndex);
	return GetTileWorldPositionFromCoordinates(tileCoords);
}

int Map::GetHexTaxicabDistance(IntVec2 const& tileCoordsA, IntVec2 const& tileCoordsB) const
{
	return int(fabsf((float)tileCoordsA.x - (float)tileCoordsB.x) + fabsf((float)tileCoordsA.x + (float)tileCoordsA.y - (float)tileCoordsB.x - (float)tileCoordsB.y) + fabsf((float)tileCoordsA.y - (float)tileCoordsB.y)) / 2;
}

void Map::GetAllNeighboringTileCoords(std::vector<IntVec2>& out_neighboringTiles, IntVec2 const& tileCoordsToFindNeighboringTilesFor) const
{
	IntVec2 const neighbor1(0, 1);
	IntVec2 const neighbor2(1, 0);
	IntVec2 const neighbor3(1, -1);
	IntVec2 const neighbor4(0, -1);
	IntVec2 const neighbor5(-1, 0);
	IntVec2 const neighbor6(-1, 1);

	out_neighboringTiles.push_back(tileCoordsToFindNeighboringTilesFor + neighbor1);
	out_neighboringTiles.push_back(tileCoordsToFindNeighboringTilesFor + neighbor2);
	out_neighboringTiles.push_back(tileCoordsToFindNeighboringTilesFor + neighbor3);
	out_neighboringTiles.push_back(tileCoordsToFindNeighboringTilesFor + neighbor4);
	out_neighboringTiles.push_back(tileCoordsToFindNeighboringTilesFor + neighbor5);
	out_neighboringTiles.push_back(tileCoordsToFindNeighboringTilesFor + neighbor6);
}

void Map::GetAllNeighboringTileHeatValues(std::vector<float>& out_heatValues, IntVec2 const& tileCoordsToFindNeighboringHeatValuesFor, TileHeatMap const* heatMap) const
{
	std::vector<IntVec2> neighboringTiles;
	GetAllNeighboringTileCoords(neighboringTiles, tileCoordsToFindNeighboringHeatValuesFor);
	for (int neighborIndex = 0; neighborIndex < (int)neighboringTiles.size(); neighborIndex++)
	{
		IntVec2 const& neighborCoords = neighboringTiles[neighborIndex];

		if (neighborCoords.x < 0 || neighborCoords.x >= m_definition.m_dimensions.x || neighborCoords.y < 0 || neighborCoords.y >= m_definition.m_dimensions.y)
		{
			continue;
		}

		out_heatValues.push_back(heatMap->GetValueAtTile(neighborCoords));
	}
}

void Map::PopuplateDistanceField(std::vector<float>& out_distanceField, IntVec2 const& goalCoords, float specialValue) const
{
	int numTiles = m_definition.m_dimensions.x * m_definition.m_dimensions.y;
	out_distanceField.resize(numTiles);
	for (int distanceIndex = 0; distanceIndex < numTiles; distanceIndex++)
	{
		out_distanceField[distanceIndex] = specialValue;
	}
	
	std::queue<IntVec2> dirtyTileQueue;
	dirtyTileQueue.push(goalCoords);
	int goalIndex = GetTileIndexFromCoords(goalCoords);
	out_distanceField[goalIndex] = 0.f;

	while (!dirtyTileQueue.empty())
	{
		IntVec2 currentTileCoords = dirtyTileQueue.front();
		dirtyTileQueue.pop();
		int currentTileIndex = GetTileIndexFromCoords(currentTileCoords);
		float currentHeatValue = out_distanceField[currentTileIndex];
		std::vector<IntVec2> neighboringTileCoords;
		GetAllNeighboringTileCoords(neighboringTileCoords, currentTileCoords);

		for (int neighborIndex = 0; neighborIndex < (int)neighboringTileCoords.size(); neighborIndex++)
		{
			IntVec2 const& neighborCoords = neighboringTileCoords[neighborIndex];
			if (neighborCoords.x < 0 || neighborCoords.x >= m_definition.m_dimensions.x || neighborCoords.y < 0 || neighborCoords.y >= m_definition.m_dimensions.y)
			{
				continue;
			}

			int neighborTileIndex = GetTileIndexFromCoords(neighborCoords);
			Tile const& neighborTile = m_tiles[neighborTileIndex];

			if (!neighborTile.m_definition.m_isBlocked && out_distanceField[neighborTileIndex] > currentHeatValue + 1.f)
			{
				out_distanceField[neighborTileIndex] = currentHeatValue + 1.f;
				dirtyTileQueue.push(neighborCoords);
			}
		}
	}
}

void Map::DebugRenderDistanceField(TileHeatMap const* heatMap) const
{
	float heatMapMaxValue = -FLT_MAX;
	for (int heatMapIndex = 0; heatMapIndex < (int)heatMap->m_values.size(); heatMapIndex++)
	{
		if (heatMap->m_values[heatMapIndex] == SPECIAL_VALUE_FOR_HEATMAP)
		{
			continue;
		}

		if (heatMap->m_values[heatMapIndex] > heatMapMaxValue)
		{
			heatMapMaxValue = heatMap->m_values[heatMapIndex];
		}
	}

	std::vector<Vertex_PCU> tileVertexes;
	int numTiles = m_definition.m_dimensions.x * m_definition.m_dimensions.y;
	for (int tileIndex = 0; tileIndex < numTiles; tileIndex++)
	{
		Vec2 tilePosition = GetTileWorldPositionFromIndex(tileIndex);

		if (IsPointInsideAABB2(tilePosition, AABB2(m_definition.m_bounds.m_mins.GetXY(), m_definition.m_bounds.m_maxs.GetXY())))
		{
			Rgba8 tileColor = Rgba8::BLUE;
			if (heatMap->m_values[tileIndex] != SPECIAL_VALUE_FOR_HEATMAP)
			{
				tileColor = Interpolate(Rgba8::WHITE, Rgba8::BLACK, heatMap->m_values[tileIndex] / heatMapMaxValue);
			}

			m_tiles[tileIndex].AddHeatVerts(tileVertexes, tilePosition.ToVec3(), tileColor);
		}
	}

	g_renderer->SetBlendMode(BlendMode::ALPHA);
	g_renderer->SetDepthMode(DepthMode::DISABLED);
	g_renderer->SetModelConstants();
	g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_BACK);
	g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
	g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_renderer->BindShader(nullptr);
	g_renderer->BindTexture(nullptr);
	g_renderer->DrawVertexArray(tileVertexes);
}

void Map::GenerateHeatMapPath(std::vector<Vec2>& out_positions, IntVec2 const& sourceCoords, IntVec2 const& destinationCoords, TileHeatMap const* heatMap) const
{
	IntVec2 currentTileCoords = sourceCoords;
	out_positions.push_back(GetTileWorldPositionFromCoordinates(sourceCoords));

	while (currentTileCoords != destinationCoords)
	{
		std::vector<IntVec2> neighboringTileCoords;
		std::vector<float> neighboringTileHeatValues;
		GetAllNeighboringTileCoords(neighboringTileCoords, currentTileCoords);
		float minHeatValue = FLT_MAX;
		IntVec2 minHeatTileCoords = currentTileCoords;

		for (int neighborIndex = 0; neighborIndex < (int)neighboringTileCoords.size(); neighborIndex++)
		{
			IntVec2 const& neighborCoords = neighboringTileCoords[neighborIndex];

			if (neighborCoords.x < 0 || neighborCoords.x >= m_definition.m_dimensions.x || neighborCoords.y < 0 || neighborCoords.y >= m_definition.m_dimensions.y)
			{
				continue;
			}

			float heatValue = heatMap->GetValueAtTile(neighborCoords);
			if (heatValue < minHeatValue)
			{
				minHeatValue = heatValue;
				minHeatTileCoords = neighborCoords;
			}
		}
		currentTileCoords = minHeatTileCoords;
		out_positions.push_back(GetTileWorldPositionFromCoordinates(currentTileCoords));
	}

	//out_positions.push_back(GetTileWorldPositionFromCoordinates(destinationCoords));
	std::reverse(std::begin(out_positions), std::end(out_positions));
}

void Map::GenerateHeatMapPath(std::vector<IntVec2>& out_tileCoordsPath, IntVec2 const& sourceCoords, IntVec2 const& destinationCoords, TileHeatMap const* heatMap) const
{
	IntVec2 currentTileCoords = sourceCoords;
	out_tileCoordsPath.push_back(sourceCoords);

	while (currentTileCoords != destinationCoords)
	{
		std::vector<IntVec2> neighboringTileCoords;
		std::vector<float> neighboringTileHeatValues;
		GetAllNeighboringTileCoords(neighboringTileCoords, currentTileCoords);
		float minHeatValue = heatMap->GetValueAtTile(currentTileCoords);
		IntVec2 minHeatTileCoords = currentTileCoords;

		for (int neighborIndex = 0; neighborIndex < (int)neighboringTileCoords.size(); neighborIndex++)
		{
			IntVec2 const& neighborCoords = neighboringTileCoords[neighborIndex];

			if (neighborCoords.x < 0 || neighborCoords.x >= m_definition.m_dimensions.x || neighborCoords.y < 0 || neighborCoords.y >= m_definition.m_dimensions.y)
			{
				continue;
			}

			float heatValue = heatMap->GetValueAtTile(neighborCoords);
			if (heatValue < minHeatValue)
			{
				minHeatValue = heatValue;
				minHeatTileCoords = neighborCoords;
			}
		}
		currentTileCoords = minHeatTileCoords;
		out_tileCoordsPath.push_back(currentTileCoords);
	}

	//out_tileCoordsPath.push_back(destinationCoords);
	std::reverse(std::begin(out_tileCoordsPath), std::end(out_tileCoordsPath));
}

bool IsPointInsideHex(Vec2 const& referencePoint, Vec2 const& hexCenter, float hexRadius)
{
	Vec2 hexVertexes[6];

	hexVertexes[0] = hexCenter + Vec2::MakeFromPolarDegrees(0.f, hexRadius);
	hexVertexes[1] = hexCenter + Vec2::MakeFromPolarDegrees(60.f, hexRadius);
	hexVertexes[2] = hexCenter + Vec2::MakeFromPolarDegrees(120.f, hexRadius);
	hexVertexes[3] = hexCenter + Vec2::MakeFromPolarDegrees(180.f, hexRadius);
	hexVertexes[4] = hexCenter + Vec2::MakeFromPolarDegrees(240.f, hexRadius);
	hexVertexes[5] = hexCenter + Vec2::MakeFromPolarDegrees(300.f, hexRadius);

	if (!IsPointToLeftOfLine(referencePoint, hexVertexes[0], hexVertexes[1]))
	{
		return false;
	}
	if (!IsPointToLeftOfLine(referencePoint, hexVertexes[1], hexVertexes[2]))
	{
		return false;
	}
	if (!IsPointToLeftOfLine(referencePoint, hexVertexes[2], hexVertexes[3]))
	{
		return false;
	}
	if (!IsPointToLeftOfLine(referencePoint, hexVertexes[3], hexVertexes[4]))
	{
		return false;
	}
	if (!IsPointToLeftOfLine(referencePoint, hexVertexes[4], hexVertexes[5]))
	{
		return false;
	}
	if (!IsPointToLeftOfLine(referencePoint, hexVertexes[5], hexVertexes[0]))
	{
		return false;
	}

	return true;
}

bool IsPointToLeftOfLine(Vec2 const& referencePoint, Vec2 const& lineStart, Vec2 const& lineEnd)
{
	Vec2 dispStartToEnd = lineEnd - lineStart;
	Vec2 linePerpendicular = dispStartToEnd.GetRotated90Degrees();
	Vec2 dispLineStartToPoint = referencePoint - lineStart;
	float pointDistAlongPerpendicular = DotProduct2D(dispLineStartToPoint, linePerpendicular);
	return (pointDistAlongPerpendicular > 0.f);
}
