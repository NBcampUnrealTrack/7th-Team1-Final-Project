// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Type/NSCosmeticEventTypes.h"
#include "UObject/Object.h"
#include "NSCosmeticEventHandler.generated.h"

class UNSEnemyCosmeticComponent;

UCLASS(Abstract, Blueprintable, BlueprintType)
class NEOSANCTUM_API UNSCosmeticEventHandler : public UObject
{
	GENERATED_BODY()

public:
	// Handler가 소유 컴포넌트를 저장하는 함수
	virtual void Initialize(UNSEnemyCosmeticComponent* InOwnerComponent);

	// Handler가 처리할 EventTag 목록을 반환하는 함수
	virtual void GetHandledEventTags(TArray<FGameplayTag>& OutEventTags) const;

	// 클라이언트에서 코스메틱 이벤트를 처리하는 함수
	virtual void HandleEvent(AActor* OwnerActor, const FNSCosmeticEventNetData& EventData);

	// Handler가 World를 반환하는 함수
	virtual UWorld* GetWorld() const override;

protected:
	// Handler를 소유한 코스메틱 컴포넌트를 저장하는 변수
	UPROPERTY(Transient)
	TObjectPtr<UNSEnemyCosmeticComponent> OwnerComponent;
};
