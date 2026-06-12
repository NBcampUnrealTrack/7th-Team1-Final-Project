// Copyright 2026 One Team. All rights reserved.


#include "GA_CompanionBasicFire.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NeoSanctum/AI/Companion/Base/NSBaseCompanionAI.h"
#include "NeoSanctum/GAS/AttributeSet/NSCompanionAttributeSet.h"
#include "NeoSanctum/AI/Companion/Controller/DroneAI/NSDroneAIController.h"
#include "NeoSanctum/AI/Companion/Spawner/NSDroneProjectile.h"
#include "NeoSanctum/Tag/NSGameplayTags_Companion.h"

UGA_CompanionBasicFire::UGA_CompanionBasicFire()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	
	AbilityTags.AddTag(NSGameplayTags::Ability_Companion_Fire);
	ActivationBlockedTags.AddTag(NSGameplayTags::State_Companion_Disable);
	
	DamageSetTag = NSGameplayTags::Data_Companion_Damage;
	CoolDownTag = NSGameplayTags::Data_Companion_CoolDown;
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
	
	ANSBaseCompanionAI* AvatarActor = Cast<ANSBaseCompanionAI>(ActorInfo->AvatarActor.Get());
	if (!AvatarActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;	
	}
	
	FVector MuzzleSocket;
	FVector OutDir;
	
	if (!CanFireAt(TargetActor, MuzzleSocket, OutDir))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	FireProjectile(MuzzleSocket, OutDir, TargetActor);
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

FVector UGA_CompanionBasicFire::GetMuzzleSocketLocation() const
{
	ANSBaseCompanionAI* CompanionAI = Cast<ANSBaseCompanionAI>(GetAvatarActorFromActorInfo());
	if (!CompanionAI) return FVector::ZeroVector;
	
	// 매쉬 캐싱
	USkeletalMeshComponent* Mesh = CompanionAI->GetSkeletalMeshComponent();
	
	// 매쉬가 있고 이름과 일치하는 소켓이 있다면
	if (Mesh && Mesh->DoesSocketExist(MuzzleSocketName))
	{
		// 소켓위치 반환
		return Mesh->GetSocketLocation(MuzzleSocketName);
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
	ANSBaseCompanionAI* DronePawn = Cast<ANSBaseCompanionAI>(GetAvatarActorFromActorInfo());
	if (!DronePawn) return nullptr;
	
	ANSDroneAIController* DroneAIController = Cast<ANSDroneAIController>(DronePawn->GetController());
	if (!DroneAIController) return nullptr;
	
	UBlackboardComponent* BB = DroneAIController->GetBlackboardComponent();
	if (!BB) return nullptr;
	
	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(EnemyTargetKey));
	if (!TargetActor) return nullptr;
	
	return TargetActor;
}

bool UGA_CompanionBasicFire::CanFireAt(AActor* Target, FVector& OutMuzzle, FVector& OutDir) const
{
	if (!Target) return false;
	
	const UNSCompanionAttributeSet* Set = GetCompanionSet();
	if (!Set) return false;
	
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor) return false;
	
	OutMuzzle = GetMuzzleSocketLocation();
	
	const FVector TargetLocation = Target->GetActorLocation();
	const FVector ToTarget = TargetLocation - OutMuzzle;
	OutDir = ComputeAimDirection(OutMuzzle, Target);
	
	const float DistToTarget = ToTarget.Size();
	if (DistToTarget > Set->GetAttackRange()) return false;
	
	FHitResult Hit;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(AvatarActor);
	
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		OutMuzzle,
		TargetLocation,
		ECC_Visibility,
		CollisionParams);
	
	if (bHit && Hit.GetActor() != Target) return false;
	
	return true;
}

void UGA_CompanionBasicFire::FireProjectile(const FVector& Muzzle, const FVector& Dir, AActor* Target)
{
	const UNSCompanionAttributeSet* Set = GetCompanionSet();
	if (!Set) return;
	
	FGameplayEffectSpecHandle DamageSpec = MakeOutgoingGameplayEffectSpec(DamageEffectClass);
	if (!DamageSpec.IsValid()) return;
	
	DamageSpec.Data->SetSetByCallerMagnitude(DamageSetTag, Set->GetAttackDamage());
	
	const float Speed = Set->GetProjectileSpeed();
	if (Speed <= 0.f) return;
	
	APawn* InstigatorPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	
	FTransform MuzzleTransform(Dir.Rotation(), Muzzle);
	
	ANSDroneProjectile* Projectile = GetWorld()->SpawnActorDeferred<ANSDroneProjectile>(
		ProjectileClass,
		MuzzleTransform,
		GetAvatarActorFromActorInfo(), InstigatorPawn,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);
	
	if (Projectile)
	{
		Projectile->InitProjectile(Dir, InstigatorPawn, DamageSpec, Speed);
		
		Projectile->FinishSpawning(MuzzleTransform);
	}
	
}
