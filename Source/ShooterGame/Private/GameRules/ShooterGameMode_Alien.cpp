// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#include "GameRules/ShooterGameMode_Alien.h"
#include "GameRules/ShooterGameState.h"
#include "UObject/ConstructorHelpers.h"
#include "Player/ShooterPlayerState.h"
#include "Player/ShooterCharacter.h"

AShooterGameMode_Alien::AShooterGameMode_Alien()
{
	GameModeInfo.bAddToMenu = true;
	AlienSpeedBoost = 4.f;
	static ConstructorHelpers::FClassFinder<APawn> AlienPawnOb(TEXT("/Game/Blueprints/Pawns/AlienPawn"));
	AlienPawnClass = AlienPawnOb.Class;
	
	GameModeInfo.GameModeName = NSLOCTEXT("Game", "Alien", "Alien");
	GameModeInfo.MinTeams=2;
	GameModeInfo.MaxTeams=2;
}

void AShooterGameMode_Alien::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	//Alien game mode shouldn't be considered a team game, for the purpose of scoring
	NumTeams = 0;
}

void AShooterGameMode_Alien::InitGameState()
{
	Super::InitGameState();
	ShooterGameState->bPlayersAddTeamScore = false;
	ShooterGameState->bChangeToTeamColors = false;
}

void AShooterGameMode_Alien::ChooseNewAlien()
{
	TArray<APlayerState*> EligiblePlayers = GameState->PlayerArray;

	if (EligiblePlayers.Num() == 0)
	{
		//dedicated server with no players
		GetWorldTimerManager().SetTimer(ChooseNewAlienHandle, this, &AShooterGameMode_Alien::ChooseNewAlien, 1.f, false);
		return;
	} 
	for (APlayerState* Player : GameState->PlayerArray)
	{
		//remove players that already played as Alien
		if (WasAlienOnce.Find(Player) != INDEX_NONE)
		{
			EligiblePlayers.Remove(Player);
		}
	}
	//check if everyone already played as Alien
	if (EligiblePlayers.Num() == 0)
	{
		WasAlienOnce.Empty();
		EligiblePlayers = GameState->PlayerArray;
		//add the previous Alien to the array
		if (EligiblePlayers.Num() > 1)
		{
			WasAlienOnce.Add(CurrentAlien);
			EligiblePlayers.Remove(CurrentAlien);
		}
	}
	CurrentAlien = EligiblePlayers[FMath::RandHelper(EligiblePlayers.Num())];
	WasAlienOnce.Add(CurrentAlien);
	//update teams for all players
	for (APlayerState* Player : GameState->PlayerArray)
	{
		AShooterPlayerState* ShooterPS = Cast<AShooterPlayerState>(Player);
		if (ShooterPS)
		{
			const int32 NewTeam = ChooseTeam(ShooterPS);
			ShooterPS->ServerSetTeamNum(NewTeam);
		}
	}
	AController* Cont = Cast<AController>(CurrentAlien->GetOwner());
	if (Cont)
	{
		SpawnAndPossessAlienPawn(Cont);
	}
}

APawn* AShooterGameMode_Alien::SpawnDefaultPawnFor(AController* NewPlayer, class AActor* StartSpot)
{
	return Super::SpawnDefaultPawnFor(NewPlayer, StartSpot);
}

void AShooterGameMode_Alien::Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType)
{
	Super::Killed(Killer, KilledPlayer, KilledPawn, KillerWeaponClass, KillerDmgType);
	if (KilledPlayer->PlayerState == CurrentAlien)
	{
		GetWorldTimerManager().SetTimer(ChooseNewAlienHandle, this, &AShooterGameMode_Alien::ChooseNewAlien, FMath::Max(1.f, MinRespawnDelay/2), false);
	}
}

void AShooterGameMode_Alien::StartMatch()
{
	Super::StartMatch();
	GetWorldTimerManager().SetTimer(ChooseNewAlienHandle, this, &AShooterGameMode_Alien::ChooseNewAlien, FMath::Max(1.f, MinRespawnDelay/2), false);
}

int32 AShooterGameMode_Alien::ChooseTeam(class AShooterPlayerState* ForPlayerState) const
{
	if (ForPlayerState == CurrentAlien)
	{
		return 1;
	}
	return 0;
}

void AShooterGameMode_Alien::CheckMatchEnd()
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

AActor* AShooterGameMode_Alien::DetermineMatchWinner()
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

	AShooterPlayerState* WinnerPlayerState = (NumBestPlayers == 1) ? Cast<AShooterPlayerState>(ShooterGameState->PlayerArray[BestPlayer]) : NULL;
	if (WinnerPlayerState)
	{
		AController* C = Cast<AController>(WinnerPlayerState->GetOwner());
		ACharacter* Ch = C ? Cast<ACharacter>(C->GetCharacter()) : NULL;
		return Ch;
	}
	return NULL;
}

AShooterCharacter* AShooterGameMode_Alien::SpawnAndPossessAlienPawn(AController* InController)
{
	AActor* SpawnPoint = ChoosePlayerStart(InController);
	if (SpawnPoint == NULL)
	{
		UE_LOG(LogShooterGameMode, Warning, TEXT("Could not find a spawn point for the Alien (Alien game mode)."));
		GetWorldTimerManager().SetTimer(ChooseNewAlienHandle, this, &AShooterGameMode_Alien::ChooseNewAlien, 5.f, false);
		return NULL;
	}
	if (InController->GetPawn())
	{
		InController->GetPawn()->Destroy();
	}
	InController->UnPossess();
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnInfo.Instigator = GetInstigator();
	AShooterCharacter* SpawnedAlien = GetWorld()->SpawnActor<AShooterCharacter>(AlienPawnClass, SpawnPoint->GetActorLocation(), SpawnPoint->GetActorRotation(), SpawnInfo);
	if (SpawnedAlien)
	{
		InController->Possess(SpawnedAlien);
	}
	RestartPlayer(InController);
	return SpawnedAlien;
}

void AShooterGameMode_Alien::Logout(AController* Exiting)
{
	const bool bIsAlien = Exiting->PlayerState == CurrentAlien;
	Super::Logout(Exiting);
	if (bIsAlien)
	{
		GetWorldTimerManager().SetTimer(ChooseNewAlienHandle, this, &AShooterGameMode_Alien::ChooseNewAlien, 3.f, false);
	}
}
