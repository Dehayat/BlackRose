#include "AssetStore.h"

#include <sdl2/SDL_image.h>

#include "Animation/AnimationImporter.h"
#include "Core/SdlContainer.h"

#include "Core/Systems.h"

#include "AssetPackage.h"

#include "ScriptAsset.h"

#include "Debugging/Logger.h"


AssetStore::AssetStore()
{
	Logger::Log("Asset Store created");
}

AssetStore::~AssetStore()
{
	ClearAssets();
	Logger::Log("Asset Store destroyed");
}

void AssetStore::ClearAssets()
{

	for (auto& asset : assets) {
		delete asset.second.asset;
	}
	assets.clear();
}

void AssetStore::AddTexture(const std::string& assetId, const std::string& filePath, int ppu)
{
	SDL_Renderer* renderer = GETSYSTEM(SdlContainer).GetRenderer();
	SDL_Surface* surface = IMG_Load(filePath.c_str());
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	auto textureAsset = new TextureAsset(texture, ppu);
	if (assets.find(assetId) != assets.end()) {
		delete assets[assetId].asset;
		assets[assetId].type = AssetType::Texture;
		assets[assetId].asset = textureAsset;
	}
	else {
		assets[assetId] = AssetHandle(AssetType::Texture, textureAsset);
	}
	Logger::Log("Loaded Texture Asset " + assetId);
}

void AssetStore::LoadAnimation(const std::string& assetId, const std::string& filePath)
{
	auto animation = AnimationImporter::LoadAnimation(filePath);
	if (assets.find(assetId) != assets.end()) {
		delete assets[assetId].asset;
		assets[assetId].type = AssetType::Animation;
		assets[assetId].asset = animation;
	}
	else {
		assets[assetId] = AssetHandle(AssetType::Animation, animation);
	}
	Logger::Log("Loaded Animation Asset " + assetId);
}

void AssetStore::LoadScript(const std::string& assetId, const std::string& filePath)
{
	FileResource fileHandle = FileResource(filePath);
	if (fileHandle.file == nullptr) {
		Logger::Error("Couldn't load script file: " + filePath);
		return;
	}
	std::string fileString = std::string("\0", SDL_RWsize(fileHandle.file));
	SDL_RWread(fileHandle.file, &fileString[0], sizeof(fileString[0]), fileString.size());

	auto script = new ScriptAsset(fileString);
	if (assets.find(assetId) != assets.end()) {
		delete assets[assetId].asset;
		assets[assetId].type = AssetType::Script;
		assets[assetId].asset = script;
	}
	else {
		assets[assetId] = AssetHandle(AssetType::Script, script);
	}
	Logger::Log("Loaded Script Asset " + assetId);
}

AssetHandle AssetStore::GetAsset(const std::string& assetId) const
{
	if (assets.find(assetId) == assets.end()) {
		return AssetHandle();
	}
	return assets.at(assetId);
}

void AssetStore::LoadPackage(const std::string& filePath)
{
	auto pkg = new AssetPackage();
	if (pkg->Load(filePath)) {
		for (auto assetFile : pkg->assets) {
			switch (assetFile->assetType)
			{
			case AssetType::Texture:
			{
				auto metaData = (TextureMetaData*)(assetFile->metaData);
				AddTexture(metaData->name, assetFile->filePath, metaData->ppu);
				break;
			}
			case AssetType::Animation:
			{
				auto metaData = (AnimationMetaData*)(assetFile->metaData);
				LoadAnimation(metaData->name, assetFile->filePath);
				break;
			}
			case AssetType::Script:
			{
				auto metaData = (AssetMetaData*)(assetFile->metaData);
				LoadScript(metaData->name, assetFile->filePath);
				break;
			}
			default:
				break;
			}
		}
		delete pkg;
	}
	else {
		Logger::Log("Failed to load asset package" + filePath);
	}

}