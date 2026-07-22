// Copyright 2026 One Team. All rights reserved.

#include "NSNPCInteractionWidgetBase.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"

void UNSNPCInteractionWidgetBase::CloseWidget()
{
	OnCloseWidget();

	/**
	 * X버튼/ESC 등 위젯 자체 경로로 닫혀도 이동 매핑 복원 + ActiveInteractionWidget 정리가 되도록 통지
	 * 파생 위젯이 개별적으로 이 통지를 빠뜨리는 것을 막기 위해 베이스에서 항상 수행
	 */
	if (ANSPlayerController* NSPC = Cast<ANSPlayerController>(OwningController.Get()))
	{
		NSPC->NotifyInteractionWidgetClosed(this);
	}

	RemoveFromParent();
}
