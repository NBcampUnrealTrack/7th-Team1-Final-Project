// Copyright 2026 One Team. All rights reserved.

#include "NSNormalMonsterPresenter.h"

#include "AbilitySystemInterface.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/Tag/NSGameplayTags_Message.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"
#include "NSMonsterStatusViewModel.h"
#include "NSMonsterUIHost.h"
#include "NSNormalMonsterStatusWidget.h"

void UNSNormalMonsterPresenter::Initialize(ULocalPlayer* InLocalPlayer)
{
	OwningLocalPlayer = InLocalPlayer;

	RegisterRevealMessageListener();
}

void UNSNormalMonsterPresenter::SetHUDHost(UObject* InHUDHostObject)
{
	if (HUDHostObject.Get() == InHUDHostObject)
	{
		return;
	}

	ReleaseAllEntries();
	HUDHostObject = InHUDHostObject;
}

void UNSNormalMonsterPresenter::Shutdown()
{
	UnregisterRevealMessageListener();
	StopPositionUpdateTimer();
	ReleaseAllEntries();

	HUDHostObject.Reset();
	OwningLocalPlayer.Reset();
	NormalMonsterWidgetClass = nullptr;
}

void UNSNormalMonsterPresenter::HandleRevealMessage(
	FGameplayTag ChannelTag,
	const FNSNormalMonsterRevealMessage& Message)
{
	AActor* TargetActor = Message.Context.TargetActor.Get();
	if (!IsValid(TargetActor) || Message.Context.bTargetDead)
	{
		return;
	}

	RevealTarget(TargetActor);
}

void UNSNormalMonsterPresenter::RevealTarget(AActor* TargetActor)
{
	if (!IsValidNormalMonsterTarget(TargetActor))
	{
		return;
	}

	UWorld* World = GetPresenterWorld();
	if (!World)
	{
		return;
	}

	const float NowSeconds = World->GetTimeSeconds();
	const int32 ExistingIndex = FindEntryIndex(TargetActor);
	if (ActiveEntries.IsValidIndex(ExistingIndex))
	{
		ActiveEntries[ExistingIndex].ExpireTimeSeconds = NowSeconds + RevealDurationSeconds;
		if (ActiveEntries[ExistingIndex].Widget)
		{
			ActiveEntries[ExistingIndex].Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		StartPositionUpdateTimer();
		return;
	}

	if (ActiveEntries.Num() >= MaxVisibleWidgets)
	{
		ReleaseEntryAt(0);
	}

	UNSNormalMonsterStatusWidget* Widget = AcquireWidget();
	if (!Widget)
	{
		return;
	}

	UNSMonsterStatusViewModel* ViewModel = NewObject<UNSMonsterStatusViewModel>(this);
	if (!ViewModel || !ViewModel->Initialize(TargetActor))
	{
		ReleaseWidget(Widget);
		return;
	}

	Widget->BindViewModel(ViewModel);
	Widget->SetVisibility(ESlateVisibility::HitTestInvisible);

	FNSNormalMonsterUIEntry NewEntry;
	NewEntry.TargetActor = TargetActor;
	NewEntry.Widget = Widget;
	NewEntry.ViewModel = ViewModel;
	NewEntry.ExpireTimeSeconds = NowSeconds + RevealDurationSeconds;

	ActiveEntries.Add(NewEntry);
	StartPositionUpdateTimer();
	UpdateActiveEntries();
}

void UNSNormalMonsterPresenter::UpdateActiveEntries()
{
	UWorld* World = GetPresenterWorld();
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!World || !PlayerController)
	{
		ReleaseAllEntries();
		return;
	}

	const float NowSeconds = World->GetTimeSeconds();

	for (int32 Index = ActiveEntries.Num() - 1; Index >= 0; --Index)
	{
		FNSNormalMonsterUIEntry& Entry = ActiveEntries[Index];
		AActor* TargetActor = Entry.TargetActor.Get();

		if (!IsValid(TargetActor) ||
			NowSeconds >= Entry.ExpireTimeSeconds ||
			!IsValidNormalMonsterTarget(TargetActor))
		{
			ReleaseEntryAt(Index);
			continue;
		}

		const APawn* PlayerPawn = PlayerController->GetPawn();
		if (PlayerPawn)
		{
			const float DistanceSquared = FVector::DistSquared(
				PlayerPawn->GetActorLocation(),
				TargetActor->GetActorLocation());

			if (DistanceSquared > FMath::Square(MaxDisplayDistance))
			{
				if (Entry.Widget)
				{
					Entry.Widget->SetVisibility(ESlateVisibility::Collapsed);
				}
				continue;
			}
		}

		const FVector AnchorLocation = ResolveAnchorLocation(TargetActor);

		if (bUseOcclusionTrace &&
			NowSeconds - Entry.LastOcclusionCheckTimeSeconds >= OcclusionTraceIntervalSeconds)
		{
			Entry.LastOcclusionCheckTimeSeconds = NowSeconds;
			Entry.bOccluded = IsTargetOccluded(TargetActor, AnchorLocation);
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

UNSNormalMonsterStatusWidget* UNSNormalMonsterPresenter::AcquireWidget()
{
	UCanvasPanel* NormalCanvas = GetNormalMonsterCanvas();
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!NormalCanvas || !PlayerController)
	{
		return nullptr;
	}

	UNSNormalMonsterStatusWidget* Widget = nullptr;
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

		Widget = CreateWidget<UNSNormalMonsterStatusWidget>(
			PlayerController,
			NormalMonsterWidgetClass);
	}

	if (!Widget)
	{
		return nullptr;
	}

	if (!Widget->GetParent())
	{
		UCanvasPanelSlot* CanvasSlot = NormalCanvas->AddChildToCanvas(Widget);
		if (CanvasSlot)
		{
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
			CanvasSlot->SetPosition(FVector2D::ZeroVector);
		}
	}

	return Widget;
}

void UNSNormalMonsterPresenter::ReleaseWidget(UNSNormalMonsterStatusWidget* Widget)
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

void UNSNormalMonsterPresenter::ReleaseEntryAt(int32 EntryIndex)
{
	if (!ActiveEntries.IsValidIndex(EntryIndex))
	{
		return;
	}

	FNSNormalMonsterUIEntry Entry = ActiveEntries[EntryIndex];

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

void UNSNormalMonsterPresenter::ReleaseAllEntries()
{
	for (int32 Index = ActiveEntries.Num() - 1; Index >= 0; --Index)
	{
		ReleaseEntryAt(Index);
	}

	for (UNSNormalMonsterStatusWidget* Widget : PooledWidgets)
	{
		if (Widget)
		{
			Widget->UnbindViewModel();
			Widget->RemoveFromParent();
		}
	}

	PooledWidgets.Reset();
}

int32 UNSNormalMonsterPresenter::FindEntryIndex(AActor* TargetActor) const
{
	return ActiveEntries.IndexOfByPredicate(
		[TargetActor](const FNSNormalMonsterUIEntry& Entry)
		{
			return Entry.TargetActor.Get() == TargetActor;
		});
}

bool UNSNormalMonsterPresenter::IsValidNormalMonsterTarget(AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(TargetActor);
	if (!EnemyAgent)
	{
		return false;
	}

	const UNSEnemyData* EnemyData = EnemyAgent->GetEnemyData();
	if (EnemyData && EnemyData->EnemyRank == ENSEnemyRank::Boss)
	{
		return false;
	}

	if (!Cast<IAbilitySystemInterface>(TargetActor))
	{
		return false;
	}

	if (const UNSEnemyStateComponent* StateComponent =
		TargetActor->FindComponentByClass<UNSEnemyStateComponent>())
	{
		if (StateComponent->IsDead() || StateComponent->IsInactive())
		{
			return false;
		}
	}

	return true;
}

FVector UNSNormalMonsterPresenter::ResolveAnchorLocation(AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return FVector::ZeroVector;
	}

	if (const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(TargetActor))
	{
		return EnemyAgent->GetAimLocation() + FVector::UpVector * AnchorVerticalOffset;
	}

	FVector Origin;
	FVector Extent;
	TargetActor->GetActorBounds(false, Origin, Extent);

	return Origin + FVector::UpVector * (Extent.Z + AnchorVerticalOffset);
}

bool UNSNormalMonsterPresenter::IsTargetOccluded(
	AActor* TargetActor,
	const FVector& AnchorLocation) const
{
	UWorld* World = GetPresenterWorld();
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!World || !PlayerController || !IsValid(TargetActor))
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NormalMonsterUIOcclusion), false);
	QueryParams.AddIgnoredActor(TargetActor);

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

void UNSNormalMonsterPresenter::StartPositionUpdateTimer()
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

void UNSNormalMonsterPresenter::StopPositionUpdateTimer()
{
	if (UWorld* World = GetPresenterWorld())
	{
		World->GetTimerManager().ClearTimer(PositionUpdateTimerHandle);
	}
}

void UNSNormalMonsterPresenter::RegisterRevealMessageListener()
{
	UWorld* World = GetPresenterWorld();
	if (!World || RevealMessageListenerHandle.IsValid())
	{
		return;
	}

	RevealMessageListenerHandle =
		UGameplayMessageSubsystem::Get(World).RegisterListener<FNSNormalMonsterRevealMessage>(
			NSGameplayTags::Message_UI_NormalMonster_Reveal,
			this,
			&ThisClass::HandleRevealMessage);
}

void UNSNormalMonsterPresenter::UnregisterRevealMessageListener()
{
	if (RevealMessageListenerHandle.IsValid())
	{
		RevealMessageListenerHandle.Unregister();
	}
}

APlayerController* UNSNormalMonsterPresenter::GetOwningPlayerController() const
{
	const ULocalPlayer* LocalPlayer = OwningLocalPlayer.Get();
	UWorld* World = GetPresenterWorld();

	return LocalPlayer && World
		       ? LocalPlayer->GetPlayerController(World)
		       : nullptr;
}

UWorld* UNSNormalMonsterPresenter::GetPresenterWorld() const
{
	const ULocalPlayer* LocalPlayer = OwningLocalPlayer.Get();
	return LocalPlayer ? LocalPlayer->GetWorld() : nullptr;
}

UCanvasPanel* UNSNormalMonsterPresenter::GetNormalMonsterCanvas() const
{
	UObject* HostObject = HUDHostObject.Get();
	if (!HostObject)
	{
		return nullptr;
	}

	const INSMonsterUIHost* Host = Cast<INSMonsterUIHost>(HostObject);
	return Host ? Cast<UCanvasPanel>(Host->GetNormalMonsterLayer()) : nullptr;
}

bool UNSNormalMonsterPresenter::ResolveWidgetClass()
{
	if (NormalMonsterWidgetClass)
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
		UIManager->GetCachedWidgetClass(TEXT("NormalMonsterStatus"));
	if (!WidgetClass || !WidgetClass->IsChildOf(UNSNormalMonsterStatusWidget::StaticClass()))
	{
		return false;
	}

	NormalMonsterWidgetClass = *WidgetClass;
	return true;
}
