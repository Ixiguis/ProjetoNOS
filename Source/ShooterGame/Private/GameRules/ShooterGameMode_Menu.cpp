// Copyright 2013-2014 Rampaging Blue Whale Games. All rights reserved.

#include "GameRules/ShooterGameMode_Menu.h"
#include "GameRules/ShooterGameSession.h"
#include "Player/ShooterPlayerController_Menu.h"


AShooterGameMode_Menu::AShooterGameMode_Menu()
{
	PlayerControllerClass = AShooterPlayerController_Menu::StaticClass();
}

void AShooterGameMode_Menu::RestartPlayer(class AController* NewPlayer)
{
	// don't restart
}

/** Returns game session class to use */
TSubclassOf<AGameSession> AShooterGameMode_Menu::GetGameSessionClass() const
{
	return AShooterGameSession::StaticClass();
}

TArray<struct FGameModeInfo> AShooterGameMode_Menu::GetGameModeList() const
{
	return GameModeList;
}
