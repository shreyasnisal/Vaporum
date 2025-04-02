#include "Game/UnitDefinition.hpp"

#include "Game/GameCommon.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Models/ModelLoader.hpp"
#include "Engine/Renderer/Renderer.hpp"


std::map<std::string, UnitDefinition> UnitDefinition::s_definitions;


UnitDefinition::UnitDefinition(XmlElement const* element)
{
	m_name = ParseXmlAttribute(*element, "name", m_name);
	m_imagePath = ParseXmlAttribute(*element, "imageFilename", "");
	if (!m_imagePath.empty())
	{
		m_image = g_renderer->CreateOrGetTextureFromFile(m_imagePath.c_str());
	}
	std::string modelFilename = ParseXmlAttribute(*element, "modelFilename", "");
	if (!modelFilename.empty())
	{
		XmlDocument modelDoc;
		XmlResult result = modelDoc.LoadFile(modelFilename.c_str());
		if (result != XmlResult::XML_SUCCESS)
		{
			ERROR_AND_DIE(Stringf("Could not open or read file \"%s\"", modelFilename.c_str()));
		}
		XmlElement const* modelElement = modelDoc.RootElement();
		m_model = g_modelLoader->CreateOrGetModelFromXml(modelElement);
	}

	m_symbol = ParseXmlAttribute(*element, "symbol", ' ');
	std::string unitTypeStr = ParseXmlAttribute(*element, "type", "");
	m_type = GetUnitTypeFromString(unitTypeStr);
	m_attackDamage = ParseXmlAttribute(*element, "groundAttackDamage", m_attackDamage);
	int attackRangeMin = ParseXmlAttribute(*element, "groundAttackRangeMin", 0);
	int attackRangeMax = ParseXmlAttribute(*element, "groundAttackRangeMax", 0);
	m_attackRange = IntRange(attackRangeMin, attackRangeMax);
	m_movementRange = ParseXmlAttribute(*element, "movementRange", m_movementRange);
	m_defense = ParseXmlAttribute(*element, "defense", m_defense);
	m_maxHealth = ParseXmlAttribute(*element, "health", m_maxHealth);

	std::string hitSoundPath = ParseXmlAttribute(*element, "hitAudioFilename", "");
	if (!hitSoundPath.empty())
	{
		m_hitSFX = g_audio->CreateOrGetSound(hitSoundPath);
	}

	std::string fireSoundPath = ParseXmlAttribute(*element, "shotAudioFilename", "");
	if (!fireSoundPath.empty())
	{
		m_fireSFX = g_audio->CreateOrGetSound(fireSoundPath);
	}

	std::string deathSoundPath = ParseXmlAttribute(*element, "explosionAudioFilename", "");
	if (!deathSoundPath.empty())
	{
		m_deathSFX = g_audio->CreateOrGetSound(deathSoundPath);
	}

	m_muzzleOffset = ParseXmlAttribute(*element, "muzzlePosition", m_muzzleOffset);
}

void UnitDefinition::InitializeUnitDefinitions()
{
	XmlDocument unitDefsDoc;
	XmlResult result = unitDefsDoc.LoadFile("Data/Definitions/UnitDefinitions.xml");
	if (result != XmlResult::XML_SUCCESS)
	{
		ERROR_AND_DIE("Could not open or read UnitDefinitions.xml");
	}

	XmlElement const* unitDefsRootElem = unitDefsDoc.RootElement();
	XmlElement const* unitDefElem = unitDefsRootElem->FirstChildElement("UnitDefinition");
	while (unitDefElem)
	{
		UnitDefinition unitDef(unitDefElem);
		s_definitions[unitDef.m_name] = unitDef;

		unitDefElem = unitDefElem->NextSiblingElement();
	}
}

UnitType GetUnitTypeFromString(std::string unitTypeStr)
{
	if (!strcmp(unitTypeStr.c_str(), "Tank"))
	{
		return UnitType::TANK;
	}
	if (!strcmp(unitTypeStr.c_str(), "Artillery"))
	{
		return UnitType::ARTILLERY;
	}
	return UnitType::NONE;
}
