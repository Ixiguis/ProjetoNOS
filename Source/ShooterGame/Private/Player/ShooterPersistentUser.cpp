// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "Player/ShooterPersistentUser.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ShooterPlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "Player/ShooterCharacter.h"
#include "Player/ShooterPlayerState.h"
#include "Player/ShooterLocalPlayer.h"

UShooterPersistentUser::UShooterPersistentUser()
{
	SetToDefaults();
}

void UShooterPersistentUser::SetToDefaults()
{
	bIsDirty = false;


	bIsRecordingDemos = false;
	
	PlayerName = "Player";
	
	for (int32 i=0; i<GNumPlayerColors; i++)
	{
		//two random colors and the rest gray, otherwise it'll look like a clown :)
		if (i < 2)
		{
			PlayerColors.Add(FLinearColor::MakeRandomColor());
		}
		else
		{
			PlayerColors.Add(FLinearColor::Gray);
		}
		//Alpha is used as Roughness
		PlayerColors[i].A = FMath::FRand() * 0.5f + 0.1f;
	}
	return;
	//create default Invasion monsters
	//revolver, weak
	static ConstructorHelpers::FClassFinder<AShooterCharacter> ZombieOb(TEXT("/Game/Blueprints/Pawns/Monsters/Monster_Zombie"));
	FInvasionMonster Zombie(ZombieOb.Class);
	//shotgun, avg
	static ConstructorHelpers::FClassFinder<AShooterCharacter> Zombie2Ob(TEXT("/Game/Blueprints/Pawns/Monsters/Monster_Zombie2"));
	FInvasionMonster Zombie2(Zombie2Ob.Class);
	//rifle, weak
	static ConstructorHelpers::FClassFinder<AShooterCharacter> Zombie3Ob(TEXT("/Game/Blueprints/Pawns/Monsters/Monster_Zombie3"));
	FInvasionMonster Zombie3(Zombie3Ob.Class);
	//mjolnir, avg
	static ConstructorHelpers::FClassFinder<AShooterCharacter> Zombie4Ob(TEXT("/Game/Blueprints/Pawns/Monsters/Monster_Zombie4"));
	FInvasionMonster Zombie4(Zombie4Ob.Class);
	//gastraphetes, strong, boss
	static ConstructorHelpers::FClassFinder<AShooterCharacter> ZombieBossOb(TEXT("/Game/Blueprints/Pawns/Monsters/Monster_ZombieBoss"));
	FInvasionMonster ZombieBoss(ZombieBossOb.Class);

	//create default Invasion waves
	FInvasionWave Wave1 = FInvasionWave();
	Wave1.AddInvasionMonster(Zombie);
	Wave1.AddInvasionMonster(Zombie);
	Wave1.AddInvasionMonster(Zombie);
	Wave1.AddInvasionMonster(Zombie3);
	Wave1.MaxMonsters = 30;
	Wave1.MonstersSpawnRate = 20.f;

	FInvasionWave Wave1b = Wave1;
	Wave1b.MonstersSpawnRate = 25.f;
	Wave1b.MaxMonsters = 45;

	FInvasionWave Wave2 = FInvasionWave();
	Wave2.AddInvasionMonster(Zombie);
	Wave2.AddInvasionMonster(Zombie);
	Wave2.AddInvasionMonster(Zombie2);
	Wave2.AddInvasionMonster(Zombie3);
	Wave2.MonstersSpawnRate = 25.f;
	Wave2.MaxMonsters = 60;

	FInvasionWave Wave2b = Wave2;
	Wave2b.MonstersSpawnRate = 30.f;
	Wave2b.MaxMonsters = 100;

	FInvasionWave Wave3 = FInvasionWave();
	Wave3.AddInvasionMonster(Zombie);
	Wave3.AddInvasionMonster(Zombie4);
	Wave3.AddInvasionMonster(Zombie2);
	Wave3.AddInvasionMonster(Zombie3);
	Wave3.MonstersSpawnRate = 35.f;
	Wave3.MaxMonsters = 110;

	FInvasionWave Wave3b = Wave3;
	Wave3b.MonstersSpawnRate = 40.f;
	Wave3b.MaxMonsters = 145;

	FInvasionWave Wave4 = FInvasionWave();
	Wave4.AddInvasionMonster(Zombie2);
	Wave4.AddInvasionMonster(Zombie3);
	Wave4.AddInvasionMonster(Zombie4);
	Wave4.AddInvasionMonster(Zombie2);
	Wave4.AddInvasionMonster(Zombie3);
	Wave4.AddInvasionMonster(Zombie4);
	Wave4.AddInvasionMonster(Zombie2);
	Wave4.AddInvasionMonster(Zombie3);
	Wave4.AddInvasionMonster(Zombie4);
	Wave4.AddInvasionMonster(ZombieBoss);
	Wave4.MonstersSpawnRate = 50.f;
	Wave4.MaxMonsters = 150;

	FInvasionWave Wave4b = Wave4;
	Wave4b.MonstersSpawnRate = 55.f;
	Wave4b.MaxMonsters = 230;

	//add them to the waves array
	InvasionWaves.Add(Wave1);
	InvasionWaves.Add(Wave1b);
	InvasionWaves.Add(Wave2);
	InvasionWaves.Add(Wave2);
	InvasionWaves.Add(Wave2b);
	InvasionWaves.Add(Wave3);
	InvasionWaves.Add(Wave3b);
	InvasionWaves.Add(Wave4);
	InvasionWaves.Add(Wave4b);
}

UShooterLocalPlayer* UShooterPersistentUser::GetLocalPlayer() const
{
	if (GEngine)
	{
		TArray<APlayerController*> PlayerList;
		GEngine->GetAllLocalPlayerControllers(PlayerList);

		for (auto It = PlayerList.CreateIterator(); It; ++It)
		{
			APlayerController* PC = *It;
			if (!PC || !PC->Player || !PC->PlayerInput)
			{
				continue;
			}

			// Update key bindings for the current user only
			UShooterLocalPlayer* LocalPlayer = Cast<UShooterLocalPlayer>(PC->Player);
			if (LocalPlayer && LocalPlayer->GetPersistentUser() == this)
			{
				return LocalPlayer;
			}
		}
	}
	return NULL;
}


AShooterPlayerController* UShooterPersistentUser::GetPlayerController() const
{
	if (GEngine)
	{
		TArray<APlayerController*> PlayerList;
		GEngine->GetAllLocalPlayerControllers(PlayerList);

		for (auto It = PlayerList.CreateIterator(); It; ++It)
		{
			AShooterPlayerController* PC = Cast<AShooterPlayerController>(*It);
			if (!PC || !PC->Player || !PC->PlayerInput)
			{
				continue;
			}

			// Update key bindings for the current user only
			UShooterLocalPlayer* LocalPlayer = Cast<UShooterLocalPlayer>(PC->Player);
			if (!LocalPlayer || LocalPlayer->GetPersistentUser() != this)
			{
				continue;
			}
			return PC;
		}
	}
	return NULL;
}


bool IsAimSensitivityDirty()
{
	return false;
}

bool IsInvertedYAxisDirty()
{
	return false;
}

void UShooterPersistentUser::SavePersistentUser()
{
	UGameplayStatics::SaveGameToSlot(this, SlotName, UserIndex);
	bIsDirty = false;
}

UShooterPersistentUser* UShooterPersistentUser::LoadPersistentUser2(FString SlotName, const int32 UserIndex)
{
	UShooterPersistentUser* Result = nullptr;

	// first set of player signins can happen before the UWorld exists, which means no OSS, which means no user names, which means no slotnames.
	// Persistent users aren't valid in this state.
	if (SlotName.Len() > 0)
	{
		Result = Cast<UShooterPersistentUser>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
		if (Result == NULL)
		{
			// if failed to load, create a new one
			Result = Cast<UShooterPersistentUser>(UGameplayStatics::CreateSaveGameObject(UShooterPersistentUser::StaticClass()));
		}
		check(Result != NULL);

		Result->SlotName = SlotName;
		Result->UserIndex = UserIndex;
	}

	return Result;
}

void UShooterPersistentUser::SaveIfDirty()
{
	if (bIsDirty || IsInvertedYAxisDirty() || IsAimSensitivityDirty())
	{
		SavePersistentUser();
	}
}

void UShooterPersistentUser::AddMatchResult(int32 MatchKills, int32 MatchDeaths, int32 MatchSuicides, int32 MatchBulletsFired, int32 MatchRocketsFired, bool bIsMatchWinner)
{
	Kills += MatchKills;
	Deaths += MatchDeaths;
	Suicides += MatchSuicides;
	BulletsFired += MatchBulletsFired;
	RocketsFired += MatchRocketsFired;

	if (bIsMatchWinner)
	{
		Wins++;
	}
	else
	{
		Losses++;
	}

	bIsDirty = true;
}

void UShooterPersistentUser::TellInputAboutKeybindings()
{
/*
	TArray<APlayerController*> PlayerList;
	GEngine->GetAllLocalPlayerControllers(PlayerList);

	for (auto It = PlayerList.CreateIterator(); It; ++It)
	{
		APlayerController* PC = *It;
		if (!PC || !PC->Player || !PC->PlayerInput)
		{
			continue;
		}

		// Update key bindings for the current user only
		UShooterLocalPlayer* LocalPlayer = Cast<UShooterLocalPlayer>(PC->Player);
		if (!LocalPlayer || LocalPlayer->GetPersistentUser() != this)
		{
			continue;
		}

		//set the aim sensitivity
		for (int32 Idx = 0; Idx < PC->PlayerInput->AxisMappings.Num(); Idx++)
		{
			FInputAxisKeyMapping &AxisMapping = PC->PlayerInput->AxisMappings[Idx];
			if (AxisMapping.AxisName == "Lookup" || AxisMapping.AxisName == "LookupRate" || AxisMapping.AxisName == "Turn" || AxisMapping.AxisName == "TurnRate")
			{
				AxisMapping.Scale = (AxisMapping.Scale < 0.0f) ? -GetAimSensitivity() : +GetAimSensitivity();
			}
		}
		PC->PlayerInput->ForceRebuildingKeyMaps();

		//invert it, and if does not equal our bool, invert it again
		if (PC->PlayerInput->GetInvertAxis("LookupRate") != GetInvertedYAxis())
		{
			PC->PlayerInput->InvertAxis("LookupRate");
		}

		if (PC->PlayerInput->GetInvertAxis("Lookup") != GetInvertedYAxis())
		{
			PC->PlayerInput->InvertAxis("Lookup");
		}
	}*/
}

int32 UShooterPersistentUser::GetUserIndex() const
{
	return UserIndex;
}

void UShooterPersistentUser::SetPlayerColor(uint8 ColorIndex, FLinearColor NewColor)
{
	bIsDirty |= PlayerColors[ColorIndex] != NewColor;
	check(ColorIndex < PlayerColors.Num());
	PlayerColors[ColorIndex] = NewColor;
	//update color locally
	AShooterPlayerController* PC = GetPlayerController();
	if (PC)
	{
		AShooterPlayerState* PS = PC->GetPlayerState<AShooterPlayerState>();
		if (PS)
		{
			PS->SetColorLocal(ColorIndex, NewColor);
		}
	}
}

void UShooterPersistentUser::SetPlayerName(FString NewName)
{
	bIsDirty |= PlayerName != NewName;
	PlayerName = NewName;
	AShooterPlayerController* PC = GetPlayerController();
	if (PC)
	{
		PC->SetPlayerName(NewName);
	}
}

int32 UShooterPersistentUser::GetKills() const
{
	return Kills;
}

int32 UShooterPersistentUser::GetDeaths() const
{
	return Deaths;
}

int32 UShooterPersistentUser::GetSuicides() const
{
	return Suicides;
}

int32 UShooterPersistentUser::GetWins() const
{
	return Wins;
}

int32 UShooterPersistentUser::GetLosses() const
{
	return Losses;
}

int32 UShooterPersistentUser::GetBulletsFired() const
{
	return BulletsFired;
}

int32 UShooterPersistentUser::GetRocketsFired() const
{
	return RocketsFired;
}

FString UShooterPersistentUser::GetName() const
{
	return SlotName;
}

FLinearColor UShooterPersistentUser::GetColor(uint8 ColorIndex) const
{
	check(ColorIndex < PlayerColors.Num());
	return PlayerColors[ColorIndex];
}

FString UShooterPersistentUser::GetPlayerName() const
{
	return PlayerName;
}

void UShooterPersistentUser::SetIsRecordingDemos(const bool InbIsRecordingDemos)
{
	bIsDirty |= bIsRecordingDemos != InbIsRecordingDemos;
	bIsRecordingDemos = InbIsRecordingDemos;
}

void UShooterPersistentUser::SetWeaponGroup(TSubclassOf<AShooterWeapon> Weapon, int32 NewGroup)
{
	WeaponGroups.Add(Weapon, NewGroup);
}

int32 UShooterPersistentUser::GetWeaponGroup(TSubclassOf<AShooterWeapon> Weapon)
{
	const int32* Group = WeaponGroups.Find(Weapon);
	if (Group != nullptr)
	{
		return *Group;
	}
	return GetDefault<AShooterWeapon>(Weapon)->WeaponCategory;
}

void UShooterPersistentUser::ReplicateColors()
{
	AShooterPlayerController* PC = GetPlayerController();
	if (PC)
	{
		AShooterPlayerState* PS = PC->GetPlayerState<AShooterPlayerState>();
		if (PS)
		{
			for (int32 i = 0; i<PlayerColors.Num(); i++)
			{
				PS->ServerSetColor(i, PlayerColors[i]);
			}
		}
	}
}
