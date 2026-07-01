// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerCharacterBase.h"

#include "Camera/CameraComponent.h"
#include "CharacterTrajectoryComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/AI/Base/NSBaseDroneAI.h"
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"
#include "NeoSanctum/AI/Companion/Pawn/NSCompanionDroneAI.h"
#include "NeoSanctum/Character/Component/NSCompanionProgressionComponent.h"
#include "NeoSanctum/Character/Component/NSInputBinderComponent.h"
#include "NeoSanctum/Character/Component/NSSpectatorViewComponent.h"
#include "NeoSanctum/Character/Component/NSPartVisualComponent.h"
#include "NeoSanctum/Character/Component/NSGateAccessComponent.h"
#include "NeoSanctum/Collision/NSCollisionProfiles.h"
#include "NeoSanctum/Interaction/Component/NSInteractionComponent.h"
#include "NeoSanctum/Combat/Component/NSMeleeAttackReservationComponent.h"
#include "NeoSanctum/Progression/Part/NSPartEquipComponent.h"
#include "NeoSanctum/Combat/Weapon/NSWeaponBase.h"
#include "NeoSanctum/Combat/HitReaction/NSHitReactionComponent.h"
#include "NeoSanctum/Combat/HitReaction/NSPlayerAttackFeedbackComponent.h"
#include "NeoSanctum/Combat/HitReaction/NSPlayerHitTakenFeedbackComponent.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Progression/Augment/NSAugmentInventoryComponent.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "NeoSanctum/Data/AI/NSCompanionDefinition.h"
#include "NeoSanctum/Data/Character/NSCharacterBaseStatTypes.h"
#include "NeoSanctum/Data/Character/NSCharacterData.h"
#include "NeoSanctum/Data/CommonUpgrade/NSCommonUpgradeTypes.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"
#include "NeoSanctum/GAS/Stats/NSCombatStatAttributeMapping.h"
#include "NeoSanctum/System/Component/NSDamageFlashComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"
#include "Net/UnrealNetwork.h"

ANSPlayerCharacterBase::ANSPlayerCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->SetCollisionProfileName(NSCollisionProfiles::PlayerCharacter);
	
	// TPS 카메라를 캐릭터 뒤쪽에 배치하는 SpringArm 기본 설정
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->SetRelativeLocation(FVector(0.0f, 40.0f, 70.0f));
	SpringArmComp->TargetArmLength = 225.0f;
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SocketOffset = FVector(0.0f, 50.0f, 0.0f);
	
	// 카메라. 자체 회전은 사용하지 않음
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);
	CameraComp->bUsePawnControlRotation = false;

	// 이동 방향 자동 회전 비활성화, 카메라 방향 회전으로 통일.
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementComponent->bOrientRotationToMovement = false;
	MovementComponent->bUseControllerDesiredRotation = false;
	MovementComponent->RotationRate = FRotator(0.f, 540.f, 0.f);
	
	CharacterTrajectoryComp = CreateDefaultSubobject<UCharacterTrajectoryComponent> (TEXT("CharacterTrajectoryComp"));
	
	InputBinderComp = CreateDefaultSubobject<UNSInputBinderComponent>(TEXT("InputBinderComp"));
	SpectatorViewComp = CreateDefaultSubobject<UNSSpectatorViewComponent>(TEXT("SpectatorViewComp"));
	PartVisualComp = CreateDefaultSubobject<UNSPartVisualComponent>(TEXT("PartVisualComp"));
	
	MeleeAttackReservationComp = CreateDefaultSubobject<UNSMeleeAttackReservationComponent>(
		TEXT("MeleeAttackReservationComp"));
	MeleeAttackReservationComp->SetMaxConcurrentAttackers(5);
	
	InteractionComp = CreateDefaultSubobject<UNSInteractionComponent>(TEXT("InteractionComp"));

	GateAccessComp = CreateDefaultSubobject<UNSGateAccessComponent>(TEXT("GateAccessComp"));
	PlayerAttackFeedbackComp = CreateDefaultSubobject<UNSPlayerAttackFeedbackComponent>(TEXT("PlayerAttackFeedbackComp"));
	PlayerHitTakenFeedbackComp = CreateDefaultSubobject<UNSPlayerHitTakenFeedbackComponent>(
		TEXT("PlayerHitTakenFeedbackComp"));
	HitReactionComponent = CreateDefaultSubobject<UNSHitReactionComponent>(TEXT("HitReactionComponent"));
	HitReactionComponent->SetTargetType(ENSHitFeedbackTargetType::Player);
	DamageFlashComponent = CreateDefaultSubobject<UNSDamageFlashComponent>(TEXT("DamageFlashComponent"));
	DamageFlashComponent->SetTriggerPolicy(ENSDamageFlashTriggerPolicy::Shield);

	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
}

void ANSPlayerCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (NSAbilitySystemComponent && IsLocallyControlled())
	{
		NSAbilitySystemComponent->ProcessAbilityInput(DeltaSeconds, false);
	}
	
	UpdateCameraFacingRotation(DeltaSeconds);
}

void ANSPlayerCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeAbilitySystem();
	
	if (SpectatorViewComp)
	{
		// 관전자에게 보낼 카메라 정보의 타겟이 되는 카메라 설정
		SpectatorViewComp->SetSourceCamera(CameraComp);
	}
}

void ANSPlayerCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 입력 바인딩 세부 구현은 전용 컴포넌트로 책임 이전
	if (InputBinderComp)
	{
		InputBinderComp->InitializePlayerInput(PlayerInputComponent);
	}
}

void ANSPlayerCharacterBase::PossessedBy(AController* EventController)
{
	Super::PossessedBy(EventController);

	InitializeAbilitySystem();

	if (InteractionComp)
	{
		InteractionComp->EnableLocalInteraction();
	}

	// 서버: PlayerState/진행도 기준으로 게이트 통과 처리 초기화 (PlayerState 미준비 시 OnRep에서 재시도)
	if (GateAccessComp)
	{
		GateAccessComp->InitializeGateAccess();
	}

	if (HasAuthority())
	{
		BindPartVisual();
		// PossessedBy 시점에 캐릭터 데이터 적용
		ApplyCurrentCharacterData();
		// 이관된 런타임 파츠가 있으면(스테이지 이동) 재적용, 없으면(허브 최초 진입/리스폰) 저장 파츠 장착
		if (ANSPlayerState* PS = GetPlayerState<ANSPlayerState>())
		{
			UNSPartEquipComponent* PartComp = PS->GetPartEquipComponent();
			if (PartComp && PartComp->HasAnyEquipped())
			{
				PartComp->ReapplyAll();
			}
			else
			{
				ApplyEquippedPart();
			}
		}
		// Seamless Travel로 새 ASC가 생성되었으므로 보유 증강을 재적용
		if (ANSPlayerState* PS = GetPlayerState<ANSPlayerState>())
		{
			if (UNSAugmentInventoryComponent* AugmentInventory = PS->GetAugmentInventory())
			{
				AugmentInventory->ReapplyAll();
			}
		}
	}
	
	// Companion 초기화 및 스폰 시도
	TryInitializeCompanion();
}

void ANSPlayerCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitializeAbilitySystem();
	BindPartVisual();

	// 클라: PlayerState 수신 후 게이트 통과/외형 초기화
	if (GateAccessComp)
	{
		GateAccessComp->InitializeGateAccess();
	}
}

void ANSPlayerCharacterBase::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (InteractionComp)
	{
		InteractionComp->EnableLocalInteraction();
	}
}

void ANSPlayerCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANSPlayerCharacterBase, CurrentCharacterData);
	DOREPLIFETIME(ANSPlayerCharacterBase, CurrentWeapon);
	DOREPLIFETIME(ANSPlayerCharacterBase, bDeathPresentationStarted);
}

UAbilitySystemComponent* ANSPlayerCharacterBase::GetAbilitySystemComponent() const
{
	if (NSAbilitySystemComponent)
	{
		return NSAbilitySystemComponent;
	}

	if (const ANSPlayerState* PS = GetPlayerState<ANSPlayerState>())
	{
		return PS->GetAbilitySystemComponent();
	}

	return nullptr;
}

void ANSPlayerCharacterBase::ChangeCharacterData(UNSCharacterData* InCharacterData)
{
	if (!InCharacterData)
	{
		return;
	}

	if (!HasAuthority())
	{
		Server_ChangeCharacterData(InCharacterData);
		return;
	}

	if (ANSPlayerState* NSPlayerState = GetPlayerState<ANSPlayerState>())
	{
		NSPlayerState->SetCurrentCharacterData(InCharacterData);
	}

	InitializeFromCharacterData(InCharacterData);
}

void ANSPlayerCharacterBase::Server_ChangeCharacterData_Implementation(UNSCharacterData* InCharacterData)
{
	ChangeCharacterData(InCharacterData);
}

void ANSPlayerCharacterBase::InitializeFromCharacterData(const UNSCharacterData* InCharacterData)
{
	if (!InCharacterData)
	{
		return;
	}
	
	// 데이터를 적용하기에 앞서 서버권한에서 데이터를 한 번 클리어
	if (HasAuthority())
	{
		ClearCharacterDataRuntimeState();
	}
	
	CurrentCharacterData = InCharacterData;

	// CommonData 로딩 단계에서 CharacterData의 Soft Reference까지 선로딩되므로,
	// 여기서 동기 로딩을 강제하지 않음.
	ApplyCharacterVisual();
	
	// 서버에서 처리할 것들
	if (HasAuthority())
	{
		ApplyInitialAttributeEffect();
		ApplyCommonUpgradeAttributeEffect();
		ApplyDefaultGameplayEffects();
		GiveCharacterDataAbilities();
		SpawnDefaultWeapon();
		
		if (NSAbilitySystemComponent)
		{
			NSAbilitySystemComponent->ForceReplication();
		}

		if (ANSPlayerState* PS = GetPlayerState<ANSPlayerState>())
		{
			PS->ForceNetUpdate();
		}
		
		// 다음 NetUpdateFrequency까지 기다리지말고 바로 업데이트 하기 위한 함수
		ForceNetUpdate();
	}
}

void ANSPlayerCharacterBase::ApplyCurrentCharacterData()
{
	ANSPlayerState* NSPlayerState = GetPlayerState<ANSPlayerState>();

	if (!NSPlayerState)
	{
		return;
	}
	
	// 현재 캐릭터 데이터를 PlayerState로부터 받아와서 적용
	if (UNSCharacterData* PlayerStateCurrentCharacterData = NSPlayerState->GetCurrentCharacterData())
	{
		InitializeFromCharacterData(PlayerStateCurrentCharacterData);
	}
}

bool ANSPlayerCharacterBase::TryGetAimTraceStartLocation(FVector& OutLocation) const
{
	if (IsValid(CameraComp))
	{
		OutLocation = CameraComp->GetComponentLocation();
		return true;
	}
	
	OutLocation = GetPawnViewLocation();
	return true;
}

void ANSPlayerCharacterBase::ApplyEquippedPart()
{
	if (!HasAuthority())
	{
		return;
	}

	ANSPlayerState* PS = GetPlayerState<ANSPlayerState>();
	if (!PS)
	{
		return;
	}
	
	UNSPlayerProgressComponent* ProgressComp = PS->GetProgressComponent();
	UNSPartEquipComponent* EquipComp = PS->GetPartEquipComponent();
	if (!ProgressComp || !EquipComp)
	{
		return;
	}

	// 기존 진입 파츠 제거(드랍 없음) — 교체/해제 모두 깨끗한 시작점
	EquipComp->ClearAll();

	const FNSPartSaveData& Equipped = ProgressComp->GetEquippedPart();
	if (Equipped.Definition.IsNull())
	{
		return; 
	}

	Equipped.Definition.LoadSynchronous();
	
	FNSPartData Part;
	Part.DefinitionPtr = Equipped.Definition;
	Part.CurrentRarity = Equipped.Rarity;
	Part.CurrentValue  = Equipped.Value;
	EquipComp->EquipPart(Part);   
}

void ANSPlayerCharacterBase::InitializeAbilitySystem()
{
	ANSPlayerState* PS = GetPlayerState<ANSPlayerState>();
	
	if (!PS)
	{
		return;
	}
	
	NSAbilitySystemComponent = Cast<UNSAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	PlayerAttributeSet = PS->GetPlayerAttributeSet();
	
	if (NSAbilitySystemComponent && PlayerAttributeSet)
	{
		NSAbilitySystemComponent->InitAbilityActorInfo(PS, this);
		
		BindAttributeDelegates();

		if (bDeathPresentationStarted)
		{
			ApplyDeathState();
		}
	}
}

void ANSPlayerCharacterBase::BindPartVisual()
{
	if (!PartVisualComp)
	{
		return;
	}

	ANSPlayerState* PS = GetPlayerState<ANSPlayerState>();
	if (!PS)
	{
		return;
	}

	UNSPartEquipComponent* EquipComp = PS->GetPartEquipComponent();
	if (!EquipComp)
	{
		return;
	}

	PartVisualComp->BindToEquipComponent(EquipComp, GetMesh());
}

void ANSPlayerCharacterBase::UpdateCameraFacingRotation(float DeltaSeconds)
{
	if (!bUseCameraFacingRotation || !Controller)
	{
		bIsCameraFacingRotationActive = false;
		return;
	}

	const FRotator ActorRotation = GetActorRotation();
	const FRotator ControlRotation = Controller->GetControlRotation();
	const float YawDelta = FRotator::NormalizeAxis(ControlRotation.Yaw - ActorRotation.Yaw);
	const float AbsYawDelta = FMath::Abs(YawDelta);
	const FVector HorizontalVelocity(GetVelocity().X, GetVelocity().Y, 0.f);
	
	// 이동 중에는 카메라 방향을 우선해서, 따로 카메라 회전에 따라 캐릭터 회전을 보간하지 않음.
	const bool bShouldFaceCamera = HorizontalVelocity.SizeSquared() > FMath::Square(CameraFacingMoveSpeedThreshold);

	// 경계값 근처 회전 상태 떨림 방지용 시작/종료 각도 분리하는 삼항연산
	bIsCameraFacingRotationActive = bShouldFaceCamera
		|| (bIsCameraFacingRotationActive
			? AbsYawDelta > CameraFacingTurnStopAngle
			: AbsYawDelta >= CameraFacingTurnStartAngle);

	if (!bIsCameraFacingRotationActive)
	{
		return;
	}

	const FRotator TargetRotation(0.f, ControlRotation.Yaw, 0.f);
	// 카메라 Yaw 방향으로 일정 속도 보간해서 즉시 회전하는 것을 방지하는 러프
	const FRotator NewRotation = FMath::RInterpConstantTo(
		ActorRotation,
		TargetRotation,
		DeltaSeconds,
		CameraFacingRotationSpeed);

	SetActorRotation(NewRotation);
}

#pragma region CompanionSpawn

void ANSPlayerCharacterBase::TryInitializeCompanion()
{
	if (CompanionAI.IsValid() || !HasAuthority()) return;
	
	ANSPlayerState* PS = GetPlayerState<ANSPlayerState>();
	if (!PS) return;
	
	UNSCompanionDefinition* CompanionDefinition = PS->GetCurrentCompanionDefinition();
	if (!CompanionDefinition)
	{
		return;
	}
	
	SpawnCompanion(CompanionDefinition);
}

void ANSPlayerCharacterBase::SpawnCompanion(const UNSCompanionDefinition* Definition)
{
	// 조건 체크
	if(!GetWorld() || !DroneAIClass || !Definition) return;
	
	// 캐릭터부터 스폰 거리 유지
	const FVector LocalOffset(-100.f, -100.f, 0.f);
	const FVector SpawnLocation = GetActorTransform().TransformPosition(LocalOffset); 
	FTransform SpawnTransform(GetActorRotation(), SpawnLocation);
	
	// SpawnActorDeferred 
	ANSCompanionDroneAI* SpawnedCompanionAI = GetWorld()->SpawnActorDeferred<ANSCompanionDroneAI>(
		DroneAIClass,
		SpawnTransform,
		this,
		this,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
		);
	
	if (!SpawnedCompanionAI) return;
	
	// 드론 스폰 성공했다면 잠시 틈만들어서 초기화
	SpawnedCompanionAI->SetOwnerPlayer(this);
	SpawnedCompanionAI->SetPendingDefinition(Definition);
	
	// 최종스폰
	SpawnedCompanionAI->FinishSpawning(SpawnTransform);
	
	// WeakPtr 갱신
	CompanionAI = SpawnedCompanionAI;
	
	// Component 내부 소유 드론 지정
	ANSPlayerState* PS = GetPlayerState<ANSPlayerState>();
	if (!PS) return;
	PS->GetCompanionProgressionComponent()->SetOwnedCompanion(CompanionAI.Get());
	
	if (UNSPlayerProgressComponent* Progress = PS->GetProgressComponent())
	{
		PS->GetCompanionProgressionComponent()->ApplyNodeLevels(Progress->GetCompanionNodeLevels());
	}
}

void ANSPlayerCharacterBase::HandleCompanionDataReady()
{
	if (!HasAuthority()) return;
	
	ANSPlayerState* PS = GetPlayerState<ANSPlayerState>();
	if (!PS) return;
	
	UNSCompanionDefinition* CurrentDefinition = PS->GetCurrentCompanionDefinition();
	if (!CurrentDefinition) return;
	
	if (CompanionAI.IsValid())
	{
		CompanionAI->ApplyDroneDefinition(CurrentDefinition);
	}
	else
	{
		TryInitializeCompanion();
	}
}

#pragma endregion

void ANSPlayerCharacterBase::OnRep_CurrentCharacterData()
{
	ApplyCharacterVisual();
}

void ANSPlayerCharacterBase::OnRep_CurrentWeapon()
{
	if (!IsValid(CurrentWeapon))
	{
		return;
	}
	
	CurrentWeapon->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		CurrentWeapon->GetAttachSocketName()
	);
}

void ANSPlayerCharacterBase::BindAttributeDelegates()
{
	if (!NSAbilitySystemComponent || !PlayerAttributeSet)
	{
		return;
	}
	
	// 사망처리 바인딩
	PlayerAttributeSet->OnOutOfHealth.RemoveAll(this);
	PlayerAttributeSet->OnOutOfHealth.AddUObject(this, &ANSPlayerCharacterBase::HandleOutOfHealth);
	
	// 중복 바인딩을 피하기 위한 바인딩 제거
	NSAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		PlayerAttributeSet->GetMoveSpeedAttribute()).RemoveAll(this);
	
	// 바인딩
	NSAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		PlayerAttributeSet->GetMoveSpeedAttribute()).AddUObject(this, &ThisClass::OnMoveSpeedChanged);
	
	// Attribute 초기화 Effect가 들어오기 전까지 주석처리 : ApplyMoveSpeedToCharacter(PlayerAttributeSet->GetMoveSpeed());
}

void ANSPlayerCharacterBase::ApplyReactiveGameplayEffect(const FGameplayTag& TriggerTag)
{
	if (!HasAuthority() || !CurrentCharacterData || !NSAbilitySystemComponent || !TriggerTag.IsValid())
	{
		return;
	}

	for (const FNSReactiveGameplayEffectData& ReactiveEffect : CurrentCharacterData->ReactiveGameplayEffects)
	{
		if (ReactiveEffect.TriggerTag != TriggerTag)
		{
			continue;
		}

		TSubclassOf<UGameplayEffect> EffectClass = ReactiveEffect.EffectClass.Get();
		if (!EffectClass)
		{
			continue;
		}

		FGameplayEffectContextHandle EffectContext = NSAbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle =
			NSAbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);

		if (SpecHandle.IsValid())
		{
			NSAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void ANSPlayerCharacterBase::ApplyCharacterVisual()
{
	if (!CurrentCharacterData)
	{
		return;
	}
	
	USkeletalMesh* LoadedMesh = CurrentCharacterData->SkeletalMesh.Get();
	if (LoadedMesh)
	{
		GetMesh()->SetSkeletalMesh(LoadedMesh);
	}
	
	UClass* LoadedAnimClass = CurrentCharacterData->AnimClass.Get();
	if (LoadedAnimClass)
	{
		GetMesh()->SetAnimInstanceClass(LoadedAnimClass);
	}

	UClass* LoadedUpperBodyAnimLayerClass = CurrentCharacterData->UpperBodyAnimLayerClass.Get();
	if (LoadedUpperBodyAnimLayerClass)
	{
		GetMesh()->LinkAnimClassLayers(LoadedUpperBodyAnimLayerClass);
	}
}

void ANSPlayerCharacterBase::ApplyInitialAttributeEffect()
{
	if (!HasAuthority() || !CurrentCharacterData || !NSAbilitySystemComponent)
	{
		return;
	}

	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem)
	{
		return;
	}

	const FNSCharacterBaseStatRow* StatRow = DataSubsystem->FindCharacterBaseStatRow(CurrentCharacterData->CharacterTag);

	if (!StatRow)
	{
		NS_ACTOR_LOG(this, LogNSGAS, Warning,
			"캐릭터 기본 스탯 Row를 찾지 못했습니다. CharacterTag={Tag}",
			("Tag", CurrentCharacterData->CharacterTag.ToString())
		);
		return;
	}

	TSubclassOf<UGameplayEffect> InitEffectClass = DataSubsystem->GetCharacterBaseStatInitEffectClass();
	if (!InitEffectClass)
	{
		return;
	}
	
	FGameplayEffectContextHandle EffectContext = NSAbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	
	FGameplayEffectSpecHandle SpecHandle = 
		NSAbilitySystemComponent->MakeOutgoingSpec(InitEffectClass, 1.0f, EffectContext);
	
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_MaxHealth, StatRow->MaxHealth);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_Health, StatRow->MaxHealth);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_BaseDamage, StatRow->BaseDamage);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_Defense, StatRow->Defense);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_MoveSpeed, StatRow->MoveSpeed);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_CritChance, StatRow->CritChance);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_CritDamage, StatRow->CritDamage);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_MaxShield, StatRow->MaxShield);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_Shield, StatRow->MaxShield);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_ShieldRechargeRate, StatRow->ShieldRechargeRate);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_ShieldRechargeCooldown, StatRow->ShieldRechargeCooldown);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_MaxDashCount, StatRow->MaxDashCount);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_DashCount, StatRow->MaxDashCount);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_DashRegenRate, StatRow->DashRegenRate);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_MaxAmmo, StatRow->MaxAmmo);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_Ammo, StatRow->MaxAmmo);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_MaxSkill1Count, StatRow->MaxSkill1Count);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_Skill1Count, StatRow->MaxSkill1Count);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_MaxSkill2Count, StatRow->MaxSkill2Count);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_Skill2Count, StatRow->MaxSkill2Count);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_MaxSkill3Count, StatRow->MaxSkill3Count);
	SpecHandle.Data->SetSetByCallerMagnitude(NSGameplayTags::Effect_SetByCaller_Init_Skill3Count, StatRow->MaxSkill3Count);

	const FActiveGameplayEffectHandle EffectHandle =
		NSAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	if (EffectHandle.IsValid())
	{
		CharacterDataEffectHandles.Add(EffectHandle);
	}
}

void ANSPlayerCharacterBase::ApplyCommonUpgradeAttributeEffect()
{
	if (!HasAuthority() || !NSAbilitySystemComponent)
	{
		return;
	}

	ANSPlayerState* NSPlayerState = GetPlayerState<ANSPlayerState>();
	UNSPlayerProgressComponent* ProgressComponent = NSPlayerState ? NSPlayerState->GetProgressComponent() : nullptr;

	if (!ProgressComponent)
	{
		return;
	}

	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem)
	{
		return;
	}

	TSubclassOf<UGameplayEffect> UpgradeEffectClass = DataSubsystem->GetCommonUpgradeInitEffectClass();
	if (!UpgradeEffectClass)
	{
		return;
	}

	// NodeId별 레벨을 Attribute 태그 기준으로 합산 (Add는 그대로, Multiply는 %로 누적)
	TMap<FGameplayTag, float> AddSums;
	TMap<FGameplayTag, float> MultiplyPercentSums;

	for (const TPair<FName, int32>& Pair : ProgressComponent->GetCommonSkillLevels())
	{
		const FNSCommonUpgradeNodeRow* Row = DataSubsystem->GetCommonUpgradeNodeRow(Pair.Key);
		if (!Row || Pair.Value <= 0)
		{
			continue;
		}

		const float Contribution = Row->ValuePerLevel * static_cast<float>(Pair.Value);

		if (Row->Operation == ENSCombatStatModifierOperation::Add)
		{
			AddSums.FindOrAdd(Row->AttributeTag) += Contribution;
		}
		else
		{
			MultiplyPercentSums.FindOrAdd(Row->AttributeTag) += Contribution;
		}
	}

	FGameplayEffectContextHandle EffectContext = NSAbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle =
		NSAbilitySystemComponent->MakeOutgoingSpec(UpgradeEffectClass, 1.0f, EffectContext);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	NSCombatStatAttribute::InitializeNeutralSetByCallers(SpecHandle);

	for (const TPair<FGameplayTag, float>& Pair : AddSums)
	{
		const FNSCombatStatAttributeMapping* Mapping = NSCombatStatAttribute::FindMapping(Pair.Key);
		if (Mapping && Mapping->AddSetByCallerTag.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(Mapping->AddSetByCallerTag, Pair.Value);
		}
	}

	for (const TPair<FGameplayTag, float>& Pair : MultiplyPercentSums)
	{
		const FNSCombatStatAttributeMapping* Mapping = NSCombatStatAttribute::FindMapping(Pair.Key);
		if (Mapping && Mapping->MultiplySetByCallerTag.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(Mapping->MultiplySetByCallerTag, 1.0f + Pair.Value * 0.01f);
		}
	}

	// MaxHealth/MaxShield가 늘어난 만큼 Current도 같이 올리기 위한 사전 스냅샷
	const float OldMaxHealth =
		NSAbilitySystemComponent->GetNumericAttribute(UNSBaseAttributeSet::GetMaxHealthAttribute());
	const float OldMaxShield =
		NSAbilitySystemComponent->GetNumericAttribute(UNSPlayerAttributeSet::GetMaxShieldAttribute());

	const FActiveGameplayEffectHandle EffectHandle =
		NSAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	if (EffectHandle.IsValid())
	{
		CharacterDataEffectHandles.Add(EffectHandle);
	}

	const float NewMaxHealth =
		NSAbilitySystemComponent->GetNumericAttribute(UNSBaseAttributeSet::GetMaxHealthAttribute());
	const float NewMaxShield =
		NSAbilitySystemComponent->GetNumericAttribute(UNSPlayerAttributeSet::GetMaxShieldAttribute());

	if (NewMaxHealth > OldMaxHealth)
	{
		const float CurrentHealth =
			NSAbilitySystemComponent->GetNumericAttribute(UNSBaseAttributeSet::GetHealthAttribute());
		NSAbilitySystemComponent->SetNumericAttributeBase(
			UNSBaseAttributeSet::GetHealthAttribute(), CurrentHealth + (NewMaxHealth - OldMaxHealth));
	}

	if (NewMaxShield > OldMaxShield)
	{
		const float CurrentShield =
			NSAbilitySystemComponent->GetNumericAttribute(UNSPlayerAttributeSet::GetShieldAttribute());
		NSAbilitySystemComponent->SetNumericAttributeBase(
			UNSPlayerAttributeSet::GetShieldAttribute(), CurrentShield + (NewMaxShield - OldMaxShield));
	}
}

void ANSPlayerCharacterBase::ApplyDefaultGameplayEffects()
{
	if (!HasAuthority() || !CurrentCharacterData || !NSAbilitySystemComponent)
	{
		return;
	}
	
	FGameplayEffectContextHandle EffectContext = NSAbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	
	for (const TSoftClassPtr<UGameplayEffect>& DefaultEffect : CurrentCharacterData->DefaultGameplayEffects)
	{
		TSubclassOf<UGameplayEffect> Effect = DefaultEffect.Get();
		if (!Effect)
		{
			continue;
		}
		
		FGameplayEffectSpecHandle SpecHandle = 
			NSAbilitySystemComponent->MakeOutgoingSpec(Effect, 1.0f, EffectContext);
		
		if (SpecHandle.IsValid())
		{
			const FActiveGameplayEffectHandle EffectHandle =
				NSAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			if (EffectHandle.IsValid())
			{
				CharacterDataEffectHandles.Add(EffectHandle);
			}
		}
	}
}

void ANSPlayerCharacterBase::GiveCharacterDataAbilities()
{
	if (!HasAuthority() || !CurrentCharacterData || !NSAbilitySystemComponent)
	{
		return;
	}
	
	for (const FNSCharacterAbilityData& AbilityData : CurrentCharacterData->DefaultAbilities)
	{
		TSubclassOf<UGameplayAbility> LoadedAbilityClass = AbilityData.AbilityClass.Get();
		if (!LoadedAbilityClass)
		{
			continue;
		}
		
		const int32 AbilityLevel = FMath::Max(1, AbilityData.AbilityLevel);
		
		FGameplayAbilitySpec AbilitySpec(LoadedAbilityClass, AbilityLevel);
		if (AbilityData.InputTag.IsValid())
		{
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilityData.InputTag);			
		}
		
		const FGameplayAbilitySpecHandle AbilityHandle = NSAbilitySystemComponent->GiveAbility(AbilitySpec);
		if (AbilityHandle.IsValid())
		{
			CharacterDataAbilityHandles.Add(AbilityHandle);
		}
	}
}

void ANSPlayerCharacterBase::SpawnDefaultWeapon()
{
	if (!HasAuthority() || !CurrentCharacterData)
	{
		return;
	}
	
	TSubclassOf<ANSWeaponBase> LoadedWeaponClass = CurrentCharacterData->DefaultWeaponClass.Get();
	if (!LoadedWeaponClass)
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	ANSWeaponBase* SpawnedWeapon = World->SpawnActor<ANSWeaponBase>(
		LoadedWeaponClass,
		FTransform::Identity,
		SpawnParams
	);
	
	if (!IsValid(SpawnedWeapon))
	{
		return;
	}
	
	SpawnedWeapon->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		SpawnedWeapon->GetAttachSocketName()
	);
	
	CurrentWeapon = SpawnedWeapon;
}

void ANSPlayerCharacterBase::ClearCharacterDataRuntimeState()
{
	if (!HasAuthority())
	{
		return;
	}
	
	// Ability와 Effect Clear
	if (NSAbilitySystemComponent)
	{
		NSAbilitySystemComponent->ClearAbilityInput();

		for (const FGameplayAbilitySpecHandle& AbilityHandle : CharacterDataAbilityHandles)
		{
			if (!AbilityHandle.IsValid())
			{
				continue;
			}

			NSAbilitySystemComponent->CancelAbilityHandle(AbilityHandle);
			NSAbilitySystemComponent->ClearAbility(AbilityHandle);
		}
		CharacterDataAbilityHandles.Reset();

		for (const FActiveGameplayEffectHandle& EffectHandle : CharacterDataEffectHandles)
		{
			if (EffectHandle.IsValid())
			{
				NSAbilitySystemComponent->RemoveActiveGameplayEffect(EffectHandle);
			}
		}
		CharacterDataEffectHandles.Reset();
	}
	
	// 무기를 Destroy
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->Destroy();
	}
	CurrentWeapon = nullptr;
}

void ANSPlayerCharacterBase::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	ApplyMoveSpeedToCharacter(Data.NewValue);
}

void ANSPlayerCharacterBase::ApplyMoveSpeedToCharacter(float MoveSpeed)
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = MoveSpeed;
	}
}

void ANSPlayerCharacterBase::HandleOutOfHealth()
{
	Die();
}

void ANSPlayerCharacterBase::Die()
{
	if (bDeathPresentationStarted)
	{
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("%s died"), *GetName());

	// 서버에서 사망처리
	if (HasAuthority())
	{
		if (ANSPlayerState* NSPlayerState = GetPlayerState<ANSPlayerState>())
		{
			// PlayerState에서 사망 상태를 관리
			NSPlayerState->SetIsDead(true);
		}

		bDeathPresentationStarted = true;
		
		// GameMode에 플레이어 사망 알림
		AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
		if (GameMode && GameMode->Implements<UNSRunGameModeInterface>())
		{
			INSRunGameModeInterface::Execute_NotifyPlayerDied(GameMode, GetController());
		}
	}
	
	ApplyDeathState();
	StartDeathRagdoll();

	if (ANSPlayerController* NSPlayerController = Cast<ANSPlayerController>(GetController()))
	{
		NSPlayerController->RequestEnterDeathSpectatorMode();
	}
}

void ANSPlayerCharacterBase::ApplyDeathState()
{
	if (!NSAbilitySystemComponent)
	{
		return;
	}

	// 실행중인 Ability 전부 취소
	NSAbilitySystemComponent->CancelAbilities();
	// 임시 : 강제로 Dead 상태 태그 부여 -> 이후 GA_Death로 로직을 빼고 GE에서 변경하도록 할 예정 
	NSAbilitySystemComponent->AddLooseGameplayTag(NSGameplayTags::State_Dead);
}

void ANSPlayerCharacterBase::StartDeathRagdoll()
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		// MovementComponent를 즉시 정지 : 이미 입력된 움직임을 즉시 정지해야 캐릭터가 튕겨져나가지 않음
		MovementComponent->StopMovementImmediately();
		// 비활성화
		MovementComponent->DisableMovement();
	}

	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		// 캡슐 충돌 비활성화
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		// Collision 프로필을 Ragdoll 프로필로 변경
		MeshComponent->SetCollisionProfileName(TEXT("Ragdoll"));
		MeshComponent->SetAllBodiesSimulatePhysics(true);
		MeshComponent->SetSimulatePhysics(true);
		MeshComponent->WakeAllRigidBodies();
		// 애니메이션 포즈와 래그돌 상태에 따른 물리 적용이 자연스럽게 섞이도록 하는 설정
		MeshComponent->bBlendPhysics = true;
	}
}

void ANSPlayerCharacterBase::OnRep_DeathPresentationStarted()
{
	if (!bDeathPresentationStarted)
	{
		return;
	}

	ApplyDeathState();
	StartDeathRagdoll();

	if (ANSPlayerController* NSPlayerController = Cast<ANSPlayerController>(GetController()))
	{
		NSPlayerController->RequestEnterDeathSpectatorMode();
	}
}
