// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NSVFXSubsystem.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UDataTable;
struct FNSVFXDataTableRow;

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

private:
	UPROPERTY()
	TObjectPtr<UDataTable> VFXDataTable;
};
