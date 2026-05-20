// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "NSPlayerState.generated.h"

class UNSAbilitySystemComponent;
class UNSPlayerAttributeSet;
/**
 * 
 */
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
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_SetReady();
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UNSAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UNSPlayerAttributeSet> PlayerAttributeSet;

private:
	UPROPERTY(Replicated)
	bool bIsReady;
};
