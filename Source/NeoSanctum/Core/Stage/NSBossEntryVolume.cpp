// Copyright 2026 One Team. All rights reserved.


#include "NSBossEntryVolume.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "NeoSanctum/Core/GameState/NSRunGameState.h"
#include "NeoSanctum/Core/Interface/NSRunGameModeInterface.h"
#include "NeoSanctum/Core/PlayerState/NSPlayerState.h"


ANSBossEntryVolume::ANSBossEntryVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(200.f));
	// 활성화 전까지 감지 꺼둠
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); 
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	// Player 채널
	TriggerBox->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(TriggerBox);
	// 자식까지 숨김, 비활성 기본
	VisualRoot->SetVisibility(false, true);
	OutlineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OutlineMesh"));
	OutlineMesh->SetupAttachment(VisualRoot);
	// 시각 전용
	OutlineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OutlineMesh->SetCollisionProfileName(TEXT("NoCollision"));
	OutlineMesh->SetGenerateOverlapEvents(false);
	// 그림자 렌더 비용 제거
	OutlineMesh->SetCastShadow(false);

	// 엔진 기본 큐브 메시 지정 (BP에서 교체 가능)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		OutlineMesh->SetStaticMesh(CubeMesh.Object);
	}
}

void ANSBossEntryVolume::BeginPlay()
{
	Super::BeginPlay();
	// 콜리전이 꺼져 있어도 바인딩은 해둬야함 (활성화 시 바로 동작)
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ANSBossEntryVolume::OnBeginOverlap);
	TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ANSBossEntryVolume::OnEndOverlap);
	
	if (OutlineMesh && TriggerBox)
	{
		OutlineMesh->SetWorldScale3D(
			TriggerBox->GetUnscaledBoxExtent() / 50.0f);
	}
	
	BindToRunGameState();
}

void ANSBossEntryVolume::Activate()
{
	if (!HasAuthority() || bActivated)
	{
		return;
	}
	
	bActivated = true;
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	UE_LOG(LogTemp, Warning, TEXT("[BossVolume] Activate 호출됨, 콜리전 QueryOnly로 전환"));
	
	// 볼륨 오버랩 타이머 활성화 중 플레이어 사망에 따른 집합 정리
	GetWorldTimerManager().SetTimer(
		RevalidateTimerHandle,
		this,
		&ANSBossEntryVolume::RevalidateOccupants,
		0.5f,
		true);

	// 활성화 순간 이미 볼륨 안에 서 있는 플레이어는 BeginOverlap이 안 오므로 직접 수집
	TArray<AActor*> Already;
	TriggerBox->GetOverlappingActors(Already, APawn::StaticClass());
	for (AActor* Actor : Already)
	{
		if (IsPlayerPawn(Actor))
		{
			OverlappingPlayers.Add(Actor);
		}
	}
	
	if (OverlappingPlayers.Num() > 0)
	{
		StartDwellTimer();
		// 활성화 시점에 이미 전원이면 단축 유예로 전환
		if (AreAllPlayersPresent())
		{
			bAllPresentScheduled = true;
			GetWorldTimerManager().ClearTimer(DwellTimerHandle);
			GetWorldTimerManager().SetTimer(
				DwellTimerHandle,
				this,
				&ANSBossEntryVolume::OnDwellCompleted,
				AllPresentDelay,
				false);
			
			// 단축 duration + 전원여부(true) 통지
			PushBossGateStateToGameState(AllPresentDelay);
		}
	}
}

void ANSBossEntryVolume::Deactivate()
{
	bActivated = false;
	bAllPresentScheduled = false;
	GetWorldTimerManager().ClearTimer(DwellTimerHandle);
	GetWorldTimerManager().ClearTimer(RevalidateTimerHandle);
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OverlappingPlayers.Empty();
	ClearBossGateState();
}

void ANSBossEntryVolume::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 큐브를 TriggerBox 반경에 맞춰 외곽선과 오버랩 범위 일치
	if (OutlineMesh && TriggerBox)
	{
		OutlineMesh->SetWorldScale3D(
			TriggerBox->GetUnscaledBoxExtent() / 50.0f);
	}
}

void ANSBossEntryVolume::OnBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& Sweep)
{
	UE_LOG(LogTemp, Warning, TEXT("[BossVolume] BeginOverlap: Actor=%s, bActivated=%d, IsPlayer=%d"),
	*GetNameSafe(OtherActor), bActivated ? 1 : 0, IsPlayerPawn(OtherActor) ? 1 : 0);
	
	if (!HasAuthority() || !bActivated || !IsPlayerPawn(OtherActor))
	{
		return;
	}
	
	OverlappingPlayers.Add(OtherActor);

	// 0→1: 기본 카운트다운 시작
	if (OverlappingPlayers.Num() == 1)
	{
		StartDwellTimer();
	}
	// 전원 집결 시 단축 유예로 전환
	if (!bAllPresentScheduled && AreAllPlayersPresent())
	{
		bAllPresentScheduled = true;
		GetWorldTimerManager().ClearTimer(DwellTimerHandle);
		GetWorldTimerManager().SetTimer(
			DwellTimerHandle,
			this,
			&ANSBossEntryVolume::OnDwellCompleted,
			AllPresentDelay,
			false);
		
		// 단축 duration + 전원여부(true) 통지
		PushBossGateStateToGameState(AllPresentDelay);
	}
}

void ANSBossEntryVolume::OnEndOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, 
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!HasAuthority() || !bActivated || !OverlappingPlayers.Contains(OtherActor))
	{
		return;
	}
	
	OverlappingPlayers.Remove(OtherActor);

	// 전원 집결이 깨졌으면 단축 예약 취소 후 상황에 맞게 복구
	if (bAllPresentScheduled)
	{
		bAllPresentScheduled = false;
		GetWorldTimerManager().ClearTimer(DwellTimerHandle);
		if (OverlappingPlayers.Num() > 0)
		{
			// 아직 누군가 남아있으면 기본 카운트다운 재개
			StartDwellTimer();  
		}
		else
		{
			// 남은 사람 없으면 미진행 통지
			ClearBossGateState();
		}
	}
	// 전원 이탈하면 기본 카운트다운 리셋
	else if (OverlappingPlayers.Num() == 0)
	{
		GetWorldTimerManager().ClearTimer(DwellTimerHandle);
		ClearBossGateState();
	}
}

void ANSBossEntryVolume::OnDwellCompleted()
{
	if (!HasAuthority())
	{
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[BossVolume] Dwell 완료 → NotifyBossGateReached 호출"));
	
	// 완료 직전 최종 검증
	RevalidateOccupants();

	// 재검사 후 아무도 안 남았으면 통지 취소
	if (OverlappingPlayers.Num() == 0)
	{
		ClearBossGateState();
		return; 
	}
	
	// 중복 트리거 방지
	Deactivate(); 

	if (AGameModeBase* GameMode = GetWorld()->GetAuthGameMode())
	{
		if (GameMode->Implements<UNSRunGameModeInterface>())
		{
			INSRunGameModeInterface::Execute_NotifyBossGateReached(GameMode);
		}
	}
}

void ANSBossEntryVolume::HandleStagePhaseChanged()
{
	if (!CachedRunGameState)
	{
		return;
	}
	
	const bool bBossReady = (CachedRunGameState->StagePhase == ENSStagePhase::BossReady);
	
	UE_LOG(LogTemp, Warning, TEXT("[BossVolume] StagePhase 변화 감지, bBossReady=%d, HasAuthority=%d"),
	bBossReady ? 1 : 0, HasAuthority() ? 1 : 0);
	
	// 연출 갱신 (표시/색)
	UpdateVisual();

	// 감지/판정은 서버만
	if (!HasAuthority())
	{
		return;
	}
	
	if (bBossReady)
	{
		Activate();
	}
	else
	{
		Deactivate();
	}
}

void ANSBossEntryVolume::RevalidateOccupants()
{
	if (!HasAuthority() || !bActivated)
	{
		return;
	}

	// 무효(파괴)되거나 사망한 폰 정리
	bool bChanged = false;
	for (auto It = OverlappingPlayers.CreateIterator(); It; ++It)
	{
		const AActor* Actor = It->Get();
		bool bValid = IsValid(Actor) && IsPlayerPawn(Actor);

		// 사망 판정
		if (bValid)
		{
			const APawn* Pawn = Cast<APawn>(Actor);
			const ANSPlayerState* NSPS = Pawn ? Pawn->GetPlayerState<ANSPlayerState>() : nullptr;
			if (NSPS && NSPS->IsDead())
			{
				bValid = false;
			}
		}

		if (!bValid)
		{
			It.RemoveCurrent();
			bChanged = true;
		}
	}

	if (!bChanged)
	{
		return;
	}

	// 인원 변화 반영
	if (OverlappingPlayers.Num() == 0)
	{
		// 볼륨위에 아무도 없으면 초기회
		bAllPresentScheduled = false;
		GetWorldTimerManager().ClearTimer(DwellTimerHandle);
		ClearBossGateState();
	}
	else if (bAllPresentScheduled && !AreAllPlayersPresent())
	{
		// 누군가 남아있으면 기존 타이머 시작
		bAllPresentScheduled = false;
		GetWorldTimerManager().ClearTimer(DwellTimerHandle);
		StartDwellTimer();
	}
	else if (!bAllPresentScheduled && AreAllPlayersPresent())
	{
		// 볼륨밖에서 누가 죽어서 전원 볼륨 위 판정이 되면 단축 타이머 실행
		bAllPresentScheduled = true;
		GetWorldTimerManager().ClearTimer(DwellTimerHandle);
		GetWorldTimerManager().SetTimer(
			DwellTimerHandle,
			this,
			&ANSBossEntryVolume::OnDwellCompleted,
			AllPresentDelay,
			false);
		
		PushBossGateStateToGameState(AllPresentDelay);
	}
}

void ANSBossEntryVolume::BindToRunGameState()
{
	ANSRunGameState* RunGS = GetWorld() ? GetWorld()->GetGameState<ANSRunGameState>() : nullptr;
	if (!RunGS)
	{
		// 클라에서 GameState 아직 미복제 — 다음 틱 재시도
		GetWorldTimerManager().SetTimerForNextTick(this, &ANSBossEntryVolume::BindToRunGameState);
		return;
	}

	CachedRunGameState = RunGS;
	RunGS->OnStagePhaseChanged.AddDynamic(
		this, 
		&ANSBossEntryVolume::HandleStagePhaseChanged);
	RunGS->OnBossGateChanged.AddDynamic(
		this,
		&ANSBossEntryVolume::HandleBossGateChanged);
	// 바인딩이 BossReady 이후에 이뤄진 경우 대비 초기 1회 동기화
	HandleStagePhaseChanged();
}

bool ANSBossEntryVolume::IsPlayerPawn(const AActor* OtherActor) const
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	
	return Pawn && Cast<APlayerController>(Pawn->GetController()) != nullptr;
}

void ANSBossEntryVolume::StartDwellTimer()
{
	// 이미 카운트다운 중이면 유지
	if (GetWorldTimerManager().IsTimerActive(DwellTimerHandle))
	{
		return;
	}
	
	GetWorldTimerManager().SetTimer(
		DwellTimerHandle,
		this,
		&ANSBossEntryVolume::OnDwellCompleted,
		DwellDuration,
		false);
	
	// 기본 duration 기준 종료시각 통지
	PushBossGateStateToGameState(DwellDuration);
}

void ANSBossEntryVolume::PushBossGateStateToGameState(float DurationFromNow)
{
	if (!HasAuthority() || !CachedRunGameState)
	{
		return;
	}
	
	// 클라 계산과 일치하도록 서버월드시간 기준으로 종료시각 산출
	const float EndServerTime =
		CachedRunGameState->GetServerWorldTimeSeconds() + DurationFromNow;
	CachedRunGameState->SetBossGateState(
		EndServerTime,
		bAllPresentScheduled);
}

void ANSBossEntryVolume::ClearBossGateState()
{
	if (!HasAuthority() || !CachedRunGameState)
	{
		return;
	}
	
	// 종료시각 0이면 미진행, 전원여부도 해제
	CachedRunGameState->SetBossGateState(
		0.0f,
		false);
}

void ANSBossEntryVolume::UpdateVisual()
{
	if (!CachedRunGameState || !OutlineMesh)
	{
		return;
	}
	
	const bool bBossReady =
		(CachedRunGameState->StagePhase == ENSStagePhase::BossReady);
	// 표시/숨김
	VisualRoot->SetVisibility(bBossReady, true);
	if (!bBossReady)
	{
		return;
	}

	// 동적 머티리얼 최초 1회 생성
	if (!OutlineMID)
	{
		OutlineMID = OutlineMesh->CreateDynamicMaterialInstance(0);
	}
	if (OutlineMID)
	{
		// 전원 집결이면 노랑, 아니면 파랑
		const FLinearColor Color =
			CachedRunGameState->bBossGateAllPresent ? AllPresentColor : WaitingColor;
		OutlineMID->SetVectorParameterValue(ColorParameterName, Color);
	}
}

void ANSBossEntryVolume::HandleBossGateChanged()
{
	UpdateVisual();
}

bool ANSBossEntryVolume::AreAllPlayersPresent() const
{
	const ANSRunGameState* GS = GetWorld() ? GetWorld()->GetGameState<ANSRunGameState>() : nullptr;
	if (!GS)
	{
		return false;
	}
	TArray<ANSPlayerState*> Alive;
	GS->GetAlivePlayerStates(Alive);
	// 생존자가 있어야 하고, 볼륨 위 인원이 그 수 이상이면 전원 집결
	return Alive.Num() > 0 && OverlappingPlayers.Num() >= Alive.Num();
}
