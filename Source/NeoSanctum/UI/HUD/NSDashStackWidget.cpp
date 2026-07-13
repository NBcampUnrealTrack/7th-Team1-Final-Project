// Copyright 2026 One Team. All rights reserved.

#include "NSDashStackWidget.h"

#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "NSDashStackEntryWidget.h"

void UNSDashStackWidget::SetDashCount(
	int32 CurrentDashCount,
	int32 MaxDashCount)
{
	const int32 SafeMaxCount =
		FMath::Max(MaxDashCount, 0);

	const int32 SafeCurrentCount =
		FMath::Clamp(
			CurrentDashCount,
			0,
			SafeMaxCount);

	if (DashStackEntries.Num() != SafeMaxCount)
	{
		RebuildEntries(SafeMaxCount);
	}

	for (int32 Index = 0;
		 Index < DashStackEntries.Num();
		 ++Index)
	{
		if (DashStackEntries[Index])
		{
			DashStackEntries[Index]->SetActive(
				Index < SafeCurrentCount);
		}
	}
}

void UNSDashStackWidget::RebuildEntries(
	int32 MaxDashCount)
{
	if (!DashStackBox || !DashStackEntryClass)
	{
		return;
	}

	DashStackBox->ClearChildren();
	DashStackEntries.Reset();

	for (int32 Index = 0;
		 Index < MaxDashCount;
		 ++Index)
	{
		UNSDashStackEntryWidget* Entry =
			CreateWidget<UNSDashStackEntryWidget>(
				GetOwningPlayer(),
				DashStackEntryClass);

		if (!Entry)
		{
			continue;
		}

		UHorizontalBoxSlot* EntrySlot =
			DashStackBox->AddChildToHorizontalBox(
				Entry);

		if (EntrySlot)
		{
			EntrySlot->SetPadding(
				FMargin(0.0f, 0.0f, 6.0f, 0.0f));
		}

		DashStackEntries.Add(Entry);
	}
}