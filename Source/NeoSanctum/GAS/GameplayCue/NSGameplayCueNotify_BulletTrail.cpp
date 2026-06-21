// Copyright 2026 One Team. All rights reserved.


#include "NeoSanctum/GAS/GameplayCue/NSGameplayCueNotify_BulletTrail.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

bool UNSGameplayCueNotify_BulletTrail::OnExecute_Implementation(
	AActor* MyTarget,
	const FGameplayCueParameters& Parameters
) const
{
	Super::OnExecute_Implementation(MyTarget, Parameters);
	
	if (!MyTarget || !BulletTrailVFX)
	{
		return false;
	}
	
	const FVector StartLocation = Parameters.Location;
	const FVector TrailDirection = Parameters.Normal.GetSafeNormal();
	const float TrailDistance = FMath::Max(Parameters.RawMagnitude, 0.0f);
	
	if (TrailDirection.IsNearlyZero() || TrailDistance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
	
	// 시작 지점으로부터 발사 방향으로 최대거리만큼 간 지점
	const FVector EndLocation = StartLocation + TrailDirection * TrailDistance;
	
	
	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		MyTarget,
		BulletTrailVFX,
		StartLocation,
		TrailDirection.Rotation(),
		FVector::OneVector,
		true,
		false
	);
	
	if (!NiagaraComponent)
	{
		return false;
	}
	
	// Ribbon Renderer 나이아가라 시스템에서 시작점과 끝점을 지정
	NiagaraComponent->SetVariableVec3(StartParameterName, StartLocation);
	NiagaraComponent->SetVariableVec3(EndParameterName, EndLocation);
	NiagaraComponent->Activate(true);
	
	return true;
}
