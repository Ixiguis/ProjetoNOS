// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "GameRules/ShooterGameMode.h"
#include "ShooterGameMode_Arena.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class AShooterGameMode_Arena : public AShooterGameMode
{
	GENERATED_BODY()

public:
	AShooterGameMode_Arena();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	virtual void HandleMatchHasStarted() override;

	/** starts new match */
	virtual void StartMatch() override;

	/** check killing spree stuff */
	virtual void Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType) override;

	/** try to pick a spawn point that is away from enemies */
	virtual APlayerStart* GetBestSpawnPoint(const TArray<APlayerStart*>& AvailableSpawns, AController* Player) const;

protected:
	/** update remaining time */
	virtual void DefaultTimer() override;
	
	UPROPERTY(EditDefaultsOnly, Category="GameMode")
	class USoundCue* SndStartMatchAnnouncer;
	
	/** checks if the match has ended due to score limit. Calls FinishMatch() if match ended. */
	virtual void CheckMatchEnd();

};
