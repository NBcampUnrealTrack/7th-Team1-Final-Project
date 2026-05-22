// Copyright 2026 One Team. All rights reserved.


#include "NSPlayerCharacterBase.h"

#include "Camera/CameraComponent.h"
#include "CharacterTrajectoryComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "NeoSanctum/Character/Component/NSInputBinderComponent.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
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
		PlayerAttributeSet->GetHealthAttribute()).RemoveAll(this);
	NSAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		PlayerAttributeSet->GetShieldAttribute()).RemoveAll(this);
	NSAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		PlayerAttributeSet->GetMoveSpeedAttribute()).RemoveAll(this);
	
	// 바인딩
	NSAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		PlayerAttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
	NSAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		PlayerAttributeSet->GetShieldAttribute()).AddUObject(this, &ThisClass::OnShieldChanged);
	NSAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		PlayerAttributeSet->GetMoveSpeedAttribute()).AddUObject(this, &ThisClass::OnMoveSpeedChanged);
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
	
	for (TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (!AbilityClass)
		{
			continue;
		}
		
		if (NSAbilitySystemComponent->FindAbilitySpecFromClass(AbilityClass))
		{
			continue;
		}
		
		FGameplayAbilitySpec AbilitySpec(AbilityClass, 1, INDEX_NONE, this);
		NSAbilitySystemComponent->GiveAbility(AbilitySpec);
	}
}

void ANSPlayerCharacterBase::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	
}

void ANSPlayerCharacterBase::OnShieldChanged(const FOnAttributeChangeData& Data)
{
	
}

void ANSPlayerCharacterBase::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = Data.NewValue;
	}
}
