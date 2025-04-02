#pragma once

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/UI/UIWidget.hpp"

#include "Game/GameCommon.hpp"


class App;
class Map;
class Player;
class Particle;


enum class GameState
{
	NONE = -1,

	INTRO,
	ATTRACT,
	MENU,
	LOBBY,
	GAME,
	PAUSED,
};

enum class GameType
{
	NONE = -1,
	LOCAL,
	NETWORK,
};

class Game
{
public:
	~Game();
	Game();
	void						Update												();
	void						Render												() const;

	static bool					Event_RemoteCommand									(EventArgs& args);
	static bool					Event_BurstTest										(EventArgs& args);
	static bool					Event_RemoteHelp									(EventArgs& args);
	static bool					Event_LoadMap										(EventArgs& args);

	static bool					Event_PlayerReady(EventArgs& args);
	static bool					Event_StartTurn(EventArgs& args);
	static bool					Event_SetFocusedHexCoords(EventArgs& args);
	static bool					Event_SelectFocusedUnit(EventArgs& args);
	static bool					Event_SelectPreviousUnit(EventArgs& args);
	static bool					Event_SelectNextUnit(EventArgs& args);
	static bool					Event_Move(EventArgs& args);
	static bool					Event_Stay(EventArgs& args);
	static bool					Event_HoldFire(EventArgs& args);
	static bool					Event_Attack(EventArgs& args);
	static bool					Event_Cancel(EventArgs& args);
	static bool					Event_EndTurn(EventArgs& args);
	static bool					Event_PlayerQuit(EventArgs& args);
	static bool					Event_NetworkDisconnected(EventArgs& args);

	static bool					ButtonEvent_StartLocalGame(EventArgs& args);
	static bool					ButtonEvent_StartNetworkGame(EventArgs& args);
	static bool					ButtonEvent_ResumeGame(EventArgs& args);
	static bool					ButtonEvent_ReturnToMenu(EventArgs& args);

	void						StartTurn();
	void						SetFocusedHex(IntVec2 const& hexCoords);
	void						SelectFocusedUnit();
	void						SelectPreviousUnit();
	void						SelectNextUnit();
	void						Move(IntVec2 const& tileCoords);
	void						Stay();
	void						HoldFire();
	void						Attack();
	void						Cancel();
	void						EndTurn();
	void						PlayerQuit();
	void						CheckGameEnd();

	Player* GetCurrentPlayer() const;
	Player* GetWaitingPlayer() const;
	Player* GetLocalPlayer() const;

	void SpawnParticle(Vec3 const& startPos, Vec3 const& velocity, float rotation, float rotationSpeed, float size, float lifetime, std::string const& textureName, Rgba8 const& color, float startAlpha, float endAlpha, float startAlphaTime, float endAlphaTime, float startScale, float endScale, float startScaleTime, float endScaleTime, float startSpeedMultiplier, float endSpeedMultiplier, float startSpeedTime, float endSpeedTime);

public:	
	bool						m_isPaused											= false;
	bool						m_drawDebug											= false;

	Camera						m_worldCamera;
	Camera						m_screenCamera;

	// Change gameState to INTRO to show intro screen
	// BEWARE: The intro screen spritesheet is a large texture and the game will take much longer to load
	GameState					m_gameState											= GameState::NONE;
	GameState					m_nextGameState										= GameState::ATTRACT;
	GameType					m_gameType											= GameType::NONE;
	Clock						m_gameClock = Clock();

	Map* m_currentMap = nullptr;

	Vec3 m_playerPosition = Vec3(5.f, 4.f, 5.f);

	EulerAngles m_sunOrientation = EulerAngles(45.f, 45.f, 0.f);
	float m_sunIntensity = 0.5f;

	Player* m_player1;
	Player* m_player2;

	bool m_isLocalPlayerReady = false;
	bool m_isRemotePlayerReady = false;

	UIWidget* m_turnWidget = nullptr;
	UIWidget* m_inputInfoWidget = nullptr;
	UIWidget* m_p1UnitInfoWidget = nullptr;
	UIWidget* m_p1UnitImageWidget = nullptr;
	UIWidget* m_p1UnitInfoContainerWidget = nullptr;
	UIWidget* m_p2UnitInfoWidget = nullptr;
	UIWidget* m_p2UnitImageWidget = nullptr;
	UIWidget* m_p2UnitInfoContainerWidget = nullptr;
	UIWidget* m_endgameWidget = nullptr;

	bool m_hasGameEnded = false;

	bool m_isAnimationPlaying = false;
	std::vector<std::string> m_commandsQueue;

	std::vector<Particle> m_particles;

public:
	static const inline EulerAngles FIXED_CAMERA_ANGLE = EulerAngles(90.f, 60.f, 0.f);

private:

	void						UpdateIntroScreen									(float deltaSeconds);
	void						UpdateAttractScreen									(float deltaSeconds);
	void						UpdateMenu											(float deltaSeconds);
	void						UpdateLobby											(float deltaSeconds);
	void						UpdateGame											(float deltaSeconds);
	void						UpdatePauseMenu										(float deltaSeconds);

	void						RenderIntroScreen									() const;
	void						RenderAttractScreen									() const;
	void						RenderMenu											() const;
	void						RenderLobby											() const;
	void						RenderGame											() const;
	void						RenderPauseMenu										() const;

	void						EnterAttract										();
	void						ExitAttract											();
	void						EnterMenu											();
	void						ExitMenu											();
	void						EnterLobby											();
	void						ExitLobby											();
	void						EnterGame											();
	void						ExitGame											();
	void						EnterPauseMenu										();
	void						ExitPauseMenu										();
	void						HandleGameStateChange								();

	void						LoadAssets											();
	void						SortParticles										();

private:
	static constexpr int BURST_SIZE = 20;

	Texture*					m_logoTexture										= nullptr;
	Texture*					m_gameLogoTexture									= nullptr;

	std::vector<Vertex_PCU>		m_gridStaticVerts;

	float						m_timeInState										= 0.f;
};