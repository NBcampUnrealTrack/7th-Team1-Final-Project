// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NSBossModeComponent.generated.h"

class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FNSBossModeChanged,
	FGameplayTag, PreviousModeTag,
	FGameplayTag, NewModeTag);

/**
 * 작성자: 최준혁
 * 
 * 파일 생성일: 26.07.02
 * 
 * 클래스 개요: Boss의 현재 전투 Mode를 GameplayTag로 관리하는 컴포넌트입니다.
 * Mode는 서버에서 변경하고, 클라이언트에 복제되며, ASC LooseTag와 동기화됩니다.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSBossModeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSBossModeComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Boss가 사용할 기본 Mode를 현재 Mode로 초기화하고 ASC Tag를 동기화하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Mode")
	void InitializeMode();

	// 서버에서 현재 Boss Mode를 변경하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Mode")
	bool SetMode(FGameplayTag NewModeTag);

	// 서버에서 현재 Boss Mode를 제거하는 함수
	UFUNCTION(BlueprintCallable, Category = "Boss|Mode")
	void ClearMode();

	// 현재 Boss Mode가 지정된 ModeTag와 일치하는지 확인하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Mode")
	bool IsInMode(FGameplayTag ModeTag) const;

	// 현재 Boss Mode가 유효한지 확인하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Mode")
	bool HasMode() const { return CurrentModeTag.IsValid(); }

	// 현재 Boss ModeTag를 반환하는 함수
	UFUNCTION(BlueprintPure, Category = "Boss|Mode")
	FGameplayTag GetCurrentModeTag() const { return CurrentModeTag; }

	// Boss Mode가 변경됐을 때 서버와 클라이언트에서 호출되는 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Boss|Mode")
	FNSBossModeChanged OnBossModeChanged;

protected:
	// Boss가 처음 시작할 때 사용할 기본 ModeTag
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Mode")
	FGameplayTag DefaultModeTag;

	// 현재 Boss ModeTag. 서버에서 관리하고 클라이언트에 복제.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentModeTag, Category = "Boss|Mode")
	FGameplayTag CurrentModeTag;

private:
	UFUNCTION()
	void OnRep_CurrentModeTag(FGameplayTag PreviousModeTag);

	// Mode 변경 이벤트와 ASC Tag 동기화를 한 번에 처리하는 함수
	void HandleModeChanged(FGameplayTag PreviousModeTag, FGameplayTag NewModeTag);

	// 현재 ModeTag를 Owner ASC의 LooseTag로 반영하는 함수
	void SyncASCModeTag(FGameplayTag NewModeTag);

	// Owner가 서버 권한을 가지고 있는지 확인하는 함수
	bool IsOwnerAuthority() const;

	// Owner의 ASC를 반환하는 함수
	UAbilitySystemComponent* GetOwnerASC() const;

private:
	// 이 컴포넌트가 ASC에 마지막으로 적용한 ModeTag
	FGameplayTag AppliedModeTag;
};
