// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NSOutRunGuideSubsystem.generated.h"

class UNSPermanentSaveGame;

/**
 * 아웃런(거점) 목표 안내 — 플레이어별 로컬
 * 영구 세이브의 안내 진행 상태를 읽어 대상 액터의 로컬 마커와
 * HUD 우측 상단 안내 텍스트를 켜고 끈다
 */
UCLASS()
class NEOSANCTUM_API UNSOutRunGuideSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 아웃런 진입 시 PlayerController가 호출 —> 세이브 기준으로 안내 시작
	void StartGuide();

	// 캐릭터 선택 콘솔과 첫 상호작용 (NSCharacterSelectNPC::OnInteract에서 호출)
	void NotifyCharacterConsoleUsed();

	// 게임시작 콘솔과 첫 상호작용 (NSReadyStartActor::OnInteract에서 호출)
	void NotifyReadyConsoleUsed();

	// 허브 NPC와 첫 상호작용 (NSPlayerController::OpenInteractionWidget에서 호출)
	void NotifyNPCInteracted(FName NPCId);

	// HUD가 재생성됐을 때 현재 안내 상태 재적용 (NSGuideTextWidget::NativeConstruct에서 호출)
	void RefreshGuideForHUD();

private:
	// 세이브 상태 기준으로 대상 액터 마커/HUD 텍스트 전체 갱신
	void RefreshGuide();

	// 세이브 캐시가 아직 없을 때 로드 완료를 기다렸다가 갱신
	void HandlePermanentDataLoaded(UNSPermanentSaveGame* Data);

	// 안내 진행 상태를 영구 저장 (완료 콜백 불필요)
	void SaveGuideState();

	// 캐시된 영구 세이브 (로딩 전이면 nullptr)
	UNSPermanentSaveGame* GetPermanentData() const;

	// 대상 액터의 마커 컴포넌트를 찾아 로컬 토글 (컴포넌트 없으면 무시)
	static void SetActorMarkerLocal(AActor* TargetActor, bool bActive);

	// 현재 우선순위에 맞는 안내 텍스트를 HUD에 표시 (전부 완료면 숨김)
	void UpdateGuideText(
		bool bNeedCharacterGuide,
		bool bNeedReadyGuide,
		bool bNeedNPCGuide) const;

	// StartGuide 호출 여부 —> 인런 등 다른 레벨에서 Notify가 와도 무시하기 위한 게이트
	bool bGuideStarted = false;

	// 세이브 로드 대기 델리게이트 핸들
	FDelegateHandle DataLoadedHandle;
};
