// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameFramework/CheatManager.h"
#include "ShooterCheatManager.generated.h"

UCLASS(Within=ShooterPlayerController)
class UShooterCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UShooterCheatManager();

	UFUNCTION(exec)
	void ToggleInfiniteAmmo();

	UFUNCTION(exec)
	void ToggleInfiniteClip();

	UFUNCTION(exec)
	void ToggleMatchTimer();

	UFUNCTION(exec)
	void ForceMatchStart();

	UFUNCTION(exec)
	void ChangeTeam(int32 NewTeamNumber);

	UFUNCTION(exec)
	void Cheat(const FString& Msg);

	UFUNCTION(exec)
	void SpawnBot();
};
