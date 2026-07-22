// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NSPartPanelWidget.generated.h"

class UNSPartEquipComponent;
class UNSPartSlotButton;
class UPanelWidget;

/**
 * 현재 장착 중인 파츠를 표시하는 패널 위젯이다
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NEOSANCTUM_API UNSPartPanelWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	//현재 장착 파츠 상태를 다시 읽어 슬롯 UI를 갱신한다.
	UFUNCTION(BlueprintCallable, Category = "UI|Part")
	void RefreshEquippedParts();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 슬롯 버튼을 런타임에 스폰할 컨테이너
	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidget))
	TObjectPtr<UPanelWidget> SlotButtonContainer;

	// 슬롯 버튼 1개짜리 템플릿
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Part")
	TSubclassOf<UNSPartSlotButton> SlotButtonTemplate;

private:
	UNSPartEquipComponent* GetPartEquipComponent() const;
	void BindPartEquipComponent();
	void UnbindPartEquipComponent();

	// 슬롯 DT를 읽어 버튼을 동적 생성
	void BuildSlotButtons();

	UFUNCTION()
	void OnOutGameDataReady();

	void ApplySlot(FGameplayTag PartSlot, UNSPartSlotButton* SlotButton);
	void HandlePartChanged(FGameplayTag PartSlot, const FNSPartData& PartData);

	// 슬롯 태그 → 버튼 맵
	TMap<FGameplayTag, TObjectPtr<UNSPartSlotButton>> SlotButtonMap;

	TWeakObjectPtr<UNSPartEquipComponent> CachedPartEquipComponent;
};
