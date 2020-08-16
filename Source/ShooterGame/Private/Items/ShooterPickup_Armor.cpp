// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "Items/ShooterPickup_Armor.h"
#include "Player/ShooterCharacter.h"

#define LOCTEXT_NAMESPACE "ShooterGame.Pickup"

AShooterPickup_Armor::AShooterPickup_Armor()
{
	ArmorAmount = 50;
	RespawnTime = 30.f;
}

bool AShooterPickup_Armor::CanBePickedUp(AShooterCharacter* TestPawn)
{
	if (!Super::CanBePickedUp(TestPawn))
	{
		return false;
	}

	bool CanPickup = false;

	if (ArmorAmount > 0 && TestPawn->GetArmor() < TestPawn->GetMaxArmor() )
	{
		CanPickup = true;
	}

	return CanPickup;
}

void AShooterPickup_Armor::GivePickupTo(AShooterCharacter* Pawn)
{
	if (Pawn)
	{
		if (ArmorAmount > 0)
		{
			MissingArmor = FMath::RoundToInt(Pawn->GiveArmor(ArmorAmount));
		}
	}
}

#undef LOCTEXT_NAMESPACE
