// Copyright (C) 2019 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Scripting/ClientEntityStore.hpp>
#include <cassert>

namespace bw
{
	inline ClientEntityStore::ClientEntityStore(std::string resourceFolder, std::shared_ptr<ScriptingContext> context) :
	SharedEntityStore(std::move(context), false),
	m_resourceFolder(std::move(resourceFolder))
	{
	}
}
