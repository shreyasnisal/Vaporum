#include "Game/MapDefinition.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"


std::map<std::string, MapDefinition> MapDefinition::s_defs;


MapDefinition::MapDefinition(XmlElement const* element)
{
	m_name = ParseXmlAttribute(*element, "name", m_name);
	m_dimensions = ParseXmlAttribute(*element, "gridSize", m_dimensions);
	Vec3 boundsMins = ParseXmlAttribute(*element, "worldBoundsMin", Vec3::ZERO);
	Vec3 boundsMaxs = ParseXmlAttribute(*element, "worldBoundsMax", Vec3::ZERO);
	m_bounds = AABB3(boundsMins, boundsMaxs);

	int numTiles = m_dimensions.x * m_dimensions.y;
	XmlElement const* tilesElement = element->FirstChildElement("Tiles");
	if (tilesElement)
	{
		char const* tilesData = tilesElement->FirstChild()->Value();
		std::string tempTilesData = tilesData;
		StripString(tempTilesData, '\n');
		StripString(tempTilesData, ' ');

		for (int tileDataIndex = 0; tileDataIndex < numTiles; tileDataIndex++)
		{
			IntVec2 tileCoords(tileDataIndex % m_dimensions.x, tileDataIndex / m_dimensions.y);
			int cdataIndex = (m_dimensions.y - tileCoords.y - 1) * m_dimensions.x + (tileCoords.x);
			m_tilesData.push_back(tempTilesData[cdataIndex]);
		}
	}

	XmlElement const* unitsElement = element->FirstChildElement("Units");
	while (unitsElement)
	{
		int player = ParseXmlAttribute(*unitsElement, "player", 0) - 1;
		char const* unitsData = unitsElement->FirstChild()->Value();
		std::string tempUnitsData = unitsData;
		std::string unitsDataStr;
		StripString(tempUnitsData, '\n');
		StripString(tempUnitsData, ' ');

		for (int unitDataIndex = 0; unitDataIndex < numTiles; unitDataIndex++)
		{
			IntVec2 tileCoords(unitDataIndex % m_dimensions.x, unitDataIndex / m_dimensions.y);
			int cdataIndex = (m_dimensions.y - tileCoords.y - 1) * m_dimensions.x + tileCoords.x;
			unitsDataStr.push_back(tempUnitsData[cdataIndex]);
		}

		m_unitsDataByPlayer[player] = unitsDataStr;
		unitsElement = unitsElement->NextSiblingElement("Units");
	}
}

void MapDefinition::InitializeMapDefinitions()
{
	XmlDocument definitionsDoc;
	XmlResult result = definitionsDoc.LoadFile("Data/Definitions/MapDefinitions.xml");

	if (result != XmlResult::XML_SUCCESS)
	{
		ERROR_AND_DIE(Stringf("Could not open or read XML file %s", "Data/Definitions/MapDefinitions.xml"));
	}

	XmlElement* mapDefsRootElement = definitionsDoc.RootElement();
	XmlElement* mapDefElement = mapDefsRootElement->FirstChildElement("MapDefinition");
	while (mapDefElement)
	{
		MapDefinition mapDef(mapDefElement);
		s_defs[mapDef.m_name] = mapDef;
		mapDefElement = mapDefElement->NextSiblingElement();
	}
}
