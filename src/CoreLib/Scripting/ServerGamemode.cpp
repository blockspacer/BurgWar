// Copyright (C) 2019 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CoreLib/Scripting/ServerGamemode.hpp>
#include <Nazara/Math/Vector2.hpp>
#include <NDK/Components.hpp>
#include <CoreLib/Match.hpp>
#include <CoreLib/Terrain.hpp>
#include <cassert>
#include <iostream>

namespace bw
{
	void ServerGamemode::InitializeGamemode()
	{
		auto& context = GetScriptingContext();

		auto Load = [&](const std::filesystem::path& filepath)
		{
			return context->Load(filepath);
		};

		sol::state& state = context->GetLuaState();
		state["GM"] = GetGamemodeTable();

		const std::filesystem::path& gamemodePath = GetGamemodePath();
		Load(gamemodePath / "shared.lua");
		Load(gamemodePath / "sv_init.lua");

		sol::table& gamemodeTable = GetGamemodeTable();
		gamemodeTable["CreateEntity"] = [&](const sol::table& gmTable, const std::string& entityType, const Nz::Vector2f& spawnPos, const sol::object& properties)
		{
			auto& entityStore = m_match.GetEntityStore();

			if (std::size_t elementIndex = entityStore.GetElementIndex(entityType); elementIndex != ServerEntityStore::InvalidIndex)
			{
				EntityProperties entityProperties;
				if (properties)
				{
					sol::table propertiesTable = properties;

					const auto& entityPtr = entityStore.GetElement(elementIndex);
					for (auto&& [propertyName, propertyData] : entityPtr->properties)
					{
						sol::object propertyValue = propertiesTable[propertyName];
						if (propertyValue)
							entityProperties.emplace(propertyName, TranslateEntityPropertyFromLua(propertyValue, propertyData.type, propertyData.isArray));
					}
				}

				const Ndk::EntityHandle& entity = entityStore.InstantiateEntity(m_match.GetTerrain().GetLayer(0).GetWorld(), elementIndex, spawnPos, 0.f, entityProperties);
				if (!entity)
					throw std::runtime_error("Failed to create \"" + entityType + "\"");

				auto& scriptComponent = entity->GetComponent<ScriptComponent>();
				return scriptComponent.GetTable();
			}
			else
				throw std::runtime_error("Entity type \"" + entityType + "\" doesn't exist");
		};
	}
}
