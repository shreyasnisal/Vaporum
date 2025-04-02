#include "Game/Tile.hpp"

#include "Engine/Core/VertexUtils.hpp"


Tile::Tile(TileDefinition const& definition)
	: m_definition(definition)
{
}

void Tile::AddVerts(std::vector<Vertex_PCU>& verts, Vec3 const& position) const
{
	if (m_definition.m_isBlocked)
	{
		//AddVertsForRing3D(verts, position, HEX_RADIUS, 0.02f, EulerAngles::ZERO, Rgba8::WHITE, 6);
		AddVertsForDisc3D(verts, position, HEX_RADIUS, Rgba8::BLACK, 6);
	}
	else
	{
		AddVertsForRing3D(verts, position, HEX_RADIUS, 0.04f, EulerAngles::ZERO, Rgba8::WHITE, 6);
	}
}

void Tile::AddVertsForHover(std::vector<Vertex_PCU>& verts, Vec3 const& position) const
{
	if (m_isHovered && !m_definition.m_isBlocked)
	{
		AddVertsForRing3D(verts, position, HEX_RADIUS * 0.8f, 0.04f, EulerAngles::ZERO, Rgba8::WHITE, 6);
	}
}

void Tile::AddVertsForHighlight(std::vector<Vertex_PCU>& verts, Vec3 const& position) const
{
	if (!m_definition.m_isBlocked)
	{
		AddVertsForRing3D(verts, position, HEX_RADIUS, 0.06f, EulerAngles::ZERO, Rgba8::WHITE, 6);
		AddVertsForDisc3D(verts, position, HEX_RADIUS, Rgba8(255, 255, 255, 127), 6);
	}
}

void Tile::AddVertsForPathHighlight(std::vector<Vertex_PCU>& verts, Vec3 const& position) const
{
	if (!m_definition.m_isBlocked)
	{
		AddVertsForRing3D(verts, position, HEX_RADIUS, 0.08f, EulerAngles::ZERO, Rgba8::WHITE, 6);
		AddVertsForDisc3D(verts, position, HEX_RADIUS, Rgba8(255, 255, 255, 127), 6);
	}
}

void Tile::AddVertsForAttackHighlight(std::vector<Vertex_PCU>& verts, Vec3 const& position) const
{
	if (m_isHovered)
	{
		AddVertsForDisc3D(verts, position, HEX_RADIUS * 0.8f, Rgba8::MAROON, 6);
	}
}

void Tile::AddHeatVerts(std::vector<Vertex_PCU>& verts, Vec3 const& position, Rgba8 const& color) const
{
	AddVertsForDisc3D(verts, position, HEX_RADIUS, color, 6);
}
