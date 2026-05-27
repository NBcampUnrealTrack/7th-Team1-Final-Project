// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "NSPlayerState.generated.h"

class UNSAbilitySystemComponent;
class UNSPlayerAttributeSet;
class UNSPlayerProgressComponent;
class UNSPermanentSaveGame;

UCLASS()
class NEOSANCTUM_API ANSPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ANSPlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UNSPlayerAttributeSet* GetPlayerAttributeSet() const;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	bool IsReady() const { return bIsReady; }
	bool IsDead() const { return bIsDead; }
	// 사망 상태를 변경하기 위한 Setter
	void SetIsDead(bool bNewIsDead);
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_SetReady();
	
	UNSPlayerProgressComponent* GetProgressComponent() const { return ProgressComponent; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UNSAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UNSPlayerAttributeSet> PlayerAttributeSet;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
	TObjectPtr<UNSPlayerProgressComponent> ProgressComponent;
	
private:
	void OnSaveDataLoaded(UNSPermanentSaveGame* Data);

	UPROPERTY(Replicated)
	bool bIsReady;

	// 사망 상태 변수
	UPROPERTY(Replicated)
	bool bIsDead;
};
