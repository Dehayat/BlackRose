#include "ScriptSystem/ScriptSystem.h"

#include "Systems.h"
#include "Entity.h"

#include "Components/ScriptComponent.h"

ScriptSystem::ScriptSystem()
{
	entt::registry& registry = GETSYSTEM(Entities).GetRegistry();
	registry.on_construct<ScriptComponent>().connect<&ScriptSystem::ScriptComponentCreated>(this);
}

void ScriptSystem::ScriptComponentCreated(entt::registry& registry, entt::entity entity)
{
	newlyAddedScripts.push(entity);
}

void ScriptSystem::Update()
{
	entt::registry& registry = GETSYSTEM(Entities).GetRegistry();
	while (!newlyAddedScripts.empty()) {
		auto& entity = newlyAddedScripts.front();
		auto& scriptComponent = registry.get<ScriptComponent>(entity);
		scriptComponent.script->Setup();
		newlyAddedScripts.pop();
	}
	auto view = registry.view<ScriptComponent>();
	for (auto entity : view) {
		auto& scriptComponent = registry.get<ScriptComponent>(entity);
		scriptComponent.script->Update();
	}
}