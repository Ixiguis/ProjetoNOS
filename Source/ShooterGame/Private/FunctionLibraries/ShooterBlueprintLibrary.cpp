// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "ShooterBlueprintLibrary.h"
#include "ShooterControllerInterface.h"
#include "ShooterGameMode_FreeForAll.h"
#include "ShooterGameMode_TeamDeathmatch.h"
#include "ShooterGameMode_CTF.h"

UShooterBlueprintLibrary::UShooterBlueprintLibrary()
{
}

UObject* UShooterBlueprintLibrary::GetDefaultObjectFromClass(TSubclassOf<UObject> ObjectClass)
{
	if (ObjectClass == NULL)
	{
		return NULL;
	}
    return ObjectClass->GetDefaultObject();
}

UObject* UShooterBlueprintLibrary::GetDefaultObject(const UObject* Object)
{
	if (Object == NULL)
	{
		return NULL;
	}
    return Object->GetClass()->GetDefaultObject();
}

void UShooterBlueprintLibrary::SetTickGroup(AActor* Actor, const ETickingGroup NewTickGroup)
{
	if (Actor)
	{
		Actor->SetTickGroup(NewTickGroup);
	}
}

bool UShooterBlueprintLibrary::ActorIsSunlit(const class AActor* TestActor, const ADirectionalLight* Sun, const class AActor* IgnoreActor)
{
	if (Sun == NULL)
	{
		//try to find a DirectionalLight if it was not passed as parameter
		Sun = GetSun();
		if (Sun == NULL)
		{
			return false;
		}
	}
	FHitResult HitInfo;
	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_SunTrace")), true, TestActor);
	RV_TraceParams.bTraceComplex = true;
	RV_TraceParams.bReturnPhysicalMaterial = true;
	RV_TraceParams.AddIgnoredActor(IgnoreActor);
 
	FVector End = Sun->GetActorRotation().Vector() * -200000.0f + TestActor->GetActorLocation();
	FVector Start = TestActor->GetActorLocation();
	
	//trace from the actor towards the sun's direction
	TestActor->GetWorld()->LineTraceSingleByChannel(HitInfo, Start, End, ECC_Visibility, RV_TraceParams);
	if (HitInfo.bBlockingHit && HitInfo.PhysMaterial != NULL && HitInfo.PhysMaterial->SurfaceType != PHYS_SURFACE_GLASS)
	{
		return false;
	}
	return true;
}


bool UShooterBlueprintLibrary::LocationIsSunlit(const FVector& TestLocation, const TArray<class AActor*> IgnoreActors, const ADirectionalLight* Sun /*= NULL*/)
{
	if (Sun == NULL)
	{
		//try to find a DirectionalLight if it was not passed as parameter
		Sun = GetSun();
		if (Sun == NULL)
		{
			return false;
		}
	}
	FHitResult HitInfo;
	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_SunTrace")), true);
	RV_TraceParams.bTraceComplex = true;
	RV_TraceParams.bReturnPhysicalMaterial = true;
	RV_TraceParams.AddIgnoredActors(IgnoreActors);

	FVector End = Sun->GetActorRotation().Vector() * -200000.0f + TestLocation;
	FVector Start = TestLocation;

	GWorld->LineTraceSingleByChannel(HitInfo, Start, End, ECC_Visibility, RV_TraceParams);
	if (HitInfo.bBlockingHit && HitInfo.PhysMaterial != NULL && HitInfo.PhysMaterial->SurfaceType != PHYS_SURFACE_GLASS)
	{
		return false;
	}
	return true;
}

ADirectionalLight* UShooterBlueprintLibrary::GetSun()
{
	if (GWorld == NULL)
	{
		return NULL;
	}
	for (TActorIterator< ADirectionalLight > ActorItr = TActorIterator< ADirectionalLight >(GWorld); ActorItr; ++ActorItr)
	{
		if (ActorItr && ActorItr->GetLightComponent()->IsUsedAsAtmosphereSunLight())
		{
			return *ActorItr;
		}
	}
	return NULL;
}

bool UShooterBlueprintLibrary::AnyEnemyWithin(const AShooterCharacter* TestPawn, const float Radius, const FVector& TestLocation, const bool bTestLOS)
{
	if (TestPawn == NULL || Radius == 0)
	{
		return false;
	}
	const FVector ActualTestLocation = TestLocation.IsZero() ? TestPawn->GetActorLocation() : TestLocation;
	const float RadiusSquared = Radius*Radius;
	for (TActorIterator<AShooterCharacter> It(TestPawn->GetWorld()); It; ++It)
	{
		AShooterCharacter* OtherPawn = *It;
		if (OtherPawn != NULL && OtherPawn != TestPawn && !OtherPawn->GetTearOff() /* (not dead) */ && TestPawn->IsEnemyFor(OtherPawn->GetController()))
		{
			const float DistanceToPawn = (ActualTestLocation - OtherPawn->GetActorLocation()).SizeSquared();
			if (DistanceToPawn <= RadiusSquared)
			{
				if (bTestLOS && TestPawn->Controller != NULL && TestPawn->Controller->LineOfSightTo(OtherPawn))
				{
					return true;
				}
				if (!bTestLOS)
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool UShooterBlueprintLibrary::AnyEnemyWithinLocation(AController* TestController, const float Radius, const FVector& TestLocation, const bool bTestLOS /*= true*/)
{
	if (!TestController || Radius == 0)
	{
		return false;
	}
	const float RadiusSquared = Radius*Radius;
	for (TActorIterator<AShooterCharacter> It(TestController->GetWorld()); It; ++It)
	{
		AShooterCharacter* OtherPawn = *It;
		IShooterControllerInterface* ControllerInterface = dynamic_cast<IShooterControllerInterface*>(TestController);
		if (OtherPawn && OtherPawn->GetController() && OtherPawn->GetController() != TestController && !OtherPawn->GetTearOff() /* (not dead) */ && ControllerInterface->IsEnemyFor(OtherPawn->GetController()))
		{
			const float DistanceToPawn = (TestLocation - OtherPawn->GetActorLocation()).SizeSquared();
			if (DistanceToPawn <= RadiusSquared)
			{
				if (bTestLOS && TestController->LineOfSightTo(OtherPawn))
				{
					return true;
				}
				if (!bTestLOS)
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool UShooterBlueprintLibrary::IsLookingAt(const class AShooterCharacter* TestPawn, FVector TestPoint, float AngleTolerance /*= 90.f*/)
{
	if (!TestPawn || AngleTolerance < 0.f)
	{
		return false;
	}
	if (AngleTolerance >= 360.f)
	{
		return true;
	}
	FVector SourceLocation;
	FRotator SourceRotation;
	TestPawn->GetActorEyesViewPoint(SourceLocation, SourceRotation);
	FVector TargetDirection = TestPoint - SourceLocation;
	if (TargetDirection.Normalize())
	{
		const float TargetDot = FVector::DotProduct(SourceRotation.Vector(), TargetDirection);
		const bool IsFacingOther = TargetDot >= FMath::Cos(FMath::DegreesToRadians(AngleTolerance / 2.f));
		return IsFacingOther;
	}
	return true;
}

/*

bool UShooterBlueprintLibrary::IsFacingTarget(const FVector& SourceLocation, const FRotator& SourceRotation, const FVector& TargetLocation, float AngleTolerance)
{
}
*/

bool UShooterBlueprintLibrary::AnyPawnOverlapsPoint(FVector Point, AShooterCharacter* Character)
{
	if (Character != nullptr)
	{
		for (TActorIterator<AShooterCharacter> It(Character->GetWorld()); It; ++It)
		{
			AShooterCharacter* OtherPawn = *It;
			if (OtherPawn && OtherPawn->IsAlive())
			{
				const float CombinedHeight = (Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + OtherPawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) * 2.0f;
				const float CombinedRadius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius() + OtherPawn->GetCapsuleComponent()->GetScaledCapsuleRadius();
				const FVector OtherLocation = OtherPawn->GetActorLocation();

				// check if player start overlaps this pawn
				if (FMath::Abs(Point.Z - OtherLocation.Z) < CombinedHeight && (Point - OtherLocation).Size2D() < CombinedRadius)
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool UShooterBlueprintLibrary::AnyPawnCanSeePoint(FVector Point, bool bCheckMonsters /*= false*/, bool bIgnoreViewRotation /*= true*/)
{
	if (GWorld == nullptr)
	{
		return false;
	}
	for (TActorIterator<AShooterCharacter> It(GWorld); It; ++It)
	{
		AShooterCharacter* TestPawn = *It;
		const bool bIsMonster = TestPawn ? TestPawn->GetPlayerState<AShooterPlayerState>() == NULL : false;
		if (TestPawn && TestPawn->IsAlive() &&
			(bCheckMonsters || (!bCheckMonsters && !bIsMonster)))
		{
			//trace from the pawn's eye point to target Point
			FHitResult HitInfo;
			static FName TraceTag = FName(TEXT("AnyPawnCanSee"));
			FCollisionQueryParams TraceParams(TraceTag, false, TestPawn);
			FVector Start;
			FRotator UnusedRot;
			TestPawn->GetActorEyesViewPoint(Start, UnusedRot);
			if (!GWorld->LineTraceSingleByChannel(HitInfo, Start, Point, ECC_WorldStatic, TraceParams))
			{
				//view not blocked. Should ignore view rotation?
				if (bIgnoreViewRotation)
				{
					return true;
				}
				//else, check if TestPawn is facing Point
				if (IsLookingAt(TestPawn, Point, 90.f))
				{
					return true;
				}
			}
			//else, keep checking other pawns
		}
	}
	return false;
}

UShooterGameUserSettings* UShooterBlueprintLibrary::GetGameUserSettings()
{
	return CastChecked<UShooterGameUserSettings>(GEngine->GetGameUserSettings());
}

TArray<AShooterGameMode*> UShooterBlueprintLibrary::GetAllGameModes(UObject* WorldContextObject)
{
	TArray<AShooterGameMode*> GameModes;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(AShooterGameMode::StaticClass())
			&& !It->HasAnyClassFlags(CLASS_Abstract))
		{
			AShooterGameMode* Game = Cast<AShooterGameMode>((*It)->GetDefaultObject());
			if (Game)
			{
				GameModes.Add(Game);
			}
		}
	}
	return GameModes;
}

bool UShooterBlueprintLibrary::FindTeleportSpot(AActor* TestActor, FVector PlaceLocation, FRotator PlaceRotation, FVector& ResultingLocation)
{
	ResultingLocation = PlaceLocation;
	if (TestActor && TestActor->GetWorld())
	{
		return TestActor->GetWorld()->FindTeleportSpot(TestActor, ResultingLocation, PlaceRotation);
	}
	return false;
}

FText UShooterBlueprintLibrary::GetTeamColorText(int32 TeamNum)
{
	if (TeamNum < GMaxTeams)
	{
		return GTeamColorsText[TeamNum];
	}
	return FText();
}

FLinearColor UShooterBlueprintLibrary::GetTeamColor(int32 TeamNum)
{
	if (TeamNum < GMaxTeams)
	{
		return GTeamColors[TeamNum];
	}
	return FLinearColor::Gray;
}

FText UShooterBlueprintLibrary::FormatTime(float TimeSeconds)
{
	const int32 TotalSeconds = FMath::Max(0, FMath::TruncToInt(TimeSeconds) % 3600);
	const int32 NumMinutes = TotalSeconds / 60;
	const int32 NumSeconds = TotalSeconds % 60;

	const FString TimeDesc = FString::Printf(TEXT("%02d:%02d"), NumMinutes, NumSeconds);
	return FText::FromString(TimeDesc);
}
