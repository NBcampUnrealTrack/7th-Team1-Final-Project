// Copyright 2026 One Team. All rights reserved.


#include "NSGameInstance.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSessionSubsystem.h"

void UNSGameInstance::Init()
{
	Super::Init();
}

void UNSGameInstance::Shutdown()
{
	// 게임 종료 시 열려있는 세션 정리
	UNSSessionSubsystem* NSSessionSubsystem = GetSubsystem<UNSSessionSubsystem>();
	if (NSSessionSubsystem)
	{
		NSSessionSubsystem->DestroySession();
	}
	
	Super::Shutdown();
	
}
