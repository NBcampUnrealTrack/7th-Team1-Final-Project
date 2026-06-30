// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/AI/Base/NSBaseDroneAI.h"
#include "NeoSanctum/AI/Companion/State/NSCompanionTypes.h"
#include "NSCompanionDroneAI.generated.h"

UCLASS()
class NEOSANCTUM_API ANSCompanionDroneAI : public ANSBaseDroneAI
{
	GENERATED_BODY()

public:
	ANSCompanionDroneAI();

public:
	virtual void PossessedBy(AController* NewController) override;
	
	void SetCurrentState(ECompanionState NewState);
	
private:
	ECompanionState CurrentState = ECompanionState::Follow;
	
#pragma region Upgrades
	
public:
	void ApplyStatUpgrade(FGameplayTag NodeTag, int32 NewLevel);
	
protected:
	UPROPERTY()
	TMap<FGameplayTag, FActiveGameplayEffectHandle> StatUpgradeHandles;
	
#pragma endregion
	
#pragma region CachedData
	
public:
	void SetOwnerPlayer(AActor* Actor);
	AActor* GetOwnerPlayer() { return OwnerPlayer;}
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OwnerPlayer")
	TObjectPtr<AActor> OwnerPlayer;
	
#pragma endregion
	
#pragma region OwnerTeleport
	
public:
	UFUNCTION()
	void CheckDistanceToOwner();
	
	UFUNCTION()
	void TeleportToOwner();
	
protected:
	FTimerHandle CheckDistanceToOwnerTimer;
	
	UPROPERTY(EditDefaultsOnly, Category="DroneAI")
	float MaxDistance = 500.f;
	
private:
	UPROPERTY(EditAnywhere, Category="DroneAI|Leash")
	float HardLeashDistance = 4000.f;
	
	UPROPERTY(EditAnywhere, Category="DroneAI|Leash")
	float StuckRecoverTime = 2.0f;
	
	float TimeBeyondLeash = 0.f;
	float PrevDistSqToOwner = -1.f;
	
	static constexpr float CheckInterval = 0.25f;
	
#pragma endregion
	
#pragma region CurrencyVaccum
	
private:
	void VacuumNearbyCurrency();
	
private:
	UPROPERTY(EditAnywhere, Category="DroneAI|Currency", meta =(AllowPrivateAccess=true))
	float CurrencyVaccumRadius = 250.f;
	
	FTimerHandle CurrencyVacuumTimer;
	
#pragma endregion
};
