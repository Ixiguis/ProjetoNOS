// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once
#include "ShooterSpectatorPawn.generated.h"


UCLASS(config = Game, Blueprintable, BlueprintType)
class AShooterSpectatorPawn : public ASpectatorPawn
{
	GENERATED_BODY()

public:
	AShooterSpectatorPawn();

	// Begin ASpectatorPawn overrides
	/** Overridden to implement Key Bindings the match the player controls */
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// End Pawn overrides
	
	void SpectateNextCharacter();
	void SpectatePreviousCharacter();
	void SpectateFreeCamera();
	
	/** sets spectator focus.
	*	@param bViewTargetIsDead if true, then NewViewTarget is a character and is dead. Must be passed as a parameter from the server 
	*		because AShooterCharacter::Health is not replicated in time for the client to check if NewViewTarget->IsAlive(). */
	void SetViewTarget(AActor* Target, bool bViewTargetIsDead = false);

	/** if false, pressing left/right mouse button will have no effect; otherwise, will spectate next/previous alive character */
	bool bAllowSwitchFocus;

	/** if false, free camera will not be allowed. */
	bool bAllowFreeCam;

protected:
	USpringArmComponent* CameraBoom;
	UCameraComponent* Camera;
	
	// Frame rate linked look
	void LookUpAtRate(float Val);
	void TurnAtRate(float Rate);

	void IncreaseZoom();
	void DecreaseZoom();

	/** returns true if the camera is on free mode, otherwise it's focusing on something */
	inline bool IsFreeCam() const { return CurrentTarget == NULL; }

	/** current view target */
	AActor* CurrentTarget;

	/** returns the first alive ShooterCharacter found in the world's pawn iterator */
	AShooterCharacter* GetFirstCharacter() const;
	
	/** returns the last alive ShooterCharacter found in the world's pawn iterator */
	AShooterCharacter* GetLastCharacter() const;

	float DesiredCameraDistance;

	virtual void Tick(float DeltaSeconds) override;
};
