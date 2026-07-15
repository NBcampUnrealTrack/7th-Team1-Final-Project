// Copyright 2026 One Team. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NSInteractionComponent.generated.h"

class USphereComponent;
class UWidgetComponent;
class UNSInteractionPromptWidget;
class UMaterialInterface;
class ANSDroppedPart;
struct FStreamableHandle;

UCLASS(ClassGroup=(NeoSanctum), meta=(BlueprintSpawnableComponent))
class NEOSANCTUM_API UNSInteractionComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UNSInteractionComponent();
	
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// PlayerController의 Interact 입력에서 호출
	void TryInteract();

	// 로컬 컨트롤 폰에서만 감지 활성화. 비로컬/중복 호출은 무시되도록
	void EnableLocalInteraction();

protected:
	// 서버에서 CanInteract 재검증 후 Client_OnInteractApproved 호출
	UFUNCTION(Server, Reliable)
	void Server_RequestInteract(AActor* Target);

	// 서버 승인 후 클라이언트에서 UI 오픈
	UFUNCTION(Client, Reliable)
	void Client_OnInteractApproved(AActor* Target);

protected:
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	// 후보 중 가장 가까운 유효 대상 갱신
	void UpdateActiveTarget();
	void ShowPromptFor(AActor* Target);
	void HidePrompt();

	// 드랍된 파츠와 현재 슬롯 장착 파츠(없으면 캐릭터 기본 스탯)를 비교해 위젯에 반영
	void UpdateStatComparisonFor(ANSDroppedPart* DroppedPart, UNSInteractionPromptWidget* Widget);

	// ActiveTarget 아웃라인
	void SetupOutlinePostProcess();

	/**
	 * 아웃라인 대상 교체 -> 이전 대상은 끄고 새 대상은 켠다.
	 * 같은 대상이면 즉시 리턴하므로 매 틱 호출되어도 비용 없다.
	 */
	void UpdateOutlineTarget(AActor* NewTarget);

	// 대상 액터의 모든 메시 컴포넌트에 CustomDepth 렌더링 on/off 적용
	void SetActorOutlineEnabled(AActor* Target, bool bEnabled);

	APlayerController* GetOwnerController() const;
	bool IsOwnerLocallyControlled() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
	TObjectPtr<USphereComponent> DetectionSphere;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
	TObjectPtr<UWidgetComponent> PromptWidgetComponent;
	
	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	float DetectionRadius = 200.f;

	// 상호작용 후보로 인정할 카메라 전방 시야 각도
	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	float InteractViewHalfAngleDeg = 60.f;

	// 프롬프트 위젯의 화면 공간 크기
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Prompt")
	FVector2D PromptWidgetDrawSize = FVector2D(200.f, 100.f);

	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	FText InteractionKeyText = FText::FromString(TEXT("F"));

	// 기본 심플 프롬프트 위젯
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Prompt")
	TSubclassOf<UNSInteractionPromptWidget> DefaultPromptWidgetClass;

	// 파츠 전용 프롬프트 위젯 —> 아이콘/이름 표시
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Prompt")
	TSubclassOf<UNSInteractionPromptWidget> PartPromptWidgetClass;
	
	/**
	 * 아웃라인용 포스트프로세스 머티리얼
	 * 스텐실 값→색상 매핑 구조로 제작됨 -> 추후 아군 or 적군쪽에서 재사용
	 * EnableLocalInteraction에서 비동기 로드
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Outline")
	TSoftObjectPtr<UMaterialInterface> OutlinePostProcessMaterial;
	
	/**
	 * 아웃라인 대상 메시에 기록할 스텐실 값
	 * PP 머티리얼에서 1=흰색으로 매핑
	 * 0은 `아웃라인 없음` 예약값이므로 사용 금지 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Outline")
	int32 OutlineStencilValue = 1;

private:
	// 현재 WidgetComponent에 세팅된 클래스 캐싱
	TSubclassOf<UNSInteractionPromptWidget> ActivePromptWidgetClass;
	
private:
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> Candidates;
	
	UPROPERTY()
	TWeakObjectPtr<AActor> ActiveTarget;

	// 현재 아웃라인이 켜져 있는 대상 — 전환 시 이전 대상을 꺼주기 위해 추적
	UPROPERTY()
	TWeakObjectPtr<AActor> OutlinedTarget;

	// 중복 셋업 방지
	bool bLocalInteractionEnabled = false;

	// 진행 중인 아웃라인 PP 머티리얼 비동기 로드 핸들
	TSharedPtr<FStreamableHandle> OutlineMaterialLoadHandle;
};
