// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NeoSanctum/Type/NSPlayerStatusMessageTypes.h"
#include "NSTeammateStatusEntryWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UWidget;
class UImage;

/**
 * GMS를 통해 전달받은 팀원 한명의 이름, 체력, 쉴드를 표시
 */
UCLASS()
class NEOSANCTUM_API UNSTeammateStatusEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	//전달받은 ViewData를 화면에 적용
	UFUNCTION(BlueprintCallable, Category = "UI|Teammate Status")
	void ApplyStatusData(const FNSPlayerStatusViewData& StatusData);
	
	//현재 Entry가 표시하는 Player 식별자
	int32 GetPlayerId() const
	{
		return PlayerId																																																																																																																																																																																																													;
	}
	
private:
	//최대 값이 0 안굥유룰 벙자헌 ProgressBar비율 계산
	float GetSafePercent(float CurrentValue, float MaxValue)const;
	
private:
	//플레이어 이름
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;
	//현재 체력/최대체력
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HealthValueText;
	//현재 쉴드/ 최대 쉴드
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ShieldValueText;
	//현재 체력 비율
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;
	//현재 쉴드 비율
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ShieldBar;
	//팀원의 캐릭터 초상화
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> PortraitImage;
	//현재 표시중인 팀원의 세션 식별자
	int32 PlayerId = INDEX_NONE;
};
