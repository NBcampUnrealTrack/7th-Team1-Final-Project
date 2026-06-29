// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NSInteractableNPCBase.h"
#include "NSRescueNPC.generated.h"

UCLASS()
class NEOSANCTUM_API ANSRescueNPC : public ANSInteractableNPCBase
{
	GENERATED_BODY()

public:
	virtual bool CanInteract_Implementation(APlayerController* Interactor) const override;
	virtual bool OnInteract_Implementation(APlayerController* Interactor) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// 이 NPC가 해금시킬 대상 ID, 같은 거점 NPC의 NPCId와 동일하게 지정해야함
	UPROPERTY(EditAnywhere, Category = "Rescue")
	FName RescueNPCId;

	// 이미 구출한 적 있는 플레이어에게 줄 보상 재화량
	UPROPERTY(EditAnywhere, Category = "Rescue", meta = (ClampMin = "0"))
	int64 AlreadyRescuedReward = 0;

	// 이번 런에서 이미 구출 처리됐는지
	UPROPERTY(Replicated)
	bool bRescued = false;
};
