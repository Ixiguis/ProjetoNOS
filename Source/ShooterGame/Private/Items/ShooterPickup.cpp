// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "Items/ShooterPickup.h"
#include "Items/ShooterPickup_Powerup.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Player/ShooterPlayerController.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameRules/ShooterGameMode.h"
#include "Player/ShooterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "Sound/SoundCue.h"

AShooterPickup::AShooterPickup()
{
	USceneComponent* SceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComp"));
	RootComponent = SceneComp;
	CollisionComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionComp"));
	CollisionComp->InitCapsuleSize(40.0f, 50.0f);
	CollisionComp->AlwaysLoadOnClient = true;
	CollisionComp->AlwaysLoadOnServer = true;
	CollisionComp->SetCollisionObjectType(COLLISION_PICKUP);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComp->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	CollisionComp->SetupAttachment(SceneComp);
	//RootComponent = CollisionComp;

	MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComp"));
	MovementComp->UpdatedComponent = CollisionComp;
	MovementComp->Buoyancy = 0.3f;
	MovementComp->Friction = 1.f;
	MovementComp->ProjectileGravityScale = 1.f;
	MovementComp->bShouldBounce = false;
	bSimulatePhysics = false;

	PickupPSC = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("PickupFX"));
	//PickupPSC->bAutoActivate = false;
	PickupPSC->bAutoDestroy = false;
	PickupPSC->CastShadow = true;
	PickupPSC->SetRelativeLocation(FVector(0.f, 0.f, -50.f));
	PickupPSC->SetupAttachment(CollisionComp);
	
	PickupMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PickupMeshComponent"));
	PickupMeshComp->bCastDynamicShadow = true;
	PickupMeshComp->bReceivesDecals = false;
	PickupMeshComp->SetRelativeLocation(FVector(0.f, 0.f, -50.f));
	PickupMeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	//PickupMeshComp->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	//PickupMeshComp->bChartDistanceFactor = false;
	PickupMeshComp->SetCollisionObjectType(ECC_WorldDynamic);
	PickupMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupMeshComp->SetupAttachment(CollisionComp);

	RespawnTime = 15.0f;
	WasDropped = false;
	bIsActive = false;
	PickedUpBy = NULL;
	SpawnAtGameStart = true;

	SetCanBeDamaged(false);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	SetReplicateMovement(false);
	RespawnTimeMultipler = 1.f;
}

void AShooterPickup::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if( !GIsEditor )
	{
		// by default, pickup PSC starts visible, so that it is visible in the editor.
		// but it must be hidden if not editor, in case pickup doesn't spawns at game start.
		// BeginPlay() will unhide it if pickup spawns at game start.
		PickupPSC->DeactivateSystem();
		PickupMeshComp->SetHiddenInGame(true);
	}

	if (!bSimulatePhysics)
	{
		MovementComp->Deactivate();
	}
}

void AShooterPickup::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() == ROLE_Authority)
	{
		AShooterGameMode* GameMode = GetWorld()->GetAuthGameMode<AShooterGameMode>();
		RespawnTimeMultipler = GameMode ? GameMode->GlobalPickupRespawnTimeMultipler : 1.f;
		WarmupTime = GameMode ? GameMode->GetWarmupTime() : 10.f;
	}
		
	if (SpawnAtGameStart || WasDropped)
	{
		RespawnPickup();
	}
	else
	{
		PickupPSC->DeactivateSystem();
		const float FirstSpawnTime = GetRespawnTime() + WarmupTime;
		if (FirstSpawnTime > 0.0f)
		{
			GetWorldTimerManager().SetTimer(RespawnPickupHandle, this, &AShooterPickup_Powerup::RespawnPickup, FirstSpawnTime, false);
		}
	}
	if (PickupStay)
	{
		GetWorldTimerManager().SetTimer(CleanupPickersHandle, this, &AShooterPickup::CleanupPickers, 10.f, true);
	}
}

void AShooterPickup::NotifyActorBeginOverlap(class AActor* Other)
{
	Super::NotifyActorBeginOverlap(Other);
	PickupOnTouch(Cast<AShooterCharacter>(Other));
}

bool AShooterPickup::CanBePickedUp(class AShooterCharacter* TestPawn)
{
	if (IsPendingKill() || !bIsActive || !TestPawn || !TestPawn->IsAlive() || !TestPawn->CanPickupPickups)
	{
		return false;
	}
	if (PickupStay && PawnAlreadyPickedUp(TestPawn) && !WasDropped)
	{
		return false;
	}
	AShooterPlayerController* PC = Cast<AShooterPlayerController>(TestPawn->GetController());
	if ((PC == nullptr || !PC->IsLocalController()) && GetLocalRole() < ROLE_Authority)
	{
		//not server and not local player
		return false;
	}
	return true;
}

bool AShooterPickup::PawnAlreadyPickedUp(class AShooterCharacter* TestPawn)
{
	for (int32 i = 0; i < Pickers.Num(); i++)
	{
		if (Pickers[i] == TestPawn)
			return true;
	}
	return false;
}

/** removes characters that are dead or no longer exist from Pickers array */
void AShooterPickup::CleanupPickers()
{
	for (int32 i = 0; i < Pickers.Num(); i++)
	{
		if (!Pickers[i] || !Pickers[i]->IsAlive())
		{
			Pickers.RemoveAt(i);
			i--;
		}
	}
}

void AShooterPickup::GivePickupTo(class AShooterCharacter* Pawn)
{
}

void AShooterPickup::PickupOnTouch(class AShooterCharacter* Pawn)
{
	if (CanBePickedUp(Pawn))
	{
		GivePickupTo(Pawn);
		GivePickupTo_BP(Pawn);
		PickedUpBy = Pawn;
		if (PickupStay)
		{
			Pickers.Add(Pawn);
		}

		AShooterPlayerController* PawnPC = Cast<AShooterPlayerController>(Pawn->GetController());
		if (PawnPC)
		{
			if (PawnPC->IsLocalController())
			{
				PawnPC->NotifyPickup(this);
			}
		}

		OnPickedUp();

		if (!PickupStay)
		{
			bIsActive = false;
			GetWorldTimerManager().SetTimer(RespawnPickupHandle, this, &AShooterPickup::RespawnPickup, GetRespawnTime(), false);
		}

		if (WasDropped || DestroyOnPickup)
		{
			Destroy();
		}
	}
}

void AShooterPickup::RespawnPickup()
{
	bIsActive = true;
	PickedUpBy = NULL;
	OnRespawned();

	TArray<AActor*> OverlappingPawns;
	GetOverlappingActors(OverlappingPawns, AShooterCharacter::StaticClass());

	for (int32 i = 0; i < OverlappingPawns.Num(); i++)
	{
		PickupOnTouch(Cast<AShooterCharacter>(OverlappingPawns[i]));	
	}
}

void AShooterPickup::OnPickedUp()
{
	if (!PickupStay)
	{
		if (RespawningFX && RespawnTime > 0.0f)
		{
			PickupPSC->SetTemplate(RespawningFX);
			PickupPSC->ActivateSystem();
		}
		else
		{
			PickupPSC->DeactivateSystem();
		}

		PickupMeshComp->SetHiddenInGame(true);
	}

	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}

	OnPickedUpEvent();
	OnPickupPickedupDelegate.Broadcast(this);
}

void AShooterPickup::OnRespawned()
{
	if (ActiveFX)
	{
		PickupPSC->SetTemplate(ActiveFX);
		PickupPSC->ActivateSystem();
	}
	else
	{
		PickupPSC->DeactivateSystem();
	}
	
	PickupMeshComp->SetHiddenInGame(false);

	const float TimeSec = GetWorld()->GetTimeSeconds();
	const bool bJustSpawned = CreationTime >= (TimeSec - 5.0f);
	if (RespawnSound && !bJustSpawned)
	{
		UGameplayStatics::PlaySoundAtLocation(this, RespawnSound, GetActorLocation());
	}

	OnRespawnEvent();
	OnPickupRespawnedDelegate.Broadcast(this);
}

void AShooterPickup::OnRep_IsActive()
{
	if (bIsActive)
	{
		OnRespawned();
	}
	else
	{
		OnPickedUp();
	}
}

void AShooterPickup::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( AShooterPickup, bIsActive );
	DOREPLIFETIME( AShooterPickup, PickupStay);
	DOREPLIFETIME(AShooterPickup, RespawnTimeMultipler);
	DOREPLIFETIME(AShooterPickup, WarmupTime);
}

bool AShooterPickup::SetVelocity_Validate(FVector_NetQuantize10 NewVelocity)
{
	return true;
}

void AShooterPickup::SetVelocity_Implementation(FVector_NetQuantize10 NewVelocity)
{
	if (MovementComp)
	{
		bSimulatePhysics = true;
		MovementComp->Activate();
		MovementComp->Velocity = NewVelocity;
	}
}

void AShooterPickup::GetPickupMessage_Implementation(FText& Text, class UTexture2D*& LeftImage, class UTexture2D*& RightImage) const
{
	Text = NSLOCTEXT("Pickup", "UndefinedPickup", "Override GetPickupMessage in order to define the pickup message.");
}

float AShooterPickup::GetRespawnTime() const
{
	return RespawnTime * RespawnTimeMultipler;
}