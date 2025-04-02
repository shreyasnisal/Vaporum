#pragma once

#include "Game/TileDefinition.hpp"

#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/MathUtils.hpp"

#include <vector>


class Tile
{
public:
	~Tile() = default;
	Tile() = default;
	Tile(Tile const& copyFrom) = default;
	Tile(TileDefinition const& definition);

	void AddVerts(std::vector<Vertex_PCU>& verts, Vec3 const& position) const;
	void AddVertsForHover(std::vector<Vertex_PCU>& verts, Vec3 const& position) const;
	void AddVertsForHighlight(std::vector<Vertex_PCU>& verts, Vec3 const& position) const;
	void AddVertsForPathHighlight(std::vector<Vertex_PCU>& verts, Vec3 const& position) const;
	void AddVertsForAttackHighlight(std::vector<Vertex_PCU>& verts, Vec3 const& position) const;
	void AddHeatVerts(std::vector<Vertex_PCU>& verts, Vec3 const& position, Rgba8 const& color) const;

public:
	static inline const float HEX_RADIUS = 1.f / sqrtf(3);

	TileDefinition m_definition;
	bool m_isHovered = false;
};
