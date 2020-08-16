// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.
#pragma once


#include "GameFramework/DamageType.h"
#include "Engine/Canvas.h"
#include "ShooterDamageType.generated.h"

// DamageType class that specifies an icon to display
UCLASS(const, Blueprintable, BlueprintType)
class UShooterDamageType : public UDamageType
{
	GENERATED_BODY()

public:
	UShooterDamageType();

	/** icon displayed in death messages log when killed with this weapon */
	UPROPERTY(EditDefaultsOnly, Category=HUD)
	FCanvasIcon KillIcon;

	/** force feedback effect to play on a player hit by this damage type */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	class UForceFeedbackEffect *HitForceFeedback;

	/** force feedback effect to play on a player killed by this damage type */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	class UForceFeedbackEffect *KilledForceFeedback;
};



