// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "Items/ShooterPickup_Weapon.h"
#include "Weapons/ShooterWeapon.h"
#include "FunctionLibraries/ShooterBlueprintLibrary.h"
#include "GameRules/ShooterGameMode.h"
#include "Player/ShooterCharacter.h"

#define LOCTEXT_NAMESPACE "ShooterGame.Pickup"

AShooterPickup_Weapon::AShooterPickup_Weapon()
{
	overrideAmmoAmount = -1;
	bNeverWeaponStay = false;
}

void AShooterPickup_Weapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

/*
	AShooterWeapon* DefWeap = WeaponClass != NULL && PickupIcon.Texture == NULL ? Cast<AShooterWeapon>(WeaponClass->GetDefaultObject()) : NULL;
	if (DefWeap)
	{
		PickupIcon = DefWeap->WeaponIcon;
	}*/
}

void AShooterPickup_Weapon::BeginPlay()
{
	AShooterGameMode* GameMode = GetWorld()->GetAuthGameMode<AShooterGameMode>();
	if (GameMode)
	{
		PickupStay = GameMode->WeaponStay;
	}
	if (bNeverWeaponStay)
	{
		PickupStay = false;
	}
	Super::BeginPlay();
}

void AShooterPickup_Weapon::GivePickupTo(AShooterCharacter* Pawn)
{
	const int32 AmmoAmount = overrideAmmoAmount > -1? overrideAmmoAmount : Cast<AShooterWeapon>(WeaponClass->GetDefaultObject())->GetInitialAmmo();
	if (Pawn && GetLocalRole() == ROLE_Authority)
	{
		AShooterWeapon* PawnWeapon = Pawn->FindWeapon(WeaponClass);
		// if pawn already has the weapon, just give ammo
		if (PawnWeapon)
		{
			Pawn->GiveAmmo(WeaponClass, AmmoAmount);
		}
		else
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			AShooterWeapon* NewWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, SpawnInfo);

			Pawn->AddWeapon(NewWeapon, AmmoAmount);

			bool ShouldEquip = true;
			AShooterWeapon* CurrentWeapon = Pawn->GetWeapon();
			if (CurrentWeapon != NULL)
			{
				if (CurrentWeapon->GetCurrentState() != EWeaponState::Idle ||
					Pawn->WeaponPriority.Find(WeaponClass) > Pawn->WeaponPriority.Find(CurrentWeapon->GetClass()) ||
					Pawn->GetCurrentAmmo(WeaponClass) == 0)// ||
					//UShooterBlueprintLibrary::AnyEnemyWithin(Pawn, 700.f) )
				{
					ShouldEquip = false;
				}
			}
			if (ShouldEquip)
			{
				Pawn->EquipWeapon(NewWeapon);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
