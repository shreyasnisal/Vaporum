#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/Shader.hpp"

#include <vector>

class Game;
class Map;
class Unit;


enum class TurnState
{
	WAITING_FOR_TURN,
	NO_SELECTION,
	UNIT_SELECTED_MOVE,
	UNIT_SELECTED_ATTACK,
	END_TURN
};

enum class NetState
{
	NONE = -1,
	LOCAL,
	REMOTE,
};

class Player
{
public:
	~Player() = default;
	Player() = default;
	explicit Player(Game* game, int playerIndex, NetState netState);

	void InitializeUnits(Map* map);

	void Update();
	void Render() const;
	void DebugRender() const;

	Rgba8 const GetTeamColor();
	Unit* GetUnitFromTileCoords(IntVec2 const& tileCoords) const;

	void DeleteGarbageUnits();
	void EndTurn();

public:
	Game* m_game = nullptr;
	int m_playerIndex = -1;
	NetState m_netState = NetState::NONE;
	TurnState m_turnState = TurnState::WAITING_FOR_TURN;
	std::vector<Unit*> m_units;
	Unit* m_selectedUnit = nullptr;
	Shader* m_diffuseShader = nullptr;
	bool m_isAlive = true;
};
