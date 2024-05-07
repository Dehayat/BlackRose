#include "Game.h"
#include <sstream>
#include <SDL2/SDL.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <ryml/ryml.hpp>
#include "SdlContainer.h"
#include "AssetStore.h"
#include "Transform.h"
#include "Physics.h"
#include "Renderer.h"
#include "InputSystem.h"
#include "LevelLoader.h"
#include "TimeSystem.h"
#include "Components/GUIDComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/CameraComponent.h"
#include "Logger.h"

#ifdef _EDITOR
#include <imgui.h>
#include "Editor/TransformEditor.h"
#include "Editor/PhysicsEditor.h"
#include "Editor/RenderEditor.h"
#endif // _EDITOR


Game::Game() {
	Logger::Log("Game Constructed");
	SetupBaseSystems();
	SetupLowLevelSystems();
}

Game::~Game() {
	Logger::Log("Game Destructed");
}

void Game::SetupBaseSystems() {
	entt::locator<SdlContainer>::emplace<SdlContainer>(1200, (float)1200 * 9 / 16);
	entt::locator<AssetStore>::emplace<AssetStore>();
	entt::locator<Entities>::emplace<Entities>();
	entt::locator<LevelLoader>::emplace<LevelLoader>();
}

void Game::SetupLowLevelSystems()
{
	TransformSystem& transformSystem = entt::locator<TransformSystem>::emplace<TransformSystem>();
	transformSystem.InitDebugDrawer();
	PhysicsSystem& physics = entt::locator<PhysicsSystem>::emplace<PhysicsSystem>(0, -10);
	physics.InitDebugDrawer();
	entt::locator<RendererSystem>::emplace<RendererSystem>();
	entt::locator<InputSystem>::emplace<InputSystem>();
	entt::locator<TimeSystem>::emplace<TimeSystem>();

#ifdef _DEBUG
	physics.EnableDebug(true);
	transformSystem.EnableDebug(true);
#endif // !_DEBUG

	isRunning = false;
}

void Game::LoadAssets()
{
	AssetStore& assetStore = entt::locator<AssetStore>::value();
	assetStore.AddTexture("rose", "./assets/Rose.png", 512);
	assetStore.AddTexture("hornet", "./assets/Hornet_Idle.png", 128);
	assetStore.AddTexture("block", "./assets/Block.jpg", 64);
	assetStore.AddTexture("big_ground", "./assets/BigGround.png", 128);
}

void Game::LoadLevel()
{
	LevelLoader& levelLoader = entt::locator<LevelLoader>::value();
	levelLoader.LoadLevel("SavedLevel.yaml");
	RegisterAllEntities();
	TransformSystem& transformSystem = entt::locator<TransformSystem>::value();
	transformSystem.InitLoaded();
}

void Game::Setup()
{
	LoadAssets();
	LoadLevel();
	RendererSystem& renderer = entt::locator<RendererSystem>::value();
	renderer.InitLoaded();

#ifdef _EDITOR
	levelTree.Init(registry);
#endif // _EDITOR

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
void Game::Update()
{
	TimeSystem& timeSystem = entt::locator<TimeSystem>::value();
	timeSystem.Update();
	TransformSystem& transformSystem = entt::locator<TransformSystem>::value();
	transformSystem.Update();

#ifdef _EDITOR
	if (imgui.ProcessEvents()) {
		isRunning = false;
	}
	input.Update(sdl->GetWindow());
#else
	if (entt::locator<SdlContainer>::value().ProcessEvents()) {
		isRunning = false;
	}
	InputSystem& input = entt::locator<InputSystem>::value();
	input.Update(entt::locator<SdlContainer>::value().GetWindow());

	PhysicsSystem& physics = entt::locator<PhysicsSystem>::value();
	physics.Update();

	Entities& entities = entt::locator<Entities>::value();
	entt::registry& registry = entities.GetRegistry();
	auto view2 = registry.view<PlayerComponent, const TransformComponent, PhysicsBodyComponent>();
	for (auto entity : view2) {
		const auto& pos = view2.get<TransformComponent>(entity);
		auto& phys = view2.get<PhysicsBodyComponent>(entity);
		auto& player = view2.get<PlayerComponent>(entity);
		if (input.GetKey(InputKey::LEFT).isPressed || input.GetKey(InputKey::A).isPressed) {
			player.input = -1;
		}
		else if (input.GetKey(InputKey::RIGHT).isPressed || input.GetKey(InputKey::D).isPressed) {
			player.input = 1;
		}
		else {
			player.input = 0;
		}
		auto vel = b2Vec2(player.speed * player.input, phys.body->GetLinearVelocity().y);
		phys.body->SetLinearVelocity(vel);
	}
#endif // _EDITOR
}

#ifdef _EDITOR


entt::entity selected = entt::entity(-1);
entt::entity created = entt::entity(-1);
bool entityList[100];
vec2 lastPos;
void Editor(entt::registry& registry, InputSystem& input, Renderer& renderer, LevelEditor::LevelTree& tree, LevelLoader& levelLoader, Physics& physics) {

	if (input.GetMouseButton(MIDDLE_BUTTON).justPressed) {
		lastPos = input.GetMousePosition();
	}
	else if (input.GetMouseButton(MIDDLE_BUTTON).isPressed) {
		auto curPos = input.GetMousePosition();
		if (glm::distance(curPos, lastPos) > 3) {
			glm::vec2 lastPosCam = glm::vec3(lastPos, 1) * renderer.GetScreenToWorldMatrix();
			glm::vec2 curPosCam = glm::vec3(curPos, 1) * renderer.GetScreenToWorldMatrix();
			renderer.GetEditorCamTrx().position += lastPosCam - curPosCam;
			lastPos = curPos;
		}
	}

	ImGui::SetNextWindowSize(ImVec2(200, 400));
	ImGui::Begin("Tools");
	const char* listbox_items[] = { "Create Entity","Move Entity" };
	static int listbox_item_current = 0;
	ImGui::ListBox("Tool", &listbox_item_current, listbox_items, IM_ARRAYSIZE(listbox_items), 2);
	if (listbox_item_current == 0) {
		auto mousePos = glm::vec3(input.GetMousePosition(), 1) * renderer.GetScreenToWorldMatrix();
		if (input.GetMouseButton(LEFT_BUTTON).justPressed) {
			Logger::Log("create");
			created = registry.create();
			tree.AddEntity(created);
			selected = created;
			registry.emplace<GUID>(created);
			registry.emplace<Transform>(created, glm::vec2(mousePos.x, mousePos.y), glm::vec2(1, 1), 0);
		}
		if (created != entt::entity(-1)) {
			if (input.GetMouseButton(LEFT_BUTTON).justReleased) {
				created = entt::entity(-1);
			}
			if (input.GetMouseButton(LEFT_BUTTON).isPressed) {
				registry.get<Transform>(created).position = glm::vec2(mousePos.x, mousePos.y);
			}
		}
	}
	if (listbox_item_current == 1) {
		auto mousePos = glm::vec3(input.GetMousePosition(), 1) * renderer.GetScreenToWorldMatrix();
		if (selected != entt::entity(-1)) {
			if (input.GetMouseButton(LEFT_BUTTON).isPressed) {
				registry.get<Transform>(selected).position = glm::vec2(mousePos.x, mousePos.y);
			}
		}
	}
	auto view = registry.view<const GUID, Transform>();
	tree.Editor(registry, selected, entityList);
	if (ImGui::Button("Save Level")) {
		levelLoader.SaveLevel("SavedLevel.yaml", registry);
	}
	ImGui::End();


	if (selected != entt::entity(-1)) {
		if (registry.any_of<Transform>(selected)) {
			ImGui::Begin("Transform component");
			TransformEditor::DrawEditor(registry.get<Transform>(selected));
			ImGui::End();
		}
		if (registry.any_of<PhysicsBody>(selected)) {
			ImGui::Begin("PhysicsBody component");
			auto trx = registry.get<Transform>(selected);
			PhysicsEditor::DrawEditor(registry.get<PhysicsBody>(selected), trx);
			ImGui::End();
			if (ImGui::Button("Remove PhysicsBody Component")) {
				registry.remove<PhysicsBody>(selected);
			}
		}
		else {
			if (ImGui::Button("Add PhysicsBody Component")) {
				registry.emplace<PhysicsBody>(selected);
			}
		}
		if (registry.any_of<Sprite>(selected)) {
			ImGui::Begin("Sprite component");
			SpriteEditor::DrawEditor(registry.get<Sprite>(selected));
			ImGui::End();
			if (ImGui::Button("Remove Sprite Component")) {
				registry.remove<Sprite>(selected);
			}
		}
		else {
			if (ImGui::Button("Add Sprite Component")) {
				registry.emplace<Sprite>(selected, "block");
			}
		}
		if (registry.any_of<Camera>(selected)) {
			ImGui::Begin("Camera component");
			CameraEditor::DrawEditor(registry.get<Camera>(selected));
			ImGui::End();
			if (ImGui::Button("Remove Camera Component")) {
				registry.remove<Camera>(selected);
			}
		}
		else {
			if (ImGui::Button("Add Camera Component")) {
				registry.emplace<Camera>(selected);
			}
		}
	}
}

#endif // _EDITOR
void Game::Render()
{

	RendererSystem& renderer = entt::locator<RendererSystem>::value();
	renderer.Render();
#ifdef _DEBUG
	PhysicsSystem& physics = entt::locator<PhysicsSystem>::value();
	physics.DebugRender(renderer.GetWorldToScreenMatrix());

	TransformSystem& transformSystem = entt::locator<TransformSystem>::value();
	transformSystem.GetDebugRenderer().SetMatrix(renderer.GetWorldToScreenMatrix());
	transformSystem.DebugRender(renderer.GetWorldToScreenMatrix());
#endif // _DEBUG

#ifdef _EDITOR
	auto camEntity = renderer->GetCamera();
	auto cam = registry.get<Camera>(camEntity);
	auto camTrx = registry.get<Transform>(camEntity);
	CameraEditor::DrawGizmos(sdl->GetRenderer(), *renderer, cam, camTrx);

	if (selected != entt::entity(-1)) {
		transformSystem.GetDebugRenderer().DrawTransform(registry.get<Transform>(selected));
	}
#endif
	renderer.Present();

#ifdef _EDITOR
	imgui.Render();
	//ImGui::ShowDemoWindow();
	Editor(registry, input, *renderer, levelTree, levelLoader, *physics);
	//Imgui Code
	imgui.Present();
#endif // _EDITOR

}

void Game::RegisterAllEntities()
{
	Entities& entities = entt::locator<Entities>::value();
	entt::registry& registry = entities.GetRegistry();
	entities.DeleteAllEntities();
	auto view = registry.view<const GUIDComponent>();
	for (auto entity : view) {
		const auto& guid = view.get<GUIDComponent>(entity);
		entities.AddEntity(guid.id, entity);
	}
}
