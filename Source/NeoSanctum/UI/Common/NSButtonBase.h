// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "NSButtonBase.generated.h"

/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()
protected:
	virtual void NativeOnClicked() override;
	virtual void NativeOnHovered() override;

	// 각 버튼 BP에서 개별 지정 가능. 비우면 소리 없음.
	// SoundDataTable의 SFX 카테고리 Row 이름을 넣는다.
	UPROPERTY(EditDefaultsOnly, Category = "NS|ButtonSound")
	FName ClickSoundID = NAME_None;

	// 각 버튼 BP에서 개별 지정 가능. 비우면 소리 없음.
	// SoundDataTable의 SFX 카테고리 Row 이름을 넣는다.
	UPROPERTY(EditDefaultsOnly, Category = "NS|ButtonSound")
	FName HoverSoundID = NAME_None;
};
