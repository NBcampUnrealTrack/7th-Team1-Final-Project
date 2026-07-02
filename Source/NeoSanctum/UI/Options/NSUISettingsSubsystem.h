// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NSUISettingsSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnCrosshairColorChanged,
	const FLinearColor&);

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSUISettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	UFUNCTION(BlueprintPure, Category = "UI|Settings")
	FLinearColor GetCrosshairColor() const;
	
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void SetCrosshairColor(FLinearColor NewColor);
	
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void ResetCrosshairColor();
	
	FOnCrosshairColorChanged OnCrosshairColorChanged;
	
private:
	void LoadSettings();
	void SaveSettings() const;
	
private:
	FLinearColor CrosshairColor = FLinearColor::White;
};
