// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "ShooterPickup_Weapon.generated.h"

UCLASS(Abstract, Blueprintable)
class AShooterPickup_Weapon : public AShooterPickup
{
	GENERATED_BODY()

public:
	AShooterPickup_Weapon();

	virtual void PostInitializeComponents() override;

	/** initial setup */
	virtual void BeginPlay() override;

	/** if > -1, gives this ammo amount, rather than the weapon's default initial ammo */
	UPROPERTY(EditAnywhere, Category=Pickup)
	int32 overrideAmmoAmount;

protected:
	
	/** Weapon to give */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=Pickup)
	TSubclassOf<class AShooterWeapon> WeaponClass;
	
	virtual void GivePickupTo(class AShooterCharacter* Pawn) override;
	
	/** if true, this pickup will never stay, even if GameMode->WeaponStay is true. */
	UPROPERTY(EditAnywhere, Category = Pickup)
	uint32 bNeverWeaponStay : 1;
};
