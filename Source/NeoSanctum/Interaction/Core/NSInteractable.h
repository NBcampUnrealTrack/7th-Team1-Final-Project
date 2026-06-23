// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NSInteractable.generated.h"

class UTexture2D;

UINTERFACE()
class UNSInteractable : public UInterface
{
	GENERATED_BODY()
};

class NEOSANCTUM_API INSInteractable
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, Category="Interaction")
	bool CanInteract(APlayerController* Interactor) const;
	
	UFUNCTION(BlueprintNativeEvent, Category="Interaction")
	bool OnInteract(APlayerController* Interactor);
	
	UFUNCTION(BlueprintNativeEvent, Category="Interaction")
	FText GetPromptText() const;

	// 프롬프트에 표시할 아이콘 (없으면 빈 값)
	UFUNCTION(BlueprintNativeEvent, Category="Interaction")
	TSoftObjectPtr<UTexture2D> GetPromptIcon() const;

	// 프롬프트 배경 색상에 쓸 등급 (-1 = 기본값)
	UFUNCTION(BlueprintNativeEvent, Category="Interaction")
	int32 GetPromptRarityIndex() const;

	// 프롬프트 위젯이 떠야 할 월드 위치 (각 대상이 자기 앵커 위치를 반환)
	UFUNCTION(BlueprintNativeEvent, Category="Interaction")
	FVector GetPromptWorldLocation() const;
};

