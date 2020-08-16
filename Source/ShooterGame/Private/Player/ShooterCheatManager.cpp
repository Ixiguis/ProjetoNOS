// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "Player/ShooterCheatManager.h"
#include "Player/ShooterPlayerState.h"
#include "Player/ShooterPlayerController.h"
#include "AI/ShooterAIController.h"
#include "GameRules/ShooterGameState.h"
#include "GameRules/ShooterGameMode.h"

UShooterCheatManager::UShooterCheatManager()
{
}

void UShooterCheatManager::ToggleInfiniteAmmo()
{
	AShooterPlayerController* MyPC = GetOuterAShooterPlayerController();

	MyPC->SetInfiniteAmmo(!MyPC->HasInfiniteAmmo());
	MyPC->ClientMessage(FString::Printf(TEXT("Infinite ammo: %s"), MyPC->HasInfiniteAmmo() ? TEXT("ENABLED") : TEXT("off")));
}

void UShooterCheatManager::ToggleInfiniteClip()
{
	AShooterPlayerController* MyPC = GetOuterAShooterPlayerController();

	MyPC->SetInfiniteClip(!MyPC->HasInfiniteClip());
	MyPC->ClientMessage(FString::Printf(TEXT("Infinite clip: %s"), MyPC->HasInfiniteClip() ? TEXT("ENABLED") : TEXT("off")));
}

void UShooterCheatManager::ToggleMatchTimer()
{
	AShooterPlayerController* MyPC = GetOuterAShooterPlayerController();

	AShooterGameState* const MyGameState = Cast<AShooterGameState>(MyPC->GetWorld()->GetGameState());
	if (MyGameState && MyGameState->GetLocalRole() == ROLE_Authority)
	{
		MyGameState->bTimerPaused = !MyGameState->bTimerPaused;
		MyPC->ClientMessage(FString::Printf(TEXT("Match timer: %s"), MyGameState->bTimerPaused ? TEXT("PAUSED") : TEXT("running")));
	}
}

void UShooterCheatManager::ForceMatchStart()
{
	AShooterPlayerController* const MyPC = GetOuterAShooterPlayerController();

	AShooterGameMode* const MyGame = MyPC->GetWorld()->GetAuthGameMode<AShooterGameMode>();
	if (MyGame && MyGame->GetMatchState() == MatchState::WaitingToStart)
	{
		MyGame->StartMatch();
	}
}

void UShooterCheatManager::ChangeTeam(int32 NewTeamNumber)
{
	AShooterPlayerController* MyPC = GetOuterAShooterPlayerController();

	AShooterPlayerState* MyPlayerState = MyPC->GetPlayerState<AShooterPlayerState>();
	if (MyPlayerState && MyPlayerState->GetLocalRole() == ROLE_Authority)
	{
		MyPlayerState->ServerSetTeamNum(NewTeamNumber);
		MyPC->ClientMessage(FString::Printf(TEXT("Team changed to: %d"), MyPlayerState->GetTeamNum()));
	}
}

void UShooterCheatManager::Cheat(const FString& Msg)
{
	GetOuterAShooterPlayerController()->ServerCheat(Msg.Left(128));
}

void UShooterCheatManager::SpawnBot()
{
	AShooterPlayerController* const MyPC = GetOuterAShooterPlayerController();
	APawn* const MyPawn = MyPC->GetPawn();
	AShooterGameMode* const MyGame = MyPC->GetWorld()->GetAuthGameMode<AShooterGameMode>();
	UWorld* World = MyPC->GetWorld();
	if (MyPawn && MyGame && World)
	{
		static int32 CheatBotNum = 50;
		AShooterAIController* AIC = MyGame->CreateBot(CheatBotNum++);
		MyGame->RestartPlayer(AIC);		
	}
}