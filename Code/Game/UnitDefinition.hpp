#pragma once

#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Core/Models/Model.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Math/IntRange.hpp"

#include <map>
#include <string>


enum class UnitType
{
	NONE = -1,
	TANK,
	ARTILLERY,
	NUM
};

UnitType GetUnitTypeFromString(std::string unitTypeStr);

class UnitDefinition
{
public:
	~UnitDefinition() = default;
	UnitDefinition() = default;
	UnitDefinition(XmlElement const* element);

public:
	std::string m_name = "";
	char m_symbol = ' ';
	std::string m_imagePath = "";
	Texture* m_image = nullptr;
	Model* m_model = nullptr;
	UnitType m_type = UnitType::NONE;
	int m_attackDamage = 0;
	IntRange m_attackRange = IntRange::ZERO;
	int m_movementRange = 0;
	int m_defense = 0;
	int m_maxHealth = 0;
	Vec3 m_muzzleOffset = Vec3::ZERO;

	SoundID m_fireSFX = MISSING_SOUND_ID;
	SoundID m_hitSFX = MISSING_SOUND_ID;
	SoundID m_deathSFX = MISSING_SOUND_ID;

public:
	static void InitializeUnitDefinitions();
	static std::map<std::string, UnitDefinition> s_definitions;
};
