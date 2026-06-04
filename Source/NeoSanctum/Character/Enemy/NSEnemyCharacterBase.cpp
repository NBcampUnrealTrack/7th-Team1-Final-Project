// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "NeoSanctum/GAS/AttributeSet/NSMonsterAttributeSet.h"
#include "NeoSanctum/System/Component/NSDissolveComponent.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/Combat/Component/NSEnemyWeaponComponent.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"

ANSEnemyCharacterBase::ANSEnemyCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	AttributeSet = CreateDefaultSubobject<UNSMonsterAttributeSet>(TEXT("AttributeSet"));

	DissolveComponent = CreateDefaultSubobject<UNSDissolveComponent>(TEXT("DissolveComponent"));
	WeaponComponent = CreateDefaultSubobject<UNSEnemyWeaponComponent>(TEXT("WeaponComponent"));

	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
}

void ANSEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (!ASC) return;

	ASC->InitAbilityActorInfo(this, this);
	
	InitializeFromData(true);
	
	// 디졸브 완료 콜백 바인딩
	if (DissolveComponent && HasAuthority())
	{
		DissolveComponent->OnDissolveComplete.BindUObject(
			this, &ANSEnemyCharacterBase::OnDissolveFinished);
	}
}

void ANSEnemyCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANSEnemyCharacterBase, bIsDead);
	DOREPLIFETIME(ANSEnemyCharacterBase, bIsInPool);
}

void ANSEnemyCharacterBase::Die()
{
	if (bIsDead) return;

	if (HasAuthority())
	{
		bIsDead = true;
		ApplyDeadVisual();
		
		// (이용호 추가) 죽을 때 게임모드에 알림
		AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
		if (GameMode && GameMode->Implements<UNSRunGameModeInterface>())
		{
			INSRunGameModeInterface::Execute_NotifyEnemyKilled(GameMode, this);
		}

		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->UnPossess();
		}

		if (ASC && DeathAbilityClass)
		{
			ASC->TryActivateAbilityByClass(DeathAbilityClass);
		}
	}
}

void ANSEnemyCharacterBase::OnRep_bIsDead()
{
	if (bIsDead)
	{
		ApplyDeadVisual();
	}
	else
	{
		ApplyAliveVisual();
	}
}

void ANSEnemyCharacterBase::ApplyDeadVisual()
{
	// 물리 캡슐 콜리전 비활성화
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

	if (GetMesh())
	{
		// 애니메이션 인스턴스 중단
		GetMesh()->bPauseAnims = true;

		// 콜리전 프로필을 Ragdoll로 변경
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));

		// 스켈레탈 메시의 물리 시뮬레이션을 활성화
		GetMesh()->SetSimulatePhysics(true);

		// 디졸브 효과 적용
		if (DissolveComponent)
		{
			DissolveComponent->StartDissolve();
		}
	}

	OnEnemyDead.Broadcast();
}

void ANSEnemyCharacterBase::InitializeFromData(bool bFullInit)
{
	if (!EnemyData) return;
	
	// 스탯은 최초, 재사용할 때 항상 초기화
	// GAS 데이터 테이블 기반 스탯 초기화
	if (HasAuthority() && EnemyData->AttributeInitData && AttributeSet)
	{
		FName RowName = EnemyData->EnemyTag.GetTagName();
		FNSMonsterAttributeRow* StatRow = 
			EnemyData->AttributeInitData->FindRow<FNSMonsterAttributeRow>(RowName, TEXT(""));

		if (StatRow)
		{
			AttributeSet->SetMaxHealth(StatRow->MaxHealth);
			AttributeSet->SetHealth(StatRow->MaxHealth);
			AttributeSet->SetDefense(StatRow->Defense);
			AttributeSet->SetBaseDamage(StatRow->BaseDamage);
		}
	}
	
	// 어빌리티, 메시, 무기 등은 최초 생성 1회시에만 적용
	if (bFullInit)
	{
		// Visual 동적 로딩
		if (GetMesh())
		{
			if (EnemyData->SkeletalMesh)
			{
				GetMesh()->SetSkeletalMeshAsset(EnemyData->SkeletalMesh);
			}
			if (EnemyData->BaseAnimClass)
			{
				GetMesh()->SetAnimInstanceClass(EnemyData->BaseAnimClass);
			}
		}
		SetActorScale3D(EnemyData->DrawScale);

		// 서버 권한 초기 이펙트 및 고유 어빌리티 일괄 부여
		if (HasAuthority())
		{
			for (const TSubclassOf<UGameplayEffect>& EffectClass : EnemyData->StartupEffects)
			{
				if (EffectClass)
				{
					FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
					Context.AddSourceObject(this);

					FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, Context);
					if (SpecHandle.IsValid())
					{
						ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
					}
				}
			}

			for (const TSubclassOf<UGameplayAbility>& AbilityClass : EnemyData->StartupAbilities)
			{
				if (AbilityClass)
				{
					ASC->GiveAbility(FGameplayAbilitySpec(
						AbilityClass,
						1,
						static_cast<int32>(AbilityClass.GetDefaultObject()->GetNetExecutionPolicy())));
				}
			}
		}

		// 서버에서만 사망 능력 부여
		if (HasAuthority())
		{
			if (DeathAbilityClass)
			{
				ASC->GiveAbility(FGameplayAbilitySpec(DeathAbilityClass, 1, -1));
			}
		}

		// 무기 장착, ABP와 공격 어빌리티 부여
		if (WeaponComponent)
		{
			WeaponComponent->EquipWeapon();
		}
	}
}

void ANSEnemyCharacterBase::ApplyAliveVisual()
{
	SetActorHiddenInGame(false);

	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CapsuleComp->SetCollisionResponseToAllChannels(ECR_Block);
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetSimulatePhysics(false);
		MeshComp->AttachToComponent(
			GetCapsuleComponent(),
			FAttachmentTransformRules::SnapToTargetIncludingScale);
		MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		MeshComp->SetCollisionProfileName(TEXT("CharacterMesh"));
		MeshComp->bPauseAnims = false;
	}

	if (DissolveComponent)
	{
		DissolveComponent->ResetDissolve();
	}
}

void ANSEnemyCharacterBase::OnDissolveFinished()
{
	if (!HasAuthority())
	{
		return;
	}

	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (GameMode && GameMode->Implements<UNSRunGameModeInterface>())
	{
		INSRunGameModeInterface::Execute_ReturnMonsterToPool(GameMode, this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("풀 매니저 없음, 풀 반환 실패"));
	}
}

void ANSEnemyCharacterBase::SetEnemyData(UNSEnemyData* InEnemyData)
{
	if (!HasAuthority() || !InEnemyData)
	{
		return;
	}

	EnemyData = InEnemyData;
}

void ANSEnemyCharacterBase::PrepareForReuse(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsInPool = false;
	bIsDead = false;

	SetActorLocationAndRotation(
		SpawnLocation,
		SpawnRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	SetActorTickEnabled(true);
	SetActorEnableCollision(true);

	ApplyAliveVisual();

	// 스탯만 리셋
	InitializeFromData(false);

	// BT 정상 작동을 위해 AIControllerClass로 재빙의
	SpawnDefaultController();
}

void ANSEnemyCharacterBase::DeactivateForPool()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsInPool = true;

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}
