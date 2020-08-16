// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "Items/ShooterPickup.h"
#include "ShooterPickup_Ammo.generated.h"

/**
 * 
 */
UCLASS()
class AShooterPickup_Ammo : public AShooterPickup
{
	GENERATED_BODY()

public:
	AShooterPickup_Ammo();

	virtual void PostInitializeComponents() override;

	/** check if pawn can use this pickup */
	virtual bool CanBePickedUp(class AShooterCharacter* TestPawn) override;
	
	/** if > -1, gives this ammo amount, rather than the weapon's default initial ammo */
	UPROPERTY(EditAnywhere, Category=Pickup)
	int32 overrideAmmoAmount;

protected:
		
	/** Which weapon gets ammo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Pickup)
	TSubclassOf<class AShooterWeapon> WeaponClass;
	
	virtual void GivePickupTo(class AShooterCharacter* Pawn) override;
	
	UPROPERTY(BlueprintReadOnly, Category=Pickup)
	int32 MissingAmmo;
};
