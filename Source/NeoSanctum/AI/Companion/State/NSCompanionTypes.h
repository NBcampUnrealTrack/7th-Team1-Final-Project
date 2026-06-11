#pragma once

#include "CoreMinimal.h"
#include "NSCompanionTypes.generated.h"

UENUM(BlueprintType)
enum class ECompanionState : uint8
{
	Follow UMETA(DisplayName = "Follow"),
	Collect UMETA(DisplayName = "Collect"),
	Combat UMETA(DisplayName = "Combat")
};
