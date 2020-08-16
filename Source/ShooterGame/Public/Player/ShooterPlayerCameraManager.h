// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "Camera/PlayerCameraManager.h"
#include "ShooterPlayerCameraManager.generated.h"

UCLASS()
class AShooterPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	AShooterPlayerCameraManager();

	/** normal FOV */
	float NormalFOV;

	/** targeting FOV */
	float TargetingFOV;

	/** After updating camera, inform pawn to update 1p mesh to match camera's location&rotation */
	virtual void UpdateCamera(float DeltaTime) override;

protected:
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;
};
