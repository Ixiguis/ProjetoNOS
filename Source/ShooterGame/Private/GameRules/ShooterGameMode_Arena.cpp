// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "ShooterBlueprintLibrary.h"
#include "ShooterGameMode_Arena.h"

AShooterGameMode_Arena::AShooterGameMode_Arena()
{
	SndStartMatchAnnouncer = ConstructorHelpers::FObjectFinder<USoundCue>(TEXT("SoundCue'/Game/Sounds/Announcer/BARBARIAN_BEGIN_MATCH_Cue.BARBARIAN_BEGIN_MATCH_Cue'")).Object;
}

void AShooterGameMode_Arena::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

}

void AShooterGameMode_Arena::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	ShooterGameState->RemainingTime = RoundTime;
}

void AShooterGameMode_Arena::StartMatch()
{
	Super::StartMatch();

	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		AShooterPlayerController* PC = Cast<AShooterPlayerController>(It->Get());
		if (PC)
		{
			PC->ClientPlaySound(SndStartMatchAnnouncer);
		}
	}
}

void AShooterGameMode_Arena::DefaultTimer()
{
	Super::DefaultTimer();
	
	// don't update timers for Play In Editor mode, it's not real match
	if (GetWorld()->IsPlayInEditor())
	{
		// start match if necessary.
		if (GetMatchState() == MatchState::WaitingToStart)
		{
			StartMatch();
		}
		//return;
	}

	if (ShooterGameState->RemainingTime > 0 && !ShooterGameState->bTimerPaused)
	{
		ShooterGameState->RemainingTime--;
		
		if (ShooterGameState->RemainingTime <= 0)
		{

			if (GetMatchState() == MatchState::WaitingPostMatch)
			{
				RestartGame();
			}

			else if (GetMatchState() == MatchState::InProgress && !bUnlimitedRoundTime)
			{
				FinishMatch();

				// Send end round events
				AShooterGameState* const MyGameState = Cast<AShooterGameState>(GameState);
				
				for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
				{
					AShooterPlayerController* PlayerController = Cast<AShooterPlayerController>(It->Get());
					
					if (PlayerController && MyGameState)
					{
						AShooterPlayerState* PlayerState = (It->Get())->GetPlayerState<AShooterPlayerState>();
						const bool bIsWinner = IsWinner(PlayerState);
					
						PlayerController->ClientSendRoundEndEvent(bIsWinner, MyGameState->ElapsedTime);
					}
				}
			}
			else if (GetMatchState() == MatchState::WaitingToStart)
			{
				StartMatch();
			}
		}
	}
}

void AShooterGameMode_Arena::Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType)
{
	Super::Killed(Killer, KilledPlayer, KilledPawn, KillerWeaponClass, KillerDmgType);
	CheckMatchEnd();
}

APlayerStart* AShooterGameMode_Arena::GetBestSpawnPoint(const TArray<APlayerStart*>& AvailableSpawns, AController* Player) const
{
	if (AvailableSpawns.Num() == 0)
	{
		return NULL;
	}

	TArray<APlayerStart*> BestSpawnPoints;
	//find a spawn point that isn't close to other players
	const float MinDistToOthers = 2000.f;
	for (int32 i = 0; i < AvailableSpawns.Num(); i++)
	{
		const FVector SpawnPointLocation = AvailableSpawns[i]->GetActorLocation();
		const bool AnyEnemyNearby = UShooterBlueprintLibrary::AnyEnemyWithinLocation(Player, MinDistToOthers, SpawnPointLocation, false);
		if (!AnyEnemyNearby)
		{
			BestSpawnPoints.Add(AvailableSpawns[i]);
		}
	}

	if (BestSpawnPoints.Num() == 0)
	{
		//all spawns points have enemies within MinDistToOthers, determine which is the farthest away from enemies
		float FarthestDist = 0.f;
		APlayerStart* FarthestPlayerStart = NULL;
		IShooterControllerInterface* MyControllerInterface = dynamic_cast<IShooterControllerInterface*>(Player);
		if (MyControllerInterface)
		{
			for (int32 i = 0; i < AvailableSpawns.Num(); i++)
			{
				const FVector SpawnPointLocation = AvailableSpawns[i]->GetActorLocation();
				for (TActorIterator<AShooterCharacter> It(GetWorld()); It; ++It)
				{
					AShooterCharacter* OtherPawn = *It;
					if (OtherPawn && !OtherPawn->GetTearOff() /* (not dead) */ && MyControllerInterface->IsEnemyFor(OtherPawn->GetController()))
					{
						const float DistToPlayer = (SpawnPointLocation - OtherPawn->GetActorLocation()).SizeSquared();
						if (FarthestDist < DistToPlayer)
						{
							FarthestDist = DistToPlayer;
							FarthestPlayerStart = AvailableSpawns[i];
						}
					}
				}
			}
			BestSpawnPoints.Add(FarthestPlayerStart);
		}
	}

	if (BestSpawnPoints.Num() > 0)
	{
		const int32 RandomBestIndex = FMath::RandHelper(BestSpawnPoints.Num());
		return BestSpawnPoints[RandomBestIndex];
	}
	const int32 RandomAvailableIndex = FMath::RandHelper(AvailableSpawns.Num());
	return AvailableSpawns[RandomAvailableIndex];
}

void AShooterGameMode_Arena::CheckMatchEnd()
{
	//nothing to do here, subclasses should specify winning condition
}
