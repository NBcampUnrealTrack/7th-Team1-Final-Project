// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/Core/Interface/NSOutGameModeInterface.h"
#include "NSOutGameMode.generated.h"



UCLASS()
class NEOSANCTUM_API ANSOutGameMode : public AGameModeBase, public INSOutGameInterface
{
	GENERATED_BODY()
	
public:
	ANSOutGameMode();
	
	// 런 레벨 이동용 함수
	virtual void RequestStartRun_Implementation() override;
	
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

protected:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
private:
	// 사용 중이지 않은 가장 작은 슬롯 인덱스를 반환 (자기 자신 제외)
	int32 FindFreeSlotIndex(const APlayerState* Requester) const;
};
