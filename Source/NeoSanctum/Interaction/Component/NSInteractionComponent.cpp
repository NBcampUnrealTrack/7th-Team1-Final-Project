// Copyright 2026 One Team. All rights reserved.

#include "NSInteractionComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
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
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/MeshComponent.h"
#include "Engine/AssetManager.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/PlayerState.h"
#include "NeoSanctum/Progression/Part/NSPartEquipComponent.h"
#include "NeoSanctum/Progression/Part/NSPartUtils.h"
#include "NeoSanctum/Data/Part/NSPartDefinition.h"
#include "NeoSanctum/Data/Character/NSCharacterBaseStatTypes.h"
#include "NeoSanctum/Character/Player/NSPlayerCharacterBase.h"
#include "NeoSanctum/Core/GameInstance/Subsystem/NSDataSubsystem.h"
#include "NeoSanctum/GAS/Stats/NSCombatStatAttributeMapping.h"

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
	PromptWidgetComponent->SetDrawSize(PromptWidgetDrawSize);

	// 이미 possess된 상태면 여기서 활성화, 아니면 무시됨
	EnableLocalInteraction();
}

void UNSInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 컴포넌트가 없어질때 확실하게 CustomDepth해제
	UpdateOutlineTarget(nullptr);

	// 진행 중이던 머티리얼 비동기 로드 취소, 파괴 후 콜백 실행 방지
	if (OutlineMaterialLoadHandle.IsValid())
	{
		OutlineMaterialLoadHandle->CancelHandle();
		OutlineMaterialLoadHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
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

	// 아웃라인 PP 머티리얼 로드, 카메라 등록
	SetupOutlinePostProcess();
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

	// 전방 판정 기준 -> 카메라 위치/방향
	FVector ViewLocation = OwnerLocation;
	FVector ViewForward = Owner->GetActorForwardVector();
	if (PC->PlayerCameraManager)
	{
		ViewLocation = PC->PlayerCameraManager->GetCameraLocation();
		ViewForward = PC->PlayerCameraManager->GetCameraRotation().Vector();
	}
	const float ViewCosThreshold = FMath::Cos(FMath::DegreesToRadians(InteractViewHalfAngleDeg));
	
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

		// 카메라 전방 시야각 밖의 대상은 후보에서 제외 (등 뒤 프롬프트 방지)
		const FVector ToCandidate = (Candidate->GetActorLocation() - ViewLocation).GetSafeNormal();
		if (FVector::DotProduct(ViewForward, ToCandidate) < ViewCosThreshold)
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

	// 프롬프트가 뜨는 대상 = 아웃라인 대상
	UpdateOutlineTarget(Target);

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

		if (ANSDroppedPart* DroppedPart = Cast<ANSDroppedPart>(Target))
		{
			Widget->SetPromptText(
			InteractionKeyText,
	NSLOCTEXT("Interaction", "ReplacePart", "교체"));
			Widget->SetPartName(PromptText);

			UpdateStatComparisonFor(DroppedPart, Widget);
		}
		else
		{
			Widget->SetPromptText(InteractionKeyText, PromptText);
			Widget->SetPartName(FText::GetEmpty());
			Widget->ClearStatComparison();
		}

		Widget->SetPromptIcon(INSInteractable::Execute_GetPromptIcon(Target));
		Widget->SetRarityStyle(INSInteractable::Execute_GetPromptRarityIndex(Target));
	}
	PromptWidgetComponent->SetVisibility(true);
}

void UNSInteractionComponent::UpdateStatComparisonFor(ANSDroppedPart* DroppedPart, UNSInteractionPromptWidget* Widget)
{
	if (!DroppedPart || !Widget)
	{
		return;
	}

	const FNSPartData& NewPart = DroppedPart->GetStoredPart();

	// Row는 PartSlot(교체 대상 슬롯) 조회용, 스탯은 드롭 인스턴스가 단일 소스
	UNSPartDefinition* Def = NSPartUtils::ResolvePartDefinition(this, NewPart);
	const FNSPartDefinitionRow* NewRow = Def ? NSPartUtils::ResolvePartRow(this, Def->GetPrimaryAssetId()) : nullptr;
	const FGameplayTag NewStatTag = NSPartUtils::GetPartStatTag(this, NewPart);
	if (!NewRow || !NewStatTag.IsValid())
	{
		Widget->ClearStatComparison();
		return;
	}

	UNSDataSubsystem* DataSS = UNSDataSubsystem::Get(this);
	const FNSStatDisplayInfoRow* StatInfo = DataSS ? DataSS->FindStatDisplayInfoRow(NewStatTag) : nullptr;
	if (!StatInfo)
	{
		Widget->ClearStatComparison();
		return;
	}

	// 캐릭터 기본 스탯 Row — 첫 줄(새 파츠 스탯)과 두 번째 줄(잃는 스탯) 양쪽의 기준선 조회에 사용
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const ANSPlayerCharacterBase* PlayerCharacter = Cast<ANSPlayerCharacterBase>(OwnerPawn);
	const UNSCharacterData* CharacterData = PlayerCharacter ? PlayerCharacter->GetCurrentCharacterData() : nullptr;
	const FNSCharacterBaseStatRow* BaseStatRow = (CharacterData && DataSS)
		? DataSS->FindCharacterBaseStatRow(CharacterData->CharacterTag)
		: nullptr;

	// 캐릭터 기본 스탯값 (매칭되는 필드 없으면 0) — 이전/이후 수치 둘 다의 기준선으로 사용
	const float BaseValue = BaseStatRow ? BaseStatRow->GetValueForTag(NewStatTag) : 0.f;

	const APlayerState* PS = OwnerPawn ? OwnerPawn->GetPlayerState() : nullptr;
	const UNSPartEquipComponent* EquipComp = PS ? PS->FindComponentByClass<UNSPartEquipComponent>() : nullptr;
	const FNSPartData* OldPart = EquipComp ? EquipComp->GetEquippedPart(NewRow->PartSlot) : nullptr;

	// 같은 종류 파츠라도 인스턴스마다 스탯이 다르게 뽑힐 수 있어 인스턴스 태그끼리 비교
	const FGameplayTag OldStatTag = OldPart ? NSPartUtils::GetPartStatTag(this, *OldPart) : FGameplayTag();
	float OldPartSameStatValue = 0.f;
	if (OldPart && OldStatTag == NewStatTag)
	{
		OldPartSameStatValue = OldPart->CurrentValue;
	}

	// DataTable 기본값 + 파츠 수치 (Attribute 조회가 불가능한 스탯용, 증강 등 다른 보정 미반영)
	float OldTotal = BaseValue + OldPartSameStatValue;
	float NewTotal = BaseValue + NewPart.CurrentValue;

	/**
	 * 이전 값 : 캐릭터의 현재 실제 스탯(ASC 라이브 값, 파츠/증강/영구강화 전부 반영)
	 * 이후 값 : 현재 스탯에서 같은 슬롯·같은 스탯 기존 파츠 기여를 뺀 뒤 새 파츠 값을 더한 예상치 (파츠 Add 연산 기준)
	 */
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS);
	const UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;

	// StatTag → Attribute 변환은 공용 매핑 테이블 사용, 매핑 없는 스탯(FireRate 등)은 무효 → DataTable 폴백 유지
	const FNSCombatStatAttributeMapping* Mapping = NSCombatStatAttribute::FindMapping(NewStatTag);
	const FGameplayAttribute Attribute = Mapping ? Mapping->Attribute : FGameplayAttribute();
	if (ASC && Attribute.IsValid() && ASC->HasAttributeSetForAttribute(Attribute))
	{
		const float LiveValue = ASC->GetNumericAttribute(Attribute);
		OldTotal = LiveValue;
		NewTotal = LiveValue - OldPartSameStatValue + NewPart.CurrentValue;
	}

	Widget->SetStatComparison(StatInfo->DisplayName, OldTotal, NewTotal, StatInfo->bHigherIsBetter);

	// ===== 두 번째 줄: 스탯이 다른 기존 파츠를 버리게 될 때, 잃는 스탯의 하락 예상치 표시 =====
	// (기존 파츠가 없거나 새 파츠와 같은 스탯이면 첫 줄이 이미 차감을 반영하므로 숨김)
	const FNSStatDisplayInfoRow* OldStatInfo = (OldStatTag.IsValid() && OldStatTag != NewStatTag && DataSS)
		? DataSS->FindStatDisplayInfoRow(OldStatTag)
		: nullptr;
	if (!OldStatInfo)
	{
		Widget->ClearSecondaryStatComparison();
		return;
	}

	// 기준선은 첫 줄과 동일한 규칙: ASC 라이브 값 우선, 불가하면 DataTable 기본값 + 기존 파츠 기여
	float OldStatCurrent = (BaseStatRow ? BaseStatRow->GetValueForTag(OldStatTag) : 0.f) + OldPart->CurrentValue;
	const FNSCombatStatAttributeMapping* OldMapping = NSCombatStatAttribute::FindMapping(OldStatTag);
	const FGameplayAttribute OldAttribute = OldMapping ? OldMapping->Attribute : FGameplayAttribute();
	if (ASC && OldAttribute.IsValid() && ASC->HasAttributeSetForAttribute(OldAttribute))
	{
		OldStatCurrent = ASC->GetNumericAttribute(OldAttribute);
	}

	// 기존 파츠가 사라지면 그 기여만큼 빠진 값이 예상치 (파츠 Add 연산 기준)
	Widget->SetSecondaryStatComparison(
		OldStatInfo->DisplayName, OldStatCurrent, OldStatCurrent - OldPart->CurrentValue, OldStatInfo->bHigherIsBetter);
}

void UNSInteractionComponent::HidePrompt()
{
	// 프롬프트가 사라지는 모든 경로에서 아웃라인도 함께 해제
	UpdateOutlineTarget(nullptr);

	if (!PromptWidgetComponent)
	{
		return;
	}
	PromptWidgetComponent->SetVisibility(false);
}

void UNSInteractionComponent::SetupOutlinePostProcess()
{
	// 머티리얼이 지정 안 된 프로젝트 상태(에디터 작업 전)에서도 조용히 동작하도록
	if (OutlinePostProcessMaterial.IsNull())
	{
		return;
	}
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	// 카메라가 없는 폰이면 아웃라인만 생략 — 감지/프롬프트는 정상 동작
	UCameraComponent* Camera = Owner->FindComponentByClass<UCameraComponent>();
	if (!Camera)
	{
		return;
	}

	// 비동기 로드 후 완료 시점에 블렌더블 등록
	TWeakObjectPtr<UNSInteractionComponent> WeakThis(this);
	OutlineMaterialLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		OutlinePostProcessMaterial.ToSoftObjectPath(),
		[WeakThis]()
		{
			UNSInteractionComponent* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			UMaterialInterface* Material = Self->OutlinePostProcessMaterial.Get();
			if (!Material)
			{
				return;
			}
			AActor* LoadedOwner = Self->GetOwner();
			if (!LoadedOwner)
			{
				return;
			}
			UCameraComponent* LoadedCamera = LoadedOwner->FindComponentByClass<UCameraComponent>();
			if (!LoadedCamera)
			{
				return;
			}
			// 아웃라인 표시 여부는 CustomDepth on/off가 결정
			LoadedCamera->PostProcessSettings.AddBlendable(Material, 1.f);
		});
}

void UNSInteractionComponent::UpdateOutlineTarget(AActor* NewTarget)
{
	AActor* Current = OutlinedTarget.Get();
	// 같은 대상이면 아무것도 안 함
	if (Current == NewTarget)
	{
		return;
	}
	// 이전 대상 끄기
	if (Current)
	{
		SetActorOutlineEnabled(Current, false);
	}
	if (NewTarget)
	{
		SetActorOutlineEnabled(NewTarget, true);
	}
	OutlinedTarget = NewTarget;
}

void UNSInteractionComponent::SetActorOutlineEnabled(AActor* Target, bool bEnabled)
{
	// NPC/드롭 파츠 등 타입 구분 없이 대상의 모든 메시에 적용
	TArray<UMeshComponent*> MeshComponents;
	Target->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* Mesh : MeshComponents)
	{
		if (!Mesh)
		{
			continue;
		}
		Mesh->SetRenderCustomDepth(bEnabled);
		// 끌 때는 0으로 되돌려 잔여 스텐실을 남기지 않음
		Mesh->SetCustomDepthStencilValue(bEnabled ? OutlineStencilValue : 0);
	}
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
