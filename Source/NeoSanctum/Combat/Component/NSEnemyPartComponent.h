// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSEnemyPartComponent.generated.h"

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
 * 클래스 개요 : EnemyData의 Part Row를 기반으로 Enemy의 장착형 무기, 부착 파츠, 메시 일체형 파츠 정보를 관리하는 컴포넌트
 * DT_EnemyParts 기반으로 파츠 Actor 스폰, 부착, PartId/AttackId 기반 소켓 조회를 담당
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

	// PartId와 일치하는 스폰 Actor를 반환하는 함수
	AActor* FindSpawnedPartActor(FName PartId) const;

	// AttackId와 연결된 Part Row 목록을 반환하는 함수
	void GetPartRowsByAttackId(FName AttackId, TArray<const FNSEnemyPartRow*>& OutPartRows) const;

	// 사용 가능한 첫 번째 Muzzle Transform을 반환하는 함수
	bool TryGetAnyMuzzleTransform(FTransform& OutTransform) const;

	// AttackId와 연결된 첫 번째 사용 가능 Muzzle Transform을 반환하는 함수
	bool TryGetMuzzleTransformByAttackId(FName AttackId, FTransform& OutTransform) const;

	// @민재 : AttackId와 연결된 모든 사용 가능 Muzzle Transform을 반환하는 함수
	void GetMuzzleTransformsByAttackId(FName AttackId, TArray<FTransform>& OutTransforms) const;
	
	// AttackId와 연결된 첫 번째 사용 가능 Trace 구간을 반환하는 함수
	bool TryGetTraceSegmentByAttackId(
		FName AttackId,
		float FallbackDistance,
		const FVector& FallbackDirection,
		FVector& OutStart,
		FVector& OutEnd) const;

	// LeftHand IK에 사용할 Transform을 반환하는 함수
	bool TryGetLeftHandIKTransform(FTransform& OutTransform) const;

	// 장착 상태에서도 LeftHand IK를 사용할 파츠가 있는지 반환하는 함수
	bool ShouldUseLeftHandIKWhileEquipped() const;

	// 현재 공격 Row 기준 Muzzle Transform을 반환하고, 없으면 첫 Muzzle을 반환하는 함수
	bool TryGetAimMuzzleTransform(FTransform& OutTransform) const;

	// AttackId와 연결된 스폰 파츠 Actor 목록을 반환하는 함수
	void GetSpawnedPartActorsByAttackId(FName AttackId, TArray<AActor*>& OutActors) const;

private:
	// DT_EnemyParts Row 하나를 기준으로 Actor를 스폰하는 함수
	AActor* SpawnPartActor(const FNSEnemyPartRow& PartRow);

	// 스폰 Actor를 Enemy Mesh에 부착하는 함수
	void AttachPartActor(AActor* PartActor, const FNSEnemyPartRow& PartRow);

	// PartType이 Actor 스폰 대상인지 확인하는 함수
	bool IsSpawnedPartType(const FNSEnemyPartRow& PartRow) const;

	// EnemyPart AttachRule을 엔진 Attachment Rule로 변환하는 함수
	FAttachmentTransformRules MakeAttachmentRules(const FNSEnemyPartRow& PartRow) const;

	// Part Row가 가리키는 Muzzle Transform을 반환하는 함수
	bool TryGetMuzzleTransformFromPartRow(const FNSEnemyPartRow& PartRow, FTransform& OutTransform) const;

	// Actor 내부 SceneComponent에서 소켓 Transform을 찾는 함수
	bool TryGetSocketTransformFromActor(AActor* Actor, FName SocketName, FTransform& OutTransform) const;

	// Owner Enemy Mesh에서 소켓 Transform을 찾는 함수
	bool TryGetSocketTransformFromOwnerMesh(FName SocketName, FTransform& OutTransform) const;

	// Part Row가 가리키는 Trace 구간을 반환하는 함수
	bool TryGetTraceSegmentFromPartRow(
		const FNSEnemyPartRow& PartRow,
		float FallbackDistance,
		const FVector& FallbackDirection,
		FVector& OutStart,
		FVector& OutEnd) const;

	// Part Row와 SocketName으로 스폰 Actor 또는 Owner Mesh에서 소켓 Transform을 찾는 함수
	bool TryGetSocketTransformFromPartRow(
		const FNSEnemyPartRow& PartRow,
		FName SocketName,
		FTransform& OutTransform) const;

	// Part Row가 가리키는 LeftHand IK Transform을 반환하는 함수
	bool TryGetLeftHandIKTransformFromPartRow(
		const FNSEnemyPartRow& PartRow,
		FTransform& OutTransform) const;

	// Spawned Part Actor 내부에서 소켓 Transform을 찾는 함수
	bool TryGetSocketTransformFromSpawnedPart(
		const FNSEnemyPartRow& PartRow,
		FName SocketName,
		FTransform& OutTransform) const;

private:
	// PartId와 함께 보관하는 스폰 파츠 목록
	UPROPERTY(Transient, Replicated)
	TArray<FNSSpawnedEnemyPart> SpawnedParts;
};
