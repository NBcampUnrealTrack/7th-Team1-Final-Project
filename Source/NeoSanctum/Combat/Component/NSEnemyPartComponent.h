// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSEnemyPartComponent.generated.h"

class ANSEnemyWeaponBase;
class UNSEnemyData;
struct FNSEnemyPartRow;

USTRUCT()
struct FNSSpawnedEnemyPart
{
	GENERATED_BODY()

	// 스폰된 파츠를 식별하는 ID
	UPROPERTY()
	FName PartId = NAME_None;

	// PartId에 의해 스폰된 Actor
	UPROPERTY()
	TObjectPtr<AActor> Actor;
};

/*
 * 작성자 : 최준혁
 * 
 * 파일 생성일 : 26.07.03
 * 
 * 클래스 개요 : EnemyData의 Part Row를 기반으로 Enemy의 장착형 무기와 부착 파츠를 스폰하고 관리하는 컴포넌트
 * DefaultWeaponClass fallback을 포함해 기존 일반 몬스터 무기 흐름과 DT_EnemyParts 기반 흐름을 연결
*/
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSEnemyPartComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSEnemyPartComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// EnemyData의 Part Row를 기반으로 파츠 Actor를 스폰하고 부착하는 함수
	UFUNCTION(BlueprintCallable, Category = "Enemy|Part")
	void EquipParts();

	// 스폰된 파츠 Actor를 제거하고 내부 상태를 초기화하는 함수
	UFUNCTION(BlueprintCallable, Category = "Enemy|Part")
	void UnEquipParts();

	// 기존 일반 몬스터 GA와 AnimInstance 호환을 위한 대표 무기 반환 함수
	ANSEnemyWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

	// PartId와 일치하는 스폰 Actor를 반환하는 함수
	AActor* FindSpawnedPartActor(FName PartId) const;

	// AttackId와 연결된 Part Row 목록을 반환하는 함수
	void GetPartRowsByAttackId(FName AttackId, TArray<const FNSEnemyPartRow*>& OutPartRows) const;

protected:
	virtual void BeginPlay() override;

private:
	// Owner가 사망했을 때 스폰된 무기 디졸브를 시작하는 함수
	void HandleOwnerDeathStarted();

	// DT_EnemyParts Row 하나를 기준으로 Actor를 스폰하는 함수
	AActor* SpawnPartActor(const FNSEnemyPartRow& PartRow);

	// DefaultWeaponClass를 사용해 기존 방식의 대표 무기를 스폰하는 함수
	void SpawnFallbackWeapon(UNSEnemyData* EnemyData);

	// 스폰 Actor를 Enemy Mesh에 부착하는 함수
	void AttachPartActor(AActor* PartActor, const FNSEnemyPartRow& PartRow);

	// PartType이 Actor 스폰 대상인지 확인하는 함수
	bool IsSpawnedPartType(const FNSEnemyPartRow& PartRow) const;

	// EnemyPart AttachRule을 엔진 Attachment Rule로 변환하는 함수
	FAttachmentTransformRules MakeAttachmentRules(const FNSEnemyPartRow& PartRow) const;

private:
	// 기존 일반 몬스터 코드와 호환되는 대표 무기 Actor
	UPROPERTY(Transient, Replicated)
	TObjectPtr<ANSEnemyWeaponBase> CurrentWeapon;

	// PartId와 함께 보관하는 스폰 파츠 목록
	UPROPERTY(Transient, Replicated)
	TArray<FNSSpawnedEnemyPart> SpawnedParts;
};
