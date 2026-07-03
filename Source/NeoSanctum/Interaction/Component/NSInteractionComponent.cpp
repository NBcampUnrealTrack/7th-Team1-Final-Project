// Copyright 2026 One Team. All rights reserved.

#include "NSInteractionComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerController.h"
#include "NeoSanctum/UI/Interaction/NSInteractionPromptWidget.h"
#include "Components/WidgetComponent.h"
#include "NeoSanctum/Interaction/Core/NSInteractable.h"
#include "NeoSanctum/Interaction/NPC/NSInteractableNPCBase.h"
#include "NeoSanctum/UI/Interaction/NSNPCInteractionWidgetBase.h"
#include "NeoSanctum/Progression/Part/NSDroppedPart.h"
#include "NeoSanctum/Core/PlayerController/NSPlayerController.h"
#include "NeoSanctum/UI/Interaction/NSNPCInteractionWidgetBase.h"

UNSInteractionComponent::UNSInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	// 오버랩 됐을때만 틱이 돌게
	PrimaryComponentTick.bStartWithTickEnabled = false;
	
	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetSphereRadius(DetectionRadius);
	DetectionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	// 로컬 컨트롤 폰에서만 EnableLocalInteraction에서 켠다
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	PromptWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PromptWidgetComponent"));
	PromptWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	PromptWidgetComponent->SetDrawSize(FVector2D(200.f, 100.f));
	PromptWidgetComponent->SetVisibility(false);
}

void UNSInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	
	DetectionSphere->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	DetectionSphere->SetSphereRadius(DetectionRadius);
	PromptWidgetComponent->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

	// 이미 possess된 상태면 여기서 활성화, 아니면 무시됨
	EnableLocalInteraction();
}

void UNSInteractionComponent::EnableLocalInteraction()
{
	if (bLocalInteractionEnabled)
	{
		return;
	}
	if (!IsOwnerLocallyControlled())
	{
		return;
	}

	bLocalInteractionEnabled = true;

	if (DefaultPromptWidgetClass)
	{
		PromptWidgetComponent->SetWidgetClass(DefaultPromptWidgetClass);
		ActivePromptWidgetClass = DefaultPromptWidgetClass;
	}

	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &UNSInteractionComponent::OnSphereBeginOverlap);
	DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &UNSInteractionComponent::OnSphereEndOverlap);
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void UNSInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateActiveTarget();
}

void UNSInteractionComponent::UpdateActiveTarget()
{
	APlayerController* PC = GetOwnerController();
	AActor* Owner = GetOwner();
	if (!PC || !Owner)
	{
		HidePrompt();
		return;
	}
	const FVector OwnerLocation = Owner->GetActorLocation();
	AActor* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();
	
	// Remove를 해주기 위해서 역순으로 돔
	for (int32 i = Candidates.Num() - 1; i >= 0; --i)
	{
		AActor* Candidate = Candidates[i].Get();
		if (!Candidate)
		{
			// RemoveAt으로 돔
			Candidates.RemoveAt(i);
			continue;
		}
		if (!INSInteractable::Execute_CanInteract(Candidate, PC))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(OwnerLocation, Candidate->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Candidate;
		}
	}
	
	if (!Nearest)
	{
		HidePrompt();
		ActiveTarget = nullptr;
		if (Candidates.Num() == 0)
		{
			SetComponentTickEnabled(false);
		}
		return;
	}
	
	ActiveTarget = Nearest;
	ShowPromptFor(Nearest);
}

void UNSInteractionComponent::TryInteract()
{
	APlayerController* PC = GetOwnerController();
	AActor* Target = ActiveTarget.Get();
	if (!PC || !Target)
	{
		return;
	}
	// 클라 사전 검사, 실제 검증은 서버에서 재수행
	if (!INSInteractable::Execute_CanInteract(Target, PC))
	{
		return;
	}
	Server_RequestInteract(Target);
}

void UNSInteractionComponent::Server_RequestInteract_Implementation(AActor* Target)
{
	if (!Target || !Target->Implements<UNSInteractable>())
	{
		return;
	}
	APlayerController* PC = GetOwnerController();
	if (!PC)
	{
		return;
	}
	// 서버 권위 검증
	if (!INSInteractable::Execute_CanInteract(Target, PC))
	{
		return;
	}
	
	// 대상의 ShouldHandleOnServer 반환값에 따라 서버에서 즉시 실행
	if (INSInteractable::Execute_ShouldHandleOnServer(Target))
	{
		INSInteractable::Execute_OnInteract(Target, PC);
		return;
	}
    
	// 그 외(거점 NPC 위젯, 드롭 파츠)는 기존 클라 승인 흐름
	Client_OnInteractApproved(Target);
}

void UNSInteractionComponent::Client_OnInteractApproved_Implementation(AActor* Target)
{
	if (!Target || !Target->Implements<UNSInteractable>())
	{
		return;
	}
	APlayerController* PC = GetOwnerController();
	if (!PC)
	{
		return;
	}

	// NPC 상호작용은 PC가 위젯 생성을 담당 (위젯 클래스가 없으면 OnInteract가 직접 처리하는 NPC)
	ANSInteractableNPCBase* NPC = Cast<ANSInteractableNPCBase>(Target);
	ANSPlayerController* NSPC = Cast<ANSPlayerController>(PC);
	if (NPC && NSPC && NPC->GetInteractionWidgetClass())
	{
		if (NPC->GetInteractionWidgetClass())
		{
			NSPC->OpenInteractionWidget(NPC);
			
			return;
		}
	}

	// 드롭 파츠 등 비-NPC 상호작용은 기존 흐름 유지
	INSInteractable::Execute_OnInteract(Target, PC);
}

void UNSInteractionComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}
	if (!OtherActor->Implements<UNSInteractable>())
	{
		return;
	}
	
	// 모두 후보로 등록
	Candidates.AddUnique(OtherActor);
	// 틱 돌리기
	SetComponentTickEnabled(true);
}

void UNSInteractionComponent::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}
	Candidates.Remove(OtherActor);
	if (Candidates.Num() == 0)
	{
		HidePrompt();
		ActiveTarget = nullptr;
		SetComponentTickEnabled(false);
	}
}

void UNSInteractionComponent::ShowPromptFor(AActor* Target)
{
	if (!Target || !PromptWidgetComponent)
	{
		return;
	}

	// 타겟 타입에 따라 위젯 클래스 결정 (타겟 전환 시에만 교체)
	TSubclassOf<UNSInteractionPromptWidget> DesiredClass = DefaultPromptWidgetClass;
	if (Cast<ANSDroppedPart>(Target))
	{
		DesiredClass = PartPromptWidgetClass;
	}
	if (DesiredClass != ActivePromptWidgetClass)
	{
		PromptWidgetComponent->SetWidgetClass(DesiredClass);
		ActivePromptWidgetClass = DesiredClass;
	}

	// 대상이 지정한 프롬프트 앵커 위치에 그대로 배치 (에디터에서 조정)
	const FVector TargetLocation = INSInteractable::Execute_GetPromptWorldLocation(Target);
	PromptWidgetComponent->SetWorldLocation(TargetLocation);

	UNSInteractionPromptWidget* Widget = Cast<UNSInteractionPromptWidget>(PromptWidgetComponent->GetUserWidgetObject());
	if (Widget)
	{
		const FText PromptText = INSInteractable::Execute_GetPromptText(Target);

		if (Cast<ANSDroppedPart>(Target))
		{
			Widget->SetPromptText(
			InteractionKeyText,
	NSLOCTEXT("Interaction", "ReplacePart", "교체"));
			Widget->SetPartName(PromptText);
		}
		else
		{
			Widget->SetPromptText(InteractionKeyText, PromptText);
			Widget->SetPartName(FText::GetEmpty());
		}

		Widget->SetPromptIcon(INSInteractable::Execute_GetPromptIcon(Target));
		Widget->SetRarityStyle(INSInteractable::Execute_GetPromptRarityIndex(Target));
	}
	PromptWidgetComponent->SetVisibility(true);
}

void UNSInteractionComponent::HidePrompt()
{
	if (!PromptWidgetComponent)
	{
		return;
	}
	PromptWidgetComponent->SetVisibility(false);
}

APlayerController* UNSInteractionComponent::GetOwnerController() const
{
	 const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return nullptr;
	}
	return Cast<APlayerController>(OwnerPawn->GetController());
}

bool UNSInteractionComponent::IsOwnerLocallyControlled() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return false;
	}
	return OwnerPawn->IsLocallyControlled();
}
