// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/Core/Waypoint/NSInRunGuideSubsystem.h"

#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Progression/Save/NSPermanentSaveGame.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/UI/NSGuideTextData.h"

FName UNSInRunGuideSubsystem::GetActiveStageRowName() const
{
	const UNSPermanentSaveGame* Save = GetPermanentData();
	if (!Save)
	{
		return NAME_None;
	}

	// 스탯 확인 → 증강 튜토리얼 순서
	if (!Save->bInRunStatGuideDone)
	{
		return TEXT("InRunStat");
	}
	if (!Save->bInRunAugmentGuideDone)
	{
		return TEXT("InRunAugment");
	}
	return NAME_None;
}

void UNSInRunGuideSubsystem::BuildStageEntries(FName RowName, TArray<FNSGuideChecklistEntry>& OutEntries) const
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

	// 인런 단계는 항목을 전부 표시 (InRunStat=1개, InRunAugment=3개)
	OutEntries = Row->Items;
}

void UNSInRunGuideSubsystem::NotifyStatPanelOpened()
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!bGuideStarted || !Save || Save->bInRunStatGuideDone)
	{
		return;
	}

	// 스탯 확인 완료 기록 (이 플래그가 서버 그랜트 1회 게이트 역할도 겸함)
	Save->bInRunStatGuideDone = true;
	SaveGuideState();

	// 스탯 줄 완료 → 다음 단계(증강)는 컨테이너 empty 시 전환
	CompleteItem(TEXT("StatCheck"));

	// 이 시점에 서버 권한으로 일반풀 오퍼 1개 + 리롤 재화 1회 지급
	RequestServerGrant();
}

void UNSInRunGuideSubsystem::NotifyAugmentPanelOpened()
{
	const UNSPermanentSaveGame* Save = GetPermanentData();
	if (!bGuideStarted || !Save || !Save->bInRunStatGuideDone || Save->bInRunAugmentGuideDone)
	{
		return;
	}

	// 증강창 열기 줄 완료
	CompleteItem(TEXT("AugmentOpen"));
}

void UNSInRunGuideSubsystem::NotifyReroll()
{
	const UNSPermanentSaveGame* Save = GetPermanentData();
	if (!bGuideStarted || !Save || !Save->bInRunStatGuideDone || Save->bInRunAugmentGuideDone)
	{
		return;
	}

	// 리롤 줄 완료 (선택 항목 — 안 눌러도 카드 선택으로 단계가 종료됨)
	CompleteItem(TEXT("AugmentReroll"));
}

void UNSInRunGuideSubsystem::NotifyCardSelected()
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!bGuideStarted || !Save || !Save->bInRunStatGuideDone || Save->bInRunAugmentGuideDone)
	{
		return;
	}

	// Phase 2 완료 기록 → 재진입 시 안내/그랜트 없음
	Save->bInRunAugmentGuideDone = true;
	SaveGuideState();

	// 선택 줄 완료 애니메이션이 끝나면(리롤 줄이 남아있어도 무관하게) 컨테이너 전체 숨김
	CompleteItemAndHide(TEXT("AugmentSelect"));
}

void UNSInRunGuideSubsystem::RequestServerGrant()
{
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	APlayerController* PlayerController =
		GameInstance ? GameInstance->GetFirstLocalPlayerController(GetWorld()) : nullptr;
	if (ANSPlayerController* NSPlayerController = Cast<ANSPlayerController>(PlayerController))
	{
		NSPlayerController->RequestTutorialAugmentGrant();
	}
}
