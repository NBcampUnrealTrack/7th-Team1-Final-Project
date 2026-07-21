// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/Core/Waypoint/NSOutRunGuideSubsystem.h"

#include "EngineUtils.h"
#include "NeoSanctum/Interaction/NPC/NSCharacterSelectNPC.h"
#include "NeoSanctum/Interaction/NPC/NSInteractableNPCBase.h"
#include "NeoSanctum/Interaction/Prop/NSReadyStartActor.h"
#include "NeoSanctum/Progression/Save/NSPermanentSaveGame.h"
#include "NeoSanctum/Core/Waypoint/NSWaypointMarkerComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/UI/NSGuideTextData.h"

bool UNSOutRunGuideSubsystem::IsMoveStageComplete() const
{
	const UNSPermanentSaveGame* Save = GetPermanentData();
	if (!Save)
	{
		return false;
	}

	// 구 단일 플래그(기존 세이브)거나, 4방향 세부 플래그가 전부 완료면 이동 단계 완료
	return Save->bMoveGuideDone
		|| (Save->bMoveGuideDone_W && Save->bMoveGuideDone_A
			&& Save->bMoveGuideDone_S && Save->bMoveGuideDone_D);
}

void UNSOutRunGuideSubsystem::NotifyMoveInput(FVector2D Direction)
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!bGuideStarted || !Save || IsMoveStageComplete())
	{
		return;
	}

	/**
	 * 아날로그 드리프트를 걸러내기 위한 임계값
	 * X = 좌(-)/우(+), Y = 후진(-)/전진(+). 대각선이면 두 방향 모두 완료 처리
	 */
	const float Threshold = 0.5f;
	bool bChanged = false;

	// 방향별로 아직 완료 안 된 것만 플래그 세팅 + 해당 줄 완료 애니메이션 재생
	auto TryComplete = [&](bool bActive, bool& Flag, const TCHAR* ItemId)
	{
		if (!bActive || Flag)
		{
			return;
		}
		Flag = true;
		bChanged = true;
		CompleteItem(FName(ItemId));
	};

	TryComplete(Direction.Y > Threshold, Save->bMoveGuideDone_W, TEXT("MoveW"));
	TryComplete(Direction.Y < -Threshold, Save->bMoveGuideDone_S, TEXT("MoveS"));
	TryComplete(Direction.X > Threshold, Save->bMoveGuideDone_D, TEXT("MoveD"));
	TryComplete(Direction.X < -Threshold, Save->bMoveGuideDone_A, TEXT("MoveA"));

	// 단계 전환은 컨테이너가 비워질 때 NotifyChecklistEmptied로 처리 → 여기선 저장만
	if (bChanged)
	{
		SaveGuideState();
	}
}

void UNSOutRunGuideSubsystem::NotifyJumpInput()
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!bGuideStarted || !Save
		|| !IsMoveStageComplete() || Save->bJumpGuideDone)
	{
		return;
	}

	Save->bJumpGuideDone = true;
	SaveGuideState();

	// 점프 줄 완료 애니메이션 재생 (다음 단계는 컨테이너 empty 시 전환)
	CompleteItem(TEXT("Jump"));
}

void UNSOutRunGuideSubsystem::NotifyDashInput()
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!bGuideStarted || !Save
		|| !IsMoveStageComplete() || !Save->bJumpGuideDone || Save->bDashGuideDone)
	{
		return;
	}

	Save->bDashGuideDone = true;
	SaveGuideState();

	CompleteItem(TEXT("Dash"));
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

	// 콘솔 마커를 즉시 끄고(다음 마커 켬), 콘솔 줄 완료 애니메이션 재생
	RefreshVisuals();
	CompleteItem(TEXT("CharacterConsole"));
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

	RefreshVisuals();
	CompleteItem(TEXT("ReadyConsole"));
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

	RefreshVisuals();
	CompleteItem(TEXT("NPC"));
}

void UNSOutRunGuideSubsystem::RefreshVisuals()
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!Save)
	{
		return;
	}

	/**
	 * 단계 우선순위: 이동→점프→대시→캐릭터 콘솔→게임시작 콘솔→NPC.
	 * 마커가 필요한 단계(콘솔/NPC)만 실제로 켜고 끈다.
	 */
	const bool bMoveStageDone = IsMoveStageComplete();
	const bool bJumpStageDone = bMoveStageDone && Save->bJumpGuideDone;
	const bool bDashStageDone = bJumpStageDone && Save->bDashGuideDone;

	const bool bNeedCharacterGuide = bDashStageDone && !Save->bCharacterConsoleGuideDone;
	const bool bCharacterStageDone = bDashStageDone && !bNeedCharacterGuide;

	const bool bNeedReadyGuide = bCharacterStageDone && !Save->bReadyConsoleGuideDone;

	// 캐릭터 선택 콘솔 마커 → "GuideTutorial" 태그가 붙은 인스턴스(게임 시작 옆)만 튜토리얼 대상
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
	}
}

FName UNSOutRunGuideSubsystem::GetActiveStageRowName() const
{
	const UNSPermanentSaveGame* Save = GetPermanentData();
	if (!Save)
	{
		return NAME_None;
	}

	// 이동→점프→대시→캐릭터 콘솔→게임시작 콘솔→NPC 순서로 첫 미완료 단계
	if (!IsMoveStageComplete())
	{
		return TEXT("MoveInput");
	}
	if (!Save->bJumpGuideDone)
	{
		return TEXT("JumpInput");
	}
	if (!Save->bDashGuideDone)
	{
		return TEXT("DashInput");
	}
	if (!Save->bCharacterConsoleGuideDone)
	{
		return TEXT("CharacterSelectConsole");
	}
	if (!Save->bReadyConsoleGuideDone)
	{
		return TEXT("NSReadyStartActor");
	}

	// 해금됐지만 아직 방문 안 한 NPC가 하나라도 있으면 NPC 단계 (Phase 3)
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
		if (Save->UnlockedNPCIds.Contains(NPCId) && !Save->GuidedNPCIds.Contains(NPCId))
		{
			return TEXT("NewNPC");
		}
	}

	return NAME_None;
}

void UNSOutRunGuideSubsystem::BuildStageEntries(FName RowName, TArray<FNSGuideChecklistEntry>& OutEntries) const
{
	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	const UDataTable* GuideTextTable =
		DataSubsystem ? DataSubsystem->GetCommonGuideTextDataTable() : nullptr;
	const FNSGuideTextData* Row =
		GuideTextTable
			? GuideTextTable->FindRow<FNSGuideTextData>(RowName, TEXT("BuildStageEntries"))
			: nullptr;
	if (!Row)
	{
		return;
	}

	// 이동 단계는 아직 완료 안 된 방향만 표시 (HUD 재생성 중간 복원 시 이미 누른 방향은 제외)
	if (RowName == TEXT("MoveInput"))
	{
		const UNSPermanentSaveGame* Save = GetPermanentData();
		if (!Save)
		{
			return;
		}

		for (const FNSGuideChecklistEntry& Entry : Row->Items)
		{
			const bool bDone =
				(Entry.ItemId == TEXT("MoveW") && Save->bMoveGuideDone_W)
				|| (Entry.ItemId == TEXT("MoveA") && Save->bMoveGuideDone_A)
				|| (Entry.ItemId == TEXT("MoveS") && Save->bMoveGuideDone_S)
				|| (Entry.ItemId == TEXT("MoveD") && Save->bMoveGuideDone_D);
			if (!bDone)
			{
				OutEntries.Add(Entry);
			}
		}
		return;
	}

	// 그 외 단계는 항목이 하나뿐이고 단계 자체가 미완료이므로 전부 표시
	OutEntries = Row->Items;
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
