#include "Components/TransformComponent.h"

#include <entt/entt.hpp>

#include "Systems.h"

using namespace glm;

TransformComponent::TransformComponent(vec2 position, vec2 scale, float rotation, entt::entity parent)
{
	this->position = position;
	this->scale = scale;
	this->rotation = rotation;
	this->parent = parent;

	this->level = 0;
	this->matrixL2W = mat3(0);
	globalPosition = vec2();
	globalScale = vec2();
	globalRotation = 0;
	this->hasParent = false;
	this->parentGUID = -1;
}

TransformComponent::TransformComponent(ryml::NodeRef& node)
{
	this->position = vec2(0, 0);
	this->scale = vec2(1, 1);
	this->rotation = 0;
	this->parent = NoEntity();

	this->matrixL2W = mat3(0);
	this->level = 0;
	globalPosition = vec2();
	globalScale = vec2();
	globalRotation = 0;
	this->hasParent = false;
	this->parentGUID = -1;

	if (node.has_child("position")) {
		node["position"][0] >> position.x;
		node["position"][1] >> position.y;
	}
	if (node.has_child("scale")) {
		node["scale"][0] >> scale.x;
		node["scale"][1] >> scale.y;
	}
	if (node.has_child("rotation")) {
		node["rotation"] >> rotation;
	}
	if (node.has_child("parent")) {
		node["parent"] >> parentGUID;
	}
}

void TransformComponent::Serialize(ryml::NodeRef node)
{
	node |= ryml::MAP;
	node["position"] |= ryml::SEQ;
	node["position"].append_child() << position.x;
	node["position"].append_child() << position.y;
	node["scale"] |= ryml::SEQ;
	node["scale"].append_child() << scale.x;
	node["scale"].append_child() << scale.y;
	node["rotation"] << rotation;
	if (hasParent) {
		node["parent"] << parentGUID;
	}
}

void TransformComponent::CalcMatrix()
{
	auto matT = mat3(
		1, 0, position.x,
		0, 1, position.y,
		0, 0, 1
	);
	auto cosT = cos(glm::radians(rotation));
	auto sinT = sin(glm::radians(rotation));
	auto matR = mat3(
		cosT, -sinT, 0,
		sinT, cosT, 0,
		0, 0, 1
	);
	auto matS = mat3(
		scale.x, 0, 0,
		0, scale.y, 0,
		0, 0, 1
	);
	matrixL2W = matS * matR * matT;
	if (hasParent) {
		entt::registry& registry = GETSYSTEM(Entities).GetRegistry();
		auto matrixP2W = registry.get<TransformComponent>(parent).matrixL2W;
		matrixL2W = matrixL2W * matrixP2W;
	}
}

void TransformComponent::UpdateGlobals()
{
	auto pos = glm::vec3(0, 0, 1);
	auto globalPos = pos * matrixL2W;
	globalPosition.x = globalPos.x;
	globalPosition.y = globalPos.y;

	globalScale = GetScale(matrixL2W);

	globalRotation = GetRotation(matrixL2W);
}

float TransformComponent::GetRotation(mat3 matrix)
{
	auto dirVec = vec2(matrix[1][0], matrix[0][0]);
	normalize(dirVec);
	auto rotation = glm::degrees(std::atan2f(dirVec.x, dirVec.y));
	return rotation;
}
vec2 TransformComponent::GetPosition(mat3 matrix, vec2 localPos)
{
	auto pos = vec3(localPos, 1);
	return pos * matrix;
}
vec2 TransformComponent::GetDir(mat3 matrix, vec2 localDir)
{
	auto dir = vec3(localDir, 0);
	return normalize(dir * matrix);
}
vec2 TransformComponent::GetScale(mat3 matrix) {
	auto scale = glm::vec2(glm::sqrt(matrix[0][0] * matrix[0][0] + matrix[1][0] * matrix[1][0]), glm::sqrt(matrix[0][1] * matrix[0][1] + matrix[1][1] * matrix[1][1]));
	return scale;
}