// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "NSSkillSlotWidget.generated.h"

struct FNSInputDisplayData;
class UCommonTextBlock;
class UImage;
class UMaterialInstanceDynamic;
class UNSAbilitySystemComponent;
class UTexture2D;
class UNSInputDisplayData;
struct FSkillCooldownUIData;
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

	//쿨타임 UI 초기화
	void ResetCooldown();

	//스킬 식별 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FGameplayTag BoundSkillTag;

	//ASC 쿨다운 데이터 조회에 사용할 스킬 슬롯 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FGameplayTag SkillSlotTag;

	//슬롯에 적용할 UI데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category =  "Skill")
	FDataTableRowHandle SkillUIDataRow;
	
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void SetInputKeyText(const FText& NewInputText);
	
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void SetInputKeyIcon(UTexture2D* NewInputIcon);
	
	//스킬 슬롯 입력 표시 갱신
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void SetInputDisplayData(const FNSInputDisplayData& NewInputDisplayData);
	
public:
	//캐릭터 변경 시 슬롯에 표시할 스킬 정보를 갱신
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void SetSkillUIData(FDataTableRowHandle NewSkillUIDataRow);
	//이 슬롯에서 입력키 표시를 사용할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bShowInputDisplay = true;
	
private:
	//GMS 쿨타임 변경 수신
	void HandleCooldownMessage(
		FGameplayTag Channel,
		const FNSSkillCooldownMessage& Message);

	//DataTable Row에서 아이콘과 SkillTag를 적용
	void ApplySkillUIData();
	
	//소유 플레이어의 ASC를 캐싱
	void CacheOwnerASC();

	//남은 시간과 전체 시간을 화면에 반영
	void UpdateCooldownDisplay(float NewRemainingCooldown, float NewCooldownDuration);
	
	//현재 충전 수와 최대 충전 수를 화면에 반영
	void UpdateChargeDisplay(int32 CurrentCharge, int32 MaxCharge);
	
	//ASC에서 스킬 쿨다운 UI 데이터를 조회
	void UpdateSkillCooldownFromASC();

	//조회한 스킬 쿨다운 데이터를 화면에 반영
	void ApplySkillCooldownUIData(const FSkillCooldownUIData& CooldownData);
	
	//슬롯 설정에 따라 입력 표시를 숨긴다
	void ApplyInputDisplayVisibility();
	
private:
	//스킬 아이콘
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkillIcon;

	//원형 쿨타임
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CooldownOverlay;

	//남은 쿨타임 텍스트
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CooldownText;

	//충전형 스킬의 남은 횟수 표시
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> ChargeText;

	//쿨타임과 머테리얼 연결
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CooldownMID;
	
	//입력키 텍스트 표시
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> InputKeyText;

	//입력키 아이콘 표시
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> InputKeyIcon;

	//위젯 제거시 GMS리스너 해제
	FGameplayMessageListenerHandle CooldownListenerHandle;

	//PlayerState에서 가져온 ASC캐시
	UPROPERTY(Transient)
	TObjectPtr<UNSAbilitySystemComponent> CachedASC;
	
	//전체 쿨타임
	float CooldownDuration = 0.0f;

	//남은 쿨타임
	float RemainingCooldown = 0.0f;
	
	//쿨타임 중일 때만 ASC 쿨타임 상태를 Tick에서 조회
	bool bCooldownTickActive = false;

	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(
		const FGeometry& InGeometry,
		float InDeltaTime) override;
};