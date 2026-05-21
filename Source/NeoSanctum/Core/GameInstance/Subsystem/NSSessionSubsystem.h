// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "NSSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNSOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNSOnJoinSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNSOnDestroySessionComplete, bool, bWasSuccessful);

// 세션 생성, 참가, 종료 담당 클래스
// 타이틀 화면에서의 연결 로직 처리용
UCLASS()
class NEOSANCTUM_API UNSSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 세션 생성(호스트용)
	UFUNCTION(BlueprintCallable, Category = "Session")
	void CreateSession();

	// 초대 주소로 직접 참가(참가하는 클라이언트용)
	UFUNCTION(BlueprintCallable, Category = "Session")
	void JoinSessionByAddress(const FString& Address);
	
	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroySession();

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FNSOnCreateSessionComplete OnCreateSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FNSOnJoinSessionComplete OnJoinSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FNSOnDestroySessionComplete OnDestroySessionComplete;

private:
	void OnCreateSessionCompleted(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionCompleted(FName SessionName, bool bWasSuccessful);

	IOnlineSessionPtr SessionInterface;

	FDelegateHandle CreateSessionDelegateHandle;
	FDelegateHandle DestroySessionDelegateHandle;

	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
};
