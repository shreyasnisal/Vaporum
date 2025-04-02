#include "Game/TileDefinition.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

std::map<std::string, TileDefinition> TileDefinition::s_definitions;


TileDefinition::TileDefinition(XmlElement const* element)
{
	m_name = ParseXmlAttribute(*element, "name", m_name);
	m_cdataSymbol = ParseXmlAttribute(*element, "symbol", m_cdataSymbol);
	m_isBlocked = ParseXmlAttribute(*element, "isBlocked", m_isBlocked);
}

void TileDefinition::InitializeTileDefinitions()
{
	XmlDocument definitionsDoc;
	XmlResult result = definitionsDoc.LoadFile("Data/Definitions/TileDefinitions.xml");

	if (result != XmlResult::XML_SUCCESS)
	{
		ERROR_AND_DIE(Stringf("Could not open or read XML file %s", "Data/Definitions/TileDefinitions.xml"));
	}

	XmlElement* tileDefsRootElement = definitionsDoc.RootElement();
	XmlElement* tileDefElement = tileDefsRootElement->FirstChildElement("TileDefinition");
	while (tileDefElement)
	{
		TileDefinition tileDef(tileDefElement);
		s_definitions[tileDef.m_name] = tileDef;
		tileDefElement = tileDefElement->NextSiblingElement();
	}
}

TileDefinition* TileDefinition::GetTileDefinitionFromSymbol(char tileSymbol)
{
    for (auto tileDefIter = s_definitions.begin(); tileDefIter != s_definitions.end(); ++tileDefIter)
	{
		if (tileDefIter->second.m_cdataSymbol == tileSymbol)
		{
			return &tileDefIter->second;
		}
	}

	return nullptr;
}
