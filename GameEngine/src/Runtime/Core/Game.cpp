#include "Game.h"

#include <entt/entt.hpp>

#include "Systems.h"
#include "Debug/Logger.h"

#include "SdlContainer.h"
#include "AssetStore/AssetStore.h"
#include "Entity.h"
#include "LevelLoader.h"

#include "Transform.h"
#include "Physics/Physics.h"
#include "Renderer/Renderer.h"
#include "Input/InputSystem.h"
#include "TimeSystem.h"
#include "Animation/AnimationSystem.h"
#include "Events/EntityEventSystem.h"
#include "Scripting/ScriptSystem.h"

#ifdef _EDITOR
#include "Editor/Editor.h"
#endif

Game::Game() {
	Logger::Log("Game Constructed");
	SetupBaseSystems();
	SetupLowLevelSystems();
#ifdef _EDITOR
	CREATESYSTEM(Editor);
#endif
}

Game::~Game() {
	Logger::Log("Game Destructed");
}

void Game::SetupBaseSystems() {
	CREATESYSTEM(SdlContainer, 1200, (float)1200 * 9 / 16);
	CREATESYSTEM(LevelLoader);
	CREATESYSTEM(Entities);
	CREATESYSTEM(AssetStore);
}

void Game::SetupLowLevelSystems()
{
	CREATESYSTEM(TimeSystem);
	CREATESYSTEM(InputSystem);
	CREATESYSTEM(RendererSystem);
	CREATESYSTEM(AnimationPlayer);
	TransformSystem& transformSystem = CREATESYSTEM(TransformSystem);
	transformSystem.InitDebugDrawer();
	PhysicsSystem& physics = CREATESYSTEM(PhysicsSystem, 0, -10);
	physics.InitDebugDrawer();
	CREATESYSTEM(EntityEventSystem);
	CREATESYSTEM(ScriptSystem);

#ifdef _DEBUG
	physics.EnableDebug(true);
	transformSystem.EnableDebug(true);
#endif // !_DEBUG

	isRunning = false;
}

void Game::Run()
{
	Setup();
	isRunning = true;
	while (isRunning) {
		Update();
		Render();
	}
}

void Game::Setup()
{
	LoadAssets();
	LoadLevel();
	GETSYSTEM(RendererSystem).InitLoaded();
#ifdef _EDITOR
	GETSYSTEM(Editor).Reset();
#endif
}

void Game::LoadAssets()
{
	AssetStore& assetStore = GETSYSTEM(AssetStore);
	assetStore.LoadPackage("assets/Packages/a.pkg");
}

void Game::LoadLevel()
{
	LevelLoader& levelLoader = GETSYSTEM(LevelLoader);
	levelLoader.LoadLevel("assets/Levels/Level.yaml");
	TransformSystem& transformSystem = GETSYSTEM(TransformSystem);
	transformSystem.InitLoaded();
}

void Game::Update()
{
#ifdef _EDITOR
	bool exitGame = GETSYSTEM(Editor).ProcessEvents();
#else
	bool exitGame = GETSYSTEM(SdlContainer).ProcessEvents();
#endif
	if (exitGame) {
		isRunning = false;
	}
#ifdef _EDITOR
	GETSYSTEM(TransformSystem).Update();
	GETSYSTEM(InputSystem).Update();
	GETSYSTEM(AnimationPlayer).Update();
	GETSYSTEM(EntityEventSystem).Update();
	GETSYSTEM(Editor).Update();
#else
	GETSYSTEM(TimeSystem).Update();
	GETSYSTEM(TransformSystem).Update();
	GETSYSTEM(InputSystem).Update();
	GETSYSTEM(PhysicsSystem).Update();
	GETSYSTEM(AnimationPlayer).Update();
	GETSYSTEM(EntityEventSystem).Update();
	GETSYSTEM(ScriptSystem).Update();
#endif
}

void Game::Render()
{

	RendererSystem& renderer = GETSYSTEM(RendererSystem);
	renderer.Render();
#ifdef _DEBUG
	GETSYSTEM(PhysicsSystem).DebugRender(renderer.GetWorldToScreenMatrix());

	TransformSystem& transformSystem = GETSYSTEM(TransformSystem);
	transformSystem.GetDebugRenderer().SetMatrix(renderer.GetWorldToScreenMatrix());
	transformSystem.DebugRender(renderer.GetWorldToScreenMatrix());
#endif // _DEBUG

#ifdef _EDITOR
	GETSYSTEM(Editor).RenderGizmos();
	GETSYSTEM(Editor).RenderEditor();
#endif
	renderer.Present();
}