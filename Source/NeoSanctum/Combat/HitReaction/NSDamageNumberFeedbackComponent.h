// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NSDamageNumberFeedbackComponent.generated.h"


class ANSDamageNumberActor;
struct FNSDamageNumberFeedbackMessage;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSDamageNumberFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSDamageNumberFeedbackComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleDamageNumberMessage(FGameplayTag ChannelTag, const FNSDamageNumberFeedbackMessage& Message);

	bool ShouldPlayLocalFeedback() const;

	UPROPERTY(EditDefaultsOnly, Category = "HitFeedback")
	TSubclassOf<ANSDamageNumberActor> DamageNumberActorClass;

	FGameplayMessageListenerHandle DamageNumberListenerHandle;
};
