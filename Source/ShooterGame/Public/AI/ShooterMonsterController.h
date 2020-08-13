// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#pragma once

#include "AI/ShooterAIController.h"
#include "ShooterMonsterController.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API AShooterMonsterController : public AShooterAIController
{
	GENERATED_BODY()

public:
	AShooterMonsterController();

	// Begin IShooterControllerInterface
	virtual bool IsEnemyFor(AController* TestPC) const;
	// End IShooterControllerInterface
	
	
};
