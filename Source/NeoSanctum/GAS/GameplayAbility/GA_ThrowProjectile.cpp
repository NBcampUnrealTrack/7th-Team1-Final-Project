// Copyright 2026 One Team. All rights reserved.


#include "GA_ThrowProjectile.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Combat/Projectile/NSThrowProjectileBase.h"
#include "NeoSanctum/Combat/Projectile/NSTurretSpawner.h"
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

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
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

	OnTargetDataReadyCallbackDelegateHandle = ASC->AbilityTargetDataSetDelegate(
		Handle,
		ActivationInfo.GetActivationPredictionKey()
	).AddUObject(this, &ThisClass::OnTargetDataReadyCallback);

	if (ActorInfo->IsNetAuthority() && !ActorInfo->IsLocallyControlled())
	{
		ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, ActivationInfo.GetActivationPredictionKey());
	}
	
	if (!AnimMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
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

	if (ActorInfo && ActorInfo->IsLocallyControlled())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			
			ASC->CurrentMontageJumpToSection(ReleaseSectionName);
		}
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
	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		false
	);
}

void UGA_ThrowProjectile::OnThrowMontageInterrupted()
{
	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		true
	);
}

void UGA_ThrowProjectile::OnAttachProjectileEventReceived(FGameplayEventData Payload)
{
	AttachHeldMesh();
}

void UGA_ThrowProjectile::OnThrowProjectileEventReceived(FGameplayEventData Payload)
{
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

	if (!AvatarActor || !AvatarActor->HasAuthority() || !World || !ProjectileClass || ThrowDirection.IsNearlyZero())
	{
		return;
	}

	APawn* OwningPawn = Cast<APawn>(AvatarActor);
	AController* OwningController = OwningPawn ? OwningPawn->GetController() : nullptr;

	ANSThrowProjectileBase* Projectile = World->SpawnActor<ANSThrowProjectileBase>(
		ProjectileClass,
		SpawnTransform
	);

	if (Projectile)
	{
		Projectile->InitializeThrowActor(OwningPawn, OwningController, ThrowDirection);

		if (ProjectileAbilityConfig.ProjectileType == EProjectileType::TurretSpawner)
		{
			if (ANSTurretSpawner* TurretSpawner = Cast<ANSTurretSpawner>(Projectile))
			{
				TurretSpawner->InitializeTurretSpawner(ProjectileAbilityConfig.TurretSpawnerTypeConfig);
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
