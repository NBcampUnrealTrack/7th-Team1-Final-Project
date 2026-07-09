// Copyright 2026 One Team. All rights reserved.


#include "NSDamageNumberActor.h"

#include "NSDamageNumberWidget.h"
#include "Components/WidgetComponent.h"
#include "NeoSanctum/Combat/HitReaction/NSHitFeedbackTypes.h"


ANSDamageNumberActor::ANSDamageNumberActor()
{
	// 데미지 숫자는 각 클라이언트 화면에서만 보이는 로컬 표시용 Actor.
	bReplicates = false;
	SetReplicateMovement(false);

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetupAttachment(RootComponent);
	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComponent->SetDrawSize(FVector2D(160.0f, 80.0f));
}

void ANSDamageNumberActor::InitializeDamageNumber(const FNSDamageNumberFeedbackContext& Context)
{
	SetActorLocation(Context.WorldLocation);
	SetLifeSpan(LifeTime);

	// 위젯이 아직 안 물려 있으면 이번 숫자는 조용히 넘김.
	UNSDamageNumberWidget* DamageNumberWidget =
		Cast<UNSDamageNumberWidget>(WidgetComponent->GetUserWidgetObject());
	if (!DamageNumberWidget)
	{
		return;
	}

	DamageNumberWidget->SetDamageNumber(Context);
}
