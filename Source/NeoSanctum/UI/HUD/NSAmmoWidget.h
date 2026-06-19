// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSAmmoWidget.generated.h"

class UCommonTextBlock;

/**
 * 탄약수를 표시하는 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSAmmoWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	//현재탄약/ 최대탄약을 화면에 표시한다.
	UFUNCTION(BlueprintCallable,Category = "UI|Ammo")
	void SetAmmo(int32 CurrentAmmo, int32 MaxAmmo);
	
	// 리로드 상태에 따라 탄약 텍스트를 전환한다.
	UFUNCTION(BlueprintCallable, Category = "UI|Ammo")
	void SetReloading(bool bReloading);

	
private:
	//탄약 표시 택스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> AmmoText;
	
	bool bIsReloading = false;
	int32 CachedCurrentAmmo = 0;
	int32 CachedMaxAmmo = 0;

	void RefreshAmmoText();
};
