// Copyright 2026 One Team. All rights reserved.


#include "NSDamageNumberFeedbackComponent.h"

#include "NSHitFeedbackTypes.h"
#include "NeoSanctum/Tag/NSGameplayTags_Message.h"
#include "NeoSanctum/UI/DamageNumber/NSDamageNumberActor.h"


UNSDamageNumberFeedbackComponent::UNSDamageNumberFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSDamageNumberFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();

	// 로컬 판정은 메시지를 받을 때 다시 확인. BeginPlay 때는 컨트롤러 연결이 아직 덜 끝날을 수 있음.
	DamageNumberListenerHandle = UGameplayMessageSubsystem::Get(this).
		RegisterListener<FNSDamageNumberFeedbackMessage>(
		NSGameplayTags::Message_UI_DamageNumber,
		this,
		&ThisClass::HandleDamageNumberMessage
	);
}

void UNSDamageNumberFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DamageNumberListenerHandle.Unregister();

	Super::EndPlay(EndPlayReason);
}

void UNSDamageNumberFeedbackComponent::HandleDamageNumberMessage(
	FGameplayTag ChannelTag, const FNSDamageNumberFeedbackMessage& Message)
{
	// 이 컴포넌트가 붙은 로컬 플레이어에게만 데미지 숫자를 띄움.
	if (!ShouldPlayLocalFeedback() || !DamageNumberActorClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ANSDamageNumberActor* DamageNumberActor = World->SpawnActor<ANSDamageNumberActor>(
		DamageNumberActorClass,
		Message.Context.WorldLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (DamageNumberActor)
	{
		DamageNumberActor->InitializeDamageNumber(Message.Context);
	}
}

bool UNSDamageNumberFeedbackComponent::ShouldPlayLocalFeedback() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}

