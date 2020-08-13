// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#pragma once

#include "ShooterControllerInterface.generated.h"

/**
 * 
 */
UINTERFACE(MinimalAPI)
class UShooterControllerInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class IShooterControllerInterface
{
	GENERATED_IINTERFACE_BODY()

	virtual bool IsEnemyFor(AController* TestPC) const;
};