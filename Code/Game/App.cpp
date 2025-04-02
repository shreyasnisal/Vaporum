#include "Game/App.hpp"

#include "Game/GameCommon.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Models/ModelLoader.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Networking/NetSystem.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/UI/UISystem.hpp"


App* g_app = nullptr;
AudioSystem* g_audio = nullptr;
RandomNumberGenerator* g_RNG = nullptr;
Renderer* g_renderer = nullptr;
Window* g_window = nullptr;
BitmapFont* g_squirrelFont = nullptr;
BitmapFont* g_butlerFont = nullptr;
NetSystem* g_netSystem = nullptr;
ModelLoader* g_modelLoader = nullptr;


float SCREEN_SIZE_X = 1600.f;
float WINDOW_ASPECT = 0.5f;
std::string APP_NAME = "Vaporum";

bool App::HandleQuitRequested(EventArgs& args)
{
	UNUSED(args);
	g_app->HandleQuitRequested();
	return true;
}

bool App::ShowControls(EventArgs& args)
{
	UNUSED(args);

	// Add controls to DevConsole
	g_console->AddLine(Rgba8::STEEL_BLUE, "Controls", false);
	g_console->AddLine(Rgba8::MAGENTA, Stringf("%-20s : Toggle console", "`"), false);
	g_console->AddLine(Rgba8::MAGENTA, Stringf("%-20s : Exit the application", "Escape"), false);
	//---------------------------------------------------------------------------------------
	// Debug Keys
	g_console->AddLine(Rgba8::STEEL_BLUE, "Debug Keys", false);
	g_console->AddLine(Rgba8::MAGENTA, Stringf("%-20s : Play particle effect on Unit selected for attack", "P"), false);
	g_console->AddLine(Rgba8::MAGENTA, Stringf("%-20s : Toggle debug heat map and movement spline", "F1"), false);

	return true;
}

bool App::Event_LoadGameConfig(EventArgs& args)
{
	std::string filename = args.GetValue("File", "");
	if (!filename.empty())
	{
		XmlDocument configXmlDoc;
		XmlResult result = configXmlDoc.LoadFile(filename.c_str());
		if (result != XmlResult::XML_SUCCESS)
		{
			ERROR_AND_DIE(Stringf("Could not open or read file \"%s\"", filename.c_str()));
		}

		XmlElement const* configRootElement = configXmlDoc.RootElement();
		g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*configRootElement);
	}

	return true;
}

App::App(char const* startupCommand)
	: m_startupCommand(startupCommand)
{

}

App::~App()
{
	delete g_netSystem;
	g_netSystem = nullptr;
	
	delete g_renderer;
	g_renderer = nullptr;

	delete g_input;
	g_input = nullptr;

	delete g_audio;
	g_audio = nullptr;

	delete g_RNG;
	g_RNG = nullptr;

	delete g_modelLoader;
	g_modelLoader = nullptr;

	delete g_netSystem;
	g_netSystem = nullptr;

	delete g_ui;
	g_ui = nullptr;

	delete m_game;
	m_game = nullptr;
}

void App::Startup()
{
	LoadConfig();

	EventSystemConfig eventSystemConfig;
	g_eventSystem = new EventSystem(eventSystemConfig);

	SubscribeEventCallbackFunction("LoadGameConfig", Event_LoadGameConfig);
	FireEvent(m_startupCommand);

	IntVec2 windowSize = g_gameConfigBlackboard.GetValue("windowSize", IntVec2(-1, -1));
	WINDOW_ASPECT = g_gameConfigBlackboard.GetValue("windowAspect", 2.f);
	if (windowSize != IntVec2(-1, -1))
	{
		WINDOW_ASPECT = (float)windowSize.x / (float)windowSize.y;
	}
	SCREEN_SIZE_X = SCREEN_SIZE_Y * WINDOW_ASPECT;
	APP_NAME = g_gameConfigBlackboard.GetValue("windowTitle", APP_NAME);

	InputConfig inputConfig;
	g_input = new InputSystem(inputConfig);

	WindowConfig windowConfig;
	windowConfig.m_inputSystem = g_input;
	windowConfig.m_windowTitle = APP_NAME;
	windowConfig.m_clientAspect = WINDOW_ASPECT;
	windowConfig.m_isFullScreen = g_gameConfigBlackboard.GetValue("windowFullscreen", false);
	windowConfig.m_windowPosition = g_gameConfigBlackboard.GetValue("windowPosition", IntVec2(-1, -1));
	windowConfig.m_windowSize = g_gameConfigBlackboard.GetValue("windowSize", IntVec2(-1, -1));
	g_window = new Window(windowConfig);

	RenderConfig renderConfig;
	renderConfig.m_window = g_window;
	g_renderer = new Renderer(renderConfig);

	DevConsoleConfig devConsoleConfig;
	devConsoleConfig.m_renderer = g_renderer;
	devConsoleConfig.m_consoleFontFilePathWithNoExtension = "Data/Images/SquirrelFixedFont";
	m_devConsoleCamera.SetOrthoView(Vec2::ZERO, Vec2(WINDOW_ASPECT, 1.f));
	devConsoleConfig.m_camera = m_devConsoleCamera;
	g_console = new DevConsole(devConsoleConfig);

	AudioConfig audioConfig;
	g_audio = new AudioSystem(audioConfig);

	DebugRenderConfig debugRenderConfig;
	debugRenderConfig.m_renderer = g_renderer;
	debugRenderConfig.m_bitmapFontFilePathWithNoExtension = "Data/Images/SquirrelFixedFont";

	NetSystemConfig netSystemConfig;
	netSystemConfig.m_modeStr = g_gameConfigBlackboard.GetValue("netMode", "");
	netSystemConfig.m_hostAddressStr = g_gameConfigBlackboard.GetValue("netHostAddress", "127.0.0.1:23456");
	netSystemConfig.m_sendBufferSize = g_gameConfigBlackboard.GetValue("netSendBufferSize", 2048);
	netSystemConfig.m_recvBufferSize = g_gameConfigBlackboard.GetValue("netRecvBufferSize", 2048);
	g_netSystem = new NetSystem(netSystemConfig);

	ModelLoaderConfig modelLoaderConfig;
	modelLoaderConfig.m_renderer = g_renderer;
	g_modelLoader = new ModelLoader(modelLoaderConfig);

	UISystemConfig uiSystemConfig;
	uiSystemConfig.m_input = g_input;
	uiSystemConfig.m_renderer = g_renderer;
	uiSystemConfig.m_fontFileNameWithNoExtension = "Data/Fonts/RobotoMonoSemiBold128";
	m_uiSystemCamera.SetOrthoView(Vec2::ZERO, Vec2(WINDOW_ASPECT, 1.f));
	uiSystemConfig.m_camera = m_uiSystemCamera;
	g_ui = new UISystem(uiSystemConfig);

	g_eventSystem->Startup();
	g_console->Startup();
	g_input->Startup();
	g_window->Startup();
	g_renderer->Startup();
	g_audio->Startup();
	DebugRenderSystemStartup(debugRenderConfig);
	g_netSystem->Startup();
	g_modelLoader->Startup();
	g_ui->Startup();

	m_game = new Game();

	SubscribeEventCallbackFunction("Quit", HandleQuitRequested, "Exits the application");
	SubscribeEventCallbackFunction("Controls", ShowControls, "Shows game controls");

	EventArgs emptyArgs;
	ShowControls(emptyArgs);
}

void App::Run()
{
	while (!IsQuitting())
	{
		RunFrame();
	}
}

void App::RunFrame()
{
	BeginFrame();
	Update();
	Render();
	EndFrame();

}

bool App::HandleQuitRequested()
{
	m_isQuitting = true;

	return true;
}

void App::BeginFrame()
{
	Clock::TickSystemClock();

	m_frameRate = 1.f / Clock::GetSystemClock().GetDeltaSeconds();

	g_eventSystem->BeginFrame();
	g_console->BeginFrame();
	g_input->BeginFrame();
	g_window->BeginFrame();
	g_renderer->BeginFrame();
	g_audio->BeginFrame();
	DebugRenderBeginFrame();
	g_netSystem->BeginFrame();
	g_modelLoader->BeginFrame();
	g_ui->BeginFrame();
}

void App::Update()
{
	m_game->Update();

	if (g_input->WasKeyJustPressed(KEYCODE_F8))
	{
		delete m_game;
		m_game = new Game();
	}
	if (g_input->WasKeyJustPressed(KEYCODE_TILDE))
	{
		g_console->ToggleMode(DevConsoleMode::OPENFULL);
	}

	//DebugAddScreenText(Stringf("%-15s FPS: %.2f, Scale: %.2f", "System Clock:", m_frameRate, Clock::GetSystemClock().GetTimeScale()), Vec2(SCREEN_SIZE_X - 16.f, SCREEN_SIZE_Y - 16.f), 16.f, Vec2(1.f, 1.f), 0.f);
	float gameFPS = 0.f;
	if (!m_game->m_gameClock.IsPaused())
	{
		gameFPS = 1.f / m_game->m_gameClock.GetDeltaSeconds();
	}
	//DebugAddScreenText(Stringf("%-15s FPS: %.2f, Scale: %.2f", "Game Clock:", gameFPS, m_game->m_gameClock.GetTimeScale()), Vec2(SCREEN_SIZE_X - 16.f, SCREEN_SIZE_Y - 32.f), 16.f, Vec2(1.f, 1.f), 0.f);
}

void App::Render() const
{
	g_renderer->ClearScreen(Rgba8::BLACK);
	m_game->Render();

	g_ui->Render();
	g_console->Render(AABB2(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y)));
}

void App::EndFrame()
{
	g_ui->EndFrame();
	g_modelLoader->EndFrame();
	g_netSystem->EndFrame();
	DebugRenderEndFrame();
	g_audio->EndFrame();
	g_renderer->EndFrame();
	g_window->EndFrame();
	g_input->EndFrame();
	g_console->EndFrame();
	g_eventSystem->EndFrame();
}

void App::LoadConfig()
{
	XmlDocument gameConfigDoc;
	XmlResult result = gameConfigDoc.LoadFile("Data/GameConfig.xml");
	if (result != XmlResult::XML_SUCCESS)
	{
		ERROR_AND_DIE("Could not open or read file GameConfig.xml");
	}
	XmlElement const* rootElement = gameConfigDoc.RootElement();
	g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*rootElement);
}

void App::Shutdown()
{
	g_ui->Shutdown();
	g_modelLoader->Shutdown();
	g_netSystem->Shutdown();
	DebugRenderSystemShutdown();
	g_audio->Shutdown();
	g_renderer->Shutdown();
	g_input->Shutdown();
	g_window->Shutdown();
	g_console->Shutdown();
	g_eventSystem->EndFrame();
}

