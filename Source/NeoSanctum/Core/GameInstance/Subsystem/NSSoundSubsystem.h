// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NSSoundSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSSoundSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	// SoundManager static getter
	static UNSSoundSubsystem * Get(const UObject* WorldContext);
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	UPROPERTY()
	TObjectPtr<UDataTable> SoundDataTable;
};
