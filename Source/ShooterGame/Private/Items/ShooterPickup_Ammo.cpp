// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "ShooterPickup_Ammo.h"
#include "ShooterWeapon.h"

#define LOCTEXT_NAMESPACE "ShooterGame.Pickup"

AShooterPickup_Ammo::AShooterPickup_Ammo()
{
	overrideAmmoAmount = -1;
}

void AShooterPickup_Ammo::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

bool AShooterPickup_Ammo::CanBePickedUp(class AShooterCharacter* TestPawn)
{
	if (TestPawn && !TestPawn->CanPickupAmmo(WeaponClass))
	{
		return false;
	}
	return Super::CanBePickedUp(TestPawn);
}

void AShooterPickup_Ammo::GivePickupTo(class AShooterCharacter* Pawn)
{
	const int32 AmmoAmount = overrideAmmoAmount > -1 ? overrideAmmoAmount : Cast<AShooterWeapon>(WeaponClass->GetDefaultObject())->GetInitialAmmo();
	if (Pawn)
	{
		MissingAmmo = Pawn->GiveAmmo(WeaponClass, AmmoAmount);
	}
}

#undef LOCTEXT_NAMESPACE
