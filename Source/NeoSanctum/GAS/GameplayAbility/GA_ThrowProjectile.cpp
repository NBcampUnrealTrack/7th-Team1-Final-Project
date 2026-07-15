// Copyright 2026 One Team. All rights reserved.


#include "GA_ThrowProjectile.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Combat/Projectile/NSGrenade.h"
#include "NeoSanctum/Combat/Projectile/NSThrowProjectileBase.h"
#include "NeoSanctum/Combat/Projectile/NSTurretSpawner.h"
#include "NeoSanctum/Combat/Projectile/NSVanguardBarrierFieldProjectile.h"
#include "NeoSanctum/Tag/NSGameplayTags_CombatStat.h"
#include "NeoSanctum/Tag/NSGameplayTags_State.h"

UGA_ThrowProjectile::UGA_ThrowProjectile()
{
	ActivationPolicy = ENSAbilityActivationPolicy::OnInputTriggered;
}

void UGA_ThrowProjectile::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (!AnimMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!ProjectileAbilityConfig.ProjectileClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Ability가 새로 활성화될 때 이전 입력/Notify 처리 상태를 초기화
	bReleaseRequested = false;
	bProjectileThrown = false;

	// 투척물에 전달할 CombatStat payload 생성
	RebuildCombatStatPayloads();

	OnTargetDataReadyCallbackDelegateHandle = ASC->AbilityTargetDataSetDelegate(
		Handle,
		ActivationInfo.GetActivationPredictionKey()
	).AddUObject(this, &ThisClass::OnTargetDataReadyCallback);

	if (ActorInfo->IsNetAuthority() && !ActorInfo->IsLocallyControlled())
	{
		ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, ActivationInfo.GetActivationPredictionKey());
	}

	// 몽타주 시작
	ThrowMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		TEXT("ThrowMontageTask"),
		AnimMontage,
		1.0f,
		NAME_None,
		false,
		1.0f,
		0.0f
	);

	if (!ThrowMontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartGameplayEventTasks();
	
	ThrowMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnThrowMontageCompleted);
	ThrowMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnThrowMontageCompleted);
	ThrowMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnThrowMontageInterrupted);
	ThrowMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnThrowMontageInterrupted);
	AddDeactivateHandIKTag();
	ThrowMontageTask->ReadyForActivation();
	
	if (ActorInfo->IsLocallyControlled())
	{
		// TODO : 던지기 궤적 + 탄착 지점 프리뷰
	}
}

void UGA_ThrowProjectile::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	// Release 섹션 재생 중 추가 입력이 들어오면 같은 섹션으로 다시 점프하지 않음
	if (bReleaseRequested)
	{
		return;
	}

	bReleaseRequested = true;

	if (!TryJumpToReleaseSection())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UGA_ThrowProjectile::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// TODO : 프리뷰 종료
	DestroyHeldMesh();
	RemoveDeactivateHandIKTag();

	// Ability 종료 시 다음 활성화를 위해 입력/투척 게이트를 정리
	bReleaseRequested = false;
	bProjectileThrown = false;

	if (AttachProjectileEventTask)
	{
		AttachProjectileEventTask->EndTask();
		AttachProjectileEventTask = nullptr;
	}

	if (ThrowProjectileEventTask)
	{
		ThrowProjectileEventTask->EndTask();
		ThrowProjectileEventTask = nullptr;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (OnTargetDataReadyCallbackDelegateHandle.IsValid())
		{
			ASC->AbilityTargetDataSetDelegate(
				Handle,
				ActivationInfo.GetActivationPredictionKey()
			).Remove(OnTargetDataReadyCallbackDelegateHandle);

			OnTargetDataReadyCallbackDelegateHandle.Reset();
		}

		ASC->ConsumeClientReplicatedTargetData(Handle, ActivationInfo.GetActivationPredictionKey());
	}

	// 몽타주 종료
	if (ThrowMontageTask)
	{
		ThrowMontageTask->EndTask();
		ThrowMontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_ThrowProjectile::OnThrowMontageCompleted()
{
	FinishThrowProjectileAbility(false);
}

void UGA_ThrowProjectile::OnThrowMontageInterrupted()
{
	FinishThrowProjectileAbility(true);
}

void UGA_ThrowProjectile::OnAttachProjectileEventReceived(FGameplayEventData Payload)
{
	AttachHeldMesh();
}

void UGA_ThrowProjectile::OnThrowProjectileEventReceived(FGameplayEventData Payload)
{
	// Release AnimNotify가 중복 발생해도 한 번의 Ability 활성화에서 Projectile은 한 번만 던짐
	if (bProjectileThrown)
	{
		return;
	}

	bProjectileThrown = true;
	DestroyHeldMesh();

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (ActorInfo && ActorInfo->IsNetAuthority() && !ActorInfo->IsLocallyControlled())
	{
		return;
	}

	FHitResult AimHitResult;
	if (!TryBuildProjectileAimTrace(AimHitResult))
	{
		return;
	}

	const FGameplayAbilityTargetDataHandle TargetDataHandle = MakeTargetDataFromHitResult(AimHitResult);
	OnTargetDataReadyCallback(TargetDataHandle, FGameplayTag());
}

void UGA_ThrowProjectile::StartGameplayEventTasks()
{
	// AttachTag에서 설정한 EventTag 대기
	AttachProjectileEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		AttachTag,
		nullptr,
		false,
		false
	);
	
	// 바인딩
	if (AttachProjectileEventTask)
	{
		AttachProjectileEventTask->EventReceived.AddDynamic(this, &ThisClass::OnAttachProjectileEventReceived);
		AttachProjectileEventTask->ReadyForActivation();
	}
	
	// ReleaseTag에서 설정한 EventTag 대기
	ThrowProjectileEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		ReleaseTag,
		nullptr,
		false,
		false
	);
	
	// 바인딩
	if (ThrowProjectileEventTask)
	{
		ThrowProjectileEventTask->EventReceived.AddDynamic(this, &ThisClass::OnThrowProjectileEventReceived);
		ThrowProjectileEventTask->ReadyForActivation();
	}
}

bool UGA_ThrowProjectile::TryJumpToReleaseSection() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr;
	if (!AnimInstance || !AnimMontage || ReleaseSectionName.IsNone())
	{
		return false;
	}

	if (!AnimInstance->Montage_IsPlaying(AnimMontage))
	{
		return false;
	}

	AnimInstance->Montage_JumpToSection(ReleaseSectionName, AnimMontage);
	return true;
}

void UGA_ThrowProjectile::FinishThrowProjectileAbility(bool bWasCancelled)
{
	if (!IsActive())
	{
		return;
	}

	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		bWasCancelled
	);
}

void UGA_ThrowProjectile::AttachHeldMesh()
{
	if (HoldMeshComponent || !HoldStaticMesh)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->GetMesh())
	{
		return;
	}
	
	HoldMeshComponent = NewObject<UStaticMeshComponent>(Character);
	if (!HoldMeshComponent)
	{
		return;
	}
	
	// 메쉬 컴포넌트 설정하고 캐릭터의 Weapon_l 소켓에 부착
	HoldMeshComponent->SetStaticMesh(HoldStaticMesh);
	HoldMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HoldMeshComponent->SetGenerateOverlapEvents(false);
	HoldMeshComponent->RegisterComponent();
	HoldMeshComponent->AttachToComponent(
		Character->GetMesh(),
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		HoldAttachSocketName
	);
	HoldMeshComponent->SetRelativeTransform(HoldRelativeTransform);
}

void UGA_ThrowProjectile::DestroyHeldMesh()
{
	if (!HoldMeshComponent)
	{
		return;
	}
	
	// 부착해두었던 MeshComponent 제거
	HoldMeshComponent->DestroyComponent();
	HoldMeshComponent = nullptr;
}

void UGA_ThrowProjectile::SpawnProjectileAtAimPoint(const FVector& AimPoint)
{
	const FTransform SpawnTransform = GetProjectileSpawnTransform();
	FVector ThrowDirection = (AimPoint - SpawnTransform.GetLocation()).GetSafeNormal();

	if (ThrowDirection.IsNearlyZero())
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = GetWorld();

	if (!AvatarActor || !AvatarActor->HasAuthority() || !World ||
		!ProjectileAbilityConfig.ProjectileClass || ThrowDirection.IsNearlyZero())
	{
		return;
	}

	APawn* OwningPawn = Cast<APawn>(AvatarActor);
	AController* OwningController = OwningPawn ? OwningPawn->GetController() : nullptr;

	ANSThrowProjectileBase* Projectile = World->SpawnActor<ANSThrowProjectileBase>(
		ProjectileAbilityConfig.ProjectileClass,
		SpawnTransform
	);

	if (Projectile)
	{
		// 계산된 payload를 투척물에 전달
		Projectile->SetSetByCallerMagnitudes(SetByCallerMagnitudes);
		Projectile->SetRuntimeStatMagnitudes(RuntimeStatMagnitudes);
		Projectile->InitializeThrowActor(OwningPawn, OwningController, ThrowDirection);

		// 공통 ThrowProjectileBase 스폰 이후 타입별 추가 설정을 주입
		if (ProjectileAbilityConfig.ProjectileType == EProjectileType::Explosive)
		{
			if (ANSGrenade* Grenade = Cast<ANSGrenade>(Projectile))
			{
				Grenade->InitializeGrenade(ProjectileAbilityConfig.ExplosiveTypeConfig);
			}
		}
		else if (ProjectileAbilityConfig.ProjectileType == EProjectileType::TurretSpawner)
		{
			if (ANSTurretSpawner* TurretSpawner = Cast<ANSTurretSpawner>(Projectile))
			{
				TurretSpawner->InitializeTurretSpawner(ProjectileAbilityConfig.TurretSpawnerTypeConfig);
			}
		}
		else if (ProjectileAbilityConfig.ProjectileType == EProjectileType::BarrierField)
		{
			if (ANSVanguardBarrierFieldProjectile* BarrierFieldProjectile =
				Cast<ANSVanguardBarrierFieldProjectile>(Projectile))
			{
				BarrierFieldProjectile->InitializeBarrierFieldProjectile(
					ProjectileAbilityConfig.ShieldFieldTypeConfig
				);
			}
		}
	}
}

FTransform UGA_ThrowProjectile::GetProjectileSpawnTransform() const
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character && Character->GetMesh() && !ProjectileSpawnSocketName.IsNone())
	{
		return Character->GetMesh()->GetSocketTransform(ProjectileSpawnSocketName, RTS_World);
	}

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	return AvatarActor ? AvatarActor->GetActorTransform() : FTransform::Identity;
}

bool UGA_ThrowProjectile::TryBuildProjectileAimTrace(FHitResult& OutHitResult) const
{
	UWorld* World = GetWorld();
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const APawn* Pawn = Cast<APawn>(AvatarActor);
	const APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;

	if (!World || !IsValid(AvatarActor) || !IsValid(PlayerController))
	{
		return false;
	}

	FVector TraceStart;
	const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(AvatarActor);
	if (!PlayerCharacter || !PlayerCharacter->TryGetAimTraceStartLocation(TraceStart))
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
		TraceStart = ViewLocation;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);

	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		return false;
	}

	const float CrosshairScreenX = ViewportSizeX * 0.5f;
	const float CrosshairScreenY = ViewportSizeY * 0.5f;

	FVector DeprojectWorldLocation;
	FVector DeprojectWorldDirection;

	if (!PlayerController->DeprojectScreenPositionToWorld(
		CrosshairScreenX,
		CrosshairScreenY,
		DeprojectWorldLocation,
		DeprojectWorldDirection))
	{
		return false;
	}

	const FVector TraceDirection = DeprojectWorldDirection.GetSafeNormal();
	if (TraceDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector TraceEnd = TraceStart + TraceDirection * AimTraceRange;

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(ThrowProjectileAimTrace),
		false,
		AvatarActor
	);

	const bool bHit = World->LineTraceSingleByChannel(
		OutHitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	OutHitResult.TraceStart = TraceStart;
	OutHitResult.TraceEnd = TraceEnd;

	if (!bHit)
	{
		OutHitResult.Location = TraceEnd;
		OutHitResult.ImpactPoint = TraceEnd;
	}

	return true;
}

FGameplayAbilityTargetDataHandle UGA_ThrowProjectile::MakeTargetDataFromHitResult(const FHitResult& HitResult) const
{
	FGameplayAbilityTargetDataHandle TargetDataHandle;

	FGameplayAbilityTargetData_SingleTargetHit* TargetData =
		new FGameplayAbilityTargetData_SingleTargetHit();

	TargetData->HitResult = HitResult;
	TargetDataHandle.Add(TargetData);

	return TargetDataHandle;
}

void UGA_ThrowProjectile::OnTargetDataReadyCallback(
	const FGameplayAbilityTargetDataHandle& TargetDataHandle,
	FGameplayTag ApplicationTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	FScopedPredictionWindow ScopedPredictionWindow(ASC);

	FGameplayAbilityTargetDataHandle LocalTargetDataHandle = TargetDataHandle;
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const bool bShouldNotifyServer =
		ActorInfo && ActorInfo->IsLocallyControlled() && !ActorInfo->IsNetAuthority();

	if (bShouldNotifyServer)
	{
		ASC->CallServerSetReplicatedTargetData(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActivationInfo().GetActivationPredictionKey(),
			LocalTargetDataHandle,
			ApplicationTag,
			ASC->ScopedPredictionKey
		);
	}

	OnThrowProjectileTargetDataReady(LocalTargetDataHandle);
}

void UGA_ThrowProjectile::OnThrowProjectileTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor || !AvatarActor->HasAuthority())
	{
		return;
	}
	
	// 
	FVector AimPoint;
	if (!TryGetAimPointFromTargetData(TargetDataHandle, AimPoint))
	{
		return;
	}

	if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
		ActorInfo && ActorInfo->IsNetAuthority() && !ActorInfo->IsLocallyControlled() && !bReleaseRequested)
	{
		bReleaseRequested = true;
		if (!TryJumpToReleaseSection())
		{
			FinishThrowProjectileAbility(true);
			return;
		}
	}

	SpawnProjectileAtAimPoint(AimPoint);
}

bool UGA_ThrowProjectile::TryGetAimPointFromTargetData(
	const FGameplayAbilityTargetDataHandle& TargetDataHandle,
	FVector& OutAimPoint) const
{
	if (TargetDataHandle.Num() <= 0)
	{
		return false;
	}

	const FGameplayAbilityTargetData* TargetData = TargetDataHandle.Get(0);
	if (!TargetData)
	{
		return false;
	}

	const FHitResult* HitResult = TargetData->GetHitResult();
	if (!HitResult)
	{
		return false;
	}

	OutAimPoint = HitResult->bBlockingHit ? HitResult->ImpactPoint : HitResult->TraceEnd;
	return true;
}

void UGA_ThrowProjectile::RebuildCombatStatPayloads()
{
	// 이전 활성화에서 만든 payload 초기화
	SetByCallerMagnitudes.Reset();
	RuntimeStatMagnitudes.Reset();

	if (ProjectileAbilityConfig.SetByCallerMappings.IsEmpty() &&
		ProjectileAbilityConfig.RuntimeStatMappings.IsEmpty())
	{
		return;
	}

	if (!SkillAbilityTag.IsValid())
	{
		return;
	}

	// 매핑 설정에 따라 SetByCaller와 runtime payload 분리 생성
	RebuildSetByCallerMagnitudes(SkillAbilityTag);
	RebuildRuntimeStatMagnitudes(SkillAbilityTag);
}

void UGA_ThrowProjectile::RebuildSetByCallerMagnitudes(const FGameplayTag& AbilityTag)
{
	// GE SetByCaller로 전달할 값 계산
	for (const FNSSetByCallerFromCombatStat& Mapping : ProjectileAbilityConfig.SetByCallerMappings)
	{
		if (!Mapping.CombatStatTag.IsValid() || !Mapping.SetByCallerTag.IsValid())
		{
			continue;
		}

		float Magnitude = 0.0f;

		// DamageCoefficient는 일반 CombatStat 조회가 아니라 PlayerBaseDamage x 계수로 계산
		const bool bIsDamageCoefficient = Mapping.CombatStatTag == NSGameplayTags::CombatStat_DamageCoefficient;
		const bool bMagnitudeResolved = bIsDamageCoefficient
			? TryGetFinalSkillDamage(AbilityTag, Magnitude)
			: TryGetFinalAbilityStat(AbilityTag, Mapping.CombatStatTag, Magnitude);

		if (!bMagnitudeResolved)
		{
			continue;
		}

		FNSSetByCallerMagnitude SetByCallerMagnitude;
		SetByCallerMagnitude.SetByCallerTag = Mapping.SetByCallerTag;
		SetByCallerMagnitude.Magnitude = Magnitude;
		SetByCallerMagnitudes.Add(SetByCallerMagnitude);
	}
}

void UGA_ThrowProjectile::RebuildRuntimeStatMagnitudes(const FGameplayTag& AbilityTag)
{
	// 투척물 로직에서 직접 사용할 값 계산
	for (const FNSRuntimeStatFromCombatStat& Mapping : ProjectileAbilityConfig.RuntimeStatMappings)
	{
		if (!Mapping.CombatStatTag.IsValid())
		{
			continue;
		}

		float Magnitude = 0.0f;
		if (!TryGetFinalAbilityStat(AbilityTag, Mapping.CombatStatTag, Magnitude))
		{
			continue;
		}

		FNSCombatStatMagnitude RuntimeStatMagnitude;
		RuntimeStatMagnitude.CombatStatTag = Mapping.CombatStatTag;
		RuntimeStatMagnitude.Magnitude = Magnitude;
		RuntimeStatMagnitudes.Add(RuntimeStatMagnitude);
	}
}

void UGA_ThrowProjectile::AddDeactivateHandIKTag()
{
	if (bDeactivateHandIKTagAdded)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(NSGameplayTags::State_Deactivate_HandIK);
		bDeactivateHandIKTagAdded = true;
	}
}

void UGA_ThrowProjectile::RemoveDeactivateHandIKTag()
{
	if (!bDeactivateHandIKTagAdded)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(NSGameplayTags::State_Deactivate_HandIK);
	}

	bDeactivateHandIKTagAdded = false;
}
