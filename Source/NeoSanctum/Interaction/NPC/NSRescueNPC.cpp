// Copyright 2026 One Team. All rights reserved.


#include "NSRescueNPC.h"
#include "Net/UnrealNetwork.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerProgressComponent.h"
#include "NeoSanctum/Progression/Currency/NSCurrencyComponent.h"
#include "NeoSanctum/Tag/NSGameplayTags_Currency.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"

void ANSRescueNPC::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANSRescueNPC, bRescued);
}

bool ANSRescueNPC::CanInteract_Implementation(APlayerController* Interactor) const
{
	// 상호작용, 대사는 항상 가능
	return Interactor != nullptr;
}

bool ANSRescueNPC::OnInteract_Implementation(APlayerController* Interactor)
{
	// 1회성 게이트
	if (!HasAuthority())
	{
		return false;
	}
	
	// 스테이즈 목표 달성 충족, 최초 구출 시에만 작동됨
	if (AGameModeBase* GameMode = GetWorld()->GetAuthGameMode())
	{
		if (GameMode->Implements<UNSRunGameModeInterface>())
		{
			INSRunGameModeInterface::Execute_NotifyNPCRescued(GameMode, NPCId);
		}
	}
	
	// 이미 처리된 NPC면 상호작용은 받되 해금/보상은 다시 안 함
	if (bRescued)
	{
		return true;
	}
	
	ANSRunGameState* RunGameState =
		GetWorld() ? GetWorld()->GetGameState<ANSRunGameState>() : nullptr;
	if (!RunGameState)
	{
		return false;
	}
	
	// 중복 진입 차단
	bRescued = true; 
	ForceNetUpdate();

	// 전체 공유: 접속 전원 순회하며 각자 본인 기준으로 분기
	for (APlayerState* PS : RunGameState->PlayerArray)
	{
		ANSPlayerState* NSPlayerState = Cast<ANSPlayerState>(PS);
		if (!NSPlayerState)
		{
			continue;
		}
		UNSPlayerProgressComponent* ProgressComponent = NSPlayerState->GetProgressComponent();
		if (!ProgressComponent)
		{
			continue;
		}

		if (ProgressComponent->IsNPCUnlocked(NPCId))
		{
			// 이미 구출 이력 있으면 보상 재화 누적
			if (UNSCurrencyComponent* CurrencyComponent = NSPlayerState->GetCurrencyComponent())
			{
				CurrencyComponent->AddRunPermanent(
					NSGameplayTags::Currency_Common,
					AlreadyRescuedReward);
			}
		}
		else
		{
			// 최초 구출하면 즉시 해금
			ProgressComponent->UnlockNPC(NPCId);
		}
	}
	
	return true;
}

