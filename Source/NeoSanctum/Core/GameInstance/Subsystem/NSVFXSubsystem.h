// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Data/VFX/NSVFXDataTableRow.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NSVFXSubsystem.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UDataTable;

UCLASS()
class NEOSANCTUM_API UNSVFXSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UNSVFXSubsystem* Get(const UObject* WorldContext);
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
public:
	// 일반적인 경우 사용하는 Location Play 옵션 : 바로 재생하는 경우
	UFUNCTION(BlueprintCallable, Category = "VFX|Play")
	UNiagaraComponent* PlayVFXAtLocation(
		FName VFXID,
		FVector Location,
		FRotator Rotation = FRotator::ZeroRotator,
		float ScaleMultiplier = 1.0f);
	
	// Location Spawn 옵션 : 일단 스폰해두고 수동으로 Activate()를 할 필요가 있는 경우
	UFUNCTION(BlueprintCallable, Category = "VFX|Spawn")
	UNiagaraComponent* SpawnVFXAtLocation(
		FName VFXID,
		FVector Location,
		FRotator Rotation = FRotator::ZeroRotator,
		float ScaleMultiplier = 1.0f,
		bool bAutoActivate = false);
	
	// 일반적인 경우 사용하는 Attached Play 옵션 : 바로 재생하는 경우
	UFUNCTION(BlueprintCallable, Category = "VFX|Play")
	UNiagaraComponent* PlayVFXAttached(
		FName VFXID,
		USceneComponent* AttachToComponent,
		FName SocketName = NAME_None,
		FVector LocationOffset = FVector::ZeroVector,
		FRotator RotationOffset = FRotator::ZeroRotator,
		float ScaleMultiplier = 1.0f);
	
	// Attached Spawn 옵션 : 일단 스폰해두고 수동으로 Activate()를 할 필요가 있는 경우
	UFUNCTION(BlueprintCallable, Category = "VFX|Spawn")
	UNiagaraComponent* SpawnVFXAttached(
		FName VFXID,
		USceneComponent* AttachToComponent,
		FName SocketName = NAME_None,
		FVector LocationOffset = FVector::ZeroVector,
		FRotator RotationOffset = FRotator::ZeroRotator,
		float ScaleMultiplier = 1.0f,
		bool bAutoActivate = false);

private:
	const FNSVFXDataTableRow* FindVFXRow(FName VFXID) const;
	UNiagaraSystem* ResolveNiagaraSystem(const FNSVFXDataTableRow& VFXRow) const;
	FVector GetFinalScale(const FNSVFXDataTableRow& VFXRow, float ScaleMultiplier) const;

	// CommonData 로드 완료 후 VFXDataTable을 받아 VFX Row 캐시를 초기화.
	UFUNCTION()
	void HandleCommonDataReady();
	// VFX RowName을 키로 캐싱해 재생 시 DataTable 조회를 반복하지 않음.
	void RebuildVFXRowCache();

private:
	UPROPERTY()
	TObjectPtr<UDataTable> VFXDataTable;
	
	UPROPERTY()
	TMap<FName, FNSVFXDataTableRow> VFXRowCache;
};
