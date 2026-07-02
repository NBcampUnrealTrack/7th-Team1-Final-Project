// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyDroneAI.h"

#include "Components/SphereComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/System/Component/NSDissolveComponent.h"
#include "NeoSanctum/AI/Enemy/Controller/NSEnemyDroneAIController.h"
#include "NeoSanctum/Collision/NSCollisionProfiles.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"
#include "NeoSanctum/GAS/AttributeSet/NSEnemyDroneAttributeSet.h"
#include "NeoSanctum/System/Component/NSDamageFlashComponent.h"
#include "Net/UnrealNetwork.h"


ANSEnemyDroneAI::ANSEnemyDroneAI()
{
	PrimaryActorTick.bCanEverTick = true;
	TeamId = ETeamId::Enemy;
	AIControllerClass = ANSEnemyDroneAIController::StaticClass();
	EnemyDroneAttributeSet = CreateDefaultSubobject<UNSEnemyDroneAttributeSet>("AttributeSet");
	
	DissolveComponent = CreateDefaultSubobject<UNSDissolveComponent>(TEXT("DissolveComponent"));
	DamageFlashComponent = CreateDefaultSubobject<UNSDamageFlashComponent>(TEXT("DamageFlashComponent"));
}

void ANSEnemyDroneAI::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
}

void ANSEnemyDroneAI::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ANSEnemyDroneAI, CurrentEnemyData);
	DOREPLIFETIME(ANSEnemyDroneAI, bIsDead);
	DOREPLIFETIME(ANSEnemyDroneAI, bIsInPool);
}

void ANSEnemyDroneAI::InitializeFromData()
{
	Super::InitializeFromData();
	InitializeEnemyDroneData(true);
}

void ANSEnemyDroneAI::SetPendingEnemyData(const UNSEnemyData* InEnemyData)
{
	if (!InEnemyData) return;
	
	CurrentEnemyData = InEnemyData;
}

void ANSEnemyDroneAI::BeginPlay()
{
	Super::BeginPlay();
	
	// 디졸브 완료 콜백 바인딩
	if (DissolveComponent && HasAuthority())
	{
		DissolveComponent->OnDissolveComplete.BindUObject(this, &ANSEnemyDroneAI::OnDissolveFinished);
	}
}

void ANSEnemyDroneAI::InitializeEnemyDroneData(bool bFullInit)
{
	if (!CurrentEnemyData) return;
	// 스탯은 최초, 재사용할 때 항상 초기화
	// GAS 데이터 테이블 기반 스탯 초기화
	if (HasAuthority() && CurrentEnemyData->AttributeInitData && EnemyDroneAttributeSet)
	{
		FName RowName = CurrentEnemyData->EnemyId.GetTagName();
		FNSMonsterAttributeRow* StatRow =
			CurrentEnemyData->AttributeInitData->FindRow<FNSMonsterAttributeRow>(RowName, TEXT(""));

		if (StatRow)
		{
			const FNSDifficultyScale& S = CurrentDifficultyScale;
			const float ScaledMaxHealth  =
				(StatRow->MaxHealth  * (1.0f + S.HealthAddRatio)) * S.Multiply;
			const float ScaledBaseDamage = 
				(StatRow->BaseDamage * (1.0f + S.DamageAddRatio)) * S.Multiply;
			const float ScaledDefense = 
				(StatRow->Defense * (1.0f + S.DefenseAddRatio)) * S.Multiply;

			EnemyDroneAttributeSet->SetMaxHealth (ScaledMaxHealth);
			EnemyDroneAttributeSet->SetHealth    (ScaledMaxHealth);
			EnemyDroneAttributeSet->SetBaseDamage(ScaledBaseDamage);
			EnemyDroneAttributeSet->SetDefense   (ScaledDefense);
		}
	}
	// 어빌리티, 메시, 무기 등은 최초 생성 1회시에만 적용
	if (bFullInit)
	{
		ApplyVisualData();

		// 서버 권한 초기 이펙트 및 고유 어빌리티 일괄 부여
		if (HasAuthority())
		{
			for (const TSubclassOf<UGameplayEffect>& EffectClass : CurrentEnemyData->DefaultEffects)
			{
				if (EffectClass)
				{
					FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
					Context.AddSourceObject(this);

					FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.0f, Context);
					if (SpecHandle.IsValid())
					{
						AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
					}
				}
			}
			
			TSet<TObjectPtr<UClass>> GrantedAbilityClasses;
			
			auto GiveAbilityOnce = [this, &GrantedAbilityClasses](TSubclassOf<UGameplayAbility> AbilityClass)
			{
				if (!AbilitySystemComponent || !AbilityClass)
				{
					return;
				}

				UClass* AbilityRawClass = AbilityClass.Get();
				if (!AbilityRawClass || GrantedAbilityClasses.Contains(AbilityRawClass))
				{
					return;
				}

				GrantedAbilityClasses.Add(AbilityRawClass);

				const UGameplayAbility* AbilityCDO = AbilityClass.GetDefaultObject();
				if (!AbilityCDO)
				{
					return;
				}

				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(
					AbilityClass,
					1,
					INDEX_NONE));
			};

			for (const TSubclassOf<UGameplayAbility>& AbilityClass : CurrentEnemyData->DefaultAbilities)
			{
				GiveAbilityOnce(AbilityClass);
			}
			
			GiveAbilityOnce(CurrentEnemyData->HitReactionAbilityClass);
			
			for (const FNSEnemyAttackDefinition& AttackDefinition : CurrentEnemyData->AttackList)
			{
				GiveAbilityOnce(AttackDefinition.AbilityClass);
			}
		}

		// 서버에서만 사망 능력 부여
		if (HasAuthority())
		{
			if (DeathAbilityClass)
			{
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DeathAbilityClass, 1, -1));
			}
		}
	}
}

void ANSEnemyDroneAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ANSEnemyDroneAI::Die()
{
	if (bIsDead) return;

	if (HasAuthority())
	{
		bIsDead = true;
		ApplyDeadVisual();
		
		OnEnemyDroneDead.Broadcast(this);
		
		// (이용호 추가) 죽을 때 게임모드에 알림
		AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
		if (GameMode && GameMode->Implements<UNSRunGameModeInterface>())
		{
			INSRunGameModeInterface::Execute_NotifyEnemyDroneKilled(GameMode, this);
		}

		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->UnPossess();
			AIController->Destroy();
		}

		if (AbilitySystemComponent && DeathAbilityClass)
		{
			AbilitySystemComponent->TryActivateAbilityByClass(DeathAbilityClass);
		}
	}
}

void ANSEnemyDroneAI::OnRep_bIsInPool()
{
	// 클라이언트에 남아있는 콜리전 정리
	SetActorHiddenInGame(bIsInPool);
	SetActorEnableCollision(!bIsInPool);
}

void ANSEnemyDroneAI::SetOwnerBoss(AActor* InOwnerBoss)
{
	if (!InOwnerBoss) return;
	
	OwnerBoss = InOwnerBoss;
}

void ANSEnemyDroneAI::OnRep_CurrentEnemyData()
{
	ApplyVisualData();
}

void ANSEnemyDroneAI::ApplyAliveVisual()
{
	SetActorHiddenInGame(false);

	if (SphereComponent)
	{
		SphereComponent->SetCollisionProfileName(NSCollisionProfiles::EnemyCharacter);
	}

	if (USkeletalMeshComponent* MeshComp = SkeletalMeshComponent)
	{
		MeshComp->SetSimulatePhysics(false);
		MeshComp->AttachToComponent(
			SphereComponent,
			FAttachmentTransformRules::SnapToTargetIncludingScale);
		/*MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));*/
		MeshComp->SetCollisionProfileName(TEXT("CharacterMesh"));
		MeshComp->bPauseAnims = false;
	}

	if (DissolveComponent)
	{
		DissolveComponent->ResetDissolve();
	}
}

void ANSEnemyDroneAI::ApplyDeadVisual()
{
	// 물리 캡슐 콜리전 비활성화
	if (!SphereComponent) return;
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);

	if (SkeletalMeshComponent)
	{
		// 애니메이션 인스턴스 중단
		SkeletalMeshComponent->bPauseAnims = true;

		// 콜리전 프로필을 Ragdoll로 변경
		SkeletalMeshComponent->SetCollisionProfileName(TEXT("Ragdoll"));

		// 스켈레탈 메시의 물리 시뮬레이션을 활성화
		SkeletalMeshComponent->SetSimulatePhysics(true);

		// 디졸브 효과 적용
		if (DissolveComponent)
		{
			DissolveComponent->StartDissolve();
		}
	}


}

void ANSEnemyDroneAI::ApplyVisualData()
{
	if (!CurrentEnemyData || !SkeletalMeshComponent)
	{
		return;
	}

	if (CurrentEnemyData->SkeletalMesh)
	{
		SkeletalMeshComponent->SetSkeletalMeshAsset(CurrentEnemyData->SkeletalMesh);
		InitializeRuntimeMaterials();
	}

	if (CurrentEnemyData->AnimClass)
	{
		SkeletalMeshComponent->SetAnimInstanceClass(CurrentEnemyData->AnimClass);
	}

	SetActorScale3D(CurrentEnemyData->DrawScale);
}

void ANSEnemyDroneAI::OnDissolveFinished()
{
	if (!HasAuthority())
	{
		return;
	}

	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (GameMode && GameMode->Implements<UNSRunGameModeInterface>())
	{
		INSRunGameModeInterface::Execute_ReturnEnemyDroneToPool(GameMode, this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("풀 매니저 없음, 풀 반환 실패"));
	}
}

void ANSEnemyDroneAI::OnRep_bIsDead()
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

void ANSEnemyDroneAI::InitializeRuntimeMaterials()
{
	USkeletalMeshComponent* MeshComponent = SkeletalMeshComponent;
	if (!MeshComponent || !CurrentEnemyData)
	{
		return;
	}

	if (DamageFlashComponent)
	{
		DamageFlashComponent->ClearMaterialFlashTargets();
	}

	RuntimeVisualMaterials.Reset();

	TArray<UMaterialInstanceDynamic*> FlashTargets;

	for (const FNSEnemyMaterialDefinition& Definition : CurrentEnemyData->MaterialDefinitions)
	{
		const int32 MaterialIndex = MeshComponent->GetMaterialIndex(Definition.MaterialSlotName);

		if (MaterialIndex == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Enemy material slot not found: %s"),
				   *Definition.MaterialSlotName.ToString());

			continue;
		}

		UMaterialInterface* InitialMaterial = Definition.InitialMaterial
												  ? Definition.InitialMaterial.Get()
												  : MeshComponent->GetMaterial(MaterialIndex);

		if (!InitialMaterial)
		{
			continue;
		}

		MeshComponent->SetMaterial(MaterialIndex, InitialMaterial);

		UMaterialInstanceDynamic* MID =
			MeshComponent->CreateDynamicMaterialInstance(MaterialIndex, InitialMaterial);

		if (!MID)
		{
			continue;
		}

		MID->SetVectorParameterValue(TEXT("MonsterTint"), Definition.MonsterTint);
		MID->SetScalarParameterValue(TEXT("HitFlashAmount"), 0.0f);

		RuntimeVisualMaterials.Add(MID);
		FlashTargets.Add(MID);
	}

	if (DamageFlashComponent)
	{
		DamageFlashComponent->SetMaterialFlashTargets(FlashTargets);
	}
}

void ANSEnemyDroneAI::PrepareForReuse(const FVector& SpawnLocation, const FRotator& SpawnRotation)
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
	
	// 이동을 멈췄으므로 재가동
	if (FloatingPawnMovementComponent)
	{
		FloatingPawnMovementComponent->Activate();
	}
	
	ApplyAliveVisual();

	// 종료할 때 전부 없앴으므로 전부 재주입
	InitializeEnemyDroneData(true);

	// BT 정상 작동을 위해 AIControllerClass로 재빙의
	SpawnDefaultController();
}

void ANSEnemyDroneAI::DeactivateForPool()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsInPool = true;

	// 이동 즉시 정지 및 비활성화
	if (FloatingPawnMovementComponent)
	{
		FloatingPawnMovementComponent->StopMovementImmediately();
		FloatingPawnMovementComponent->Deactivate();
	}

	// 진행 중인 몽타주 정지
	if (SkeletalMeshComponent)
	{
		if (UAnimInstance* Anim = SkeletalMeshComponent->GetAnimInstance())
		{
			Anim->StopAllMontages(0.0f);
		}
	}

	// 살아있는 채로 반환된 경우 AI, 컨트롤러 정리
	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
		AICon->UnPossess();
		AICon->Destroy();
	}

	// GAS 정리(실행 중 어빌리티 취소,활성 이펙트 전부 제거,그랜트 해제)
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();
		AbilitySystemComponent->RemoveActiveEffects(FGameplayEffectQuery());
		AbilitySystemComponent->ClearAllAbilities();
	}
	
	if (DamageFlashComponent)
	{
		DamageFlashComponent->CancelFlash();
	}

	// 물리적 중지
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}