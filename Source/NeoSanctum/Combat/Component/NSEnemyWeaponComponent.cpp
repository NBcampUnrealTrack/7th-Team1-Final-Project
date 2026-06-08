// Copyright 2026 One Team. All rights reserved.


#include "NSEnemyWeaponComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbility.h"
#include "NeoSanctum/Character/Enemy/NSEnemyCharacterBase.h"
#include "NeoSanctum/Combat/Weapon/NSEnemyWeaponBase.h"
#include "NeoSanctum/Data/AI/NSEnemyData.h"


UNSEnemyWeaponComponent::UNSEnemyWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNSEnemyWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ANSEnemyCharacterBase* OwnerCharacter = Cast<ANSEnemyCharacterBase>(GetOwner()))
	{
		OwnerCharacter->OnEnemyDead.AddUObject(this, &UNSEnemyWeaponComponent::OnOwnerDead);
	}
}

void UNSEnemyWeaponComponent::EquipWeapon()
{
	ANSEnemyCharacterBase* Owner = Cast<ANSEnemyCharacterBase>(GetOwner());
	if (!Owner || !Owner->HasAuthority()) return;

	// 재장착할 때 대비용
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}
	
	UNSEnemyData* EnemyData = Owner->GetEnemyData();
	if (!EnemyData || !EnemyData->DefaultWeaponClass) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.Instigator = Owner;

	// 무기 액터 스폰
	CurrentWeapon = GetWorld()->SpawnActor<ANSEnemyWeaponBase>(EnemyData->DefaultWeaponClass, SpawnParams);
	if (!CurrentWeapon) return;

	const FWeaponConfig& Config = CurrentWeapon->GetWeaponConfig();

	// 무기 소켓에 부착
	CurrentWeapon->AttachToComponent(
		Owner->GetMesh(),
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		Config.EquipSocketName
	);

	// 무기 고유의 Transform 적용
	CurrentWeapon->SetActorRelativeTransform(Config.RelativeTransform);

	// 무기 전용 ABP 적용
	if (Config.AnimBlueprintClass)
	{
		Owner->GetMesh()->SetAnimInstanceClass(Config.AnimBlueprintClass);
	}

	// 무기 전용 GA 적용
	if (UAbilitySystemComponent* ASC = Owner->GetAbilitySystemComponent())
	{
		if (Config.WeaponAbility)
		{
			ASC->GiveAbility(FGameplayAbilitySpec(
				Config.WeaponAbility,
				1,
				Config.WeaponAbility.GetDefaultObject()->GetNetExecutionPolicy()
			));
		}
	}
}

void UNSEnemyWeaponComponent::UnEquipWeapon()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}
}


void UNSEnemyWeaponComponent::OnOwnerDead()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartDissolve();
	}
}
