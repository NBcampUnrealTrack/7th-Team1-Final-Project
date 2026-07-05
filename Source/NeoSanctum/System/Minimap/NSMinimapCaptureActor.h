// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "NSMinimapCaptureActor.generated.h"

class USceneCaptureComponent2D;
class UTexture2D;
class ULevelStreamingDynamic;
class UTextureRenderTarget2D;

//미니맵 캡처 층 설정
USTRUCT(BlueprintType)
struct FNSMinimapCaptureLayerConfig
{
	GENERATED_BODY()

	//층 식별 번호
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	int32 LayerIndex = 0;

	//층 하단 높이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float FloorZ = 0.0f;

	//층 상단 높이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float CeilingZ = 3000.0f;
};

UCLASS()
class NEOSANCTUM_API ANSMinimapCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	ANSMinimapCaptureActor();

	// 던전 미니맵 캡처 요청
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void CaptureDungeonMinimap();

	// DungeonGenerator 바인딩
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void BindToDungeonGenerator(AActor* NewDungeonGeneratorActor);

	UFUNCTION(BlueprintPure, Category = "Minimap")
	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "Minimap")
	USceneCaptureComponent2D* GetSceneCaptureComponent() const { return SceneCaptureComponent.Get(); }

	// 현재 액터 위치 기준 수동 캡처
	UFUNCTION(BlueprintCallable, Category = "Minimap|Debug")
	void CaptureFromCurrentActorTransform(float ManualOrthoWidth);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleDungeonPostGeneration();

	UFUNCTION()
	void HandleRoomLevelLoaded();

	// DungeonGenerator 자동 탐색
	void TryAutoBindDungeonGenerator();

	// 던전 룸 로딩 대기 시작
	void BeginWaitingForDungeonRooms();

	// 던전 룸 준비 상태 확인
	void CheckDungeonRoomsReady();

	// 예약된 캡처 실행
	void ExecuteScheduledCapture();

	// 준비된 캡처 실행
	void ExecutePreparedCapture();

	// 준비된 캡처 종료
	void FinishPreparedCapture();

	// 룸 로드 이벤트 바인딩
	void BindRoomLoadEvents();

	// 룸 로드 이벤트 해제
	void UnbindRoomLoadEvents();

	// 던전 룸 준비 여부 확인
	bool AreDungeonRoomsReady(int32& OutReadyRoomCount, int32& OutTotalRoomCount) const;

	// 렌더 가능한 룸 컴포넌트 수 계산
	int32 CountRenderableRoomPrimitiveComponents() const;

	// 로컬 플레이어 캡처 준비 여부 확인
	bool IsLocalPlayerReadyForCapture(const FBox& DungeonBounds, FVector& OutPawnLocation) const;

	// 던전 월드 범위 계산
	bool BuildDungeonWorldBounds(FBox& OutWorldBounds) const;

	// 렌더 타겟 생성 보장
	void EnsureRenderTarget();

	// 미니맵 캡처 표시 플래그 설정
	void ConfigureMinimapShowFlags() const;

	// 던전 룸 강제 표시 설정
	void SetDungeonRoomsForceVisible(bool bForceVisible) const;

	// 캡처용 룸 액터 표시 상태 준비
	void PrepareRoomActorVisibilityForCapture();

	// 캡처 후 룸 액터 표시 상태 복구
	void RestoreRoomActorVisibilityAfterCapture();

	// 렌더 타겟 파일 덤프
	void DumpRenderTargetToFile();

	// 텍스처 파일 덤프
	void DumpTextureToFile(UTexture2D* Texture, const FString& Label) const;

	// 층별 렌더 타겟 생성 보장
	void EnsureLayerRenderTargets();

	// NavMesh 기반 층 텍스처 생성
	UTexture2D* BuildNavMeshTextureForLayer(const FNSMinimapCaptureLayerConfig& LayerConfig, const FBox& TextureWorldBounds);

	// NavMesh 기반 다층 캡처
	void CapturePreparedNavMeshLayers();

	// 층 캡처용 룸 액터 표시 상태 준비
	void PrepareRoomActorVisibilityForLayer(const FNSMinimapCaptureLayerConfig& LayerConfig);

	// 렌더 타겟 기반 다층 캡처
	void CapturePreparedLayers();

	// 상단 캡처 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneCaptureComponent2D> SceneCaptureComponent;

	// 캡처 대상 DungeonGenerator
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> DungeonGeneratorActor;

	// DungeonGenerator 자동 탐색 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	bool bAutoFindDungeonGenerator = true;

	// 미니맵 텍스처 해상도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "128", ClampMax = "4096"))
	int32 RenderTargetSize = 1024;

	// 층별 캡처 사용 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Layers", meta = (AllowPrivateAccess = "true"))
	bool bUseLayeredCapture = true;

	// NavMesh 텍스처 생성 사용 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|NavMesh", meta = (AllowPrivateAccess = "true"))
	bool bUseNavMeshTextureCapture = true;

	// NavMesh 영역 채움 색상
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|NavMesh", meta = (AllowPrivateAccess = "true"))
	FLinearColor NavMeshFillColor = FLinearColor(0.05f, 0.55f, 1.0f, 0.78f);

	// NavMesh 외곽선 색상
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|NavMesh", meta = (AllowPrivateAccess = "true"))
	FLinearColor NavMeshOutlineColor = FLinearColor(0.0f, 0.12f, 0.28f, 0.95f);

	// NavMesh 배경 색상
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|NavMesh", meta = (AllowPrivateAccess = "true"))
	FLinearColor NavMeshBackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// NavMesh 채움 확장 픽셀
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|NavMesh", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "16"))
	int32 NavMeshFillExpansionPixels = 1;

	// NavMesh 외곽선 두께 픽셀
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|NavMesh", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "32"))
	int32 NavMeshOutlineThicknessPixels = 3;

	// 캡처할 층 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Layers", meta = (AllowPrivateAccess = "true"))
	TArray<FNSMinimapCaptureLayerConfig> CaptureLayers;

	// 던전 경계 여유 범위
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float BoundsPadding = 600.0f;

	// 플레이어 주변 디버그 캡처 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Debug", meta = (AllowPrivateAccess = "true"))
	bool bDebugCaptureAroundPlayer = false;

	// 플레이어 주변 디버그 캡처 폭
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Debug", meta = (AllowPrivateAccess = "true", ClampMin = "100.0", EditCondition = "bDebugCaptureAroundPlayer"))
	float DebugPlayerOrthoWidth = 6000.0f;

	// 캡처 카메라 높이
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "100.0"))
	float CaptureHeight = 12000.0f;

	// 캡처 실행 지연 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CaptureDelay = 1.0f;

	// 표시 상태 적용 후 캡처 대기 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CaptureVisibilitySettleDelay = 0.05f;

	// 캡처 후 표시 상태 복구 지연 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CaptureVisibilityRestoreDelay = 0.75f;

	// 표시 상태 유지 중 매 프레임 캡처 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	bool bCaptureEveryFrameDuringVisibilityWindow = true;

	// 룸 준비 상태 확인 간격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "0.02"))
	float RoomReadyCheckInterval = 0.1f;

	// 룸 준비 대기 최대 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MaxRoomReadyWaitTime = 10.0f;

	// 안정 상태로 판단할 연속 확인 횟수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 RequiredStableReadyCheckCount = 5;

	// 로컬 플레이어 위치 대기 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	bool bWaitForLocalPlayerPawnInDungeon = true;

	// 캡처 중 룸 강제 표시 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	bool bForceRoomsVisibleForCapture = true;

	// 태그 기반 표시 액터 제한 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	bool bUseShowOnlyActorsWithTag = false;

	// 캡처에 포함할 액터 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true", EditCondition = "bUseShowOnlyActorsWithTag"))
	FName ShowOnlyActorTag = TEXT("MinimapCapture");

	// 캡처에서 제외할 액터 태그 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	TArray<FName> HiddenActorTags = { TEXT("MinimapCaptureHidden") };

	// 씬 캡처 소스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<ESceneCaptureSource> CaptureSource = SCS_FinalColorLDR;

	// 대기/안개/후처리 비활성화 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Rendering", meta = (AllowPrivateAccess = "true"))
	bool bDisableAtmosphereFogAndPostProcess = true;

	// 렌더 타겟 초기화 색상
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
	FLinearColor ClearColor = FLinearColor(0.015f, 0.015f, 0.015f, 1.0f);

	// 캡처 결과 파일 저장 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Debug", meta = (AllowPrivateAccess = "true"))
	bool bDumpRenderTargetToFile = false;

	// 캡처 결과 저장 폴더
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap|Debug", meta = (AllowPrivateAccess = "true", EditCondition = "bDumpRenderTargetToFile"))
	FString DebugDumpDirectory = TEXT("MinimapCaptures");

	// 단일 캡처 렌더 타겟
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	// 층별 캡처 렌더 타겟 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextureRenderTarget2D>> LayerRenderTargets;

	// NavMesh 기반 생성 텍스처 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> NavMeshLayerTextures;

	// 로드 이벤트를 구독한 룸 인스턴스 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULevelStreamingDynamic>> BoundRoomInstances;

	// 캡처 실행 타이머
	FTimerHandle CaptureTimerHandle;

	// 준비된 캡처 실행 타이머
	FTimerHandle PreparedCaptureTimerHandle;

	// 표시 상태 복구 타이머
	FTimerHandle RestoreVisibilityTimerHandle;

	// 룸 준비 확인 타이머
	FTimerHandle RoomReadyCheckTimerHandle;

	// 캡처 전 액터 숨김 상태 보관
	TMap<TWeakObjectPtr<AActor>, bool> ActorHiddenStateBeforeCapture;

	// 캡처에 사용할 확장된 월드 범위
	FBox PendingCaptureBounds = FBox(ForceInit);

	// 던전 원본 월드 범위
	FBox PendingDungeonBounds = FBox(ForceInit);

	// 대기 중인 캡처 폭
	float PendingOrthoWidth = 0.0f;

	// 준비된 캡처 보유 여부
	bool bHasPreparedCapture = false;

	// 안정 상태 확인 누적 횟수
	int32 StableReadyCheckCount = 0;

	// 룸 준비 대기 시작 시간
	double RoomReadyWaitStartTime = 0.0;
};
