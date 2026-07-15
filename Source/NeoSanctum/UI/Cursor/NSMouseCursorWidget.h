// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NSMouseCursorWidget.generated.h"

class USizeBox;
/**
 *
 */
UCLASS(Abstract)
class NEOSANCTUM_API UNSMouseCursorWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> CursorRootSizeBox;

	// 1920X1080 해상도에서 실제로 보여줄 커서 크기.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cursor")
	FVector2D BaseCursorImageSize = FVector2D(32.0f, 39.0f);

private:
	void HandleViewPortResized(FViewport* Viewport, uint32 Unused);
	void ApplyCursorSize();
};
