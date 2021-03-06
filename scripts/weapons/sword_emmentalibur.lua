RegisterClientScript()

WEAPON.Cooldown = 0.5
WEAPON.Scale = 0.6
WEAPON.Sprite = "emmentalibur.png"
WEAPON.SpriteOrigin = Vec2(40, 284) * WEAPON.Scale
WEAPON.WeaponOffset = Vec2(20, -60) -- This should not be here
WEAPON.Animations = {
	{"attack", 0.3}
}

RegisterClientAssets(WEAPON.Sprite)

if (SERVER) then
	function WEAPON:OnAttack()
		local pos = self:GetPosition()
		local maxs = Vec2(128, 66)
		local mins = Vec2(28, -76)

		if (not self:IsLookingRight()) then
			maxs = maxs * -1
			mins = mins * -1
		end

		self:PlayAnim("attack")
		self:DealDamage(pos, 100, Rect(pos + mins, pos + maxs), 50000)
	end
end

if (CLIENT) then
	function WEAPON:OnAnimationStart(animationId)
		local startRotation = self:GetRotation()
		local endRotation
		if (self:IsLookingRight()) then
			endRotation = startRotation + 180
		else
			endRotation = startRotation - 180
		end

		animation.Rotate(self, startRotation, endRotation, 0.1)
		animation.Rotate(self, endRotation, startRotation, 0.2)
	end
end
