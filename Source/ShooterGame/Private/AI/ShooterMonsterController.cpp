// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#include "AI/ShooterMonsterController.h"


AShooterMonsterController::AShooterMonsterController()
{
	bWantsPlayerState = false;
}

bool AShooterMonsterController::IsEnemyFor(AController* TestPC) const
{
	//only human players and bots have PlayerStates, so if TestPC has a player state, then it's not a monster, therefore, enemy.
	return (TestPC && TestPC->PlayerState != NULL) ? true : false;
}


