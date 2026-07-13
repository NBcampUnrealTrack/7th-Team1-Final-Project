// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "NSDashStackWidget.generated.h"

class UHorizontalBox;
class UNSDashStackEntryWidget;

UCLASS()
class NEOSANCTUM_API UNSDashStackWidget
	: public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetDashCount(
		int32 CurrentDashCount,
		int32 MaxDashCount);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> DashStackBox;

	UPROPERTY(
		EditDefaultsOnly,
		Category = "UI")
	TSubclassOf<UNSDashStackEntryWidget>
		DashStackEntryClass;

	void RebuildEntries(int32 MaxDashCount);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNSDashStackEntryWidget>>
		DashStackEntries;
};