// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Burgwar" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CoreLib/Scripting/SharedGamemode.hpp>
#include <CoreLib/SharedMatch.hpp>
#include <CoreLib/LogSystem/Logger.hpp>
#include <cassert>

namespace bw
{
	template<typename... Args>
	std::optional<sol::object> SharedGamemode::ExecuteCallback(const std::string& callbackName, Args&&... args)
	{
		sol::protected_function callback = m_gamemodeTable[callbackName];
		if (callback)
		{
			auto co = m_context->CreateCoroutine(callback);

			auto result = co(m_gamemodeTable, std::forward<Args>(args)...);
			if (!result.valid())
			{
				sol::error err = result;
				bwLog(m_sharedMatch.GetLogger(), LogLevel::Error, "Gamemode callback {} failed: {}", callbackName, err.what());
				return std::nullopt;
			}
			
			return result;
		}
		else
			return sol::nil;
	}

	inline sol::table& SharedGamemode::GetTable()
	{
		return m_gamemodeTable;
	}

	inline const std::filesystem::path& SharedGamemode::GetGamemodePath() const
	{
		return m_gamemodePath;
	}

	inline sol::table& SharedGamemode::GetGamemodeTable()
	{
		return m_gamemodeTable;
	}

	inline const std::shared_ptr<ScriptingContext>& SharedGamemode::GetScriptingContext() const
	{
		return m_context;
	}
}
