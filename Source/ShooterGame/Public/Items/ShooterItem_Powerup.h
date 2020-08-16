// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "Items/ShooterItem.h"
#include "ShooterItem_Powerup.generated.h"

/**
 * 
 */
UCLASS()
class AShooterItem_Powerup : public AShooterItem
{
	GENERATED_BODY()

public:
	AShooterItem_Powerup();

	/** item's duration when activated. Replicated only to owner. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category=Powerup)
	float Duration;
	
	/** Sets TimerUp() timer, and plays DurationSound. Then calls ActivatedEvent.
	*	Called by the server, broadcasted to clients. */
	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, WithValidation, Category=Powerup)
	void Activate();

	/** Stops duration sound, then calls DeactivatedEvent.
	*	Called by the server, broadcasted to clients. */
	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, WithValidation, Category=Powerup)
	void Deactivate();

	UFUNCTION(BlueprintCallable,  Category=Powerup)
	float GetRemainingDuration();

	bool IsActive();

protected:
	
	/** Simply calls Deactivate(), unless overridden in Blueprint. */
	UFUNCTION(BlueprintNativeEvent, Category=Powerup)
	void TimerUp();

	/** Blueprint event to specify powerup functionality. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category=Powerup)
	void ActivatedEvent();

	/** Blueprint event to specify powerup functionality end. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category=Powerup)
	void DeactivatedEvent();
	
	/** Game time when this was activated */
	float ActivatedTime;

	UPROPERTY(BlueprintReadOnly, Category=Powerup)
	uint32 bIsActive : 1;

	/** sound attached to character when activated, stops if character dies */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Powerup)
	USoundCue* DurationSound;

	UAudioComponent* DurationAC;

	FTimerHandle TimerUpHandle;
};
