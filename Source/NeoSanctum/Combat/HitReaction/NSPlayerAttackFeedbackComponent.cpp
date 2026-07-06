// Copyright 2026 One Team. All rights reserved.

#include "NSPlayerAttackFeedbackComponent.h"

#include "Engine/DataTable.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Combat/Weapon/Summon/NSBarrierBase.h"
#include "NeoSanctum/Combat/Weapon/Summon/NSTurret.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSSoundSubsystem.h"
#include "NeoSanctum/Data/Combat/NSPlayerAttackFeedbackData.h"
#include "NeoSanctum/Interaction/Prop/NSDestructibleObjectBase.h"
#include "NeoSanctum/Tag/NSGameplayTags_Message.h"

UNSPlayerAttackFeedbackComponent::UNSPlayerAttackFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSPlayerAttackFeedbackComponent::HandleAttackHitFeedback(const FNSHitFeedbackContext& Context)
{
	// 실제 히트 결과를 UI 피드백 규칙에 맞게 해석
	const FNSHitFeedbackContext ResolvedContext = BuildResolvedContext(Context);
	const FNSPlayerAttackFeedbackData* FeedbackData = FindBestFeedbackData(ResolvedContext);
	if (!FeedbackData)
	{
		return;
	}
	
	if (FeedbackData->CrosshairFeedbackType != ENSCrosshairAttackFeedbackType::None)
	{
		// 크로스헤어 피드백 재생
		PlayCrosshairFeedback(FeedbackData->CrosshairFeedbackType, ResolvedContext);
	}
	
	// 공격자 로컬 히트 확인 사운드 재생
	PlaySoundFeedback(FeedbackData->SoundID);
}

FNSHitFeedbackContext UNSPlayerAttackFeedbackComponent::BuildResolvedContext(
	const FNSHitFeedbackContext& Context) const
{
	// AttributeSet에서 넘어온 원본 Context에 피드백용 분류를 채움
	FNSHitFeedbackContext ResolvedContext = Context;
	ResolvedContext.TargetType = ResolveTargetType(Context.TargetActor);
	ResolvedContext.Outcome = ResolveOutcome(Context);
	return ResolvedContext;
}

ENSHitFeedbackTargetType UNSPlayerAttackFeedbackComponent::ResolveTargetType(const AActor* TargetActor) const
{
	// 피격 액터 타입을 DataTable 매칭용 TargetType으로 변환
	if (!TargetActor)
	{
		return ENSHitFeedbackTargetType::Any;
	}
	
	// 타겟이 Enemy
	if (TargetActor->IsA<ANSEnemyCharacterBase>())
	{
		return ENSHitFeedbackTargetType::Enemy;
	}
	
	// 타겟이 Barrier
	if (TargetActor->IsA<ANSBarrierBase>())
	{
		return ENSHitFeedbackTargetType::Barrier;
	}
	
	// 타겟이 Object
	if (TargetActor->IsA<ANSDestructibleObjectBase>())
	{
		return ENSHitFeedbackTargetType::DestructibleObject;
	}
	
	// 타겟이 Turret
	if (TargetActor->IsA<ANSTurret>())
	{
		return ENSHitFeedbackTargetType::Turret;
	}
	
	// 타겟이 무엇이든 상관없는 경우
	return ENSHitFeedbackTargetType::Any;
}

ENSHitFeedbackOutcome UNSPlayerAttackFeedbackComponent::ResolveOutcome(
	const FNSHitFeedbackContext& Context) const
{
	// Health가 고갈된 대상만 Kill/Destroy 피드백
	if (!Context.bTargetDead)
	{
		return ENSHitFeedbackOutcome::None;
	}
	
	const ENSHitFeedbackTargetType TargetType = ResolveTargetType(Context.TargetActor);
	if (TargetType == ENSHitFeedbackTargetType::Enemy)
	{
		return ENSHitFeedbackOutcome::Kill;
	}
	
	if (TargetType == ENSHitFeedbackTargetType::Barrier ||
		TargetType == ENSHitFeedbackTargetType::DestructibleObject ||
		TargetType == ENSHitFeedbackTargetType::Turret)
	{
		return ENSHitFeedbackOutcome::Destroy;
	}
	
	return ENSHitFeedbackOutcome::None;
}

const FNSPlayerAttackFeedbackData* UNSPlayerAttackFeedbackComponent::FindBestFeedbackData(
	const FNSHitFeedbackContext& Context) const
{
	const UNSDataSubsystem* DataSubsystem = UNSDataSubsystem::Get(this);
	if (!DataSubsystem)
	{
		return nullptr;
	}
	
	const TArray<FNSPlayerAttackFeedbackData>& Rows = DataSubsystem->GetCachedPlayerAttackFeedbackRows();
	const FNSPlayerAttackFeedbackData* BestData = nullptr;
	int32 BestPriority = TNumericLimits<int32>::Lowest();
	
	for (const FNSPlayerAttackFeedbackData& Row : Rows)
	{
		if (CheckFeedbackDataMatch(Row, Context) && (!BestData || Row.Priority > BestPriority))
		{
			BestData = &Row;
			BestPriority = Row.Priority;
		}
	}
	
	return BestData;
}

bool UNSPlayerAttackFeedbackComponent::CheckFeedbackDataMatch(
	const FNSPlayerAttackFeedbackData& Data,
	const FNSHitFeedbackContext& Context) const
{
	const bool bTargetMatched =
		Data.TargetType == ENSHitFeedbackTargetType::Any ||
		Data.TargetType == Context.TargetType;

	const bool bQualityMatched =
		Data.HitQuality == ENSHitFeedbackQuality::Any ||
		Data.HitQuality == Context.HitQuality;

	const bool bOutcomeMatched =
		Data.Outcome == ENSHitFeedbackOutcome::Any ||
		Data.Outcome == Context.Outcome;

	return bTargetMatched && bQualityMatched && bOutcomeMatched;
}

void UNSPlayerAttackFeedbackComponent::PlayCrosshairFeedback(
	const ENSCrosshairAttackFeedbackType FeedbackType,
	const FNSHitFeedbackContext& Context) const
{
	// 로컬 플레이어에게만 크로스헤어 피드백 메시지를 전달
	const AActor* OwnerActor = GetOwner();
	const APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		return;
	}

	FNSCrosshairAttackFeedbackMessage Message;
	Message.FeedbackType = FeedbackType;
	Message.Context = Context;

	UGameplayMessageSubsystem::Get(OwnerActor).BroadcastMessage(
		NSGameplayTags::Message_UI_Crosshair_AttackFeedback,
		Message);
}

void UNSPlayerAttackFeedbackComponent::PlaySoundFeedback(const FName SoundID) const
{
	if (SoundID.IsNone())
	{
		return;
	}
	
	const AActor* OwnerActor = GetOwner();
	const APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	
	// 반드시 로컬에서만 재생되는 Feedback Sound임
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		return;
	}
	
	if (UNSSoundSubsystem* SoundSubsystem = UNSSoundSubsystem::Get(OwnerActor))
	{
		SoundSubsystem->PlaySound2D(SoundID);
	}
}
