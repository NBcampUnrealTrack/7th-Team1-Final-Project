// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "NSPlayerController.generated.h"

class ANSDeathSpectatorPawn;
class ANSPlayerState;
class UNSAugmentSelectionComponent;
class UNSCharacterSelectWidget;

UCLASS()
class NEOSANCTUM_API ANSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ANSPlayerController();
	
	// 게임 시작 요청
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_RequestStartRun();
	
	void ExitSpectatorAndRespawn();
	
	UFUNCTION(Client, Reliable)
	void Client_ShowRunOverUI(bool bIsClear);

	// 클라이언트에 인런 데이터 로드 지시
	UFUNCTION(Client, Reliable)
	void Client_NotifyRunStarted();
	
public:
	// 사망 관전자 상태로 진입 요청 : 캐릭터의 사망 로직에서 요청하도록 되어있음
	void RequestEnterDeathSpectatorMode();
	void SpectatePreviousPlayer();
	void SpectateNextPlayer();

	// Tab키 : 증강 패널 토글 (InputBinderComponent에서 호출)
	void ToggleAugmentationPanel();

	//상호작용 시도(키 입력시 호출)
	void TryInteract();
	
private:
	// 실제로 사망 관전자 상태로 진입
	void EnterDeathSpectatorMode();
	// Spectator Pawn을 스폰하고 Possess하는 헬퍼
	void SpawnAndPossessDeathSpectatorPawn();
	// 진입 타이머 초기화 헬퍼
	void ClearDeathSpectatorModeTimer();
	
	// 관전자 스위칭
	void SwitchSpectatorTarget(int32 Direction);
	void SetSpectatorTarget(ANSPlayerState* NewSpectatorTarget);
	APawn* GetPawnFromPlayerState(const ANSPlayerState* TargetPlayerState) const;
	
	// Spectator Pawn을 스폰하고 Posses를 서버 권한에서 해야하기 때문에 서버 RPC로 처리
	UFUNCTION(Server, Reliable)
	void Server_EnterDeathSpectatorMode();
	
private:
	const FGameplayTagContainer& GetGameplayInputModeTags() const { return GameplayInputModeTags; }
	const FGameplayTagContainer& GetDeathSpectatorInputModeTags() const { return DeathSpectatorInputModeTags; }

private:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyRespawn();
	
	//PlayerState의 Attribute 변경을 HUD와 연결
	void BindAttributeToHUD();
	
	//현재 Attribute 값을 한번 읽어서 HUD초기값으로 반영
	void UpdateHUDHealthAndShield();
	
	//체력 변경시 갱신
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	
	//최대 체력 변경시 갱신
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	
	//실드 변경시 갱신
	void OnShieldChanged(const FOnAttributeChangeData& Data);
	
	//최대 실드 변경시 갱신
	void OnMaxShieldChanged(const FOnAttributeChangeData& Data);
	
	//HUD Attribute Delegate 중복 바인딩 방지
	bool bHUDAttributeBound = false;
	
	//캐릭터 선택 위젯 표시
	UFUNCTION(BlueprintCallable,Category="UI")
	void ShowCharacterSelectWidget();
	// 캐릭터 선택 위젯 닫기
	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideCharacterSelectWidget();
	
private:
	//캐릭터 선택 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI|CharacterSelect")
	TSubclassOf<UNSCharacterSelectWidget> CharacterSelectWidgetClass;
	//캐릭터 선택 위젯 인스턴스
	UPROPERTY()
	TObjectPtr<UNSCharacterSelectWidget> CharacterSelectWidget;
	// 캐릭터 선택 UI가 열려있는지
	UPROPERTY()
	bool bCharacterSelectOpen = false;
	
private:
	// 기본적인 Gameplay 상태일 때의 Input Mode 태그 목록
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FGameplayTagContainer GameplayInputModeTags;
	
	// 사망 시 Input Mode 태그 목록
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FGameplayTagContainer DeathSpectatorInputModeTags;
	
private:
	// 사망 후 몇 초 뒤에 Death Spectator 모드로 진입할지 결정
	UPROPERTY(EditDefaultsOnly, Category = "Spectator", meta = (ClampMin = "0.0"))
	float DeathSpectatorModeDelay = 2.0f;
	
	// 사망 시 Spawn / Possess될 Spectator Pawn
	UPROPERTY(EditDefaultsOnly, Category = "Spectator")
	TSubclassOf<ANSDeathSpectatorPawn> DeathSpectatorPawnClass;

	// 관전 대상 PlayerState 캐싱
	UPROPERTY(Transient)
	TObjectPtr<ANSPlayerState> SpectatingPlayerState;

	FTimerHandle DeathSpectatorModeTimerHandle;

	// 증강 추첨/선택 로직을 담당하는 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "NS|Augment")
	TObjectPtr<UNSAugmentSelectionComponent> AugmentSelectionComponent;

protected:
	virtual void BeginPlay() override;
	virtual void ClientRestart_Implementation(class APawn* NewPawn) override;
	
	virtual void SetupInputComponent() override;

	// O키(디버그 용) : 테스트용 증강 적재
	void Debug_EnqueueAugmentOffer();
};
