// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NeoSanctum/Type/NSPetUpgradeMessageTypes.h"
#include "NeoSanctum/UI/Interaction/NSNPCInteractionWidgetBase.h"
#include "NSPetUpgradeWidget.generated.h"


class UNSPetUpgradeNodeWidget;

/**
 * 펫 강화 UI (최소 구현)
 * 펫 강화 백엔드 미구현 —> 현재는 오픈/클로즈 + 입력모드 전환만 담당
 */
UCLASS()
class NEOSANCTUM_API UNSPetUpgradeWidget : public UNSNPCInteractionWidgetBase
{
	GENERATED_BODY()

public:
	virtual void OpenForInteractor(APlayerController* Interactor) override;

	UFUNCTION(BlueprintCallable, Category = "Pet")
	virtual void CloseWidget() override;
	

private:
	//현재 선택된 펫의 강화 상태를 요청
	void RequestPetUpgradeSnapshot();

	//강화 상태를 전달 받음
	void HandleSnapshotMessage(FGameplayTag Channel, const FNSPetUpgradeSnapshotMessage& Message);
	
	//강화 요청 결과 처리
	void HandleUpgradeResultMessage(
	FGameplayTag Channel,
	const FNSPetUpgradeResultMessage& Message);
	
	// 배치된 펫 강화 노드 탐색 및 이벤트 연결
	void CacheNodeWidgets();

	// 노드 이벤트 연결 해제 및 캐시 초기화
	void UnbindNodeWidgets();

	// Snapshot을 NodeTag가 일치하는 위젯에 적용
	void ApplySnapshotToNodeWidgets(
		const FNSPetUpgradeSnapshotMessage& Snapshot);

	// 자식 노드의 강화 요청 처리
	UFUNCTION()
	void HandleNodeUpgradeRequested(
		FGameplayTag CompanionTag,
		FGameplayTag NodeTag);
	
	//선택한 노드의 강화를 GMS로 요청
	void RequestNodeUpgrade(
		FGameplayTag CompanionTag,
		FGameplayTag NodeTag);
	
private:
	TWeakObjectPtr<APlayerController> OwningController;
	// 펫 강화 Snapshot 메시지 리스너
	FGameplayMessageListenerHandle SnapshotListenerHandle;
	// 현재 Snapshot 요청 식별자
	FGuid PendingRequestId;
	

	//현재 진행중인 강화 요청 식별
	FGuid PendingUpgradeRequestId;
	// 강화 결과 메시지 리스너
	FGameplayMessageListenerHandle UpgradeResultListenerHandle;
	
	// NodeTag를 기준으로 배치된 노드 위젯 캐싱
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UNSPetUpgradeNodeWidget>>
		NodeWidgetMap;
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
};
