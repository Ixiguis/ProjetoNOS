// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#pragma once

#include "GameRules/ShooterGameState.h"
#include "ShooterGameState_Invasion.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API AShooterGameState_Invasion : public AShooterGameState
{
	GENERATED_BODY()

public:
	AShooterGameState_Invasion();

	/** number of teams in current game */
	UPROPERTY(Transient, Replicated)
	int32 RemainingMonsters;

	/** number of teams in current game */
	UPROPERTY(Transient, Replicated)
	int32 TotalMonstersSpawned;

	/** current (or upcoming) wave number */
	UPROPERTY(Transient, Replicated)
	uint8 CurrentWave;

	/** total number of waves */
	UPROPERTY(Transient, Replicated)
	uint8 TotalWaves;

	/** maximum number of monsters for current wave */
	UPROPERTY(Transient, Replicated)
	uint8 MaxMonsters;

	/** Invasion warm-up time remaining, or time remaining for current wave */
	UPROPERTY(Transient, Replicated)
	int32 InvasionRemainingTime;
	
	/** true: wave in progress, monsters are spawning. False: invasion warm-up */
	UPROPERTY(Transient, Replicated)
	bool bWaveInProgress;

};
