// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerWorldStatusSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "NSPlayerWorldStatusHost.h"
#include "NSPlayerWorldStatusPresenter.h"

// PlayerController에서 플레이어 월드 상태 Subsystem을 조회하는 함수
UNSPlayerWorldStatusSubsystem* UNSPlayerWorldStatusSubsystem::Get(const APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return nullptr;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	return LocalPlayer
		       ? LocalPlayer->GetSubsystem<UNSPlayerWorldStatusSubsystem>()
		       : nullptr;
}

// Subsystem 생성 시 Presenter를 준비하는 함수
void UNSPlayerWorldStatusSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Presenter = NewObject<UNSPlayerWorldStatusPresenter>(this);
	if (Presenter)
	{
		Presenter->Initialize(GetLocalPlayer());
	}
}

// Subsystem 해제 시 Presenter와 Host 참조를 정리하는 함수
void UNSPlayerWorldStatusSubsystem::Deinitialize()
{
	if (Presenter)
	{
		Presenter->Shutdown();
		Presenter = nullptr;
	}

	HUDHostObject.Reset();

	Super::Deinitialize();
}

// HUD Root가 제공하는 플레이어 월드 상태 Host를 등록하는 함수
void UNSPlayerWorldStatusSubsystem::RegisterHUDHost(UObject* InHostObject)
{
	if (!InHostObject ||
		!InHostObject->GetClass()->ImplementsInterface(UNSPlayerWorldStatusHost::StaticClass()))
	{
		return;
	}

	HUDHostObject = InHostObject;

	if (Presenter)
	{
		Presenter->SetHUDHost(InHostObject);
	}
}

// 등록된 플레이어 월드 상태 Host를 해제하는 함수
void UNSPlayerWorldStatusSubsystem::UnregisterHUDHost(UObject* InHostObject)
{
	if (!InHostObject || HUDHostObject.Get() != InHostObject)
	{
		return;
	}

	if (Presenter)
	{
		Presenter->SetHUDHost(nullptr);
	}

	HUDHostObject.Reset();
}

// 현재 등록된 플레이어 월드 상태 Host를 반환하는 함수
INSPlayerWorldStatusHost* UNSPlayerWorldStatusSubsystem::GetHUDHost() const
{
	return HUDHostObject.IsValid()
		       ? Cast<INSPlayerWorldStatusHost>(HUDHostObject.Get())
		       : nullptr;
}
