// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "ShooterGameMode_FreeForAll.h"

AShooterGameMode_FreeForAll::AShooterGameMode_FreeForAll()
{
	bDelayedStart = true;
	GameModeInfo.bAddToMenu = true;
	GameModeInfo.GameModeName = NSLOCTEXT("Game", "Deathmatch", "Deathmatch");
}

AActor* AShooterGameMode_FreeForAll::DetermineMatchWinner()
{
	float BestScore = -MAX_FLT;
	int32 BestPlayer = -1;
	int32 NumBestPlayers = 0;

	for (int32 i = 0; i < ShooterGameState->PlayerArray.Num(); i++)
	{
		const float PlayerScore = ShooterGameState->PlayerArray[i]->GetScore();
		if (BestScore < PlayerScore)
		{
			BestScore = PlayerScore;
			BestPlayer = i;
			NumBestPlayers = 1;
		}
		else if (BestScore == PlayerScore)
		{
			NumBestPlayers++;
		}
	}

	WinnerPlayerState = (NumBestPlayers == 1) ? Cast<AShooterPlayerState>(ShooterGameState->PlayerArray[BestPlayer]) : NULL;
	if (WinnerPlayerState)
	{
		AController* C = Cast<AController>(WinnerPlayerState->GetOwner());
		ACharacter* Ch = C ? Cast<ACharacter>(C->GetCharacter()) : NULL;
		return Ch;
	}
	return NULL;
}

bool AShooterGameMode_FreeForAll::IsWinner(class AShooterPlayerState* PlayerState) const
{
	return PlayerState && PlayerState == WinnerPlayerState;
}

void AShooterGameMode_FreeForAll::Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType)
{
	Super::Killed(Killer, KilledPlayer, KilledPawn, KillerWeaponClass, KillerDmgType);
}

void AShooterGameMode_FreeForAll::CheckMatchEnd()
{
	if (ScoreLimit > 0)
	{
		for (APlayerState* Player : ShooterGameState->PlayerArray)
		{
			AShooterPlayerState* PS = CastChecked<AShooterPlayerState>(Player);
			if (PS->GetScore() >= ScoreLimit)
			{
				FinishMatch();
			}
		}
	}
}
