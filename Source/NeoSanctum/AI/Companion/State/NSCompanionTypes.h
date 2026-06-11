#pragma once

UENUM(BlueprintType)
enum class EDroneState : uint8
{
	Follow UMETA(DisplayName = "Follow"),
	Collect UMETA(DisplayName = "Collect"),
	Combat UMETA(DisplayName = "Combat")
};
