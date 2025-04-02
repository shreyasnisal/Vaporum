#pragma once

#include "Engine/Core/XMLUtils.hpp"

#include <map>


class TileDefinition
{
public:
	~TileDefinition() = default;
	TileDefinition() = default;
	TileDefinition(TileDefinition const& copyFrom) = default;
	TileDefinition(XmlElement const* element);

public:
	char m_cdataSymbol = ' ';
	std::string m_name = "";
	bool m_isBlocked = false;

public:
	static std::map<std::string, TileDefinition> s_definitions;
	static void InitializeTileDefinitions();
	static TileDefinition* GetTileDefinitionFromSymbol(char tileSymbol);
};
