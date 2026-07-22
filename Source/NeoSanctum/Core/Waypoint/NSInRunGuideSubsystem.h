// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Core/Waypoint/NSGuideSubsystemBase.h"
#include "NSInRunGuideSubsystem.generated.h"

struct FNSGuideChecklistEntry;

/**
 * 인런 안내 — Phase 2(캐릭터 스탯 확인 → 증강 튜토리얼)
 * 인런 최초 진입자에게만 진행. C로 스탯 확인 시 서버 권한으로 일반풀 증강 오퍼 1개 + 리롤 재화를 1회 지급하고,
 * Tab(패널 열기) → (선택)T(리롤) → 1/2/3(카드 선택)로 이어진다. 카드 선택 시 명시적으로 종료.
 */
UCLASS()
class NEOSANCTUM_API UNSInRunGuideSubsystem : public UNSGuideSubsystemBase
{
	GENERATED_BODY()

public:
	// C(캐릭터 스탯/파츠 인벤토리 열기) → 스탯 줄 완료 + 서버 그랜트 1회
	void NotifyStatPanelOpened();

	// Tab(증강 패널 열기) → 증강창 열기 줄 완료
	void NotifyAugmentPanelOpened();

	// T(리롤) → 리롤 줄 완료 (선택 항목 — 단계 완료 조건 아님)
	void NotifyReroll();

	// 1/2/3(카드 선택) → 선택 줄 완료 + Phase 2 명시적 종료
	void NotifyCardSelected();

protected:
	// !bInRunStatGuideDone → InRunStat, !bInRunAugmentGuideDone → InRunAugment, else NAME_None
	virtual FName GetActiveStageRowName() const override;

	// 지정 단계 항목 전체를 DT에서 읽어 채움
	virtual void BuildStageEntries(FName RowName, TArray<FNSGuideChecklistEntry>& OutEntries) const override;

private:
	// 로컬 PC를 찾아 튜토리얼 오퍼/재화 서버 그랜트를 요청 (HasAuthority 분기는 PC가 처리)
	void RequestServerGrant();
};
