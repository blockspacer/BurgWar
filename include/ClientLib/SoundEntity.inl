// Copyright (C) 2019 J�r�me Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/SoundEntity.hpp>

namespace bw
{
	inline const Ndk::EntityHandle& SoundEntity::GetEntity() const
	{
		return m_entity;
	}
}
