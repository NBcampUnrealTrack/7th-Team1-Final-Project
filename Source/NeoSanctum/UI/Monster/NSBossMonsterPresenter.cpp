// Copyright 2026 One Team. All rights reserved.

#include "NSBossMonsterPresenter.h"

#include "AbilitySystemInterface.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/AI/Enemy/Interface/NSEnemyAgent.h"
#include "NeoSanctum/Combat/Component/NSEnemyStateComponent.h"
#include "NeoSanctum/Core/GameFlow/NSRunFlowType.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"
#include "NSBossMonsterStatusWidget.h"
#include "NSMonsterStatusViewModel.h"
#include "NSMonsterUIHost.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/UI/NSMonsterUIData.h"

void UNSBossMonsterPresenter::Initialize(ULocalPlayer* InLocalPlayer)
{
	OwningLocalPlayer = InLocalPlayer;

	BindRunGameState();
	HandleStagePhaseChanged();
}

void UNSBossMonsterPresenter::SetHUDHost(UObject* InHUDHostObject)
{
	if (HUDHostObject.Get() == InHUDHostObject)
	{
		return;
	}

	ClearBosses();
	HUDHostObject = InHUDHostObject;
	HandleStagePhaseChanged();
}

void UNSBossMonsterPresenter::Shutdown()
{
	StopBossDiscovery();
	UnbindRunGameState();
	ClearBosses();

	HUDHostObject.Reset();
	OwningLocalPlayer.Reset();
	BossMonsterWidgetClass = nullptr;
}

void UNSBossMonsterPresenter::HandleStagePhaseChanged()
{
	BindRunGameState();

	const ANSRunGameState* RunGameState = BoundRunGameState.Get();
	if (!RunGameState || RunGameState->StagePhase != ENSStagePhase::BossFight)
	{
		StopBossDiscovery();
		ClearBosses();
		return;
	}

	StartBossDiscovery();
	RefreshBosses();
}

void UNSBossMonsterPresenter::BindRunGameState()
{
	UWorld* World = GetPresenterWorld();
	if (!World)
	{
		return;
	}

	ANSRunGameState* RunGameState = World->GetGameState<ANSRunGameState>();
	if (!RunGameState || BoundRunGameState.Get() == RunGameState)
	{
		return;
	}

	UnbindRunGameState();

	BoundRunGameState = RunGameState;
	RunGameState->OnStagePhaseChanged.AddDynamic(
		this,
		&ThisClass::HandleStagePhaseChanged);
}

void UNSBossMonsterPresenter::UnbindRunGameState()
{
	if (ANSRunGameState* RunGameState = BoundRunGameState.Get())
	{
		RunGameState->OnStagePhaseChanged.RemoveDynamic(
			this,
			&ThisClass::HandleStagePhaseChanged);
	}

	BoundRunGameState.Reset();
}

void UNSBossMonsterPresenter::StartBossDiscovery()
{
	UWorld* World = GetPresenterWorld();
	if (!World || World->GetTimerManager().IsTimerActive(BossDiscoveryTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		BossDiscoveryTimerHandle,
		this,
		&ThisClass::RefreshBosses,
		BossDiscoveryIntervalSeconds,
		true);
}

void UNSBossMonsterPresenter::StopBossDiscovery()
{
	if (UWorld* World = GetPresenterWorld())
	{
		World->GetTimerManager().ClearTimer(BossDiscoveryTimerHandle);
	}
}

void UNSBossMonsterPresenter::RefreshBosses()
{
	UWorld* World = GetPresenterWorld();
	if (!World || !GetBossMonsterBox())
	{
		return;
	}

	for (int32 Index = ActiveBossEntries.Num() - 1; Index >= 0; --Index)
	{
		AActor* BossActor = ActiveBossEntries[Index].BossActor.Get();
		if (!IsValidBossTarget(BossActor))
		{
			RemoveBossAt(Index);
		}
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValidBossTarget(Actor))
		{
			continue;
		}

		if (FindBossEntryIndex(Actor) == INDEX_NONE)
		{
			AddBoss(Actor);
		}
	}
}

void UNSBossMonsterPresenter::AddBoss(AActor* BossActor)
{
	if (!IsValidBossTarget(BossActor) || !ResolveWidgetClass())
	{
		return;
	}

	UHorizontalBox* BossBox = GetBossMonsterBox();
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!BossBox || !PlayerController)
	{
		return;
	}

	UNSBossMonsterStatusWidget* Widget = CreateWidget<UNSBossMonsterStatusWidget>(
		PlayerController,
		BossMonsterWidgetClass);
	if (!Widget)
	{
		return;
	}

	const FNSMonsterUIData* ProfileRow = FindMonsterUIData(BossActor);
	const FNSMonsterUIDisplayPolicy BossPolicy = BuildBossDisplayPolicy(ProfileRow);

	UNSMonsterStatusViewModel* ViewModel = NewObject<UNSMonsterStatusViewModel>(this);
	if (!ViewModel || !ViewModel->Initialize(BossActor, BossPolicy))
	{
		return;
	}

	Widget->BindViewModel(ViewModel);
	Widget->SetVisibility(ESlateVisibility::HitTestInvisible);

	UHorizontalBoxSlot* BossSlot = BossBox->AddChildToHorizontalBox(Widget);
	if (BossSlot)
	{
		BossSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BossSlot->SetHorizontalAlignment(HAlign_Fill);
		BossSlot->SetVerticalAlignment(VAlign_Top);
		BossSlot->SetPadding(FMargin(8.0f, 0.0f));
	}

	FNSBossMonsterUIEntry NewEntry;
	NewEntry.BossActor = BossActor;
	NewEntry.Widget = Widget;
	NewEntry.ViewModel = ViewModel;

	ActiveBossEntries.Add(NewEntry);
}

void UNSBossMonsterPresenter::RemoveBossAt(int32 EntryIndex)
{
	if (!ActiveBossEntries.IsValidIndex(EntryIndex))
	{
		return;
	}

	FNSBossMonsterUIEntry Entry = ActiveBossEntries[EntryIndex];

	if (Entry.ViewModel)
	{
		Entry.ViewModel->Shutdown();
	}

	if (Entry.Widget)
	{
		Entry.Widget->UnbindViewModel();
		Entry.Widget->RemoveFromParent();
	}

	ActiveBossEntries.RemoveAt(EntryIndex);
}

void UNSBossMonsterPresenter::ClearBosses()
{
	for (int32 Index = ActiveBossEntries.Num() - 1; Index >= 0; --Index)
	{
		RemoveBossAt(Index);
	}
}

int32 UNSBossMonsterPresenter::FindBossEntryIndex(AActor* BossActor) const
{
	return ActiveBossEntries.IndexOfByPredicate(
		[BossActor](const FNSBossMonsterUIEntry& Entry)
		{
			return Entry.BossActor.Get() == BossActor;
		});
}

bool UNSBossMonsterPresenter::IsValidBossTarget(AActor* BossActor) const
{
	if (!IsValid(BossActor))
	{
		return false;
	}

	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(BossActor);
	if (!EnemyAgent)
	{
		return false;
	}

	const UNSEnemyData* EnemyData = EnemyAgent->GetEnemyData();
	if (!EnemyData || EnemyData->EnemyRank != ENSEnemyRank::Boss)
	{
		return false;
	}

	if (!Cast<IAbilitySystemInterface>(BossActor))
	{
		return false;
	}

	if (const UNSEnemyStateComponent* StateComponent =
		BossActor->FindComponentByClass<UNSEnemyStateComponent>())
	{
		if (StateComponent->IsDead() || StateComponent->IsInactive())
		{
			return false;
		}
	}

	return true;
}

bool UNSBossMonsterPresenter::ResolveWidgetClass()
{
	if (BossMonsterWidgetClass)
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
		UIManager->GetCachedWidgetClass(TEXT("BossMonsterStatus"));
	if (!WidgetClass || !WidgetClass->IsChildOf(UNSBossMonsterStatusWidget::StaticClass()))
	{
		return false;
	}

	BossMonsterWidgetClass = *WidgetClass;
	return true;
}

APlayerController* UNSBossMonsterPresenter::GetOwningPlayerController() const
{
	const ULocalPlayer* LocalPlayer = OwningLocalPlayer.Get();
	UWorld* World = GetPresenterWorld();

	return LocalPlayer && World
		       ? LocalPlayer->GetPlayerController(World)
		       : nullptr;
}

UWorld* UNSBossMonsterPresenter::GetPresenterWorld() const
{
	const ULocalPlayer* LocalPlayer = OwningLocalPlayer.Get();
	return LocalPlayer ? LocalPlayer->GetWorld() : nullptr;
}

UHorizontalBox* UNSBossMonsterPresenter::GetBossMonsterBox() const
{
	UObject* HostObject = HUDHostObject.Get();
	if (!HostObject)
	{
		return nullptr;
	}

	const INSMonsterUIHost* Host = Cast<INSMonsterUIHost>(HostObject);
	return Host ? Host->GetBossMonsterLayer() : nullptr;
}

const FNSMonsterUIData* UNSBossMonsterPresenter::FindMonsterUIData(AActor* BossActor) const
{
	const INSEnemyAgent* EnemyAgent = Cast<INSEnemyAgent>(BossActor);
	const UNSEnemyData* EnemyData = EnemyAgent ? EnemyAgent->GetEnemyData() : nullptr;
	if (!EnemyData || !EnemyData->EnemyId.IsValid())
	{
		return nullptr;
	}

	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(BossActor);
	return DataSubsystem ? DataSubsystem->FindMonsterUIData(EnemyData->EnemyId) : nullptr;
}

FNSMonsterUIDisplayPolicy UNSBossMonsterPresenter::BuildBossDisplayPolicy(const FNSMonsterUIData* ProfileRow) const
{
	FNSMonsterUIDisplayPolicy Policy;
	Policy.bShowName = true;
	Policy.bShowHealth = true;
	Policy.bShowHealthText = true;
	Policy.bShowShield = true;
	Policy.bShowShieldText = true;
	Policy.bShowHitGauge = true;
	Policy.bShowHitGaugeText = true;

	if (!ProfileRow)
	{
		return Policy;
	}

	Policy.bShowName = ProfileRow->bShowName;
	Policy.bShowHealth = ProfileRow->bShowHealth;
	Policy.bShowHealthText = ProfileRow->bShowHealthText;
	Policy.bShowShield = ProfileRow->bShowShield;
	Policy.bShowShieldText = ProfileRow->bShowShieldText;
	Policy.bShowHitGauge = ProfileRow->bShowHitGauge;
	Policy.bShowHitGaugeText = ProfileRow->bShowHitGaugeText;
	Policy.OverrideName = ProfileRow->DisplayName;

	return Policy;
}
