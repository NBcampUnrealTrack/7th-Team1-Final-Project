// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine//DataTable.h"
#include "NSUIWidgetData.generated.h"

class UCommonUserWidget;

/**
 *  UIManager에서 생성할 위젯을 담는 데이터테이블
 */

USTRUCT(BlueprintType)
struct FNSUIWidgetData : public FTableRowBase
{
	GENERATED_BODY()
	
	//생성할 위젯 블루프린트 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSoftClassPtr<UCommonUserWidget> WidgetClass;
};
