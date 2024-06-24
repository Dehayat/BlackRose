#include "Editor.h"

#include <imgui.h>
#include <imgui_impl_sdlrenderer2.h>

#include <imgui_impl_sdl2.h>

#include <FileDialog.h>

#include "Core/SdlContainer.h"
#include "Levels/LevelLoader.h"
#include "AssetPipline/AssetStore.h"

#include "Input/InputSystem.h"
#include "Renderer/Renderer.h"
#include "Core/Transform.h"
#include "Animation/AnimationSystem.h"
#include "Events/EntityEventSystem.h"
#include "Scripting/ScriptSystem.h"
#include "Core/DisableSystem.h"

#include "Core/Systems.h"

#include "Components/GUIDComponent.h"
#include "Components/PhysicsBodyComponent.h"
#include "Components/TransformComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/CameraComponent.h"
#include "Components/AnimationComponent.h"
#include "Components/ScriptComponent.h"
#include "Components/SendEventsToParentComponent.h"

#include "Editor/PhysicsEditor.h"
#include "Editor/ScriptEditor.h"

#include "DefaultEditor.h"

#define ROSE_DEFAULT_COMP_EDITOR(COMP,DEL)	ROSE_INIT_VARS(COMP);\
											RenderComponent<COMP, DefaultComponentEditor<COMP>>(DEL, #COMP, selectedEntity)



void Editor::Run()
{
	Setup();

#ifdef _DEBUG
	TransformSystem& transformSystem = ROSE_GETSYSTEM(TransformSystem);
	transformSystem.InitDebugDrawer();
	PhysicsSystem& physics = ROSE_GETSYSTEM(PhysicsSystem);
	physics.InitDebugDrawer();
	physics.EnableDebug(true);
	transformSystem.EnableDebug(true);
#endif // !_DEBUG

	isRunning = true;
	while(isRunning)
	{
		Update();
		Render();
	}
}


Editor::Editor():BaseGame()
{
	SetupImgui();
	Reset();
	isGameRunning = false;
	selectedTool = Tools::SelectEntity;
	gizmosSetting = Gizmos::ALL;
}

void Editor::SetupImgui()
{
	auto& sdl = ROSE_GETSYSTEM(SdlContainer);
	SDL_RenderSetVSync(sdl.GetRenderer(), 1);

	window = sdl.GetWindow();
	renderer = sdl.GetRenderer();
	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
	IMGUI_CHECKVERSION();
	auto imguiContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(imguiContext);
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer2_Init(renderer);
}

Editor::~Editor()
{
	CloseImgui();
}

void Editor::Reset()
{
	createdEntity = entt::entity(-1);
}

void Editor::CloseImgui()
{
	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

void Editor::Update()
{
	bool exitGame = ProcessEvents();
	if(exitGame)
	{
		isRunning = false;
	}
	bool isGameRunning = IsGameRunning();
	ROSE_GETSYSTEM(TimeSystem).Update();
	ROSE_GETSYSTEM(TransformSystem).Update();
	ROSE_GETSYSTEM(InputSystem).Update();
	if(isGameRunning)
	{
		ROSE_GETSYSTEM(PhysicsSystem).Update();
	}
	ROSE_GETSYSTEM(AnimationPlayer).Update();
	if(isGameRunning)
	{
		ROSE_GETSYSTEM(ScriptSystem).Update();
		ROSE_GETSYSTEM(EntityEventSystem).Update();
	}

	if(mouseInViewport)
	{
		UpdateViewportControls();
	}
	if(!ImGui::IsAnyItemActive())
	{
		UpdateGlobalControls();
	}
}
void Editor::Render()
{

	RendererSystem& renderer = ROSE_GETSYSTEM(RendererSystem);
	renderer.Render();
	TransformSystem& transformSystem = ROSE_GETSYSTEM(TransformSystem);
	transformSystem.GetDebugRenderer().SetMatrix(renderer.GetWorldToScreenMatrix());
	if(GetGizmos() == Gizmos::TRANSFORM || GetGizmos() == Gizmos::ALL)
	{
		transformSystem.DebugRender(renderer.GetWorldToScreenMatrix(), GetSelectedEntity());
	}
	if(GetGizmos() == Gizmos::PHYSICS || GetGizmos() == Gizmos::ALL)
	{
		ROSE_GETSYSTEM(PhysicsSystem).DebugRender(renderer.GetWorldToScreenMatrix());
	}
	RenderGizmos();
	RenderEditor();
	renderer.Present();
}

void Editor::UpdateViewportControls()
{
	auto& entities = ROSE_GETSYSTEM(Entities);
	auto& registry = entities.GetRegistry();
	auto& input = ROSE_GETSYSTEM(InputSystem);
	auto& gameRenderer = ROSE_GETSYSTEM(RendererSystem);
	if(selectedTool == Tools::CreateEntity)
	{
		auto mousePos = glm::vec3(input.GetMousePosition(), 1) * gameRenderer.GetScreenToWorldMatrix();
		if(input.GetMouseButton(LEFT_BUTTON).justPressed)
		{
			ROSE_LOG("entity created");
			createdEntity = entities.CreateEntity();
			levelTreeEditor.SelectEntity(createdEntity);
			registry.emplace<TransformComponent>(createdEntity, glm::vec2(mousePos.x, mousePos.y), glm::vec2(1, 1), 0);
		}
		if(createdEntity != entt::entity(-1))
		{
			if(input.GetMouseButton(LEFT_BUTTON).justReleased)
			{
				createdEntity = entt::entity(-1);
			}
			if(input.GetMouseButton(LEFT_BUTTON).isPressed)
			{
				registry.get<TransformComponent>(createdEntity).globalPosition = glm::vec2(mousePos.x, mousePos.y);
				registry.get<TransformComponent>(createdEntity).UpdateLocals();
			}
		}
	}
	if(selectedTool == Tools::MoveEntity)
	{
		auto mousePos = glm::vec3(input.GetMousePosition(), 1) * gameRenderer.GetScreenToWorldMatrix();
		if(levelTreeEditor.GetSelectedEntity() != entt::entity(-1))
		{
			if(input.GetMouseButton(LEFT_BUTTON).isPressed)
			{
				registry.get<TransformComponent>(levelTreeEditor.GetSelectedEntity()).globalPosition = glm::vec2(mousePos.x, mousePos.y);
				registry.get<TransformComponent>(levelTreeEditor.GetSelectedEntity()).UpdateLocals();
			}
		}
	}
	if(selectedTool == Tools::SelectEntity)
	{
		UpdateSelectTool();
	}
}

void Editor::UpdateGlobalControls()
{
	auto& entities = ROSE_GETSYSTEM(Entities);
	auto& input = ROSE_GETSYSTEM(InputSystem);
	auto& levelLoader = ROSE_GETSYSTEM(LevelLoader);
	if(input.GetKey(InputKey::LCTRL).isPressed || input.GetKey(InputKey::RCTRL).isPressed)
	{
		if(input.GetKey(InputKey::N).justPressed)
		{
			levelLoader.UnloadLevel();
			levelTreeEditor.CleanTree();
		}
		if(input.GetKey(InputKey::O).justPressed)
		{
			auto fileName = ROSE_GETSYSTEM(FileDialog).OpenFile("yaml");
			if(fileName != "")
			{
				levelLoader.UnloadLevel();
				levelTreeEditor.CleanTree();
				levelLoader.LoadLevel(fileName);
				ROSE_GETSYSTEM(RendererSystem).InitLoaded();
			}
		}
		if(input.GetKey(InputKey::S).justPressed)
		{
			if(input.GetKey(InputKey::LSHIFT).isPressed || input.GetKey(InputKey::RSHIFT).isPressed)
			{
				auto fileName = ROSE_GETSYSTEM(FileDialog).SaveFile("yaml");
				if(fileName != "")
				{
					levelLoader.SaveLevel(fileName);
				}
			} else
			{
				levelLoader.SaveLevel(levelLoader.GetCurrentLevelFile());
			}
		}
		if(input.GetKey(InputKey::D).justPressed)
		{
			if(GetSelectedEntity() != NoEntity())
			{
				levelTreeEditor.SelectEntity(ROSE_GETSYSTEM(Entities).Copy(GetSelectedEntity()));
			}
		}
		return;
	}

	if(GetSelectedEntity() != NoEntity())
	{
		if(input.GetKey(InputKey::DELETE).justReleased)
		{
			if(levelTreeEditor.GetSelectedEntity() != NoEntity())
			{
				entities.DestroyEntity(levelTreeEditor.GetSelectedEntity());
				levelTreeEditor.CleanTree();
			}
		}
	}

	if(input.GetKey(InputKey::C).justPressed)
	{
		selectedTool = Tools::CreateEntity;
	}
	if(input.GetKey(InputKey::M).justPressed)
	{
		selectedTool = Tools::MoveEntity;
	}
	if(input.GetKey(InputKey::S).justPressed)
	{
		selectedTool = Tools::SelectEntity;
	}
	if(input.GetKey(InputKey::X).justPressed)
	{
		selectedTool = Tools::NoTool;
	}
	if(input.GetKey(InputKey::G).justPressed)
	{
		gizmosSetting = (Gizmos)(gizmosSetting + 1);
		if(gizmosSetting > Gizmos::ALL)
		{
			gizmosSetting = Gizmos::NONE;
		}
	}
}
Gizmos Editor::GetGizmos()
{
	return gizmosSetting;
}


static bool IsPointInsideRect(vec2 point, SDL_FRect rect)
{
	if(point.x<rect.x || point.x > rect.x + rect.w)
	{
		return false;
	}
	if(point.y<rect.y || point.y > rect.y + rect.h)
	{
		return false;
	}
	return true;
}

void Editor::UpdateSelectTool()
{
	auto& entities = ROSE_GETSYSTEM(Entities);
	auto& registry = entities.GetRegistry();
	auto& input = ROSE_GETSYSTEM(InputSystem);
	if(input.GetMouseButton(LEFT_BUTTON).justPressed)
	{
		auto mousePos = input.GetMousePosition();
		auto view = registry.view<const SpriteComponent>();
		bool selected = false;
		for(auto entity : view)
		{
			auto& sprite = registry.get<SpriteComponent>(entity);
			if(IsPointInsideRect(mousePos, sprite.destRect))
			{
				levelTreeEditor.SelectEntity(entity);
				selected = true;
			}
		}
		if(!selected)
		{
			levelTreeEditor.SelectEntity(NoEntity());
		}
	}
}

bool Editor::ProcessEvents()
{
	bool exit = false;
	SDL_Event sdlEvent;
	while(SDL_PollEvent(&sdlEvent))
	{
		switch(sdlEvent.type)
		{
		case SDL_QUIT:
			exit = true;
			break;
		case SDL_KEYDOWN:
			if(sdlEvent.key.keysym.sym == SDLK_ESCAPE)
			{
				exit = true;
			}
			break;
		default:
			break;
		}
		ImGui_ImplSDL2_ProcessEvent(&sdlEvent);
	}
	return exit;
}

void Editor::RenderGizmos()
{
}

void Editor::RenderEditor()
{
	levelTreeEditor.CleanTree();
	RenderImgui();

	int w, h;
	SDL_GetWindowSize(window, &w, &h);

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;
	ImGui::Begin("ToolBar", nullptr, windowFlags);
	ImGui::SetWindowSize(ImVec2(w, 90));
	ImGui::SetWindowPos(ImVec2(0, 0));
	RenderTools();
	ImGui::End();

	ImGui::Begin("Level Tree", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	ImGui::SetWindowSize(ImVec2(200, 200));
	ImGui::SetWindowPos(ImVec2(w - 200, 90));
	levelTreeEditor.Editor();
	ImGui::End();

	ImGui::Begin("Entity Properties", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	ImGui::SetWindowSize(ImVec2(200, h - 290));
	ImGui::SetWindowPos(ImVec2(w - 200, 290));
	EntityEditor();
	ImGui::End();

	PresentImGui();
}

entt::entity Editor::GetSelectedEntity()
{
	return levelTreeEditor.GetSelectedEntity();
}

bool Editor::IsGameRunning()
{
	return isGameRunning;
}

void Editor::RenderTools()
{
	auto& entities = ROSE_GETSYSTEM(Entities);
	auto& registry = entities.GetRegistry();
	auto& input = ROSE_GETSYSTEM(InputSystem);
	auto& gameRenderer = ROSE_GETSYSTEM(RendererSystem);
	auto& levelLoader = ROSE_GETSYSTEM(LevelLoader);
	ImGui::BeginTable("ToolBar", 3, ImGuiTableFlags_BordersInnerV);
	ImGui::TableNextColumn();
	ImGui::BeginTable("Tools", 3, ImGuiTableFlags_BordersInnerV);
	ImGui::TableNextColumn();
	RenderToolButton("Move", Tools::MoveEntity);
	RenderToolButton("Select", Tools::SelectEntity);
	RenderToolButton("Create", Tools::CreateEntity);
	ImGui::TableNextColumn();
	if(ImGui::Button("Delete"))
	{
		if(levelTreeEditor.GetSelectedEntity() != NoEntity())
		{
			entities.DestroyEntity(levelTreeEditor.GetSelectedEntity());
			levelTreeEditor.CleanTree();
		}
	}

	ImGui::TableNextColumn();
	if(IsGameRunning())
	{
		if(ImGui::Button("Stop"))
		{
			isGameRunning = false;
		}
	} else
	{
		if(ImGui::Button("Play"))
		{
			isGameRunning = true;
			selectedTool = Tools::NoTool;
		}
	}
	ImGui::EndTable();

	ImGui::TableNextColumn();
	{
		if(ImGui::Button("Load Package"))
		{
			auto fileName = ROSE_GETSYSTEM(FileDialog).OpenFile("pkg");
			if(fileName != "")
			{
				AssetStore& assetStore = ROSE_GETSYSTEM(AssetStore);
				assetStore.LoadPackage(fileName);
			}
		}
		ImGui::SameLine();
	}

	ImGui::TableNextColumn();
	auto view = registry.view<const GUIDComponent, TransformComponent>();
	{
		if(ImGui::Button("Load Level"))
		{
			auto fileName = ROSE_GETSYSTEM(FileDialog).OpenFile("yaml");
			if(fileName != "")
			{
				levelLoader.UnloadLevel();
				levelTreeEditor.CleanTree();
				levelLoader.LoadLevel(fileName);
				ROSE_GETSYSTEM(RendererSystem).InitLoaded();
			}
		}
	}
	{
		if(ImGui::Button("Save Level"))
		{
			if(levelLoader.GetCurrentLevelFile() != "")
			{
				levelLoader.SaveLevel(levelLoader.GetCurrentLevelFile());
			}
		}
	}
	{
		if(ImGui::Button("Save Level As"))
		{
			auto fileName = ROSE_GETSYSTEM(FileDialog).SaveFile("yaml");
			if(fileName != "")
			{
				levelLoader.SaveLevel(fileName);
			}
		}
		ImGui::SameLine();
	}

	ImGui::EndTable();
}

void Editor::RenderToolButton(std::string name, Tools tool)
{
	bool isSelected = selectedTool == tool;
	if(isSelected)
	{
		auto activeColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
		ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
	}
	if(ImGui::Button(name.c_str()))
	{
		selectedTool = tool;
	}
	if(isSelected)
	{
		ImGui::PopStyleColor();
	}
}

void Editor::EntityEditor()
{
	auto& entities = ROSE_GETSYSTEM(Entities);
	auto& registry = entities.GetRegistry();
	auto selectedEntity = levelTreeEditor.GetSelectedEntity();
	if(levelTreeEditor.GetSelectedEntity() != entt::entity(-1))
	{
		RenderEntityEditor(selectedEntity);
		ROSE_DEFAULT_COMP_EDITOR(GUIDComponent, false);
		ROSE_DEFAULT_COMP_EDITOR(TransformComponent, false);
		RenderComponent<PhysicsBodyComponent, PhysicsEditor>(true, "Physics Body Component", selectedEntity);
		ROSE_DEFAULT_COMP_EDITOR(SpriteComponent, true);
		ROSE_DEFAULT_COMP_EDITOR(CameraComponent, true);
		ROSE_DEFAULT_COMP_EDITOR(AnimationComponent, true);
		RenderComponent<ScriptComponent, ScriptEditor>(true, "Script Component", selectedEntity);
		ROSE_DEFAULT_COMP_EDITOR(SendEventsToParentComponent, true);
	}
}
void Editor::RenderEntityEditor(entt::entity entity)
{
	static bool enabled = true;
	auto& entities = ROSE_GETSYSTEM(Entities);
	auto& registry = entities.GetRegistry();
	if(registry.any_of<DisableComponent>(entity))
	{
		enabled = !registry.get<DisableComponent>(entity).selfDisabled;
	} else
	{
		enabled = true;
	}
	ImGui::SeparatorText("Entity");
	if(ImGui::Checkbox("Enabled", &enabled))
	{
		if(enabled)
		{
			ROSE_GETSYSTEM(DisableSystem).Enable(entity);
		} else
		{
			ROSE_GETSYSTEM(DisableSystem).Disable(entity);
		}
	}
}


void Editor::PresentImGui()
{
	ImGui::Render();
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
}

void Editor::RenderImgui()
{
	ImGuiIO& io = ImGui::GetIO();
	mouseInViewport = !io.WantCaptureMouse;
	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	io.DisplaySize = ImVec2((float)w, (float)h);
	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}
