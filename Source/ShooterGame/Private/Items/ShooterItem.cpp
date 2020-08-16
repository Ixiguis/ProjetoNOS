// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "Items/ShooterItem.h"
#include "Player/ShooterCharacter.h"
#include "Items/ShooterPickup.h"
#include "Items/ShooterItem_Powerup.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "ShooterGame.Item"

AShooterItem::AShooterItem()
{
	SetCanBeDamaged(false);
	
	ItemName = LOCTEXT("UndefinedItem", "SomeItem");
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
}

void AShooterItem::SetOwningPawn(AShooterCharacter* NewOwner)
{
	if (MyPawn != NewOwner)
	{
		MyPawn = NewOwner;
		SetInstigator(NewOwner);
		// net owner for RPC calls
		SetOwner(NewOwner);
	}
}

void AShooterItem::OnLeaveInventory_Implementation()
{
	AShooterItem_Powerup* Powerup = Cast<AShooterItem_Powerup>(this);
	if (Powerup && Powerup->IsActive())
	{
		Powerup->Deactivate();
	}
}

void AShooterItem::EquipItem()
{
}

void AShooterItem::OnRep_MyPawn()
{
	if (MyPawn)
	{
		OnEnterInventory();
	}
	else
	{
		OnLeaveInventory();
	}
}

void AShooterItem::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterItem, MyPawn);
}


#undef LOCTEXT_NAMESPACE
