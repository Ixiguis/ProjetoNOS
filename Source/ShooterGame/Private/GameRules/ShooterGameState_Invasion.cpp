// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#include "GameRules/ShooterGameState_Invasion.h"


AShooterGameState_Invasion::AShooterGameState_Invasion()
{
	CurrentWave = 0;
	RemainingMonsters = 0;
}

void AShooterGameState_Invasion::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterGameState_Invasion, CurrentWave);
	DOREPLIFETIME(AShooterGameState_Invasion, RemainingMonsters);
	DOREPLIFETIME(AShooterGameState_Invasion, InvasionRemainingTime);
	DOREPLIFETIME(AShooterGameState_Invasion, bWaveInProgress);
	DOREPLIFETIME(AShooterGameState_Invasion, TotalMonstersSpawned);
	DOREPLIFETIME(AShooterGameState_Invasion, TotalWaves);
	DOREPLIFETIME(AShooterGameState_Invasion, MaxMonsters);
}
