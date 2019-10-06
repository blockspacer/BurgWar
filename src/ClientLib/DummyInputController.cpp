// Copyright (C) 2019 J�r�me Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/DummyInputController.hpp>

namespace bw
{
	PlayerInputData& DummyInputController::GetInputs()
	{
		return m_inputs;
	}
	
	PlayerInputData DummyInputController::Poll(LocalMatch& /*localMatch*/, const Ndk::EntityHandle& /*controlledEntity*/)
	{
		return m_inputs;
	}
}
