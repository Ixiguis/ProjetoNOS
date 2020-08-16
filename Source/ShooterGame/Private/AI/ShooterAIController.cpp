// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "AI/ShooterAIController.h"
#include "Weapons/ShooterWeapon.h"
#include "GameRules/ShooterGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "Player/ShooterPlayerState.h"
#include "Player/ShooterCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "EngineUtils.h"

#define MIN_WEAPON_SWITCH_INTERVAL 3.0f

AShooterAIController::AShooterAIController()
{
 	BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackBoardComp"));
 	
	BrainComponent = BehaviorComp = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorComp"));	

	bWantsPlayerState = true;
}

void AShooterAIController::OnPossess(APawn* InPawn)
{
	Super::Possess(InPawn);

	AShooterCharacter* Bot = Cast<AShooterCharacter>(InPawn);

	// start behavior
	if (Bot && Bot->BotBehavior)
	{
		if (Bot->BotBehavior->BlackboardAsset)
		{
			BlackboardComp->InitializeBlackboard(*Bot->BotBehavior->BlackboardAsset);
		}

		EnemyKeyID = BlackboardComp->GetKeyID("Enemy");
		NeedAmmoKeyID = BlackboardComp->GetKeyID("NeedAmmo");

		BehaviorComp->StartTree(*(Bot->BotBehavior));
		
		bShouldRespawn = Bot->bShouldRespawn;
	}
}

void AShooterAIController::BeginInactiveState()
{
	Super::BeginInactiveState();

	if (bShouldRespawn)
	{
		AGameStateBase* GameState = GetWorld()->GetGameState();
		const float MinRespawnDelay = (GameState && GameState->GameModeClass) ? GetDefault<AGameMode>(GameState->GameModeClass)->MinRespawnDelay : 1.0f;
		GetWorldTimerManager().SetTimer(TimerHandle_Respawn, this, &AShooterAIController::Respawn, MinRespawnDelay);
	}
}

void AShooterAIController::Respawn()
{
	GetWorld()->GetAuthGameMode()->RestartPlayer(this);
}

void AShooterAIController::FindClosestEnemy()
{
	APawn* MyBot = GetPawn();
	if (MyBot == NULL)
	{
		return;
	}

	const FVector MyLoc = MyBot->GetActorLocation();
	float BestDistSq = MAX_FLT;
	AShooterCharacter* BestPawn = NULL;

	for (TActorIterator<AShooterCharacter> It(GetWorld()); It; ++It)
	{
		AShooterCharacter* TestPawn = *It;
		if (TestPawn && TestPawn->IsAlive() && IsEnemyFor(TestPawn->Controller))
		{
			const float DistSq = (TestPawn->GetActorLocation() - MyLoc).SizeSquared();
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestPawn = TestPawn;
			}
		}
	}

	if (BestPawn)
	{
		SetEnemy(BestPawn);
	}
}


bool AShooterAIController::FindClosestEnemyWithLOS(AShooterCharacter* ExcludeEnemy)
{
	bool bGotEnemy = false;
	APawn* MyBot = GetPawn();
	if (MyBot != NULL)
	{
		const FVector MyLoc = MyBot->GetActorLocation();
		float BestDistSq = MAX_FLT;
		AShooterCharacter* BestPawn = NULL;

		for (TActorIterator<AShooterCharacter> It(GetWorld()); It; ++It)
		{
			AShooterCharacter* TestPawn = *It;
			if (TestPawn && TestPawn != ExcludeEnemy && TestPawn->IsAlive() && IsEnemyFor(TestPawn->Controller))
			{
				if (HasWeaponLOSToEnemy(TestPawn, true) == true)
				{
					const float DistSq = (TestPawn->GetActorLocation() - MyLoc).SizeSquared();
					if (DistSq < BestDistSq)
					{
						BestDistSq = DistSq;
						BestPawn = TestPawn;
					}
				}
			}
		}
		if (BestPawn)
		{
			SetEnemy(BestPawn);
			bGotEnemy = true;
		}
	}
	return bGotEnemy;
}

bool AShooterAIController::HasWeaponLOSToEnemy(AActor* InEnemyActor, const bool bAnyEnemy) const
{
	static FName LosTag = FName(TEXT("AIWeaponLosTrace"));

	AShooterCharacter* MyBot = Cast<AShooterCharacter>(GetPawn());

	bool bHasLOS = false;
	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(LosTag, true, GetPawn());
	//TraceParams.bTraceAsyncScene = true;

	TraceParams.bReturnPhysicalMaterial = true;
	FVector StartLocation = MyBot->GetActorLocation();
	StartLocation.Z += GetPawn()->BaseEyeHeight; //look from eyes

	FHitResult Hit(ForceInit);
	const FVector EndLocation = InEnemyActor->GetActorLocation();
	GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, COLLISION_WEAPON, TraceParams);
	if (Hit.bBlockingHit == true)
	{
		// Theres a blocking hit - check if its our enemy actor
		AActor* HitActor = Hit.GetActor();
		if (Hit.GetActor() != NULL)
		{
			if (HitActor == InEnemyActor)
			{
				bHasLOS = true;
			}
			else if (bAnyEnemy == true)
			{
				// Its not our actor, maybe its still an enemy ?
				ACharacter* HitChar = Cast<ACharacter>(HitActor);
				if (HitChar != NULL)
				{
					AShooterPlayerState* HitPlayerState = HitChar->GetPlayerState<AShooterPlayerState>();
					AShooterPlayerState* MyPlayerState = GetPlayerState<AShooterPlayerState>();
					if ((HitPlayerState != NULL) && (MyPlayerState != NULL))
					{
						if (HitPlayerState->GetTeamNum() != MyPlayerState->GetTeamNum())
						{
							bHasLOS = true;
						}
					}
				}
			}
		}
	}
	return bHasLOS;
}


void AShooterAIController::ShootEnemy()
{
	AShooterCharacter* MyBot = Cast<AShooterCharacter>(GetPawn());
	if (MyBot == NULL)
	{
		return;
	}
	CurrentWeapon = SwitchToBestWeapon();
	if (CurrentWeapon == NULL)
	{
		return;
	}

	bool bCanShoot = false;
	AShooterCharacter* Enemy = GetEnemy();
	if (Enemy && Enemy->IsAlive() && CurrentWeapon->HasEnoughAmmo() && CurrentWeapon->CanFire() == true )
	{
		if (LineOfSightTo(Enemy, MyBot->GetActorLocation()))
		{
			bCanShoot = true;
		}
	}

	if (bCanShoot && Enemy->GetMesh())
	{
		/*FRotator UnusedRot;
		FVector BotEyes;
		GetActorEyesViewPoint(BotEyes, UnusedRot);
		const FVector Target = Enemy->GetMesh()->GetSocketLocation(TEXT("Heart"));
		FVector TargetDirection = Target - BotEyes;
		TargetDirection.Normalize();
		const float ConeHalfAngle = FMath::DegreesToRadians(MyBot->GetAimingDispersion() * 0.5f);
		const FVector ShootDir = FMath::VRandCone(TargetDirection, ConeHalfAngle, ConeHalfAngle);
		const FVector AimAt = ShootDir * 100000.f;
		SetFocalPoint(AimAt);
		DrawDebugSphere(GetWorld(), AimAt, 30.f, 12, FColor::Red, false, 1.f);
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::SanitizeFloat(MyBot->GetAimingDispersion()));*/
		//const FVector AimAt = Enemy->GetMesh()->GetSocketLocation(TEXT("Heart"));
		//SetFocalPoint(AimAt);
		MyBot->StartWeaponFire(0);
	}
	else
	{
		MyBot->StopAllWeaponFire();
	}
}

AShooterWeapon* AShooterAIController::SwitchToBestWeapon()
{
	AShooterCharacter* MyBot = Cast<AShooterCharacter>(GetPawn());
	float DistanceToEnemy = 0.f;
	AShooterCharacter* Enemy = GetEnemy();
	if (Enemy)
	{
		DistanceToEnemy = (MyBot->GetActorLocation() - Enemy->GetActorLocation()).SizeSquared();
	}
	AShooterWeapon* BestWeapon = MyBot->GetBestWeapon(DistanceToEnemy);
	if (BestWeapon && BestWeapon != CurrentWeapon && GetWorld()->GetTimeSeconds() - LastWeaponSwitchTime >= MIN_WEAPON_SWITCH_INTERVAL)
	{
		MyBot->EquipWeapon(BestWeapon);
		LastWeaponSwitchTime = GetWorld()->GetTimeSeconds();
		CurrentWeapon = BestWeapon;
	}
	return BestWeapon;
}

void AShooterAIController::CheckAmmo(const class AShooterWeapon* WeaponToCheck)
{
	AShooterCharacter* MyPawn = Cast<AShooterCharacter>(GetPawn());
	if (WeaponToCheck && BlackboardComp && MyPawn)
	{
		const int32 Ammo = MyPawn->GetCurrentAmmo(WeaponToCheck->GetClass());
		const int32 MaxAmmo = WeaponToCheck->GetMaxAmmo();
		const float Ratio = (float) Ammo / (float) MaxAmmo;

		BlackboardComp->SetValue<UBlackboardKeyType_Bool>(NeedAmmoKeyID, (Ratio <= 0.2f));
	}
}

void AShooterAIController::SetEnemy(class APawn* InPawn)
{
	if (BlackboardComp)
	{
		BlackboardComp->SetValue<UBlackboardKeyType_Object>(EnemyKeyID, InPawn);
		SetFocus(InPawn);
	}
}

class AShooterCharacter* AShooterAIController::GetEnemy() const
{
	if (BlackboardComp)
	{
		return Cast<AShooterCharacter>(BlackboardComp->GetValue<UBlackboardKeyType_Object>(EnemyKeyID));
	}

	return NULL;
}


void AShooterAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	// Look toward focus
	FVector FocalPoint = GetFocalPoint();
	if( !FocalPoint.IsZero() && GetPawn())
	{
		FVector Direction = FocalPoint - GetPawn()->GetActorLocation();
		FRotator NewControlRotation = Direction.Rotation();
		
		NewControlRotation.Yaw = FRotator::ClampAxis(NewControlRotation.Yaw);

		SetControlRotation(NewControlRotation);

		APawn* const P = GetPawn();
		if (P && bUpdatePawn)
		{
			P->FaceRotation(NewControlRotation, DeltaTime);
		}
		
	}
}

void AShooterAIController::GameHasEnded(AActor* EndGameFocus, bool bIsWinner)
{
	// Stop the behaviour tree/logic
	BehaviorComp->StopTree();

	// Stop any movement we already have
	StopMovement();

	// Cancel the repsawn timer
	GetWorldTimerManager().ClearTimer(TimerHandle_Respawn);

	// Clear any enemy
	SetEnemy(NULL);

	// finally stop firing
	AShooterCharacter* MyBot = Cast<AShooterCharacter>(GetPawn());
	AShooterWeapon* MyWeapon = MyBot ? MyBot->GetWeapon() : NULL;
	if (MyWeapon == NULL)
	{
		return;
	}
	MyBot->StopAllWeaponFire();
}

bool AShooterAIController::IsEnemyFor(AController* TestPC) const
{
	if (TestPC == this || TestPC == NULL)
	{
		return false;
	}

	AShooterPlayerState* TestPlayerState = TestPC->GetPlayerState<AShooterPlayerState>();
	AShooterPlayerState* MyPlayerState = GetPlayerState<AShooterPlayerState>();

	if (TestPC->GetWorld()->GetGameState() && GetWorld()->GetGameState()->GameModeClass)
	{
		const AShooterGameMode* DefGame = GetWorld()->GetGameState()->GameModeClass->GetDefaultObject<AShooterGameMode>();
		if (DefGame && MyPlayerState && TestPlayerState)
		{
			return DefGame->CanDealDamage(TestPlayerState, MyPlayerState);
		}
	}
	return true;
}
