// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#pragma once

#include "GameRules/ShooterGameMode_Arena.h"
#include "GameRules/ShooterGameMode_TeamDeathMatch.h"
#include "ShooterGameMode_Alien.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API AShooterGameMode_Alien : public AShooterGameMode_TeamDeathMatch
{
	GENERATED_BODY()

public:
	AShooterGameMode_Alien();

	virtual void InitGameState() override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

protected:
	/** put everyone but the Alien in the same team */
	virtual int32 ChooseTeam(class AShooterPlayerState* ForPlayerState) const;

	/** assigns a new Alien, gives him the bonuses, and changes teams */
	void ChooseNewAlien();

	/** assign first Alien */
	virtual void StartMatch() override;

	virtual APawn* SpawnDefaultPawnFor(AController* NewPlayer, class AActor* StartSpot);
	
	/** check if the current Alien died; assign new Alien */
	virtual void Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType);

	/** if a given PlayerState is in this array, then it was already chosen as Alien at least once. Reset when all players are in this array. */
	TArray<APlayerState*> WasAlienOnce;

	APlayerState* CurrentAlien;

	UPROPERTY(config)
	float AlienSpeedBoost;
	
	/** The pawn class used by the Alien. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=GameMode)
	TSubclassOf<class APawn>  AlienPawnClass;

	AShooterCharacter* Alien;

	/** Spawns and returns an Alien, also causes InController to possess the new alien pawn. */
	AShooterCharacter* SpawnAndPossessAlienPawn(AController* InController);

	virtual void CheckMatchEnd() override;
	virtual class AActor* DetermineMatchWinner() override;

	/** Check if the current Alien left the match, have to set a new alien if so. */
	virtual void Logout( AController* Exiting ) override;

	FTimerHandle ChooseNewAlienHandle;
};

