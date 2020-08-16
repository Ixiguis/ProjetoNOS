// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "Items/ShooterPickup.h"
#include "ShooterPickup_Powerup.generated.h"

/**
 * 
 */
UCLASS()
class AShooterPickup_Powerup : public AShooterPickup
{
	GENERATED_BODY()

public:
	AShooterPickup_Powerup();

	/** check if pawn can use this pickup */
	virtual bool CanBePickedUp(class AShooterCharacter* TestPawn) override;

	/** if > 0, uses this duration for the powerup, rather than the powerup's default */
	UPROPERTY(EditAnywhere, Category = Pickup)
	float overrideDuration;

protected:
	
	/** Powerup to give */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Pickup)
	TSubclassOf<class AShooterItem_Powerup> PowerupClass;

	virtual void GivePickupTo(class AShooterCharacter* Pawn) override;
	
};
