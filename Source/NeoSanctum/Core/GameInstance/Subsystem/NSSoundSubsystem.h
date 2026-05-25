// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NeoSanctum/Data/Sound/SoundDataTableRow.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NSSoundSubsystem.generated.h"

class UNSSoundData;

UENUM()
enum class ENSActiveSoundMode : uint8
{
	TwoDimension,
	AtLocation,
	Attached
};

USTRUCT()
struct FNSActiveSound
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAudioComponent> Component;

	UPROPERTY()
	FName SoundID = NAME_None;

	UPROPERTY()
	ENSSoundCategory Category = ENSSoundCategory::SFX;

	UPROPERTY()
	ENSActiveSoundMode Mode = ENSActiveSoundMode::TwoDimension;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	TWeakObjectPtr<USceneComponent> AttachComponent;

	UPROPERTY()
	FName SocketName = NAME_None;
};

UCLASS()
class NEOSANCTUM_API UNSSoundSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// SoundManager static getter
	static UNSSoundSubsystem* Get(const UObject* WorldContext);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	// BGM 재생	
	UFUNCTION(BlueprintCallable, Category = "Sound|Play")
	UAudioComponent* PlayBGM(FName SoundID, float FadeIn = 1.f);

	// 어디서든 동일한 볼륨으로 들리는 사운드 재생
	UFUNCTION(BlueprintCallable, Category = "Sound|Play")
	UAudioComponent* PlaySound2D(FName SoundID, float FadeIn = 1.f);

	// 특정 위치에서 재생하는 사운드
	UFUNCTION(BlueprintCallable, Category = "Sound|Play")
	UAudioComponent* PlaySoundAtLocation(FName SoundID, FVector Location);

	// 특정 소켓에 붙어서 함께 이동하는 사운드
	UFUNCTION(BlueprintCallable, Category = "Sound|Play")
	UAudioComponent* PlaySoundAttached(
		FName SoundID,
		USceneComponent* AttachToComponent,
		FName SocketName = NAME_None
	);

public:
	// BGM 재생 중지
	UFUNCTION(BlueprintCallable, Category = "Sound|Stop")
	void StopBGM(float FadeOut = -1.f);

	// 특정 사운드 재생 중지
	UFUNCTION(BlueprintCallable)
	void StopSound(UAudioComponent* AudioComponent, float FadeOut = -1.f);

	// 특정 카테고리 사운드 전체 중지
	UFUNCTION(BlueprintCallable, Category = "Sound|Stop")
	void StopCategory(ENSSoundCategory Category, float FadeOut = -1.f);

public:
	UFUNCTION(BlueprintCallable, Category = "Sound|Setter")
	void SetMasterVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Sound|Setter")
	void SetCategoryVolume(ENSSoundCategory Category, float Volume);

public:
	UFUNCTION(BlueprintPure, Category = "Sound|Getter")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintPure, Category = "Sound|Getter")
	float GetCategoryVolume(ENSSoundCategory Category) const;

private:
	// ID기반 DataTableRow 찾기 
	const FNSSoundDataTableRow* FindSoundRow(FName SoundID) const;

	// 단발성 사운드 플레이 관련 헬퍼
	void PlayOneShot2D(const FNSSoundDataTableRow& SoundRow) const;

	// 루프 사운드 플레이 관련 헬퍼
	UAudioComponent* PlayLoop2D(
		FName SoundID,
		const FNSSoundDataTableRow& SoundRow,
		float FadeIn = -1.f
	);
	// 특정 위치에서 재생되는 루프 사운드 헬퍼
	UAudioComponent* PlayLoopAtLocation(
		FName SoundID,
		const FNSSoundDataTableRow& SoundRow,
		FVector Location
	);
	// 특정 소켓에 부착되어 이동하는 루프 사운드 헬퍼
	UAudioComponent* PlayLoopAttached(
		FName SoundID,
		const FNSSoundDataTableRow& SoundRow,
		USceneComponent* AttachToComponent, FName SocketName
	);
	// 루프 사운드를 ActiveSounds 배열에 등록하기 위한
	void RegisterLoop(
		UAudioComponent* Component,
		FName SoundID, ENSSoundCategory Category,
		ENSActiveSoundMode Mode,
		FVector Location = FVector::ZeroVector,
		USceneComponent* AttachToComponent = nullptr,
		FName SocketName = NAME_None
	);
	// 사운드 루프를 걸기 위한 콜백
	void OnLoopFinished(UAudioComponent* FinishedComponent);
	// 루프 재생 중인 특정 사운드를 ID 기반으로 찾아서 재생 중지
	void StopSoundLoop(FName SoundID, float FadeOut = -1.f);
	// 모든 루프 중인 사운드를 재생 중지
	void StopAllLoops(float FadeOut = -1.f);

	// 최종 볼륨값 반환 : SoundRow의 기본 볼륨 * MasterVolume * 해당 사운드의 카테고리 볼륨
	float GetFinalVolume(const FNSSoundDataTableRow& SoundRow) const;
	// 카테고리 볼륨 설정 Initialize
	void InitializeCategoryVolumes();
	// 볼륨 설정을 직접 적용시키는 함수 : 주로 루프 재생되는 사운드에서 한 루프가 끝나고 나면 다시 볼륨을 조정해줘야하기 때문에 사용
	void ApplyVolume(ENSSoundCategory Category);

private:
	// 사운드 데이터 캐시
	UPROPERTY()
	TObjectPtr<UNSSoundData> SoundData;

	// 현재 지속적으로 재생중인 사운드 맵 : BGM이나 환경음처럼 계속 재생되는 사운드
	UPROPERTY()
	TArray<FNSActiveSound> ActiveSounds;

	// 마스터 볼륨
	UPROPERTY()
	float MasterVolume = 1.f;

	// 카테고리별 볼륨 맵
	UPROPERTY()
	TMap<ENSSoundCategory, float> CategoryVolumes;
};
