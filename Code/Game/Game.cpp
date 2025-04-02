#include "Game/Game.hpp"

#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/MapDefinition.hpp"
#include "Game/Player.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/Unit.hpp"
#include "Game/UnitDefinition.hpp"
#include "Game/Particle.hpp"

#include "Engine/Core/DevConsole.hpp"
#include "Engine/Networking/NetSystem.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Renderer/Spritesheet.hpp"
#include "Engine/UI/UISystem.hpp"


bool Game::Event_RemoteCommand(EventArgs& args)
{
	std::string command = args.GetValue("command", "");
	if (!command.empty())
	{
		g_netSystem->QueueMessageForSend(command);
		return true;
	}

	g_console->AddLine(DevConsole::WARNING, "Unable to send remote command", false);
	return false;
}

bool Game::Event_BurstTest(EventArgs& args)
{
	UNUSED(args);

	g_netSystem->QueueMessageForSend("@echo off");
	for (int i = 1; i <= BURST_SIZE; i++)
	{
		g_netSystem->QueueMessageForSend(Stringf("echo %d", i));
	}
	g_netSystem->QueueMessageForSend("@echo on");

	return true;
}

bool Game::Event_RemoteHelp(EventArgs& args)
{
	UNUSED(args);

	std::map<std::string, std::string, cmpCaseInsensitive> commandsList;
	commandsList = g_eventSystem->GetAllCommandsList();

	g_netSystem->QueueMessageForSend("@echo off");
	for (auto mapIter = commandsList.begin(); mapIter != commandsList.end(); ++mapIter)
	{
		if (!strcmp(mapIter->first.c_str(), "WM_CHAR") ||
			!strcmp(mapIter->first.c_str(), "WM_KEYDOWN") ||
			!strcmp(mapIter->first.c_str(), "WM_KEYUP") || 
			!strcmp(mapIter->first.c_str(), "WM_MOUSEWHEEL"))
		{
			continue;
		}

		g_netSystem->QueueMessageForSend(Stringf("echo %-10s : %s", mapIter->first.c_str(), mapIter->second.c_str()));
	}
	g_netSystem->QueueMessageForSend("@echo on");

	return true;
}

bool Game::Event_LoadMap(EventArgs& args)
{
	std::string mapName = args.GetValue("name", "");

	if (!mapName.empty())
	{
		auto mapDefIter = MapDefinition::s_defs.find(mapName);

		if (mapDefIter == MapDefinition::s_defs.end())
		{
			g_console->AddLine(DevConsole::WARNING, Stringf("Could not find map \"%s\"!", mapName.c_str()));
			return true;
		}

		delete g_app->m_game->m_currentMap;
		g_app->m_game->m_currentMap = new Map(g_app->m_game, mapDefIter->second);
	}

	return true;
}

bool Game::Event_PlayerReady(EventArgs& args)
{
	UNUSED(args);

	g_app->m_game->m_isRemotePlayerReady = true;

	return true;
}

bool Game::Event_StartTurn(EventArgs& args)
{
	UNUSED(args);

	return false;
}

bool Game::Event_SetFocusedHexCoords(EventArgs& args)
{
	IntVec2 hexCoords = args.GetValue("hexCoords", IntVec2(-1, -1));
	if (hexCoords == IntVec2(-1, -1))
	{
		return false;
	}

	g_app->m_game->SetFocusedHex(hexCoords);

	return true;
}

bool Game::Event_SelectFocusedUnit(EventArgs& args)
{
	UNUSED(args);

	g_app->m_game->SelectFocusedUnit();

	return true;
}

bool Game::Event_SelectPreviousUnit(EventArgs& args)
{
	UNUSED(args);

	g_app->m_game->SelectPreviousUnit();

	return true;
}

bool Game::Event_SelectNextUnit(EventArgs& args)
{
	UNUSED(args);

	g_app->m_game->SelectNextUnit();

	return true;
}

bool Game::Event_Move(EventArgs& args)
{
	IntVec2 hexCoords = args.GetValue("hexCoords", IntVec2(-1, -1));
	if (hexCoords == IntVec2(-1, -1))
	{
		return false;
	}

	g_app->m_game->Move(hexCoords);

	return true;
}

bool Game::Event_Stay(EventArgs& args)
{
	UNUSED(args);

	g_app->m_game->Stay();

	return true;
}

bool Game::Event_HoldFire(EventArgs& args)
{
	UNUSED(args);

	g_app->m_game->HoldFire();

	return true;
}

bool Game::Event_Attack(EventArgs& args)
{
	UNUSED(args);

	g_app->m_game->Attack();

	return true;
}

bool Game::Event_Cancel(EventArgs& args)
{
	UNUSED(args);

	g_app->m_game->Cancel();

	return true;
}

bool Game::Event_EndTurn(EventArgs& args)
{
	UNUSED(args);

	g_app->m_game->EndTurn();

	return true;
}

bool Game::Event_PlayerQuit(EventArgs& args)
{
	UNUSED(args);

	g_app->m_game->PlayerQuit();

	return true;
}

bool Game::Event_NetworkDisconnected(EventArgs& args)
{
	UNUSED(args);
	FireEvent("PlayerQuit");
	return true;
}

bool Game::ButtonEvent_StartLocalGame(EventArgs& args)
{
	UNUSED(args);

	g_app->m_game->m_gameType = GameType::LOCAL;
	g_app->m_game->m_nextGameState = GameState::LOBBY;

	return true;
}

bool Game::ButtonEvent_StartNetworkGame(EventArgs& args)
{
	UNUSED(args);

	g_app->m_game->m_gameType = GameType::NETWORK;
	g_app->m_game->m_nextGameState = GameState::LOBBY;

	return true;
}

bool Game::ButtonEvent_ResumeGame(EventArgs& args)
{
	UNUSED(args);

	g_app->m_game->m_nextGameState = GameState::GAME;

	return true;
}

bool Game::ButtonEvent_ReturnToMenu(EventArgs& args)
{
	UNUSED(args);

	if (g_app->m_game->m_gameType == GameType::NETWORK)
	{
		g_netSystem->QueueMessageForSend("PlayerQuit");
	}
	g_app->m_game->m_nextGameState = GameState::MENU;
	return true;
}

void Game::StartTurn()
{
}

void Game::SetFocusedHex(IntVec2 const& hexCoords)
{
	for (int tileIndex = 0; tileIndex < (int)m_currentMap->m_tiles.size(); tileIndex++)
	{
		m_currentMap->m_tiles[tileIndex].m_isHovered = false;
	}

	int tileIndex = m_currentMap->GetTileIndexFromCoords(hexCoords);
	m_currentMap->m_tiles[tileIndex].m_isHovered = true;
	m_currentMap->m_hoveredTile = hexCoords;
}

void Game::SelectFocusedUnit()
{
	Player* currentPlayer = GetCurrentPlayer();
	if (currentPlayer->m_selectedUnit)
	{
		currentPlayer->m_selectedUnit->m_isSelected = false;
	}

	Unit* hoveredUnit = currentPlayer->GetUnitFromTileCoords(m_currentMap->m_hoveredTile);
	if (hoveredUnit && !hoveredUnit->m_ordersIssued)
	{
		hoveredUnit->m_isSelected = true;
		currentPlayer->m_turnState = TurnState::UNIT_SELECTED_MOVE;
		currentPlayer->m_selectedUnit = hoveredUnit;
	}
}

void Game::SelectPreviousUnit()
{
}

void Game::SelectNextUnit()
{
}

void Game::Move(IntVec2 const& tileCoords)
{
	Player* currentPlayer = GetCurrentPlayer();
	Player* waitingPlayer = GetWaitingPlayer();

	for (int unitIndex = 0; unitIndex < (int)currentPlayer->m_units.size(); unitIndex++)
	{
		if (currentPlayer->m_units[unitIndex]->m_tileCoords == tileCoords)
		{
			return;
		}
	}
	for (int unitIndex = 0; unitIndex < (int)waitingPlayer->m_units.size(); unitIndex++)
	{
		if (waitingPlayer->m_units[unitIndex]->m_tileCoords == tileCoords)
		{
			return;
		}
	}

	int tileIndex = m_currentMap->GetTileIndexFromCoords(tileCoords);
	if (m_currentMap->m_tiles[tileIndex].m_definition.m_isBlocked)
	{
		return;
	}

	if (m_currentMap->GetHexTaxicabDistance(tileCoords, currentPlayer->m_selectedUnit->m_tileCoords) > currentPlayer->m_selectedUnit->m_definition.m_movementRange)
	{
		return;
	}

	currentPlayer->m_selectedUnit->Move(tileCoords);
	currentPlayer->m_turnState = TurnState::UNIT_SELECTED_ATTACK;
}

void Game::Stay()
{
	Player* currentPlayer = GetCurrentPlayer();
	currentPlayer->m_turnState = TurnState::UNIT_SELECTED_ATTACK;
}

void Game::HoldFire()
{
	Player* currentPlayer = GetCurrentPlayer();
	currentPlayer->m_selectedUnit->HoldFire();
	currentPlayer->m_selectedUnit = nullptr;
	currentPlayer->m_turnState = TurnState::NO_SELECTION;
}

void Game::Attack()
{
	Player* currentPlayer = GetCurrentPlayer();
	Player* waitingPlayer = GetWaitingPlayer();

	int attackDistance = m_currentMap->GetHexTaxicabDistance(currentPlayer->m_selectedUnit->m_tileCoords, m_currentMap->m_hoveredTile);
	if (attackDistance < currentPlayer->m_selectedUnit->m_definition.m_attackRange.m_min || attackDistance > currentPlayer->m_selectedUnit->m_definition.m_attackRange.m_max)
	{
		return;
	}

	Unit* targetUnit =  waitingPlayer->GetUnitFromTileCoords(m_currentMap->m_hoveredTile);
	if (!targetUnit)
	{
		return;
	}

	if (targetUnit->m_definition.m_type == UnitType::ARTILLERY && targetUnit->m_didMove)
	{
		return;
	}

	currentPlayer->m_selectedUnit->Attack(targetUnit);
	currentPlayer->m_selectedUnit = nullptr;
	currentPlayer->m_turnState = TurnState::NO_SELECTION;
}

void Game::Cancel()
{
	Player* currentPlayer = GetCurrentPlayer();
	if (currentPlayer->m_turnState == TurnState::NO_SELECTION || currentPlayer->m_turnState == TurnState::END_TURN)
	{
		return;
	}
	if (currentPlayer->m_turnState == TurnState::UNIT_SELECTED_MOVE)
	{
		currentPlayer->m_selectedUnit->m_isSelected = false;
		currentPlayer->m_selectedUnit = nullptr;
		currentPlayer->m_turnState = TurnState::NO_SELECTION;
		return;
	}

	currentPlayer->m_selectedUnit->Cancel();
	currentPlayer->m_turnState = TurnState::UNIT_SELECTED_MOVE;
}

void Game::EndTurn()
{
	Player* currentPlayer = GetCurrentPlayer();
	Player* waitingPlayer = GetWaitingPlayer();
	currentPlayer->EndTurn();
	CheckGameEnd();
	if (!m_hasGameEnded)
	{
		waitingPlayer->m_turnState = TurnState::NO_SELECTION;
	}
	g_input->HandleKeyReleased('Y');
}

void Game::PlayerQuit()
{
	Player* localPlayer = GetLocalPlayer();

	m_hasGameEnded = true;
	if (m_endgameWidget)
	{
		m_endgameWidget->SetText(Stringf("Player %d Wins!", localPlayer->m_playerIndex + 1));
		m_endgameWidget->SetVisible(true);
	}
	else
	{
		m_nextGameState = GameState::MENU;
	}
}

void Game::CheckGameEnd()
{
	bool isPlayer1Alive = false;
	bool isPlayer2Alive = false;

	for (int unitIndex = 0; (int)unitIndex < m_player1->m_units.size(); unitIndex++)
	{
		if (!m_player1->m_units[unitIndex]->m_isDead)
		{
			isPlayer1Alive = true;
			break;
		}
	}

	for (int unitIndex = 0; (int)unitIndex < m_player2->m_units.size(); unitIndex++)
	{
		if (!m_player2->m_units[unitIndex]->m_isDead)
		{
			isPlayer2Alive = true;
			break;
		}
	}

	if (!isPlayer1Alive && !isPlayer2Alive)
	{
		m_endgameWidget->SetText("Draw!");
		m_endgameWidget->SetVisible(true);
		m_hasGameEnded = true;
	}
	else if (!isPlayer1Alive)
	{
		m_endgameWidget->SetText("Player 2 Wins!");
		m_endgameWidget->SetVisible(true);
		m_hasGameEnded = true;
	}
	else if (!isPlayer2Alive)
	{
		m_endgameWidget->SetText("Player 1 Wins!");
		m_endgameWidget->SetVisible(true);
		m_hasGameEnded = true;
	}
}

Player* Game::GetCurrentPlayer() const
{
	if (m_player1 && m_player1->m_turnState != TurnState::WAITING_FOR_TURN)
	{
		return m_player1;
	}
	else if (m_player2 && m_player2->m_turnState != TurnState::WAITING_FOR_TURN)
	{
		return m_player2;
	}
	return nullptr;
}

Player* Game::GetWaitingPlayer() const
{
	if (m_player1->m_turnState == TurnState::WAITING_FOR_TURN)
	{
		return m_player1;
	}
	else if (m_player2->m_turnState == TurnState::WAITING_FOR_TURN)
	{
		return m_player2;
	}
	return nullptr;
}

Player* Game::GetLocalPlayer() const
{
	if (m_gameType == GameType::NETWORK)
	{
		if (m_player1 && m_player1->m_netState == NetState::LOCAL)
		{
			return m_player1;
		}
		else if (m_player2 && m_player2->m_netState == NetState::LOCAL)
		{
			return m_player2;
		}
	}

	return nullptr;
}

void Game::SpawnParticle(Vec3 const& startPos, Vec3 const& velocity, float rotation, float rotationSpeed, float size, float lifetime, std::string const& textureName, Rgba8 const& color, float startAlpha, float endAlpha, float startAlphaTime, float endAlphaTime, float startScale, float endScale, float startScaleTime, float endScaleTime, float startSpeedMultiplier, float endSpeedMultiplier, float startSpeedTime, float endSpeedTime)
{
	Particle particle(startPos, velocity, rotation, rotationSpeed, size, &m_gameClock, lifetime, textureName, color, startAlpha, endAlpha, startAlphaTime, endAlphaTime, startScale, endScale, startScaleTime, endScaleTime, startSpeedMultiplier, endSpeedMultiplier, startSpeedTime, endSpeedTime);
	m_particles.emplace_back(particle);
}

Game::Game()
{
	LoadAssets();
	
	m_worldCamera.SetPerspectiveView(g_window->GetAspect(), 60.f, 0.01f, 100.f);
	m_worldCamera.SetRenderBasis(Vec3::SKYWARD, Vec3::WEST, Vec3::NORTH);
	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	SubscribeEventCallbackFunction("RemoteCommand", Event_RemoteCommand, "Send a remote command over the network");
	SubscribeEventCallbackFunction("BurstTest", Event_BurstTest, "Send a burst of test messages over the network");
	SubscribeEventCallbackFunction("RemoteHelp", Event_RemoteHelp, "Send help text over the network");
	SubscribeEventCallbackFunction("LoadMap", Event_LoadMap, "Load a map with the specified name");
	SubscribeEventCallbackFunction("PlayerReady", Event_PlayerReady, "Indicate that the player is ready");
	SubscribeEventCallbackFunction("SetFocusedHex", Event_SetFocusedHexCoords, "Set coordinates for the focused hex");
	SubscribeEventCallbackFunction("SelectFocusedUnit", Event_SelectFocusedUnit, "Set coordinates for the focused hex");
	SubscribeEventCallbackFunction("SelectPreviousUnit", Event_SelectPreviousUnit, "Set coordinates for the focused hex");
	SubscribeEventCallbackFunction("SelectNextUnit", Event_SelectNextUnit, "Set coordinates for the focused hex");
	SubscribeEventCallbackFunction("Move", Event_Move, "Set coordinates for the focused hex");
	SubscribeEventCallbackFunction("Stay", Event_Stay, "Set coordinates for the focused hex");
	SubscribeEventCallbackFunction("HoldFire", Event_HoldFire, "Set coordinates for the focused hex");
	SubscribeEventCallbackFunction("Attack", Event_Attack, "Set coordinates for the focused hex");
	SubscribeEventCallbackFunction("Cancel", Event_Cancel, "Set coordinates for the focused hex");
	SubscribeEventCallbackFunction("EndTurn", Event_EndTurn, "Set coordinates for the focused hex");
	SubscribeEventCallbackFunction("PlayerQuit", Event_PlayerQuit, "Set coordinates for the focused hex");
	SubscribeEventCallbackFunction("NetworkDisconnected", Event_NetworkDisconnected, "Set coordinates for the focused hex");

	SubscribeEventCallbackFunction("StartLocalGame", ButtonEvent_StartLocalGame, "Set coordinates for the focused hex");
	SubscribeEventCallbackFunction("StartNetworkGame", ButtonEvent_StartNetworkGame, "Set coordinates for the focused hex");
	SubscribeEventCallbackFunction("ResumeGame", ButtonEvent_ResumeGame, "Set coordinates for the focused hex");
	SubscribeEventCallbackFunction("ReturnToMenu", ButtonEvent_ReturnToMenu, "Set coordinates for the focused hex");
}

Game::~Game()
{
}

void Game::LoadAssets()
{
	g_squirrelFont = g_renderer->CreateOrGetBitmapFont("Data/Images/SquirrelFixedFont");
	g_butlerFont = g_renderer->CreateOrGetBitmapFont("Data/Fonts/RobotoMonoSemiBold128");
	m_gameLogoTexture = g_renderer->CreateOrGetTextureFromFile("Data/Images/Vaporum_Logo.png");

	UnitDefinition::InitializeUnitDefinitions();
	TileDefinition::InitializeTileDefinitions();
	MapDefinition::InitializeMapDefinitions();
	Particle::InitializeParticleTextures();

	if (m_gameState == GameState::INTRO)
	{
		m_logoTexture = g_renderer->CreateOrGetTextureFromFile("Data/Images/Logo.png");
	}
}

void Game::SortParticles()
{
	std::sort(m_particles.begin(), m_particles.end(), [&](Particle const& particleA, Particle const& particleB)
	{
		Vec3 const& displacementCameraToParticleA = particleA.m_position - m_playerPosition;
		Vec3 const& displacementCameraToParticleB = particleB.m_position - m_playerPosition;
		Vec3 const& cameraFwd = FIXED_CAMERA_ANGLE.GetAsMatrix_iFwd_jLeft_kUp().GetIBasis3D();
		float distCameraToParticleA = DotProduct3D(displacementCameraToParticleA, cameraFwd);
		float distCameraToParticleB = DotProduct3D(displacementCameraToParticleB, cameraFwd);
		return distCameraToParticleA > distCameraToParticleB;
	});
}

void Game::Update()
{
	float deltaSeconds = m_gameClock.GetDeltaSeconds();

	switch (m_gameState)
	{
		case GameState::INTRO:					UpdateIntroScreen(deltaSeconds);				break;
		case GameState::ATTRACT:				UpdateAttractScreen(deltaSeconds);				break;
		case GameState::MENU:					UpdateMenu(deltaSeconds);						break;
		case GameState::LOBBY:					UpdateLobby(deltaSeconds);						break;
		case GameState::GAME:					UpdateGame(deltaSeconds);						break;
		case GameState::PAUSED:					UpdatePauseMenu(deltaSeconds);					break;
	}

	m_timeInState += deltaSeconds;

	std::string nextReceivedMessage = g_netSystem->GetNextReceivedMessage();
	while (!nextReceivedMessage.empty())
	{
		m_commandsQueue.push_back(nextReceivedMessage);
		nextReceivedMessage = g_netSystem->GetNextReceivedMessage();
	}

	if (!m_isAnimationPlaying)
	{
		while (!m_commandsQueue.empty())
		{
			g_console->Execute(m_commandsQueue[m_commandsQueue.size() - 1]);
			m_commandsQueue.erase(m_commandsQueue.begin() + m_commandsQueue.size() - 1);
		}
	}

	HandleGameStateChange();
}

void Game::Render() const
{
	switch (m_gameState)
	{
		case GameState::INTRO:				RenderIntroScreen();					break;
		case GameState::ATTRACT:			RenderAttractScreen();					break;
		case GameState::MENU:				RenderMenu();							break;
		case GameState::LOBBY:				RenderLobby();							break;
		case GameState::GAME:				RenderGame();							break;
		case GameState::PAUSED:				RenderPauseMenu();						break;
	}

	DebugRenderWorld(m_worldCamera);
	DebugRenderScreen(m_screenCamera);
}

void Game::UpdateIntroScreen(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	if (m_timeInState >= 4.5f)
	{
		m_nextGameState = GameState::ATTRACT;
		m_timeInState = 0.f;
	}
}

void Game::UpdateAttractScreen(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	if (g_input->WasKeyJustPressed(KEYCODE_ESC))
	{
		g_app->HandleQuitRequested();
	}

	if (g_input->WasKeyJustPressed(KEYCODE_SPACE) || g_input->WasKeyJustPressed(KEYCODE_LMB))
	{
		m_nextGameState = GameState::MENU;
	}

	std::string networkMessage = g_netSystem->GetNextReceivedMessage();
	if (!networkMessage.empty())
	{
		g_console->Execute(networkMessage);
	}
}

void Game::UpdateMenu(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	if (g_input->WasKeyJustPressed(KEYCODE_ESC))
	{
		FireEvent("Quit");
	}

	if (g_input->WasKeyJustPressed(KEYCODE_DOWNARROW) || g_input->WasKeyJustPressed('S'))
	{
		UIWidget* nextWidget = g_ui->GetNextWidget();
		g_ui->SetLastHoveredWidget(nextWidget);
	}

	if (g_input->WasKeyJustPressed(KEYCODE_UPARROW) || g_input->WasKeyJustPressed('W'))
	{
		UIWidget* previousWidget = g_ui->GetPreviousWidget();
		g_ui->SetLastHoveredWidget(previousWidget);
	}


	if (g_input->WasKeyJustPressed(KEYCODE_ENTER) || g_input->WasKeyJustPressed(KEYCODE_SPACE))
	{
		UIWidget* selectedWidget = g_ui->GetLastHoveredWidget();
		FireEvent(selectedWidget->m_clickEventName);
	}
}

void Game::UpdateLobby(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	if (!m_currentMap && m_timeInState > 0.f)
	{
		m_currentMap = new Map(this, MapDefinition::s_defs["Grid12x12"]);
	}

	if (m_gameType == GameType::NETWORK)
	{
		if (m_isLocalPlayerReady && m_isRemotePlayerReady)
		{
			m_nextGameState = GameState::GAME;
		}
	}
	else
	{
		if (m_isLocalPlayerReady)
		{
			m_nextGameState = GameState::GAME;
		}
	}
}

void Game::UpdateGame(float deltaSeconds)
{
	if (g_input->WasKeyJustPressed(KEYCODE_ESC))
	{
		m_nextGameState = GameState::PAUSED;
	}

	if (m_hasGameEnded && g_input->WasKeyJustPressed(KEYCODE_LMB))
	{
		m_nextGameState = GameState::MENU;
	}

	if (m_currentMap)
	{
		m_currentMap->Update();
	}

	if (m_player1)
	{
		m_player1->Update();
		m_player1->DeleteGarbageUnits();
	}
	if (m_player2)
	{
		m_player2->Update();
		m_player2->DeleteGarbageUnits();
	}

	Player* currentPlayer = GetCurrentPlayer();
	if (m_hasGameEnded)
	{
		m_turnWidget->SetVisible(false);
	}
	else if (currentPlayer)
	{
		m_turnWidget->SetText(Stringf("Player %d's Turn", currentPlayer->m_playerIndex + 1))->SetColor(currentPlayer->GetTeamColor())->SetHoverColor(currentPlayer->GetTeamColor());
	}

	m_worldCamera.SetTransform(m_playerPosition, FIXED_CAMERA_ANGLE);

	for (int particleIndex = 0; (int)particleIndex < m_particles.size(); particleIndex++)
	{
		m_particles[particleIndex].Update(deltaSeconds);
	}

	// Delete destroyed particles
	for (int particleIndex = 0; (int)particleIndex < m_particles.size(); particleIndex++)
	{
		if (m_particles[particleIndex].m_isDestroyed)
		{
			m_particles.erase(m_particles.begin() + particleIndex);
		}
	}

	SortParticles();
}

void Game::UpdatePauseMenu(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	if (g_input->WasKeyJustPressed(KEYCODE_ESC))
	{
		m_nextGameState = GameState::GAME;
	}

	if (g_input->WasKeyJustPressed(KEYCODE_DOWNARROW) || g_input->WasKeyJustPressed('S'))
	{
		UIWidget* nextWidget = g_ui->GetNextWidget();
		g_ui->SetLastHoveredWidget(nextWidget);
	}

	if (g_input->WasKeyJustPressed(KEYCODE_UPARROW) || g_input->WasKeyJustPressed('W'))
	{
		UIWidget* previousWidget = g_ui->GetPreviousWidget();
		g_ui->SetLastHoveredWidget(previousWidget);
	}

	if (g_input->WasKeyJustPressed(KEYCODE_ENTER) || g_input->WasKeyJustPressed(KEYCODE_SPACE))
	{
		UIWidget* selectedWidget = g_ui->GetLastHoveredWidget();
		FireEvent(selectedWidget->m_clickEventName);
	}
}

void Game::RenderIntroScreen() const
{
	SpriteSheet logoSpriteSheet(m_logoTexture, IntVec2(15, 19));
	SpriteAnimDefinition logoAnimation(&logoSpriteSheet, 0, 271, 2.f, SpriteAnimPlaybackType::ONCE);
	SpriteAnimDefinition logoBlinkAnimation(&logoSpriteSheet, 270, 271, 0.2f, SpriteAnimPlaybackType::LOOP);

	g_renderer->BeginCamera(m_screenCamera);
	
	std::vector<Vertex_PCU> introScreenVertexes;
	std::vector<Vertex_PCU> introScreenFadeOutVertexes;
	AABB2 animatedLogoBox(Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y) * 0.5f - Vec2(320.f, 200.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y) * 0.5f + Vec2(320.f, 200.f));
	if (m_timeInState >= 2.f)
	{
		SpriteDefinition currentSprite = logoBlinkAnimation.GetSpriteDefAtTime(m_timeInState);
		AddVertsForAABB2(introScreenVertexes, animatedLogoBox, Rgba8::WHITE, currentSprite.GetUVs().m_mins, currentSprite.GetUVs().m_maxs);
	
		if (m_timeInState >= 3.5f)
		{
			AddVertsForAABB2(introScreenFadeOutVertexes, AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)), Rgba8(0, 0, 0, static_cast<unsigned char>(RoundDownToInt(RangeMapClamped((4.5f - m_timeInState), 1.f, 0.f, 0.f, 255.f)))));
		}
	}
	else
	{
		SpriteDefinition currentSprite = logoAnimation.GetSpriteDefAtTime(m_timeInState);
		AddVertsForAABB2(introScreenVertexes, animatedLogoBox, Rgba8::WHITE, currentSprite.GetUVs().m_mins, currentSprite.GetUVs().m_maxs);
	}
	g_renderer->BindTexture(m_logoTexture);
	g_renderer->DrawVertexArray(introScreenVertexes);
	g_renderer->BindTexture(nullptr);
	g_renderer->DrawVertexArray(introScreenFadeOutVertexes);

	g_renderer->EndCamera(m_screenCamera);
}

void Game::RenderAttractScreen() const
{
	g_renderer->BeginCamera(m_screenCamera);
	{
		std::vector<Vertex_PCU> logoVerts;
		AddVertsForAABB2(logoVerts, AABB2(Vec2::ZERO, Vec2::ONE), Rgba8::WHITE);
		TransformVertexArrayXY3D(logoVerts, SCREEN_SIZE_Y * 0.8f, 0.f, Vec2(SCREEN_SIZE_Y * (g_window->GetAspect() - 0.8f), SCREEN_SIZE_Y * 0.2f) * 0.5f);

		g_renderer->SetBlendMode(BlendMode::ALPHA);
		g_renderer->SetDepthMode(DepthMode::DISABLED);
		g_renderer->SetModelConstants();
		g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_BACK);
		g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
		g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_renderer->BindShader(nullptr);
		g_renderer->BindTexture(m_gameLogoTexture);
		g_renderer->DrawVertexArray(logoVerts);
	}
	g_renderer->EndCamera(m_screenCamera);
}

void Game::RenderMenu() const
{
	g_renderer->BeginCamera(m_screenCamera);
	{
		std::vector<Vertex_PCU> logoVerts;
		std::vector<Vertex_PCU> textVerts;
		std::vector<Vertex_PCU> menuVerts;

		AddVertsForAABB2(logoVerts, AABB2(Vec2::ZERO, Vec2::ONE), Rgba8::WHITE);
		TransformVertexArrayXY3D(logoVerts, SCREEN_SIZE_Y * 0.8f, 0.f, Vec2(SCREEN_SIZE_Y * (g_window->GetAspect() - 0.8f), SCREEN_SIZE_Y * 0.2f) * 0.5f);

		float textWidth = g_butlerFont->GetTextWidth(80.f, "Main Menu", 0.5f);
		g_butlerFont->AddVertsForTextInBox2D(textVerts, AABB2(Vec2(-textWidth * 0.5f, -40.f), Vec2(textWidth * 0.5f, 40.f)), 80.f, "Main Menu", Rgba8::WHITE, 0.5f, Vec2(0.5f, 0.5f));
		TransformVertexArrayXY3D(textVerts, 1.f, 90.f, Vec2(SCREEN_SIZE_X * 0.1f, textWidth));

		AddVertsForAABB2(menuVerts, AABB2(Vec2(SCREEN_SIZE_X * 0.12f, 0.f), Vec2(SCREEN_SIZE_X * 0.125f, SCREEN_SIZE_Y)), Rgba8::WHITE);

		g_renderer->SetBlendMode(BlendMode::ALPHA);
		g_renderer->SetDepthMode(DepthMode::DISABLED);
		g_renderer->SetModelConstants();
		g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_BACK);
		g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
		g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_renderer->BindShader(nullptr);
		g_renderer->BindTexture(m_gameLogoTexture);
		g_renderer->DrawVertexArray(logoVerts);

		g_renderer->BindTexture(g_butlerFont->GetTexture());
		g_renderer->DrawVertexArray(textVerts);

		g_renderer->BindTexture(nullptr);
		g_renderer->DrawVertexArray(menuVerts);
	}
	g_renderer->EndCamera(m_screenCamera);
}

void Game::RenderLobby() const
{
}

void Game::RenderGame() const
{
	if (m_currentMap)
	{
		m_currentMap->Render();
		if (m_currentMap->m_debugDraw)
		{
			if (m_player1)
			{
				m_player1->DebugRender();
			}
			if (m_player2)
			{
				m_player2->DebugRender();
			}
		}
	}
	if (m_player1)
	{
		m_player1->Render();
	}
	if (m_player2)
	{
		m_player2->Render();
	}

	g_renderer->BeginCamera(m_worldCamera);
	{
		for (int particleIndex = 0; (int)particleIndex < m_particles.size(); particleIndex++)
		{
			m_particles[particleIndex].Render(m_worldCamera);
		}
	}
	g_renderer->EndCamera(m_worldCamera);

	g_renderer->RenderEmissive();
}

void Game::RenderPauseMenu() const
{
	g_renderer->BeginCamera(m_screenCamera);
	{
		std::vector<Vertex_PCU> logoVerts;
		std::vector<Vertex_PCU> textVerts;
		std::vector<Vertex_PCU> menuVerts;

		AddVertsForAABB2(logoVerts, AABB2(Vec2::ZERO, Vec2::ONE), Rgba8::WHITE);
		TransformVertexArrayXY3D(logoVerts, SCREEN_SIZE_Y * 0.8f, 0.f, Vec2(SCREEN_SIZE_Y * (g_window->GetAspect() - 0.8f), SCREEN_SIZE_Y * 0.2f) * 0.5f);

		float textWidth = g_butlerFont->GetTextWidth(80.f, "Paused", 0.5f);
		g_butlerFont->AddVertsForTextInBox2D(textVerts, AABB2(Vec2(-textWidth * 0.5f, -40.f), Vec2(textWidth * 0.5f, 40.f)), 80.f, "Paused", Rgba8::WHITE, 0.5f, Vec2(0.5f, 0.5f));
		TransformVertexArrayXY3D(textVerts, 1.f, 90.f, Vec2(SCREEN_SIZE_X * 0.1f, textWidth));

		AddVertsForAABB2(menuVerts, AABB2(Vec2(SCREEN_SIZE_X * 0.12f, 0.f), Vec2(SCREEN_SIZE_X * 0.125f, SCREEN_SIZE_Y)), Rgba8::WHITE);

		g_renderer->SetBlendMode(BlendMode::ALPHA);
		g_renderer->SetDepthMode(DepthMode::DISABLED);
		g_renderer->SetModelConstants();
		g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_BACK);
		g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
		g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_renderer->BindShader(nullptr);
		g_renderer->BindTexture(m_gameLogoTexture);
		g_renderer->DrawVertexArray(logoVerts);

		g_renderer->BindTexture(g_butlerFont->GetTexture());
		g_renderer->DrawVertexArray(textVerts);

		g_renderer->BindTexture(nullptr);
		g_renderer->DrawVertexArray(menuVerts);
	}
	g_renderer->EndCamera(m_screenCamera);
}

void Game::EnterAttract()
{
	UIWidget* titleText = g_ui->CreateWidget();
	titleText->SetText("Vaporum")
			 ->SetPosition(Vec2(0.5f, 0.95f))
			 ->SetDimensions(Vec2(1.f, 1.f))
			 ->SetPivot(Vec2(0.5f, 0.5f))
			 ->SetAlignment(Vec2(0.5f, 0.5f))
			 ->SetColor(Rgba8::WHITE)
			 ->SetHoverColor(Rgba8::WHITE)
			 ->SetBackgroundColor(Rgba8::TRANSPARENT_BLACK)
			 ->SetHoverBackgroundColor(Rgba8::TRANSPARENT_BLACK)
			 ->SetFontSize(12.f);

	UIWidget* infoText = g_ui->CreateWidget();
	infoText->SetText("Press Space or Click Anywhere to Continue\nEscape to Quit")
			->SetPosition(Vec2(0.5f, 0.1f))
			->SetDimensions(Vec2(1.f, 0.1f))
			->SetPivot(Vec2(0.5f, 0.5f))
			->SetAlignment(Vec2(0.5f, 0.5f))
			->SetBackgroundColor(Rgba8::TRANSPARENT_BLACK)
			->SetHoverBackgroundColor(Rgba8::TRANSPARENT_BLACK)
			->SetColor(Rgba8::WHITE)
			->SetHoverColor(Rgba8::WHITE)
			->SetFontSize(32.f);
}

void Game::ExitAttract()
{
	g_ui->Clear();
}

void Game::EnterMenu()
{
	delete m_currentMap;
	m_currentMap = nullptr;

	m_isLocalPlayerReady = false;
	if (m_gameType == GameType::NETWORK)
	{
		m_isRemotePlayerReady = false;
	}
	m_hasGameEnded = false;
	m_gameType = GameType::NONE;

	UIWidget* localGameButton = g_ui->CreateWidget();
	localGameButton->SetText("Local Game")
				   ->SetPosition(Vec2(0.13f, 0.5f))
				   ->SetDimensions(Vec2(0.3f, 0.05f))
				   ->SetAlignment(Vec2(0.f, 0.5f))
				   ->SetClickEventName("StartLocalGame")
				   ->SetBackgroundColor(Rgba8::TRANSPARENT_BLACK)
				   ->SetHoverBackgroundColor(Rgba8::WHITE)
				   ->SetColor(Rgba8::WHITE)
				   ->SetHoverColor(Rgba8::BLACK)
				   ->SetBorderWidth(0.001f)
				   ->SetBorderColor(Rgba8::TRANSPARENT_BLACK)
				   ->SetHoverBorderColor(Rgba8::GRAY)
				   ->SetFontSize(32.f);

	AABB2 exitButtonBounds(Vec2(0.13f, 0.425f), Vec2(0.43f, 0.475f));

	if (g_netSystem->GetNetworkMode() != NetworkMode::NONE)
	{
		UIWidget* networkGameButton = g_ui->CreateWidget();
		networkGameButton->SetText("Network Game")
			->SetPosition(Vec2(0.13f, 0.425f))
			->SetDimensions(Vec2(0.3f, 0.05f))
			->SetAlignment(Vec2(0.f, 0.5f))
			->SetClickEventName("StartNetworkGame")
			->SetBackgroundColor(Rgba8::TRANSPARENT_BLACK)
			->SetHoverBackgroundColor(Rgba8::WHITE)
			->SetColor(Rgba8::WHITE)
			->SetHoverColor(Rgba8::BLACK)
			->SetBorderWidth(0.001f)
			->SetBorderColor(Rgba8::TRANSPARENT_BLACK)
			->SetHoverBorderColor(Rgba8::GRAY)
			->SetFontSize(32.f);

		exitButtonBounds.Translate(Vec2(0.f, -0.075f));
	}

	UIWidget* exitButton = g_ui->CreateWidget();
	exitButton->SetText("Exit")
		->SetPosition(exitButtonBounds.m_mins)
		->SetDimensions(exitButtonBounds.GetDimensions())
		->SetAlignment(Vec2(0.f, 0.5f))
		->SetClickEventName("Quit")
		->SetBackgroundColor(Rgba8::TRANSPARENT_BLACK)
		->SetHoverBackgroundColor(Rgba8::WHITE)
		->SetColor(Rgba8::WHITE)
		->SetHoverColor(Rgba8::BLACK)
		->SetBorderWidth(0.001f)
		->SetBorderColor(Rgba8::TRANSPARENT_BLACK)
		->SetHoverBorderColor(Rgba8::GRAY)
		->SetFontSize(32.f);
}

void Game::ExitMenu()
{
	g_ui->Clear();
}

void Game::EnterLobby()
{
	std::string infoTextStr = "Waiting for Players...";
	if (m_gameType == GameType::LOCAL)
	{
		infoTextStr = "Loading Map...";
	}

	UIWidget* infoText = g_ui->CreateWidget();
	infoText->SetText(infoTextStr)
		->SetPosition(Vec2(0.5f, 0.5f))
		->SetDimensions(Vec2(0.3f, 0.3f))
		->SetPivot(Vec2(0.5f, 0.5f))
		->SetAlignment(Vec2(0.5f, 0.5f))
		->SetBackgroundColor(Rgba8::TRANSPARENT_BLACK)
		->SetHoverBackgroundColor(Rgba8::TRANSPARENT_BLACK)
		->SetColor(Rgba8::WHITE)
		->SetHoverColor(Rgba8::WHITE)
		->SetFontSize(4.f)
		->SetBorderWidth(0.2f)
		->SetBorderColor(Rgba8::WHITE)
		->SetHoverBorderColor(Rgba8::WHITE);
}

void Game::ExitLobby()
{
	g_ui->Clear();
}

void Game::EnterGame()
{
	m_turnWidget = g_ui->CreateWidget();
	m_turnWidget->SetPosition(Vec2(0.05f, 0.95f))
		->SetDimensions(Vec2(0.3f, 0.09f))
		->SetPivot(Vec2(0.1f, 0.5f))
		->SetAlignment(Vec2(0.5f, 0.5f))
		->SetBorderWidth(0.1f)
		->SetBorderColor(Rgba8::WHITE)
		->SetHoverBorderColor(Rgba8::WHITE)
		->SetColor(Rgba8::WHITE)
		->SetBackgroundColor(Rgba8::BLACK)
		->SetHoverBackgroundColor(Rgba8::BLACK)
		->SetHoverColor(Rgba8::WHITE)
		->SetFontSize(6.f);

	m_p1UnitInfoContainerWidget = g_ui->CreateWidget();
	m_p1UnitInfoContainerWidget->SetPosition(Vec2(0.01f, 0.1f))
		->SetDimensions(Vec2(0.2f, 0.5f))
		->SetVisible(false)
		->SetBorderWidth(0.2f)
		->SetBorderColor(Rgba8::WHITE)
		->SetHoverBorderColor(Rgba8::WHITE)
		->SetBackgroundColor(Rgba8::BLACK)
		->SetHoverBackgroundColor(Rgba8::BLACK);

	m_p1UnitImageWidget = g_ui->CreateWidget(m_p1UnitInfoContainerWidget);
	m_p1UnitImageWidget->SetPosition(Vec2(0.f, 0.6f))
		->SetDimensions(Vec2(1.f, 0.4f))
		->SetPivot(Vec2::ZERO)
		->SetAlignment(Vec2(0.5f, 0.5f))
		->SetVisible(false)
		->SetColor(Rgba8::WHITE)
		->SetHoverColor(Rgba8::WHITE);

	m_p1UnitInfoWidget = g_ui->CreateWidget(m_p1UnitInfoContainerWidget);
	m_p1UnitInfoWidget->SetPosition(Vec2(0.5f, 0.1f))
		->SetDimensions(Vec2(1.f, 0.5f))
		->SetPivot(Vec2(0.5f, 0.0f))
		->SetAlignment(Vec2(0.5f, 0.0f))
		->SetVisible(false)
		->SetFontSize(4.f)
		->SetColor(Rgba8::WHITE)
		->SetHoverColor(Rgba8::WHITE);


	m_p2UnitInfoContainerWidget = g_ui->CreateWidget();
	m_p2UnitInfoContainerWidget->SetPosition(Vec2(0.79f, 0.1f))
		->SetDimensions(Vec2(0.2f, 0.5f))
		->SetVisible(false)
		->SetBorderWidth(0.2f)
		->SetBorderColor(Rgba8::WHITE)
		->SetHoverBorderColor(Rgba8::WHITE)
		->SetBackgroundColor(Rgba8::BLACK)
		->SetHoverBackgroundColor(Rgba8::BLACK);

	m_p2UnitImageWidget = g_ui->CreateWidget(m_p2UnitInfoContainerWidget);
	m_p2UnitImageWidget->SetPosition(Vec2(0.f, 0.6f))
		->SetDimensions(Vec2(1.f, 0.4f))
		->SetPivot(Vec2(0.f, 0.f))
		->SetAlignment(Vec2(0.5f, 0.5f))
		->SetVisible(false)
		->SetColor(Rgba8::WHITE)
		->SetHoverColor(Rgba8::WHITE);

	m_p2UnitInfoWidget = g_ui->CreateWidget(m_p2UnitInfoContainerWidget);
	m_p2UnitInfoWidget->SetPosition(Vec2(0.5f, 0.1f))
		->SetDimensions(Vec2(1.f, 0.5f))
		->SetPivot(Vec2(0.5f, 0.0f))
		->SetAlignment(Vec2(0.5f, 0.5f))
		->SetVisible(false)
		->SetFontSize(4.f)
		->SetColor(Rgba8::WHITE)
		->SetHoverColor(Rgba8::WHITE);

	m_endgameWidget = g_ui->CreateWidget();
	m_endgameWidget->SetText("")
		->SetPosition(Vec2(0.5f, 0.5f))
		->SetDimensions(Vec2(0.3f, 0.3f))
		->SetVisible(false)
		->SetPivot(Vec2(0.5f, 0.5f))
		->SetAlignment(Vec2(0.5f, 0.5f))
		->SetBackgroundColor(Rgba8::BLACK)
		->SetHoverBackgroundColor(Rgba8::BLACK)
		->SetColor(Rgba8::WHITE)
		->SetHoverColor(Rgba8::WHITE)
		->SetFontSize(4.f)
		->SetBorderWidth(0.2f)
		->SetBorderColor(Rgba8::WHITE)
		->SetHoverBorderColor(Rgba8::WHITE);
}

void Game::ExitGame()
{
	g_ui->Clear();
}

void Game::EnterPauseMenu()
{
	UIWidget* resumeButton = g_ui->CreateWidget();
	resumeButton->SetText("Resume")
		->SetPosition(Vec2(0.13f, 0.5f))
		->SetDimensions(Vec2(0.3f, 0.05f))
		->SetAlignment(Vec2(0.f, 0.5f))
		->SetClickEventName("ResumeGame")
		->SetBackgroundColor(Rgba8::TRANSPARENT_BLACK)
		->SetHoverBackgroundColor(Rgba8::WHITE)
		->SetColor(Rgba8::WHITE)
		->SetHoverColor(Rgba8::BLACK)
		->SetBorderWidth(0.001f)
		->SetBorderColor(Rgba8::TRANSPARENT_BLACK)
		->SetHoverBorderColor(Rgba8::GRAY)
		->SetFontSize(32.f);

	UIWidget* menuButton = g_ui->CreateWidget();
	menuButton->SetText("Main Menu")
		->SetPosition(Vec2(0.13f, 0.425f))
		->SetDimensions(Vec2(0.3f, 0.05f))
		->SetAlignment(Vec2(0.f, 0.5f))
		->SetClickEventName("ReturnToMenu")
		->SetBackgroundColor(Rgba8::TRANSPARENT_BLACK)
		->SetHoverBackgroundColor(Rgba8::WHITE)
		->SetColor(Rgba8::WHITE)
		->SetHoverColor(Rgba8::BLACK)
		->SetBorderWidth(0.001f)
		->SetBorderColor(Rgba8::TRANSPARENT_BLACK)
		->SetHoverBorderColor(Rgba8::GRAY)
		->SetFontSize(32.f);
}

void Game::ExitPauseMenu()
{
	g_ui->Clear();
}

void Game::HandleGameStateChange()
{
	if (m_nextGameState == GameState::NONE)
	{
		return;
	}

	switch (m_gameState)
	{
		case GameState::ATTRACT:		ExitAttract();			break;
		case GameState::MENU:			ExitMenu();				break;
		case GameState::LOBBY:			ExitLobby();			break;
		case GameState::GAME:			ExitGame();				break;
		case GameState::PAUSED:			ExitPauseMenu();		break;
	}

	m_gameState = m_nextGameState;
	m_nextGameState = GameState::NONE;
	m_timeInState = 0.f;

	switch (m_gameState)
	{
		case GameState::ATTRACT:		EnterAttract();			break;
		case GameState::MENU:			EnterMenu();			break;
		case GameState::LOBBY:			EnterLobby();			break;
		case GameState::GAME:			EnterGame();			break;
		case GameState::PAUSED:			EnterPauseMenu();		break;
	}
}
