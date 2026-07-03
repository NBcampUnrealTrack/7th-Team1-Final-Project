// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_PlayAttachedVFXByID.generated.h"

class UNiagaraComponent;

/**
 * 소켓을 가진 컴포넌트에 VFX를 붙여 NotifyState 구간 동안 유지하는 Anim Notify State
 */
UCLASS(meta = (DisplayName = "NS Play Attached VFX By ID"))
class NEOSANCTUM_API UANS_PlayAttachedVFXByID : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference
	) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference
	) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	// DT_VFXDataTable의 Row Name
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (AllowPrivateAccess = "true"))
	FName VFXID = NAME_None;

	// VFX 부착 소켓
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (AllowPrivateAccess = "true"))
	FName SocketName = NAME_None;

	// 같은 소켓이 여러 컴포넌트에 있을 때 우선 탐색할 컴포넌트 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (AllowPrivateAccess = "true"))
	FName PreferredComponentName = NAME_None;

	// 소켓 기준 위치 오프셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (AllowPrivateAccess = "true"))
	FVector LocationOffset = FVector::ZeroVector;

	// 소켓 기준 회전 오프셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (AllowPrivateAccess = "true"))
	FRotator RotationOffset = FRotator::ZeroRotator;

	// DataTable의 Scale에 추가로 곱해지는 값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", UIMin = "0.0", UIMax = "3.0"))
	float ScaleMultiplier = 1.0f;

	// Notify가 발생한 MeshComp 탐색 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|Search", meta = (AllowPrivateAccess = "true"))
	bool bSearchNotifyMesh = true;

	// Owner Actor의 컴포넌트 탐색 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|Search", meta = (AllowPrivateAccess = "true"))
	bool bSearchOwnerComponents = true;

	// Owner Actor에 Attach된 Actor의 컴포넌트 탐색 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|Search", meta = (AllowPrivateAccess = "true"))
	bool bSearchAttachedActors = true;

	// 소켓을 찾지 못했을 때 Notify Mesh에 부착할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX|Search", meta = (AllowPrivateAccess = "true"))
	bool bFallbackToNotifyMesh = false;

private:
	// 탐색 옵션에 따른 최종 부착 컴포넌트 검색
	USceneComponent* FindAttachComponent(USkeletalMeshComponent* MeshComp, FName& OutSocketName) const;

	// 특정 Actor 안의 SceneComponent 검색
	USceneComponent* FindAttachComponentInActor(AActor* Actor, bool bRequirePreferredComponent, FName& OutSocketName) const;

	// 컴포넌트 이름과 소켓 조건 검사
	bool CheckComponent(USceneComponent* SceneComponent, bool bRequirePreferredComponent, FName& OutSocketName) const;

	// 생성된 VFX 비활성화
	void StopVFX(USkeletalMeshComponent* MeshComp);

private:
	// Notify를 발생시킨 Mesh별 활성 VFX
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TWeakObjectPtr<UNiagaraComponent>> ActiveVFXComponents;
};
