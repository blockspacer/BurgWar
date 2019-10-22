// Copyright (C) 2019 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CoreLib/MatchClientVisibility.hpp>

namespace bw
{
	namespace Detail
	{
		template<typename T> 
		struct HasStateTick 
		{
			using UnrefT = std::decay_t<T>;
			struct Fallback { Nz::UInt16 stateTick; };
			struct Derived : UnrefT, Fallback { };

			template<typename C, C> struct ChT;

			template<typename C> static char(&f(ChT<Nz::UInt16 Fallback::*, &C::stateTick>*))[1];
			template<typename C> static char(&f(...))[2];

			static bool const value = sizeof(f<Derived>(0)) == 2;
		};
	}

	inline MatchClientVisibility::MatchClientVisibility(Match& match, MatchClientSession& session) :
	m_match(match),
	m_session(session)
	{
	}
	
	inline bool MatchClientVisibility::IsLayerVisible(std::size_t layerIndex) const
	{
		return m_visibleLayers.UnboundedTest(layerIndex);
	}

	template<typename T>
	void MatchClientVisibility::PushEntityPacket(Nz::UInt16 layerIndex, Nz::UInt32 entityId, T&& packet)
	{
		Nz::UInt64 entityKey = BuildEntityId(layerIndex, entityId);
		auto it = m_pendingEntitiesEvent.find(entityKey);
		if (it == m_pendingEntitiesEvent.end())
			it = m_pendingEntitiesEvent.emplace(entityKey, std::vector<EntityPacketSendFunction>()).first;

		if constexpr (Detail::HasStateTick<T>::value)
		{
			it.value().emplace_back([this, packet = std::forward<T>(packet)]() mutable
			{
				packet.stateTick = m_match.GetNetworkTick();

				m_session.SendPacket(packet);
			});
		}
		else
		{
			it.value().emplace_back([this, packet = std::forward<T>(packet)]() mutable
			{
				m_session.SendPacket(packet);
			});
		}
	}

	template<typename T>
	void MatchClientVisibility::PushEntitiesPacket(Nz::UInt16 layerIndex, Nz::Bitset<Nz::UInt64> entitiesId, T&& packet)
	{
		if constexpr (Detail::HasStateTick<T>::value)
		{
			m_multiplePendingEntitiesEvent.emplace_back(PendingMultipleEntities{
				layerIndex,
				std::move(entitiesId),
				[this, packet = std::forward<T>(packet)]() mutable
				{
					packet.stateTick = m_match.GetNetworkTick();

					m_session.SendPacket(packet);
				}
			});
		}
		else
		{
			m_multiplePendingEntitiesEvent.emplace_back(PendingMultipleEntities{
				layerIndex,
				std::move(entitiesId),
				[this, packet = std::forward<T>(packet)]() mutable
				{
					m_session.SendPacket(packet);
				}
			});
		}
	}

	Nz::UInt64 MatchClientVisibility::BuildEntityId(Nz::UInt16 layerIndex, Nz::UInt32 entityId)
	{
		return Nz::UInt64(layerIndex) << 32 | entityId;
	}

	inline Packets::Helper::EntityId bw::MatchClientVisibility::DecodeEntityId(Nz::UInt64 entityId)
	{
		return { CompressedUnsigned<Nz::UInt16>(entityId >> 32), CompressedUnsigned<Nz::UInt32>(entityId & 0xFFFFFFFF) };
	}
}