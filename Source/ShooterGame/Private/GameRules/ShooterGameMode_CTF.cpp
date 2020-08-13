// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#include "GameRules/ShooterGameMode_CTF.h"


AShooterGameMode_CTF::AShooterGameMode_CTF()
{
	GameModeInfo.GameModeName = NSLOCTEXT("Game", "CaptureTheFlag", "Capture the Flag");
	GameModeInfo.MinTeams=2;
	GameModeInfo.MaxTeams=4;
	GameModeInfo.bRequiresMapPrefix=true;
	GameModeInfo.bAddToMenu = true;
	GameModeInfo.bIgnoreMapTeamCountRestriction = false;
}

void AShooterGameMode_CTF::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	//NumTeams = 3;
	FlagAutoReturnTime = 30.f;
}

void AShooterGameMode_CTF::InitGameState()
{
	Super::InitGameState();
	LoadFlags();
	ShooterGameState->bPlayersAddTeamScore = false;
}

void AShooterGameMode_CTF::BroadcastFlagTaken_Implementation(uint8 FlagIdx, AShooterCharacter* Taker)
{
	if (Taker == NULL)
	{
		AController* Dropper = Flags[FlagIdx]->GetFlagCarrier() ? Flags[FlagIdx]->GetFlagCarrier()->GetController() : NULL;
		Flags[FlagIdx]->DropFlag();
		if (GetLocalRole() == ROLE_Authority)
		{
			MessagePlayers(EMessageTypes::FlagDropped, Dropper, Taker, NULL, 0, FlagIdx);
		}
		return;
	}
	AShooterPlayerState* SPS = Taker->GetPlayerState<AShooterPlayerState>();
	if (SPS)
	{
		if (FlagIdx == SPS->GetTeamNum())
		{
			//my flag is on the ground? Return it to base
			if (Flags[FlagIdx]->IsDropped)
			{
				Flags[FlagIdx]->ReturnFlag();
				if (GetLocalRole() == ROLE_Authority)
				{
					MessagePlayers(EMessageTypes::FlagRecovered, Taker->GetController(), Taker, NULL, 0, FlagIdx);
				}
			}
			//my flag is at base? Score for each enemy flag that the Taker is carrying
			else if (Flags[FlagIdx]->IsAtBase())
			{
				Score(Taker);
			}
		}
		else
		{
			const bool bFlagTaken = Flags[FlagIdx]->TakeFlag(Taker);
			if (GetLocalRole() == ROLE_Authority && bFlagTaken)
			{
				MessagePlayers(EMessageTypes::FlagTaken, Taker->GetController(), Taker, NULL, 0, FlagIdx);
			}
		}
	}
}


void AShooterGameMode_CTF::Score(AShooterCharacter* Scorer)
{
#if UE_BUILD_DEBUG
	if (!Scorer)
	{
		//no scorer -- this shouldn't happen
		FPlatformMisc::DebugBreak();
		return;
	}
#endif //UE_BUILD_DEBUG
	AShooterPlayerState* SPS = Scorer->GetPlayerState<AShooterPlayerState>();
	if (SPS)
	{
		//score for each enemy flag taken
		for (uint8 i = 0; i < Flags.Num(); i++)
		{
			if (Flags[i]->GetFlagCarrier() == Scorer)
			{
				ShooterGameState->AddTeamScore(SPS->GetTeamNum(), 1);
				Flags[i]->ReturnFlag();
				if (GetLocalRole() == ROLE_Authority)
				{
					MessagePlayers(EMessageTypes::FlagCaptured, Scorer->GetController(), Scorer, NULL, 0, i);
				}
			}
		}
	}
	CheckMatchEnd();
}

void AShooterGameMode_CTF::LoadFlags()
{
	AShooterFlagBase* FlagBases[GMaxTeams];
	for (uint8 i = 0; i < GMaxTeams; i++)
	{
		FlagBases[i] = NULL;
	}
	uint8 BasesFound = 0;
	for (TActorIterator< AShooterFlagBase > ActorItr = TActorIterator< AShooterFlagBase >(GetWorld()); ActorItr; ++ActorItr)
	{
		AShooterFlagBase* FlagBase = *ActorItr;
		check(FlagBase->TeamNumber < GMaxTeams);
		if (FlagBases[FlagBase->TeamNumber] == NULL)
		{
			FlagBases[FlagBase->TeamNumber] = FlagBase;
			BasesFound++;
		}
		else
		{
			UE_LOG(LogShooter, Warning, TEXT("Flag Base %s has a TeamNumber that was already defined by another Flag Base."), *FlagBase->GetFullName());
		}
	}
	if (BasesFound < 2)
	{
		UE_LOG(LogShooter, Fatal, TEXT("Map %s has no FlagBases for CTF games (or only one, or they aren't in the persistent level). [apenas os mapas Farce e Fallout suportam CTF]"), *GetWorld()->PersistentLevel->GetFullName());
	}
	for (uint8 i = 0; i < ShooterGameState->GetNumTeams(); i++)
	{
		if (FlagBases[i] == NULL)
		{
			UE_LOG(LogShooter, Warning, TEXT("Map %s has no FlagBase defined for team %s. Reducing the team count."), *GetWorld()->PersistentLevel->GetFullName(), *FString::FromInt(i));
			ShooterGameState->SetNumTeams(i);
			break;
		}
		else
		{
			FActorSpawnParameters SpawnInfo;
			AShooterFlag* Flag = GetWorld()->SpawnActor<AShooterFlag>(FlagBases[i]->FlagTemplate, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
			Flag->GetRootComponent()->AttachToComponent(FlagBases[i]->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
			Flag->SetFlagBase(FlagBases[i]);
			Flag->AutoReturnTime = FlagAutoReturnTime;
			Flags.Add(Flag);
		}
	}
}

void AShooterGameMode_CTF::Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType)
{
	Super::Killed(Killer, KilledPlayer, KilledPawn, KillerWeaponClass, KillerDmgType);

	for (uint8 i = 0; i < Flags.Num(); i++)
	{
		if (KilledPawn == Flags[i]->GetFlagCarrier())
		{
			BroadcastFlagTaken(i, NULL);
		}
	}
}

void AShooterGameMode_CTF::FinishMatch()
{
	for (uint8 i = 0; i < Flags.Num(); i++)
	{
		Flags[i]->ReturnFlag();
	}
	Super::FinishMatch();
}

AActor* AShooterGameMode_CTF::DetermineMatchWinner()
{
	Super::DetermineMatchWinner();

	if (WinnerTeam != NumTeams)
	{
		return Flags[WinnerTeam];
	}
	return NULL;
}

void AShooterGameMode_CTF::CheckMatchEnd()
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