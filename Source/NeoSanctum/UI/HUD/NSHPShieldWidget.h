// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSHPShieldWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;
class UNSCharacterData;
class UWidget;

/**
 *  플레이어의 체력과 실드를 표시하는 HUD위젯
 */
UCLASS()
class NEOSANCTUM_API UNSHPShieldWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	//체력 UI 갱신
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetHealth(float CurrentHealth, float MaxHealth);
	//쉴드 UI 갱신
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetShield(float CurrentShield, float MaxShield);
	//체력 수치 텍스트 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetHealthTextVisible(bool bVisible);
	//실드 수치 텍스트 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetShieldTextVisible(bool bVisible);
	//체략 실드 초기화
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ResetHealthAndShield();
	//현재 경험치와 다음 단계 요구 경험치를 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetExperience(float CurrentExperience, float RequiredExperience);
	
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetAmmo(int32 CurrentAmmo, int32 MaxAmmo);
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetReloading(bool bReloading);
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetCharacterInputIconVisible(bool bVisible);
	
private:
	//0 나누기 방지 후 ProgressBar 비율 계산
	float GetSafePercent(float CurrentValue, float MaxValue)const;
	
	//체력 바
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;
	//실드 바
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ShieldBar;
	//체력 수치 
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HealthText;
	//쉴드 수치 
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ShieldText;
	// 본인 경험치 진행도
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PlayerExperienceBar;
	// 본인 경험치 수치
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlayerExperienceValueText;
	//캐릭터 프로필
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> PortraitImage;
	//총알 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AmmoText;
	//탄약을 사용하는 캐릭터만 표시하는 AMMO 행
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> AmmoRow;
	// 캐릭터 메뉴 C 단축키 아이콘
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CharacterInputSizeBox;
	
	bool bIsReloading = false;
	int32 CachedCurrentAmmo = 0;
	int32 CachedMaxAmmo = 0;
	
	UPROPERTY()
	TObjectPtr<const UNSCharacterData> CachedPortraitCharacterData;
	
	void RefreshPortrait();
	void RefreshAmmoText();
	
protected:
	virtual void NativeConstruct()override;
};
