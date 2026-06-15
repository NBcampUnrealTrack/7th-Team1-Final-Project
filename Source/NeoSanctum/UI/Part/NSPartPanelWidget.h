// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NSPartPanelWidget.generated.h"

class UNSPartEquipComponent;
class UNSPartSlotButton;

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

	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UNSPartSlotButton> BodySlotButton;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UNSPartSlotButton> ArmSlotButton;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Part", meta = (BindWidgetOptional))
	TObjectPtr<UNSPartSlotButton> LegSlotButton;

private:
	UNSPartEquipComponent* GetPartEquipComponent() const;
	void BindPartEquipComponent();
	void UnbindPartEquipComponent();

	void ApplySlot(ENSPartSlot PartSlot, UNSPartSlotButton* SlotButton);
	void HandlePartChanged(ENSPartSlot PartSlot, const FNSPartData& PartData);

	TWeakObjectPtr<UNSPartEquipComponent> CachedPartEquipComponent;
};