// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "ShooterPickup_Armor.generated.h"

/**
 * A pickup object that replenishes character armor.
 */
UCLASS()
class AShooterPickup_Armor : public AShooterPickup
{
	GENERATED_BODY()

public:
	AShooterPickup_Armor();

	/** check if pawn can use this pickup */
	virtual bool CanBePickedUp(class AShooterCharacter* TestPawn) override;

protected:

	/** Amount of armor added */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Pickup)
	int32 ArmorAmount;

	/** how much armor this pickup restored */
	UPROPERTY(BlueprintReadOnly, Category = Pickup)
	int32 MissingArmor;

	/** give pickup */
	virtual void GivePickupTo(class AShooterCharacter* Pawn) override;
	
};
