// Copyright 2026 One Team. All rights reserved.

#include "NSMonsterUISubsystem.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "NSBossMonsterPresenter.h"
#include "NSMonsterUIHost.h"
#include "NSNormalMonsterPresenter.h"

UNSMonsterUISubsystem* UNSMonsterUISubsystem::Get(
	const APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return nullptr;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	return LocalPlayer
		       ? LocalPlayer->GetSubsystem<UNSMonsterUISubsystem>()
		       : nullptr;
}

void UNSMonsterUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	NormalMonsterPresenter = NewObject<UNSNormalMonsterPresenter>(this);
	if (NormalMonsterPresenter)
	{
		NormalMonsterPresenter->Initialize(GetLocalPlayer());
	}

	BossMonsterPresenter = NewObject<UNSBossMonsterPresenter>(this);
	if (BossMonsterPresenter)
	{
		BossMonsterPresenter->Initialize(GetLocalPlayer());
	}
}

void UNSMonsterUISubsystem::Deinitialize()
{
	if (NormalMonsterPresenter)
	{
		NormalMonsterPresenter->Shutdown();
		NormalMonsterPresenter = nullptr;
	}

	if (BossMonsterPresenter)
	{
		BossMonsterPresenter->Shutdown();
		BossMonsterPresenter = nullptr;
	}

	HUDHostObject.Reset();

	Super::Deinitialize();
}

void UNSMonsterUISubsystem::RegisterHUDHost(UObject* InHostObject)
{
	if (!InHostObject ||
		!InHostObject->GetClass()->ImplementsInterface(UNSMonsterUIHost::StaticClass()))
	{
		return;
	}

	HUDHostObject = InHostObject;
}

void UNSMonsterUISubsystem::UnregisterHUDHost(UObject* InHostObject)
{
	if (!InHostObject || HUDHostObject.Get() != InHostObject)
	{
		return;
	}

	HUDHostObject.Reset();
}

INSMonsterUIHost* UNSMonsterUISubsystem::GetHUDHost() const
{
	return HUDHostObject.IsValid()
		       ? Cast<INSMonsterUIHost>(HUDHostObject.Get())
		       : nullptr;
}
