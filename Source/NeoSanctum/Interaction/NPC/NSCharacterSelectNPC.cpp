// Copyright 2026 One Team. All rights reserved.


#include "NSCharacterSelectNPC.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/Waypoint/NSOutRunGuideSubsystem.h"

bool ANSCharacterSelectNPC::CanInteract_Implementation(APlayerController* Interactor) const
{
	// 캐릭터 교체는 해금요소가 아니므로 Interactor가 nullptr이 아니면 바로 가능
	return Interactor != nullptr;
}

bool ANSCharacterSelectNPC::OnInteract_Implementation(APlayerController* Interactor)
{
	ANSPlayerController* PC = Cast<ANSPlayerController>(Interactor);
	if (!PC)
	{
		return false;
	}

	PC->ShowCharacterSelectWidget();

	// 첫 상호작용 → 안내 완료 처리 (이 함수는 상호작용한 플레이어의 클라에서 실행됨)
	if (UNSOutRunGuideSubsystem* GuideSubsystem =
		GetWorld()->GetSubsystem<UNSOutRunGuideSubsystem>())
	{
		GuideSubsystem->NotifyCharacterConsoleUsed();
	}

	return true;
}
