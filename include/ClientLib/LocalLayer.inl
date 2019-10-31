// Copyright (C) 2019 J�r�me Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/LocalLayer.hpp>

namespace bw
{
	inline const Ndk::EntityHandle& LocalLayer::GetCameraEntity()
	{
		return m_camera;
	}

	inline Nz::Node& LocalLayer::GetCameraNode()
	{
		return *m_cameraNode;
	}

	inline Nz::Node& LocalLayer::GetNode()
	{
		return *m_node;
	}
}