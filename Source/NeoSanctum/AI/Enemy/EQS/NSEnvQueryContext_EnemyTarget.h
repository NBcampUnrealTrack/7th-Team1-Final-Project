// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "NSEnvQueryContext_EnemyTarget.generated.h"

UCLASS()
class NEOSANCTUM_API UNSEnvQueryContext_EnemyTarget : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	virtual void ProvideContext(
		FEnvQueryInstance& QueryInstance,
		FEnvQueryContextData& ContextData) const override;
};
