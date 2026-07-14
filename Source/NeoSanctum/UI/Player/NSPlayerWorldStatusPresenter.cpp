// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerWorldStatusPresenter.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"
#include "NSPlayerWorldStatusHost.h"
#include "NSPlayerWorldStatusViewModel.h"
#include "NSPlayerWorldStatusWidget.h"
#include "GameFramework/GameStateBase.h"

// 로컬 플레이어 기준 플레이어 월드 상태 Presenter를 초기화하는 함수
void UNSPlayerWorldStatusPresenter::Initialize(ULocalPlayer* InLocalPlayer)
{
	OwningLocalPlayer = InLocalPlayer;

	StartRosterRefreshTimer();
	RefreshTrackedPlayers();
}

// HUD Host를 등록하거나 해제하는 함수
void UNSPlayerWorldStatusPresenter::SetHUDHost(UObject* InHUDHostObject)
{
	if (HUDHostObject.Get() == InHUDHostObject)
	{
		return;
	}

	ReleaseAllEntries();
	HUDHostObject = InHUDHostObject;

	RefreshTrackedPlayers();
}

// Presenter가 보유한 런타임 상태를 해제하는 함수
void UNSPlayerWorldStatusPresenter::Shutdown()
{
	StopRosterRefreshTimer();
	StopPositionUpdateTimer();
	ReleaseAllEntries();

	HUDHostObject.Reset();
	OwningLocalPlayer.Reset();
	PlayerWorldStatusWidgetClass = nullptr;
}

// 현재 접속 중인 팀원 목록과 활성 Entry 목록을 동기화하는 함수
void UNSPlayerWorldStatusPresenter::RefreshTrackedPlayers()
{
	UWorld* World = GetPresenterWorld();
	UCanvasPanel* PlayerCanvas = GetPlayerWorldStatusCanvas();
	if (!World || !PlayerCanvas)
	{
		ReleaseAllEntries();
		return;
	}

	AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		return;
	}

	TSet<int32> CurrentPlayerIds;

	for (APlayerState* PlayerStateBase : GameState->PlayerArray)
	{
		ANSPlayerState* PlayerState = Cast<ANSPlayerState>(PlayerStateBase);
		if (!IsValidPlayerStateTarget(PlayerState))
		{
			continue;
		}

		const int32 PlayerId = PlayerState->GetPlayerId();
		if (PlayerId == INDEX_NONE)
		{
			continue;
		}

		CurrentPlayerIds.Add(PlayerId);

		const int32 ExistingIndex = FindEntryIndex(PlayerId);
		if (!ActiveEntries.IsValidIndex(ExistingIndex))
		{
			AddTrackedPlayer(PlayerState);
			continue;
		}

		if (ActiveEntries[ExistingIndex].PlayerState.Get() != PlayerState)
		{
			ReleaseEntryAt(ExistingIndex);
			AddTrackedPlayer(PlayerState);
		}
	}

	for (int32 Index = ActiveEntries.Num() - 1; Index >= 0; --Index)
	{
		if (!CurrentPlayerIds.Contains(ActiveEntries[Index].PlayerId))
		{
			ReleaseEntryAt(Index);
		}
	}

	if (ActiveEntries.IsEmpty())
	{
		StopPositionUpdateTimer();
	}
}

// 대상 PlayerState의 월드 상태 UI를 생성하는 함수
void UNSPlayerWorldStatusPresenter::AddTrackedPlayer(ANSPlayerState* PlayerState)
{
	if (!IsValidPlayerStateTarget(PlayerState))
	{
		return;
	}

	UNSPlayerWorldStatusWidget* Widget = AcquireWidget();
	if (!Widget)
	{
		return;
	}

	UNSPlayerWorldStatusViewModel* ViewModel = NewObject<UNSPlayerWorldStatusViewModel>(this);
	if (!ViewModel || !ViewModel->Initialize(PlayerState))
	{
		ReleaseWidget(Widget);
		return;
	}

	Widget->BindViewModel(ViewModel);
	Widget->SetVisibility(ESlateVisibility::Collapsed);

	FNSPlayerWorldStatusEntry NewEntry;
	NewEntry.PlayerId = PlayerState->GetPlayerId();
	NewEntry.PlayerState = PlayerState;
	NewEntry.TargetCharacter = ResolveTargetCharacter(PlayerState);
	NewEntry.Widget = Widget;
	NewEntry.ViewModel = ViewModel;

	ActiveEntries.Add(NewEntry);
	StartPositionUpdateTimer();
	UpdateActiveEntries();
}

// 특정 PlayerId의 활성 Entry를 제거하는 함수
void UNSPlayerWorldStatusPresenter::RemoveTrackedPlayer(int32 PlayerId)
{
	const int32 EntryIndex = FindEntryIndex(PlayerId);
	if (ActiveEntries.IsValidIndex(EntryIndex))
	{
		ReleaseEntryAt(EntryIndex);
	}
}

// 활성 플레이어 위젯들의 위치와 표시 상태를 갱신하는 함수
void UNSPlayerWorldStatusPresenter::UpdateActiveEntries()
{
	UWorld* World = GetPresenterWorld();
	APlayerController* PlayerController = GetOwningPlayerController();
	UCanvasPanel* PlayerCanvas = GetPlayerWorldStatusCanvas();
	if (!World || !PlayerController || !PlayerCanvas)
	{
		ReleaseAllEntries();
		return;
	}

	const float NowSeconds = World->GetTimeSeconds();
	const APawn* LocalPawn = PlayerController->GetPawn();

	for (int32 Index = ActiveEntries.Num() - 1; Index >= 0; --Index)
	{
		FNSPlayerWorldStatusEntry& Entry = ActiveEntries[Index];
		ANSPlayerState* PlayerState = Entry.PlayerState.Get();

		if (!IsValidPlayerStateTarget(PlayerState))
		{
			ReleaseEntryAt(Index);
			continue;
		}

		ANSPlayerCharacterBase* TargetCharacter = Entry.TargetCharacter.Get();
		if (!IsValid(TargetCharacter) || TargetCharacter->GetPlayerState<ANSPlayerState>() != PlayerState)
		{
			TargetCharacter = ResolveTargetCharacter(PlayerState);
			Entry.TargetCharacter = TargetCharacter;
		}

		if (!IsValid(TargetCharacter))
		{
			if (Entry.Widget)
			{
				Entry.Widget->SetVisibility(ESlateVisibility::Collapsed);
			}
			continue;
		}

		if (Entry.ViewModel && !Entry.ViewModel->GetStatus().bVisible)
		{
			if (Entry.Widget)
			{
				Entry.Widget->SetVisibility(ESlateVisibility::Collapsed);
			}
			continue;
		}

		if (LocalPawn)
		{
			const float DistanceSquared = FVector::DistSquared(
				LocalPawn->GetActorLocation(),
				TargetCharacter->GetActorLocation());

			if (DistanceSquared > FMath::Square(MaxDisplayDistance))
			{
				if (Entry.Widget)
				{
					Entry.Widget->SetVisibility(ESlateVisibility::Collapsed);
				}
				continue;
			}
		}

		const FVector AnchorLocation = ResolveAnchorLocation(TargetCharacter);

		if (bUseOcclusionTrace)
		{
			if (NowSeconds - Entry.LastOcclusionCheckTimeSeconds >= OcclusionTraceIntervalSeconds)
			{
				Entry.LastOcclusionCheckTimeSeconds = NowSeconds;
				Entry.bOccluded = IsTargetOccluded(TargetCharacter, AnchorLocation);
			}
		}
		else
		{
			Entry.bOccluded = false;
		}

		if (Entry.bOccluded)
		{
			if (Entry.Widget)
			{
				Entry.Widget->SetVisibility(ESlateVisibility::Collapsed);
			}
			continue;
		}

		FVector2D ScreenPosition;
		const bool bProjected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PlayerController,
			AnchorLocation,
			ScreenPosition,
			true);

		if (!bProjected)
		{
			if (Entry.Widget)
			{
				Entry.Widget->SetVisibility(ESlateVisibility::Collapsed);
			}
			continue;
		}

		if (Entry.Widget)
		{
			Entry.Widget->SetVisibility(ESlateVisibility::HitTestInvisible);

			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Entry.Widget->Slot))
			{
				CanvasSlot->SetPosition(ScreenPosition);
				CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
				CanvasSlot->SetAutoSize(true);
			}
		}
	}

	if (ActiveEntries.IsEmpty())
	{
		StopPositionUpdateTimer();
	}
}

// 플레이어 월드 상태 위젯을 풀에서 얻거나 새로 생성하는 함수
UNSPlayerWorldStatusWidget* UNSPlayerWorldStatusPresenter::AcquireWidget()
{
	UCanvasPanel* PlayerCanvas = GetPlayerWorldStatusCanvas();
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerCanvas || !PlayerController)
	{
		return nullptr;
	}

	UNSPlayerWorldStatusWidget* Widget = nullptr;
	if (!PooledWidgets.IsEmpty())
	{
		Widget = PooledWidgets.Pop();
	}

	if (!Widget)
	{
		if (!ResolveWidgetClass())
		{
			return nullptr;
		}

		Widget = CreateWidget<UNSPlayerWorldStatusWidget>(
			PlayerController,
			PlayerWorldStatusWidgetClass);
	}

	if (!Widget)
	{
		return nullptr;
	}

	if (!Widget->GetParent())
	{
		UCanvasPanelSlot* CanvasSlot = PlayerCanvas->AddChildToCanvas(Widget);
		if (CanvasSlot)
		{
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
			CanvasSlot->SetPosition(FVector2D::ZeroVector);
		}
	}

	return Widget;
}

// 플레이어 월드 상태 위젯을 풀로 반환하는 함수
void UNSPlayerWorldStatusPresenter::ReleaseWidget(UNSPlayerWorldStatusWidget* Widget)
{
	if (!Widget)
	{
		return;
	}

	Widget->UnbindViewModel();
	Widget->SetVisibility(ESlateVisibility::Collapsed);
	Widget->RemoveFromParent();

	PooledWidgets.Add(Widget);
}

// 활성 Entry를 해제하는 함수
void UNSPlayerWorldStatusPresenter::ReleaseEntryAt(int32 EntryIndex)
{
	if (!ActiveEntries.IsValidIndex(EntryIndex))
	{
		return;
	}

	FNSPlayerWorldStatusEntry Entry = ActiveEntries[EntryIndex];

	if (Entry.ViewModel)
	{
		Entry.ViewModel->Shutdown();
	}

	if (Entry.Widget)
	{
		ReleaseWidget(Entry.Widget);
	}

	ActiveEntries.RemoveAt(EntryIndex);
}

// 모든 활성 Entry와 풀 위젯을 정리하는 함수
void UNSPlayerWorldStatusPresenter::ReleaseAllEntries()
{
	for (int32 Index = ActiveEntries.Num() - 1; Index >= 0; --Index)
	{
		ReleaseEntryAt(Index);
	}

	for (UNSPlayerWorldStatusWidget* Widget : PooledWidgets)
	{
		if (Widget)
		{
			Widget->UnbindViewModel();
			Widget->RemoveFromParent();
		}
	}

	PooledWidgets.Reset();
}

// PlayerId에 해당하는 활성 Entry 인덱스를 찾는 함수
int32 UNSPlayerWorldStatusPresenter::FindEntryIndex(int32 PlayerId) const
{
	return ActiveEntries.IndexOfByPredicate(
		[PlayerId](const FNSPlayerWorldStatusEntry& Entry)
		{
			return Entry.PlayerId == PlayerId;
		});
}

// 대상 PlayerState가 플레이어 월드 상태 UI에 표시 가능한지 확인하는 함수
bool UNSPlayerWorldStatusPresenter::IsValidPlayerStateTarget(ANSPlayerState* PlayerState) const
{
	if (!IsValid(PlayerState) || PlayerState->IsDead())
	{
		return false;
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	const APlayerState* LocalPlayerState = PlayerController ? PlayerController->PlayerState : nullptr;
	if (PlayerState == LocalPlayerState)
	{
		return false;
	}

	return IsValid(PlayerState->GetAbilitySystemComponent());
}

// 대상 PlayerState에서 따라갈 캐릭터 Pawn을 찾는 함수
ANSPlayerCharacterBase* UNSPlayerWorldStatusPresenter::ResolveTargetCharacter(ANSPlayerState* PlayerState) const
{
	if (!IsValid(PlayerState))
	{
		return nullptr;
	}

	if (ANSPlayerCharacterBase* Character = Cast<ANSPlayerCharacterBase>(PlayerState->GetPawn()))
	{
		return Character;
	}

	if (const AController* Controller = Cast<AController>(PlayerState->GetOwner()))
	{
		if (ANSPlayerCharacterBase* Character = Cast<ANSPlayerCharacterBase>(Controller->GetPawn()))
		{
			return Character;
		}
	}

	UWorld* World = GetPresenterWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ANSPlayerCharacterBase> It(World); It; ++It)
	{
		ANSPlayerCharacterBase* Character = *It;
		if (IsValid(Character) && Character->GetPlayerState<ANSPlayerState>() == PlayerState)
		{
			return Character;
		}
	}

	return nullptr;
}

// 대상 캐릭터의 UI 기준 월드 위치를 계산하는 함수
FVector UNSPlayerWorldStatusPresenter::ResolveAnchorLocation(const ANSPlayerCharacterBase* TargetCharacter) const
{
	if (!IsValid(TargetCharacter))
	{
		return FVector::ZeroVector;
	}

	if (const USkeletalMeshComponent* MeshComponent = TargetCharacter->GetMesh())
	{
		if (!PlayerStatusSocketName.IsNone() && MeshComponent->DoesSocketExist(PlayerStatusSocketName))
		{
			return MeshComponent->GetSocketLocation(PlayerStatusSocketName) + FVector::UpVector * AnchorVerticalOffset;
		}
	}

	FVector Origin;
	FVector Extent;
	TargetCharacter->GetActorBounds(false, Origin, Extent);

	return Origin + FVector::UpVector * (Extent.Z + AnchorVerticalOffset);
}

// 대상 캐릭터가 벽 뒤에 가려졌는지 검사하는 함수
bool UNSPlayerWorldStatusPresenter::IsTargetOccluded(
	const ANSPlayerCharacterBase* TargetCharacter,
	const FVector& AnchorLocation) const
{
	UWorld* World = GetPresenterWorld();
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!World || !PlayerController || !IsValid(TargetCharacter))
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerWorldStatusOcclusion), false);
	QueryParams.AddIgnoredActor(TargetCharacter);

	if (APawn* PlayerPawn = PlayerController->GetPawn())
	{
		QueryParams.AddIgnoredActor(PlayerPawn);
	}

	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		ViewLocation,
		AnchorLocation,
		ECC_Visibility,
		QueryParams);

	return bHit;
}

// 위치 갱신 Timer를 시작하는 함수
void UNSPlayerWorldStatusPresenter::StartPositionUpdateTimer()
{
	UWorld* World = GetPresenterWorld();
	if (!World || World->GetTimerManager().IsTimerActive(PositionUpdateTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		PositionUpdateTimerHandle,
		this,
		&ThisClass::UpdateActiveEntries,
		PositionUpdateIntervalSeconds,
		true);
}

// 위치 갱신 Timer를 정지하는 함수
void UNSPlayerWorldStatusPresenter::StopPositionUpdateTimer()
{
	if (UWorld* World = GetPresenterWorld())
	{
		World->GetTimerManager().ClearTimer(PositionUpdateTimerHandle);
	}
}

// 팀원 목록 갱신 Timer를 시작하는 함수
void UNSPlayerWorldStatusPresenter::StartRosterRefreshTimer()
{
	UWorld* World = GetPresenterWorld();
	if (!World || World->GetTimerManager().IsTimerActive(RosterRefreshTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		RosterRefreshTimerHandle,
		this,
		&ThisClass::RefreshTrackedPlayers,
		RosterRefreshIntervalSeconds,
		true);
}

// 팀원 목록 갱신 Timer를 정지하는 함수
void UNSPlayerWorldStatusPresenter::StopRosterRefreshTimer()
{
	if (UWorld* World = GetPresenterWorld())
	{
		World->GetTimerManager().ClearTimer(RosterRefreshTimerHandle);
	}
}

// Presenter가 사용할 PlayerController를 반환하는 함수
APlayerController* UNSPlayerWorldStatusPresenter::GetOwningPlayerController() const
{
	const ULocalPlayer* LocalPlayer = OwningLocalPlayer.Get();
	UWorld* World = GetPresenterWorld();

	return LocalPlayer && World
		       ? LocalPlayer->GetPlayerController(World)
		       : nullptr;
}

// Presenter가 사용할 World를 반환하는 함수
UWorld* UNSPlayerWorldStatusPresenter::GetPresenterWorld() const
{
	const ULocalPlayer* LocalPlayer = OwningLocalPlayer.Get();
	return LocalPlayer ? LocalPlayer->GetWorld() : nullptr;
}

// HUD Host에서 플레이어 월드 상태 Canvas Layer를 반환하는 함수
UCanvasPanel* UNSPlayerWorldStatusPresenter::GetPlayerWorldStatusCanvas() const
{
	UObject* HostObject = HUDHostObject.Get();
	if (!HostObject)
	{
		return nullptr;
	}

	const INSPlayerWorldStatusHost* Host = Cast<INSPlayerWorldStatusHost>(HostObject);
	return Host ? Cast<UCanvasPanel>(Host->GetPlayerWorldStatusLayer()) : nullptr;
}

// 플레이어 월드 상태 위젯 클래스를 UIManager 캐시에서 찾는 함수
bool UNSPlayerWorldStatusPresenter::ResolveWidgetClass()
{
	if (PlayerWorldStatusWidgetClass)
	{
		return true;
	}

	const APlayerController* PlayerController = GetOwningPlayerController();
	const UNSUIManagerSubsystem* UIManager =
		PlayerController ? UNSUIManagerSubsystem::Get(PlayerController) : nullptr;
	if (!UIManager)
	{
		return false;
	}

	TSubclassOf<UUserWidget> WidgetClass =
		UIManager->GetCachedWidgetClass(TEXT("PlayerWorldStatus"));
	if (!WidgetClass || !WidgetClass->IsChildOf(UNSPlayerWorldStatusWidget::StaticClass()))
	{
		return false;
	}

	PlayerWorldStatusWidgetClass = *WidgetClass;
	return true;
}
