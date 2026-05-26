// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerCharacterBase.h"

#include "Camera/CameraComponent.h"
#include "CharacterTrajectoryComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "NeoSanctum/Character/Component/NSInputBinderComponent.h"
#include "NeoSanctum/Combat/Weapon/NSWeaponBase.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Data/Character/NSCharacterData.h"
#include "NeoSanctum/GAS/NSAbilitySystemComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSPlayerAttributeSet.h"

ANSPlayerCharacterBase::ANSPlayerCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	// TPS 카메라를 캐릭터 뒤쪽에 배치하는 SpringArm 기본 설정
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->SetRelativeLocation(FVector(0.0f, 40.0f, 60.0f));
	SpringArmComp->TargetArmLength = 165.0f;
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
}

void ANSPlayerCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateCameraFacingRotation(DeltaSeconds);
}

void ANSPlayerCharacterBase::BeginPlay()
{
	Super::BeginPlay();
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
	BindAttributeDelegates();
	ApplyDefaultGameplayEffects();
	GiveDefaultAbilities();
}

void ANSPlayerCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	InitializeAbilitySystem();
	BindAttributeDelegates();
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

void ANSPlayerCharacterBase::InitializeFromCharacterData(const UNSCharacterData* InCharacterData)
{
	if (!InCharacterData)
	{
		return;
	}
	
	CharacterData = InCharacterData;
	
	ApplyCharacterVisual();
	
	// 서버에서 처리할 것들
	if (HasAuthority())
	{
		ApplyInitialAttributeEffect();
		GiveCharacterDataAbilities();
		SpawnDefaultWeapon();
	}
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
	}
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

void ANSPlayerCharacterBase::BindAttributeDelegates()
{
	if (!NSAbilitySystemComponent || !PlayerAttributeSet)
	{
		return;
	}
	
	// 중복 바인딩을 피하기 위한 바인딩 제거
	NSAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		PlayerAttributeSet->GetMoveSpeedAttribute()).RemoveAll(this);
	
	// 바인딩
	NSAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		PlayerAttributeSet->GetMoveSpeedAttribute()).AddUObject(this, &ThisClass::OnMoveSpeedChanged);
	
	// Attribute 초기화 Effect가 들어오기 전까지 주석처리 : ApplyMoveSpeedToCharacter(PlayerAttributeSet->GetMoveSpeed());
}

void ANSPlayerCharacterBase::GiveDefaultAbilities()
{
	if (!NSAbilitySystemComponent)
	{
		return;
	}
	
	// 서버권한에서만 어빌리티 부여
	if (!HasAuthority())
	{
		return;
	}
	
	for (FNSDefaultAbilityData& AbilityData : DefaultAbilities)
	{
		if (!AbilityData.AbilityClass)
		{
			continue;
		}
		
		if (NSAbilitySystemComponent->FindAbilitySpecFromClass(AbilityData.AbilityClass))
		{
			continue;
		}
		
		FGameplayAbilitySpec AbilitySpec(AbilityData.AbilityClass, 1, INDEX_NONE, this);		
		if (AbilityData.InputTag.IsValid())
		{
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilityData.InputTag);
		}
		
		NSAbilitySystemComponent->GiveAbility(AbilitySpec);
	}
}

void ANSPlayerCharacterBase::ApplyDefaultGameplayEffects()
{
	if (!NSAbilitySystemComponent)
	{
		return;
	}

	// 서버권한에서만 기본 이펙트 부여
	if (!HasAuthority())
	{
		return;
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : DefaultGameplayEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		const UGameplayEffect* EffectCDO = EffectClass->GetDefaultObject<UGameplayEffect>();
		if (EffectCDO && EffectCDO->DurationPolicy != EGameplayEffectDurationType::Instant)
		{
			FGameplayEffectQuery Query;
			Query.EffectDefinition = EffectClass;

			if (NSAbilitySystemComponent->GetActiveEffects(Query).Num() > 0)
			{
				continue;
			}
		}

		FGameplayEffectContextHandle EffectContext = NSAbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		const FGameplayEffectSpecHandle SpecHandle = NSAbilitySystemComponent->MakeOutgoingSpec(
			EffectClass,
			1.f,
			EffectContext);

		if (SpecHandle.IsValid())
		{
			NSAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void ANSPlayerCharacterBase::ApplyCharacterVisual()
{
	if (!CharacterData)
	{
		return;
	}
	
	USkeletalMesh* LoadedMesh = CharacterData->SkeletalMesh.Get();
	if (LoadedMesh)
	{
		GetMesh()->SetSkeletalMesh(LoadedMesh);
	}
	
	UClass* LoadedAnimClass = CharacterData->AnimClass.Get();
	if (LoadedAnimClass)
	{
		GetMesh()->SetAnimInstanceClass(LoadedAnimClass);
	}
}

void ANSPlayerCharacterBase::ApplyInitialAttributeEffect()
{
	if (!HasAuthority() || !CharacterData || !NSAbilitySystemComponent)
	{
		return;
	}
	
	TSubclassOf<UGameplayEffect> LoadedEffectClass = CharacterData->InitialAttributeEffect.Get();
	if (!LoadedEffectClass)
	{
		return;
	}
	
	FGameplayEffectContextHandle EffectContext = NSAbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	
	FGameplayEffectSpecHandle SpecHandle = 
		NSAbilitySystemComponent->MakeOutgoingSpec(LoadedEffectClass, 1.0f, EffectContext);
	
	if (SpecHandle.IsValid())
	{
		NSAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void ANSPlayerCharacterBase::GiveCharacterDataAbilities()
{
	if (!HasAuthority() || !CharacterData || !NSAbilitySystemComponent)
	{
		return;
	}
	
	for (const FNSCharacterAbilityData& AbilityData : CharacterData->DefaultAbilities)
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
		
		NSAbilitySystemComponent->GiveAbility(AbilitySpec);
	}
}

void ANSPlayerCharacterBase::SpawnDefaultWeapon()
{
	if (!HasAuthority() || !CharacterData)
	{
		return;
	}
	
	TSubclassOf<ANSWeaponBase> LoadedWeaponClass = CharacterData->DefaultWeaponClass.Get();
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
