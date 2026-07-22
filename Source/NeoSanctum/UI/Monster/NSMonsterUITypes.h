// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSMonsterUITypes.generated.h"

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 클래스 개요: 몬스터 상태 UI에서 어떤 요소를 표시할지 정의하는 정책 구조체입니다.
 */
USTRUCT(BlueprintType)
struct FNSMonsterUIDisplayPolicy
{
	GENERATED_BODY()

	// 이름 표시 여부를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI")
	bool bShowName = false;

	// 체력바 표시 여부를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI")
	bool bShowHealth = true;

	// 체력 수치 텍스트 표시 여부를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI")
	bool bShowHealthText = false;

	// 실드바 표시 여부를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI")
	bool bShowShield = false;

	// 실드 수치 텍스트 표시 여부를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI")
	bool bShowShieldText = false;

	// 피격 게이지 표시 여부를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI")
	bool bShowHitGauge = true;

	// 피격 게이지 수치 텍스트 표시 여부를 보관하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI")
	bool bShowHitGaugeText = false;

	// 표시 이름을 강제로 덮어쓸 때 사용하는 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MonsterUI")
	FText OverrideName;
};

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 클래스 개요: 몬스터 상태 UI 위젯에 전달할 표시 데이터를 정의합니다.
 * Widget은 Attribute를 직접 계산하지 않고, 이 구조체의 값만 표시합니다.
 */
USTRUCT(BlueprintType)
struct FNSMonsterUIStatus
{
	GENERATED_BODY()

	// 몬스터 이름 텍스트를 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "MonsterUI")
	FText MonsterName;

	// 체력 진행도를 0~1로 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "MonsterUI")
	float HealthPercent = 1.0f;

	// 실드 진행도를 0~1로 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "MonsterUI")
	float ShieldPercent = 0.0f;

	// 피격 게이지 진행도를 0~1로 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "MonsterUI")
	float HitGaugePercent = 0.0f;

	// 체력 수치 텍스트를 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "MonsterUI")
	FText HealthText;

	// 실드 수치 텍스트를 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "MonsterUI")
	FText ShieldText;

	// 피격 게이지 수치 텍스트를 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "MonsterUI")
	FText HitGaugeText;

	// 이름 표시 여부를 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "MonsterUI")
	bool bShowName = false;

	// 체력바 표시 여부를 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "MonsterUI")
	bool bShowHealth = true;

	// 체력 수치 텍스트 표시 여부를 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "MonsterUI")
	bool bShowHealthText = false;

	// 실드바 표시 여부를 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "MonsterUI")
	bool bShowShield = false;

	// 실드 수치 텍스트 표시 여부를 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "MonsterUI")
	bool bShowShieldText = false;

	// 피격 게이지 표시 여부를 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "MonsterUI")
	bool bShowHitGauge = true;

	// 피격 게이지 수치 텍스트 표시 여부를 보관하는 변수
	UPROPERTY(BlueprintReadOnly, Category = "MonsterUI")
	bool bShowHitGaugeText = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FNSMonsterUIStatusChanged, const FNSMonsterUIStatus&);
