// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef BURGWAR_SERVER_SESSIONMANAGER_HPP
#define BURGWAR_SERVER_SESSIONMANAGER_HPP

#include <CoreLib/NetworkReactor.hpp>
#include <CoreLib/PlayerCommandStore.hpp>
#include <CoreLib/SessionBridge.hpp>
#include <CoreLib/SessionManager.hpp>
#include <Nazara/Core/MemoryPool.hpp>
#include <Thirdparty/tsl/hopscotch_map.h>
#include <vector>

namespace bw
{
	class Logger;
	class Match;
	class MatchClientSession;

	class MatchSessions
	{
		public:
			MatchSessions(Match& match);
			~MatchSessions();

			void Clear();

			MatchClientSession* CreateSession(std::shared_ptr<SessionBridge> bridge);
			template<typename T, typename... Args> T* CreateSessionManager(Args&&... args);
			void DeleteSession(MatchClientSession* session);

			template<typename F> void ForEachSession(F&& cb);

			inline Match& GetMatch();

			void Poll();

		private:
			std::size_t m_nextSessionId;
			std::vector<std::unique_ptr<SessionManager>> m_managers;
			Match& m_match;
			PlayerCommandStore m_commandStore;
			Nz::MemoryPool m_sessionPool;
			tsl::hopscotch_map<std::size_t /*sessionId*/, MatchClientSession* /*session*/> m_sessionIdToSession;
	};
}

#include <CoreLib/MatchSessions.inl>

#endif
