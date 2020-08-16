// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "ShooterGameDelegates.h"


class FShooterGameModule : public FDefaultGameModuleImpl
{
	virtual void StartupModule() override
	{
		InitializeShooterGameDelegates();
	}

};

IMPLEMENT_PRIMARY_GAME_MODULE(FShooterGameModule, ShooterGame, "ShooterGame");

