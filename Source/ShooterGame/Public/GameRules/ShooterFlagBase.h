// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#pragma once

#include "GameFramework/Actor.h"
#include "ShooterFlagBase.generated.h"

/**
 * Where the flag will spawn. Make sure to define owner team.
 * This is the actor that you will place on levels to determine flag location.
 */
UCLASS(ABSTRACT, Blueprintable)
class SHOOTERGAME_API AShooterFlagBase : public AActor
{
	GENERATED_BODY()

public:
		
	AShooterFlagBase();

	UPROPERTY(VisibleAnywhere, Category = Sprite)
	class UBillboardComponent* SpriteComponent;

	/** To which team this flag base belongs. */
	UPROPERTY(EditAnywhere, Category=Flag)
	uint8 TeamNumber;

	/** Flag class or blueprint, to be spawned. */
	UPROPERTY(EditAnywhere, Category=Flag)
	TSubclassOf<class AShooterFlag> FlagTemplate;

};
