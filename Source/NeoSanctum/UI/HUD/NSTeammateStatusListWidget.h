// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NSTeammateStatusListWidget.generated.h"

class UNSTeammateStatusEntryWidget;
class UVerticalBox;
struct FNSPlayerStatusSnapshotMessage;
struct FNSPlayerStatusChangedMessage;
struct FNSPlayerStatusViewData;

/**
 * 팀원 상태 Snapshot과 변경메시지를 구독하고 팀원 Entry목록을 관리
 */

UCLASS()
class NEOSANCTUM_API UNSTeammateStatusListWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
private:
	//현재 팀원 전체 상태를 GMS로 요청
	void RequestSnapshot();
	
	//전체 팀원 목록 Snapshot 수신
	void HandleSnapshotMessage(FGameplayTag Channel, const FNSPlayerStatusSnapshotMessage& Message);
	
	//특정 팀원의 상태변경 도는 퇴장 메세지 수신
	void HandleChangedMessage(FGameplayTag Channel, const FNSPlayerStatusChangedMessage& Message);
	
	//기존 Entry갱신 또는 새로운 Entry생성
	void ApplyPlayerStatus(const FNSPlayerStatusViewData& StatusData);
	
	//PlayerId에 해당하는 Entry제거
	void RemovePlayerEntry(int32 PlayerId);
	
	//현재 생성된 모든 Entry 제거
	void ClearPlayerEntries();
	
private:
	//팀원 Entry가 추가되는 목록 컨테이너
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> TeammateListBox;
	
	//동적으로 생성할 팀원 Entry WBP 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Teammate Status", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UNSTeammateStatusEntryWidget> EntryWidgetClass;
	
	//PlayerId별로 생성된 Entry위젯
	UPROPERTY(Transient)
	TMap<int32,TObjectPtr<UNSTeammateStatusEntryWidget>> EntryWidgets;
	
	//Snapshot 응답 리스너
	FGameplayMessageListenerHandle SnapshotListenerHandle;
	
	//실시간 상태변경 리스너
	FGameplayMessageListenerHandle ChangedListenerHandle;
	
	//현재 Snapshot 여청을 식별하는 ID
	FGuid PendingRequestId;
	
protected:
	//GMS리스너 등록후 최초 Snapshot 요청
	virtual void NativeConstruct() override;
	
	//GMS리스너와 생성된 Entry정리
	virtual void NativeDestruct() override;
};
