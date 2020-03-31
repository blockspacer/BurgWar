// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CoreLib/Player.hpp>
#include <Nazara/Core/CallOnExit.hpp>
#include <Nazara/Graphics/Material.hpp>
#include <Nazara/Graphics/Sprite.hpp>
#include <Nazara/Physics2D/Collider2D.hpp>
#include <NDK/Components.hpp>
#include <CoreLib/BurgApp.hpp>
#include <CoreLib/ConfigFile.hpp>
#include <CoreLib/Map.hpp>
#include <CoreLib/Match.hpp>
#include <CoreLib/MatchClientSession.hpp>
#include <CoreLib/MatchClientVisibility.hpp>
#include <CoreLib/Scripting/ServerScriptingLibrary.hpp>
#include <CoreLib/Terrain.hpp>
#include <CoreLib/TerrainLayer.hpp>
#include <CoreLib/Components/CooldownComponent.hpp>
#include <CoreLib/Components/InputComponent.hpp>
#include <CoreLib/Components/HealthComponent.hpp>
#include <CoreLib/Components/MatchComponent.hpp>
#include <CoreLib/Components/NetworkSyncComponent.hpp>
#include <CoreLib/Components/OwnerComponent.hpp>
#include <CoreLib/Components/PlayerControlledComponent.hpp>
#include <CoreLib/Components/PlayerMovementComponent.hpp>
#include <CoreLib/Components/ScriptComponent.hpp>
#include <CoreLib/Components/WeaponComponent.hpp>
#include <CoreLib/Scripting/ServerGamemode.hpp>

namespace bw
{
	Player::Player(Match& match, MatchClientSession& session, std::size_t playerIndex, Nz::UInt8 localIndex, std::string playerName) :
	m_layerIndex(NoLayer),
	m_activeWeaponIndex(NoWeapon),
	m_inputIndex(0),
	m_playerIndex(playerIndex),
	m_name(std::move(playerName)),
	m_localIndex(localIndex),
	m_match(match),
	m_session(session),
	m_isAdmin(false),
	m_isReady(false),
	m_shouldSendWeapons(false)
	{
	}

	Player::~Player()
	{
		MatchClientVisibility& visibility = GetSession().GetVisibility();
		for (std::size_t layerIndex = m_visibleLayers.FindFirst(); layerIndex != m_visibleLayers.npos; layerIndex = m_visibleLayers.FindNext(layerIndex))
			visibility.HideLayer(static_cast<LayerIndex>(layerIndex));
	}

	bool Player::GiveWeapon(std::string weaponClass)
	{
		if (m_layerIndex == NoLayer)
			return false;

		if (!m_playerEntity)
			return false;

		if (HasWeapon(weaponClass))
			return false;

		Terrain& terrain = m_match.GetTerrain();

		ServerWeaponStore& weaponStore = m_match.GetWeaponStore();

		// Create weapon
		if (std::size_t weaponEntityIndex = weaponStore.GetElementIndex(weaponClass); weaponEntityIndex != ServerEntityStore::InvalidIndex)
		{
			Nz::Int64 uniqueId = m_match.AllocateUniqueId();

			const Ndk::EntityHandle& weapon = weaponStore.InstantiateWeapon(terrain.GetLayer(m_layerIndex), weaponEntityIndex, uniqueId, {}, m_playerEntity);
			if (!weapon)
				return false;

			m_match.RegisterEntity(uniqueId, weapon);

			weapon->AddComponent<OwnerComponent>(CreateHandle());

			std::size_t weaponIndex = m_weapons.size();
			m_weapons.emplace_back(weapon);
			m_weaponByName.emplace(std::move(weaponClass), weaponIndex);

			m_shouldSendWeapons = true;
		}

		return true;
	}

	void Player::HandleConsoleCommand(const std::string& str)
	{
		if (!m_isAdmin)
			return;

		if (!m_scriptingEnvironment)
		{
			const std::string& scriptFolder = m_match.GetApp().GetConfig().GetStringValue("Assets.ScriptFolder");

			m_scriptingEnvironment.emplace(m_match.GetLogger(), m_match.GetScriptingLibrary(), std::make_shared<VirtualDirectory>(scriptFolder));
			m_scriptingEnvironment->SetOutputCallback([ply = CreateHandle()](const std::string& text, Nz::Color color)
			{
				if (!ply)
					return;

				Packets::ConsoleAnswer answer;
				answer.color = color;
				answer.localIndex = ply->GetLocalIndex();
				answer.response = text;

				ply->SendPacket(std::move(answer));
			});
		}

		m_scriptingEnvironment->Execute(str);
	}

	void Player::MoveToLayer(LayerIndex layerIndex)
	{
		if (m_layerIndex != layerIndex)
		{
			m_match.GetGamemode()->ExecuteCallback("OnPlayerChangeLayer", CreateHandle(), layerIndex);

			if (m_layerIndex != NoLayer)
				UpdateLayerVisibility(m_layerIndex, false);

			if (m_layerIndex != NoLayer && layerIndex != NoLayer)
			{
				if (m_playerEntity)
				{
					Terrain& terrain = m_match.GetTerrain();
					Ndk::World& world = terrain.GetLayer(layerIndex).GetWorld();

					const Ndk::EntityHandle& newPlayerEntity = world.CloneEntity(m_playerEntity);
					/*const auto& componentBits = m_playerEntity->GetComponentBits();
					for (std::size_t i = componentBits.FindFirst(); i != componentBits.npos; i = componentBits.FindNext(i))
					{
						// Leave physics component because dropping it during a physics callback would crash the physics engine
						if (i != Ndk::GetComponentIndex<MatchComponent>() && i != Ndk::GetComponentIndex<Ndk::PhysicsComponent2D>())
							newPlayerEntity->AddComponent(m_playerEntity->DropComponent(static_cast<Ndk::ComponentIndex>(i)));
					}

					newPlayerEntity->AddComponent(m_playerEntity->GetComponent<Ndk::PhysicsComponent2D>().Clone());*/

					Nz::Int64 uniqueId = m_match.AllocateUniqueId();

					newPlayerEntity->AddComponent<MatchComponent>(m_match, layerIndex, uniqueId);

					m_match.RegisterEntity(uniqueId, newPlayerEntity);

					UpdateControlledEntity(newPlayerEntity, true, true);

					for (auto& weaponEntity : m_weapons)
					{
						Nz::Int64 weaponUniqueId = m_match.AllocateUniqueId();

						weaponEntity = world.CloneEntity(weaponEntity);
						weaponEntity->AddComponent<MatchComponent>(m_match, layerIndex, weaponUniqueId);
						weaponEntity->GetComponent<Ndk::NodeComponent>().SetParent(newPlayerEntity);
						weaponEntity->GetComponent<NetworkSyncComponent>().UpdateParent(newPlayerEntity);
						weaponEntity->GetComponent<WeaponComponent>().UpdateOwner(newPlayerEntity);

						m_match.RegisterEntity(weaponUniqueId, weaponEntity);
					}

					m_shouldSendWeapons = true;
				}
			}
			else
			{
				m_playerEntity.Reset();

				m_activeWeaponIndex = NoWeapon;
				m_weapons.clear();
				m_weaponByName.clear();
			}

			m_layerIndex = layerIndex;

			MatchClientVisibility& visibility = GetSession().GetVisibility();
			visibility.PushLayerUpdate(m_localIndex, m_layerIndex);

			if (m_layerIndex != NoLayer)
				UpdateLayerVisibility(m_layerIndex, true);
		}
	}

	void Player::PrintChatMessage(std::string message)
	{
		Packets::ChatMessage chatPacket;
		chatPacket.content = std::move(message);
		chatPacket.localIndex = m_localIndex;

		SendPacket(chatPacket);
	}
	
	void Player::OnTick(bool lastTick)
	{
		if (lastTick && m_shouldSendWeapons)
		{
			Packets::PlayerWeapons weaponPacket;
			weaponPacket.localIndex = m_localIndex;
			weaponPacket.layerIndex = m_layerIndex;

			Nz::Bitset<Nz::UInt64> weaponIds;
			for (const Ndk::EntityHandle& weapon : m_weapons)
			{
				assert(weapon);

				weaponPacket.weaponEntities.emplace_back(Nz::UInt32(weapon->GetId()));
				weaponIds.UnboundedSet(weapon->GetId());
			}

			m_session.GetVisibility().PushEntitiesPacket(m_layerIndex, std::move(weaponIds), std::move(weaponPacket));

			m_shouldSendWeapons = false;
		}

		if (auto& inputOpt = m_inputBuffer[m_inputIndex])
		{
			UpdateInputs(inputOpt.value());
			inputOpt.reset();
		}

		if (++m_inputIndex >= m_inputBuffer.size())
			m_inputIndex = 0;
	}

	void Player::RemoveWeapon(const std::string& weaponClass)
	{
		if (m_layerIndex == NoLayer)
			return;

		if (!m_playerEntity)
			return;

		auto it = m_weaponByName.find(weaponClass);
		if (it == m_weaponByName.end())
			return;

		std::size_t droppedIndex = it->second;

		if (m_activeWeaponIndex == droppedIndex)
			SelectWeapon(NoWeapon);

		m_weaponByName.erase(it);
		m_weapons.erase(m_weapons.begin() + droppedIndex);

		// Shift indexes by one
		for (auto weaponIt = m_weaponByName.begin(); weaponIt != m_weaponByName.end(); ++weaponIt)
		{
			std::size_t& weaponIndex = weaponIt.value();
			if (weaponIndex > droppedIndex)
				weaponIndex--;
		}

		m_shouldSendWeapons = true;
	}

	void Player::SelectWeapon(std::size_t weaponIndex)
	{
		assert(weaponIndex == NoWeapon || weaponIndex < m_weapons.size());

		if (m_activeWeaponIndex == weaponIndex || !m_playerEntity)
			return;

		if (m_activeWeaponIndex != NoWeapon)
		{
			auto& weapon = m_weapons[m_activeWeaponIndex]->GetComponent<WeaponComponent>();
			weapon.SetActive(false);
		}

		m_activeWeaponIndex = weaponIndex;
		if (m_activeWeaponIndex != NoWeapon)
		{
			auto& weapon = m_weapons[m_activeWeaponIndex]->GetComponent<WeaponComponent>();
			weapon.SetActive(true);
		}

		Packets::EntityWeapon weaponPacket;
		weaponPacket.layerIndex = m_layerIndex;
		weaponPacket.entityId = m_playerEntity->GetId();
		weaponPacket.weaponEntityId = (m_activeWeaponIndex != NoWeapon) ? m_weapons[m_activeWeaponIndex]->GetId() : Packets::EntityWeapon::NoWeapon;

		Nz::Bitset<Nz::UInt64> entityIds;
		entityIds.UnboundedSet(weaponPacket.entityId);

		if (weaponPacket.weaponEntityId != 0xFFFFFFFF)
			entityIds.UnboundedSet(weaponPacket.weaponEntityId);

		m_match.ForEachPlayer([&](Player* ply)
		{
			MatchClientSession& session = ply->GetSession();
			session.GetVisibility().PushEntitiesPacket(m_layerIndex, entityIds, weaponPacket);
		});
	}

	void Player::SetAdmin(bool isAdmin)
	{
		m_isAdmin = isAdmin;
	}
	
	std::string Player::ToString() const
	{
		return "Player(" + m_name + ")";
	}

	void Player::UpdateControlledEntity(const Ndk::EntityHandle& entity, bool sendPacket, bool ignoreLayerUpdate)
	{
		MatchClientVisibility& visibility = m_session.GetVisibility();

		if (m_playerEntity)
		{
			m_playerEntity->RemoveComponent<OwnerComponent>();
			m_playerEntity->RemoveComponent<PlayerControlledComponent>();

			auto& matchComponent = m_playerEntity->GetComponent<MatchComponent>();
			visibility.SetEntityControlledStatus(matchComponent.GetLayerIndex(), m_playerEntity->GetId(), false);
		}

		m_playerEntity = Ndk::EntityHandle::InvalidHandle;
		if (entity)
		{
			auto& matchComponent = entity->GetComponent<MatchComponent>();
			if (!ignoreLayerUpdate)
				MoveToLayer(matchComponent.GetLayerIndex());

			m_playerEntity = entity; //< FIXME (deferred because of MoveToLayer)

			m_playerEntity->AddComponent<OwnerComponent>(CreateHandle());
			m_playerEntity->AddComponent<PlayerControlledComponent>(CreateHandle());

			if (m_playerEntity->HasComponent<HealthComponent>())
			{
				auto& healthComponent = m_playerEntity->GetComponent<HealthComponent>();

				m_onPlayerEntityDied.Connect(healthComponent.OnDied, [this](const HealthComponent* /*health*/, const Ndk::EntityHandle& attacker)
				{
					OnDeath(attacker);
				});
			}

			m_onPlayerEntityDestruction.Connect(m_playerEntity->OnEntityDestruction, [this](Ndk::Entity* /*entity*/)
			{
				OnDeath(Ndk::EntityHandle::InvalidHandle);
			});

			visibility.SetEntityControlledStatus(matchComponent.GetLayerIndex(), m_playerEntity->GetId(), true);
		}
		else
		{
			m_onPlayerEntityDied.Disconnect();
			m_onPlayerEntityDestruction.Disconnect();
		}

		if (sendPacket)
		{
			Packets::ControlEntity controlEntity;
			controlEntity.localIndex = m_localIndex;
			if (m_playerEntity)
			{
				auto& matchComponent = m_playerEntity->GetComponent<MatchComponent>();

				controlEntity.layerIndex = matchComponent.GetLayerIndex();
				controlEntity.entityId = static_cast<Nz::UInt32>(entity->GetId());

				visibility.PushEntityPacket(matchComponent.GetLayerIndex(), controlEntity.entityId, controlEntity);
			}
			else
			{
				controlEntity.layerIndex = NoLayer;
				controlEntity.entityId = 0;

				SendPacket(controlEntity);
			}
		}
	}

	void Player::UpdateInputs(const PlayerInputData& inputData)
	{
		if (m_playerEntity && m_playerEntity->HasComponent<InputComponent>())
		{
			auto& inputComponent = m_playerEntity->GetComponent<InputComponent>();
			inputComponent.UpdateInputs(inputData);
		}
	}

	void Player::UpdateInputs(std::size_t tickDelay, PlayerInputData inputData)
	{
		assert(tickDelay < m_inputBuffer.size());
		std::size_t index = (m_inputIndex + tickDelay) % m_inputBuffer.size();

		m_inputBuffer[index] = std::move(inputData);
	}

	void Player::UpdateLayerVisibility(LayerIndex layerIndex, bool isVisible)
	{
		MatchClientVisibility& visibility = GetSession().GetVisibility();

		if (isVisible)
			visibility.ShowLayer(layerIndex);
		else
			visibility.HideLayer(layerIndex);

		m_visibleLayers.UnboundedSet(layerIndex, isVisible);
	}

	void Player::UpdateName(std::string newName)
	{
		m_match.GetGamemode()->ExecuteCallback("OnPlayerNameUpdate", CreateHandle(), newName);
		m_name = std::move(newName);

		Packets::PlayerNameUpdate nameUpdatePacket;
		nameUpdatePacket.newName = m_name;
		nameUpdatePacket.playerIndex = Nz::UInt16(m_playerIndex);

		m_match.BroadcastPacket(nameUpdatePacket);
	}

	void Player::OnDeath(const Ndk::EntityHandle& attacker)
	{
		assert(m_playerEntity);

		UpdateControlledEntity(Ndk::EntityHandle::InvalidHandle, false);

		Packets::ChatMessage chatPacket;
		if (attacker && attacker->HasComponent<OwnerComponent>())
		{
			auto& ownerComponent = attacker->GetComponent<OwnerComponent>();
			Player* playerKiller = ownerComponent.GetOwner();
			if (playerKiller != this)
				chatPacket.content = playerKiller->GetName() + " killed " + GetName();
			else
				chatPacket.content = GetName() + " suicided";
		}
		else
			chatPacket.content = GetName() + " suicided";

		m_match.ForEachPlayer([&](Player* otherPlayer)
		{
			otherPlayer->SendPacket(chatPacket);
		});

		if (attacker && attacker->HasComponent<ScriptComponent>())
		{
			auto& attackerScript = attacker->GetComponent<ScriptComponent>();
			m_match.GetGamemode()->ExecuteCallback("OnPlayerDeath", CreateHandle(), attackerScript.GetTable());
		}
		else
			m_match.GetGamemode()->ExecuteCallback("OnPlayerDeath", CreateHandle(), sol::nil);

		m_weapons.clear();
		m_weaponByName.clear();
		m_activeWeaponIndex = NoWeapon;
	}

	void Player::SetReady()
	{
		assert(!m_isReady);
		m_isReady = true;
	}
}
