// Copyright 2013-2014 Rampaging Blue Whale Games. All rights reserved.

#pragma once

#include "GameFramework/Info.h"
#include "ShooterMutator.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, Abstract, Meta = (ChildCanTick))
class SHOOTERGAME_API AShooterMutator : public AInfo
{
	GENERATED_BODY()
	
public:
	AShooterMutator() {}
	
};
