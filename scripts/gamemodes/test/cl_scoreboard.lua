function GM:OnInitScoreboard(scoreboard)
	self.Scoreboard = scoreboard
	self.ScoreboardColumns = {}

	local columns = { "Name", "Kill", "Death", "Ping" }

	for _, columnName in pairs(columns) do
		self.ScoreboardColumns[columnName] = scoreboard:AddColumn(columnName)
	end

	for _, player in pairs(match.GetPlayers()) do
		self:RegisterScoreboardPlayer(player)
	end
end

function GM:OnPlayerJoined(player)
	local scoreboard = self.Scoreboard
	if (not scoreboard) then
		return
	end

	self:RegisterScoreboardPlayer(player)
end

function GM:RegisterScoreboardPlayer(player)
	local scoreboard = assert(self.Scoreboard)

	local playerIndex = player:GetPlayerIndex()
	local isLocalPlayer = false
	local localPlayerCount = engine_GetLocalPlayerCount()
	for i = 1, localPlayerCount do
		if (engine_GetLocalPlayer_PlayerIndex(i - 1) == playerIndex) then
			isLocalPlayer = true
			break
		end
	end

	scoreboard:RegisterPlayer(playerIndex, 0, {
		player:GetName(),
		tostring(self:GetPlayerKills(player)),
		tostring(self:GetPlayerDeaths(player)),
		tostring(player:GetPing() or ""),
	}, isLocalPlayer)
end

function GM:OnPlayerLeave(player)
	local scoreboard = self.Scoreboard
	if (not scoreboard) then
		return
	end

	scoreboard:UnregisterPlayer(player:GetPlayerIndex())
end

function GM:OnPlayerNameUpdate(player, newName)
	print(player:GetName() .. " has changed name to " .. newName)

	local scoreboard = self.Scoreboard
	if (not scoreboard) then
		return
	end

	for _, player in pairs(match.GetPlayers()) do
		scoreboard:UpdatePlayerValue(player:GetPlayerIndex(), self.ScoreboardColumns.Name, newName)
	end
end

function GM:OnPlayerPingUpdate()
	local scoreboard = self.Scoreboard
	if (not scoreboard) then
		return
	end

	for _, player in pairs(match.GetPlayers()) do
		scoreboard:UpdatePlayerValue(player:GetPlayerIndex(), self.ScoreboardColumns.Ping, tostring(player:GetPing() or ""))
	end
end

function GM:UpdateScoreboard(deaths, kills)
	local scoreboard = self.Scoreboard
	if (not scoreboard) then
		return
	end

	for playerIndex, death in pairs(deaths) do
		scoreboard:UpdatePlayerValue(playerIndex, self.ScoreboardColumns.Death, tostring(death))
	end

	for playerIndex, kill in pairs(kills) do
		scoreboard:UpdatePlayerValue(playerIndex, self.ScoreboardColumns.Kill, tostring(kill))
	end
end
