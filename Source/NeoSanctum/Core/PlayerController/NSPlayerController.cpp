// Copyright 2026 One Team. All rights reserved.

#include "NSPlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "AbilitySystemComponent.h"
#include "NeoSanctum/Core/Interface/NSOutGameModeInterface.h"
#include "NeoSanctum/Core/Interface/NSGameInstanceInterface.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"
#include "NeoSanctum/GAS/AttributeSet/NsPlayerAttributeSet.h"

ANSPlayerController::ANSPlayerController()
{
	
}

void ANSPlayerController::BindAttributeToHUD()
{
	ANSPlayerState*NSPlayerState = GetPlayerState<ANSPlayerState>();
	if (!NSPlayerState)
	{
		return;
	}
	UAbilitySystemComponent* ASC = NSPlayerState->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}
	//체력 변경시 델리게이트에서 바인딩으로 체력 갱신
	ASC->GetGameplayAttributeValueChangeDelegate(
		UNSBaseAttributeSet::GetHealthAttribute()
		).AddUObject(this, &ANSPlayerController::OnHealthChanged);	
	//최대체력 변경시 델리게이트에서 바인딩으로 체력 갱신
	ASC->GetGameplayAttributeValueChangeDelegate(
		UNSBaseAttributeSet::GetMaxHealthAttribute()
		).AddUObject(this, &ANSPlayerController::OnMaxHealthChanged);
	//실드 변경시 델리게이트에서 바인딩으로 체력 갱신
	ASC->GetGameplayAttributeValueChangeDelegate(
		UNSPlayerAttributeSet::GetShieldAttribute()
		).AddUObject(this, &ANSPlayerController::OnShieldChanged);
	//최대실드 변경시 델리게이트에서 바인딩으로 체력갱신
	ASC->GetGameplayAttributeValueChangeDelegate(
		UNSPlayerAttributeSet::GetMaxShieldAttribute()
			).AddUObject(this, &ANSPlayerController::OnMaxShieldChanged);
}

void ANSPlayerController::UpdateHUDHealthAndShield()
{
	ANSPlayerState* NSPlayerState =
		GetPlayerState<ANSPlayerState>();
	if (!NSPlayerState)
	{
		return;
	}
	const UNSPlayerAttributeSet* PlayerAttributeSet =
		NSPlayerState->GetPlayerAttributeSet();
	if (!PlayerAttributeSet)
	{
		return;
	}
	UNSUIManagerSubsystem* UIManager =
		GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>();
	if (!UIManager)
	{
		return;
	}
	// 현재 Attribute 값을 로그로 확인
	UE_LOG(LogTemp, Warning, TEXT("HP: %.0f / %.0f, Shield: %.0f / %.0f"),
		PlayerAttributeSet->GetHealth(),
		PlayerAttributeSet->GetMaxHealth(),
		PlayerAttributeSet->GetShield(),
		PlayerAttributeSet->GetMaxShield()
	);

	// 현재 Attribute 값을 HUD에 한번 직접 반영
	UIManager->UpdateHealthAndShield(
		PlayerAttributeSet->GetHealth(),
		PlayerAttributeSet->GetMaxHealth(),
		PlayerAttributeSet->GetShield(),
		PlayerAttributeSet->GetMaxShield()
	);
}

void ANSPlayerController::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	//체력값이 바뀌면 전체 HP/Shield값을 다시 읽어 HUD에 반영
	UpdateHUDHealthAndShield();
}

void ANSPlayerController::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	//최대체력 변경시 전체 값을 다시 갱신한다
	UpdateHUDHealthAndShield();
}

void ANSPlayerController::OnShieldChanged(const FOnAttributeChangeData& Data)
{
	//실드 값이 바뀌면 HUD에 반영
	UpdateHUDHealthAndShield();
}

void ANSPlayerController::OnMaxShieldChanged(const FOnAttributeChangeData& Data)
{
	//최대실드 변경시 전체 값을 다시 갱신
	UpdateHUDHealthAndShield();
}

void ANSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	//HUD 생성 이후 Attribute 값 연결
	BindAttributeToHUD();
	UpdateHUDHealthAndShield();
}

void ANSPlayerController::Server_RequestStartRun_Implementation()
{
	if (HasAuthority())
	{
		AGameModeBase* CurrentGameMode = GetWorld()->GetAuthGameMode();
		
		if (CurrentGameMode && CurrentGameMode->Implements<UNSOutGameInterface>())
		{
			INSOutGameInterface::Execute_RequestStartRun(CurrentGameMode);
		}
	}
}

void ANSPlayerController::ExitSpectatorAndRespawn()
{
	if (!HasAuthority())
	{
		return;
	}

	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (!GameMode)
	{
		return;
	}
	
	// 직접 소환
	AActor* PlayerStartSpot = GameMode->FindPlayerStart(this);
	if (!PlayerStartSpot)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerStart를 찾을 수 없음"));
		return;
	}

	APawn* NewPawn = GameMode->SpawnDefaultPawnFor(this, PlayerStartSpot);
	if (!NewPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("폰 스폰 실패"));
		return;
	}
	
	Possess(NewPawn);

	Multicast_NotifyRespawn();
}

void ANSPlayerController::Multicast_NotifyRespawn_Implementation()
{
	if (IsLocalController())
	{
		// 호스트 클라이언트 리스폰 처리용
		if (GetPawn())
		{
			SetViewTargetWithBlend(GetPawn());
		}
		
		// 로딩 UI 종료용
		if (GetGameInstance() && GetGameInstance()->Implements<UNSGameInstanceInterface>())
		{
			INSGameInstanceInterface::Execute_HideLoadingScreen(GetGameInstance());
		}
	}
	
	
}
