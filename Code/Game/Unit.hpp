#pragma once

#include "Game/UnitDefinition.hpp"

#include "Engine/Core/HeatMaps/TileHeatMap.hpp"
#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Splines.hpp"
#include "Engine/Math/Vec3.hpp"


class Map;
class Player;


class Unit
{
public:
	~Unit() = default;
	Unit() = default;
	Unit(UnitDefinition const& definition, Map* map, IntVec2 const& tileCoords, Vec3 const& position, EulerAngles const& orientation, Player* owner);

	void Update();
	void Render() const;

	void Move(IntVec2 const& newTileCoords);
	void Attack(Unit* targetUnit);
	void HoldFire();
	void Cancel();

	void TakeDamage(int damage, Vec3 const& hitDirection);
	void Die();

public:
	static constexpr float TURN_SPEED_PER_SECOND = 90.f;
	static constexpr float MOVE_SPEED = 4.f;

	UnitDefinition m_definition;
	Map* m_map = nullptr;
	int m_health = 0;
	Vec3 m_position = Vec3::ZERO;
	EulerAngles m_defaultOrientation = EulerAngles::ZERO;
	EulerAngles m_orientation = EulerAngles::ZERO;
	Player* m_owner = nullptr;
	IntVec2 m_tileCoords = IntVec2::ZERO;
	bool m_isDead = false;
	bool m_isGarbage = false;
	bool m_isSelected = false;
	IntVec2 m_previousTileCoords = IntVec2::ZERO;
	IntVec2 m_currentTileCoords = IntVec2::ZERO;
	bool m_didMove = false;
	bool m_ordersIssued = false;
	TileHeatMap* m_heatMap = nullptr;

	bool m_isMoving = false;
	int m_pathLength = -1;
	CatmullRomSpline* m_pathSpline = nullptr;
	Stopwatch* m_movementTimer = nullptr;

	std::string m_floatingDamageStr = "";
	Stopwatch* m_floatingDamageTimer = nullptr;
};
