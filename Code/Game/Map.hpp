#pragma once

#include "Game/MapDefinition.hpp"
#include "Game/Tile.hpp"

#include "Engine/Core/HeatMaps/TileHeatMap.hpp"
#include "Engine/Core/Models/Material.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"


class Game;
class Unit;


bool IsPointInsideHex(Vec2 const& referencePoint, Vec2 const& hexCenter, float hexRadius);
bool IsPointToLeftOfLine(Vec2 const& referencePoint, Vec2 const& lineStart, Vec2 const& lineEnd);

class Map
{
public:
	~Map();
	Map(Game* game, MapDefinition const& definition);
	void LoadAssets();

	void Update();
	void Render() const;

	RaycastResult3D RaycastCursorVsMap();

	IntVec2 GetTileCoordsFromIndex(int tileIndex) const;
	int GetTileIndexFromCoords(IntVec2 const& tileCoords) const;
	Vec2 GetTileWorldPositionFromCoordinates(IntVec2 const& tileCoords) const;
	Vec2 GetTileWorldPositionFromIndex(int tileIndex) const;
	int GetHexTaxicabDistance(IntVec2 const& tileCoordsA, IntVec2 const& tileCoordsB) const;

	void GetAllNeighboringTileCoords(std::vector<IntVec2>& out_neighboringTiles, IntVec2 const& tileCoordsToFindNeighboringTilesFor) const;
	void GetAllNeighboringTileHeatValues(std::vector<float>& out_heatValues, IntVec2 const& tileCoordsToFindNeighboringHeatValuesFor, TileHeatMap const* heatMap) const;

	void PopuplateDistanceField(std::vector<float>& out_distanceField, IntVec2 const& goalCoords, float specialValue = 9999.f) const;
	void DebugRenderDistanceField(TileHeatMap const* heatMap) const;
	void GenerateHeatMapPath(std::vector<Vec2>& out_positions, IntVec2 const& sourceCoords, IntVec2 const& destinationCoords, TileHeatMap const* heatMap) const;
	void GenerateHeatMapPath(std::vector<IntVec2>& out_tileCoordsPath, IntVec2 const& sourceCoords, IntVec2 const& destinationCoords, TileHeatMap const* heatMap) const;

public:
	static inline const Vec2 HEX_GRID_IBASIS = Vec2(0.866f, 0.5f);
	static inline const Vec2 HEX_GRID_JBASIS = Vec2(0.f, 1.f);
	static inline const Mat44 GRID_TO_WORLD_TRANSFORM = Mat44(HEX_GRID_IBASIS, HEX_GRID_JBASIS, Vec2::ZERO);
	static constexpr float SPECIAL_VALUE_FOR_HEATMAP = 9999.f;

	Game* m_game = nullptr;
	MapDefinition m_definition;

	Material m_moonMaterial;

	std::vector<Tile> m_tiles;
	IntVec2 m_hoveredTile = IntVec2(-1, -1);

	VertexBuffer* m_mapVBO = nullptr;
	VertexBuffer* m_tilesVBO = nullptr;

	std::vector<Unit*> m_units;

	bool m_debugDraw = false;
};

