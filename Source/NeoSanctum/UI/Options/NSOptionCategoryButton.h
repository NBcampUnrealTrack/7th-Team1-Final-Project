// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/UI/Common/NSButtonBase.h"
#include "NSOptionCategoryButton.generated.h"

class UImage;
class UTexture2D;

/**
 * 옵션 카테고리의 선택 상태에 따라 배경 이미지를 변경하는 버튼
 */
UCLASS(Abstract, Blueprintable)
class NEOSANCTUM_API UNSOptionCategoryButton : public UNSButtonBase
{
	GENERATED_BODY()
public:
	UNSOptionCategoryButton(
		const FObjectInitializer& ObjectInitializer);
	
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeOnSelected(bool bBroadcast) override;
	virtual void NativeOnDeselected(bool bBroadcast) override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;

	// 버튼 내부의 상태 표시 이미지
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CategoryBackgroundImage;

	// 선택되지 않은 상태의 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Option|Visual")
	TObjectPtr<UTexture2D> NormalTexture;

	// 선택된 상태의 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Option|Visual")
	TObjectPtr<UTexture2D> SelectedTexture;
	
	// 선택되지 않은 버튼에 마우스를 올렸을 때 사용할 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Option|Visual")
	TObjectPtr<UTexture2D> HoveredTexture;
	
	// 마우스를 올렸을 때 배경 이미지에 적용할 강조 색상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Option|Visual")
	FLinearColor HoverTint =
		FLinearColor(
			0.2f,
			1.5f,
			2.0f,
			1.0f);

private:
	void ApplySelectionVisual(bool bInSelected);
	void ApplyTexture(UTexture2D* Texture);
};