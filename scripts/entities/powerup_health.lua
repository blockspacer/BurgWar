RegisterClientScript("powerup_health.lua")

ENTITY.IsNetworked = true
ENTITY.CollisionType = 3
ENTITY.PlayerControlled = false
ENTITY.MaxHealth = 0

ENTITY.Properties = {}

function ENTITY:Initialize()
	local colliderSize = Vec2(400, 200) / 4 / 2
	self:SetCollider(Rect(-colliderSize, colliderSize))

	if (CLIENT) then
		self:AddSprite("placeholder/healthpack.png", Vec2(0.25, 0.25))
	end
end

function ENTITY:OnCollisionStart(other)
	if (SERVER and other.Name == "burger") then
		if (not other:IsFullHealth()) then
			other:Heal(250)
			self:Kill()
			self.Parent:OnPowerupConsumed()
		end
	end

	return false
end

