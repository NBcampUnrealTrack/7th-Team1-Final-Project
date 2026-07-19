// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CommonActivatableWidget.h"
#include "NeoSanctum/Data/UI/NSCharacterSelectData.h"
#include "NSCharacterSelectSkillSlotWidget.h"
#include "NSCharacterSelectWidget.generated.h"

class UNSCharacterSelectSkillStatRowWidget;
class UVerticalBox;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnCharacterSelectionConfirmed, UNSCharacterData*, ConfirmedCharacterData);

class UCommonAnimatedSwitcher;
class UCommonButtonBase;
class UCommonTextBlock;
class UNSCharacterSlotWidget;
class UImage;
class UTexture2D;
struct FNSSkillUIData;

/**
 * 스킬 정보를 어떤 입력으로 변경할지 정함.
 */
UENUM(BlueprintType)
enum class ENSCharacterSelectSkillPreviewMode : uint8
{
	Hover UMETA(DisplayName = "호버"),
	Click UMETA(DisplayName = "클릭")
};

/**
 * 거점에서 플레이어 캐릭터를 선택하는 UI 위젯.
 *
 * 캐릭터 목록은 위젯이 DataTable을 직접 들고 있지 않고,
 * NSDataSubsystem의 OutGame 캐시에서 받아 사용.
 */
UCLASS()
class NEOSANCTUM_API UNSCharacterSelectWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UCommonButtonBase> NextButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UCommonButtonBase> PrevButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UCommonButtonBase> ConfirmButton;

	// WBP에 닫기 버튼이 없어도 ESC 닫기는 동작하도록 Optional로 연결함.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UCommonButtonBase> CloseButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UCommonTextBlock> CharacterNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
	TObjectPtr<UCommonAnimatedSwitcher> CharacterSwitcher;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UImage> PreviewImage;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Stats")
	TObjectPtr<UCommonTextBlock> MaxHealthText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI")
	TObjectPtr<UCommonTextBlock> CharacterDescriptionText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Skill")
	TObjectPtr<UNSCharacterSelectSkillSlotWidget> BaseAttackSlot;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Skill")
	TObjectPtr<UNSCharacterSelectSkillSlotWidget> Skill1Slot;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Skill")
	TObjectPtr<UNSCharacterSelectSkillSlotWidget> Skill2Slot;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Skill")
	TObjectPtr<UNSCharacterSelectSkillSlotWidget> Skill3Slot;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Skill")
	TObjectPtr<UImage> SkillDetailIconImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Skill")
	TObjectPtr<UImage> SkillDetailInputIconImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Skill")
	TObjectPtr<UCommonTextBlock> SkillDetailNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Skill")
	TObjectPtr<UCommonTextBlock> SkillDetailDescriptionText;

	// 동적으로 생성한 스킬 스탯 행들이 들어갈 컨테이너.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Skill")
	TObjectPtr<UVerticalBox> SkillDetailStatsBox;

	// WBP의 Class Defaults에서 행 위젯 클래스를 지정할 수 있게 함.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterSelect|Skill")
	TSubclassOf<UNSCharacterSelectSkillStatRowWidget> SkillStatRowWidgetClass;

	// 기본값은 호버이며 나중에 WBP 기본값에서 클릭 방식으로 바꿀 수 있음.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterSelect|Skill")
	ENSCharacterSelectSkillPreviewMode SkillPreviewMode = ENSCharacterSelectSkillPreviewMode::Hover;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Stats")
	TObjectPtr<UCommonTextBlock> BaseDamageText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Stats")
	TObjectPtr<UCommonTextBlock> DefenseText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Stats")
	TObjectPtr<UCommonTextBlock> MoveSpeedText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Stats")
	TObjectPtr<UCommonTextBlock> CritChanceText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Stats")
	TObjectPtr<UCommonTextBlock> CritDamageText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Stats")
	TObjectPtr<UCommonTextBlock> MaxShieldText;

	UFUNCTION(BlueprintCallable, Category = "CharacterSelect")
	void SelectNext();

	UFUNCTION(BlueprintCallable, Category = "CharacterSelect")
	void SelectPrev();

	UFUNCTION(BlueprintCallable, Category = "CharacterSelect")
	void ConfirmSelection();

private:
	void HandleCharacterChanged();
	void HandleCloseButtonClicked();
	void FadeAndSwitch();
	void OnFadeOutFinished();

	FTimerHandle FadeTimerHandle;
	int32 CurrentIndex = 0;

	TArray<FNSCharacterSelectData> CachedCharacters;

	void ApplyPreviewImage(const FNSCharacterSelectData& Data);
	
	void UpdateBaseStatTexts(const FNSCharacterSelectData& Data);
	void ClearBaseStatTexts();
	
	// 현재 선택된 캐릭터에 해당하는 인덱스를 찾는다. 못 찾으면 0
	int32 FindInitialCharacterIndex() const;

	void BindSkillSlotEvents();
	void RefreshSkillSection(const FNSCharacterSelectData& Data);
	void ClearSkillSection();

	void PreviewSkillSlot(ENSCharacterSelectSkillSlot SlotType);
	void UpdateSkillSlotPreviewIndicators();
	void UpdateSkillDetailPanel(const FDataTableRowHandle& SkillUIDataRow, UTexture2D* InputIconTexture);
	void ClearSkillDetailPanel();

	// 선택한 스킬 데이터 개수에 맞춰 스탯 행을 다시 만듦.
	void RefreshSkillDetailStats(const FNSSkillUIData& SkillUIData);

	UNSCharacterSelectSkillSlotWidget* GetSkillSlotWidget(ENSCharacterSelectSkillSlot SlotType) const;

	void HandleSkillSlotHovered(ENSCharacterSelectSkillSlot SlotType);
	void HandleSkillSlotClicked(ENSCharacterSelectSkillSlot SlotType);

private:
	ENSCharacterSelectSkillSlot PreviewedSkillSlot = ENSCharacterSelectSkillSlot::BaseAttack;

	bool bHasPreviewedSkillSlot = false;

	// 같은 누락 항목이 호버할 때마다 반복 출력되지 않게 기억함.
	TSet<FString> LoggedMissingSkillStatKeys;
	
public:
	UPROPERTY(BlueprintAssignable, Category = "CharacterSelect")
	FOnCharacterSelectionConfirmed OnCharacterSelectionConfirmed;
};
