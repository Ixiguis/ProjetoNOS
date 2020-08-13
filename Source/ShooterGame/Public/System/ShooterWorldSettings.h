// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#pragma once

#include "GameFramework/WorldSettings.h"
#include "ShooterWorldSettings.generated.h"

/**
 * 
 */
UCLASS()
class AShooterWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:
	AShooterWorldSettings();

	/** Map's author, shown on the UI. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WorldSettings)
	FString Author;
	
	/** Map's title, shown on the UI. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WorldSettings)
	FString Title;
	
	/** Whether this map supports arena game modes (Deathmatch, Invasion, CTF, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameMode)
	bool bSupportsArenaGameModes;
	
	/** Minimum number of teams supported in this map, if it's for a team-based game mode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameMode, Meta = (EditCondition = "bSupportsArenaGameModes"))
	int32 MinTeams;
	
	/** Maximum number of teams supported in this map, if it's for a team-based game mode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameMode, Meta = (EditCondition = "bSupportsArenaGameModes"))
	int32 MaxTeams;
	
#if WITH_EDITOR
	/** writes map's metadata (MaxTeamsCTF, title, author, etc.) to a txt file on the same location as the level, for fast reading */
	void WriteMetaData(uint32 SaveFlags, UWorld* World, bool bSuccess);
#endif

};
