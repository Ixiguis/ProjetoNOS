// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "ShooterPickup_Armor.h"

#define LOCTEXT_NAMESPACE "ShooterGame.Pickup"

AShooterPickup_Armor::AShooterPickup_Armor()
{
	ArmorAmount = 50;
	RespawnTime = 30.f;
}

bool AShooterPickup_Armor::CanBePickedUp(class AShooterCharacter* TestPawn)
{
	if (!Super::CanBePickedUp(TestPawn))
	{
		return false;
	}

	bool CanPickup = false;

	if (ArmorAmount > 0 && TestPawn->Armor < TestPawn->GetMaxArmor() )
	{
		CanPickup = true;
	}

	return CanPickup;
}

void AShooterPickup_Armor::GivePickupTo(class AShooterCharacter* Pawn)
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
