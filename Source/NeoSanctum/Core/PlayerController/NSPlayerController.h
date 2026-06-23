// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "NeoSanctum/Core/PlayerState/NSProgressTypes.h"
#include "NeoSanctum/Data/Character/NSCharacterData.h"
#include "NeoSanctum/Data/Progression/Currency/NSCurrencyTypes.h"
#include "NSPlayerController.generated.h"

class ANSDeathSpectatorPawn;
class ANSPlayerState;
class UNSAugmentSelectionComponent;
class UNSCharacterSelectWidget;
class UNSPermanentSaveGame;
class ANSRunGameState;
class UNSCurrencyComponent;
struct FNSSkillCooldownMessage;

UCLASS()
class NEOSANCTUM_API ANSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ANSPlayerController();
	
	UFUNCTION(Server, Reliable)
	void Server_SetReady(bool bNewReady);

	// 테스트용 임시 코드 (재화 드랍 치트 — 드롭 테이블 연동 후 삭제)
	// 치트를 서버 권한으로 실행시키기 위한 통로 (지정한 타입 1종을 각 플레이어 앞에 드랍)
	UFUNCTION(Server, Reliable)
	void Server_DebugSpawnCurrency(FGameplayTag Type, ENSCurrencyGrade Grade);

	// 테스트용 임시 코드 (재화 드랍 치트 — 드롭 테이블 연동 후 삭제)
	// 대기 영구재화를 서버 권한에서 커밋
	UFUNCTION(Server, Reliable)
	void Server_DebugCommitPermanent();

	// ===== 테스트용 임시 코드 — 인런 구출 NPC(M3) 구현 후 삭제 =====
	// 콘솔(`) 열고 DebugUnlockAllNPCs 입력 → 월드의 모든 거점 NPC를 로컬 진행도에 해금.
	UFUNCTION(Exec)
	void Debug_UnlockAllNPCs();
	// ===== 테스트용 임시 코드 끝 =====

	// 거점 레디 UI가 호출해야할 함수
	UFUNCTION(BlueprintCallable, Category="Run")
	void RequestReady(); 
	
	// 게임 시작 요청
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_RequestStartRun();
	
	void ExitSpectatorAndRespawn();

	// 클라이언트에 인런 데이터 로드 지시
	UFUNCTION(Client, Reliable)
	void Client_NotifyRunStarted();
	
	// 투표 확정 입력용
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="RunEnd")
	void Server_ConfirmVote(ENSRunChoice Choice);
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "RunEnd")
	void Server_CancelVote();
	
public:
	// 사망 관전자 상태로 진입 요청 : 캐릭터의 사망 로직에서 요청하도록 되어있음
	void RequestEnterDeathSpectatorMode();
	void SpectatePreviousPlayer();
	void SpectateNextPlayer();

	// Tab키 : 증강 패널 토글 (InputBinderComponent에서 호출)
	void ToggleAugmentationPanel();
	
	// UI 나올때 플레이어 인풋 제어용
	UFUNCTION(BlueprintCallable, Category="RunEnd")
	void EnterRunEndInputMode();
	UFUNCTION(BlueprintCallable, Category="RunEnd")
	void ExitRunEndInputMode();

	//상호작용 시도(키 입력시 호출)
	void TryInteract();
	
	// 세이브 데이터 업로드/ 저장용 RPC 함수
	UFUNCTION(Server, Reliable)
	void Server_UploadProgress(const FNSProgressPayload& Payload);
	UFUNCTION(Client, Reliable)
	void Client_SaveProgress(const FNSProgressPayload& Payload);
	UFUNCTION(BlueprintCallable, Category="Progress")
	void UploadLocalProgress(FName SelectedCharacterId);
	
	// 게임모드 호출용: 현재 진행도를 빌드해 소유 클라에 저장
	void SaveProgressToOwningClient();
	
	void CommitCharacterSelection(UNSCharacterData* SelectedCharacterData);
	//캐릭터 선택 위젯 표시
	UFUNCTION(BlueprintCallable,Category="UI")
	void ShowCharacterSelectWidget();

	// 거점 입장시 가장 최근 캐릭터 데이터 읽어오는 용도
	UFUNCTION(BlueprintCallable, Category="CharacterSelect")
	void RestoreLastSelectedCharacter();
	
	// 거점 라이브 장착(로컬 저장,서버 업로드), UI 호출용 함수
	UFUNCTION(BlueprintCallable, Category = "Progression|Part")
	void EquipPartLive(FName CharacterId, TSoftObjectPtr<UNSPartDefinition> Definition, ENSPartRarity Rarity);
	
	UFUNCTION(Client, Reliable)
	void Client_NotifySkillCooldownChanged(
		const FNSSkillCooldownMessage& Message);

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
	
	// 캐릭터 선택 위젯 닫기
	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideCharacterSelectWidget();
	// 캐릭터 선택 후 핸들러
	UFUNCTION()
	void HandleCharacterSelectionConfirmed(UNSCharacterData* ConfirmedCharacterData);

	//RunGameState의 런 종료 페이즈 변경시 델리게이트 바인딩
	void BindRunEndPhase();
	
	//런 종료 페이즈가 바뀌었을때 결과창 표시 상태 갱신
	UFUNCTION()
	void HandleRunEndPhaseChanged();

	// Travel 호출 직전에 UIManager 로딩창을 띄움
	void ShowTravelLoadingScreen();
	// Seamless Travel/ClientRestart 이후 위젯이 사라진 경우에 다시 복원
	void RestoreTravelLoadingScreenIfRequested();
	// 현재는 Spectator 단계에서는 유지, 실제 플레이어 캐릭터를 조작할 수 있게 된 뒤에 로딩창을 닫음
	// 추후에는 추가로 데이터가 다 로딩된 시점에 닫도록 해야하지 않을까 생각함
	void HideTravelLoadingScreenIfPlayable(APawn* NewPawn);
	
	//현재 바인딩한 RunGameState를 캐싱
	UPROPERTY()
	TObjectPtr<ANSRunGameState> CachedRunGameState;
	
	//테스트용 : 클리어
	void Debug_ForceRunClear();
	
	//클라이언트 입력으로 호출된 클리어 테스트를 서버에서 처리한다
	UFUNCTION(Server, Reliable)
	void Server_DebugForceRunClear();

	//현재 탄약 값을 읽어 HUD에 반영
	void UpdateHUDAmmo();

	//탄약 변경시 갱신
	void OnAmmoChanged(const FOnAttributeChangeData& Data);

	//최대 탄약 변경시 갱신
	void OnMaxAmmoChanged(const FOnAttributeChangeData& Data);
	
	//리로드 태그 변경시 탄약 UI 상태 갱신
	void OnReloadingTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	
	//PlayerState의 재화 컴포넌트를 HUD에 연결
	void BindCurrencyToHUD();
	
	//현재 재화 값을 한번 읽어서 HUD에 반영
	void UpdateHUDCurrency();
	
	//임시 재화 변경시 HUD갱신
	void OnTempCurrencyChanged(int64 Amount);
	
	//영구 재화 버킷 변경시 HUD 갱신
	void OnPermanentCurrencyChanged(FGameplayTag Type, int64 Amount);
	
	//클라이언트 캐시에 저장된 진행 데이터를 현재 PlayerState에 적용
	void ApplyCachedProgressToLocalPlayerState();
	
	TWeakObjectPtr<UNSCurrencyComponent> CachedCurrencyComponent;
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
	
	//캐릭터 데이터로 스킬 슬롯 UI 갱신
	void UpdateSkillUIFromCurrentCharacter();
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
	
	// 로드 완료 후 데이터 복원되도록 할 용도
	void HandlePermanentDataLoaded(UNSPermanentSaveGame* Data);
	FDelegateHandle PermanentDataLoadedHandle;
protected:
	virtual void BeginPlay() override;
	virtual void ClientRestart_Implementation(class APawn* NewPawn) override;
	
	// 클라이언트가 다른 맵/서버로 이동하기 직전에 호출되는 함수로, Loading창을 띄우는 시점을 관리하기 위해 가져왔음
	virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel) override;
	
	virtual void SetupInputComponent() override;

	// O키(디버그 용) : 테스트용 증강 적재
	void Debug_EnqueueAugmentOffer();

#pragma region Companion Cheat
	
public:
	void CompanionCheatUpgrade(FGameplayTag CompanionTag);
	
	void CompanionCheatSelect(FGameplayTag CompanionTag);
	
	FName GetActiveCharacterIdForUpload() const;
#pragma endregion Companion Cheat
};
