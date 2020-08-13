// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "ShooterControllerInterface.h"


UShooterControllerInterface::UShooterControllerInterface(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{

}

bool IShooterControllerInterface::IsEnemyFor(AController* TestPC) const
{
	return false;
}
