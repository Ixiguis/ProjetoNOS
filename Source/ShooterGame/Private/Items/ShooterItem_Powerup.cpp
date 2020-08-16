// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "Items/ShooterItem_Powerup.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Player/ShooterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundCue.h"

AShooterItem_Powerup::AShooterItem_Powerup()
{
	bIsActive = false;
	Duration = 30.f;
}

float AShooterItem_Powerup::GetRemainingDuration()
{
	if (!bIsActive)
	{
		return 0.f;
	}
	return Duration - GetWorld()->GetTimeSeconds() + ActivatedTime;
}

bool AShooterItem_Powerup::Activate_Validate()
{
	return true;
}

void AShooterItem_Powerup::Activate_Implementation()
{
	ActivatedTime = GetWorld()->GetTimeSeconds();
	bIsActive = true;
	GetWorldTimerManager().SetTimer(TimerUpHandle, this, &AShooterItem_Powerup::TimerUp, Duration, false);	
	if (MyPawn)
	{
		MyPawn->AnyPowerupActive = true;
		if (DurationSound)
		{
			DurationAC = UGameplayStatics::SpawnSoundAttached(DurationSound, MyPawn->GetRootComponent());
			if (DurationAC)
			{
				AShooterItem_Powerup* DefaultPowerup = Cast<AShooterItem_Powerup>(GetClass()->GetDefaultObject());
				if (DefaultPowerup)
				{
					DurationAC->Play(DefaultPowerup->Duration - Duration);
				}
			}
		}
	}
	ActivatedEvent();
}

bool AShooterItem_Powerup::Deactivate_Validate()
{
	return true;
}

void AShooterItem_Powerup::Deactivate_Implementation()
{
	bIsActive = false;
	GetWorldTimerManager().ClearTimer(TimerUpHandle);
	DeactivatedEvent();
	if (MyPawn)
	{
		MyPawn->AnyPowerupActive = false;
		if (GetLocalRole() == ROLE_Authority)
		{
			MyPawn->RemoveItem(this);
		}
	}
	if (DurationAC)
	{
		DurationAC->Stop();
		DurationAC = NULL;
	}
}

void AShooterItem_Powerup::TimerUp_Implementation()
{
	Deactivate();
}

bool AShooterItem_Powerup::IsActive()
{
	return bIsActive;
}

void AShooterItem_Powerup::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AShooterItem_Powerup, Duration, COND_OwnerOnly);
}