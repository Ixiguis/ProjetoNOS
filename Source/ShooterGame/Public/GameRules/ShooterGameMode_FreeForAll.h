// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.
#pragma once
#include "ShooterGameMode_Arena.h"
#include "ShooterGameMode_FreeForAll.generated.h"

UCLASS()
class AShooterGameMode_FreeForAll : public AShooterGameMode_Arena
{
	GENERATED_BODY()

public:
	AShooterGameMode_FreeForAll();

protected:

	/** best player */
	UPROPERTY(transient)
	class AShooterPlayerState* WinnerPlayerState;
	
	/** check who won. Returns the focus actor. */
	virtual class AActor* DetermineMatchWinner() override;

	/** check if PlayerState is a winner */
	virtual bool IsWinner(class AShooterPlayerState* PlayerState) const override;
	
	/** check score limit when a player scores */
	virtual void Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType) override;

	virtual void CheckMatchEnd() override;
};
