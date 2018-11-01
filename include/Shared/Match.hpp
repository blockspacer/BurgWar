// Copyright (C) 2018 Jérôme Leclercq
// This file is part of the "Burgwar Client" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef BURGWAR_SHARED_MATCH_HPP
#define BURGWAR_SHARED_MATCH_HPP

#include <Nazara/Core/ObjectHandle.hpp>
#include <Shared/MatchSessions.hpp>
#include <memory>
#include <string>
#include <vector>

namespace bw
{
	class Player;
	class Terrain;

	using PlayerHandle = Nz::ObjectHandle<Player>;

	class Match
	{
		public:
			Match(std::string matchName, std::size_t maxPlayerCount);
			Match(const Match&) = delete;
			~Match();

			void Leave(Player* player);

			inline MatchSessions& GetSessions();
			inline const MatchSessions& GetSessions() const;
			inline Terrain& GetTerrain();
			inline const Terrain& GetTerrain() const;

			bool Join(Player* player);

			void Update(float elapsedTime);

			Match& operator=(const Match&) = delete;

		private:
			std::size_t m_maxPlayerCount;
			std::string m_name;
			std::unique_ptr<Terrain> m_terrain;
			std::vector<PlayerHandle> m_players;
			MatchSessions m_sessions;
	};
}

#include <Shared/Match.inl>

#endif
