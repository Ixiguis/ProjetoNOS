// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "GameRules/ShooterGameMode_TeamDeathMatch.h"
#include "GameRules/ShooterGameState.h"
#include "Player/ShooterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ShooterCharacter.h"
#include "Player/ShooterPlayerState.h"
#include "ShooterTeamStart.h"
#include "ShooterGameInstance.h"
#include "AI/ShooterAIController.h"

AShooterGameMode_TeamDeathMatch::AShooterGameMode_TeamDeathMatch()
{
	GameModeInfo.GameModeName = NSLOCTEXT("Game", "TeamDeathmatch", "Team Deathmatch");
	GameModeInfo.MinTeams = 2;
	GameModeInfo.MaxTeams = GetDefault<UShooterGameInstance>()->GetMaxTeams();
	GameModeInfo.bAddToMenu = true;
	GameModeInfo.bIgnoreMapTeamCountRestriction = true;
	bDelayedStart = true;
}

void AShooterGameMode_TeamDeathMatch::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
}

void AShooterGameMode_TeamDeathMatch::InitGameState()
{
	Super::InitGameState();
	ShooterGameState->bChangeToTeamColors = true;
	ShooterGameState->SetNumTeams(NumTeams);
}

void AShooterGameMode_TeamDeathMatch::PostLogin(APlayerController* NewPlayer)
{
	// Place player on a team before Super (VoIP team based init, findplayerstart, etc)
	AShooterPlayerState* NewPlayerState = CastChecked<AShooterPlayerState>(NewPlayer->PlayerState);
	const int32 TeamNum = ChooseTeam(NewPlayerState);
	NewPlayerState->ServerSetTeamNum(TeamNum);
	
	//load 3-team, 4-team, etc, streaming levels as needed
	AShooterPlayerController* PC = Cast<AShooterPlayerController>(NewPlayer);
	if (PC)
	{
		LoadAdditionalMaps(PC);
	}

	Super::PostLogin(NewPlayer);
}

void AShooterGameMode_TeamDeathMatch::LoadAdditionalMaps(AShooterPlayerController* PC)
{
	class FFindMapsVisitor : public IPlatformFile::FDirectoryVisitor
	{
	public:
		FFindMapsVisitor() {}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
		{
			if (!bIsDirectory)
			{
				FString FullFilePath(FilenameOrDirectory);
				if (FPaths::GetExtension(FullFilePath) == TEXT("umap"))
				{
					FString CleanFilename = FPaths::GetBaseFilename(FullFilePath);
					CleanFilename = CleanFilename.Replace(TEXT(".umap"), TEXT(""));
					MapsFound.Add(CleanFilename);
				}
			}
			return true;
		}
		TArray<FString> MapsFound;
	};
	
	const FString MapsFolder = FPaths::ProjectContentDir() + TEXT("Maps");
	FFindMapsVisitor Visitor;
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*MapsFolder, Visitor);

	const FString PersistentLevelName = GetWorld()->GetLevel(0)->OwningWorld->GetName();
	for (uint8 i=1; i<=ShooterGameState->GetNumTeams(); i++)
	{
		const FString TeamLevelName = PersistentLevelName + TEXT("_") + FString::FromInt(i) + TEXT("teams");
		for (int32 j = 0; j < Visitor.MapsFound.Num(); j++)
		{
			if (Visitor.MapsFound[j].Find(TeamLevelName) != INDEX_NONE)
			{
				FString PathName = TEXT("/Game/Maps/") + Visitor.MapsFound[j];// + TEXT(".") + Visitor.MapsFound[j] + TEXT(":PersistentLevel");
				//FString PathName = Visitor.MapsFound[j].Replace
				
				if (PC && PC->IsLocalController())
				{
					FLatentActionInfo info;
					info.UUID = j;
					UGameplayStatics::LoadStreamLevel(this, FName(*Visitor.MapsFound[j]), true, true, info);
				}
				else
				{
					PC->ClientLoadStreamingLevel(FName(*Visitor.MapsFound[j]), j);
				}
				
			}
		}
	}

}

APawn* AShooterGameMode_TeamDeathMatch::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, class AActor* StartSpot)
{
	return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
}

bool AShooterGameMode_TeamDeathMatch::CanDealDamage(class AShooterPlayerState* DamageInstigator, class AShooterPlayerState* DamagedPlayer) const
{
	return DamageInstigator && DamagedPlayer && (DamagedPlayer == DamageInstigator || DamagedPlayer->GetTeamNum() != DamageInstigator->GetTeamNum());
}

int32 AShooterGameMode_TeamDeathMatch::ChooseTeam(AShooterPlayerState* ForPlayerState) const
{
	TArray<int32> TeamBalance;
	TeamBalance.AddZeroed(NumTeams);

	// get current team balance
	for (int32 i = 0; i < GameState->PlayerArray.Num(); i++)
	{
		AShooterPlayerState const* const TestPlayerState = Cast<AShooterPlayerState>(GameState->PlayerArray[i]);
		if (TestPlayerState && TestPlayerState != ForPlayerState && TeamBalance.IsValidIndex(TestPlayerState->GetTeamNum()))
		{
			TeamBalance[TestPlayerState->GetTeamNum()]++;
		}
	}

	// find least populated one
	int32 BestTeamScore = TeamBalance[0];
	for (int32 i = 1; i < TeamBalance.Num(); i++)
	{
		if (BestTeamScore > TeamBalance[i])
		{
			BestTeamScore = TeamBalance[i];
		}
	}

	// there could be more than one...
	TArray<int32> BestTeams;
	for (int32 i = 0; i < TeamBalance.Num(); i++)
	{
		if (TeamBalance[i] == BestTeamScore)
		{
			BestTeams.Add(i);
		}
	}

	// get random from best list
	const int32 RandomBestTeam = BestTeams[FMath::RandHelper(BestTeams.Num())];
	return RandomBestTeam;
}

AActor* AShooterGameMode_TeamDeathMatch::DetermineMatchWinner()
{
	int32 BestScore = MAX_uint32;
	int32 BestTeam = -1;
	int32 NumBestTeams = 1;

	for (int32 i = 0; i < ShooterGameState->GetNumTeams(); i++)
	{
		const int32 TeamScore = ShooterGameState->GetTeamScore(i);
		if (BestScore < TeamScore)
		{
			BestScore = TeamScore;
			BestTeam = i;
			NumBestTeams = 1;
		}
		else if (BestScore == TeamScore)
		{
			NumBestTeams++;
		}
	}
	WinnerTeam = (NumBestTeams == 1) ? BestTeam : NumTeams;
	AShooterPlayerState* BestPlayer = (NumBestTeams == 1) ? FindBestPlayer(WinnerTeam) : NULL;
	if (BestPlayer)
	{
		AController* C = Cast<AController>(BestPlayer->GetOwner());
		ACharacter* Ch = C ? Cast<ACharacter>(C->GetCharacter()) : NULL;
		return Ch;
	}
	return NULL;
}

AShooterPlayerState* AShooterGameMode_TeamDeathMatch::FindBestPlayer(uint8 TeamNum)
{
	float BestScore = -MAX_FLT;
	TArray<int32> BestPlayers;

	for (int32 i = 0; i < ShooterGameState->PlayerArray.Num(); i++)
	{
		const AShooterPlayerState* PS = Cast<AShooterPlayerState>(ShooterGameState->PlayerArray[i]);
		if (PS)
		{
			const float PlayerScore = PS->GetScore();
			if (BestScore <= PlayerScore && PS->GetTeamNum() == TeamNum)
			{
				BestScore = PlayerScore;
				BestPlayers.Add(i);
			}
		}
	}
	AShooterPlayerState* BestPlayer = BestPlayers.Num() > 0 ? Cast<AShooterPlayerState>(ShooterGameState->PlayerArray[BestPlayers[FMath::RandHelper(BestPlayers.Num())]]) : NULL;
	return BestPlayer;
}


bool AShooterGameMode_TeamDeathMatch::IsWinner(class AShooterPlayerState* PlayerState) const
{
	return PlayerState && !PlayerState->IsQuitter() && PlayerState->GetTeamNum() == WinnerTeam;
}

bool AShooterGameMode_TeamDeathMatch::IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const
{
	if (Player)
	{
		AShooterTeamStart* TeamStart = Cast<AShooterTeamStart>(SpawnPoint);
		AShooterPlayerState* PlayerState = Cast<AShooterPlayerState>(Player->PlayerState);
		if (PlayerState && TeamStart && TeamStart->SpawnTeam != PlayerState->GetTeamNum())
		{
			return false;
		}
	}

	return Super::IsSpawnpointAllowed(SpawnPoint, Player);
}

void AShooterGameMode_TeamDeathMatch::InitBot(AShooterAIController* AIC, int32 BotNum)
{	
	AShooterPlayerState* BotPlayerState = CastChecked<AShooterPlayerState>(AIC->PlayerState);
	const int32 TeamNum = ChooseTeam(BotPlayerState);
	BotPlayerState->ServerSetTeamNum(TeamNum);		
	Super::InitBot(AIC, BotNum);
}

void AShooterGameMode_TeamDeathMatch::Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType)
{
	Super::Killed(Killer, KilledPlayer, KilledPawn, KillerWeaponClass, KillerDmgType);
}

void AShooterGameMode_TeamDeathMatch::PlayerChangedToTeam(AShooterPlayerState* Player, uint8 NewTeam)
{
	for (int32 i = 0; i < GameState->PlayerArray.Num(); i++)
	{
		AShooterPlayerState* TestPlayerState = Cast<AShooterPlayerState>(GameState->PlayerArray[i]);
		//attempt to assign a bot in NewTeam to another team, to rebalance.
		AShooterAIController* BotController = Cast<AShooterAIController>(TestPlayerState->GetOwner());
		if (TestPlayerState->GetTeamNum() == NewTeam && BotController)
		{
			const int32 NewTeamForBot = ChooseTeam(TestPlayerState);
			TestPlayerState->ServerSetTeamNum(NewTeamForBot);
			AShooterCharacter* BotCharacter = Cast<AShooterCharacter>(BotController->GetPawn());
			if (BotCharacter)
			{
				BotCharacter->Suicide();
			}
			return;
		}
	}
}

void AShooterGameMode_TeamDeathMatch::CheckMatchEnd()
{
	if (ScoreLimit > 0)
	{
		for (uint8 i = 0; i < ShooterGameState->GetNumTeams(); i++)
		{
			if (ShooterGameState->GetTeamScore(i) >= ScoreLimit)
			{
				FinishMatch();
			}
		}
	}
}
