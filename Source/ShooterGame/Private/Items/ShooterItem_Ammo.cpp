// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "ShooterItem_Ammo.h"

AShooterItem_Ammo::AShooterItem_Ammo()
{
}

void AShooterItem_Ammo::InitializeAmmo(TSubclassOf<AShooterWeapon> InWeaponClass, int32 Amount)
{
	WeaponClass = InWeaponClass;
	if (Amount <= -1)
	{
		AmmoAmount = WeaponClass.GetDefaultObject()->GetInitialAmmo();
	}
	else
	{
		AmmoAmount = Amount;
	}
}

int32 AShooterItem_Ammo::AddAmmo(int AddAmount)
{
	const int32 MissingAmmo = FMath::Max(0, WeaponClass.GetDefaultObject()->GetMaxAmmo() - AmmoAmount);
	AddAmount = FMath::Min(AddAmount, MissingAmmo);
	AmmoAmount += AddAmount;
	return AddAmount;
}

void AShooterItem_Ammo::UseAmmo(int UseAmount)
{
	AmmoAmount -= UseAmount;
	if (AmmoAmount < 0)
	{
		AmmoAmount = 0;
	}
}

bool AShooterItem_Ammo::IsMaxAmmo() const
{
	return AmmoAmount >= WeaponClass.GetDefaultObject()->GetMaxAmmo();
}

int32 AShooterItem_Ammo::GetAmmoAmount() const
{
	return AmmoAmount;
}

void AShooterItem_Ammo::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME_CONDITION(AShooterItem_Ammo, WeaponClass, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AShooterItem_Ammo, AmmoAmount, COND_OwnerOnly);
}
