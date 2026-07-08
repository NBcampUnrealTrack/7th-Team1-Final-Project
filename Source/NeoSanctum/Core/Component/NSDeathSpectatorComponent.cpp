// Copyright 2026 One Team. All rights reserved.

#include "NSDeathSpectatorComponent.h"

#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Character/Spectator/NSDeathSpectatorPawn.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"

UNSDeathSpectatorComponent::UNSDeathSpectatorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	DeathSpectatorPawnClass = ANSDeathSpectatorPawn::StaticClass();
}

void UNSDeathSpectatorComponent::RequestEnterDeathSpectatorMode()
{
	ANSPlayerController* OwnerPlayerController = GetOwnerPlayerController();
	if (!OwnerPlayerController || !OwnerPlayerController->IsLocalController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ClearDeathSpectatorModeTimer();

	if (DeathSpectatorModeDelay <= 0.f)
	{
		EnterDeathSpectatorMode();
		return;
	}

	// 사망 후 DeathSpectatorModeDelay(기본 2초) 시간 이후에 관전자 모드로 진입
	World->GetTimerManager().SetTimer(
		DeathSpectatorModeTimerHandle,
		this,
		&ThisClass::EnterDeathSpectatorMode,
		DeathSpectatorModeDelay,
		false
	);
}

void UNSDeathSpectatorComponent::SpectatePreviousPlayer()
{
	SwitchSpectatorTarget(-1);
}

void UNSDeathSpectatorComponent::SpectateNextPlayer()
{
	SwitchSpectatorTarget(1);
}

void UNSDeathSpectatorComponent::ApplyConfirmedSpectatorTarget(ANSPlayerCharacterBase* TargetCharacter)
{
	ANSPlayerController* OwnerPlayerController = GetOwnerPlayerController();
	if (!OwnerPlayerController || !OwnerPlayerController->IsLocalController() || !TargetCharacter)
	{
		return;
	}

	ANSPlayerState* TargetPlayerState = TargetCharacter->GetPlayerState<ANSPlayerState>();
	SpectatingPlayerState = TargetPlayerState;

	// 서버에서 확정한 관전 대상 Pawn을 실제 ViewTarget으로 적용
	OwnerPlayerController->SetViewTargetWithBlend(TargetCharacter, DeathSpectatorViewBlendTime);

	if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(this))
	{
		UIManager->ShowSpectator(TargetPlayerState ? TargetPlayerState->GetPlayerName() : TargetCharacter->GetName());
	}
}

bool UNSDeathSpectatorComponent::HandleClientRestart(APawn* NewPawn)
{
	ANSPlayerController* OwnerPlayerController = GetOwnerPlayerController();
	if (!OwnerPlayerController || !OwnerPlayerController->IsLocalController())
	{
		return false;
	}

	ClearDeathSpectatorModeTimer();
	const bool bIsDeathSpectatorRestart = NewPawn && NewPawn->IsA<ANSDeathSpectatorPawn>();
	if (!bIsDeathSpectatorRestart)
	{
		SpectatingPlayerState = nullptr;
		return false;
	}

	// 사망 직후 첫 관전 대상을 결정하고 해당 화면 View를 볼 수 있게 수동으로 NextPlayer를 호출해줘야함
	if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(this))
	{
		UIManager->CreateSpectator(OwnerPlayerController);
		UIManager->ShowSpectator(TEXT(""));
	}
	OwnerPlayerController->SetViewTarget(NewPawn);
	if (ANSDeathSpectatorPawn* DeathSpectatorPawn = Cast<ANSDeathSpectatorPawn>(NewPawn))
	{
		DeathSpectatorPawn->RefreshSpectatorTargetView();
	}
	return true;
}

void UNSDeathSpectatorComponent::ClearSpectatorState()
{
	ClearDeathSpectatorModeTimer();
	SpectatingPlayerState = nullptr;
}

bool UNSDeathSpectatorComponent::GetSpectatorReplicationViewPoint(FVector& Location, FRotator& Rotation) const
{
	const ANSPlayerController* OwnerPlayerController = GetOwnerPlayerController();
	if (!OwnerPlayerController || OwnerPlayerController->IsLocalController())
	{
		return false;
	}

	if (const ANSDeathSpectatorPawn* DeathSpectatorPawn = Cast<ANSDeathSpectatorPawn>(OwnerPlayerController->GetPawn()))
	{
		// 서버에서 관전자의 복제 기준 위치만 Spectator Pawn 위치와 방향으로 고정하기 위함
		Location = DeathSpectatorPawn->GetActorLocation();
		Rotation = DeathSpectatorPawn->GetActorRotation();
		return true;
	}

	return false;
}

ANSPlayerController* UNSDeathSpectatorComponent::GetOwnerPlayerController() const
{
	return Cast<ANSPlayerController>(GetOwner());
}

void UNSDeathSpectatorComponent::EnterDeathSpectatorMode()
{
	ANSPlayerController* OwnerPlayerController = GetOwnerPlayerController();
	if (!OwnerPlayerController || !OwnerPlayerController->IsLocalController())
	{
		return;
	}

	if (OwnerPlayerController->HasAuthority())
	{
		SpawnAndPossessDeathSpectatorPawn();
		return;
	}

	// 사망 관전자 모드 Input 태그에 따라서 InputConfig 안에 있는 IMC를 골라서 교체
	// 서버 권한으로 관전자 Pawn 생성 요청
	Server_EnterDeathSpectatorMode();
}

void UNSDeathSpectatorComponent::SpawnAndPossessDeathSpectatorPawn()
{
	ANSPlayerController* OwnerPlayerController = GetOwnerPlayerController();
	if (!OwnerPlayerController || !OwnerPlayerController->HasAuthority())
	{
		return;
	}

	if (OwnerPlayerController->GetPawn() && OwnerPlayerController->GetPawn()->IsA<ANSDeathSpectatorPawn>())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !DeathSpectatorPawnClass)
	{
		return;
	}

	APawn* PreviousPawn = OwnerPlayerController->GetPawn();
	// 사망 지점에 관전자 Pawn 생성 후 서버에서 관전 대상 근처로 이동
	const FVector SpectatorSpawnLocation = PreviousPawn ? PreviousPawn->GetActorLocation() : FVector::ZeroVector;
	const FRotator SpectatorSpawnRotation = PreviousPawn ? PreviousPawn->GetActorRotation() : OwnerPlayerController->GetControlRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerPlayerController;
	SpawnParams.Instigator = PreviousPawn;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ANSDeathSpectatorPawn* DeathSpectatorPawn = World->SpawnActor<ANSDeathSpectatorPawn>(
		DeathSpectatorPawnClass,
		SpectatorSpawnLocation,
		SpectatorSpawnRotation,
		SpawnParams
	);

	if (!DeathSpectatorPawn)
	{
		return;
	}

	OwnerPlayerController->Possess(DeathSpectatorPawn);
	OwnerPlayerController->SetViewTarget(DeathSpectatorPawn);

	// 서버 권한에서 첫 관전 대상 즉시 확정
	ApplyServerSpectatorTargetChange(1);
}

void UNSDeathSpectatorComponent::ClearDeathSpectatorModeTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathSpectatorModeTimerHandle);
	}
}

void UNSDeathSpectatorComponent::SwitchSpectatorTarget(int32 Direction)
{
	ANSPlayerController* OwnerPlayerController = GetOwnerPlayerController();
	if (!OwnerPlayerController || !OwnerPlayerController->IsLocalController())
	{
		return;
	}

	// 관전자 Pawn을 빙의한 상태에서만 관전 대상 전환 허용
	if (!OwnerPlayerController->GetPawn() || !OwnerPlayerController->GetPawn()->IsA<ANSDeathSpectatorPawn>())
	{
		return;
	}

	if (!OwnerPlayerController->HasAuthority())
	{
		// 서버 권한에서 관전 대상 선택 확정
		Server_RequestSpectatorTargetChange(Direction);
		return;
	}

	ApplyServerSpectatorTargetChange(Direction);
}

void UNSDeathSpectatorComponent::ApplyServerSpectatorTargetChange(int32 Direction)
{
	ANSPlayerController* OwnerPlayerController = GetOwnerPlayerController();
	if (!OwnerPlayerController || !OwnerPlayerController->HasAuthority())
	{
		return;
	}

	ANSDeathSpectatorPawn* DeathSpectatorPawn = Cast<ANSDeathSpectatorPawn>(OwnerPlayerController->GetPawn());
	const ANSPlayerState* ViewerPlayerState = OwnerPlayerController->GetPlayerState<ANSPlayerState>();
	const ANSRunGameState* RunGameState = GetWorld() ? GetWorld()->GetGameState<ANSRunGameState>() : nullptr;
	if (!DeathSpectatorPawn || !ViewerPlayerState || !RunGameState)
	{
		return;
	}

	TArray<ANSPlayerState*> AlivePlayerStates;
	RunGameState->GetAlivePlayerStates(AlivePlayerStates, ViewerPlayerState);
	if (AlivePlayerStates.IsEmpty())
	{
		DeathSpectatorPawn->SetSpectatorTarget(nullptr);
		return;
	}

	// 생존 후보 1명이고 기존 대상과 동일한 경우 재확정 생략
	if (AlivePlayerStates.Num() == 1
		&& AlivePlayerStates[0] == SpectatingPlayerState.Get()
		&& DeathSpectatorPawn->GetSpectatorTarget())
	{
		return;
	}

	// 현재 관전 대상 기준 이전/다음 생존자 순환 탐색
	const int32 Step = Direction >= 0 ? 1 : -1;
	int32 CurrentIndex = AlivePlayerStates.IndexOfByKey(SpectatingPlayerState.Get());
	if (CurrentIndex == INDEX_NONE)
	{
		CurrentIndex = Step > 0 ? -1 : 0;
	}

	for (int32 Attempt = 0; Attempt < AlivePlayerStates.Num(); ++Attempt)
	{
		const int32 TargetIndex = (CurrentIndex + (Step * (Attempt + 1)) + AlivePlayerStates.Num()) % AlivePlayerStates.Num();
		ANSPlayerState* TargetPlayerState = AlivePlayerStates[TargetIndex];
		ANSPlayerCharacterBase* TargetCharacter = ResolveServerSpectatorTargetPawn(TargetPlayerState);
		if (!TargetCharacter)
		{
			continue;
		}

		SpectatingPlayerState = TargetPlayerState;

		// 서버 확정 대상 복제 및 대상 위치 추적 시작
		DeathSpectatorPawn->SetSpectatorTarget(TargetCharacter);

		if (OwnerPlayerController->IsLocalController())
		{
			ApplyConfirmedSpectatorTarget(TargetCharacter);
		}
		return;
	}

	DeathSpectatorPawn->SetSpectatorTarget(nullptr);
}

ANSPlayerCharacterBase* UNSDeathSpectatorComponent::ResolveServerSpectatorTargetPawn(const ANSPlayerState* TargetPlayerState) const
{
	const ANSPlayerController* OwnerPlayerController = GetOwnerPlayerController();
	if (!OwnerPlayerController || !OwnerPlayerController->HasAuthority() || !TargetPlayerState)
	{
		return nullptr;
	}

	if (ANSPlayerCharacterBase* TargetCharacter = Cast<ANSPlayerCharacterBase>(TargetPlayerState->GetPawn()))
	{
		return TargetCharacter;
	}

	// PlayerState가 Pawn을 캐시하지 못한 경우 소유 Controller에서 재확인
	const AController* TargetController = Cast<AController>(TargetPlayerState->GetOwner());
	return TargetController ? Cast<ANSPlayerCharacterBase>(TargetController->GetPawn()) : nullptr;
}

void UNSDeathSpectatorComponent::Server_EnterDeathSpectatorMode_Implementation()
{
	SpawnAndPossessDeathSpectatorPawn();
}

void UNSDeathSpectatorComponent::Server_RequestSpectatorTargetChange_Implementation(int32 Direction)
{
	ApplyServerSpectatorTargetChange(Direction);
}
