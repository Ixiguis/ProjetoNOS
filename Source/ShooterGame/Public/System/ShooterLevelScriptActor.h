// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#pragma once

#include "Engine/LevelScriptActor.h"
#include "ShooterLevelScriptActor.generated.h"

/**
 * 
 */
UCLASS()
class AShooterLevelScriptActor : public ALevelScriptActor
{
	GENERATED_BODY()

public:
	AShooterLevelScriptActor();

	/** blueprint event: pickup disappears */
// 	UFUNCTION(BlueprintImplementableEvent, Category=Pickup)
// 	void PickupPickedUp(AShooterPickup* Pickup);
// 
// 	/** blueprint event: pickup appears */
// 	UFUNCTION(BlueprintImplementableEvent, Category=Pickup)
// 	void PickupRespawned(AShooterPickup* Pickup);
};
