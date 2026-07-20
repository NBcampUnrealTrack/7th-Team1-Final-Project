// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSVoterEntry.generated.h"

class UImage;
class UCommonTextBlock;


UCLASS()
class NEOSANCTUM_API UNSVoterEntry : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	// 투표자 있음: 이름 + 채워진 이미지
	void SetVoter(const FString& PlayerName);
	// 빈 슬롯: 이름 비우고 기본 이미지로 복귀
	void ClearVoter();

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> VoterImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> VoterNameText;

	// 투표된 슬롯에 쓸 이미지
	UPROPERTY(EditDefaultsOnly, Category = "Vote")
	TObjectPtr<UObject> FilledSprite;

	// 빈 슬롯(기본) 이미지 = 지금 슬롯에 이미 들어있는 그 이미지
	UPROPERTY(EditDefaultsOnly, Category = "Vote")
	TObjectPtr<UObject> EmptySprite;
};
