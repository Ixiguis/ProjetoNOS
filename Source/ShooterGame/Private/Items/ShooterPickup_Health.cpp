// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "ShooterPickup_Health.h"

#define LOCTEXT_NAMESPACE "ShooterGame.Pickup"

AShooterPickup_Health::AShooterPickup_Health()
{
	Health = 25;
	Mana = 0.f;
	BoostHealth = false;
}

bool AShooterPickup_Health::CanBePickedUp(class AShooterCharacter* TestPawn)
{
	if (!Super::CanBePickedUp(TestPawn))
	{
		return false;
	}

	bool CanPickup = false;

	if (Mana > 0.f && TestPawn->Mana < TestPawn->MaxMana)
	{
		CanPickup = true;
	}
	else if (BoostHealth && TestPawn->Health < TestPawn->MaxBoostedHealth)
	{
		CanPickup = true;
	}
	else if (!BoostHealth && TestPawn->Health < TestPawn->GetMaxHealth())
	{
		CanPickup = true;
	}

	return CanPickup;
}

void AShooterPickup_Health::GivePickupTo(class AShooterCharacter* Pawn)
{
	if (Pawn)
	{
		if (!BoostHealth)
		{
			MissingHealth = FMath::Min(Health, Pawn->GetMaxHealth() - Pawn->Health);
			Pawn->Health += MissingHealth;
		}
		else
		{
			MissingHealth = FMath::Min(Health, Pawn->MaxBoostedHealth - Pawn->Health);
			Pawn->Health += MissingHealth;
		}
		if (Mana > 0.f)
		{
			Pawn->RestoreMana(Mana);
		}
	}
}

#undef LOCTEXT_NAMESPACE
