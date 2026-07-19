// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NeoSanctum/Data/Part/NSPartTypes.h"
#include "NSPartDetailWidget.generated.h"

class UTextBlock;
class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNSPartDefinition;
class ANSPartPreviewStage;
struct FNSPartSaveData;

/**
 * 파츠 설명 상시 패널. 카탈로그 후보(정의)와 장착중인 파츠(인스턴스) 양쪽에서 공용으로 사용한다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NEOSANCTUM_API UNSPartDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 카탈로그에서 선택한 후보 파츠 표시 (정의 정보: 비용/등급별 수치범위/리롤가능)
	void SetupFromDefinition(const FNSPartDefinitionRow& Row, const UNSPartDefinition* Def);

	// 실제 장착중인 파츠 표시 (인스턴스 정보: 현재 등급/현재 수치)
	void SetupFromEquipped(const FNSPartSaveData& SaveData, const UNSPartDefinition* Def);

	// 인런 인스턴스(FNSPartData) 표시: 이름 + 현재 등급 + 현재 수치
	void SetupFromInstance(const FNSPartData& Part, const UNSPartDefinition* Def);

	// 잠긴 슬롯의 언락 정보 표시 (슬롯 이름 + 언락 비용). 3D 프리뷰 없음
	void SetupFromSlotLock(const FNSPartSlotRow& SlotRow);

	// 선택/장착 없음 상태로 초기화
	void ClearDetail();

	// 실시간 3D 프리뷰 연결 (null이면 ClearPreview와 동일). 드래그 회전을 위해 스테이지 자체를 받는다
	void SetPreviewTarget(ANSPartPreviewStage* Stage);

	// 프리뷰 이미지 숨김
	void ClearPreview();

protected:
	virtual void NativeConstruct() override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 프리뷰 RenderTarget을 표시할 UI 머티리얼 (TextureParameter 이름: "Texture")
	UPROPERTY(EditDefaultsOnly, Category = "Part|Preview")
	TObjectPtr<UMaterialInterface> PreviewMaterialBase;

	// 마우스 드래그 1px당 회전 각도
	UPROPERTY(EditDefaultsOnly, Category = "Part|Preview")
	float PreviewDragSensitivity = 0.5f;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> PreviewImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PartNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RarityText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PreviewMID;

	TWeakObjectPtr<ANSPartPreviewStage> PreviewStageRef;
	bool bIsDraggingPreview = false;
};
