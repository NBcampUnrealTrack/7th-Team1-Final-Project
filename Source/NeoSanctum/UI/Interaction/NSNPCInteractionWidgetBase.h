// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSNPCInteractionWidgetBase.generated.h"

/**
 * NPC 상호작용 위젯의 공통 베이스
 * 컨트롤러 통지(ActiveInteractionWidget 정리 + 입력 매핑 복원) 및 RemoveFromParent를
 * CloseWidget()이 항상 보장하므로, 파생 위젯은 이 절차를 직접 챙길 필요 없이
 * OnCloseWidget()에서 자신만의 입력모드/마우스커서/리소스 정리
 */
UCLASS(Abstract)
class NEOSANCTUM_API UNSNPCInteractionWidgetBase : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	virtual void OpenForInteractor(APlayerController* Interactor) {};

	// 파생 클래스는 재정의하지 말 것 — 위젯별 정리는 OnCloseWidget()에서 처리한다.
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void CloseWidget();

protected:
	//파생 위젯이 자신만의 입력모드/마우스커서/리소스를 정리하는 지점
	virtual void OnCloseWidget() {};

	UPROPERTY()
	TWeakObjectPtr<APlayerController> OwningController;
};
