// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "Items/ShooterItem.h"
#include "ShooterItem_Ammo.generated.h"

UCLASS()
class AShooterItem_Ammo : public AShooterItem
{
	GENERATED_BODY()

public:
	AShooterItem_Ammo();

	void InitializeAmmo(TSubclassOf<class AShooterWeapon> InWeaponClass, int32 Amount = -1);

	/** @return the amount of ammo added. */
	int32 AddAmmo(int AddAmount);

	void UseAmmo(int UseAmount);

	int32 GetAmmoAmount() const;

	/** returns true if AmmoAmount equals the weapon's max ammo */
	bool IsMaxAmmo() const;

	/** which weapon this ammo serves */
	UPROPERTY(Replicated)
	TSubclassOf<class AShooterWeapon> WeaponClass;
	
private:
	/** current ammo */
	UPROPERTY(Replicated)
	int32 AmmoAmount;

};
