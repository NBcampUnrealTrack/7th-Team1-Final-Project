// Copyright 2026 One Team. All rights reserved.


#include "NSDamageFlashComponent.h"
#include "AbilitySystemInterface.h"
#include "NeoSanctum/GAS/AttributeSet/NSBaseAttributeSet.h"

UNSDamageFlashComponent::UNSDamageFlashComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSDamageFlashComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	
	CancelFlash(); // 내부에서 ClearTimer + StopFlash
	
	Super::EndPlay(EndPlayReason);
}

void UNSDamageFlashComponent::PlayFlash()
{
	UWorld* World = GetWorld();
	if (!World || !FlashMaterial || FlashDuration <= 0.0f)
	{
		return;
	}

	EnsureDynamicMaterials();
	if (OverlayMIDs.Num() == 0)
	{
		return;
	}

	// 색 결정 (데미지 차감 후 체력 기준 — ASC 직접 조회)
	const FLinearColor Color = ResolveFlashColor();

	// 오버레이 적용 + 색/색 강도/ 최대 불투명도 세팅 (재시작 시에도 색 갱신됨)
	for (int32 i = 0; i < CachedMeshes.Num(); ++i)
	{
		UMeshComponent* Mesh = CachedMeshes[i];
		UMaterialInstanceDynamic* MID = OverlayMIDs[i];
		if (!IsValid(Mesh) || !MID)
		{
			continue;
		}

		MID->SetVectorParameterValue(TEXT("FlashColor"), Color);
		MID->SetScalarParameterValue(TEXT("ColorPower"), ColorPower);
		MID->SetScalarParameterValue(TEXT("Opacity"), PeakOpacity);
		Mesh->SetOverlayMaterial(MID);
	}

	// 수명주기 시작. 이미 돌고 있으면 SetTimer가 핸들을 교체 = 추가 데미지 시 재시작
	FlashStartTime = World->GetTimeSeconds();
	World->GetTimerManager().SetTimer(
		FlashTimerHandle,
		this,
		&UNSDamageFlashComponent::UpdateFlash,
		0.016f,
		true
	);
}

void UNSDamageFlashComponent::CancelFlash()
{
	CancelMaterialFlash();
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FlashTimerHandle);
	}
	StopFlash();
}

void UNSDamageFlashComponent::EnsureDynamicMaterials()
{
	if (OverlayMIDs.Num() > 0)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner || !FlashMaterial)
	{
		return;
	}

	TArray<UMeshComponent*> Meshes;
	Owner->GetComponents<UMeshComponent>(Meshes);

	for (UMeshComponent* Mesh : Meshes)
	{
		if (!IsValid(Mesh))
		{
			continue;
		}

		// StaticMesh(파괴오브젝트 본체)·SkeletalMesh(적 본체 + 무기)만 대상.
		if (!Mesh->IsA<UStaticMeshComponent>() && !Mesh->IsA<USkeletalMeshComponent>())
		{
			continue;
		}

		// FlashMaterial을 베이스로 오버레이 전용 DMI 생성 (슬롯 머티리얼과 독립)
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(FlashMaterial, this))
		{
			CachedMeshes.Add(Mesh);
			OverlayMIDs.Add(MID);
		}
	}
}

void UNSDamageFlashComponent::UpdateFlash()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 불투명도 페이드아웃
	const float Elapsed = World->GetTimeSeconds() - FlashStartTime;
	const float Alpha = FMath::Clamp(Elapsed / FlashDuration, 0.0f, 1.0f);
	const float CurrentOpacity = FMath::Lerp(PeakOpacity, 0.0f, Alpha); 

	for (UMaterialInstanceDynamic* MID : OverlayMIDs)
	{
		if (MID)
		{
			MID->SetScalarParameterValue(TEXT("Opacity"), CurrentOpacity);
		}
	}

	if (Alpha >= 1.0f)
	{
		World->GetTimerManager().ClearTimer(FlashTimerHandle);
		StopFlash();
	}
}

void UNSDamageFlashComponent::StopFlash()
{
	// 오버레이 해제 — Translucent 레이어 잔존 방지. DMI는 캐시 유지(재사용)
	for (UMeshComponent* Mesh : CachedMeshes)
	{
		if (IsValid(Mesh))
		{
			Mesh->SetOverlayMaterial(nullptr);
		}
	}
}

FLinearColor UNSDamageFlashComponent::ResolveFlashColor() const
{
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner());
	if (!ASI)
	{
		return FLinearColor::White;
	}

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC)
	{
		return FLinearColor::White;
	}

	const float MaxHealth = ASC->GetNumericAttribute(UNSBaseAttributeSet::GetMaxHealthAttribute());
	if (MaxHealth <= 0.0f)
	{
		return FLinearColor::White;
	}

	const float Health = ASC->GetNumericAttribute(UNSBaseAttributeSet::GetHealthAttribute());
	const float Ratio = FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f);

	if (Ratio >= HighHealthThreshold) return FLinearColor::White; 
	if (Ratio >= LowHealthThreshold)  return FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);
	return FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
}

void UNSDamageFlashComponent::SetMaterialFlashTargets(const TArray<UMaterialInstanceDynamic*>& InTargetMaterials)
{
	CancelMaterialFlash();
	MaterialFlashMIDs.Reset();

	for (UMaterialInstanceDynamic* MID : InTargetMaterials)
	{
		if (!IsValid(MID))
		{
			continue;
		}

		MaterialFlashMIDs.AddUnique(MID);

		MID->SetVectorParameterValue(MaterialFlashColorParameterName, MaterialFlashColor);
		MID->SetScalarParameterValue(MaterialFlashAmountParameterName, 0.0f);
	}
}

void UNSDamageFlashComponent::ClearMaterialFlashTargets()
{
	CancelMaterialFlash();
	MaterialFlashMIDs.Reset();
}

bool UNSDamageFlashComponent::TryPlayMaterialFlash(float Strength)
{
	UWorld* World = GetWorld();
	if (!World || MaterialFlashDuration <= 0.0f)
	{
		return false;
	}

	for (int32 Index = MaterialFlashMIDs.Num() - 1; Index >= 0; --Index)
	{
		if (!IsValid(MaterialFlashMIDs[Index]))
		{
			MaterialFlashMIDs.RemoveAtSwap(Index);
		}
	}

	if (MaterialFlashMIDs.Num() == 0)
	{
		return false;
	}

	ApplyMaterialFlashColor(ResolveMaterialFlashColor());

	ActiveMaterialFlashStrength = FMath::Clamp(Strength, 0.0f, 1.0f);
	MaterialFlashStartTime = World->GetTimeSeconds();

	const float InitialAmount =
		MaterialFlashPeakAmount *
		ActiveMaterialFlashStrength *
		EvaluateMaterialFlashCurve(0.0f);

	ApplyMaterialFlashAmount(InitialAmount);

	World->GetTimerManager().SetTimer(
		MaterialFlashTimerHandle,
		this,
		&ThisClass::UpdateMaterialFlash,
		0.016f,
		true);

	return true;
}

void UNSDamageFlashComponent::CancelMaterialFlash()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MaterialFlashTimerHandle);
	}

	ApplyMaterialFlashAmount(0.0f);
}

void UNSDamageFlashComponent::UpdateMaterialFlash()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float ElapsedTime = World->GetTimeSeconds() - MaterialFlashStartTime;
	const float NormalizedTime = FMath::Clamp(ElapsedTime / MaterialFlashDuration, 0.0f, 1.0f);

	const float CurrentAmount =
		MaterialFlashPeakAmount *
		ActiveMaterialFlashStrength *
		EvaluateMaterialFlashCurve(NormalizedTime);

	ApplyMaterialFlashAmount(CurrentAmount);

	if (NormalizedTime >= 1.0f)
	{
		World->GetTimerManager().ClearTimer(MaterialFlashTimerHandle);

		ApplyMaterialFlashAmount(0.0f);
	}
}

void UNSDamageFlashComponent::ApplyMaterialFlashAmount(float Amount)
{
	for (UMaterialInstanceDynamic* MID : MaterialFlashMIDs)
	{
		if (IsValid(MID))
		{
			MID->SetScalarParameterValue(MaterialFlashAmountParameterName, FMath::Max(Amount, 0.0f));
		}
	}
}

float UNSDamageFlashComponent::EvaluateMaterialFlashCurve(
	float NormalizedTime) const
{
	const float ClampedTime = FMath::Clamp(NormalizedTime, 0.0f, 1.0f);

	if (MaterialFlashCurve)
	{
		return FMath::Clamp(MaterialFlashCurve->GetFloatValue(ClampedTime), 0.0f, 1.0f);
	}

	return FMath::Square(1.0f - ClampedTime);
}

FLinearColor UNSDamageFlashComponent::ResolveMaterialFlashColor() const
{
	if (bUseHealthBasedMaterialFlashColor)
	{
		return ResolveFlashColor();
	}

	return MaterialFlashColor;
}

void UNSDamageFlashComponent::ApplyMaterialFlashColor(const FLinearColor& InFlashColor)
{
	for (int32 Index = MaterialFlashMIDs.Num() - 1; Index >= 0; --Index)
    {
        UMaterialInstanceDynamic* MID = MaterialFlashMIDs[Index].Get();

        if (!IsValid(MID))
        {
            MaterialFlashMIDs.RemoveAtSwap(Index);
            continue;
        }

        MID->SetVectorParameterValue(MaterialFlashColorParameterName, InFlashColor);
    }
}
