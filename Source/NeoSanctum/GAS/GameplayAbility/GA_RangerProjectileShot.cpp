// Copyright 2026 One Team. All rights reserved.


#include "GA_RangerProjectileShot.h"

#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Combat/Projectile/NSRangerProjectile.h"
#include "NeoSanctum/Combat/Weapon/NSWeaponBase.h"
#include "NeoSanctum/Debug/Logging/NSLogMacros.h"
#include "NeoSanctum/Tag/NSGameplayTags_Ability.h"

UGA_RangerProjectileShot::UGA_RangerProjectileShot()
{
	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(NSGameplayTags::Ability_Ranger_ProjectileShot);
	SetAssetTags(AssetTags);

	ActivationPolicy = ENSAbilityActivationPolicy::OnInputTriggered;
}

void UGA_RangerProjectileShot::ActivateAbility(
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

	// 서버가 원격 클라이언트의 TargetData를 받을 수 있도록 먼저 등록
	OnTargetDataReadyCallbackDelegateHandle = ASC->AbilityTargetDataSetDelegate(
		Handle,
		ActivationInfo.GetActivationPredictionKey()
	).AddUObject(this, &ThisClass::OnTargetDataReadyCallback);

	const bool bShouldWaitForClientTargetData = ActorInfo->IsNetAuthority() && !ActorInfo->IsLocallyControlled();

	if (bShouldWaitForClientTargetData)
	{
		// TargetData가 델리게이트 등록보다 먼저 도착한 경우 바로 처리
		ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, ActivationInfo.GetActivationPredictionKey());
		return;
	}

	// 호스트 또는 로컬 클라이언트는 직접 조준 TargetData를 만듬
	FireProjectileShot();
}

void UGA_RangerProjectileShot::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DebugEndAbilityTimerHandle);
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

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_RangerProjectileShot::FireProjectileShot()
{
	FHitResult HitResult;

	if (!TryBuildProjectileAimTrace(HitResult))
	{
		EndAbility(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActorInfo(),
			GetCurrentActivationInfo(),
			true,
			true);
		
		return;
	}
	
	const FGameplayAbilityTargetDataHandle TargetDataHandle = MakeTargetDataFromHitResult(HitResult);
	
	OnTargetDataReadyCallback(TargetDataHandle, FGameplayTag());
}

FGameplayAbilityTargetDataHandle UGA_RangerProjectileShot::MakeTargetDataFromHitResult(
	const FHitResult& HitResult) const
{
	FGameplayAbilityTargetDataHandle TargetDataHandle;

	FGameplayAbilityTargetData_SingleTargetHit* TargetData =
		new FGameplayAbilityTargetData_SingleTargetHit();

	TargetData->HitResult = HitResult;
	TargetDataHandle.Add(TargetData);

	return TargetDataHandle;
}

void UGA_RangerProjectileShot::OnTargetDataReadyCallback(
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
	
	// 서버에게만 로컬 데이터 전송
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
	
	OnProjectileTargetDataReady(LocalTargetDataHandle);
}

void UGA_RangerProjectileShot::OnProjectileTargetDataReady(
	const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	
	// 로컬 조작자 화면에 예측 발사 방향 표시
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	
	if (bDrawDebugProjectileLaunch && ActorInfo && ActorInfo->IsLocallyControlled())
	{
		FVector AimPoint;

		if (TryGetAimPointFromTargetData(TargetDataHandle, AimPoint))
		{
			FTransform MuzzleTransform;

			if (TryGetAttackOriginTransform(MuzzleTransform))
			{
				// Cyan: 로컬 조작자가 계산한 예측 발사 방향
				DrawDebugProjectileLaunch(
					MuzzleTransform.GetLocation(),
					AimPoint,
					FColor::Cyan
				);
			}
		}
	}
	
	if (AvatarActor && AvatarActor->HasAuthority())
	{
		TrySpawnProjectileFromTargetData(TargetDataHandle);
	}
	
	if (bKeepAbilityActiveForDebug)
	{
		if (UWorld* World = GetWorld())
		{
			const float Duration = FMath::Max(DebugActiveDuration, 0.1f);
			
			// GameplayDebugger에서 Active 상태를 확인하는 타이머
			World->GetTimerManager().SetTimer(
				DebugEndAbilityTimerHandle,
				this,
				&ThisClass::FinishDebugAbility,
				Duration,
				false
			);
			
			return;
		}
	}
	
	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		false
	);
}

bool UGA_RangerProjectileShot::TryBuildProjectileAimTrace(FHitResult& OutHitResult) const
{
	UWorld* World = GetWorld();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(AvatarActor);
	
	if (!World || !IsValid(PlayerCharacter))
	{
		return false;
	}
	
	const APawn* Pawn = Cast<APawn>(AvatarActor);
	const APlayerController* PlayerController = Pawn ?
		Cast<APlayerController>(Pawn->GetController()) : nullptr;
	
	if (!IsValid(PlayerController))
	{
		return false;
	}
	
	FVector TraceStart;
	
	if (!PlayerCharacter->TryGetAimTraceStartLocation(TraceStart))
	{
		return false;
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
	
	const FVector TraceEnd = TraceStart + TraceDirection * TraceRange;
	
	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(RangerProjectileAimTrace),
		false,
		PlayerCharacter
	);
	
	const ANSWeaponBase* CurrentWeapon = PlayerCharacter->GetCurrentWeapon();
	
	if (IsValid(CurrentWeapon))
	{
		QueryParams.AddIgnoredActor(CurrentWeapon);
	}
	
	const bool bHit = World->LineTraceSingleByChannel(
		OutHitResult,
		TraceStart,
		TraceEnd,
		TraceChannel,
		QueryParams
	);
	
	OutHitResult.TraceStart = TraceStart;
	OutHitResult.TraceEnd = TraceEnd;
	
	// Miss도 서버 스폰 방향 계산에 사용하기 위해 TraceEnd를 AimPoint로 채움
	if (!bHit)
	{
		OutHitResult.Location = TraceEnd;
		OutHitResult.ImpactPoint = TraceEnd;
	}
	
	// 로컬 조준 Trace 표시
	if (bDrawDebugProjectileAimTrace)
	{
		DrawDebugProjectileAimTrace(TraceStart, TraceEnd, OutHitResult, bHit);
	}
	
	return true;
}

bool UGA_RangerProjectileShot::TrySpawnProjectileFromTargetData(
	const FGameplayAbilityTargetDataHandle& TargetDataHandle) const
{
	FVector AimPoint;
	
	if (!TryGetAimPointFromTargetData(TargetDataHandle, AimPoint))
	{
		return false;
	}
	
	return TrySpawnProjectileAtAimPoint(AimPoint);
}

bool UGA_RangerProjectileShot::TrySpawnProjectileAtAimPoint(const FVector& AimPoint) const
{
	UWorld* World = GetWorld();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	
	if (!World || !IsValid(AvatarActor))
	{
		return false;
	}
	
	if (!ProjectileClass)
	{
		NS_ACTOR_LOG(AvatarActor, LogNSGAS, Warning, "ProjectileShot 실패. ProjectileClass가 설정되지 않음");
		return false;
	}
	
	FTransform MuzzleTransform;
	
	if (!TryGetAttackOriginTransform(MuzzleTransform))
	{
		// 무기 소켓을 찾지 못하면 캐릭터 전방으로 임시 발사
		MuzzleTransform = FTransform(
			AvatarActor->GetActorRotation(),
			AvatarActor->GetActorLocation() + AvatarActor->GetActorForwardVector() * 100.0f
		);
	}
	
	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	const FVector LaunchDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
	
	if (LaunchDirection.IsNearlyZero())
	{
		NS_ACTOR_LOG(AvatarActor, LogNSGAS, Warning, "ProjectileShot 실패. 발사 방향을 계산할 수 없음");
		return false;
	}
	
	// 서버가 처리한 원격 클라이언트 발사 방향 표시
	const APawn* Pawn = Cast<APawn>(AvatarActor);
	const bool bIsLocallyControlled = Pawn && Pawn->IsLocallyControlled();

	if (bDrawDebugProjectileLaunch && !bIsLocallyControlled)
	{
		// Orange: 서버가 원격 클라이언트 발사를 처리한 실제 발사 방향
		DrawDebugProjectileLaunch(MuzzleLocation, AimPoint, FColor::Orange);
	}
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = AvatarActor;
	SpawnParameters.Instigator = Cast<APawn>(AvatarActor);
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	ANSRangerProjectile* Projectile = World->SpawnActor<ANSRangerProjectile>(
		ProjectileClass,
		MuzzleLocation,
		LaunchDirection.Rotation(),
		SpawnParameters
	);
	
	if (!IsValid(Projectile))
	{
		return false;
	}
	
	Projectile->LaunchProjectile(LaunchDirection);
	return true;
}

bool UGA_RangerProjectileShot::TryGetAttackOriginTransform(FTransform& OutTransform) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(AvatarActor);

	if (!IsValid(PlayerCharacter))
	{
		return false;
	}

	const ANSWeaponBase* CurrentWeapon = PlayerCharacter->GetCurrentWeapon();

	if (!IsValid(CurrentWeapon))
	{
		return false;
	}

	return CurrentWeapon->TryGetAttackOriginTransform(OutTransform);
}

bool UGA_RangerProjectileShot::TryGetAimPointFromTargetData(
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

	// Hit가 없으면 TraceEnd를 조준점으로 사용
	OutAimPoint = HitResult->bBlockingHit ? HitResult->ImpactPoint : HitResult->TraceEnd;
	return true;
}

void UGA_RangerProjectileShot::DrawDebugProjectileAimTrace(
	const FVector& TraceStart,
	const FVector& TraceEnd,
	const FHitResult& HitResult,
	bool bHit) const
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	const FVector DebugEnd = bHit ? HitResult.ImpactPoint : TraceEnd;
	const FColor DebugColor = bHit ? FColor::Red : FColor::Green;
	const FVector DebugDirection = (DebugEnd - TraceStart).GetSafeNormal();

	if (DebugDirection.IsNearlyZero())
	{
		return;
	}
	
	const float DebugDistance = static_cast<float>(FVector::Dist(TraceStart, DebugEnd));
	const float AppliedOffset =
		FMath::Clamp(DebugAimTraceStartOffset, 0.0f, FMath::Max(DebugDistance - 10.0f, 0.0f));

	const FVector DebugStart = TraceStart + DebugDirection * AppliedOffset;
	
	// 초록: 허공 조준, 빨강: 조준 Trace 명중
	DrawDebugLine(
		World,
		DebugStart,
		DebugEnd,
		DebugColor,
		false,
		DebugLineDuration,
		0,
		DebugLineThickness
	);

	DrawDebugPoint(
		World,
		DebugEnd,
		DebugPointSize,
		DebugColor,
		false,
		DebugLineDuration
	);
}

void UGA_RangerProjectileShot::DrawDebugProjectileLaunch(
	const FVector& MuzzleLocation,
	const FVector& AimPoint,
	const FColor& DebugColor) const
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	const FVector LaunchDirection = (AimPoint - MuzzleLocation).GetSafeNormal();

	if (LaunchDirection.IsNearlyZero())
	{
		return;
	}

	const FVector DebugEnd = MuzzleLocation + LaunchDirection * 1000.0f;

	// 총구에서 실제 발사 방향으로 짧게 표시
	DrawDebugLine(
		World,
		MuzzleLocation,
		DebugEnd,
		DebugColor,
		false,
		DebugLineDuration,
		0,
		DebugLineThickness
	);

	DrawDebugPoint(
		World,
		MuzzleLocation,
		DebugPointSize,
		DebugColor,
		false,
		DebugLineDuration
	);
}

void UGA_RangerProjectileShot::FinishDebugAbility()
{
	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		false
	);
}
