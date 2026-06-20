// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NSPartEquipWidget.generated.h"

class UNSPartDefinition;
class UNSPartEquipComponent;

UCLASS()
class NEOSANCTUM_API UNSPartEquipWidget : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	// 파츠 NPC가 호출 -> 뷰포트 추가 + 입력모드 전환
	void OpenForInteractor(APlayerController* Interactor);

	// 목록에서 선택한 Common 파츠 장착
	UFUNCTION(BlueprintCallable, Category = "Part")
	void EquipSelectedPart(int32 SelectableIndex);

	// 닫기 호출
	UFUNCTION(BlueprintCallable, Category = "Part")
	void RequestClose();

	// 실제 닫기 + 입력모드 복구
	UFUNCTION(BlueprintCallable, Category = "Part")
	void CloseWidget();

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
