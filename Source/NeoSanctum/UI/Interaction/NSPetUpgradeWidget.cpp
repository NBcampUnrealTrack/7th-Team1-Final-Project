// Copyright 2026 One Team. All rights reserved.

#include "NSPetUpgradeWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NeoSanctum/Tag/NSGameplayTags_Message.h"
#include "NeoSanctum/Type/NSPetUpgradeMessageTypes.h"
#include "NeoSanctum/UI/PetUpgrade/NSPetUpgradeNodeWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"

void UNSPetUpgradeWidget::OpenForInteractor(APlayerController* Interactor)
{
	if (!Interactor)
	{
		return;
	}

	OwningController = Interactor;
	AddToViewport();
	
	RequestPetUpgradeSnapshot();

	Interactor->SetShowMouseCursor(true);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Interactor->SetInputMode(InputMode);
}

void UNSPetUpgradeWidget::CloseWidget()
{
	if (APlayerController* PC = OwningController.Get())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}

	RemoveFromParent();
}
void UNSPetUpgradeWidget::CacheNodeWidgets()
{
	NodeWidgetMap.Reset();

	if (!WidgetTree)
	{
		return;
	}

	TArray<UWidget*> AllWidgets;
	WidgetTree->GetAllWidgets(AllWidgets);

	for (UWidget* Widget : AllWidgets)
	{
		UNSPetUpgradeNodeWidget* NodeWidget =
			Cast<UNSPetUpgradeNodeWidget>(Widget);

		if (!NodeWidget)
		{
			continue;
		}

		const FGameplayTag NodeTag =
			NodeWidget->GetBoundNodeTag();

		if (!NodeTag.IsValid())
		{
			continue;
		}

		NodeWidget->OnUpgradeRequested.RemoveDynamic(
			this,
			&ThisClass::HandleNodeUpgradeRequested);

		NodeWidget->OnUpgradeRequested.AddUniqueDynamic(
			this,
			&ThisClass::HandleNodeUpgradeRequested);

		NodeWidgetMap.FindOrAdd(NodeTag) = NodeWidget;
	}
}

void UNSPetUpgradeWidget::UnbindNodeWidgets()
{
	for (TPair<
		FGameplayTag,
		TObjectPtr<UNSPetUpgradeNodeWidget>>& Pair
		: NodeWidgetMap)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->OnUpgradeRequested.RemoveDynamic(
				this,
				&ThisClass::HandleNodeUpgradeRequested);
		}
	}

	NodeWidgetMap.Reset();
}

void UNSPetUpgradeWidget::ApplySnapshotToNodeWidgets(
	const FNSPetUpgradeSnapshotMessage& Snapshot)
{
	for (const FNSPetUpgradeNodeViewData& NodeData
		: Snapshot.Nodes)
	{
		TObjectPtr<UNSPetUpgradeNodeWidget>* FoundWidget =
			NodeWidgetMap.Find(NodeData.NodeTag);

		if (!FoundWidget || !IsValid(FoundWidget->Get()))
		{
			continue;
		}

		FoundWidget->Get()->ApplyNodeData(NodeData);
	}
}

void UNSPetUpgradeWidget::HandleNodeUpgradeRequested(
	FGameplayTag CompanionTag,
	FGameplayTag NodeTag)
{
	RequestNodeUpgrade(CompanionTag, NodeTag);
}

void UNSPetUpgradeWidget::RequestNodeUpgrade(
	FGameplayTag CompanionTag,
	FGameplayTag NodeTag)
{
	// 유효하지 않은 태그가 브리지로 전달되지 않도록 사전 검증
	if (!CompanionTag.IsValid() || !NodeTag.IsValid())
	{
		return;
	}
	// 여러 요청 중 현재 요청의 결과만 처리하기 위한 식별자 생성
	PendingUpgradeRequestId = FGuid::NewGuid();

	FNSPetUpgradeRequestMessage RequestMessage;
	RequestMessage.RequestId = PendingUpgradeRequestId;
	RequestMessage.CompanionTag = CompanionTag;
	RequestMessage.NodeTag = NodeTag;
	// UI는 실제 강화 시스템을 호출하지 않고 GMS 요청만 방송
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_PetUpgrade_Upgrade_Request,
		RequestMessage);
}

void UNSPetUpgradeWidget::RequestPetUpgradeSnapshot()
{
	// 현재 Snapshot 요청의 응답만 처리하기 위한 식별자 생성
	PendingRequestId = FGuid::NewGuid();

	FNSPetUpgradeQueryMessage QueryMessage;
	QueryMessage.RequestId = PendingRequestId;
	
	// 현재 선택 펫과 전체 강화 트리 상태 조회 요청
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_PetUpgrade_Query,
		QueryMessage);
}

void UNSPetUpgradeWidget::HandleSnapshotMessage(FGameplayTag Channel, const FNSPetUpgradeSnapshotMessage& Message)
{
	// 다른 위젯 또는 이전 요청의 응답은 무시
	if (Message.RequestId != PendingRequestId)
	{
		return;
	}
	ApplySnapshotToNodeWidgets(Message);
}

void UNSPetUpgradeWidget::HandleUpgradeResultMessage(FGameplayTag Channel, const FNSPetUpgradeResultMessage& Message)
{
	// 현재 위젯에서 보낸 강화 요청의 결과만 처리
	if (Message.RequestId != PendingUpgradeRequestId)
	{
		return;
	}

	if (Message.bSuccess)
	{
		// 강화된 레벨을 즉시 화면에 반영하기 위해 최신 Snapshot 재요청
		RequestPetUpgradeSnapshot();
	}
}

void UNSPetUpgradeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	CacheNodeWidgets();
	
	// 펫 강화 트리 전체 상태 응답 구독
	SnapshotListenerHandle =
		UGameplayMessageSubsystem::Get(this)
		.RegisterListener<FNSPetUpgradeSnapshotMessage>(
			NSGameplayTags::Message_UI_PetUpgrade_Snapshot,
			this,
			&ThisClass::HandleSnapshotMessage);
	
	// 특정 노드 강화 성공 또는 실패 결과 구독
	UpgradeResultListenerHandle =
	UGameplayMessageSubsystem::Get(this)
	.RegisterListener<FNSPetUpgradeResultMessage>(
		NSGameplayTags::Message_UI_PetUpgrade_Upgrade_Result,
		this,
		&ThisClass::HandleUpgradeResultMessage);
}


void UNSPetUpgradeWidget::NativeDestruct()
{
	UnbindNodeWidgets();
	
	// 위젯 재생성 시 메시지가 중복 처리되지 않도록 리스너 해제
	SnapshotListenerHandle.Unregister();
	UpgradeResultListenerHandle.Unregister();

	Super::NativeDestruct();
}