// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSNPCInteractionWidgetBase.generated.h"

UCLASS(Abstract)
class NEOSANCTUM_API UNSNPCInteractionWidgetBase : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	virtual void OpenForInteractor(APlayerController* Interactor) {};
	virtual void CloseWidget() {};
};
