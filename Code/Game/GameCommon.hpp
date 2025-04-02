#pragma once

#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/SimpleTriangleFont.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"

#include <string>

class App;
class NetSystem;
class ModelLoader;
class UISystem;

extern App*							g_app;
extern RandomNumberGenerator*		g_RNG;
extern Renderer*					g_renderer;
extern AudioSystem*					g_audio;
extern Window*						g_window;
extern BitmapFont*					g_squirrelFont;
extern BitmapFont*					g_butlerFont;
extern NetSystem*					g_netSystem;
extern ModelLoader*					g_modelLoader;
extern UISystem*					g_ui;

constexpr float SCREEN_SIZE_Y		= 800.f;
extern float SCREEN_SIZE_X;
extern std::string APP_NAME;
