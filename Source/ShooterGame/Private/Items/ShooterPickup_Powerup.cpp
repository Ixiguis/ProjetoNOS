// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "Items/ShooterPickup_Powerup.h"
#include "Items/ShooterItem_Powerup.h"
#include "Player/ShooterCharacter.h"

#define LOCTEXT_NAMESPACE "ShooterGame.Pickup"

AShooterPickup_Powerup::AShooterPickup_Powerup()
{
	overrideDuration = 0.f;
	SpawnAtGameStart = false;
}

bool AShooterPickup_Powerup::CanBePickedUp(AShooterCharacter* TestPawn)
{
	if (TestPawn && TestPawn->AnyPowerupActive)
	{
		return false;
	}
	return Super::CanBePickedUp(TestPawn);
}

void AShooterPickup_Powerup::GivePickupTo(AShooterCharacter* Pawn)
{
	if (Pawn && GetLocalRole() == ROLE_Authority)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AShooterItem_Powerup* NewPowerup = GetWorld()->SpawnActor<AShooterItem_Powerup>(PowerupClass, SpawnInfo);
		if (overrideDuration > 0.f)
		{
			NewPowerup->Duration = overrideDuration;
		}
		Pawn->AddItem(NewPowerup);
		NewPowerup->Activate();
	}
}

#undef LOCTEXT_NAMESPACE
