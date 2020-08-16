// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.
#pragma once

#include "Engine/LocalPlayer.h"
#include "ShooterLocalPlayer.generated.h"

UCLASS(BlueprintType, config=Engine, transient)
class UShooterLocalPlayer : public ULocalPlayer
{
	GENERATED_BODY()

public:
	UShooterLocalPlayer();

	virtual void SetControllerId(int32 NewControllerId) override;
	
	virtual FString GetNickname() const;

	/** Initializes the PersistentUser */
	void LoadPersistentUser();

	UFUNCTION(BlueprintCallable, Category=LocalPlayer)
	class UShooterPersistentUser* GetPersistentUser() const;
	
private:
	/** Persistent user data stored between sessions (i.e. the user's savegame) */
	UPROPERTY()
	class UShooterPersistentUser* PersistentUser;

};



