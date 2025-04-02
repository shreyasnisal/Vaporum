#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Camera.hpp"

#include "Game/Game.hpp"


class App
{
public:
						App							(char const* startupCommand);
						~App						();
	void				Startup						();
	void				Shutdown					();
	void				Run							();
	void				RunFrame					();

	bool				IsQuitting					() const		{ return m_isQuitting; }
	bool				HandleQuitRequested			();
	static bool			HandleQuitRequested			(EventArgs& args);
	static bool			ShowControls				(EventArgs& args);
	static bool			Event_LoadGameConfig		(EventArgs& args);

public:
	Game*				m_game;
	float				m_frameRate;

private:
	void				BeginFrame					();
	void				Update						();
	void				Render						() const;
	void				EndFrame					();
	void				LoadConfig					();

private:
	bool				m_isQuitting				= false;
	Camera				m_devConsoleCamera			= Camera();
	Camera				m_uiSystemCamera			= Camera();
	std::string			m_startupCommand;
};
