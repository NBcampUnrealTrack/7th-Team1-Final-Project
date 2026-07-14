// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NSPlayerWorldStatusTypes.h"
#include "NSPlayerWorldStatusViewModel.generated.h"

class ANSPlayerState;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;

/**
 * 작성자: 최준혁
 *
 * 파일 생성일: 26.07.13
 *
 * 클래스 개요: 플레이어 ASC를 관찰해 월드 상태 UI 표시용 상태값을 계산하는 ViewModel입니다.
 * Widget에는 Current/Max가 아닌 Percent와 표시 가능 여부만 전달합니다.
 */
UCLASS()
class NEOSANCTUM_API UNSPlayerWorldStatusViewModel : public UObject
{
	GENERATED_BODY()

public:
	// 대상 PlayerState와 ASC를 연결하는 함수
	bool Initialize(ANSPlayerState* InPlayerState);

	// Attribute Delegate 구독을 해제하고 참조를 정리하는 함수
	void Shutdown();

	// 현재 UI 상태값을 반환하는 함수
	const FNSPlayerWorldStatusData& GetStatus() const { return CachedStatus; }

	// UI 상태 변경 시 Widget에 알리는 델리게이트 변수
	FNSPlayerWorldStatusChanged OnStatusChanged;

private:
	// 현재 Attribute 값을 읽어 UI 상태값을 갱신하는 함수
	void RefreshStatus();

	// Attribute 변경 Delegate를 구독하는 함수
	void BindAttributeDelegates();

	// Attribute 변경 Delegate 구독을 해제하는 함수
	void UnbindAttributeDelegates();

	// Attribute 변경 시 UI 상태값을 갱신하는 함수
	void HandleAttributeChanged(const FOnAttributeChangeData& Data);

private:
	// 상태 표시 대상 PlayerState를 약하게 보관하는 변수
	TWeakObjectPtr<ANSPlayerState> TargetPlayerState;

	// 상태 표시 대상 ASC를 약하게 보관하는 변수
	TWeakObjectPtr<UAbilitySystemComponent> TargetASC;

	// Widget에 전달할 캐시된 상태값 변수
	FNSPlayerWorldStatusData CachedStatus;

	// Health 변경 Delegate Handle 변수
	FDelegateHandle HealthChangedHandle;

	// MaxHealth 변경 Delegate Handle 변수
	FDelegateHandle MaxHealthChangedHandle;
};
