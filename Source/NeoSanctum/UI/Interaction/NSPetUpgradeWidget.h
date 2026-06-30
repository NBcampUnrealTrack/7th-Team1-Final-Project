// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/UI/Interaction/NSNPCInteractionWidgetBase.h"
#include "NSPetUpgradeWidget.generated.h"

class UCommonButtonBase;
class UCommonTextBlock;
class UNSCompanionDefinition;
class UNSPlayerProgressComponent;
class UImage;

/**
 * 펫 강화 UI (최소 구현)
 * 펫 강화 백엔드 미구현 —> 현재는 오픈/클로즈 + 입력모드 전환만 담당
 */
UCLASS()
class NEOSANCTUM_API UNSPetUpgradeWidget : public UNSNPCInteractionWidgetBase
{
	GENERATED_BODY()

public:
	virtual void OpenForInteractor(APlayerController* Interactor) override;

	UFUNCTION(BlueprintCallable, Category = "Pet")
	virtual void CloseWidget() override;
	
	//현재 표시할 펫/Companion 강화 데이터 설정
	UFUNCTION(BlueprintCallable, Category = "Pet|Upgrade")
	void SetCompanionDefinition(UNSCompanionDefinition* NewDefinition);
	
	//특정 노드를 선택할때 호출
	UFUNCTION(BlueprintCallable, Category = "Pet|Upgrade")
	void SelectUpgradeNodeByIndex(int32 NodeIndex);
	
	UFUNCTION(BlueprintCallable, Category = "Pet|Upgrade")
	void SelectUpgradeNodeByTag(FGameplayTag NodeTag);

private:
	void RefreshUpgradeInfo();
	void RefreshSelectedNodeInfo();

	UFUNCTION()
	void HandleUpgradeClicked();

	UFUNCTION()
	void HandleCloseClicked();
	
	
	UPROPERTY(EditDefaultsOnly, Category = "Pet|Upgrade")
	FGameplayTag DefaultSelectedNodeTag;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> UpgradeButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> SelectedNodeNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> SelectedNodeLevelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> SelectedNodeEffectText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> SelectedNodeConditionText;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> SelectedNodeIcon;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> UpgradeCostText;

	UPROPERTY(Transient)
	TObjectPtr<UNSCompanionDefinition> CurrentCompanionDefinition;

	TWeakObjectPtr<APlayerController> OwningController;
	TWeakObjectPtr<UNSPlayerProgressComponent> BoundProgressComponent;

	FGameplayTag SelectedNodeTag;

	int32 SelectedNodeIndex = INDEX_NONE;
	
	void BindProgressChanged();
	void UnbindProgressChanged();
	void HandleProgressChanged();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
};
