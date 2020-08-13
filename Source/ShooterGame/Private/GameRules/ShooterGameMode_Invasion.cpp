// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#include "GameRules/ShooterGameMode_Invasion.h"


AShooterGameMode_Invasion::AShooterGameMode_Invasion()
{
	GameStateClass = AShooterGameState_Invasion::StaticClass();
	GameModeInfo.GameModeName = NSLOCTEXT("Game", "Invasion", "Invasion");
	GameModeInfo.MinTeams = 2;
	GameModeInfo.MaxTeams = 2;
	GameModeInfo.bAddToMenu = true;
}

void AShooterGameMode_Invasion::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	NumTeams = 1;
	Waves = UShooterPersistentUser::LoadPersistentUser2("SaveSlot1", 0)->InvasionWaves;
	RoundTime = 0;
	bUnlimitedRoundTime = true;
	ScoreLimit = 0;
}


void AShooterGameMode_Invasion::InitGameState()
{
	Super::InitGameState();
	InvasionGameState = CastChecked<AShooterGameState_Invasion>(GameState);
	InvasionGameState->bChangeToTeamColors = false;
	InvasionGameState->CurrentWave = 0;
	InvasionGameState->TotalWaves = Waves.Num();
}

void AShooterGameMode_Invasion::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	// assign to humans team
	AShooterPlayerState* NewPlayerState = CastChecked<AShooterPlayerState>(NewPlayer->PlayerState);
	NewPlayerState->ServerSetTeamNum(0);
	NewPlayerState->SetLives(3);
}

void AShooterGameMode_Invasion::InitBot(AShooterAIController* AIC, int32 BotNum)
{
	Super::InitBot(AIC, BotNum);

	if (AIC)
	{
		AShooterPlayerState* BotPlayerState = CastChecked<AShooterPlayerState>(AIC->PlayerState);
		BotPlayerState->ServerSetTeamNum(0);
		BotPlayerState->SetLives(3);
	}
}

void AShooterGameMode_Invasion::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	InvasionGameState->InvasionRemainingTime = Waves[InvasionGameState->CurrentWave].WarmupTime;
	InvasionGameState->bWaveInProgress = false;
}

bool AShooterGameMode_Invasion::CanDealDamage(class AShooterPlayerState* DamageInstigator, class AShooterPlayerState* DamagedPlayer) const
{
	//monsters don't have a Player State
	if (DamagedPlayer == NULL || DamageInstigator == NULL)
	{
		return true;
	}
	return Super::CanDealDamage(DamageInstigator, DamagedPlayer);
}

void AShooterGameMode_Invasion::DefaultTimer()
{
	Super::DefaultTimer();

	if (GetMatchState() == MatchState::InProgress)
	{
		InvasionGameState->InvasionRemainingTime--;
		if (InvasionGameState->InvasionRemainingTime <= 0)
		{
			InvasionGameState->InvasionRemainingTime = 0;
			if (InvasionGameState->bWaveInProgress)
			{
				//time over, players win this wave
				StopWave();
			}
			else
			{
				StartWave();
			}
		}
	}
}

void AShooterGameMode_Invasion::StartWave()
{
	InvasionGameState->bWaveInProgress = true;
	InvasionGameState->InvasionRemainingTime = GetCurrWave().WaveDuration;
	InvasionGameState->MaxMonsters = GetCurrWave().MaxMonsters;

	//MonstersSpawnRate is in minutes, convert it to seconds to use on the Timer
	const float SpawnRate = 60.f / GetCurrWave().MonstersSpawnRate;
	GetWorldTimerManager().SetTimer(SpawnMonsterHandle, this, &AShooterGameMode_Invasion::SpawnMonster, SpawnRate, true);
}

void AShooterGameMode_Invasion::StopWave()
{
	InvasionGameState->bWaveInProgress = false;
	InvasionGameState->InvasionRemainingTime = GetCurrWave().WarmupTime;
	InvasionGameState->TotalMonstersSpawned = 0;
	InvasionGameState->CurrentWave++;

	//check if previous wave was the last one
	CheckMatchEnd();

	GetWorldTimerManager().ClearTimer(SpawnMonsterHandle);

	//kill all monsters alive, if any
	for (TActorIterator<AShooterCharacter> It(GetWorld()); It; ++It)
	{
		AShooterCharacter* TestPawn = *It;
		if (TestPawn)
		{
			AShooterMonsterController* MonsterController = Cast<AShooterMonsterController>(TestPawn->GetController());
			if (MonsterController)
			{
				UGameplayStatics::ApplyDamage(TestPawn, MAX_FLT, MonsterController, TestPawn, UDamageType::StaticClass());
			}
		}
	}
	InvasionGameState->RemainingMonsters = 0;

	//reset player lifes
	SetPlayersLifes(3);
	//restart all dead players
	RestartAllPlayers(true);
}

void AShooterGameMode_Invasion::SpawnMonster()
{
	FInvasionWave Wave = GetCurrWave();
	//TSubclassOf<AShooterCharacter> RandomMonsterClass = Wave.InvasionMonsters[FMath::RandHelper(Wave.InvasionMonsters.Num())].PawnClass;
	TSubclassOf<AShooterCharacter> RandomMonsterClass = Wave.InvasionMonsters[FMath::RandHelper(Wave.InvasionMonsters.Num())].PawnClass;

	FVector SpawnLocation;
	AShooterCharacter* MonsterToCreate = Cast<AShooterCharacter>(RandomMonsterClass->GetDefaultObject());
	if (GetSpawnPoint(SpawnLocation, MonsterToCreate))
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AShooterCharacter* Monster = GetWorld()->SpawnActor<AShooterCharacter>(MonsterToCreate->GetClass(), SpawnLocation, FRotator::ZeroRotator, SpawnInfo);
		if (Monster)
		{
			Monster->SpawnDefaultController();
			InvasionGameState->RemainingMonsters++;
			InvasionGameState->TotalMonstersSpawned++;
			if (InvasionGameState->TotalMonstersSpawned >= Wave.MaxMonsters)
			{
				GetWorldTimerManager().ClearTimer(SpawnMonsterHandle);
			}
		}
	}
}

bool AShooterGameMode_Invasion::GetSpawnPoint(FVector& OutSpawnLocation, AShooterCharacter* TestCharacter) const
{
	OutSpawnLocation = FVector::ZeroVector;

	TArray<FVector> PossibleSpawns;

	UNavigationSystemV1* Nav = Cast< UNavigationSystemV1>(GetWorld()->GetNavigationSystem());

	if (Nav == NULL)
	{
		UE_LOG(LogShooter, Warning, TEXT("Map %s has no navigation mesh. Cannot spawn monsters."), *GetWorld()->PersistentLevel->GetFullName());
		return false;
	}
	FNavLocation PossibleSpawnPoint;
	// Try to look for 20 possible spawn points
	for (int32 i = 0; i < 20; i++)
	{
		if (Nav->GetRandomPoint(PossibleSpawnPoint, Nav->MainNavData))
		{
			if (!UShooterBlueprintLibrary::AnyPawnOverlapsPoint(PossibleSpawnPoint.Location, TestCharacter))
			{
				PossibleSpawns.Add(PossibleSpawnPoint.Location);
			}
		}
	}

	// no spawn points?
	if (PossibleSpawns.Num() == 0)
	{
		return false;
	}

	// from these 20 (or less) possible spawn points, return one that has no player in line of sight
	for (int32 i = 0; i < PossibleSpawns.Num(); i++)
	{
		FVector TestPoint = PossibleSpawns[i];
		//TestPoint is very close to the ground, so up it according to the monster's height
		//TODO: Causing crashes TestPoint.Z += TestCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2;
		TestPoint.Z += 50;
		if (!UShooterBlueprintLibrary::AnyPawnCanSeePoint(TestPoint))
		{
			OutSpawnLocation = PossibleSpawns[i];
			return true;
		}
	}

	//if it hit here then all spawn points are in LOS of players; return any
	OutSpawnLocation = PossibleSpawns[FMath::RandHelper(PossibleSpawns.Num())];
	return true;
}

void AShooterGameMode_Invasion::Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType)
{
	if (KilledPlayer->GetClass() == AShooterMonsterController::StaticClass())
	{
		InvasionGameState->RemainingMonsters--;
		if (InvasionGameState->RemainingMonsters <= 0 && InvasionGameState->TotalMonstersSpawned >= GetCurrWave().MaxMonsters)
		{
			StopWave();
		}
	}

	//this will update lives remaining, if a player was killed, and check match end
	Super::Killed(Killer, KilledPlayer, KilledPawn, KillerWeaponClass, KillerDmgType);
}

AActor* AShooterGameMode_Invasion::DetermineMatchWinner()
{
	if (!AnyPlayerHasLivesRemaining())
	{
		WinnerTeam = -1;
		//focus actor is the first monster found
		for (TActorIterator<AShooterCharacter> It(GetWorld()); It; ++It)
		{
			AShooterCharacter* TestPawn = *It;
			if (TestPawn && TestPawn->IsAlive() && TestPawn->GetPlayerState<AShooterPlayerState>() == NULL)
			{
				return TestPawn;
			}
		}
	}
	else
	{
		WinnerTeam = 0;
		AShooterPlayerState* BestPlayer = FindBestPlayer(0);
		if (BestPlayer)
		{
			AController* C = Cast<AController>(BestPlayer->GetOwner());
			ACharacter* Ch = C ? Cast<ACharacter>(C->GetCharacter()) : NULL;
			return Ch;
		}
	}
	return NULL;
}

void AShooterGameMode_Invasion::CheckMatchEnd()
{
	//are all players dead and with no lives left?
	if (!AnyPlayerHasLivesRemaining())
	{
		//players lose!
		FinishMatch();
	}
	else if (InvasionGameState->CurrentWave == Waves.Num())
	{
		//players win!
		FinishMatch();
	}
}