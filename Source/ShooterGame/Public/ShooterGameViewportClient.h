// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ShooterTypes.h"
#include "Engine/GameViewportClient.h"
#include "ShooterGameViewportClient.generated.h"


UCLASS(Within=Engine, transient, config=Engine)
class UShooterGameViewportClient : public UGameViewportClient
{
	GENERATED_BODY()

public:
	
	UShooterGameViewportClient();
	// FViewportClient interface.
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
	virtual bool InputAxis(FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples = 1, bool bGamepad = false) override;
	// End FViewportClient interface.

 	// start UGameViewportClient interface
 	void NotifyPlayerAdded( int32 PlayerIndex, ULocalPlayer* AddedPlayer ) override;
};