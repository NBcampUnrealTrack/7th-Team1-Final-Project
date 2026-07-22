// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Core/Waypoint/NSGuideSubsystemBase.h"
#include "NSOutRunGuideSubsystem.generated.h"

class UNSPermanentSaveGame;
struct FNSGuideChecklistEntry;

/**
 * 아웃런(거점) 안내 — Phase 1(조작법/콘솔) + Phase 3(NPC 귀환)
 * 세이브 상태 기준으로 단계를 순서대로 진행하며 대상 액터 로컬 마커와 HUD 체크리스트를 켜고 끈다
 */
UCLASS()
class NEOSANCTUM_API UNSOutRunGuideSubsystem : public UNSGuideSubsystemBase
{
	GENERATED_BODY()

public:
	// 이동 입력 발동 (NSInputBinderComponent::Input_Move에서 방향값과 함께 호출) → 해당 방향(W/A/S/D) 줄 완료
	void NotifyMoveInput(FVector2D Direction);

	// 점프 입력 발동 → 즉시 점프 줄 완료
	void NotifyJumpInput();

	// 대시 어빌리티 입력 발동 → 즉시 대시 줄 완료
	void NotifyDashInput();

	// 캐릭터 선택 콘솔과 첫 상호작용
	void NotifyCharacterConsoleUsed();

	// 게임시작 콘솔과 첫 상호작용
	void NotifyReadyConsoleUsed();

	// 허브 NPC와 첫 상호작용 (Phase 3)
	void NotifyNPCInteracted(FName NPCId);

protected:
	// 세이브 상태로 현재 활성 단계 RowName 반환 (이동→점프→대시→캐릭터콘솔→시작콘솔→NewNPC)
	virtual FName GetActiveStageRowName() const override;

	// 지정 단계 항목 채움 (MoveInput은 미완료 방향만, 나머지는 전체)
	virtual void BuildStageEntries(FName RowName, TArray<FNSGuideChecklistEntry>& OutEntries) const override;

	// 콘솔/NPC 마커 로컬 토글
	virtual void RefreshVisuals() override;

private:
	// 이동 단계 완료 여부 (구 플래그 OR 4방향 세부 플래그 전부)
	bool IsMoveStageComplete() const;

	// 대상 액터의 마커 컴포넌트를 찾아 로컬 토글 (컴포넌트 없으면 무시)
	static void SetActorMarkerLocal(AActor* TargetActor, bool bActive);
};
