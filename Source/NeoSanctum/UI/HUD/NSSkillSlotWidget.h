// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "NSSkillSlotWidget.generated.h"

class UCommonTextBlock;
class UImage;
class UMaterialInstanceDynamic;
struct FNSSkillCooldownMessage;
struct FNSSkillUIData;

/**
 *  스킬의 쿨타임을 시각적으로 보여주는 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSSkillSlotWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	//쿨타임 UI표시 시작
	void StartCooldown(float NewCooldownDuration);
	//쿨타임 UI 촉화
	void ResetCooldown();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FGameplayTag BoundSkillTag;
	//슬롯에 적용할 UI데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category =  "Skill")
	FDataTableRowHandle SkillUIDataRow;
	//현재 충전된 스킬 횟수
	
private:
	//GMS 쿨타임 시작 수신
	void HandleCooldownMessage(
		FGameplayTag Channel,
		const FNSSkillCooldownMessage& Message);
	//DataTable Row에서 아이콘과 SkillTag를 적용
	void ApplySkillUIData();
	//충전횟수갱신
	void UpdateChargeText(
	int32 NewCurrentCharge,
	int32 NewMaxCharge);
	
	//스킬 아이콘
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkillIcon;
	//원형 쿨타임
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CooldownOverlay;
	// 남은 쿨타임 텍스트
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CooldownText;
	//현재 충전된 스킬 횟수
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> ChargeText;
	//쿨타임과 머테리얼 연결
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CooldownMID;//(MID : MaterialInstanceDynamic)
	//우젯 제거시 GMS리스너 해제
	FGameplayMessageListenerHandle CooldownListenerHandle;
	
	//전체 쿨타임
	float CooldownDuration = 0.0f;
	//남은 쿨타임
	float RemainingCooldown = 0.0f;
	
	int32 CurrentCharge = 0;
	int32 MaxCharge = 0;
	//UI에서 이어서 돌릴 대시 회복 쿨타임 개수
	int32 PendingCooldownCount = 0;
	
	protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(
			const FGeometry& InGeometry,
			float InDeltaTime) override;
};
