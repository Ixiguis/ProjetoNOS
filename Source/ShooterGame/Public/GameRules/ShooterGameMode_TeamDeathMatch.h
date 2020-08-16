// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "GameRules/ShooterGameMode_Arena.h"
#include "ShooterGameMode_TeamDeathMatch.generated.h"

UCLASS()
class AShooterGameMode_TeamDeathMatch : public AShooterGameMode_Arena
{
	GENERATED_BODY()

public:
	AShooterGameMode_TeamDeathMatch();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/** setup team changes at player login */
	void PostLogin(APlayerController* NewPlayer) override;

	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, class AActor* StartSpot) override;

	/** initialize replicated game data */
	virtual void InitGameState() override;

	/** can players damage each other? */
	virtual bool CanDealDamage(class AShooterPlayerState* DamageInstigator, class AShooterPlayerState* DamagedPlayer) const override;

	/** called when a player changes teams, attempts to rebalance teams by sending bots to other teams */
	virtual void PlayerChangedToTeam(AShooterPlayerState* Player, uint8 NewTeam);

protected:

	/** best team */
	int32 WinnerTeam;

	virtual void CheckMatchEnd() override;

	/** pick team with least players in or random when it's equal */
	virtual int32 ChooseTeam(class AShooterPlayerState* ForPlayerState) const;
	
	/** check who won. Returns the focus actor. */
	virtual class AActor* DetermineMatchWinner() override;
	
	/** Returns the best player for the given team. */
	virtual class AShooterPlayerState* FindBestPlayer(uint8 TeamNum);

	/** check if PlayerState is a winner */
	virtual bool IsWinner(class AShooterPlayerState* PlayerState) const override;

	/** check team constraints */
	virtual bool IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const;

	/** initialization for bot after spawning */
	virtual void InitBot(AShooterAIController* AIC, int32 BotNum) override;	

	virtual void Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType) override;

	/** Loads Map_3teams, Map_4teams, etc, on receiving client */
	virtual void LoadAdditionalMaps(class AShooterPlayerController* PC);
};
