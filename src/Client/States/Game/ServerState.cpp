// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Client/States/Game/ServerState.hpp>
#include <CoreLib/NetworkSessionManager.hpp>
#include <ClientLib/LocalSessionManager.hpp>
#include <Client/ClientApp.hpp>
#include <Client/States/Game/ConnectionState.hpp>

namespace bw
{
	ServerState::ServerState(std::shared_ptr<StateData> stateDataPtr, Nz::UInt16 listenPort) :
	AbstractState(std::move(stateDataPtr))
	{
		ClientApp& app = *GetStateData().app;
		const ConfigFile& config = app.GetConfig();

		Map map = Map::LoadFromBinary(config.GetStringValue("GameSettings.MapFile"));
		float tickRate = config.GetFloatValue<float>("GameSettings.TickRate");

		m_match.emplace(app, "local", "gamemodes/test", std::move(map), 64, 1.f / tickRate);

		MatchSessions& sessions = m_match->GetSessions();
		m_localSessionManager = sessions.CreateSessionManager<LocalSessionManager>();
		if (listenPort != 0)
			m_networkSessionManager = sessions.CreateSessionManager<NetworkSessionManager>(listenPort, 64);
		else
			m_networkSessionManager = nullptr;

		if (config.GetBoolValue("Debug.SendServerState"))
			m_match->InitDebugGhosts();
	}

	void ServerState::Enter(Ndk::StateMachine& fsm)
	{
		fsm.PushState(std::make_shared<ConnectionState>(GetStateDataPtr(), m_localSessionManager));
		/*Nz::IpAddress serverAddress = Nz::IpAddress::LoopbackIpV4;
		serverAddress.SetPort(14768);

		fsm.PushState(std::make_shared<ConnectionState>(GetStateDataPtr(), serverAddress));*/
	}

	bool ServerState::Update(Ndk::StateMachine& fsm, float elapsedTime)
	{
		if (!AbstractState::Update(fsm, elapsedTime))
			return false;

		m_match->Update(elapsedTime);
		return true;
	}
}
