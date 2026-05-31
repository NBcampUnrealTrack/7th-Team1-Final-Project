// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSAugmentCardWidget.generated.h"

class UBorder;
class UTextBlock;
class UImage;
class UTexture2D;

/**
 *  증강 카드를 표시하는 위젯
 */
UCLASS()
class NEOSANCTUM_API UNSAugmentCardWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	//증강 이름 설정
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetAugmentName(const FString& NewName);
	//증강 설명 설정
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetAugmentDescription(const FString& NewDescription);
	//카드 하이라이트 표시
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetHighLighted(bool bHighLighted);
	//증강 아이콘 설정
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetAugmentIcon(UTexture2D* NewIcon);
	
private:
	//증강 이름 텍스트
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> AugmentNameText;
	//증강 설명 텍스트
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> AugmentDescriptionText;
	//카드 테두리
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UBorder> CardBorder;
	//증강 아이콘
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> AugmentIcon;
	
protected:
	virtual void NativeConstruct() override;
};
