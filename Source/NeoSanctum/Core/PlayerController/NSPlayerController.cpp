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
#include "NeoSanctum/System/NSSaveGameSubsystem.h"
#include "NeoSanctum/Progression/Save/NSPermanentSaveGame.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "NeoSanctum/Interaction/NPC/NSInteractableNPCBase.h"
#include "EngineUtils.h"
#include "NeoSanctum/Progression/Augment/NSAugmentSelectionComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Augment.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"
#include "NeoSanctum/UI/CharacterSelect/NSCharacterSelectWidget.h"
#include "NeoSanctum/GAS/AttributeSet/NsPlayerAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Input.h"
#include "InputCoreTypes.h"
#include "Engine/GameInstance.h"
#include "NeoSanctum/Interaction/Component/NSInteractionComponent.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "NeoSanctum/Data/Character/NSCharacterData.h"
#include "Engine/AssetManager.h"
#include "NeoSanctum/Core/Cheat/NSCheatManager.h"
#include "NeoSanctum/System/Subsystem/NSCurrencyDropSubsystem.h"
#include "NeoSanctum/Progression/Currency/NSCurrencyComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Currency.h"
#include "NeoSanctum/Data/Progression/Currency/NSCurrencyTypes.h"


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

// ===== 테스트용 임시 코드 — 인런 구출 NPC(M3) 구현 후 삭제 =====
void ANSPlayerController::Debug_UnlockAllNPCs()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ANSPlayerState* PS = GetPlayerState<ANSPlayerState>();
	if (!PS)
	{
		return;
	}

	UNSPlayerProgressComponent* Progress = PS->GetProgressComponent();
	if (!Progress)
	{
		return;
	}

	int32 Count = 0;
	for (TActorIterator<ANSInteractableNPCBase> It(World); It; ++It)
	{
		Progress->UnlockNPC(It->GetNPCId());
		++Count;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Debug] 거점 NPC %d개 해금 — 범위 밖으로 나갔다 다시 들어오면 프롬프트 표시"), Count);
}
// ===== 테스트용 임시 코드 끝 =====

void ANSPlayerController::BindAttributeToHUD()
{
	if (!IsLocalController() || bHUDAttributeBound)
	{
		return;
	}
	ANSPlayerState*NSPlayerState = GetPlayerState<ANSPlayerState>();
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
	// 현재 Attribute 값을 로그로 확인
	UE_LOG(LogTemp, Warning, TEXT("HP: %.0f / %.0f, Shield: %.0f / %.0f"),
		PlayerAttributeSet->GetHealth(),
		PlayerAttributeSet->GetMaxHealth(),
		PlayerAttributeSet->GetShield(),
		PlayerAttributeSet->GetMaxShield()
	);

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
		UIManager->CacheRunResultTime();
		UIManager->UpdateRunEndResult(CachedRunGameState->bIsClear);
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
	if (!CurrentCharacterData)
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

		// 현재 레벨이 타이틀일 때
		if (MapName.Contains(TEXT("Title")))
		{
			UIManager->CreateTitle(this);
			UIManager->ShowTitle();
			UIManager->HideHUD();
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
	
	//HUD 생성 이후 Attribute 값 연결
	BindAttributeToHUD();
	UpdateHUDHealthAndShield();
	BindRunEndPhase();
	UpdateHUDAmmo();
	BindCurrencyToHUD();
	//UpdateSkillUIFromCurrentCharacter();
}

void ANSPlayerController::ClientRestart_Implementation(class APawn* NewPawn){
	Super::ClientRestart_Implementation(NewPawn);

	// 이 함수는 클라이언트 본인 PC에서 실행되므로 IsLocalController()가 완벽하게 작동합니다.
	if (!IsLocalController()) return;

	ClearDeathSpectatorModeTimer();
	SpectatingPlayerState = nullptr;

	// 사망 직후 첫 관전 대상을 결정하고 해당 화면 View를 볼 수 있게 수동으로 NextPlayer를 호출해줘야함
	if (NewPawn && NewPawn->IsA<ANSDeathSpectatorPawn>())
	{
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
		//UpdateSkillUIFromCurrentCharacter();
		GetWorldTimerManager().SetTimerForNextTick(
	this,
	&ANSPlayerController::UpdateHUDHealthAndShield);
		GetWorldTimerManager().SetTimerForNextTick(
	this,
	&ANSPlayerController::UpdateHUDAmmo);
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

	//서버 인런 데이터 로드
	if (UNSDataSubsystem* Data = UNSDataSubsystem::Get(this))
	{
		Data->EnterRun();
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
	if (UNSDataSubsystem* Data = UNSDataSubsystem::Get(this))
	{
		Data->EnterRun();
	}
	if (UNSUIManagerSubsystem* UIManager =
		GetGameInstance() ? GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>() : nullptr)
	{
		UIManager->ResetRunResultStats();
		UIManager->ShowHUD();
		UIManager->ShowInRunGoods();
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
	// 업로드로 서버 ProgressComponent가 갱신된 직후, 현재 폰이 있으면 즉시 적용
	if (ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(GetPawn()))
	{
		PlayerCharacter->ApplyEquippedPart();
	}
}

void ANSPlayerController::Client_SaveProgress_Implementation(const FNSProgressPayload& Payload)
{
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
		AugmentSelectionComponent->EnqueueOffer(NSGameplayTags::Augment_Pool_HighGrade);
	}
	else
	{
		AugmentSelectionComponent->Server_EnqueueOffer(NSGameplayTags::Augment_Pool_HighGrade);
	}
}
