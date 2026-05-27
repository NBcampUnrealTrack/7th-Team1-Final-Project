#pragma once

#include "CoreMinimal.h"
#include "NSTeamTypes.generated.h"

UENUM(BlueprintType)
enum class ETeamId : uint8
{
	Player = 0 UMETA(DisplayName = "Player"),
	Enemy = 1 UMETA(DisplayName = "Enemy"),
	Neutral = 2 UMETA(DisplayName = "Neutral")
};
