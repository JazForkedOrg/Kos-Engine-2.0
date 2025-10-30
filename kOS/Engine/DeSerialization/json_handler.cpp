/******************************************************************/
/*!
\file      json_handler.cpp
\author    Jaz Winn Ng, jazwinn.ng, 2301502 
\par       jazwinn.ngc@digipen.edu
\date      Aug 29, 2025
\brief     This file handles JSON-related operations for component serialization and deserialization in the ECS framework.
			- m_LoadConfig: Loads configuration settings such as window dimensions and FPS cap from a config file.
			- m_JsonFileValidation: Validates if the JSON file exists and creates a new one if it doesn't.
			- m_LoadComponentsJson: Loads component data from a JSON file into the ECS.
			- m_SaveComponentsJson: Saves component data from the ECS to a JSON file.
			- m_SaveEntity: Serializes individual entity data to a JSON structure.
			- m_LoadEntity: Deserializes individual entity data from a JSON structure.
			- m_LoadEntity: Deserializes individual entity data from a JSON structure.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/********************************************************************/
/********************************************************************/

//REFLECTION

//SAVING

//template <typename T>//T refers to component
//void m_saveComponentreflect(T* component, rapidjson::Value& entityData, rapidjson::Document::AllocatorType& allocator);

//LOADING

//template <typename T>//T refers to component
//void m_LoadComponentreflect(T* component, rapidjson::Value& entityData);


/********************************************************************/
#include "Config/pch.h"
#include "json_handler.h"

#include <RAPIDJSON/filewritestream.h>
#include <RAPIDJSON/istreamwrapper.h>
#include <RAPIDJSON/prettywriter.h>
#include <RAPIDJSON/filereadstream.h>


#include "ECS/ECS.h"
#include "Debugging/Logging.h"
#include "ECS/Hierachy.h"


#include "Physics/PhysicsLayer.h"

namespace Serialization {



	std::string configFilePath;

	void JsonFileValidation(const std::string& filePath) {

		std::ifstream checkFile(filePath);

		if (!checkFile) {
			std::ofstream createFile(filePath);
			createFile << "[]";  // Initialize empty JSON array
			createFile.close();
		}

		checkFile.close();
	}

	void LoadScene(const std::filesystem::path& jsonFilePath)
	{


		// Open the JSON file for reading
		std::ifstream inputFile(jsonFilePath.string());

		if (!inputFile) {
			LOGGING_ERROR("Failed to open JSON file for reading: {}", jsonFilePath.string().c_str());
			return;
		}

		std::string fileContent((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
		inputFile.close();

		// Parse the JSON content
		rapidjson::Document doc;
		doc.Parse(fileContent.c_str());

		std::string scenename = jsonFilePath.filename().string();

		
		

		/*******************INSERT INTO FUNCTION*****************************/

		// Iterate through each component entry in the JSON array
		for (rapidjson::SizeType i = 0; i < doc.Size(); i++) {
			const rapidjson::Value& entityData = doc[i];

			if (entityData.HasMember(SceneData::classname())) {
				//load scene data
				SceneData sceneData;
				LoadComponentreflect(&sceneData, entityData);
				ecs::ECS::GetInstance()->AddScene(scenename, sceneData);

			}
			else {
				LoadEntity(entityData, std::nullopt, scenename);
			}
		}



		LOGGING_INFO("Load Json Successful");
	}

	void SaveScene(const std::filesystem::path& scene)
	{
		auto* ecs = ecs::ECS::GetInstance();
		std::string jsonFilePath = scene.string();
		JsonFileValidation(jsonFilePath);

		// Create JSON object to hold the updated values
		rapidjson::Document doc;

		if (!doc.IsArray()) {
			doc.SetArray();  // Initialize as an empty array
		}

		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

		//save scene data
		auto sceneName = scene.filename().string();
		{
			
			SceneData data;
			if (ecs->sceneMap.find(sceneName) != ecs->sceneMap.end())
			{
				data = ecs->sceneMap.at(sceneName);
			}

			rapidjson::Value sceneData(rapidjson::kObjectType);
			saveComponentreflect(&data, sceneData, allocator);
			doc.PushBack(sceneData, allocator);
		}



		{
			if (ecs->sceneMap.find(sceneName) != ecs->sceneMap.end())
			{
				std::unordered_set<ecs::EntityID> savedEntities;  //track saved entities
				//Start saving the entities
				std::vector<ecs::EntityID> entities = ecs->sceneMap.find(sceneName)->second.sceneIDs;
				for (const auto& entityId : entities) {
					if (!hierachy::GetParent(entityId).has_value()) {
						SaveEntity(entityId, doc, allocator, savedEntities);
					}
				}
			}
		}



		// Write the JSON back to file
		rapidjson::StringBuffer writeBuffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(writeBuffer);
		doc.Accept(writer);

		std::ofstream outputFile(jsonFilePath);
		//if file exist
		if (outputFile) {
			outputFile << writeBuffer.GetString();
			outputFile.close();
		}
		else { //craete new file
			std::ofstream createFile(jsonFilePath);
			createFile << writeBuffer.GetString();
			createFile.close();
		}

		LOGGING_INFO("Save Json Successful");
	}

	void SaveEntity(ecs::EntityID entityId, rapidjson::Value& parentArray, rapidjson::Document::AllocatorType& allocator, std::unordered_set<ecs::EntityID>& savedEntities) {
		auto* ecs = ecs::ECS::GetInstance();
		auto signature = ecs->GetEntitySignature(entityId);

		if (savedEntities.find(entityId) != savedEntities.end()) {
			return;
		}

		rapidjson::Value entityData(rapidjson::kObjectType);

		
		const auto& componentKey = ecs->GetComponentKeyData();
		for (const auto& [ComponentName, key] : componentKey) {
			if (signature.test(ecs->GetComponentKey(ComponentName))) {
				auto* component = ecs->GetIComponent<ecs::Component*>(ComponentName,entityId);
				if (component) {
					auto& actionInvoker = ecs->componentAction[ComponentName];
					actionInvoker->Save(component, entityData, allocator);
				}
			}
		}


		// Add children
		std::optional<std::vector<ecs::EntityID>> childrenOptional = hierachy::m_GetChild(entityId);
		if (childrenOptional.has_value()) {
			std::vector<ecs::EntityID> children = childrenOptional.value();
			if (!children.empty()) {
				rapidjson::Value childrenArray(rapidjson::kArrayType);
				for (const auto& childID : children) {
					SaveEntity(childID, childrenArray, allocator, savedEntities);
				}
				entityData.AddMember("children", childrenArray, allocator);
			}
		}

		parentArray.PushBack(entityData, allocator);
		savedEntities.insert(entityId);
	}

	void LoadEntity(const rapidjson::Value& entityData, std::optional<ecs::EntityID> parentID, const std::string& sceneName)
	{
		ecs::ECS* ecs = ecs::ECS::GetInstance();
		ecs::EntityID newEntityId = ecs->CreateEntity(sceneName);

		const auto& componentKey = ecs->GetComponentKeyData();
		for (const auto& [ComponentName, key] : componentKey) {
			if (entityData.HasMember(ComponentName.c_str()) && entityData[ComponentName.c_str()].IsObject()) {
				auto& actionInvoker = ecs->componentAction[ComponentName];
				actionInvoker->Load(newEntityId, entityData);
			}

		}


		//Attach entity to parent
		if (parentID.has_value()) {
			hierachy::m_SetParent(parentID.value(), newEntityId);
		}

		// Load children 
		if (entityData.HasMember("children") && entityData["children"].IsArray()) {
			const rapidjson::Value& childrenArray = entityData["children"];
			for (rapidjson::SizeType i = 0; i < childrenArray.Size(); i++) {
				LoadEntity(childrenArray[i], newEntityId, sceneName);
			}
		}
	}

}

