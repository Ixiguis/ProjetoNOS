// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "Items/ShooterPickup.h"
#include "ShooterPickup_Health.generated.h"

// A pickup object that replenishes character health
UCLASS(Abstract, Blueprintable)
class AShooterPickup_Health : public AShooterPickup
{
	GENERATED_BODY()

public:
	AShooterPickup_Health();

	/** check if pawn can use this pickup */
	virtual bool CanBePickedUp(class AShooterCharacter* TestPawn) override;

protected:

	/** how much health does it give? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Pickup)
	float Health;
	
	/** if true, health restored limit is Pawn->MaxBoostedHealth, rather than GetMaxHealth() */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Pickup)
	bool BoostHealth;
	
	/** how much health this pickup restored */
	UPROPERTY(BlueprintReadOnly, Category = Pickup)
	float MissingHealth;

	/** give pickup */
	virtual void GivePickupTo(class AShooterCharacter* Pawn) override;
	
};
