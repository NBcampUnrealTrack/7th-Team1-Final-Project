// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/UI/Interaction/NSNPCInteractionWidgetBase.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NSPartEquipWidget.generated.h"

class UNSPartDefinition;
class UNSPartEquipComponent;

UCLASS()
class NEOSANCTUM_API UNSPartEquipWidget : public UNSNPCInteractionWidgetBase
{
	GENERATED_BODY()
public:
	virtual void OpenForInteractor(APlayerController* Interactor) override;

	// 목록에서 선택한 Common 파츠 장착
	UFUNCTION(BlueprintCallable, Category = "Part")
	void EquipSelectedPart(int32 SelectableIndex);

	// 닫기 호출
	UFUNCTION(BlueprintCallable, Category = "Part")
	void RequestClose();

	// 실제 닫기 + 입력모드 복구
	UFUNCTION(BlueprintCallable, Category = "Part")
	virtual void CloseWidget() override;

	// 현재 슬롯 장착 파츠 조회
	UFUNCTION(BlueprintCallable, Category = "Part")
	bool GetEquippedDefinition(ENSPartSlot PartSlot, UNSPartDefinition*& OutDefinition) const;

protected:
	// 변경사항 있을 때 저장 확인 다이얼로그
	UFUNCTION(BlueprintImplementableEvent, Category = "Part")
	void ShowSaveConfirmDialog();

	// 선택 가능한 파츠 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Part")
	TArray<TSoftObjectPtr<UNSPartDefinition>> SelectableParts;
private:
	UNSPartEquipComponent* GetEquipComponent() const;

	UPROPERTY()
	TWeakObjectPtr<APlayerController> OwningController;

	bool bDirty = false;
};
