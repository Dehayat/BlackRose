#include "EventSystem/EntityEventSystem.h"

#include <entt/entt.hpp>

#include "Entity.h"

#include "Systems.h"

#include "Components/ScriptComponent.h"

#include "Logger.h"

void EntityEventSystem::Update()
{
	auto& registry = GETSYSTEM(Entities).GetRegistry();

	while (!eventQueue.empty()) {
		auto& entityEvent = eventQueue.front();
		auto scriptComponent = registry.try_get<ScriptComponent>(entityEvent.entity);
		if (scriptComponent != nullptr) {
			scriptComponent->script->OnEvent(entityEvent);
		}
		eventQueue.pop();
	}
}

void EntityEventSystem::QueueEvent(EntityEvent entityEvent)
{
	eventQueue.push(entityEvent);
}