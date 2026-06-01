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
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Progression/Augment/NSAugmentSelectionComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Augment.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"
#include "NeoSanctum/UI/Core/NSUIManagerSubsystem.h"
#include "NeoSanctum/GAS/AttributeSet/NsPlayerAttributeSet.h"
#include "NeoSanctum/Tag/NSGameplayTags_Input.h"
#include "InputCoreTypes.h"
#include "Engine/GameInstance.h"

ANSPlayerController::ANSPlayerController()
{
	// 기본 태그 초기화
	DeathSpectatorPawnClass = ANSDeathSpectatorPawn::StaticClass();

	GameplayInputModeTags.AddTag(NSGameplayTags::InputMode_Gameplay);
	GameplayInputModeTags.AddTag(NSGameplayTags::InputMode_UI);
	
	// 사망 상태 태그 초기화
	DeathSpectatorInputModeTags.AddTag(NSGameplayTags::InputMode_DeathSpectator);
	DeathSpectatorInputModeTags.AddTag(NSGameplayTags::InputMode_UI);

	// 증강 선택 컴포넌트 생성
	AugmentSelectionComponent = CreateDefaultSubobject<UNSAugmentSelectionComponent>(TEXT("AugmentSelectionComponent"));
}

void ANSPlayerController::BindAttributeToHUD()
{
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
}

void ANSPlayerController::UpdateHUDHealthAndShield()
{
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
	UNSUIManagerSubsystem* UIManager =
		GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>();
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

void ANSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (!IsLocalController())
	{
		return;
	}

	UNSUIManagerSubsystem* UIManager = GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>();
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
			FInputModeGameOnly InputModeData;
			SetInputMode(InputModeData);
			bShowMouseCursor = false;
		}
	
	//HUD 생성 이후 Attribute 값 연결
	BindAttributeToHUD();
	UpdateHUDHealthAndShield();
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

	UNSUIManagerSubsystem* UIManager = GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>();
	if (!UIManager) return;

	// 1. 심리스 트레블 전 스테이지의 HUD 잔재를 안전하게 청소
	UIManager->ClearHUD();

	FString MapName = GetWorld()->GetName();

	// 2. 현재 로드된 맵이 타이틀이나 거점(HideOut)이 아닌 '진짜 인게임'일 때만 생성
	if (!MapName.Contains(TEXT("Title")) && !MapName.Contains(TEXT("HideOut")))
	{
		UIManager->HideTitle();
        
		// 청소된 상태이므로 nullptr 검사를 통과하고 새 HUD 위젯이 깔끔하게 생성됩니다.
		UIManager->CreateHUD(this);
		UIManager->ShowHUD();
        
		// 마우스 커서 및 입력 모드 제어
		FInputModeGameOnly InputModeData;
		SetInputMode(InputModeData);
		bShowMouseCursor = false;

		if (ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(NewPawn))
		{
			if (UNSInputBinderComponent* InputBinder = PlayerCharacter->GetInputBinderComponent())
			{
				InputBinder->SetActiveInputModeTags(GetGameplayInputModeTags());
			}
		}

		// 3. 캐릭터가 새로 배치되었으니 체력/실드 등 GAS 어트리뷰트 값을 HUD에 연동
		UpdateHUDHealthAndShield();
	}
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
	Client_NotifyRunStarted();
}

void ANSPlayerController::Client_NotifyRunStarted_Implementation()
{
	if (UNSDataSubsystem* Data = UNSDataSubsystem::Get(this))
	{
		Data->EnterRun();
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

void ANSPlayerController::Client_ShowRunOverUI_Implementation(bool bIsClear)
{
	UNSUIManagerSubsystem* UIManager = GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>();
	if (!UIManager)
	{
		return;
	}

	UIManager->CreateRunEnd(this);
	UIManager->ShowRunEnd();
	
	FInputModeUIOnly InputModeData;
	SetInputMode(InputModeData);
	bShowMouseCursor = true;
	
	UE_LOG(LogTemp, Warning, TEXT("게임 종료 UI: %s"),
		bIsClear ? TEXT("클리어") : TEXT("전멸"));
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

	// Tab키로 증강 패널 토글
	InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ANSPlayerController::ToggleAugmentationPanel);

	// 디버그 : O키로 풀 적재, TODO : 테스트 끝나면 삭제
	InputComponent->BindKey(EKeys::O, IE_Pressed, this, &ANSPlayerController::Debug_EnqueueAugmentOffer);
}

void ANSPlayerController::ToggleAugmentationPanel()
{
	UNSUIManagerSubsystem* UIManager = GetGameInstance() ? GetGameInstance()->GetSubsystem<UNSUIManagerSubsystem>() : nullptr;
	if (!UIManager)
	{
		return;
	}

	if (UIManager->IsAugmentationPanelOpen())
	{
		// 열려있으면 닫음 (토글)
		UIManager->CloseAugmentationPanel();
	}
	else
	{
		// 패널 UI 표시
		UIManager->OpenAugmentationPanel();
		// 서버에 대기열 front 오퍼 표시 요청 (대기 있으면 카드가 옴)
		if (AugmentSelectionComponent)
		{
			AugmentSelectionComponent->Server_OpenPanel();
		}
	}
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

	// 런 데이터가 없으면 로드 후 자동 적재 (테스트용)
	if (!Data->IsRunReady())
	{
		Data->OnRunGameDataReady.AddUniqueDynamic(this, &ANSPlayerController::OnTestRunDataReady);
		Data->EnterRun();
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

void ANSPlayerController::OnTestRunDataReady()
{
	UNSDataSubsystem* Data = UNSDataSubsystem::Get(this);
	if (!Data)
	{
		return;
	}
	Data->OnRunGameDataReady.RemoveDynamic(this, &ANSPlayerController::OnTestRunDataReady);

	if (!AugmentSelectionComponent)
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
