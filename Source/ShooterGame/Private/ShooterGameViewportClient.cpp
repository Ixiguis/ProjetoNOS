// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ShooterGameViewportClient.h"
#include "Player/ShooterLocalPlayer.h"
#include "ShooterGameUserSettings.h"
#include "FunctionLibraries/ShooterBlueprintLibrary.h"

UShooterGameViewportClient::UShooterGameViewportClient()
{
	//SetSuppressTransitionMessage(true);
}

void UShooterGameViewportClient::NotifyPlayerAdded(int32 PlayerIndex, ULocalPlayer* AddedPlayer)
{
	Super::NotifyPlayerAdded(PlayerIndex, AddedPlayer);

 	UShooterLocalPlayer* const ShooterLP = Cast<UShooterLocalPlayer>(AddedPlayer);
 	if (ShooterLP)
 	{
 		ShooterLP->LoadPersistentUser();
 	}
}

bool UShooterGameViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
	FInputKeyEventArgs NewArgs = EventArgs;
	if (NewArgs.Key.IsGamepadKey() && UShooterBlueprintLibrary::GetGameUserSettings()->bFirstPlayerOnKeyboard)
	{
		NewArgs.ControllerId++;
	}
	return Super::InputKey(NewArgs);
}

bool UShooterGameViewportClient::InputAxis(FViewport* InViewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	if (Key.IsGamepadKey() && UShooterBlueprintLibrary::GetGameUserSettings()->bFirstPlayerOnKeyboard)
	{
		ControllerId++;
	}
	return Super::InputAxis(InViewport, ControllerId, Key, Delta, DeltaTime, NumSamples , bGamepad );
}
