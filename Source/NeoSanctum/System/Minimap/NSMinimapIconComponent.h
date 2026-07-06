// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSMinimapIconComponent.generated.h"

/**
 * 미니맵 아이콘을 표시할 액터에 부착할 컴포넌트
 * BlueprintSpawnableComponent 메타지정자를 통해 블루프린트에서도 추가할 수 있음
 */
UCLASS(ClassGroup = (Minimap), meta = (BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSMinimapIconComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNSMinimapIconComponent();

	UFUNCTION(BlueprintPure, Category = "Minimap")
	FName GetIconRowName() const { return IconRowName; }

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetIconRowName(FName NewIconRowName);

	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool ShouldShowOnMinimap() const;

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetShowOnMinimap(bool bNewShowOnMinimap);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetHideWhenOwnerHealthZero(bool bNewHideWhenOwnerHealthZero);

	UFUNCTION(BlueprintPure, Category = "Minimap")
	FVector GetIconWorldLocation() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	FName IconRowName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	bool bShowOnMinimap = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	bool bHideWhenOwnerHealthZero = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	FVector WorldLocationOffset = FVector::ZeroVector;
};
