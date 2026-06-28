// Copyright 2026 One Team. All rights reserved.

#include "NSPlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "AbilitySystemComponent.h"
#include "NeoSanctum/Character/Component/NSInputBinderComponent.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Character/Spectator/NSDeathSpectatorPawn.h"
#include "NeoSanctum/Character/Component/NSSpectatorViewComponent.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Core/Interface/NSOutGameModeInterface.h"
#include "NeoSanctum/Core/Interface/NSGameInstanceInterface.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSProgressionSubsystem.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "NeoSanctum/Type/NSSkillCooldownTypes.h"
#include "NeoSanctum/System/NSSaveGameSubsystem.h"
#include "NeoSanctum/Progression/Save/NSPermanentSaveGame.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "NeoSanctum/Interaction/NPC/NSInteractableNPCBase.h"
#include "NeoSanctum/UI/Interaction/NSNPCInteractionWidgetBase.h"
#include "EngineUtils.h"
#include "NeoSanctum/Progression/Augment/NSAugmentSelectionComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Augment.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"
#include "NeoSanctum/UI/CharacterSelect/NSCharacterSelectWidget.h"
#include "NeoSanctum/GAS/AttributeSet/NsPlayerAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Input.h"
#include "NeoSanctum/Tag/NSGameplayTags_Message.h"
#include "InputCoreTypes.h"
#include "Engine/GameInstance.h"
#include "NeoSanctum/Interaction/Component/NSInteractionComponent.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "NeoSanctum/Data/Character/NSCharacterData.h"
#include "Engine/AssetManager.h"
#include "NeoSanctum/Character/Component/NSCompanionProgressionComponent.h"
#include "NeoSanctum/Core/Cheat/NSCheatManager.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSessionSubsystem.h"
#include "NeoSanctum/System/Subsystem/NSCurrencyDropSubsystem.h"
#include "NeoSanctum/Progression/Currency/NSCurrencyComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Currency.h"
#include "NeoSanctum/Data/Progression/Currency/NSCurrencyTypes.h"
#include "NeoSanctum/Tag/NSGameplayTags_Reward.h"
#include "NeoSanctum/Combat/HitReaction/NSPlayerAttackFeedbackComponent.h"


ANSPlayerController::ANSPlayerController()
{
	// 기본 태그 초기화
	DeathSpectatorPawnClass = ANSDeathSpectatorPawn::StaticClass();

	// 테스트용 임시 코드 (재화 드랍 치트 — 드롭 테이블 연동 후 삭제)
	CheatClass = UNSCheatManager::StaticClass();

	GameplayInputModeTags.AddTag(NSGameplayTags::InputMode_Gameplay);
	GameplayInputModeTags.AddTag(NSGameplayTags::InputMode_UI);
	
	// 사망 상태 태그 초기화
	DeathSpectatorInputModeTags.AddTag(NSGameplayTags::InputMode_DeathSpectator);
	DeathSpectatorInputModeTags.AddTag(NSGameplayTags::InputMode_UI);

	// 증강 선택 컴포넌트 생성
	AugmentSelectionComponent = CreateDefaultSubobject<UNSAugmentSelectionComponent>(TEXT("AugmentSelectionComponent"));
}

void ANSPlayerController::RequestReady()
{
	const ANSPlayerState* OwningPlayerState = GetPlayerState<ANSPlayerState>();
	const bool bCurrentlyReady =
		OwningPlayerState ? OwningPlayerState->IsReady() : false;
	const bool bNewReady = !bCurrentlyReady;
	
	// 레디 ON 전환 시: 허브에서의 최신 변경분(강화/파츠 구매 등)을 서버 컴포넌트로 업로드
	if (bNewReady)
	{
		UGameInstance* GameInstance = GetGameInstance();
		UNSSaveGameSubsystem* SaveSubsystem =
			GameInstance ? GameInstance->GetSubsystem<UNSSaveGameSubsystem>() : nullptr;
		UNSPermanentSaveGame* PermanentSave =
			SaveSubsystem ? SaveSubsystem->GetCachedPermanentData() : nullptr;

		if (PermanentSave && !PermanentSave->LastSelectedCharacterId.IsNone())
		{
			UploadLocalProgress(PermanentSave->LastSelectedCharacterId);
		}
	}
	
	Server_SetReady(bNewReady);
}

void ANSPlayerController::Server_SetReady_Implementation(bool bNewReady)
{
	ANSPlayerState* OwningPlayerState = GetPlayerState<ANSPlayerState>();
	if (!OwningPlayerState)
	{
		return;
	}
	OwningPlayerState->SetReady(bNewReady);
}

// 테스트용 임시 코드 (재화 드랍 치트 — 드롭 테이블 연동 후 삭제)
void ANSPlayerController::Server_DebugSpawnCurrency_Implementation(FGameplayTag Type, ENSCurrencyGrade Grade)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UNSCurrencyDropSubsystem* DropSys = World->GetSubsystem<UNSCurrencyDropSubsystem>();
	if (!DropSys)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || !PC->GetPawn())
		{
			continue;
		}

		// 발밑에 두면 그 위에 선 플레이어가 즉시 자동 획득하므로, 앞쪽으로 떨어뜨려 둠
		const APawn* DropPawn = PC->GetPawn();
		const FVector DropLoc = DropPawn->GetActorLocation() + DropPawn->GetActorForwardVector() * 200.f;
		DropSys->RegisterDrop(Type, Grade, 10, DropLoc, 60.f);
	}
}

// 테스트용 임시 코드 (재화 드랍 치트 — 드롭 테이블 연동 후 삭제)
void ANSPlayerController::Server_DebugCommitPermanent_Implementation()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		ANSPlayerState* PS = PC->GetPlayerState<ANSPlayerState>();
		if (!PS)
		{
			continue;
		}

		if (UNSCurrencyComponent* Currency = PS->GetCurrencyComponent())
		{
			// 클리어 시뮬레이션: 1배율로 커밋
			Currency->CommitRunPermanent(1.0f);
		}
	}
}

void ANSPlayerController::BindAttributeToHUD()
{
	if (!IsLocalController())
	{
		return;
	}
	ANSPlayerState* NSPlayerState = GetPlayerState<ANSPlayerState>();
	if (!NSPlayerState)
	{
		return;
	}
	
	UAbilitySystemComponent* ASC = NSPlayerState->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}
	//체력 변경시 델리게이트에서 바인딩으로 체력 갱신
	ASC->GetGameplayAttributeValueChangeDelegate(
		UNSBaseAttributeSet::GetHealthAttribute()
		).AddUObject(this, &ANSPlayerController::OnHealthChanged);	
	//최대체력 변경시 델리게이트에서 바인딩으로 체력 갱신
	ASC->GetGameplayAttributeValueChangeDelegate(
		UNSBaseAttributeSet::GetMaxHealthAttribute()
		).AddUObject(this, &ANSPlayerController::OnMaxHealthChanged);
	//실드 변경시 델리게이트에서 바인딩으로 체력 갱신
	ASC->GetGameplayAttributeValueChangeDelegate(
		UNSPlayerAttributeSet::GetShieldAttribute()
		).AddUObject(this, &ANSPlayerController::OnShieldChanged);
	//최대실드 변경시 델리게이트에서 바인딩으로 체력갱신
	ASC->GetGameplayAttributeValueChangeDelegate(
		UNSPlayerAttributeSet::GetMaxShieldAttribute()
			).AddUObject(this, &ANSPlayerController::OnMaxShieldChanged);
	
	ASC->GetGameplayAttributeValueChangeDelegate(
	UNSPlayerAttributeSet::GetAmmoAttribute()
).AddUObject(this, &ANSPlayerController::OnAmmoChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(
		UNSPlayerAttributeSet::GetMaxAmmoAttribute()
	).AddUObject(this, &ANSPlayerController::OnMaxAmmoChanged);
	
	ASC->RegisterGameplayTagEvent(
	NSGameplayTags::State_Reloading,
	EGameplayTagEventType::NewOrRemoved
).AddUObject(this, &ANSPlayerController::OnReloadingTagChanged);
	
	bHUDAttributeBound = true;
	
	UpdateHUDHealthAndShield();
	UpdateHUDAmmo();
}

void ANSPlayerController::UpdateHUDHealthAndShield()
{
	if (!IsLocalController())
	{
		return;
	}
	ANSPlayerState* NSPlayerState =
		GetPlayerState<ANSPlayerState>();
	if (!NSPlayerState)
	{
		return;
	}
	const UNSPlayerAttributeSet* PlayerAttributeSet =
		NSPlayerState->GetPlayerAttributeSet();
	if (!PlayerAttributeSet)
	{
		return;
	}
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}
	UNSUIManagerSubsystem* UIManager = 
		GameInstance->GetSubsystem<UNSUIManagerSubsystem>();
	if (!UIManager)
	{
		return;
	}

	// 현재 Attribute 값을 HUD에 한번 직접 반영
	UIManager->UpdateHealthAndShield(
		PlayerAttributeSet->GetHealth(),
		PlayerAttributeSet->GetMaxHealth(),
		PlayerAttributeSet->GetShield(),
		PlayerAttributeSet->GetMaxShield()
	);
}

void ANSPlayerController::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	//체력값이 바뀌면 전체 HP/Shield값을 다시 읽어 HUD에 반영
	UpdateHUDHealthAndShield();
}

void ANSPlayerController::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	//최대체력 변경시 전체 값을 다시 갱신한다
	UpdateHUDHealthAndShield();
}

void ANSPlayerController::OnShieldChanged(const FOnAttributeChangeData& Data)
{
	//실드 값이 바뀌면 HUD에 반영
	UpdateHUDHealthAndShield();
}

void ANSPlayerController::OnMaxShieldChanged(const FOnAttributeChangeData& Data)
{
	//최대실드 변경시 전체 값을 다시 갱신
	UpdateHUDHealthAndShield();
}

void ANSPlayerController::ShowCharacterSelectWidget()
{
	if (!IsLocalController())
	{
		return;
	}
	
	if (bCharacterSelectOpen)
	{
		return;
	}
	
	if (!CharacterSelectWidgetClass)
	{
		return;
	}

	if (!CharacterSelectWidget)
	{
		CharacterSelectWidget = CreateWidget<UNSCharacterSelectWidget>(
			this,
			CharacterSelectWidgetClass
		);
		
		if (CharacterSelectWidget)
		{
			CharacterSelectWidget->OnCharacterSelectionConfirmed.AddDynamic(
				this, &ANSPlayerController::HandleCharacterSelectionConfirmed);
		}
	}

	if (!CharacterSelectWidget)
	{
		return;
	}

	if (!CharacterSelectWidget->IsInViewport())
	{
		CharacterSelectWidget->AddToViewport();
	}

	bCharacterSelectOpen = true;
	
	SetShowMouseCursor(true);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(CharacterSelectWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void ANSPlayerController::HideCharacterSelectWidget()
{
	if (CharacterSelectWidget)
	{
		CharacterSelectWidget->RemoveFromParent();
		CharacterSelectWidget = nullptr;
	}
	bCharacterSelectOpen = false;

	SetShowMouseCursor(false);

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}	

void ANSPlayerController::HandleCharacterSelectionConfirmed(UNSCharacterData* ConfirmedCharacterData)
{
	if (!ConfirmedCharacterData)
	{
		return;
	}
	
	CommitCharacterSelection(ConfirmedCharacterData);
	// 확인 버튼 클릭 후에도 UI화면이 남아있어야 한다면 밑의 함수 제거
	HideCharacterSelectWidget(); 
}

void ANSPlayerController::BindRunEndPhase()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	ANSRunGameState* RunGameState =
		World->GetGameState<ANSRunGameState>();
	if (!IsValid(RunGameState))
	{
		return;
	}
	
	CachedRunGameState = RunGameState;
	
	//중복 바인딩 방지
	RunGameState->OnRunEndPhaseChanged.RemoveDynamic(this,
	&ANSPlayerController::HandleRunEndPhaseChanged);
	RunGameState->OnRunEndPhaseChanged.AddDynamic(this,
		&ANSPlayerController::HandleRunEndPhaseChanged);
	
	//이미 Result상태라면 바인딩 직후 동기화
	HandleRunEndPhaseChanged();
}

void ANSPlayerController::HandleRunEndPhaseChanged()
{
	if (!IsValid(CachedRunGameState) || !GetGameInstance())
	{
		return;
	}
	
	UNSUIManagerSubsystem* UIManager =
		GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>();
	if (!UIManager)
	{
		return;
	}
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[RunEnd UI] Phase=%d, bIsClear=%d, WinningChoice=%d"),
		static_cast<int32>(CachedRunGameState->RunEndPhase),
		CachedRunGameState->bIsClear ? 1 : 0,
		static_cast<int32>(CachedRunGameState->WinningChoice)
	);
	
	switch (CachedRunGameState->RunEndPhase)
	{
	case ENSRunEndPhase::Voting:
	case ENSRunEndPhase::Result:
		UIManager->CreateRunEnd(this);
		UIManager->UpdateRunEndResultFromGameState(CachedRunGameState);
		UIManager->UpdateRunEndVotes(
			CachedRunGameState->NextVotes,
			CachedRunGameState->HubVotes);
		UIManager->ShowRunEnd();
		EnterRunEndInputMode();
		break;
		
	case ENSRunEndPhase::None :
		default:
			UIManager->HideRunEnd();
			break;
	}
}

void ANSPlayerController::Debug_ForceRunClear()
{
	if (!IsLocalController())
	{
		return;
	}

	Server_DebugForceRunClear();
}

void ANSPlayerController::UpdateHUDAmmo()
{
	if (!IsLocalController())
	{
		return;
	}

	ANSPlayerState* NSPlayerState = GetPlayerState<ANSPlayerState>();
	if (!NSPlayerState)
	{
		return;
	}

	const UNSPlayerAttributeSet* PlayerAttributeSet =
		NSPlayerState->GetPlayerAttributeSet();
	if (!PlayerAttributeSet)
	{
		return;
	}

	UNSUIManagerSubsystem* UIManager =
		GetGameInstance()
		? GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>()
		: nullptr;
	if (!UIManager)
	{
		return;
	}

	UIManager->UpdateAmmo(
		FMath::FloorToInt(PlayerAttributeSet->GetAmmo()),
		FMath::FloorToInt(PlayerAttributeSet->GetMaxAmmo()));
}

void ANSPlayerController::OnAmmoChanged(const FOnAttributeChangeData& Data)
{
	UpdateHUDAmmo();
}

void ANSPlayerController::OnMaxAmmoChanged(const FOnAttributeChangeData& Data)
{
	UpdateHUDAmmo();
}

void ANSPlayerController::OnReloadingTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (!IsLocalController())
	{
		return;
	}

	UNSUIManagerSubsystem* UIManager =
		GetGameInstance()
		? GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>()
		: nullptr;
	if (!UIManager)
	{
		return;
	}

	UIManager->SetReloading(NewCount > 0);

	if (NewCount <= 0)
	{
		UpdateHUDAmmo();
	}
}

void ANSPlayerController::BindCurrencyToHUD()
{
	if (!IsLocalController())
	{
		return;
	}
	
	ANSPlayerState* NSPlayerState = GetPlayerState<ANSPlayerState>();
	if (!NSPlayerState)
	{
		return;
	}
	
	UNSCurrencyComponent* CurrencyComponent =
		NSPlayerState->GetCurrencyComponent();
	if (!CurrencyComponent)
	{
		return;
	}
	
	CachedCurrencyComponent = CurrencyComponent;
	
	//중복 방지
	CurrencyComponent->OnTempChanged.RemoveAll(this);
	CurrencyComponent->OnPermanenetChanged.RemoveAll(this);

	CurrencyComponent->OnTempChanged.AddUObject(
		this,
		&ANSPlayerController::OnTempCurrencyChanged);

	CurrencyComponent->OnPermanenetChanged.AddUObject(
		this,
		&ANSPlayerController::OnPermanentCurrencyChanged);

	UpdateHUDCurrency();
}

void ANSPlayerController::UpdateHUDCurrency()
{
	if (!IsLocalController())
	{
		return;
	}
	
	UNSCurrencyComponent* CurrencyComponent =
		CachedCurrencyComponent.Get();
	
	if (!CurrencyComponent)
	{
		ANSPlayerState* NSPlayerState = GetPlayerState<ANSPlayerState>();
		CurrencyComponent = NSPlayerState
			? NSPlayerState->GetCurrencyComponent()
			: nullptr;
	}

	if (!CurrencyComponent)
	{
		return;
	}

	UNSUIManagerSubsystem* UIManager =
		GetGameInstance()
		? GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>()
		: nullptr;

	if (!UIManager)
	{
		return;
	}

	//임시 재화
	UIManager->UpdateRunInGoods(
		static_cast<int32>(CurrencyComponent->GetTemp()));

	//이번 런에서 얻은 공통 영구 재화
	UIManager->UpdateRunOutGoods(
		static_cast<int32>(CurrencyComponent->GetPermanent(
			NSGameplayTags::Currency_Common)));

	//이번 런에서 얻은 스킬 재화
	UIManager->UpdateRunSkillGoods(
		static_cast<int32>(CurrencyComponent->GetPermanent(
			NSGameplayTags::Currency_Skill)));
}

void ANSPlayerController::OnTempCurrencyChanged(int64 Amount)
{
	UNSUIManagerSubsystem* UIManager =
	GetGameInstance()
	? GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>()
	: nullptr;
	if (!UIManager)
	{
		return;
	}

	UIManager->UpdateRunInGoods(static_cast<int32>(Amount));
}

void ANSPlayerController::OnPermanentCurrencyChanged(FGameplayTag Type, int64 Amount)
{
	UNSUIManagerSubsystem* UIManager =
		GetGameInstance()
		? GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>()
		: nullptr;

	if (!UIManager)
	{
		return;
	}

	if (Type == NSGameplayTags::Currency_Common)
	{
		UIManager->UpdateRunOutGoods(static_cast<int32>(Amount));
		UIManager->UpdateRunResultCommonGoods(static_cast<int32>(Amount));
		return;
	}

	if (Type == NSGameplayTags::Currency_Skill)
	{
		UIManager->UpdateRunSkillGoods(static_cast<int32>(Amount));
		UIManager->UpdateRunResultSkillGoods(static_cast<int32>(Amount));
		return;
	}
}

void ANSPlayerController::ApplyCachedProgressToLocalPlayerState()
{
	UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem)
	{
		return;
	}

	ANSPlayerState* NSPlayerState = GetPlayerState<ANSPlayerState>();
	if (!IsValid(NSPlayerState))
	{
		return;
	}

	DataSubsystem->ApplyCachedProgressTo(
		NSPlayerState->GetProgressComponent());
}

void ANSPlayerController::StartSkillUIApplyRetry()
{
	SkillUIApplyRetryCount = 0;
	
	if (TryApplySkillUIFromCurrentCharacter())
	{
		GetWorldTimerManager().ClearTimer(SkillUIApplyRetryTimerHandle);
		return;
	}
	GetWorldTimerManager().ClearTimer(SkillUIApplyRetryTimerHandle);
	GetWorldTimerManager().SetTimer(
		SkillUIApplyRetryTimerHandle,
		this,
		&ANSPlayerController::HandleSkillUIApplyRetry,
		0.1f,
		true);
}

void ANSPlayerController::RetryApplySkillUIFromCurrentCharacter()
{
	const bool bApplied = TryApplySkillUIFromCurrentCharacter();
	
	SkillUIApplyRetryCount++;
	
	if (bApplied || SkillUIApplyRetryCount >= 10)
	{
		GetWorldTimerManager().ClearTimer(SkillUIApplyRetryTimerHandle);
	}
}

bool ANSPlayerController::TryApplySkillUIFromCurrentCharacter()
{
	if (!IsLocalController())
	{
		return false;
	}
	
	ANSPlayerState* NSPlayerState = GetPlayerState<ANSPlayerState>();
	if (!NSPlayerState)
	{
		return false;
	}
	
	UNSCharacterData* CurrentCharacterData =
		NSPlayerState->GetCurrentCharacterData();
	if (!CurrentCharacterData || !CurrentCharacterData->CharacterTag.IsValid())
	{
		return false;
	}
	
	UNSUIManagerSubsystem* UIManagerSubsystem =
		UNSUIManagerSubsystem::Get(this);
	if (!UIManagerSubsystem)
	{
		return false;
	}
	
	UIManagerSubsystem->ApplyCharacterSkillUISet(
		CurrentCharacterData->CharacterTag.GetTagName());

	return true;
}

void ANSPlayerController::HandleSkillUIApplyRetry()
{
	constexpr int32 MaxRetryCount = 20;
	++SkillUIApplyRetryCount;
	if (TryApplySkillUIFromCurrentCharacter() || SkillUIApplyRetryCount >= MaxRetryCount)
	{
		GetWorldTimerManager().ClearTimer(SkillUIApplyRetryTimerHandle);
	}
}

void ANSPlayerController::UpdateSkillUIFromCurrentCharacter()
{
	if (!IsLocalController())
	{
		return;
	}

	ANSPlayerState* NSPlayerState = GetPlayerState<ANSPlayerState>();
	if (!NSPlayerState)
	{
		return;
	}

	UNSCharacterData* CurrentCharacterData =
		NSPlayerState->GetCurrentCharacterData();
	if (!CurrentCharacterData || !CurrentCharacterData->CharacterTag.IsValid())
	{
		return;
	}

	UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(this);
	if (!UIManager)
	{
		return;
	}

	// DT_CharacterSkillUISet의 RowName이 Character.Player.Engineer 형태이므로
	// 태그의 마지막 조각만 자르지 않고 전체 TagName을 사용한다.
	UIManager->ApplyCharacterSkillUISet(
		CurrentCharacterData->CharacterTag.GetTagName());
}
void ANSPlayerController::Server_CancelVote_Implementation()
{
	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (GameMode && GameMode->Implements<UNSRunGameModeInterface>())
	{
		INSRunGameModeInterface::Execute_CancelRunChoice(GameMode, this);
	}
}

void ANSPlayerController::Server_DebugForceRunClear_Implementation()
{
	AGameModeBase* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr;
	if (GameMode && GameMode->Implements<UNSRunGameModeInterface>())
	{
		INSRunGameModeInterface::Execute_NotifyStageCleared(GameMode);
	}

	UE_LOG(LogTemp, Log, TEXT("[RunEnd Debug] 강제 클리어 요청"));
}

void ANSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	// 테스트용 임시 코드 (재화 드랍 치트 — 드롭 테이블 연동 후 삭제)
	// 클라에는 CheatManager가 기본 생성되지 않으므로 강제로 생성
	EnableCheats();

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UNSUIManagerSubsystem* UIManager =
		GameInstance->GetSubsystem<UNSUIManagerSubsystem>();
	if (!UIManager)
	{
		return;
	}
	
	if (!UIManager)
	{
		return;
	}
	
	FString MapName = GetWorld()->GetName();
	
	// 레벨 상관없이 항상 비워야함
	UIManager->ClearPauseMenu();
	UIManager->ClearOptionPanel();

	// 현재 레벨이 타이틀일 때
	if (MapName.Contains(TEXT("Title")))
	{
		// 로딩창은 타이틀로 넘어갈때는 띄우지 않음
		UIManager->HideTravelLoadingScreen();
		
		UIManager->ClearTitle();
		UIManager->CreateTitle(this);
		UIManager->ShowTitle();
		UIManager->HideHUD();
		
		FInputModeUIOnly InputModeData;          
		SetInputMode(InputModeData);            
		bShowMouseCursor = true; 
		return;
	}
	// 현재 레벨이 아웃게임(거점)일 때
	else if (MapName.Contains(TEXT("HideOut")))
	{
		UIManager->HideTitle();
		UIManager->CreateHUD(this);
		UIManager->ShowHUD();
		if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
		{
			if (ANSPlayerState* NSPlayerState = GetPlayerState<ANSPlayerState>())
			{
				DataSubsystem->ApplyCachedProgressTo(
				NSPlayerState->GetProgressComponent());
			}
		}
		UIManager->ShowOutRunGoods();
			
		FInputModeGameOnly InputModeData;
		SetInputMode(InputModeData);
		bShowMouseCursor = false;
	}
	// 현재 레벨이 인 런일 때
	else
	{
		UIManager->HideTitle();
		UIManager->CreateHUD(this);
		UIManager->ShowHUD();
		UIManager->ResetRunResultStats();
		UIManager->ShowInRunGoods();
			
		FInputModeGameOnly InputModeData;
		SetInputMode(InputModeData);
		bShowMouseCursor = false;
	}
	
	// 맵 진입 직후 기존 Travel 로딩 상태가 남아 있으면 새 PlayerController 기준으로 위젯을 복구
	RestoreTravelLoadingScreenIfRequested();
	
	//HUD 생성 이후 Attribute 값 연결
	BindAttributeToHUD();
	UpdateHUDHealthAndShield();
	BindRunEndPhase();
	UpdateHUDAmmo();
	BindCurrencyToHUD();
	//UpdateSkillUIFromCurrentCharacter();
}

void ANSPlayerController::ShowTravelLoadingScreen()
{
	if (!IsLocalController())
	{
		return;
	}
	
	if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(this))
	{
		UIManager->ShowTravelLoadingScreen(this);
	}
}

void ANSPlayerController::RestoreTravelLoadingScreenIfRequested()
{
	// Seamless Travel에서는 PlayerController는 유지되지만, UMG 위젯은 viewport에서 빠지기 때문에 복원해야함
	if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(this))
	{
		UIManager->RestoreTravelLoadingScreen(this);
	}
}

void ANSPlayerController::HideTravelLoadingScreenIfPlayable(APawn* NewPawn)
{
	// Travel 직후 던전제너레이터 플러그인으로 인해 Spectator Pawn을 반드시 거치게 되므로
	// 실제 플레이어 캐릭터가 붙기 전에는 로딩창을 유지하는 것이 필요했음
	if (!IsLocalController() || !Cast<ANSPlayerCharacterBase>(NewPawn))
	{
		return;
	}
	
	if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(this))
	{
		UIManager->HideTravelLoadingScreen();
	}
}

void ANSPlayerController::SeamlessTravelTo(APlayerController* NewPC)
{
	Super::SeamlessTravelTo(NewPC);

	ANSPlayerController* NSNewPC = Cast<ANSPlayerController>(NewPC);
	if (!NSNewPC || !AugmentSelectionComponent || !NSNewPC->AugmentSelectionComponent)
	{
		return;
	}

	NSNewPC->AugmentSelectionComponent->CopyRunStateFrom(AugmentSelectionComponent);
}

void ANSPlayerController::PreClientTravel(
	const FString& PendingURL,
	ETravelType TravelType,
	bool bIsSeamlessTravel)
{
	// Travel 직전에 로컬 화면에 로딩창을 띄움
	ShowTravelLoadingScreen();
	Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);
}

void ANSPlayerController::ClientRestart_Implementation(class APawn* NewPawn){
	Super::ClientRestart_Implementation(NewPawn);

	// 이 함수는 클라이언트 본인 PC에서 실행되므로 IsLocalController()가 완벽하게 작동합니다.
	if (!IsLocalController()) return;

	// ClientRestart는 Seamless Travel 이후 다시 호출될 수 있으므로, 로딩창이 유지 중이면 먼저 복구한다.
	RestoreTravelLoadingScreenIfRequested();

	ClearDeathSpectatorModeTimer();
	SpectatingPlayerState = nullptr;

	// 사망 직후 첫 관전 대상을 결정하고 해당 화면 View를 볼 수 있게 수동으로 NextPlayer를 호출해줘야함
	if (NewPawn && NewPawn->IsA<ANSDeathSpectatorPawn>())
	{
		if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(this))
		{
			UIManager->CreateSpectator(this);
			UIManager->ShowSpectator(TEXT(""));
		}
		SetViewTarget(NewPawn);
		SpectateNextPlayer();
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UNSUIManagerSubsystem* UIManager =
		GameInstance->GetSubsystem<UNSUIManagerSubsystem>();
	if (!UIManager)
	{
		return;
	}

	//심리스 트레블 전 스테이지의 HUD 잔재를 안전하게 청소
	UIManager->HideSpectator();
	UIManager->ClearHUD();
	UIManager->ClearRunEnd();

	FString MapName = GetWorld()->GetName();

	//현재 로드된 맵이 타이틀이나 거점(HideOut)이 아닌 '진짜 인게임'일 때만 생성
	if (!MapName.Contains(TEXT("Title")))
	{
		UIManager->HideTitle();
        
		//청소된 상태이므로 nullptr 검사를 통과하고 새 HUD 위젯이 깔끔하게 생성됩니다.
		UIManager->CreateHUD(this);
		UIManager->ShowHUD();
		
		RebindHUDRuntimeState();

	// Spectator가 아니라 실제 플레이어 캐릭터에 붙은 순간부터 플레이 가능 상태로 보고 로딩창을 닫는다.
	HideTravelLoadingScreenIfPlayable(NewPawn);

	if (MapName.Contains(TEXT("HideOut")))
	{
		// 거점 복귀 후 새 PlayerState에 클라이언트 캐시 진행 데이터를 적용한다.
		ApplyCachedProgressToLocalPlayerState();

		// 거점에서는 아웃런 재화 UI 표시
		UIManager->ShowOutRunGoods();

		// PlayerState 초기화가 한 프레임 늦는 경우를 대비해서 다음 틱에 한 번 더 적용한다.
		GetWorldTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				ApplyCachedProgressToLocalPlayerState();

				UNSUIManagerSubsystem* NextTickUIManager =
					GetGameInstance()
					? GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>()
					: nullptr;

				if (NextTickUIManager)
				{
					NextTickUIManager->ShowOutRunGoods();
				}
			}));
	}
	else
	{
		// 인런에서는 인런 재화 UI 표시
		UIManager->ShowInRunGoods();
	}

	// 마우스 커서 및 입력 모드 제어
	FInputModeGameOnly InputModeData;
	SetInputMode(InputModeData);
	SetShowMouseCursor(false);

		if (ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(NewPawn))
		{
			if (UNSInputBinderComponent* InputBinder = PlayerCharacter->GetInputBinderComponent())
			{
				InputBinder->SetActiveInputModeTags(GetGameplayInputModeTags());
			}
		}

		//캐릭터가 새로 배치되었으니 체력/실드 등 GAS 어트리뷰트 값을 HUD에 연동
		BindAttributeToHUD();
		UpdateHUDHealthAndShield();
		UpdateHUDAmmo();
		BindCurrencyToHUD();
		
		GetWorldTimerManager().SetTimerForNextTick(
	this,
	&ANSPlayerController::UpdateHUDHealthAndShield);
		GetWorldTimerManager().SetTimerForNextTick(
	this,
	&ANSPlayerController::UpdateHUDAmmo);
		if (!MapName.Contains(TEXT("HideOut")))
		{
			GetWorldTimerManager().SetTimerForNextTick(
				this,
				&ANSPlayerController::UpdateSkillUIFromCurrentCharacter);
		}
	}
	if (MapName.Contains(TEXT("HideOut")))
	{
		// 클라가 CachedData 읽어 ChangeCharacterData로 적용
		UNSSaveGameSubsystem* SaveSubsystem =
		GameInstance->GetSubsystem<UNSSaveGameSubsystem>();
		if (SaveSubsystem)
		{
			if (SaveSubsystem->GetCachedPermanentData())
			{
				// 로드가 완료되었다면 즉시 복원
				RestoreLastSelectedCharacter();
			}
			else if (!PermanentDataLoadedHandle.IsValid())
			{
				// 로드가 아직 안되었다면 완료 후 복원되도록 바인딩
				PermanentDataLoadedHandle = SaveSubsystem->OnPermanentDataLoaded.AddUObject(
					this, &ANSPlayerController::HandlePermanentDataLoaded);
			}
		}
	}
}

void ANSPlayerController::HandlePermanentDataLoaded(UNSPermanentSaveGame* Data)
{
	// 로드 완료 후 1회만 복원하고 바인딩 해제
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UNSSaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UNSSaveGameSubsystem>())
		{
			SaveSubsystem->OnPermanentDataLoaded.Remove(PermanentDataLoadedHandle);
		}
	}
	PermanentDataLoadedHandle.Reset();

	RestoreLastSelectedCharacter();
}

void ANSPlayerController::Server_RequestStartRun_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	AGameModeBase* CurrentGameMode = GetWorld()->GetAuthGameMode();
	if (CurrentGameMode && CurrentGameMode->Implements<UNSOutGameInterface>())
	{
		INSOutGameInterface::Execute_RequestStartRun(CurrentGameMode);
	}
	
	//각 클라이언트에 인런 데이터 로드 지시
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ANSPlayerController* PC = Cast<ANSPlayerController>(It->Get()))
		{
			PC->Client_NotifyRunStarted();
		}
	}
}

void ANSPlayerController::Client_NotifyRunStarted_Implementation()
{
	if (UNSUIManagerSubsystem* UIManager =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>() : nullptr)
	{
		UIManager->ResetRunResultStats();
		UIManager->ShowHUD();
		UIManager->ShowInRunGoods();
	}
}

void ANSPlayerController::Client_NotifyReturnToHub_Implementation()
{
	if (UNSDataSubsystem* Data = UNSDataSubsystem::Get(this))
	{
		Data->ReturnToOutGame();
	}
}

void ANSPlayerController::ExitSpectatorAndRespawn()
{
	ClearDeathSpectatorModeTimer();
	SpectatingPlayerState = nullptr;

	if (!HasAuthority())
	{
		return;
	}

	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (!GameMode)
	{
		return;
	}
	
	// 직접 소환
	AActor* PlayerStartSpot = GameMode->FindPlayerStart(this);
	if (!PlayerStartSpot)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerStart를 찾을 수 없음"));
		return;
	}

	APawn* NewPawn = GameMode->SpawnDefaultPawnFor(this, PlayerStartSpot);
	if (!NewPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("폰 스폰 실패"));
		return;
	}
	
	Possess(NewPawn);
	
	// 트래블 전 남아있던 회전값 초기화
	ClientSetRotation(PlayerStartSpot->GetActorRotation(), true);
	
	Multicast_NotifyRespawn();
}

void ANSPlayerController::Multicast_NotifyRespawn_Implementation()
{
	if (IsLocalController())
	{
		// 호스트 클라이언트 리스폰 처리용
		if (GetPawn())
		{
			SetViewTargetWithBlend(GetPawn());
		}
		
		// 로딩 UI 종료용
		if (GetGameInstance() && GetGameInstance()->Implements<UNSGameInstanceInterface>())
		{
			INSGameInstanceInterface::Execute_HideLoadingScreen(GetGameInstance());
		}
	}
	
	
}

void ANSPlayerController::Server_ConfirmVote_Implementation(ENSRunChoice Choice)
{
	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (GameMode && GameMode->Implements<UNSRunGameModeInterface>())
	{
		INSRunGameModeInterface::Execute_SubmitRunChoice(GameMode,this, Choice);
	}
}

void ANSPlayerController::RequestEnterDeathSpectatorMode()
{
	if (!IsLocalController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ClearDeathSpectatorModeTimer();
	
	if (DeathSpectatorModeDelay <= 0.f)
	{
		EnterDeathSpectatorMode();
		return;
	}

	// 사망 후 DeathSpectatorModeDelay(기본 2초) 시간 이후에 관전자 모드로 진입
	World->GetTimerManager().SetTimer(
		DeathSpectatorModeTimerHandle,
		this,
		&ThisClass::EnterDeathSpectatorMode,
		DeathSpectatorModeDelay,
		false
	);
}

void ANSPlayerController::EnterDeathSpectatorMode()
{
	if (!IsLocalController())
	{
		return;
	}

	if (HasAuthority())
	{
		SpawnAndPossessDeathSpectatorPawn();
		SpectateNextPlayer();
		return;
	}

	// 사망 관전자 모드 Input 태그에 따라서 InputConfig 안에 있는 IMC를 골라서 교체
	Server_EnterDeathSpectatorMode();
	SpectateNextPlayer();
}

void ANSPlayerController::ClearDeathSpectatorModeTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathSpectatorModeTimerHandle);
	}
}

void ANSPlayerController::SpectatePreviousPlayer()
{
	SwitchSpectatorTarget(-1);
}

void ANSPlayerController::SpectateNextPlayer()
{
	SwitchSpectatorTarget(1);
}

void ANSPlayerController::SwitchSpectatorTarget(int32 Direction)
{
	if (!IsLocalController())
	{
		return;
	}
	
	const ANSPlayerState* ViewerPlayerState = GetPlayerState<ANSPlayerState>();
	const ANSRunGameState* RunGameState = GetWorld() ? GetWorld()->GetGameState<ANSRunGameState>() : nullptr;
	if (!ViewerPlayerState || !RunGameState)
	{
		return;
	}
	
	TArray<ANSPlayerState*> AlivePlayerStates;
	// GameState에서 PlayerState를 순회해서 살아있는 Player를 찾음
	RunGameState->GetAlivePlayerStates(AlivePlayerStates, ViewerPlayerState);
	if (AlivePlayerStates.IsEmpty())
	{
		return;
	}
	
	int32 CurrentIndex = AlivePlayerStates.IndexOfByKey(SpectatingPlayerState.Get());
	if (CurrentIndex == INDEX_NONE)
	{
		CurrentIndex = Direction >= 0 ? -1 : 0;
	}
	
	const int32 TargetIndex = (CurrentIndex + Direction + AlivePlayerStates.Num()) % AlivePlayerStates.Num();
	SetSpectatorTarget(AlivePlayerStates[TargetIndex]);
}

void ANSPlayerController::SetSpectatorTarget(ANSPlayerState* NewSpectatorTarget)
{
	if (!NewSpectatorTarget)
	{
		return;
	}

	SpectatingPlayerState = NewSpectatorTarget;
	
	// 관전 대상을 찾은 후라서 해당 TargetCharacter의 SpectatorViewComponent를 보고 카메라 정보를 받아 설정하는 부분
	ANSDeathSpectatorPawn* DeathSpectatorPawn = Cast<ANSDeathSpectatorPawn>(GetPawn());
	ANSPlayerCharacterBase* TargetCharacter = Cast<ANSPlayerCharacterBase>(GetPawnFromPlayerState(NewSpectatorTarget));
	UNSSpectatorViewComponent* TargetSpectatorView =
		TargetCharacter ? TargetCharacter->GetSpectatorViewComponent() : nullptr;
	if (DeathSpectatorPawn)
	{
		DeathSpectatorPawn->SetSpectatorView(TargetSpectatorView);
	}
	UE_LOG(LogTemp, Log, TEXT("관전 대상 : %s"), *NewSpectatorTarget->GetPlayerName());
	if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(this))
	{
		UIManager->ShowSpectator(NewSpectatorTarget->GetPlayerName());
	}
}

APawn* ANSPlayerController::GetPawnFromPlayerState(const ANSPlayerState* TargetPlayerState) const
{
	if (!TargetPlayerState)
	{
		return nullptr;
	}

	if (APawn* TargetPawn = TargetPlayerState->GetPawn())
	{
		return TargetPawn;
	}

	const AController* OwningController = Cast<AController>(TargetPlayerState->GetOwner());
	return OwningController ? OwningController->GetPawn() : nullptr;
}

void ANSPlayerController::Server_EnterDeathSpectatorMode_Implementation()
{
	SpawnAndPossessDeathSpectatorPawn();
}

void ANSPlayerController::SpawnAndPossessDeathSpectatorPawn()
{
	if (!HasAuthority())
	{
		return;
	}

	if (GetPawn() && GetPawn()->IsA<ANSDeathSpectatorPawn>())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !DeathSpectatorPawnClass)
	{
		return;
	}

	APawn* PreviousPawn = GetPawn();
	const FVector SpectatorSpawnLocation = PreviousPawn ? PreviousPawn->GetActorLocation() : FVector::ZeroVector;
	const FRotator SpectatorSpawnRotation = PreviousPawn ? PreviousPawn->GetActorRotation() : GetControlRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = PreviousPawn;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ANSDeathSpectatorPawn* DeathSpectatorPawn = World->SpawnActor<ANSDeathSpectatorPawn>(
		DeathSpectatorPawnClass,
		SpectatorSpawnLocation,
		SpectatorSpawnRotation,
		SpawnParams
	);
	
	if (!DeathSpectatorPawn)
	{
		return;
	}

	Possess(DeathSpectatorPawn);
	SetViewTarget(DeathSpectatorPawn);
}
void ANSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// 디버그 : O키로 풀 적재, TODO : 테스트 끝나면 삭제
	InputComponent->BindKey(EKeys::O, IE_Pressed, this, &ANSPlayerController::Debug_EnqueueAugmentOffer);
	// 디버그: k키로 런 클리어 화면 확인, TODO: 테스트 종료 후 제거
	InputComponent->BindKey(EKeys::K, IE_Pressed, this, &ANSPlayerController::Debug_ForceRunClear);
	// esc키로 퍼즈메뉴 띄우기
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ANSPlayerController::TogglePauseMenu);
}

void ANSPlayerController::ToggleAugmentationPanel()
{
	//인런에서만 탭 사용
	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Data || !Data->IsRunReady())
	{
		return;
	}
	
	UNSUIManagerSubsystem* UIManager = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>() : nullptr;
	if (!UIManager)
	{
		return;
	}

	if (UIManager->IsAugmentationPanelOpen())
	{
		// 열려있으면 닫음 (토글)
		UIManager->CloseAugmentationPanel();
		UIManager->ClosePartPanel();
	}
	else
	{
		// 패널 UI 표시
		UIManager->OpenAugmentationPanel();
		UIManager->OpenPartPanel();
		// 서버에 대기열 front 오퍼 표시 요청 (대기 있으면 카드가 옴)
		if (AugmentSelectionComponent)
		{
			AugmentSelectionComponent->Server_OpenPanel();
		}
	}
}

void ANSPlayerController::EnterRunEndInputMode()
{
	// 살아있는 캐릭터면 게임플레이 매핑을 제거하고 UI 태그만 남김
	if (ANSPlayerCharacterBase* Char =
		Cast<ANSPlayerCharacterBase>(GetPawn()))
	{
		if (UNSInputBinderComponent* InputBinder = 
			Char->GetInputBinderComponent())
		{
			FGameplayTagContainer UIOnly;
			// 게임플레이 태그 제외
			UIOnly.AddTag(NSGameplayTags::InputMode_UI); 
			InputBinder->SetActiveInputModeTags(UIOnly);
		}
	}
	SetInputMode(FInputModeUIOnly());
	bShowMouseCursor = true;
}

void ANSPlayerController::ExitRunEndInputMode()
{
	if (ANSPlayerCharacterBase* Char =
		Cast<ANSPlayerCharacterBase>(GetPawn()))
	{
		if (UNSInputBinderComponent* InputBinder =
			Char->GetInputBinderComponent())
		{
			InputBinder->SetActiveInputModeTags(GetGameplayInputModeTags());
		}
	}
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void ANSPlayerController::OpenInteractionWidget(ANSInteractableNPCBase* NPC)
{
	CloseInteractionWidget();

	if (!NPC)
	{
		return;
	}

	TSubclassOf<UNSNPCInteractionWidgetBase> WidgetClass = NPC->GetInteractionWidgetClass();
	if (!WidgetClass)
	{
		return;
	}

	UNSNPCInteractionWidgetBase* Widget = CreateWidget<UNSNPCInteractionWidgetBase>(this, WidgetClass);
	if (!Widget)
	{
		return;
	}

	ActiveInteractionWidget = Widget;
	Widget->OpenForInteractor(this);
}

void ANSPlayerController::CloseInteractionWidget()
{
	if (ActiveInteractionWidget)
	{
		ActiveInteractionWidget->CloseWidget();
		ActiveInteractionWidget = nullptr;
	}
}

void ANSPlayerController::TryInteract()
{
	if (bCharacterSelectOpen)
	{
		return;
	}

	ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(GetPawn());
	if (!PlayerCharacter)
	{
		return;
	}

	UNSInteractionComponent* Interaction = PlayerCharacter->FindComponentByClass<UNSInteractionComponent>();
	if (!Interaction)
	{
		return;
	}

	Interaction->TryInteract();
}

void ANSPlayerController::Server_UploadProgress_Implementation(const FNSProgressPayload& Payload)
{
	ANSPlayerState* OwningPlayerState = GetPlayerState<ANSPlayerState>();
	if (!OwningPlayerState)
	{
		return;
	}

	UNSPlayerProgressComponent* ProgressComponent = OwningPlayerState->GetProgressComponent();
	if (!ProgressComponent)
	{
		return;
	}

	ProgressComponent->ApplyPayload(Payload);
	
	if (!Payload.ActiveCharacterId.IsNone())
	{
		const FPrimaryAssetType CharacterType =
			OwningPlayerState->GetDefaultCharacterDataId().PrimaryAssetType;
		const FPrimaryAssetId NewCharacterDataId(
	CharacterType,
	Payload.ActiveCharacterId);
		OwningPlayerState->SetCurrentCharacterDataId(NewCharacterDataId);
	}
	
	// 업로드로 서버 ProgressComponent가 갱신된 직후, 현재 폰이 있으면 즉시 적용
	if (ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(GetPawn()))
	{
		PlayerCharacter->ApplyEquippedPart();
	}
	
	// 컴패니언: 선택 태그 런타임 동기화
	const FGameplayTag SelectedCompanionTag = ProgressComponent->GetSelectedCompanion();
	if (SelectedCompanionTag.IsValid())
	{
		OwningPlayerState->SetCurrentCompanionDefinitionTag(SelectedCompanionTag);
	}

	// 컴패니언: 살아있는 드론에 정의+노드 즉시 반영 (허브 라이브 변경/업그레이드)
	if (UNSCompanionProgressionComponent* CompanionProg = OwningPlayerState->GetCompanionProgressionComponent())
	{
		UNSCompanionDefinition* SelectedDefinition = OwningPlayerState->GetCurrentCompanionDefinition();
		CompanionProg->ApplySelectedAndNodes(SelectedDefinition, ProgressComponent->GetCompanionNodeLevels());
	}
	
	const FGameplayTag SelectedTag = ProgressComponent->GetSelectedCompanion();
	if (SelectedTag.IsValid())
	{
		OwningPlayerState->SetCurrentCompanionDefinitionTag(SelectedTag);
	}
}

void ANSPlayerController::Client_SaveProgress_Implementation(const FNSProgressPayload& Payload)
{
	if (UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this))
	{
		DataSubsystem->SetCachedProgressPayload(Payload);

		if (ANSPlayerState* PS = GetPlayerState<ANSPlayerState>())
		{
			DataSubsystem->ApplyCachedProgressTo(PS->GetProgressComponent());
		}
	}
	
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UNSSaveGameSubsystem* SaveSubsystem =
		GameInstance->GetSubsystem<UNSSaveGameSubsystem>();
	if (!SaveSubsystem)
	{
		return;
	}

	UNSPermanentSaveGame* PermanentSave = SaveSubsystem->GetCachedPermanentData();
	if (!PermanentSave)
	{
		return;
	}

	// 계정 단위 덮어쓰기
	PermanentSave->CommonCurrency = Payload.CommonCurrency;
	PermanentSave->UnlockedNPCIds = TSet<FName>(Payload.UnlockedNPCIds);

	PermanentSave->CommonSkillLevels.Reset();
	for (const FNSNodeLevel& NodeLevel : Payload.CommonSkillLevels)
	{
		PermanentSave->CommonSkillLevels.Add(NodeLevel.NodeId, NodeLevel.Level);
	}

	PermanentSave->PetUpgradeLevels.Reset();
	for (const FNSNodeLevel& NodeLevel : Payload.PetUpgradeLevels)
	{
		PermanentSave->PetUpgradeLevels.Add(NodeLevel.NodeId, NodeLevel.Level);
	}

	// 활성 캐릭터 슬롯 갱신
	if (!Payload.ActiveCharacterId.IsNone())
	{
		FNSCharacterSaveData& CharacterSlot =
			PermanentSave->Characters.FindOrAdd(Payload.ActiveCharacterId);
		CharacterSlot.JobCurrency = Payload.JobCurrency;
		CharacterSlot.CharacterSkillLevels.Reset();
		for (const FNSNodeLevel& NodeLevel : Payload.CharacterSkillLevels)
		{
			CharacterSlot.CharacterSkillLevels.Add(NodeLevel.NodeId, NodeLevel.Level);
		}
		PermanentSave->LastSelectedCharacterId = Payload.ActiveCharacterId;
	}

	// CachedData를 그대로 넘기므로 머지 분기 없이 그대로 저장됨
	SaveSubsystem->SavePermanent(PermanentSave, FNSSaveComplete());
	
	if (UNSUIManagerSubsystem* UIManager = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>()
		: nullptr)
	{
		UIManager->RefreshOutRunGoods();
	}
}

void ANSPlayerController::UploadLocalProgress(FName SelectedCharacterId)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UNSSaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UNSSaveGameSubsystem>();
	if (!SaveSubsystem)
	{
		return;
	}

	UNSPermanentSaveGame* PermanentSave = SaveSubsystem->GetCachedPermanentData();
	if (!PermanentSave)
	{
		return;
	}

	FNSProgressPayload Payload;

	// 계정 단위
	Payload.CommonCurrency = PermanentSave->CommonCurrency;
	Payload.UnlockedNPCIds = PermanentSave->UnlockedNPCIds.Array();
	for (const TPair<FName, int32>& SkillPair : PermanentSave->CommonSkillLevels)
	{
		FNSNodeLevel NodeLevel;
		NodeLevel.NodeId = SkillPair.Key;
		NodeLevel.Level = SkillPair.Value;
		Payload.CommonSkillLevels.Add(NodeLevel);
	}
	for (const TPair<FName, int32>& PetPair : PermanentSave->PetUpgradeLevels)
	{
		FNSNodeLevel NodeLevel;
		NodeLevel.NodeId = PetPair.Key;
		NodeLevel.Level = PetPair.Value;
		Payload.PetUpgradeLevels.Add(NodeLevel);
	}
	
	Payload.SelectedCompanionTag = PermanentSave->Companion.SelectedCompanionTag;
	for (const TPair<FGameplayTag, int32>& NodeLevelPair : PermanentSave->Companion.NodeLevels)
	{
		FNSCompanionNodeLevel NodeLevelEntry;
		NodeLevelEntry.Tag   = NodeLevelPair.Key;
		NodeLevelEntry.Level = NodeLevelPair.Value;
		Payload.CompanionNodeLevels.Add(NodeLevelEntry);
	}

	// 활성 캐릭터 슬롯
	Payload.ActiveCharacterId = SelectedCharacterId;

	
	const FNSCharacterSaveData* CharacterSlot = PermanentSave->Characters.Find(SelectedCharacterId);
	if (CharacterSlot)
	{
		Payload.JobCurrency = CharacterSlot->JobCurrency;
		// 캐릭터 장착 참조(정의,등급)로 계정 인벤토리에서 풀 데이터(값,강화) 해석
		if (CharacterSlot && !CharacterSlot->EquippedPartDefinition.IsNull())
		{
			const FNSPartSaveData* Owned = PermanentSave->OwnedParts.FindByPredicate(
				[CharacterSlot](const FNSPartSaveData& P)
				{
					return P.Definition == CharacterSlot->EquippedPartDefinition
						&& P.Rarity == CharacterSlot->EquippedPartRarity;
				});
			if (Owned)
			{
				Payload.EquippedPart = *Owned;
			}
		}
		for (const TPair<FName, int32>& SkillPair : CharacterSlot->CharacterSkillLevels)
		{
			FNSNodeLevel NodeLevel;
			NodeLevel.NodeId = SkillPair.Key;
			NodeLevel.Level = SkillPair.Value;
			Payload.CharacterSkillLevels.Add(NodeLevel);
		}
	}

	Server_UploadProgress(Payload);
}

void ANSPlayerController::SaveProgressToOwningClient()
{
	ANSPlayerState* OwningPlayerState = GetPlayerState<ANSPlayerState>();
	if (!OwningPlayerState)
	{
		return;
	}

	UNSPlayerProgressComponent* ProgressComponent = OwningPlayerState->GetProgressComponent();
	if (!ProgressComponent)
	{
		return;
	}

	FNSProgressPayload Payload;
	ProgressComponent->BuildPayload(Payload);
	Client_SaveProgress(Payload);
}

void ANSPlayerController::CommitCharacterSelection(UNSCharacterData* SelectedCharacterData)
{
	if (!SelectedCharacterData)
	{
		return;
	}

	// 캐릭터 변경
	if (ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(GetPawn()))
	{
		PlayerCharacter->ChangeCharacterData(SelectedCharacterData);
	}
	
	// 캐릭터 변경에 맞춰 스킬 슬롯 UI 갱신
	if (UNSUIManagerSubsystem* UIManager =
		GetGameInstance()
		? GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>()
		: nullptr)
	{

		UIManager->ApplyCharacterSkillUISet(
			SelectedCharacterData->CharacterTag.GetTagName());
	}

	const FName SelectedKey = SelectedCharacterData->GetPrimaryAssetId().PrimaryAssetName;
	
	// 최근 선택 캐릭터 로컬 저장
	UGameInstance* GameInstance = GetGameInstance();
	UNSSaveGameSubsystem* SaveSubsystem =
		GameInstance ? GameInstance->GetSubsystem<UNSSaveGameSubsystem>() : nullptr;
	if (SaveSubsystem)
	{
		UNSPermanentSaveGame* PermanentSave = SaveSubsystem->GetCachedPermanentData();
		if (PermanentSave)
		{
			PermanentSave->LastSelectedCharacterId = SelectedKey;
			SaveSubsystem->SavePermanent(PermanentSave, FNSSaveComplete());
		}
	}

	// 서버 업로드 (방금 고른 키를 직접 전달)
	UploadLocalProgress(SelectedKey);
}

void ANSPlayerController::RestoreLastSelectedCharacter()
{
	UGameInstance* GameInstance = GetGameInstance();
	UNSSaveGameSubsystem* SaveSubsystem =
		GameInstance ? GameInstance->GetSubsystem<UNSSaveGameSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		return;
	}

	UNSPermanentSaveGame* PermanentSave = SaveSubsystem->GetCachedPermanentData();
	if (!PermanentSave || PermanentSave->LastSelectedCharacterId.IsNone())
	{
		return;
	}
	
	ANSPlayerState* OwningPlayerState = GetPlayerState<ANSPlayerState>();
	if (!OwningPlayerState)
	{
		return;
	}

	// FName -> FPrimaryAssetId
	const FPrimaryAssetType CharacterType =
		OwningPlayerState->GetDefaultCharacterDataId().PrimaryAssetType;
	const FPrimaryAssetId RestoredId(CharacterType, PermanentSave->LastSelectedCharacterId);

	const FSoftObjectPath DataPath = UAssetManager::Get().GetPrimaryAssetPath(RestoredId);
	if (!DataPath.IsValid())
	{

		return;
	}

	UNSCharacterData* RestoredData = Cast<UNSCharacterData>(DataPath.TryLoad());
	if (!RestoredData)
	{

		return;
	}

	// 동일 커밋 경로 재사용
	CommitCharacterSelection(RestoredData);
	
	if (IsLocalController())
	{
		const FGameplayTag RestoredCharacterTag =
			RestoredData->CharacterTag;

		GetWorldTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(
				this,
				[this, RestoredCharacterTag]()
				{
					if (!RestoredCharacterTag.IsValid())
					{
						return;
					}

					if (UNSUIManagerSubsystem* UIManager = UNSUIManagerSubsystem::Get(this))
					{
						UIManager->ApplyCharacterSkillUISet(
							RestoredCharacterTag.GetTagName());
					}
				}));
	}
}

void ANSPlayerController::EquipPartLive(FName CharacterId, TSoftObjectPtr<UNSPartDefinition> Definition, ENSPartRarity Rarity)
{
	// 파사드는 클라 로컬 저장이라 로컬 컨트롤러에서만
	if (!IsLocalController())
	{
		return; 
	}
	
	UNSProgressionSubsystem* Prog = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UNSProgressionSubsystem>()
		: nullptr;
	if (!Prog)
	{
		return;
	}
	
	// 로컬 CachedData 저장
	Prog->SetEquippedPart(CharacterId, Definition, Rarity);   
	// 서버 업로드
	UploadLocalProgress(CharacterId);       
}

void ANSPlayerController::Client_NotifySkillCooldownChanged_Implementation(
	const FNSSkillCooldownMessage& Message)
{
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(
		NSGameplayTags::Message_UI_SkillCooldown_Changed,
		Message);
}

void ANSPlayerController::Client_PlayAttackHitFeedback_Implementation(const FNSHitFeedbackContext& Context)
{
	// 로컬 Pawn의 피드백 컴포넌트로 공격 결과를 전달
	if (!IsLocalController())
	{
		return;
	}
	
	ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(GetPawn());
	if (!PlayerCharacter)
	{
		return;
	}
	
	UNSPlayerAttackFeedbackComponent* FeedbackComponent =
		PlayerCharacter->GetPlayerAttackFeedbackComponent();
	if (!FeedbackComponent)
	{
		return;
	}
	
	// Context에 따라 Feedback 재생흐름으로 진입
	FeedbackComponent->HandleAttackHitFeedback(Context);
}

void ANSPlayerController::Debug_EnqueueAugmentOffer()
{
	if (!AugmentSelectionComponent)
	{
		return;
	}

	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Data)
	{
		return;
	}

	// 인런에서만 동작 (런 데이터 준비 전이면 무시)
	if (!Data->IsRunReady())
	{
		return;
	}
	
	if (HasAuthority())
	{
		AugmentSelectionComponent->EnqueueOffer(NSGameplayTags::Reward_Trigger_EliteKill);
	}
	else
	{
		AugmentSelectionComponent->Server_EnqueueOffer(NSGameplayTags::Reward_Trigger_EliteKill);
	}
}

void ANSPlayerController::UnbindAttributeFromHUD()
{
	if (UAbilitySystemComponent* ASC = CachedHUDASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNSBaseAttributeSet::GetHealthAttribute()).RemoveAll(this);
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNSBaseAttributeSet::GetMaxHealthAttribute()).RemoveAll(this);
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNSPlayerAttributeSet::GetShieldAttribute()).RemoveAll(this);
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNSPlayerAttributeSet::GetMaxShieldAttribute()).RemoveAll(this);
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNSPlayerAttributeSet::GetAmmoAttribute()).RemoveAll(this);
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNSPlayerAttributeSet::GetMaxAmmoAttribute()).RemoveAll(this);

		ASC->RegisterGameplayTagEvent(
			NSGameplayTags::State_Reloading,
			EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
	}

	CachedHUDASC.Reset();
	bHUDAttributeBound = false;
}

void ANSPlayerController::RebindHUDRuntimeState()
{
	if (!IsLocalController())
	{
		return;
	}

	ApplyCachedProgressToLocalPlayerState();

	BindAttributeToHUD();
	BindCurrencyToHUD();

	UpdateHUDHealthAndShield();
	UpdateHUDAmmo();
	UpdateHUDCurrency();
	//UpdateSkillUIFromCurrentCharacter();

	if (UNSUIManagerSubsystem* UIManager = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>()
		: nullptr)
	{
		UIManager->RefreshOutRunGoods();
	}
}

void ANSPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	RebindHUDRuntimeState();
}

void ANSPlayerController::BeginPlayingState()
{
	Super::BeginPlayingState();
	RebindHUDRuntimeState();
}

#pragma region CompanionCheat

void ANSPlayerController::CompanionCheatUpgrade(FGameplayTag NodeTag)
{
	// 로컬 파사드 경로
	if (!IsLocalController() || !NodeTag.IsValid())
	{
		return;
	}   
	
	UNSProgressionSubsystem* Prog = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UNSProgressionSubsystem>() : nullptr;
	if (!Prog)
	{
		return;
	}

	const FGameplayTag SelectedCompanionTag = Prog->GetSelectedCompanion();
	// 치트: Max/Cost 게이트 통과용 임의값. NodeTag와 CompanionTag(=선택드론)는 분리해 전달
	Prog->UpgradeCompanionNode(SelectedCompanionTag, NodeTag, /*MaxLevel*/9999, /*Cost*/0);
	UploadLocalProgress(GetActiveCharacterIdForUpload());  
}

void ANSPlayerController::CompanionCheatSelect(FGameplayTag CompanionTag)
{
	if (!IsLocalController() || !CompanionTag.IsValid())
	{
		return;
	}
	
	UNSProgressionSubsystem* Prog = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UNSProgressionSubsystem>() : nullptr;
	if (!Prog)
	{
		return;
	}

	// 치트: 해금 게이트 무시 위해 RequiredTag 무효 + Count 0
	Prog->SelectCompanion(CompanionTag, FGameplayTag(), 0);
	UploadLocalProgress(GetActiveCharacterIdForUpload());
}

FName ANSPlayerController::GetActiveCharacterIdForUpload() const
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return NAME_None;
	}
	
	UNSSaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UNSSaveGameSubsystem>();
	if (!SaveSubsystem)
	{
		return NAME_None;
	}
	
	UNSPermanentSaveGame* PermanentSave = SaveSubsystem->GetCachedPermanentData();
	
	return PermanentSave ? PermanentSave->LastSelectedCharacterId : NAME_None;
}

#pragma endregion CompanionCheat

void ANSPlayerController::TogglePauseMenu()
{
	if (!IsLocalController())
	{
		return;
	}
	
	if (UWorld* World = GetWorld())
	{
		if (World->GetName().Contains(TEXT("Title")))
		{
			return;
		}
	}
	
	UNSUIManagerSubsystem* UIManager = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>() : nullptr;
	if (!UIManager)
	{
		return;
	}

	if (UIManager->IsOptionPanelOpen())
	{
		UIManager->CloseOptionPanel();
		
		return;
	}
	
	// 폰 인풋바인더 캐싱 (열기/닫기 공통)
	UNSInputBinderComponent* InputBinder = nullptr;
	if (ANSPlayerCharacterBase* Char = Cast<ANSPlayerCharacterBase>(GetPawn()))
	{
		InputBinder = Char->GetInputBinderComponent();
	}

	if (UIManager->IsPauseMenuOpen())
	{
		// 닫기: 게임플레이 매핑 복원
		UIManager->ClosePauseMenu();
		if (InputBinder)
		{
			FGameplayTagContainer GameplayAndUI;
			GameplayAndUI.AddTag(NSGameplayTags::InputMode_Gameplay);
			GameplayAndUI.AddTag(NSGameplayTags::InputMode_UI);
			InputBinder->SetActiveInputModeTags(GameplayAndUI);
		}
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
	else
	{
		// 열기: 게임플레이 매핑 제거, UI
		UIManager->OpenPauseMenu(this);
		if (InputBinder)
		{
			FGameplayTagContainer UIOnly;
			UIOnly.AddTag(NSGameplayTags::InputMode_UI);
			InputBinder->SetActiveInputModeTags(UIOnly);
		}
		FInputModeGameAndUI InputMode;                 
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
}

void ANSPlayerController::RequestLeaveToMainMenu()
{
	if (!IsLocalController())
	{
		return;
	}
	
	if (UNSUIManagerSubsystem* UIManager = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>() : nullptr)
	{
		UIManager->ClosePauseMenu();
		UIManager->CloseOptionPanel();
	}
	// 세션 정리 + 타이틀
	if (UNSSessionSubsystem* Session = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UNSSessionSubsystem>() : nullptr)
	{
		Session->LeaveSessionToTitle();
	}
}

void ANSPlayerController::RestoreGameplayInputMode()
{
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
}
