// Copyright 2026 One Team. All rights reserved.


#include "NSTeammateStatusListWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/VerticalBox.h"
#include "NeoSanctum/Tag/NSGameplayTags_Message.h"
#include "NeoSanctum/Type/NSPlayerStatusMessageTypes.h"
#include "NeoSanctum/UI/HUD/NSTeammateStatusEntryWidget.h"

void UNSTeammateStatusListWidget::RequestSnapshot()
{
	PendingRequestId = FGuid::NewGuid();
	
	FNSPlayerStatusQueryMessage QueryMessage;
	QueryMessage.RequestId = PendingRequestId;
	
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_TeammateStatus_Query,
		QueryMessage);
}

void UNSTeammateStatusListWidget::HandleSnapshotMessage(FGameplayTag Channel,
	const FNSPlayerStatusSnapshotMessage& Message)
{
	//다른 위젯이나 이전 요청 응담 무시
	if (Message.RequestId != PendingRequestId)
	{
		return;
	}
	
	ClearPlayerEntries();
	
	for (const FNSPlayerStatusViewData& StatusData
		: Message.Players)
	{
		ApplyPlayerStatus(StatusData);
	}
}

void UNSTeammateStatusListWidget::HandleChangedMessage(FGameplayTag Channel,
	const FNSPlayerStatusChangedMessage& Message)
{
	if (Message.bRemoved)
	{
		RemovePlayerEntry(
			Message.StatusData.PlayerId);
		return;
	}
	ApplyPlayerStatus(Message.StatusData);
}

void UNSTeammateStatusListWidget::ApplyPlayerStatus(const FNSPlayerStatusViewData& StatusData)
{
	if (StatusData.PlayerId == INDEX_NONE)
	{
		return;
	}

	TObjectPtr<UNSTeammateStatusEntryWidget>*
		ExistingEntry =
			EntryWidgets.Find(StatusData.PlayerId);

	if (ExistingEntry &&
		IsValid(ExistingEntry->Get()))
	{
		ExistingEntry->Get()->ApplyStatusData(
			StatusData);
		return;
	}

	if (!TeammateListBox ||
		!EntryWidgetClass)
	{
		return;
	}

	UNSTeammateStatusEntryWidget* NewEntry =
		CreateWidget<UNSTeammateStatusEntryWidget>(
			GetOwningPlayer(),
			EntryWidgetClass);

	if (!NewEntry)
	{
		return;
	}

	NewEntry->ApplyStatusData(StatusData);
	TeammateListBox->AddChildToVerticalBox(NewEntry);

	EntryWidgets.Add(
		StatusData.PlayerId,
		NewEntry);
}

void UNSTeammateStatusListWidget::RemovePlayerEntry(int32 PlayerId)
{
	TObjectPtr<UNSTeammateStatusEntryWidget>*
		FoundEntry =
			EntryWidgets.Find(PlayerId);

	if (!FoundEntry)
	{
		return;
	}

	if (IsValid(FoundEntry->Get()))
	{
		FoundEntry->Get()->RemoveFromParent();
	}

	EntryWidgets.Remove(PlayerId);
}

void UNSTeammateStatusListWidget::ClearPlayerEntries()
{
	if (TeammateListBox)
	{
		TeammateListBox->ClearChildren();
	}
	EntryWidgets.Reset();
}

void UNSTeammateStatusListWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	UGameplayMessageSubsystem& MessageSubsystem =
		UGameplayMessageSubsystem::Get(this);
	
	//Query보다 먼저 Snapshot리스너 등록
	SnapshotListenerHandle =
		MessageSubsystem.RegisterListener<
			FNSPlayerStatusSnapshotMessage>(
				NSGameplayTags::Message_UI_TeammateStatus_Snapshot,
				this,
				&ThisClass::HandleSnapshotMessage);
	
	ChangedListenerHandle =
		MessageSubsystem.RegisterListener<
			FNSPlayerStatusChangedMessage>(
				NSGameplayTags::Message_UI_TeammateStatus_Changed,
				this,
				&ThisClass::HandleChangedMessage);
	
	RequestSnapshot();
}

void UNSTeammateStatusListWidget::NativeDestruct()
{
	SnapshotListenerHandle.Unregister();
	ChangedListenerHandle.Unregister();
	
	ClearPlayerEntries();
	
	Super::NativeDestruct();
}
