RegisterClientScript()

ENTITY.IsNetworked = true

ENTITY.Properties = {
	{ Name = "source_entity", Type = PropertyType.Entity, Shared = true },
	{ Name = "source_offset", Type = PropertyType.FloatPosition, Shared = true },
	{ Name = "target_entity", Type = PropertyType.Entity, Shared = true },
	{ Name = "target_offset", Type = PropertyType.FloatPosition, Shared = true },
}

function ENTITY:Initialize()
	local sourceEntity = self:GetProperty("source_entity")
	local sourceOffset = self:GetProperty("source_offset")
	local targetEntity = self:GetProperty("target_entity")
	local targetOffset = self:GetProperty("target_offset")
	if (sourceEntity == NoEntity or not sourceEntity:IsValid()) then
		self:Remove()
		return
	end

	self.Constraint = physics.CreateDampenedSpringConstraint(sourceEntity, targetEntity, sourceOffset, targetOffset, 10, 500, 0)
end

function ENTITY:OnKilled()
	if (self.Constraint) then
		self.Constraint:Remove()
	end
end
