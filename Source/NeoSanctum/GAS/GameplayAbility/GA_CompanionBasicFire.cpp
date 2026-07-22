// Copyright 2026 One Team. All rights reserved.


#include "GA_CompanionBasicFire.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NeoSanctum/AI/Base/NSBaseDroneAI.h"
#include "NeoSanctum/GAS/AttributeSet/NSCompanionAttributeSet.h"
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"
#include "NeoSanctum/AI/Companion/Pawn/NSCompanionDroneAI.h"
#include "NeoSanctum/AI/Companion/Spawner/NSDroneProjectile.h"
#include "AbilitySystemComponent.h"
#include "NeoSanctum/AI/Companion/Pawn/NSCompanionDroneAI.h"
#include "NeoSanctum/Tag/NSGameplayTags_Companion.h"
#include "NeoSanctum/Tag/NSGameplayTags_Effect.h"

UGA_CompanionBasicFire::UGA_CompanionBasicFire()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	
	FGameplayTagContainer CompanionAbilityTags = GetAssetTags();
	CompanionAbilityTags.AddTag(NSGameplayTags::Ability_Companion_Active);
	SetAssetTags(CompanionAbilityTags);
	
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Companion_Disable);
	
	DamageSetTag = NSGameplayTags::Effect_Damage_Base;
	CoolDownTag = NSGameplayTags::Data_Companion_CoolDown;
}

bool UGA_CompanionBasicFire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;
	
	if (!GetCombatTarget()) return false;
	
	return true;
}

void UGA_CompanionBasicFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                             const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	AActor* TargetActor = GetCombatTarget();
	
	if (!TargetActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	ANSBaseDroneAI* AvatarActor = Cast<ANSBaseDroneAI>(ActorInfo->AvatarActor.Get());
	if (!AvatarActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;	
	}
	
	if (!CanFireAt(TargetActor))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	const int32 MuzzleCount = MuzzleSocketNames.Num();
	if (MuzzleCount <= 0)
	{
		// BP의 MuzzleSocketNames가 비어 있으면 발사 불가
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 모든 머즐 소켓에서 동시에 이펙트 + 투사체 발사 (데미지는 소켓 수로 분할)
	for (const FName& SocketName : MuzzleSocketNames)
	{
		const FVector Muzzle = GetMuzzleSocketLocation(SocketName);
		const FVector Dir = ComputeAimDirection(Muzzle, TargetActor);
		if (Dir.IsNearlyZero())
		{
			continue;
		}

		PlayFireCue(Muzzle, Dir);
		FireProjectile(Muzzle, Dir, TargetActor, MuzzleCount);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_CompanionBasicFire::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UGameplayEffect* CoolDownEffect = GetCooldownGameplayEffect();
	if (!CoolDownEffect) return;
	
	FGameplayEffectSpecHandle CoolDownSpecHandle = 
		MakeOutgoingGameplayEffectSpec(CoolDownEffect->GetClass(), GetAbilityLevel());
	
	const UNSCompanionAttributeSet* Set = GetCompanionSet();
	if (!Set) return;
	
	float CoolDownInterval = (1.f / Set->GetFireRate());
	
	if (!CoolDownSpecHandle.IsValid()) return;
	CoolDownSpecHandle.Data->SetSetByCallerMagnitude(CoolDownTag, CoolDownInterval);
	
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CoolDownSpecHandle);
}

FVector UGA_CompanionBasicFire::ComputeAimDirection(const FVector& Muzzle, AActor* Target) const
{
	// 타겟이 없다면 직선방향으로 발사되게
	if (!Target) return FVector::ZeroVector;
	
	const FVector ToTarget = Target->GetActorLocation() - Muzzle;
	
	// 예측이 필요없다면 적고정상태
	if (!bPredictiveAim)
	{
		// 해당 적방향으로 발사
		return ToTarget.GetSafeNormal();
	}
	
	const UNSCompanionAttributeSet* Set = GetCompanionSet();
	if (!Set) return ToTarget.GetSafeNormal();
	
	// 타겟과 드론사이의 거리 / 탄환 스피드
	const float FlightTime = ToTarget.Size() / Set->GetProjectileSpeed();
	
	// 목표방향은 타겟위치 + 타겟이 달리는 방향 * 예측한 비행시간
	FVector ComputePoint = Target->GetActorLocation() + Target->GetVelocity() * FlightTime;
	
	// 예측해서 나온 위치의 방향 반환
	return (ComputePoint - Muzzle).GetSafeNormal();
}

void UGA_CompanionBasicFire::PlayFireCue(const FVector& Location, const FVector& Dir) const
{
	if (!FireCueTag.IsValid()) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	AActor* Avatar = GetAvatarActorFromActorInfo();

	FGameplayCueParameters CueParams;
	CueParams.Instigator = Avatar;
	CueParams.EffectCauser = Avatar;
	CueParams.Location = Location;   // 소켓 위치
	CueParams.Normal = Dir;          // 발사 방향

	ASC->ExecuteGameplayCue(FireCueTag, CueParams);
}

FVector UGA_CompanionBasicFire::GetMuzzleSocketLocation(FName SocketName) const
{
	ANSBaseDroneAI* CompanionAI = Cast<ANSBaseDroneAI>(GetAvatarActorFromActorInfo());
	if (!CompanionAI) return FVector::ZeroVector;

	USkeletalMeshComponent* Mesh = CompanionAI->GetSkeletalMeshComponent();
	if (Mesh && !SocketName.IsNone() && Mesh->DoesSocketExist(SocketName))
	{
		return Mesh->GetSocketLocation(SocketName);
	}
	return CompanionAI->GetActorLocation();
}

const UNSCompanionAttributeSet* UGA_CompanionBasicFire::GetCompanionSet() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return nullptr;
	
	return ASC->GetSet<UNSCompanionAttributeSet>();
}

AActor* UGA_CompanionBasicFire::GetCombatTarget() const
{
	ANSBaseDroneAI* DronePawn = Cast<ANSBaseDroneAI>(GetAvatarActorFromActorInfo());
	if (!DronePawn) return nullptr;
	
	ANSDroneAIController* DroneAIController = Cast<ANSDroneAIController>(DronePawn->GetController());
	if (!DroneAIController) return nullptr;
	
	UBlackboardComponent* BB = DroneAIController->GetBlackboardComponent();
	if (!BB) return nullptr;
	
	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(EnemyTargetKey));
	if (!TargetActor) return nullptr;
	
	return TargetActor;
}

bool UGA_CompanionBasicFire::CanFireAt(AActor* Target) const
{
	if (!Target) return false;

	const UNSCompanionAttributeSet* Set = GetCompanionSet();
	if (!Set) return false;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor) return false;

	// 검증 기준점: 드론 액터 위치 (소켓별로 나눠 검사하지 않음)
	const FVector Origin = AvatarActor->GetActorLocation();
	const FVector TargetLocation = Target->GetActorLocation();

	if (FVector::Dist(Origin, TargetLocation) > Set->GetAttackRange()) return false;

	FHitResult Hit;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(AvatarActor);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, Origin, TargetLocation, ECC_Visibility, CollisionParams);

	if (bHit && Hit.GetActor() != Target) return false;

	return true;
}

void UGA_CompanionBasicFire::FireProjectile(const FVector& Muzzle, const FVector& Dir, AActor* Target, int32 MuzzleCount)
{
	const UNSCompanionAttributeSet* Set = GetCompanionSet();
	if (!Set) return;

	const float Speed = Set->GetProjectileSpeed();
	if (Speed <= 0.f) return;

	FGameplayEffectSpecHandle DamageSpec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	if (!DamageSpec.IsValid()) return;

	// 소유 플레이어가 있으면 데미지 숫자가 그 플레이어에게 뜨도록 Instigator 지정
	if (ANSCompanionDroneAI* DroneAI = Cast<ANSCompanionDroneAI>(GetAvatarActorFromActorInfo()))
	{
		if (APawn* OwnerPawn = Cast<APawn>(DroneAI->GetOwnerPlayer()))
		{
			DamageSpec.Data->GetContext().AddInstigator(OwnerPawn, DroneAI);
		}
	}

	// 양쪽 소켓 동시 발사 시 총 데미지가 유지되도록 소켓 수로 분할
	const float PerShotDamage = Set->GetAttackDamage() / FMath::Max(1, MuzzleCount);
	DamageSpec.Data->SetSetByCallerMagnitude(DamageSetTag, PerShotDamage);

	APawn* InstigatorPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	FTransform MuzzleTransform(Dir.Rotation(), Muzzle);

	ANSDroneProjectile* Projectile = GetWorld()->SpawnActorDeferred<ANSDroneProjectile>(
		ProjectileClass,
		MuzzleTransform,
		GetAvatarActorFromActorInfo(), InstigatorPawn,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (Projectile)
	{
		Projectile->InitProjectile(Dir, InstigatorPawn, DamageSpec, Speed);
		Projectile->FinishSpawning(MuzzleTransform);
	}
}
