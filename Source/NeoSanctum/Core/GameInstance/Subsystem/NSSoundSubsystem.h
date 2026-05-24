// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NSSoundSubsystem.generated.h"

enum class ENSSoundCategory : uint8;
/**
 * 
 */
UCLASS()
class NEOSANCTUM_API UNSSoundSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	// SoundManager static getter
	static UNSSoundSubsystem * Get(const UObject* WorldContext);
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
public:
	// BGM 재생	
	UFUNCTION(BlueprintCallable, Category = "Sound|Play")
	void PlayBGM(FName SoundID, float FadeIn = 1.f);
	
	// 어디서든 동일한 볼륨으로 들리는 사운드 재생
	UFUNCTION(BlueprintCallable, Category = "Sound|Play")
	void PlaySound2D(FName SoundID);
	
	// 특정 위치에서 재생하는 사운드
	UFUNCTION(BlueprintCallable, Category = "Sound|Play")
	void PlaySoundAtLocation(FName SoundID, FVector Location);
	
	// 특정 소켓에 붙어서 함께 이동하는 사운드
	UFUNCTION(BlueprintCallable, Category = "Sound|Play")
	void PlaySoundAttached(FName SoundID, USceneComponent* AttachToComponent, FName SocketName = NAME_None);
	
public:
	// BGM 재생 중지
	UFUNCTION(BlueprintCallable, Category = "Sound|Stop")
	void StopBGM(float FadeOut = -1.f);
	
	// 특정 사운드 재생 중지
	UFUNCTION(BlueprintCallable, Category = "Sound|Stop")
	void StopSound(FName SoundID, float FadeOut = -1.f);

	// 특정 카테고리 사운드 전체 중지
	UFUNCTION(BlueprintCallable, Category = "Sound|Stop")
	void StopCategory(ENSSoundCategory Category, float FadeOut = -1.f);
	
private:
	// 사운드 데이터 캐시
	UPROPERTY()
	TObjectPtr<UDataTable> SoundDataTable;
	
	// 현재 BGM 캐시
	UPROPERTY()
	TObjectPtr<UAudioComponent> CurrentBGM;
	
	UPROPERTY()
	FName CurrentBGMID = NAME_None;
	
private:
	UPROPERTY()
	float MasterVolume = 1.f;
	
	UPROPERTY()
	float BGMVolume = 1.f;
	
	UPROPERTY()
	float SFXVolume = 1.f;
	
	UPROPERTY()
	float UIVolume = 1.f;
};
