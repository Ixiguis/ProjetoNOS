// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "ShooterBlueprintLibrary.generated.h"

UCLASS()
class UShooterBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UShooterBlueprintLibrary();

	/** Returns ObjectClass's default object, so you can read its default properties. */
	UFUNCTION(BlueprintPure, Category = "Utilities", meta = (Keywords = "properties"))
	static UObject* GetDefaultObjectFromClass(TSubclassOf<UObject> ObjectClass);

	/** Returns Object's default object, so you can read its default properties. */
	UFUNCTION(BlueprintPure, Category = "Utilities", meta = (Keywords = "properties", DefaultToSelf = "Object"))
	static UObject* GetDefaultObject(const class UObject* Object);

	/** Changes an actor's tick group. */
	UFUNCTION(BlueprintCallable, Category = "Actor")
	static void SetTickGroup(AActor* Actor, const ETickingGroup NewTickGroup);
	
	/** Determines whether TestActor's root component position is currently being hit by a DirectionalLight with bUsedAsAtmosphericSunlight=true.
	*	@param TestActor Actor to test.
	*	@param Sun The directional light to test. If NULL, looks for the first one in the world (do not call this every frame with Sun == NULL). 
	*	@param IgnoreActors Optional additional actor to ignore on the trace. Note: TestActor is always ignored. */
	UFUNCTION(BlueprintPure, Category = "Lights")
	static bool ActorIsSunlit(const class AActor* TestActor, const ADirectionalLight* Sun = NULL, const class AActor* IgnoreActor = NULL);

	/** Determines whether TestLocation is currently being hit by a DirectionalLight with bUsedAsAtmosphericSunlight=true.
	*	@param Sun The directional light to test. If NULL, looks for the first one in the world (avoid calling this every frame with Sun == NULL).
	*	@param IgnoreActors Optional additional actor to ignore on the trace. */
	UFUNCTION(BlueprintPure, Category = "Lights")
	static bool LocationIsSunlit(const FVector& TestLocation, const TArray<class AActor*> IgnoreActors, const ADirectionalLight* Sun = NULL);

	/** Finds the first DirectionalLight with bUsedAsAtmosphericSunlight=true. */
	UFUNCTION(BlueprintPure, Category = "Lights")
	static ADirectionalLight* GetSun();

	/** Determines whether any enemy for TestPawn is within the specified radius.
	*	@param TestLocation if non-zero, uses that location as the center of the test. Otherwise, uses the pawn's location. 
	*	@param bTestLOS Whether enemies should also be in line of sight to return true. 
	*	@return true if any enemy is within the specified radius. */
	UFUNCTION(BlueprintPure, Category = "Pawn")
	static bool AnyEnemyWithin(const class AShooterCharacter* TestPawn, const float Radius, const FVector& TestLocation = FVector::ZeroVector, const bool bTestLOS = true);

	/** Determines whether any enemy for TestController is within the specified radius.
	*	@param bTestLOS Whether enemies should also be in line of sight to return true.
	*	@return true if any enemy is within the specified radius. */
	UFUNCTION(BlueprintPure, Category = "Pawn")
	static bool AnyEnemyWithinLocation(class AController* TestController, const float Radius, const FVector& TestLocation, const bool bTestLOS = true);

	UFUNCTION(BlueprintPure, Category = "Pawn")
	static bool IsLookingAt(const class AShooterCharacter* TestPawn, FVector TestPoint, float AngleTolerance = 90.f);
	
	/** Returns whether any alive pawn overlaps given Point. 
	*	@param	Character The character we're testing if it fits, will use its collision size to determine if it overlaps any other pawn. */
	UFUNCTION(BlueprintPure, Category = "Pawn")
	static bool AnyPawnOverlapsPoint(FVector Point, AShooterCharacter* Character);

	/** Returns whether any alive pawn has line of sight to given Point.
	*	@param	Character The character we're testing if it fits, will use its collision size to determine if it overlaps any other pawn.
		@param bCheckMonsters Whether monsters (pawns without a PlayerState) should also check if they can see Point (default off: only check players and bots).
		@param bIgnoreViewRotation Whether pawns should also be facing the Point, within a 90 degree tolerance. */
	UFUNCTION(BlueprintPure, Category = "Pawn")
	static bool AnyPawnCanSeePoint(FVector Point, bool bCheckMonsters = false, bool bIgnoreViewRotation = true);

	UFUNCTION(BlueprintPure, Category = "Options")
	static class UShooterGameUserSettings* GetGameUserSettings();
	
	/** Returns an array of all available game modes. */
	UFUNCTION(BlueprintPure, Category = "Game", meta=(WorldContext="WorldContextObject"))
	static TArray<class AShooterGameMode*> GetAllGameModes(class UObject* WorldContextObject);

	/** Try to find an acceptable position to place TestActor as close to possible to PlaceLocation.  Expects PlaceLocation to be a valid location inside the level. */
	UFUNCTION(BlueprintPure, Category = "Collision")
	static bool FindTeleportSpot(AActor* TestActor, FVector PlaceLocation, FRotator PlaceRotation, FVector& ResultingLocation);
	
	/** Returns the text associated with a team number, e.g. 0 returns localized "Red". */
	UFUNCTION(BlueprintPure, Category = "Game")
	static FText GetTeamColorText(int32 TeamNum);
	
	/** Returns the color associated with a team number, e.g. 0 returns the red team's color. -1 returns gray. */
	UFUNCTION(BlueprintPure, Category = "Game")
	static FLinearColor GetTeamColor(int32 TeamNum);
	
	/** Returns time formatted as MM:SS. */
	UFUNCTION(BlueprintPure, Category = "Game")
	static FText FormatTime(float TimeSeconds);

protected:
};
