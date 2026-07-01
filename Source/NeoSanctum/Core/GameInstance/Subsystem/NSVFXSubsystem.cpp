// Copyright 2026 One Team. All rights reserved.

#include "NSVFXSubsystem.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NSDataSubsystem.h"
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
	
	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->OnCommonDataReady.RemoveDynamic(this, &ThisClass::HandleCommonDataReady);
		
		if (DataSubsystem->IsCommonReady())
		{
			HandleCommonDataReady();
			return;
		}
		
		DataSubsystem->OnCommonDataReady.AddDynamic(this, &ThisClass::HandleCommonDataReady);
	}
}

void UNSVFXSubsystem::Deinitialize()
{
	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->OnCommonDataReady.RemoveDynamic(this, &ThisClass::HandleCommonDataReady);
	}
	
	VFXRowCache.Reset();
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
	return VFXID.IsNone() ? nullptr : VFXRowCache.Find(VFXID);
}

UNiagaraSystem* UNSVFXSubsystem::ResolveNiagaraSystem(const FNSVFXDataTableRow& VFXRow) const
{
	// NSDataSubsystem에서 CommonDataReady 전에 선로드하므로 여기서는 동기 로드를 하지 않음.
	return VFXRow.NiagaraSystem.Get();
}

FVector UNSVFXSubsystem::GetFinalScale(
	const FNSVFXDataTableRow& VFXRow,
	const float ScaleMultiplier) const
{
	return FVector::OneVector * VFXRow.ScaleMultiplier * ScaleMultiplier;
}

void UNSVFXSubsystem::HandleCommonDataReady()
{
	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->OnCommonDataReady.RemoveDynamic(this, &ThisClass::HandleCommonDataReady);
		VFXDataTable = DataSubsystem->GetCommonVFXDataTable();
	}
	
	RebuildVFXRowCache();
	
}

void UNSVFXSubsystem::RebuildVFXRowCache()
{
	VFXRowCache.Reset();
	
	if (!VFXDataTable)
	{
		return;
	}
	
	const FString ContextString = TEXT("NSVFXSubsystem");
	for (const FName& RowName : VFXDataTable->GetRowNames())
	{
		if (const FNSVFXDataTableRow* Row =
			VFXDataTable->FindRow<FNSVFXDataTableRow>(RowName, ContextString, false))
		{
			VFXRowCache.Add(RowName, *Row);
		}
	}
}
