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

	// 연속 피해 숫자가 같은 위치에 겹치지 않도록 다음 표시 위치를 기억.
	int32 NextDisplayOffsetIndex = 0;
};
