#pragma once

#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Shader.hpp"

#include <map>
#include <string>


class MapDefinition
{
public:
	~MapDefinition() = default;
	MapDefinition() = default;
	MapDefinition(XmlElement const* element);

public:
	std::string m_name = "";
	Shader* m_shader = nullptr;
	IntVec2 m_dimensions = IntVec2::ZERO;
	AABB3 m_bounds;
	std::string m_tilesData;
	std::map<int, std::string> m_unitsDataByPlayer;
	
public:
	static std::map<std::string, MapDefinition> s_defs;
	static void InitializeMapDefinitions();
};

