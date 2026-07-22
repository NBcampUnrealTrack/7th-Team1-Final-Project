// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NeoSanctum/Type/NSCharacterStatsMessageTypes.h"
#include "NSCharacterStatsWidget.generated.h"

class UTextBlock;

/**
 *  C패널에서 표시되는 캐릭터 스텟 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSCharacterStatsWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CT_MaxHealthLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MaxHealthText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CT_MaxShieldLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MaxShieldText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CT_ShieldRegenRateLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ShieldRegenRateText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CT_ShieldRegenCooldownLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ShieldRegenCooldownText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CT_BaseDamageLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BaseDamageText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CT_DefenseLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DefenseText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CT_MoveSpeedLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MoveSpeedText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CT_CritChanceLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CritChanceText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CT_CritDamageLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CritDamageText;

	void ApplyStatToWidgets(
		const FNSCharacterStatViewData& Stat,
		UTextBlock* LabelWidget,
		UTextBlock* ValueWidget) const;

	FText FormatStatValue(
		float Value,
		ENSCharacterStatDisplayType DisplayType) const;
	
	void HandleCharacterStatsSnapshot(
		FGameplayTag Channel,
		const FNSCharacterStatsSnapshotMessage& Message);
	
	FGameplayMessageListenerHandle StatsSnapshotListenerHandle;
};
