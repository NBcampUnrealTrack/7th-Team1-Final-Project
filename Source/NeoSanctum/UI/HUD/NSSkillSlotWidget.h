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
class UAbilitySystemComponent;
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
	//슬롯이 반응할 쿨타임 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FGameplayTagQuery CooldownTagQuery;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	bool bShowChargeText = false;
private:
	//GMS 쿨타임 시작 수신
	void HandleCooldownMessage(
		FGameplayTag Channel,
		const FNSSkillCooldownMessage& Message);
	//DataTable Row에서 아이콘과 SkillTag를 적용
	void ApplySkillUIData();
	
	//스킬 아이콘
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkillIcon;
	//원형 쿨타임
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CooldownOverlay;
	// 남은 쿨타임 텍스트
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CooldownText;
	//충전형 스킬의 남은 횟수 표시
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> ChargeText;
	//쿨타임과 머테리얼 연결
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CooldownMID;//(MID : MaterialInstanceDynamic)
	//우젯 제거시 GMS리스너 해제
	FGameplayMessageListenerHandle CooldownListenerHandle;
	//소유 플레이어의 ASC를 캐싱
	void CacheOwnerASC();

	//ASC에 적용된 쿨타임 GE를 조회해 UI를 갱신
	void UpdateCooldownFromASC();

	//조회한 남은 시간화면에 반영
	void UpdateCooldownDisplay(float NewRemainingCooldown, float NewCooldownDuration);
	
	//조회한 남은 충전횟수 화면에 반영
	void UpdateChargeDisplay(int32 CurrentCharge, int32 MaxCharge);
	
	//ASC의 AttributeSet에서 대쉬 충전수를 읽어온다
	void UpdateDashChargeFromASC();
	
	//PlayerState에서 가져온 ASC캐시
	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> CachedASC;
	
	//전체 쿨타임
	float CooldownDuration = 0.0f;
	//남은 쿨타임
	float RemainingCooldown = 0.0f;
	
	protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(
			const FGeometry& InGeometry,
			float InDeltaTime) override;
};
