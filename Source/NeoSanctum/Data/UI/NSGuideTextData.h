// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NSGuideTextData.generated.h"

/**
 * 체크리스트 한 줄에 대응하는 안내 항목 1개
 * ItemId는 코드의 하드코딩 상수(MoveW/MoveA/MoveS/MoveD/Jump/Dash/CharacterConsole/ReadyConsole/NPC)와
 * 반드시 일치해야 한다 — 서브시스템이 이 ItemId로 해당 줄의 완료 애니메이션을 재생하기 때문
 */
USTRUCT(BlueprintType)
struct FNSGuideChecklistEntry
{
	GENERATED_BODY()

	// 완료 처리 시 어느 줄인지 식별하는 키 (코드 상수와 일치 필수)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FName ItemId;

	// 이 줄에 표시할 안내 문구
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FText Text;
};

/**
 * 아웃런 목표 안내 한 단계(RowName)에 표시할 체크리스트 항목들
 * MoveInput 행만 4개(W/A/S/D) 엔트리, 나머지 행은 1개 엔트리
 */
USTRUCT(BlueprintType)
struct FNSGuideTextData : public FTableRowBase
{
	GENERATED_BODY()

	// 이 단계에서 체크리스트로 표시할 항목 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TArray<FNSGuideChecklistEntry> Items;
};
