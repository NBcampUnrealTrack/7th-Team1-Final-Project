// Copyright 2026 One Team. All rights reserved.

#include "NSVFXSubsystem.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Engine/DataTable.h"
#include "NeoSanctum/Core/GameInstance/NSGameInstance.h"
#include "NeoSanctum/Data/VFX/NSVFXDataTableRow.h"

UNSVFXSubsystem* UNSVFXSubsystem::Get(const UObject* WorldContext)
{
	UNSGameInstance* GameInstance = UNSGameInstance::Get(WorldContext);
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UNSVFXSubsystem>();
}

void UNSVFXSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// TODO : 지금은 GameInstance에서 가져오지만, 결국은 비동기로딩을 통해 로딩된 테이블이나 데이터를 Get할 예정
	if (const UNSGameInstance* GameInstance = UNSGameInstance::Get(GetWorld()))
	{
		VFXDataTable = GameInstance->VFXDataTable;
	}
}

void UNSVFXSubsystem::Deinitialize()
{
	VFXDataTable = nullptr;
	Super::Deinitialize();
}

UNiagaraComponent* UNSVFXSubsystem::PlayVFXAtLocation(
	const FName VFXID,
	const FVector Location,
	const FRotator Rotation,
	const float ScaleMultiplier)
{
	return SpawnVFXAtLocation(VFXID, Location, Rotation, ScaleMultiplier, true);
}

UNiagaraComponent* UNSVFXSubsystem::SpawnVFXAtLocation(
	const FName VFXID,
	const FVector Location,
	const FRotator Rotation,
	const float ScaleMultiplier,
	const bool bAutoActivate)
{
	const FNSVFXDataTableRow* VFXRow = FindVFXRow(VFXID);
	if (!VFXRow)
	{
		return nullptr;
	}
	
	UNiagaraSystem* NiagaraSystem = ResolveNiagaraSystem(*VFXRow);
	if (!NiagaraSystem)
	{
		return nullptr;
	}
	
	return UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		NiagaraSystem,
		Location,
		Rotation,
		GetFinalScale(*VFXRow, ScaleMultiplier),
		VFXRow->bAutoDestroy,
		bAutoActivate,
		ENCPoolMethod::None,
		true);
}

UNiagaraComponent* UNSVFXSubsystem::PlayVFXAttached(
	const FName VFXID,
	USceneComponent* AttachToComponent,
	const FName SocketName,
	const FVector LocationOffset,
	const FRotator RotationOffset,
	const float ScaleMultiplier)
{
	return SpawnVFXAttached(
		VFXID,
		AttachToComponent,
		SocketName,
		LocationOffset,
		RotationOffset,
		ScaleMultiplier,
		true);
}

UNiagaraComponent* UNSVFXSubsystem::SpawnVFXAttached(
	const FName VFXID,
	USceneComponent* AttachToComponent,
	const FName SocketName,
	const FVector LocationOffset,
	const FRotator RotationOffset,
	const float ScaleMultiplier,
	const bool bAutoActivate)
{
	if (!AttachToComponent)
	{
		return nullptr;
	}
	
	const FNSVFXDataTableRow* VFXRow = FindVFXRow(VFXID);
	if (!VFXRow)
	{
		return nullptr;
	}
	
	UNiagaraSystem* NiagaraSystem = ResolveNiagaraSystem(*VFXRow);
	if (!NiagaraSystem)
	{
		return nullptr;
	}
	
	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		NiagaraSystem,
		AttachToComponent,
		SocketName,
		LocationOffset,
		RotationOffset,
		EAttachLocation::KeepRelativeOffset,
		VFXRow->bAutoDestroy,
		bAutoActivate,
		ENCPoolMethod::None,
		true);
	
	if (NiagaraComponent)
	{
		NiagaraComponent->SetRelativeScale3D(GetFinalScale(*VFXRow, ScaleMultiplier));
	}
	
	return NiagaraComponent;
}

const FNSVFXDataTableRow* UNSVFXSubsystem::FindVFXRow(const FName VFXID) const
{
	if (!VFXDataTable || VFXID.IsNone())
	{
		return nullptr;
	}
	
	return VFXDataTable->FindRow<FNSVFXDataTableRow>(VFXID, TEXT("NSVFXSubsystem"));
}

UNiagaraSystem* UNSVFXSubsystem::ResolveNiagaraSystem(const FNSVFXDataTableRow& VFXRow) const
{
	if (VFXRow.NiagaraSystem.IsNull())
	{
		return nullptr;
	}
	
	if (UNiagaraSystem* LoadedSystem = VFXRow.NiagaraSystem.Get())
	{
		return LoadedSystem;
	}
	
	// 임시 동기 로드. 추후 DataSubsystem 비동기 로딩 흐름으로 대체
	return VFXRow.NiagaraSystem.LoadSynchronous();
}

FVector UNSVFXSubsystem::GetFinalScale(
	const FNSVFXDataTableRow& VFXRow,
	const float ScaleMultiplier) const
{
	return FVector::OneVector * VFXRow.ScaleMultiplier * ScaleMultiplier;
}
