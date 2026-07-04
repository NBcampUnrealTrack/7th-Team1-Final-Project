// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerStatusBridgeSubsystem.h"
#include "AbilitySystemComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Message.h"
#include "NeoSanctum/Type/NSPlayerStatusMessageTypes.h"

void UNSPlayerStatusBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	//GMS가 브리지보다 먼저 초기화 되도록 보장
	Collection.InitializeDependency<UGameplayMessageSubsystem>();
	
	UGameplayMessageSubsystem& MessageSubsystem =
		UGameplayMessageSubsystem::Get(this);
	
	QueryListenerHandle =
		MessageSubsystem.RegisterListener<
			FNSPlayerStatusQueryMessage>(
				NSGameplayTags::Message_UI_TeammateStatus_Query,
				this,
				&ThisClass::HandleQueryMessage);
}

void UNSPlayerStatusBridgeSubsystem::Deinitialize()
{
	QueryListenerHandle.Unregister();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(
			RosterRefreshTimerHandle);
	}
	UnbindAllPlayers();
	Super::Deinitialize();
}

void UNSPlayerStatusBridgeSubsystem::HandleQueryMessage(FGameplayTag Channel,
	const FNSPlayerStatusQueryMessage& Message)
{
	EnsureTrackingStarted();
	RefreshTrackedPlayers();
	
	FNSPlayerStatusSnapshotMessage Snapshot;
	Snapshot.RequestId = Message.RequestId;
	
	for (const TPair<int32, FNSPlayerStatusBinding>& Pair : TrackedPlayers)
	{
		const ANSPlayerState* PlayerState = Pair.Value.PlayerState.Get();
		
		FNSPlayerStatusViewData ViewData;
		if (BuildViewData(PlayerState, ViewData))
		{
			Snapshot.Players.Add(MoveTemp(ViewData));
		}
	}
	//TMap 순회 순서는 보장되지 않으므로 PlayerId기준으로 정렬
	Snapshot.Players.Sort(
		[](const FNSPlayerStatusViewData& Left, const FNSPlayerStatusViewData& Right)
		{
			return Left.PlayerId < Right.PlayerId;
		});
	
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_TeammateStatus_Snapshot,
		Snapshot);
}

void UNSPlayerStatusBridgeSubsystem::EnsureTrackingStarted()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	FTimerManager& TimerManager =
		World->GetTimerManager();
	if (TimerManager.IsTimerActive(
		RosterRefreshTimerHandle))
	{
		return;
	}
	
	//접속 및 퇴장만 저빈도 타이머로 롹인
	TimerManager.SetTimer(
		RosterRefreshTimerHandle,
		this,
		&ThisClass::RefreshTrackedPlayers,
		0.5f,
		true);
}

void UNSPlayerStatusBridgeSubsystem::RefreshTrackedPlayers()
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = GetGameInstance();
	
	if (!World || !GameInstance)
	{
		return;
	}
	AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		return;
	}

	APlayerController* LocalController =
		GameInstance->GetFirstLocalPlayerController(World);

	const APlayerState* LocalPlayerState =
		LocalController
			? LocalController->PlayerState
			: nullptr;

	TSet<int32> CurrentPlayerIds;

	for (APlayerState* PlayerState
		: GameState->PlayerArray)
	{
		ANSPlayerState* NSPlayerState =
			Cast<ANSPlayerState>(PlayerState);

		if (!IsValid(NSPlayerState) ||
			NSPlayerState == LocalPlayerState)
		{
			continue;
		}

		const int32 PlayerId =
			NSPlayerState->GetPlayerId();

		if (PlayerId == INDEX_NONE)
		{
			continue;
		}

		CurrentPlayerIds.Add(PlayerId);

		FNSPlayerStatusBinding* ExistingBinding =
			TrackedPlayers.Find(PlayerId);

		const bool bNeedsNewBinding =
			!ExistingBinding ||
			ExistingBinding->PlayerState.Get()
				!= NSPlayerState ||
			!ExistingBinding->AbilitySystem.IsValid();

		if (!bNeedsNewBinding)
		{
			continue;
		}

		if (ExistingBinding)
		{
			RemoveTrackedPlayer(PlayerId, false);
		}

		AddTrackedPlayer(NSPlayerState);
	}

	TArray<int32> RemovedPlayerIds;

	for (const TPair<int32, FNSPlayerStatusBinding>& Pair
		: TrackedPlayers)
	{
		if (!CurrentPlayerIds.Contains(Pair.Key))
		{
			RemovedPlayerIds.Add(Pair.Key);
		}
	}

	for (const int32 RemovedPlayerId
		: RemovedPlayerIds)
	{
		RemoveTrackedPlayer(
			RemovedPlayerId,
			true);
	}
}

void UNSPlayerStatusBridgeSubsystem::AddTrackedPlayer(ANSPlayerState* PlayerState)
{
	if (!IsValid(PlayerState))
	{
		return;
	}

	const int32 PlayerId =
		PlayerState->GetPlayerId();

	if (PlayerId == INDEX_NONE)
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystem =
		PlayerState->GetAbilitySystemComponent();

	if (!IsValid(AbilitySystem))
	{
		return;
	}

	FNSPlayerStatusBinding Binding;
	Binding.PlayerState = PlayerState;
	Binding.AbilitySystem = AbilitySystem;

	Binding.HealthChangedHandle =
		AbilitySystem
		->GetGameplayAttributeValueChangeDelegate(
			UNSBaseAttributeSet::GetHealthAttribute())
		.AddUObject(
			this,
			&ThisClass::HandleAttributeChanged,
			PlayerId);

	Binding.MaxHealthChangedHandle =
		AbilitySystem
		->GetGameplayAttributeValueChangeDelegate(
			UNSBaseAttributeSet::GetMaxHealthAttribute())
		.AddUObject(
			this,
			&ThisClass::HandleAttributeChanged,
			PlayerId);

	Binding.ShieldChangedHandle =
		AbilitySystem
		->GetGameplayAttributeValueChangeDelegate(
			UNSPlayerAttributeSet::GetShieldAttribute())
		.AddUObject(
			this,
			&ThisClass::HandleAttributeChanged,
			PlayerId);

	Binding.MaxShieldChangedHandle =
		AbilitySystem
		->GetGameplayAttributeValueChangeDelegate(
			UNSPlayerAttributeSet::GetMaxShieldAttribute())
		.AddUObject(
			this,
			&ThisClass::HandleAttributeChanged,
			PlayerId);

	TrackedPlayers.Add(
		PlayerId,
		MoveTemp(Binding));

	BroadcastPlayerChanged(PlayerId);
}

void UNSPlayerStatusBridgeSubsystem::RemoveTrackedPlayer(int32 PlayerId, bool bBroadcastRemoval)
{
	FNSPlayerStatusBinding* Binding =
	TrackedPlayers.Find(PlayerId);

	if (!Binding)
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystem =
		Binding->AbilitySystem.Get();

	if (IsValid(AbilitySystem))
	{
		if (Binding->HealthChangedHandle.IsValid())
		{
			AbilitySystem
			->GetGameplayAttributeValueChangeDelegate(
				UNSBaseAttributeSet::GetHealthAttribute())
			.Remove(Binding->HealthChangedHandle);
		}

		if (Binding->MaxHealthChangedHandle.IsValid())
		{
			AbilitySystem
			->GetGameplayAttributeValueChangeDelegate(
				UNSBaseAttributeSet::GetMaxHealthAttribute())
			.Remove(Binding->MaxHealthChangedHandle);
		}

		if (Binding->ShieldChangedHandle.IsValid())
		{
			AbilitySystem
			->GetGameplayAttributeValueChangeDelegate(
				UNSPlayerAttributeSet::GetShieldAttribute())
			.Remove(Binding->ShieldChangedHandle);
		}

		if (Binding->MaxShieldChangedHandle.IsValid())
		{
			AbilitySystem
			->GetGameplayAttributeValueChangeDelegate(
				UNSPlayerAttributeSet::GetMaxShieldAttribute())
			.Remove(Binding->MaxShieldChangedHandle);
		}
	}
	TrackedPlayers.Remove(PlayerId);

	if (!bBroadcastRemoval)
	{
		return;
	}

	FNSPlayerStatusChangedMessage ChangedMessage;
	ChangedMessage.StatusData.PlayerId = PlayerId;
	ChangedMessage.bRemoved = true;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_TeammateStatus_Changed,
		ChangedMessage);
}

void UNSPlayerStatusBridgeSubsystem::UnbindAllPlayers()
{
	TArray<int32> PlayerIds;
	TrackedPlayers.GetKeys(PlayerIds);

	for (const int32 PlayerId : PlayerIds)
	{
		RemoveTrackedPlayer(PlayerId, false);
	}

	TrackedPlayers.Reset();
}

void UNSPlayerStatusBridgeSubsystem::HandleAttributeChanged(const FOnAttributeChangeData& ChangeData, int32 PlayerId)
{
	BroadcastPlayerChanged(PlayerId);
}

bool UNSPlayerStatusBridgeSubsystem::BuildViewData(const ANSPlayerState* PlayerState,
	FNSPlayerStatusViewData& OutData) const
{
	if (!IsValid(PlayerState))
	{
		return false;
	}

	const UAbilitySystemComponent* AbilitySystem =
		PlayerState->GetAbilitySystemComponent();

	if (!IsValid(AbilitySystem))
	{
		return false;
	}

	OutData = FNSPlayerStatusViewData();
	OutData.PlayerId = PlayerState->GetPlayerId();
	OutData.PlayerName = PlayerState->GetPlayerName();

	OutData.CurrentHealth =
		AbilitySystem->GetNumericAttribute(
			UNSBaseAttributeSet::GetHealthAttribute());

	OutData.MaxHealth =
		AbilitySystem->GetNumericAttribute(
			UNSBaseAttributeSet::GetMaxHealthAttribute());

	OutData.CurrentShield =
		AbilitySystem->GetNumericAttribute(
			UNSPlayerAttributeSet::GetShieldAttribute());

	OutData.MaxShield =
		AbilitySystem->GetNumericAttribute(
			UNSPlayerAttributeSet::GetMaxShieldAttribute());
	
	OutData.bIsDead = PlayerState->IsDead();

	return OutData.PlayerId != INDEX_NONE;
}

void UNSPlayerStatusBridgeSubsystem::BroadcastPlayerChanged(int32 PlayerId)
{
	const FNSPlayerStatusBinding* Binding =
		TrackedPlayers.Find(PlayerId);

	if (!Binding)
	{
		return;
	}

	FNSPlayerStatusChangedMessage ChangedMessage;

	if (!BuildViewData(
		Binding->PlayerState.Get(),
		ChangedMessage.StatusData))
	{
		return;
	}

	ChangedMessage.bRemoved = false;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_TeammateStatus_Changed,
		ChangedMessage);
}


