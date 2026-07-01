// Copyright 2026 One Team. All rights reserved.

#include "NSVFXSubsystem.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NSDataSubsystem.h"
#include "Engine/DataTable.h"
#include "TimerManager.h"
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
	bCommonVFXWarmedUp = false;

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

	if (!bCommonVFXWarmedUp)
	{
		WarmupCommonVFX();
		bCommonVFXWarmedUp = true;
	}
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

void UNSVFXSubsystem::WarmupCommonVFX()
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	for (const TPair<FName, FNSVFXDataTableRow>& Pair : VFXRowCache)
	{
		const FNSVFXDataTableRow& VFXRow = Pair.Value;
		if (!VFXRow.bWarmupOnCommonDataReady)
		{
			continue;
		}

		UNiagaraSystem* NiagaraSystem = ResolveNiagaraSystem(VFXRow);
		if (!NiagaraSystem)
		{
			continue;
		}

		// 화면에 보이지 않는 임시 컴포넌트를 한 번 활성화해서 Niagara 첫 사용 컴파일 비용을 앞당김.
		UNiagaraComponent* WarmupComponent = NewObject<UNiagaraComponent>(World);
		if (!WarmupComponent)
		{
			continue;
		}

		WarmupComponent->SetAsset(NiagaraSystem);
		WarmupComponent->SetAutoActivate(false);
		WarmupComponent->SetHiddenInGame(true);
		WarmupComponent->SetVisibility(false, true);
		WarmupComponent->SetWorldLocation(FVector(0.0f, 0.0f, -100000.0f));
		WarmupComponent->RegisterComponentWithWorld(World);
		WarmupComponent->Activate(true);

		TWeakObjectPtr<UNiagaraComponent> WeakWarmupComponent = WarmupComponent;
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(this, [WeakWarmupComponent]()
			{
				if (WeakWarmupComponent.IsValid())
				{
					WeakWarmupComponent->DeactivateImmediate();
					WeakWarmupComponent->DestroyComponent();
				}
			})
		);
	}
}
