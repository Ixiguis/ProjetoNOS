// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "Items/ShooterPickup_Health.h"
#include "Player/ShooterCharacter.h"

#define LOCTEXT_NAMESPACE "ShooterGame.Pickup"

AShooterPickup_Health::AShooterPickup_Health()
{
	Health = 25;
	BoostHealth = false;
}

bool AShooterPickup_Health::CanBePickedUp(AShooterCharacter* TestPawn)
{
	if (!Super::CanBePickedUp(TestPawn))
	{
		return false;
	}

	if (BoostHealth && TestPawn->GetHealth() < TestPawn->GetMaxBoostedHealth())
	{
		return true;
	}
	if (!BoostHealth && TestPawn->GetHealth() < TestPawn->GetMaxHealth())
	{
		return true;
	}
	return false;
}

void AShooterPickup_Health::GivePickupTo(AShooterCharacter* Pawn)
{
	if (Pawn)
	{
		MissingHealth = Pawn->GiveHealth(Health, BoostHealth);
	}
}

#undef LOCTEXT_NAMESPACE
