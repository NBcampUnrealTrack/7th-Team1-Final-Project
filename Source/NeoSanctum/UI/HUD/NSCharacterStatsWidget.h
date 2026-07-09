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
	TObjectPtr<UTextBlock> StatsText;
	
	void HandleCharacterStatsSnapshot(
		FGameplayTag Channel,
		const FNSCharacterStatsSnapshotMessage& Message);
	
	FGameplayMessageListenerHandle StatsSnapshotListenerHandle;
};
