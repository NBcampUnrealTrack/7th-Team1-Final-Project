// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSEntranceGate.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

/**
* 거점 입장 게이트(문).
* 특정 NPC 구출 진행도에 따라 플레이어별로 통과 가능/불가가 갈린다.
* - 문은 서버에 1개만 존재하며 복제하지 않는다.
* - 외형(잠금/해금)은 각 클라이언트가 본인 진행도 기준으로 로컬에서만 머티리얼을 교체한다.
* - 통과 차단은 기본 잠금(Player 채널 Block)이며, 해금한 폰만 IgnoreActorWhenMoving으로 통과한다.
*   (통과 처리는 UNSGateAccessComponent가 담당)
*/
UCLASS()
class NEOSANCTUM_API ANSEntranceGate : public AActor
{
	GENERATED_BODY()

public:
	ANSEntranceGate();

	// 이 문을 여는 구출 대상 NPC ID
	FName GetRequiredNPCId() const { return RequiredNPCId; }

	// 로컬 시각만 교체(해금=Unlock 머티리얼 / 잠금=Lock 머티리얼). 복제 없음.
	void SetLocalUnlockVisual(bool bUnlocked);

protected:
	virtual void BeginPlay() override;

private:
	// GateMesh의 모든 슬롯 머티리얼을 지정 머티리얼로 교체 (null이면 무시)
	void ApplyMaterial(UMaterialInterface* Material);

	// 로컬(이 화면의) 플레이어 진행도 기준 해금 여부. 진행도 미준비 시 false(잠금) 폴백.
	bool IsUnlockedForLocalPlayer() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> GateMesh;

	// 이 문을 여는 구출 대상 NPC ID
	UPROPERTY(EditAnywhere, Category = "Gate")
	FName RequiredNPCId;

	// 잠금 상태(기본) 외형 머티리얼
	UPROPERTY(EditAnywhere, Category = "Gate|Visual")
	TObjectPtr<UMaterialInterface> LockMaterial;

	// 해금 상태 외형 머티리얼
	UPROPERTY(EditAnywhere, Category = "Gate|Visual")
	TObjectPtr<UMaterialInterface> UnlockMaterial;
};
