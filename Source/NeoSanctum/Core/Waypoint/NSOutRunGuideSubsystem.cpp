// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/Core/Waypoint/NSOutRunGuideSubsystem.h"

#include "EngineUtils.h"
#include "NeoSanctum/Interaction/NPC/NSCharacterSelectNPC.h"
#include "NeoSanctum/Interaction/NPC/NSInteractableNPCBase.h"
#include "NeoSanctum/Interaction/Prop/NSReadyStartActor.h"
#include "NeoSanctum/Progression/Save/NSPermanentSaveGame.h"
#include "NeoSanctum/System/NSSaveGameSubsystem.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"
#include "NeoSanctum/UI/HUD/NSHUDWidget.h"
#include "NeoSanctum/Core/Waypoint/NSWaypointMarkerComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/UI/NSGuideTextData.h"
#include "TimerManager.h"

void UNSOutRunGuideSubsystem::StartGuide()
{
	bGuideStarted = true;

	// 세이브 캐시가 아직 로드 전이면 로드 완료 시점에 갱신 (중복 구독 방지)
	if (!GetPermanentData())
	{
		UNSSaveGameSubsystem* SaveSubsystem =
			GetWorld()->GetGameInstance()
				? GetWorld()->GetGameInstance()->GetSubsystem<UNSSaveGameSubsystem>()
				: nullptr;
		if (SaveSubsystem && !DataLoadedHandle.IsValid())
		{
			DataLoadedHandle = SaveSubsystem->OnPermanentDataLoaded.AddUObject(
				this, &UNSOutRunGuideSubsystem::HandlePermanentDataLoaded);
		}
		return;
	}

	RefreshGuide();
}

void UNSOutRunGuideSubsystem::CompleteMoveGuide()
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!Save || Save->bMoveGuideDone)
	{
		return;
	}

	Save->bMoveGuideDone = true;
	SaveGuideState();
	RefreshGuide();
}

void UNSOutRunGuideSubsystem::CompleteJumpGuide()
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!Save || Save->bJumpGuideDone)
	{
		return;
	}

	Save->bJumpGuideDone = true;
	SaveGuideState();
	RefreshGuide();
}

void UNSOutRunGuideSubsystem::CompleteDashGuide()
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!Save || Save->bDashGuideDone)
	{
		return;
	}

	Save->bDashGuideDone = true;
	SaveGuideState();
	RefreshGuide();
}

void UNSOutRunGuideSubsystem::NotifyMoveInput()
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!bGuideStarted || !Save || Save->bMoveGuideDone)
	{
		return;
	}

	StartStageTimeout(TEXT("MoveInput"), &UNSOutRunGuideSubsystem::CompleteMoveGuide);
}

void UNSOutRunGuideSubsystem::NotifyJumpInput()
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!bGuideStarted || !Save
		|| !Save->bMoveGuideDone || Save->bJumpGuideDone)
	{
		return;
	}

	StartStageTimeout(TEXT("JumpInput"), &UNSOutRunGuideSubsystem::CompleteJumpGuide);
}

void UNSOutRunGuideSubsystem::NotifyDashInput()
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!bGuideStarted || !Save
		|| !Save->bMoveGuideDone || !Save->bJumpGuideDone || Save->bDashGuideDone)
	{
		return;
	}

	StartStageTimeout(TEXT("DashInput"), &UNSOutRunGuideSubsystem::CompleteDashGuide);
}

void UNSOutRunGuideSubsystem::NotifyCharacterConsoleUsed()
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!bGuideStarted || !Save || Save->bCharacterConsoleGuideDone)
	{
		return;
	}

	Save->bCharacterConsoleGuideDone = true;
	SaveGuideState();
	RefreshGuide();
}

void UNSOutRunGuideSubsystem::NotifyReadyConsoleUsed()
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!bGuideStarted || !Save || Save->bReadyConsoleGuideDone)
	{
		return;
	}

	Save->bReadyConsoleGuideDone = true;
	SaveGuideState();
	RefreshGuide();
}

void UNSOutRunGuideSubsystem::NotifyNPCInteracted(FName NPCId)
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!bGuideStarted || !Save || NPCId.IsNone()
		|| Save->GuidedNPCIds.Contains(NPCId))
	{
		return;
	}

	Save->GuidedNPCIds.Add(NPCId);
	SaveGuideState();
	RefreshGuide();
}

void UNSOutRunGuideSubsystem::RefreshGuideForHUD()
{
	// 안내가 시작된 월드(아웃런)에서만 의미 있음 —> 인런 HUD 생성 시엔 무시
	if (bGuideStarted)
	{
		RefreshGuide();
	}
}

void UNSOutRunGuideSubsystem::RefreshGuide()
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!Save)
	{
		return;
	}

	UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem || !DataSubsystem->IsCommonReady())
	{
		if (DataSubsystem)
		{
			DataSubsystem->OnCommonDataReady.RemoveDynamic(this, &UNSOutRunGuideSubsystem::HandleCommonDataReady);
			DataSubsystem->OnCommonDataReady.AddDynamic(this, &UNSOutRunGuideSubsystem::HandleCommonDataReady);
		}
		return;
	}

	// 안내 필요 여부 판정 —> 이동→점프→대시→캐릭터 콘솔→게임시작 콘솔 순서로 이전 단계가 끝나야 다음 단계가 활성화
	const bool bNeedMoveGuide = !Save->bMoveGuideDone;
	const bool bMoveStageDone = !bNeedMoveGuide;

	const bool bNeedJumpGuide = bMoveStageDone && !Save->bJumpGuideDone;
	const bool bJumpStageDone = bMoveStageDone && !bNeedJumpGuide;

	const bool bNeedDashGuide = bJumpStageDone && !Save->bDashGuideDone;
	const bool bDashStageDone = bJumpStageDone && !bNeedDashGuide;

	const bool bNeedCharacterGuide = bDashStageDone && !Save->bCharacterConsoleGuideDone;
	const bool bCharacterStageDone = bDashStageDone && !bNeedCharacterGuide;

	const bool bNeedReadyGuide = bCharacterStageDone && !Save->bReadyConsoleGuideDone;

	// 캐릭터 선택 콘솔 마커 —> 같은 BP가 여러 곳에 배치되므로, 레벨에서 "GuideTutorial" 태그를 붙인 인스턴스(게임 시작 옆)만 튜토리얼 대상
	for (TActorIterator<ANSCharacterSelectNPC> It(GetWorld()); It; ++It)
	{
		const bool bIsTutorialConsole = It->ActorHasTag(TEXT("GuideTutorial"));
		SetActorMarkerLocal(*It, bNeedCharacterGuide && bIsTutorialConsole);
	}

	// 게임시작 콘솔 마커
	for (TActorIterator<ANSReadyStartActor> It(GetWorld()); It; ++It)
	{
		SetActorMarkerLocal(*It, bNeedReadyGuide);
	}

	// 해금됐지만 아직 첫 방문 안 한 허브 NPC 마커
	bool bNeedNPCGuide = false;
	for (TActorIterator<ANSInteractableNPCBase> It(GetWorld()); It; ++It)
	{
		if (Cast<ANSCharacterSelectNPC>(*It))
		{
			continue;
		}

		const FName NPCId = It->GetNPCId();
		if (NPCId.IsNone())
		{
			continue;
		}

		const bool bNeedGuide = Save->UnlockedNPCIds.Contains(NPCId)
			&& !Save->GuidedNPCIds.Contains(NPCId);
		SetActorMarkerLocal(*It, bNeedGuide);
		bNeedNPCGuide |= bNeedGuide;
	}

	UpdateGuideText(
		bNeedMoveGuide, bNeedJumpGuide, bNeedDashGuide,
		bNeedCharacterGuide, bNeedReadyGuide, bNeedNPCGuide);
}

void UNSOutRunGuideSubsystem::HandleCommonDataReady()
{
	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->OnCommonDataReady.RemoveDynamic(this, &UNSOutRunGuideSubsystem::HandleCommonDataReady);
	}

	if (bGuideStarted)
	{
		RefreshGuide();
	}
}

void UNSOutRunGuideSubsystem::HandlePermanentDataLoaded(UNSPermanentSaveGame* Data)
{
	// 1회성 대기 —> 구독 해제 후 갱신
	if (UNSSaveGameSubsystem* SaveSubsystem =
		GetWorld()->GetGameInstance()
			? GetWorld()->GetGameInstance()->GetSubsystem<UNSSaveGameSubsystem>()
			: nullptr)
	{
		SaveSubsystem->OnPermanentDataLoaded.Remove(DataLoadedHandle);
	}
	DataLoadedHandle.Reset();

	if (bGuideStarted)
	{
		RefreshGuide();
	}
}

void UNSOutRunGuideSubsystem::SaveGuideState()
{
	UNSSaveGameSubsystem* SaveSubsystem =
		GetWorld()->GetGameInstance()
			? GetWorld()->GetGameInstance()->GetSubsystem<UNSSaveGameSubsystem>()
			: nullptr;
	if (!SaveSubsystem)
	{
		return;
	}

	// 캐시를 직접 수정했으므로 그대로 영구 저장
	SaveSubsystem->SavePermanent(
		SaveSubsystem->GetCachedPermanentData(), FNSSaveComplete());
}

UNSPermanentSaveGame* UNSOutRunGuideSubsystem::GetPermanentData() const
{
	const UGameInstance* GameInstance =
		GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UNSSaveGameSubsystem* SaveSubsystem =
		GameInstance ? GameInstance->GetSubsystem<UNSSaveGameSubsystem>() : nullptr;

	return SaveSubsystem ? SaveSubsystem->GetCachedPermanentData() : nullptr;
}

void UNSOutRunGuideSubsystem::SetActorMarkerLocal(AActor* TargetActor, bool bActive)
{
	if (!TargetActor)
	{
		return;
	}

	// 마커 컴포넌트는 BP에서 부착
	if (UNSWaypointMarkerComponent* Marker =
		TargetActor->FindComponentByClass<UNSWaypointMarkerComponent>())
	{
		Marker->SetMarkerActiveLocal(bActive);
	}
}

void UNSOutRunGuideSubsystem::UpdateGuideText(
	bool bNeedMoveGuide,
	bool bNeedJumpGuide,
	bool bNeedDashGuide,
	bool bNeedCharacterGuide,
	bool bNeedReadyGuide,
	bool bNeedNPCGuide)
{
	UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(GetWorld());
	UNSHUDWidget* HUDWidget = UIManager ? UIManager->GetHUDWidget() : nullptr;
	if (!HUDWidget)
	{
		return;
	}

	// 텍스트는 한 줄이므로 우선순위대로 하나만 표시, RowName은 DT_GuideText 기준
	FName RowName = NAME_None;
	if (bNeedMoveGuide)
	{
		RowName = TEXT("MoveInput");
	}
	else if (bNeedJumpGuide)
	{
		RowName = TEXT("JumpInput");
	}
	else if (bNeedDashGuide)
	{
		RowName = TEXT("DashInput");
	}
	else if (bNeedCharacterGuide)
	{
		RowName = TEXT("CharacterSelectConsole");
	}
	else if (bNeedReadyGuide)
	{
		RowName = TEXT("NSReadyStartActor");
	}
	else if (bNeedNPCGuide)
	{
		RowName = TEXT("NewNPC");
	}

	if (RowName.IsNone())
	{
		HUDWidget->HideGuideText();
		return;
	}

	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	const UDataTable* GuideTextTable =
		DataSubsystem ? DataSubsystem->GetCommonGuideTextDataTable() : nullptr;
	const FNSGuideTextData* Row =
		GuideTextTable
			? GuideTextTable->FindRow<FNSGuideTextData>(RowName, TEXT("UpdateGuideText"))
			: nullptr;

	if (!Row)
	{
		HUDWidget->HideGuideText();
		return;
	}

	HUDWidget->ShowGuideText(Row->GuideText);
}

void UNSOutRunGuideSubsystem::StartStageTimeout(FName RowName, void (UNSOutRunGuideSubsystem::* CompleteFunc)())
{
	// 이미 대기 중이면 재시작하지 않음 —> 같은 단계에서 반복 입력이 들어와도 카운트다운 유지
	if (GetWorld()->GetTimerManager().IsTimerActive(GuideTimeoutHandle))
	{
		return;
	}

	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	const UDataTable* GuideTextTable =
		DataSubsystem ? DataSubsystem->GetCommonGuideTextDataTable() : nullptr;
	const FNSGuideTextData* Row =
		GuideTextTable
			? GuideTextTable->FindRow<FNSGuideTextData>(RowName, TEXT("StartStageTimeout"))
			: nullptr;
	const float DelaySeconds = Row ? Row->AutoAdvanceSeconds : 0.0f;

	if (DelaySeconds <= 0.0f)
	{
		(this->*CompleteFunc)();
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		GuideTimeoutHandle,
		FTimerDelegate::CreateUObject(this, CompleteFunc),
		DelaySeconds,
		false);
}
