// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSSpectatorWidget.generated.h"

class UCommonTextBlock;

/**
 * 관전자 상태에서 관전 대상 정보를 표시하는 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSSpectatorWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	//관전 대상 플레이어 이름 표시
	UFUNCTION(Blueprintable, Category = "UI|Spectator")
	void SetSpectatingPlayerName(const FString& PlayerName);
	
private:
	//관전 대상 텍스트
	UPROPERTY(meta =(BindWidget))
	TObjectPtr<UCommonTextBlock> SpectatingNameText;
};
