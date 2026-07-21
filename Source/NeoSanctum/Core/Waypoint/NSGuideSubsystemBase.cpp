// Copyright 2026 One Team. All rights reserved.

#include "NeoSanctum/Core/Waypoint/NSGuideSubsystemBase.h"

#include "NeoSanctum/Progression/Save/NSPermanentSaveGame.h"
#include "NeoSanctum/System/NSSaveGameSubsystem.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"
#include "NeoSanctum/UI/HUD/NSHUDWidget.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Data/UI/NSGuideTextData.h"

void UNSGuideSubsystemBase::StartGuide()
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
				this, &UNSGuideSubsystemBase::HandlePermanentDataLoaded);
		}
		return;
	}

	RefreshGuide();
}

void UNSGuideSubsystemBase::NotifyChecklistEmptied()
{
	if (!bGuideStarted)
	{
		return;
	}

	/**
	 * 컨테이너가 비워졌으니 현재 단계 표시를 초기화하고 재평가한다
	 * → 다음 단계가 있으면 표시, 없으면 숨김. 같은 단계 반복(다른 NPC 등)도 재평가되도록 NAME_None으로 강제.
	 */
	CurrentChecklistStage = NAME_None;
	RefreshGuide();
}

void UNSGuideSubsystemBase::RefreshGuideForHUD()
{
	// 안내가 시작된 월드에서만 의미 있음 → 다른 월드 HUD 생성 시엔 무시
	if (!bGuideStarted)
	{
		return;
	}

	// 새 HUD 위젯은 비어 있으므로 현재 단계를 강제로 다시 스폰
	CurrentChecklistStage = NAME_None;
	RefreshGuide();
}

void UNSGuideSubsystemBase::RefreshGuide()
{
	UNSPermanentSaveGame* Save = GetPermanentData();
	if (!Save)
	{
		return;
	}

	// 안내 문구 DataTable(공용)이 아직 준비 전이면 준비 완료 시점에 재시도
	UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem || !DataSubsystem->IsCommonReady())
	{
		if (DataSubsystem)
		{
			DataSubsystem->OnCommonDataReady.RemoveDynamic(this, &UNSGuideSubsystemBase::HandleCommonDataReady);
			DataSubsystem->OnCommonDataReady.AddDynamic(this, &UNSGuideSubsystemBase::HandleCommonDataReady);
		}
		return;
	}

	RefreshVisuals();
	UpdateGuideChecklist();
}

void UNSGuideSubsystemBase::UpdateGuideChecklist()
{
	UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(GetWorld());
	UNSHUDWidget* HUDWidget = UIManager ? UIManager->GetHUDWidget() : nullptr;
	if (!HUDWidget)
	{
		// HUD가 아직 없으면 CurrentChecklistStage를 갱신하지 않고 다음 기회에 재시도
		return;
	}

	const FName ActiveRow = GetActiveStageRowName();

	/**
	 * 진행할 단계가 없으면 항상 숨김을 호출한다 (재평가 강제를 위해 CurrentChecklistStage를
	 * NAME_None으로 초기화해두는 NotifyChecklistEmptied/RefreshGuideForHUD와, "더 이상 단계 없음"을
	 * 뜻하는 ActiveRow.IsNone()이 같은 NAME_None 값이라 아래의 "같은 단계면 재스폰 안 함" 가드에 걸려
	 * HideGuideChecklist가 한 번도 안 불리는 문제가 있었다 — HideChecklist는 멱등이라 중복 호출해도 안전)
	 */
	if (ActiveRow.IsNone())
	{
		CurrentChecklistStage = NAME_None;
		HUDWidget->HideGuideChecklist();
		return;
	}

	// 이미 같은 단계를 표시 중이면 재스폰하지 않음 (개별 완료는 CompleteItem이 처리)
	if (ActiveRow == CurrentChecklistStage)
	{
		return;
	}
	CurrentChecklistStage = ActiveRow;

	TArray<FNSGuideChecklistEntry> Entries;
	BuildStageEntries(ActiveRow, Entries);
	HUDWidget->ShowGuideChecklist(Entries);
}

void UNSGuideSubsystemBase::CompleteItem(FName ItemId)
{
	// HUD가 없으면 안내 기능 없이 동작 (조용히 무시)
	if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(GetWorld()))
	{
		if (UNSHUDWidget* HUDWidget = UIManager->GetHUDWidget())
		{
			HUDWidget->CompleteGuideChecklistItem(ItemId);
		}
	}
}

void UNSGuideSubsystemBase::CompleteItemAndHide(FName ItemId)
{
	/**
	 * 명시적 완료: 이 항목의 완료 애니메이션이 끝나는 시점(위젯의 HandleItemCompleteFinished)에
	 * 다른 미완료 항목(예: Phase 2의 선택적 리롤)과 무관하게 컨테이너 전체가 숨겨진다
	 */
	CurrentChecklistStage = NAME_None;
	if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(GetWorld()))
	{
		if (UNSHUDWidget* HUDWidget = UIManager->GetHUDWidget())
		{
			HUDWidget->CompleteGuideChecklistItemAndHide(ItemId);
		}
	}
}

void UNSGuideSubsystemBase::HandleCommonDataReady()
{
	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->OnCommonDataReady.RemoveDynamic(this, &UNSGuideSubsystemBase::HandleCommonDataReady);
	}

	if (bGuideStarted)
	{
		RefreshGuide();
	}
}

void UNSGuideSubsystemBase::HandlePermanentDataLoaded(UNSPermanentSaveGame* Data)
{
	// 1회성 대기 → 구독 해제 후 갱신
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

void UNSGuideSubsystemBase::SaveGuideState()
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

UNSPermanentSaveGame* UNSGuideSubsystemBase::GetPermanentData() const
{
	const UGameInstance* GameInstance =
		GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UNSSaveGameSubsystem* SaveSubsystem =
		GameInstance ? GameInstance->GetSubsystem<UNSSaveGameSubsystem>() : nullptr;

	return SaveSubsystem ? SaveSubsystem->GetCachedPermanentData() : nullptr;
}
