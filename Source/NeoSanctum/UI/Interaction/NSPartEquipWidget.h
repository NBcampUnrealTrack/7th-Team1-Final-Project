// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/UI/Interaction/NSNPCInteractionWidgetBase.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NeoSanctum/Core/PlayerState/NSProgressTypes.h"
#include "NSPartEquipWidget.generated.h"

class UNSPartDefinition;

UCLASS()
class NEOSANCTUM_API UNSPartEquipWidget : public UNSNPCInteractionWidgetBase
{
	GENERATED_BODY()
public:
	virtual void OpenForInteractor(APlayerController* Interactor) override;

	// 닫기 호출 (변경 있으면 다이얼로그)
	UFUNCTION(BlueprintCallable, Category = "Part")
	void RequestClose();

	// 실제 닫기 + 입력모드 복구
	UFUNCTION(BlueprintCallable, Category = "Part")
	virtual void CloseWidget() override;

	// 영구재화로 파츠 언락 (Common 등급)
	UFUNCTION(BlueprintCallable, Category = "Part")
	bool RequestUnlockPart(TSoftObjectPtr<UNSPartDefinition> Definition);

	// 소유한 파츠를 장착 저장 (Common 등급)
	UFUNCTION(BlueprintCallable, Category = "Part")
	void RequestEquipPart(TSoftObjectPtr<UNSPartDefinition> Definition);

	// 해당 파츠(Common 등급) 소유 여부
	UFUNCTION(BlueprintPure, Category = "Part")
	bool IsPartOwned(TSoftObjectPtr<UNSPartDefinition> Definition) const;

	// 현재 캐릭터의 장착 파츠 (미장착 시 Definition == null)
	UFUNCTION(BlueprintPure, Category = "Part")
	FNSPartSaveData GetEquippedPart() const;

	// DT 전체 row 목록 (bEnabled 포함 전부 — 필터는 BP에서)
	UFUNCTION(BlueprintPure, Category = "Part")
	TArray<FNSPartDefinitionRow> GetAllPartRows() const;

	// 현재 영구 재화량
	UFUNCTION(BlueprintPure, Category = "Part")
	int64 GetCommonCurrency() const;

protected:
	// 변경사항 있을 때 저장 확인 다이얼로그
	UFUNCTION(BlueprintImplementableEvent, Category = "Part")
	void ShowSaveConfirmDialog();

private:
	UPROPERTY()
	TWeakObjectPtr<APlayerController> OwningController;

	bool bDirty = false;
};
