// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once
#include "ShooterTypes.h"
#include "ShooterPersistentUser.generated.h"

UCLASS(BlueprintType)
class UShooterPersistentUser : public USaveGame
{
	GENERATED_BODY()

public:
	UShooterPersistentUser();

	/** Loads user persistence data if it exists, creates an empty record otherwise. */
	static UShooterPersistentUser* LoadPersistentUser2(FString SlotName, const int32 UserIndex);

	/** Saves data if anything has changed. */
	UFUNCTION(BlueprintCallable, Category=SaveGame)
	void SaveIfDirty();

	/** Records the result of a match. */
	void AddMatchResult(int32 MatchKills, int32 MatchDeaths, int32 MatchSuicides, int32 MatchBulletsFired, int32 MatchRocketsFired, bool bIsMatchWinner);

	/** needed because we can recreate the subsystem that stores it */
	void TellInputAboutKeybindings();

	int32 GetUserIndex() const;

	UFUNCTION(BlueprintCallable, Category=SaveGame)
	int32 GetKills() const;

	UFUNCTION(BlueprintCallable, Category=SaveGame)
	int32 GetDeaths() const;
	
	UFUNCTION(BlueprintCallable, Category=SaveGame)
	int32 GetSuicides() const;

	UFUNCTION(BlueprintCallable, Category=SaveGame)
	int32 GetWins() const;

	UFUNCTION(BlueprintCallable, Category=SaveGame)
	int32 GetLosses() const;

	UFUNCTION(BlueprintCallable, Category=SaveGame)
	int32 GetBulletsFired() const;

	UFUNCTION(BlueprintCallable, Category=SaveGame)
	int32 GetRocketsFired() const;

	UFUNCTION(BlueprintCallable, Category=SaveGame)
	FString GetName() const;
	
	inline bool PlayerNameIsDefault() const
	{
		return PlayerName == "Player";
	}
	
	UFUNCTION(BlueprintCallable, Category=SaveGame)
	FString GetPlayerName() const;

	UFUNCTION(BlueprintCallable, Category=SaveGame)
	FLinearColor GetColor(uint8 ColorIndex) const;
	
	inline int32 GetNumColors() const { return PlayerColors.Num(); }

	UFUNCTION(BlueprintCallable, Category=SaveGame)
	void SetPlayerColor(uint8 ColorIndex, FLinearColor NewColor);

	UFUNCTION(BlueprintCallable, Category=SaveGame)
	void SetPlayerName(FString NewName);

	/** Sends all player's colors to the PlayerState, for replication. */
	UFUNCTION(BlueprintCallable, Category=SaveGame)
	void ReplicateColors();

	/** Stores configuration about waves on Invasion game mode */
	UPROPERTY()
	TArray<FInvasionWave> InvasionWaves;

	/** Returns the ShooterLocalPlayer that owns this save game. */
	UFUNCTION(BlueprintCallable, Category=SaveGame)
	class UShooterLocalPlayer* GetLocalPlayer() const;
	
	/** Returns the AShooterPlayerController that owns this save game. */
	UFUNCTION(BlueprintCallable, Category=SaveGame)
	class AShooterPlayerController* GetPlayerController() const;
	
	FORCEINLINE bool IsRecordingDemos() const
	{
		return bIsRecordingDemos;
	}
	
	void SetIsRecordingDemos(const bool InbIsRecordingDemos);

	UFUNCTION(BlueprintCallable, Category = SaveGame)
	void SetWeaponGroup(TSubclassOf<AShooterWeapon> Weapon, int32 NewGroup);
	
	UFUNCTION(BlueprintCallable, Category = SaveGame)
	int32 GetWeaponGroup(TSubclassOf<AShooterWeapon> Weapon);

protected:
	void SetToDefaults();

	/** Triggers a save of this data. */
	void SavePersistentUser();

	/** Lifetime count of kills */
	UPROPERTY()
	int32 Kills;

	/** Lifetime count of deaths */
	UPROPERTY()
	int32 Deaths;
	
	/** Lifetime count of suicides */
	UPROPERTY()
	int32 Suicides;

	/** Lifetime count of match wins */
	UPROPERTY()
	int32 Wins;

	/** Lifetime count of match losses */
	UPROPERTY()
	int32 Losses;

	/** Lifetime count of bullets fired */
	UPROPERTY()
	int32 BulletsFired;

	/** Lifetime count of rockets fired */
	UPROPERTY()
	int32 RocketsFired;


	/** is recording demos? */
	UPROPERTY()
	bool bIsRecordingDemos;
	/** color assigned to vector parameter Color1, Color2, Color3, etc, on all character's meshes */
	UPROPERTY()
	TArray<FLinearColor> PlayerColors;
	
	UPROPERTY()
 	FString PlayerName;

	UPROPERTY()
	TMap<TSubclassOf<AShooterWeapon>, int32> WeaponGroups;

private:
	/** Internal.  True if data is changed but hasn't been saved. */
	bool bIsDirty;

	/** The string identifier used to save/load this persistent user. */
	FString SlotName;
	int32 UserIndex;
};