// Copyright 2026 One Team. All rights reserved.


#include "NSGeneratedNavLinksProxy.h"

#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

bool UNSGeneratedNavLinksProxy::OnLinkMoveStarted(UObject* PathComp, const FVector& DestPoint)
{
	// 생성 링크는 Agent 대신 Agent의 PathFollowingComponent를 전달한다.
	UPathFollowingComponent* PathFollowingComponent = Cast<UPathFollowingComponent>(PathComp);

	if (!PathComp)
	{
		return false;
	}

	AActor* Agent = PathFollowingComponent->GetOwner();

	if (AController* Controller = Cast<AController>(Agent))
	{
		Agent = Controller->GetPawn();
	}

	ACharacter* Character = Cast<ACharacter>(Agent);

	if (!Character)
	{
		// 캐릭터가 아닌경우 점프 X
		return false;
	}

	FVector LaunchVelocity = FVector::ZeroVector;

	const bool bFoundVelocity =
		UGameplayStatics::SuggestProjectileVelocity_CustomArc(
			Character,                     // World Context Object
			LaunchVelocity,             // Out Launch Velocity
			Character->GetActorLocation(), // Start Pos
			DestPoint,                     // End Pos
			0.0f,                          // 기본 World Gravity 사용
			0.5f);                 // 기본 중간 높이 Arc

	if (!bFoundVelocity)
	{
		return false;
	}

	LaunchVelocity.Z += 200.0f;

	Character->LaunchCharacter(
		LaunchVelocity,
		true,           // XY Override
		true); // Z Override

	// 점프 도중 PathFollowing이 이동 속도를 덮어쓰지 않는다.
	return true;
}
