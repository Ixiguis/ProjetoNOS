// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "Weapons/ShooterWeapon.h"
#include "Effects/ShooterImpactEffect.h"
#include "Weapons/ShooterProjectile.h"
#include "GameRules/ShooterGameState.h"
#include "GameFramework/ForceFeedbackEffect.h"
#include "Camera/CameraShake.h"
#include "Player/ShooterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ShooterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"
#include "AI/ShooterAIController.h"
#include "Sound/SoundCue.h"

DEFINE_LOG_CATEGORY(LogShooterWeapon);

// To maintain consistency between clients and server on RandomStream::VRandCone() calls,
// we'll use a constant vector on that function, instead of passing the AimDir directly,
// because AimDir will be slightly different across clients and VRandCone() will generate very different results.
const FVector ForwardVector = FVector(1.0f, 0.f, 0.f);

AShooterWeapon::AShooterWeapon()
{
	NetUpdateFrequency = 20.f;

	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh1P"));
	Mesh1P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Mesh1P->bReceivesDecals = false;
	Mesh1P->CastShadow = false;
	Mesh1P->bOnlyOwnerSee = true;
	Mesh1P->bOwnerNoSee = false;
	Mesh1P->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh1P->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh1P->bLocalSpaceSimulation = true;
	RootComponent = Mesh1P;

	Mesh3P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh3P"));
	Mesh3P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Mesh3P->bReceivesDecals = false;
	Mesh3P->CastShadow = true;
	Mesh3P->bOwnerNoSee = true;
	Mesh3P->bOnlyOwnerSee = false;
	Mesh3P->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh3P->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh3P->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	Mesh3P->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh3P->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	Mesh3P->SetupAttachment(Mesh1P);

	bPlayingFireAnim = false;
	bIsEquipped = false;
	bWantsToFire = false;
	bPendingEquip = false;
	bRewardHeadshots = false;
	CurrentState = EWeaponState::Idle;
	MaxChargeTime = 5.0f;
	ChargeRandomAdd = 0.f;
	ChargeAmmoTimer = 0.5f;
	ChargeRandomDisturbance = 0.2f;
	BurstCounter = 0;
	bIndependentFireModeCooldown = true;
	CharacterAnim = EWeaponAnim::Rifle;
	SetCanBeDamaged(false);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	for(uint8 i=0; i<NUM_FIRING_MODES; i++)
	{
		TimeBetweenShots[i] = 0.2f;
		ShotsPerTick[i] = 1;
		LastFireTime[i] = 0.0f;
		LastFireCooldown[i] = 0.1f;
		AIWeaponRange[i] = MAX_FLT;
	}
}

void AShooterWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	CurrentFiringDispersion = WeaponConfig.BaseFiringDispersion;

	//ShotThickness is used by GetAdjustedAim on projectile weapons, so update its values for projectiles weapon as well
	for (uint8 i = 0; i < NUM_FIRING_MODES; i++)
	{
		if (FiringMode[i] == EFireMode::FM_Projectile)
		{
			AShooterProjectile* DefProj = Cast<AShooterProjectile>(ProjectileClass[i].GetDefaultObject());
			if (DefProj)
			{
				InstantConfig[i].ShotThickness = DefProj->GetSimpleCollisionRadius() * 2.f;
			}
		}
	}
	DetachMeshFromPawn();
}

void AShooterWeapon::Destroyed()
{
	Super::Destroyed();

	StopSimulatingWeaponFire();
}

void AShooterWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (FiringMode[CurrentFireMode] == FM_Beam && (BurstCounter > 0 || bRefiring))
	{
		UpdateBeam();
	}
	
	if ( (!bRefiring || !HasEnoughAmmo()) && !WeaponConfig.OverrideDispersion && CurrentFiringDispersion > WeaponConfig.BaseFiringDispersion)
	{
		CurrentFiringDispersion = FMath::Max(CurrentFiringDispersion - WeaponConfig.FiringDispersionDecrement * DeltaSeconds, WeaponConfig.BaseFiringDispersion);
	}

#if !UE_BUILD_SHIPPING
	if (bIsEquipped || CurrentState == EWeaponState::Equipping)
	{
		AttachMeshToPawn();
		FTransform RightHand = Mesh3P->GetSocketTransform(RightHandAttachPoint, RTS_Component);
		Mesh3P->SetRelativeTransform(RightHand);
		RightHand = Mesh1P->GetSocketTransform(RightHandAttachPoint, RTS_Component);
		Mesh1P->SetRelativeTransform(RightHand);
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
// Inventory

void AShooterWeapon::EquipItem()
{
	if (MyPawn)
	{
		MyPawn->EquipWeapon(this);
	}
}

//called on everyone
void AShooterWeapon::OnEquip()
{
	AttachMeshToPawn();

	WeaponBeingEquippedEvent();
	bPendingEquip = true;
	DetermineWeaponState();

	float Duration = PlayWeaponAnimation(EquipAnim);
	if (Duration <= 0.0f)
	{
		// failsafe
		Duration = 0.5f;
	}
	EquipStartedTime = GetWorld()->GetTimeSeconds();
	EquipDuration = Duration;

	GetWorldTimerManager().SetTimer(OnEquipFinishedHandle, this, &AShooterWeapon::OnEquipFinished, Duration, false);

	if (MyPawn)
	{
		PlayWeaponSound(EquipSound);
	}
}

//called on everyone
void AShooterWeapon::OnEquipFinished()
{
	//AttachMeshToPawn();

	bIsEquipped = true;
	bPendingEquip = false;

	// Determine the state so that the can reload checks will work
	DetermineWeaponState();

	if (MyPawn)
	{
		// try to reload empty clip
		if (IsLocallyControlled() &&
			CurrentAmmoInClip <= 0 &&
			CanReload())
		{
			StartReload();
		}
	}
	WeaponEquippedEvent();
}

//called on everyone
void AShooterWeapon::OnUnEquip()
{
	DetachMeshFromPawn();
	bIsEquipped = false;

	if (bWantsToFire)
	{
		StopFire(CurrentFireMode);
	}

	if (bPendingReload)
	{
		StopWeaponAnimation(ReloadAnim);
		bPendingReload = false;

		GetWorldTimerManager().ClearTimer(StopReloadHandle);
		GetWorldTimerManager().ClearTimer(ReloadWeaponHandle);
	}

	if (bPendingEquip)
	{
		StopWeaponAnimation(EquipAnim);
		bPendingEquip = false;

		GetWorldTimerManager().ClearTimer(OnEquipFinishedHandle);
	}
	bPendingEquip = false;

	DetermineWeaponState();
	WeaponUnequippedEvent();
}

void AShooterWeapon::UseAmmo()
{
	//only reduce ammo in inventory if weapon doesn't requires reloading
	if (GetLocalRole() == ROLE_Authority && !HasInfiniteAmmo() && HasInfiniteClip())
	{
		MyPawn->UseAmmo(GetClass(), WeaponConfig.AmmoToConsume[CurrentFireMode]);
	}
	//otherwise, only reduce clip ammo (inventory ammo is reduced on reload)
	else if (GetLocalRole() == ROLE_Authority && !HasInfiniteClip())
	{
		CurrentAmmoInClip -= WeaponConfig.AmmoToConsume[CurrentFireMode];
	}

	AShooterAIController* BotAI = MyPawn ? MyPawn->GetController<AShooterAIController>() : NULL;
	if (BotAI)
	{
		BotAI->CheckAmmo(this);
	}
}

void AShooterWeapon::GiveAmmo(int32 Amount)
{
	if (MyPawn)
	{
		MyPawn->GiveAmmo(GetClass(), Amount);
	}
}

void AShooterWeapon::AttachMeshToPawn()
{
	if (MyPawn)
	{
		DetachMeshFromPawn();

		const FName AttachPoint = MyPawn->GetWeaponAttachPoint();
		
		if (MyPawn->GetPawnMesh1P()->SkeletalMesh != nullptr)
		{
			Mesh1P->AttachToComponent(MyPawn->GetPawnMesh1P(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachPoint);
			Mesh1P->SetHiddenInGame(false, true);
			// @@ se o cloth da soul hunter falhar:
			//Mesh1P->TickClothing(0.16f);
			Mesh1P->ForceClothNextUpdateTeleportAndReset();
		}

		if (MyPawn->GetPawnMesh3P()->SkeletalMesh != nullptr)
		{
			Mesh3P->AttachToComponent(MyPawn->GetPawnMesh3P(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachPoint);
			Mesh3P->SetHiddenInGame(false, true);

			FTransform RightHand = Mesh3P->GetSocketTransform(RightHandAttachPoint, RTS_Component);
			Mesh3P->RootBoneTranslation = RightHand.GetLocation();
			//Mesh3P->SetRelativeTransform(RightHand);
			RightHand = Mesh1P->GetSocketTransform(RightHandAttachPoint, RTS_Component);
			Mesh1P->RootBoneTranslation = RightHand.GetLocation();
			//Mesh1P->SetRelativeTransform(RightHand);
		}
	}
}

void AShooterWeapon::DetachMeshFromPawn()
{
	Mesh1P->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Mesh1P->SetHiddenInGame(true, true);

	Mesh1P->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Mesh3P->SetHiddenInGame(true, true);
}

void AShooterWeapon::OwnerDied()
{	
	if (BurstCounter > 0)
	{
		OnBurstFinished();
		StopFire(CurrentFireMode);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

// local + server
void AShooterWeapon::StartFire(uint8 FireMode)
{
	check(FireMode < NUM_FIRING_MODES);
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerStartFire(FireMode);
	}

	if (FiringMode[FireMode] == EFireMode::FM_Custom)
	{
		StartCustomFire(FireMode);
	}
	else if (!bWantsToFire )
	{
		CurrentFireMode = FireMode;
		bWantsToFire = true;
		DetermineWeaponState();
	}
}

// local + server
void AShooterWeapon::StopFire(uint8 FireMode)
{
	check(FireMode < NUM_FIRING_MODES);
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerStopFire(FireMode);
	}
	
	if (FiringMode[FireMode] == EFireMode::FM_Custom)
	{
		StopCustomFire(FireMode);
	}
	else if (bWantsToFire)
	{
		bWantsToFire = false;
		DetermineWeaponState();
	}
}

bool AShooterWeapon::ServerStartFire_Validate(uint8 FireMode)
{
	return FireMode < NUM_FIRING_MODES;
}

void AShooterWeapon::ServerStartFire_Implementation(uint8 FireMode)
{
	StartFire(FireMode);
}

bool AShooterWeapon::ServerStopFire_Validate(uint8 FireMode)
{
	return FireMode < NUM_FIRING_MODES;
}

void AShooterWeapon::ServerStopFire_Implementation(uint8 FireMode)
{
	StopFire(FireMode);
}

void AShooterWeapon::StartReload(bool bFromReplication)
{
	if (!bFromReplication && GetLocalRole() < ROLE_Authority)
	{
		ServerStartReload();
	}

	if (bFromReplication || CanReload())
	{
		bPendingReload = true;
		DetermineWeaponState();

		float AnimDuration = PlayWeaponAnimation(ReloadAnim);
		if (AnimDuration <= 0.0f)
		{
			AnimDuration = WeaponConfig.NoAnimReloadDuration;
		}

		GetWorldTimerManager().SetTimer(StopReloadHandle, this, &AShooterWeapon::StopReload, AnimDuration, false);
		GetWorldTimerManager().SetTimer(ReloadWeaponHandle, this, &AShooterWeapon::ReloadWeapon, FMath::Max(0.1f, AnimDuration - 0.1f), false);
		
		if (IsLocallyControlled())
		{
			PlayWeaponSound(ReloadSound);
		}
	}
}

void AShooterWeapon::StopReload()
{
	if (bPendingReload)
	{
		bPendingReload = false;
		DetermineWeaponState();
		StopWeaponAnimation(ReloadAnim);
	}
}

bool AShooterWeapon::ServerStartReload_Validate()
{
	return !HasInfiniteClip();
}

void AShooterWeapon::ServerStartReload_Implementation()
{
	StartReload();
}

bool AShooterWeapon::ServerStopReload_Validate()
{
	return !HasInfiniteClip();
}

void AShooterWeapon::ServerStopReload_Implementation()
{
	StopReload();
}

//////////////////////////////////////////////////////////////////////////
// Control

bool AShooterWeapon::CanFire() const
{
	bool bCanFire = MyPawn && MyPawn->CanFire();
	bool bStateOKToFire = ( ( CurrentState ==  EWeaponState::Idle ) || ( CurrentState == EWeaponState::Firing) || ( CurrentState == EWeaponState::Reloading && !bPendingReload ) );	
	return (( bCanFire == true ) && ( bStateOKToFire == true ));
}

bool AShooterWeapon::CanReload() const
{
	return (!HasInfiniteClip() && CurrentAmmoInClip < WeaponConfig.AmmoPerClip && GetCurrentTotalAmmo() > CurrentAmmoInClip);
}

bool AShooterWeapon::HasEnoughAmmo() const
{
	if (HasInfiniteAmmo())
	{
		return true;
	}
	//remote clients (not authority or owner) don't have access to owner's inventory,
	//so assume the owner has enough ammo in their inventory.
	if (GetLocalRole() < ROLE_Authority && MyPawn && !MyPawn->IsLocallyControlled())
	{
		return true;
	}
	return GetCurrentAmmo() >= WeaponConfig.AmmoToConsume[CurrentFireMode];
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage

// on equip/unequip: everyone
// on start fire/stop fire: local + server
void AShooterWeapon::DetermineWeaponState()
{
	EWeaponState::Type NewState = EWeaponState::Idle;

	if (bIsEquipped)
	{
		if (CurrentState == EWeaponState::Equipping)
		{
			//update state, weapon is equipped
			CurrentState = EWeaponState::Idle;
		}
		if (bPendingReload && CanReload())
		{
			NewState = EWeaponState::Reloading;
		}
		else if (bWantsToFire && CanFire())
		{
			NewState = EWeaponState::Firing;
		}
	}
	else if (bPendingEquip)
	{
		NewState = EWeaponState::Equipping;
	}

	SetWeaponState(NewState);
}

void AShooterWeapon::SetWeaponState(EWeaponState::Type NewState)
{
	const EWeaponState::Type PrevState = CurrentState;

	if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
	{
		OnBurstFinished();
	}

	CurrentState = NewState;

	if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
	{
		OnBurstStarted();
	}
}

// local + server
void AShooterWeapon::OnBurstStarted()
{
	UserStartedFiringEvent(CurrentFireMode);
	// start firing, can be delayed to satisfy TimeBetweenShots
	const uint8 i = bIndependentFireModeCooldown ? 0 : CurrentFireMode;
	const bool bTimeIsValid = LastFireTime[i] > 0.f && TimeBetweenShots[CurrentFireMode] > 0.f;
	const bool bOnCooldown = IsOnCooldown(CurrentFireMode);
	if (bTimeIsValid && bOnCooldown)
	{
		const float GameTime = GetWorld()->GetTimeSeconds();
		GetWorldTimerManager().SetTimer(HandleFiringHandle, this, &AShooterWeapon::HandleFiring, LastFireTime[i] + LastFireCooldown[i] - GameTime, false);
	}
	else
	{
		HandleFiring();
	}
}

// local + server
void AShooterWeapon::OnBurstFinished()
{
	// stop firing FX on remote clients
	BurstCounter = 0;

	// stop firing FX locally, unless it's a dedicated server
	if (GetNetMode() != NM_DedicatedServer)
	{
		StopSimulatingWeaponFire();
	}
	
	if (MyPawn && MyPawn->IsAlive())
	{
		if (IsCharging())
		{
			ChargingNotify.bIsCharging = false;
			StopCharging();
		}
		UserStoppedFiringEvent(CurrentFireMode);
	}

	GetWorldTimerManager().ClearTimer(HandleFiringHandle);

	bRefiring = false;
}

// local + server
void AShooterWeapon::HandleFiring()
{
	//if has ammo, fire
	if (HasEnoughAmmo() && CanFire())
	{
		if (GetNetMode() != NM_DedicatedServer && FiringMode[CurrentFireMode] != FM_Charge)
		{
			SimulateWeaponFire();
		}
		FireWeapon();
	}
	//no ammo -- stop effects on server
	else if (MyPawn && BurstCounter > 0)
	{
		OnBurstFinished();
	}
	//no ammo -- stop firing effects and notify local player
	if (IsLocallyControlled() && GetCurrentTotalAmmo() < WeaponConfig.AmmoToConsume[0])
	{
		PlayWeaponSound(OutOfAmmoSound);
		MyPawn->NotifyOutOfAmmo();
		/*AShooterWeapon* NextWeapon = MyPawn->GetWeapon();
		if (NextWeapon != this && bRefiring)
		{
			NextWeapon->CurrentFireMode = CurrentFireMode;
			NextWeapon->bWantsToFire = true;
		}*/
		
		// stop weapon fire FX, but stay in Firing state
		if (bRefiring)
		{
			OnBurstFinished();
		}
	}
	
	if (GetLocalRole() == ROLE_Authority)
	{
		const bool bShouldUpdateAmmo = (HasEnoughAmmo() && CanFire());
		if (bShouldUpdateAmmo)
		{
			// update firing FX on remote clients
			BurstCounter++;
			//check uint8 overflow
			if (BurstCounter == 0)
			{
				BurstCounter++;
			}
		}
	}

	if (IsLocallyControlled())
	{
		// reload after firing last RoundToInt
		if (CurrentAmmoInClip <= 0 && CanReload())
		{
			StartReload();
		}
	}
	
	// setup refire timer
	bRefiring = (CurrentState == EWeaponState::Firing && TimeBetweenShots[CurrentFireMode] > 0.0f && FiringMode[CurrentFireMode] != FM_Charge);
	if (bRefiring)
	{
		GetWorldTimerManager().SetTimer(HandleFiringHandle, this, &AShooterWeapon::HandleFiring, TimeBetweenShots[CurrentFireMode], false);
	}
	TriggerCooldown();
}

void AShooterWeapon::ReloadWeapon()
{
	int32 ClipDelta = FMath::Min(WeaponConfig.AmmoPerClip - CurrentAmmoInClip, MyPawn->GetCurrentAmmo(GetClass(), true) );

	if (ClipDelta > 0)
	{
		CurrentAmmoInClip += ClipDelta;
		if (GetLocalRole() == ROLE_Authority && !HasInfiniteAmmo())
		{
			MyPawn->UseAmmo(GetClass(), ClipDelta);
		}
	}
}

// local + server
void AShooterWeapon::FireWeapon()
{
	uint8 RandomSeed = FMath::Rand();
	//new seed must be different than the previous, to ensure replication
	while (RandomSeed == PreviousSeed)
	{
		RandomSeed = FMath::Rand();
	}
	PreviousSeed = RandomSeed;

	const bool bShouldProcessInstantHit =	(GetLocalRole() == ROLE_Authority && !GetGameState()->bClientSideHitVerification) ||
											(IsLocallyControlled() && GetGameState()->bClientSideHitVerification) ||
											(IsLocallyControlled() && GetLocalRole() == ROLE_Authority);

	if ( FiringMode[CurrentFireMode] == FM_Charge && !IsCharging() )
	{
		ChargingNotify.bIsCharging = true;
		ChargingNotify.RandomSeed = RandomSeed;
		if (IsLocallyControlled())
		{
			StartCharging(RandomSeed);
		}
		if (GetLocalRole() < ROLE_Authority)
		{
			ServerStartCharging(RandomSeed);
		}
	}
	else if ( (FiringMode[CurrentFireMode] == FM_Instant || FiringMode[CurrentFireMode] == FM_Beam) && bShouldProcessInstantHit)
	{
		ProcessInstantHit(RandomSeed);
		// update firing FX on remote clients if function was called on server
		BurstCounter++;
	}
	else if (GetLocalRole() == ROLE_Authority && FiringMode[CurrentFireMode] == FM_Projectile)
	{
		FireProjectile(RandomSeed);
	}
	//adjust weapon dispersion (aiming reticle size)
	if ( !WeaponConfig.OverrideDispersion )
	{
		CurrentFiringDispersion = FMath::Min(WeaponConfig.FiringDispersionMax, CurrentFiringDispersion + WeaponConfig.FiringDispersionIncrement[CurrentFireMode]);
	}
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage helpers

UAudioComponent* AShooterWeapon::PlayWeaponSound(USoundCue* Sound)
{
	UAudioComponent* AC = NULL;
	if (Sound && MyPawn)
	{
		AC = UGameplayStatics::SpawnSoundAttached(Sound, MyPawn->GetRootComponent());
	}

	return AC;
}

float AShooterWeapon::PlayWeaponAnimation(const FWeaponAnim& Animation)
{
	float Duration = 0.0f;
	if (MyPawn)
	{
		if (Animation.Pawn1P)
		{
			Duration = MyPawn->PlayAnimMontage(Animation.Pawn1P);
		}
		if (Animation.Pawn3P)
		{
			Duration = MyPawn->PlayAnimMontage(Animation.Pawn3P);
		}
	}

	return Duration;
}

void AShooterWeapon::StopWeaponAnimation(const FWeaponAnim& Animation)
{
	if (MyPawn)
	{
		if (Animation.Pawn1P)
		{
			MyPawn->StopAnimMontage(Animation.Pawn1P);
		}
		if (Animation.Pawn3P)
		{
			MyPawn->StopAnimMontage(Animation.Pawn3P);
		}
	}
}

FVector AShooterWeapon::GetCameraAim() const
{
	AShooterPlayerController* const PC = GetInstigator() ? GetInstigator()->GetController<AShooterPlayerController>() : NULL;
	AShooterAIController* const AIPC = GetInstigator() ? GetInstigator()->GetController<AShooterAIController>() : NULL;
	
	FVector CamLoc, finalAim;
	FRotator CamRot;

	if (PC)
	{
		PC->GetPlayerViewPoint(CamLoc, CamRot);
		finalAim = CamRot.Vector();
	}
	else if (AIPC && AIPC->GetEnemy())
	{
		AIPC->GetActorEyesViewPoint(CamLoc, CamRot);
		finalAim = (AIPC->GetEnemy()->GetActorLocation() - CamLoc);
		finalAim.Normalize();
		finalAim = MyPawn->GetActorRotation().Vector();
	}
	else if (GetInstigator()) //remote clients don't have access other pawn's controller
	{
		finalAim = GetInstigator()->GetBaseAimRotation().Vector();
	}

	return finalAim;
}

void AShooterWeapon::GetAdjustedAim(FVector& OutAimDir, FVector& OutStartTrace) const
{
	OutAimDir = GetCameraAim();
	
	const FVector MuzzleLocation = GetMuzzleLocation();
	const FVector CameraDamageStartLocation = GetCameraDamageStartLocation(OutAimDir);
	OutStartTrace = CameraDamageStartLocation;
	FVector EndTrace = MuzzleLocation;
	
	//check if the muzzle location is "inside" a wall
	FHitResult Impact = WeaponTrace(OutStartTrace, EndTrace, MyPawn);

	if (Impact.bBlockingHit)
	{
		//There was a hit: the weapon's muzzle location is inside a wall
		//leave OutStartTrace at CameraDamageStartLocation (center of the screen) and do not adjust OutAimDir, and return
		return;
	}

	//else, the muzzle location is not inside a wall... 
	//but it is possible that the weapon is too close to a ledge (for example, a sniper bunker), and will hit the ledge
	//rather than the target if the trace starts from the muzzle. So, detect if this is the case.

	//EndTrace = where the player is aiming at
	EndTrace = CameraDamageStartLocation + OutAimDir * FMath::Max(InstantConfig[CurrentFireMode].WeaponRange, 300.f);
	//find hit from camera's center to target location
	Impact = WeaponTrace( CameraDamageStartLocation, EndTrace, MyPawn);
	if (Impact.bBlockingHit)
	{
		const FVector HitLocation = Impact.Location;
		//now let's see if it's possible to adjust OutStartTrace and Aimdir without hitting a ledge
		//find hit from muzzle location to hit location
		EndTrace = Impact.Location;
		Impact = WeaponTrace(MuzzleLocation, EndTrace, MyPawn);
		if (Impact.bBlockingHit && !HitLocation.Equals(Impact.Location, 10.f))
		{
			//hit a nearby ledge. Again, leave OutStartTrace at CameraDamageStartLocation, do not adjust OutAimDir, and return
			return;
		}
	}

	//otherwise, we are free to adjust aim from muzzle location to target location
	OutStartTrace = MuzzleLocation;
	OutAimDir = EndTrace - OutStartTrace;
	OutAimDir.Normalize();
}

FVector AShooterWeapon::GetCameraDamageStartLocation(const FVector& AimDir) const
{
	AShooterPlayerController* PC = MyPawn ? Cast<AShooterPlayerController>(MyPawn->Controller) : NULL;
	AShooterAIController* AIPC = MyPawn ? Cast<AShooterAIController>(MyPawn->Controller) : NULL;
	FVector OutStartTrace = FVector::ZeroVector;

	if (PC)
	{
		// use player's camera
		FRotator UnusedRot;
		PC->GetPlayerViewPoint(OutStartTrace, UnusedRot);

		// Adjust trace so there is nothing blocking the ray between the camera and the pawn, and calculate distance from adjusted start
		OutStartTrace = OutStartTrace + AimDir * ((GetInstigator()->GetActorLocation() - OutStartTrace) | AimDir);
	}
	else if (AIPC)
	{
		FRotator UnusedRot;
		AIPC->GetActorEyesViewPoint(OutStartTrace, UnusedRot);
		OutStartTrace = OutStartTrace + AimDir * ((GetInstigator()->GetActorLocation() - OutStartTrace) | AimDir);
	}
	else
	{
		OutStartTrace = GetMuzzleLocation();
	}

	return OutStartTrace;
}

FVector AShooterWeapon::GetMuzzleLocation() const
{
	if (MuzzleAttachPoint.Num() == 0)
	{
		UE_LOG(LogShooterWeapon, Warning, TEXT("Weapon %s has no muzzle attach points defined."), *GetNameSafe(this));
		return GetActorLocation();
	}
	USkeletalMeshComponent* UseMesh = GetWeaponMesh();
	if (Effects[CurrentFireMode].MuzzleChooseMethod == EMuzzleChooser::Random)
	{
		return UseMesh->GetSocketLocation(MuzzleAttachPoint[WeaponRandomStream.RandRange(0, MuzzleAttachPoint.Num() - 1)]);
	}
	if (Effects[CurrentFireMode].MuzzleChooseMethod == EMuzzleChooser::Sequential)
	{
		return UseMesh->GetSocketLocation(MuzzleAttachPoint[Effects[CurrentFireMode].CurrentMuzzleIndex]);
	}
	if (Effects[CurrentFireMode].MuzzleChooseMethod == EMuzzleChooser::FireMode)
	{
		if (CurrentFireMode < MuzzleAttachPoint.Num())
		{
			return UseMesh->GetSocketLocation(MuzzleAttachPoint[CurrentFireMode]);
		}
		//else
		UE_LOG(LogShooterWeapon, Warning, TEXT("Weapon %s has MuzzleChooseMethod == FireMode on fire mode %d, but only %d MuzzleAttachPoints."), *GetNameSafe(this), CurrentFireMode, MuzzleAttachPoint.Num());
	}
	return UseMesh->GetSocketLocation(MuzzleAttachPoint[0]);
}

FName AShooterWeapon::GetMuzzleName() const
{
	if (MuzzleAttachPoint.Num() == 0)
	{
		UE_LOG(LogShooterWeapon, Warning, TEXT("Weapon %s has no muzzle attach points defined."), *GetNameSafe(this));
		return FName();
	}
	if (Effects[CurrentFireMode].MuzzleChooseMethod == EMuzzleChooser::Random)
	{
		return MuzzleAttachPoint[WeaponRandomStream.RandRange(0, MuzzleAttachPoint.Num() - 1)];
	}
	if (Effects[CurrentFireMode].MuzzleChooseMethod == EMuzzleChooser::Sequential)
	{
		return MuzzleAttachPoint[Effects[CurrentFireMode].CurrentMuzzleIndex];
	}
	if (Effects[CurrentFireMode].MuzzleChooseMethod == EMuzzleChooser::FireMode)
	{
		if (CurrentFireMode < MuzzleAttachPoint.Num())
		{
			return MuzzleAttachPoint[CurrentFireMode];
		}
		//else
		UE_LOG(LogShooterWeapon, Warning, TEXT("Weapon %s has MuzzleChooseMethod == FireMode on fire mode %d, but only %d MuzzleAttachPoints."), *GetNameSafe(this), CurrentFireMode, MuzzleAttachPoint.Num());
	}
	return MuzzleAttachPoint[0];
}

void AShooterWeapon::IncrementMuzzleIndex()
{
	if (MuzzleAttachPoint.Num() > 0)
	{
		Effects[CurrentFireMode].CurrentMuzzleIndex = (Effects[CurrentFireMode].CurrentMuzzleIndex + 1) % MuzzleAttachPoint.Num();
	}
}

FHitResult AShooterWeapon::WeaponTrace(FVector TraceFrom, FVector TraceTo, AActor* IgnoreActor, float TraceDist) const
{
	if (TraceFrom == FVector::ZeroVector || TraceTo == FVector::ZeroVector)
	{
		FVector AimDir, DmgStartLoc;
		GetAdjustedAim(AimDir, DmgStartLoc);
		if (TraceFrom == FVector::ZeroVector)
		{
			TraceFrom = DmgStartLoc;
		}
		if (TraceTo == FVector::ZeroVector)
		{
			TraceTo = TraceFrom + AimDir * TraceDist;
		}
	}
	
	if (IgnoreActor == NULL)
	{
		IgnoreActor = GetPawnOwner();
	}
	
	static FName WeaponFireTag = FName(TEXT("WeaponTrace"));

	FCollisionQueryParams TraceParams(WeaponFireTag, true, IgnoreActor);
	FHitResult Hit(ForceInit);
	const bool bHit = GetWorld()->SweepSingleByChannel(Hit, TraceFrom, TraceTo, FQuat(ForceInit), COLLISION_WEAPON, FCollisionShape::MakeSphere(InstantConfig[CurrentFireMode].ShotThickness * 0.5f), TraceParams);

	if (!bHit)
	{
		Hit.Location = TraceTo;
		Hit.ImpactPoint = TraceTo;
	}

	return Hit;
}


TArray<FHitResult> AShooterWeapon::WeaponTraceMulti(FVector TraceFrom /*= FVector::ZeroVector*/, FVector TraceTo /*= FVector::ZeroVector*/, AActor* IgnoreActor /*= NULL*/, float TraceDist /*= 100000.0f*/) const
{
	if (TraceFrom == FVector::ZeroVector || TraceTo == FVector::ZeroVector)
	{
		FVector AimDir, DmgStartLoc;
		GetAdjustedAim(AimDir, DmgStartLoc);
		if (TraceFrom == FVector::ZeroVector)
		{
			TraceFrom = DmgStartLoc;
		}
		if (TraceTo == FVector::ZeroVector)
		{
			TraceTo = TraceFrom + AimDir * TraceDist;
		}
	}

	if (IgnoreActor == NULL)
	{
		IgnoreActor = GetPawnOwner();
	}

	static FName WeaponFireTag = FName(TEXT("WeaponTrace"));

	FCollisionQueryParams TraceParams(WeaponFireTag, true, IgnoreActor);

	TArray<FHitResult> Hits;
	const bool bHit = GetWorld()->SweepMultiByChannel(Hits, TraceFrom, TraceTo, FQuat(ForceInit), COLLISION_WEAPON, FCollisionShape::MakeSphere(InstantConfig[CurrentFireMode].ShotThickness * 0.5f), TraceParams);

	if (!bHit)
	{
		FHitResult NewHit(ForceInit);
		NewHit.Location = TraceTo;
		NewHit.ImpactPoint = TraceTo;
		Hits.Add(NewHit);
	}

	return Hits;
}

void AShooterWeapon::SetOwningPawn(AShooterCharacter* NewOwner)
{
	if (MyPawn != NewOwner)
	{
		SetInstigator(NewOwner);
		MyPawn = NewOwner;
		// net owner for RPC calls
		SetOwner(NewOwner);
	}	
}

//////////////////////////////////////////////////////////////////////////
// Replication & effects

void AShooterWeapon::OnRep_Reload()
{
	if (bPendingReload)
	{
		StartReload(true);
	}
	else
	{
		StopReload();
	}
}

void AShooterWeapon::OnRep_BurstCounter()
{
	if (BurstCounter > 0 && FiringMode[CurrentFireMode] != FM_Charge)
	{
		SimulateWeaponFire();
	}
	else
	{
		StopSimulatingWeaponFire();
	}
}

void AShooterWeapon::SimulateWeaponFire()
{
	if (GetLocalRole() == ROLE_Authority && CurrentState != EWeaponState::Firing)
	{
		return;
	}

	if (Effects[CurrentFireMode].MuzzleFX)
	{
		USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
		if (!Effects[CurrentFireMode].bLoopedMuzzleFX || MuzzlePSC == NULL)
		{
			for (uint8 ShotIndex = 0; ShotIndex < ShotsPerTick[CurrentFireMode]; ShotIndex++)
			{
				MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(Effects[CurrentFireMode].MuzzleFX, UseWeaponMesh, GetMuzzleName());
				IncrementMuzzleIndex();
			}
		}
	}

	if (!Effects[CurrentFireMode].bLoopedFireAnim || !bPlayingFireAnim)
	{
		PlayWeaponAnimation(Effects[CurrentFireMode].FireAnim);
		bPlayingFireAnim = true;
	}
	if ( Effects[CurrentFireMode].bLoopedFireSound && FireAC == NULL)
	{
		PlayWeaponSound(Effects[CurrentFireMode].FireSound);
		FireAC = PlayWeaponSound(Effects[CurrentFireMode].FireLoopSound);
	}
	else if (!Effects[CurrentFireMode].bLoopedFireSound)
	{
		PlayWeaponSound(Effects[CurrentFireMode].FireSound);
	}

	if (Effects[CurrentFireMode].FireCameraShake != NULL)
	{
		AShooterPlayerController* PC = (MyPawn != NULL) ? Cast<AShooterPlayerController>(MyPawn->Controller) : NULL;
		if (PC != NULL && PC->IsLocalController())
		{
			PC->ClientPlayCameraShake(Effects[CurrentFireMode].FireCameraShake, 1);
		}
	}

	if (FiringMode[CurrentFireMode] == FM_Beam && Effects[CurrentFireMode].TrailFX && BeamPSC.Num() == 0)
	{
		for (uint8 BounceNum = 0; BounceNum < InstantConfig[CurrentFireMode].Bounces + 1; BounceNum++)
		{
			for (uint8 ShotNum = 0; ShotNum < ShotsPerTick[CurrentFireMode]; ShotNum++)
			{
				UParticleSystemComponent* NewBeamPSC = UGameplayStatics::SpawnEmitterAttached(Effects[CurrentFireMode].TrailFX, GetWeaponMesh());
				if (NewBeamPSC)
				{
					BeamPSC.Add(NewBeamPSC);
					NewBeamPSC->SetHiddenInGame(true);
					TracerCreatedEvent(NewBeamPSC, CurrentFireMode, BounceNum, FVector::ZeroVector, FVector::ZeroVector);
				}
			}
		}
	}
	SimulateWeaponFireEvent(CurrentFireMode, MuzzlePSC);
}

void AShooterWeapon::StopSimulatingWeaponFire()
{
	if (Effects[CurrentFireMode].bLoopedMuzzleFX && MuzzlePSC)
	{
		MuzzlePSC->DeactivateSystem();
		MuzzlePSC = NULL;
	}

	if (Effects[CurrentFireMode].bLoopedFireAnim && bPlayingFireAnim)
	{
		StopWeaponAnimation(Effects[CurrentFireMode].FireAnim);
		bPlayingFireAnim = false;
	}

	if (FireAC)
	{
		FireAC->FadeOut(0.1f, 0.0f);
		FireAC = NULL;

		PlayWeaponSound(Effects[CurrentFireMode].FireLoopFinishSound);
	}
	
	if (FiringMode[CurrentFireMode] == FM_Beam && Effects[CurrentFireMode].TrailFX)
	{
		for (uint8 i=0; i<BeamPSC.Num(); i++)
		{
			if (BeamPSC[i])
			{
				BeamPSC[i]->DeactivateSystem();
				BeamPSC[i] = NULL;
			}
		}
		BeamPSC.Empty();
	}
	StopSimulatingWeaponFireEvent(CurrentFireMode);
}

// local only
void AShooterWeapon::ProcessInstantHit(uint8 RandomSeed)
{
	// if Server: replicate FX on remote clients
	if (GetLocalRole() == ROLE_Authority)
	{
		HitNotifySeed = RandomSeed;
	}
	// if Client: notify server of shot; server will replicate effects
	else if (GetGameState()->bClientSideHitVerification)
	{
		ServerNotifyShot(RandomSeed);
	}

	const float ConeHalfAngle = FMath::DegreesToRadians(GetFiringDispersion() * 0.5f);
	WeaponRandomStream.Initialize(RandomSeed);
	
	FVector AimDir, StartTrace;
	FHitResult Impact;
	FVector EndTrace;
	WeaponPreFireEvent(CurrentFireMode);

	for (uint8 ShotIndex = 0; ShotIndex < ShotsPerTick[CurrentFireMode] && HasEnoughAmmo(); ShotIndex++)
	{
		UseAmmo();
		GetAdjustedAim(AimDir, StartTrace);
		IncrementMuzzleIndex();
		const uint8 NumPellets = InstantConfig[CurrentFireMode].BulletsToSpawn;
		for (uint8 Pellet=0; Pellet < NumPellets; Pellet++)
		{
			const FVector ShootDirFwd = WeaponRandomStream.VRandCone(ForwardVector, ConeHalfAngle, ConeHalfAngle);
			FVector ShootDir = AimDir.Rotation().RotateVector(ShootDirFwd);
			EndTrace = StartTrace + ShootDir * InstantConfig[CurrentFireMode].WeaponRange;
			uint8  Bounce = 0;
			while (Bounce <= InstantConfig[CurrentFireMode].Bounces)
			{
				if (Bounce == 0)
				{
					Impact = WeaponTrace(StartTrace, EndTrace, GetInstigator());
				}
				else
				{
					Impact = WeaponTrace(StartTrace, EndTrace, this);
				}
				if (Impact.bBlockingHit)
				{
					WeaponInstantHitEvent(CurrentFireMode, Impact, MyPawn);
				}

				// handle damage
				if (ShouldDealDamage(Impact.GetActor()))
				{
					DealDamage(Impact, ShootDir, Bounce);
				}
				else if ( ClientShouldNotifyHit(Impact.GetActor()) )
				{
					ServerNotifyInstantHit(Impact, RandomSeed, ShotIndex * NumPellets + Pellet, Bounce);
				}

				// play FX locally
				if (GetNetMode() != NM_DedicatedServer)
				{
					const FVector EndPoint = Impact.bBlockingHit ? Impact.ImpactPoint : EndTrace;

					if (FiringMode[CurrentFireMode] != FM_Beam)
					{
						UParticleSystemComponent* TrailPSC = SpawnTrailEffect(StartTrace, EndPoint, Bounce == 0? Effects[CurrentFireMode].SpawnTrailAttached : false);
						if (TrailPSC)
						{
							TracerCreatedEvent(TrailPSC, CurrentFireMode, Bounce, StartTrace, EndPoint);
						}
					}
					SpawnImpactEffects(Impact);
				}
				if (InstantConfig[CurrentFireMode].Bounces > 0 && Impact.bBlockingHit && Cast<AShooterCharacter>(Impact.GetActor()) == NULL )
				{
					InstantHitBounceEvent(Impact, Bounce+1);
					FVector Velocity = Impact.ImpactPoint - StartTrace;
					Velocity.Normalize();
					ShootDir = -2 * FVector::DotProduct( Velocity, Impact.ImpactNormal )  * Impact.ImpactNormal + Velocity;
					StartTrace = Impact.ImpactPoint + Impact.ImpactNormal * InstantConfig[CurrentFireMode].ShotThickness;
					EndTrace = StartTrace + ShootDir * InstantConfig[CurrentFireMode].WeaponRange;
				}
				else
				{
					Bounce = MAX_uint8 - 1;
				}
				Bounce += 1;
			}
		}
	}
}

bool AShooterWeapon::ServerNotifyShot_Validate(uint8 RandomSeed)
{
	return GetGameState()->bClientSideHitVerification;
}

void AShooterWeapon::ServerNotifyShot_Implementation(uint8 RandomSeed)
{
	// play FX on remote clients
	HitNotifySeed = RandomSeed;
	
	// play effects on server
	SimulateInstantHit(HitNotifySeed);
}

bool AShooterWeapon::ServerNotifyInstantHit_Validate(FHitResult Impact, uint8 RandomSeed, uint8 ShotIndex, uint8 BounceNumber)
{
	return GetGameState()->bClientSideHitVerification;
}

void AShooterWeapon::ServerNotifyInstantHit_Implementation(FHitResult Impact, uint8 RandomSeed, uint8 ShotIndex, uint8 BounceNumber)
{
	if (!ShouldDealDamage(Impact.GetActor()))
	{
		return;
	}
	
	// *** firing mode test ***
	if (!(FiringMode[CurrentFireMode] == FM_Instant || FiringMode[CurrentFireMode] == FM_Beam))
	{
		UE_LOG(LogShooterWeapon, Log, TEXT("%s Rejected client side hit of %s (current firing mode is not FM_Instant or FM_Beam)"), *GetNameSafe(this), *GetNameSafe(Impact.GetActor()));
		return;
	}
	
	// *** CurrentState test ***
	if ( CurrentState != EWeaponState::Firing )
	{
		UE_LOG(LogShooterWeapon, Log, TEXT("%s Rejected client side hit of %s (not in firing state)"), *GetNameSafe(this), *GetNameSafe(Impact.GetActor()));
		return;
	}

	// *** bounce number test ***
	if (BounceNumber > InstantConfig[CurrentFireMode].Bounces)
	{
		UE_LOG(LogShooterWeapon, Log, TEXT("%s Rejected client side hit of %s (reported BounceNumber greater than weapon's InstantConfig.Bounces)"), *GetNameSafe(this), *GetNameSafe(Impact.GetActor()));
		return;
	}

	// *** ammo test ***
	// @TODO

	// *** fire rate test ***
	// @TODO

	// *** weapon dispersion test ***
	// @TODO

	WeaponRandomStream.Initialize(RandomSeed);
	const float WeaponAngleDot = FMath::Abs(FMath::Sin(GetFiringDispersion() * PI / 180.f));
	const float ConeHalfAngle = FMath::DegreesToRadians(GetFiringDispersion() * 0.5f);
	FVector AimDir, StartTrace;
	GetAdjustedAim(AimDir, StartTrace);

	FVector ShootDirFwd;
	for (uint8 i = 0; i <= ShotIndex; i++)
	{
		//call this a number of times equal to ShotIndex, to get the same result as the client had
		ShootDirFwd = WeaponRandomStream.VRandCone(ForwardVector, ConeHalfAngle, ConeHalfAngle);
	}
	FVector ShootDir = AimDir.Rotation().RotateVector(ShootDirFwd);

	// assume the client told the truth about static things,
	// they usually don't have significant gameplay implications
	if (Impact.GetActor()->IsRootComponentStatic() || Impact.GetActor()->IsRootComponentStationary())
	{
		DealDamage(Impact, ShootDir, BounceNumber);
		return;
	}

	// *** view direction test ***	

	//ignore if target is too close (ViewDotHitDir can be very large due to latency)
	const FVector MyLoc = MyPawn ? MyPawn->GetActorLocation() : GetActorLocation();
	const bool TargetIsClose = (Impact.GetActor()->GetActorLocation() - MyLoc).SizeSquared2D() < 150 * 150 ? true : false;
	
	if (!TargetIsClose)
	{
		FVector TargetDirection = Impact.Location - GetCameraDamageStartLocation(AimDir);
		TargetDirection.Normalize();
		// calculate dot between the view and the target
		const float ViewDotHitDir = FVector::DotProduct(TargetDirection, ShootDir);
		// is the angle between the hit and the view within allowed limits (limit + weapon max angle)
		if (ViewDotHitDir <= InstantConfig[CurrentFireMode].AllowedViewDotHitDir - WeaponAngleDot  && BounceNumber == 0)
		{
			UE_LOG(LogShooterWeapon, Log, TEXT("%s Rejected client side hit of %s (facing too far from the hit direction)"), *GetNameSafe(this), *GetNameSafe(Impact.GetActor()));
			return;
		}
	}

	// *** bounding box test ***

	// Get the component bounding box
	const FBox HitBox = Impact.GetActor()->GetComponentsBoundingBox();

	// calculate the box extent, and increase by a leeway
	FVector BoxExtent = 0.5 * (HitBox.Max - HitBox.Min);
	BoxExtent = BoxExtent + InstantConfig[CurrentFireMode].ClientSideHitLeeway;

	// avoid precision errors with really thin objects
	BoxExtent.X = FMath::Max(20.0f, BoxExtent.X);
	BoxExtent.Y = FMath::Max(20.0f, BoxExtent.Y);
	BoxExtent.Z = FMath::Max(20.0f, BoxExtent.Z);

	// Get the box center
	const FVector BoxCenter = (HitBox.Min + HitBox.Max) * 0.5;

	// check that the hit location is within client tolerance
	if (!(FMath::Abs(Impact.Location.Z - BoxCenter.Z) < BoxExtent.Z &&
		FMath::Abs(Impact.Location.X - BoxCenter.X) < BoxExtent.X &&
		FMath::Abs(Impact.Location.Y - BoxCenter.Y) < BoxExtent.Y))
	{
		UE_LOG(LogShooterWeapon, Log, TEXT("%s Rejected client side hit of %s (outside bounding box tolerance)"), *GetNameSafe(this), *GetNameSafe(Impact.GetActor()));
		return;
	}

	// all verification tests passed, accept the hit
	DealDamage(Impact, ShootDir, BounceNumber);
}

bool AShooterWeapon::ShouldDealDamage(AActor* TestActor) const
{
	// if we're an actor on the server, or the actor's role is authoritative, we should register damage
	if (TestActor)
	{
		if (GetNetMode() < NM_Client ||
			TestActor->GetLocalRole() == ROLE_Authority ||
			TestActor->GetTearOff())
		{
			return true;
		}
	}

	return false;
}

bool AShooterWeapon::ClientShouldNotifyHit(AActor* TestActor) const
{
	// if we're the server, ignore
	if (GetLocalRole() == ROLE_Authority)
	{
		return false;
	}
	if (!GetGameState()->bClientSideHitVerification)
	{
		return false;
	}
	if (TestActor)
	{
		//if actor is not authorative and can be damaged, then we should notify server of the hit
		if ( (TestActor->GetLocalRole() < ROLE_Authority || !TestActor->GetTearOff() ) && TestActor->CanBeDamaged())
		{
			return true;
		}
	}
	return false;
}


void AShooterWeapon::DealDamage(const FHitResult& Impact, const FVector& ShootDir, uint8 BounceNumber)
{
	FPointDamageEvent PointDmg;
	PointDmg.DamageTypeClass = InstantConfig[CurrentFireMode].DamageType;
	PointDmg.HitInfo = Impact;
	PointDmg.ShotDirection = ShootDir;
	float BounceDmgMult = 1.0f;
	for (uint8 i = 0; i < BounceNumber; i++)
	{
		BounceDmgMult *= InstantConfig[CurrentFireMode].BounceDamageMultiplier;
	}
	PointDmg.Damage = InstantConfig[CurrentFireMode].HitDamage * BounceDmgMult;

	AShooterCharacter* HitCharacter = Cast<AShooterCharacter>(Impact.GetActor());
	if (HitCharacter && HitCharacter->HeadBoneNames.Find(Impact.BoneName) != INDEX_NONE)
	{
		PointDmg.Damage *= InstantConfig[CurrentFireMode].HeadshotDamageMultiplier;
		Impact.GetActor()->TakeDamage(PointDmg.Damage, PointDmg, MyPawn->Controller, this);
	}
	else
	{
		Impact.GetActor()->TakeDamage(PointDmg.Damage, PointDmg, MyPawn->Controller, this);
	}

	WeaponDealtDamageEvent(Impact, PointDmg, ShootDir, CurrentFireMode, BounceNumber);
}

void AShooterWeapon::OnRep_HitNotify()
{
	if (FiringMode[CurrentFireMode] == FM_Projectile)
	{
		FireProjectile(HitNotifySeed);
	}
	else if (FiringMode[CurrentFireMode] == FM_Instant || FiringMode[CurrentFireMode] == FM_Beam || FiringMode[CurrentFireMode] == FM_Charge)
	{
		if (IsLocallyControlled() && GetGameState() && GetGameState()->bClientSideHitVerification)
		{
			//do not play effects if is local player and ClientSideHitVerification=true, as local player will have processed instant hit and effects already
			return;
		}
		SimulateInstantHit(HitNotifySeed);
	}
}

void AShooterWeapon::OnRep_ChargingNotify()
{
	if (ChargingNotify.bIsCharging)
	{
		StartCharging(ChargingNotify.RandomSeed);
	}
	else
	{
		StopCharging();
	}
}

void AShooterWeapon::SimulateInstantHit(uint8 RandomSeed)
{
	WeaponRandomStream.Initialize(RandomSeed);
	const float ConeHalfAngle = FMath::DegreesToRadians(GetFiringDispersion()  * 0.5f);
	
	FVector AimDir, StartTrace;
	FHitResult Impact;
	FVector EndTrace;

	WeaponPreFireEvent(CurrentFireMode);

	for (uint8 ShotIndex = 0; ShotIndex < ShotsPerTick[CurrentFireMode]; ShotIndex++)
	{
		UseAmmo();
		GetAdjustedAim(AimDir, StartTrace);
		IncrementMuzzleIndex();
		for (uint8 Pellet=0; Pellet < InstantConfig[CurrentFireMode].BulletsToSpawn; Pellet++)
		{
			const FVector ShootDirFwd = WeaponRandomStream.VRandCone(ForwardVector, ConeHalfAngle, ConeHalfAngle);
			FVector ShootDir = AimDir.Rotation().RotateVector(ShootDirFwd);
			EndTrace = StartTrace + ShootDir * InstantConfig[CurrentFireMode].WeaponRange;
			uint8  Bounce = 0;
			while (Bounce <= InstantConfig[CurrentFireMode].Bounces)
			{
				if (Bounce == 0)
				{
					Impact = WeaponTrace(StartTrace, EndTrace, GetInstigator());
				}
				else
				{
					Impact = WeaponTrace(StartTrace, EndTrace, this);
				}
				const FVector EndPoint = Impact.bBlockingHit ? Impact.ImpactPoint : EndTrace;
				if (Impact.bBlockingHit)
				{
					WeaponInstantHitEvent(CurrentFireMode, Impact, MyPawn);
					SpawnImpactEffects(Impact);
				}
				// deal damage to non-replicated actors (ragdolls, etc)
				if (Impact.GetActor() && Impact.GetActor()->GetTearOff())
				{
					DealDamage(Impact, ShootDir, Bounce);
				}

				if (FiringMode[CurrentFireMode] != FM_Beam)
				{
					UParticleSystemComponent* TrailPSC = SpawnTrailEffect(StartTrace, EndPoint, Bounce == 0? Effects[CurrentFireMode].SpawnTrailAttached : false);
					if (TrailPSC)
					{
						TracerCreatedEvent(TrailPSC, CurrentFireMode, Bounce, StartTrace, EndPoint);
					}
				}
				if (InstantConfig[CurrentFireMode].Bounces > 0 && Impact.bBlockingHit && Cast<AShooterCharacter>(Impact.GetActor()) == NULL )
				{
					InstantHitBounceEvent(Impact, Bounce+1);
					FVector Velocity = Impact.ImpactPoint - StartTrace;
					Velocity.Normalize();
					ShootDir = -2 * FVector::DotProduct( Velocity, Impact.ImpactNormal )  * Impact.ImpactNormal + Velocity;
					StartTrace = Impact.ImpactPoint + Impact.ImpactNormal * InstantConfig[CurrentFireMode].ShotThickness;
					EndTrace = StartTrace + ShootDir * InstantConfig[CurrentFireMode].WeaponRange;
				}
				else
				{
					Bounce = MAX_uint8 - 1;
				}
				Bounce += 1;
			}
		}
	}
}


void AShooterWeapon::SpawnImpactEffects(const FHitResult& Impact)
{
	if (Effects[CurrentFireMode].ImpactTemplate && (Impact.bBlockingHit || Impact.bStartPenetrating || Impact.Component != NULL) )
	{
		FHitResult UseImpact = Impact;

		// trace again to find component lost during replication
		if (!Impact.Component.IsValid())
		{
			const FVector StartTrace = Impact.ImpactPoint + Impact.ImpactNormal * 10.0f;
			const FVector EndTrace = Impact.ImpactPoint - Impact.ImpactNormal * 10.0f;
			FHitResult Hit = WeaponTrace(StartTrace, EndTrace, GetInstigator());
			UseImpact = Hit;
		}

		FTransform const SpawnTransform(Impact.ImpactNormal.Rotation(), Impact.ImpactPoint);
		AShooterImpactEffect* EffectActor = GetWorld()->SpawnActorDeferred<AShooterImpactEffect>(Effects[CurrentFireMode].ImpactTemplate, SpawnTransform);
		if (EffectActor)
		{
			EffectActor->SurfaceHit = UseImpact;
			UGameplayStatics::FinishSpawningActor(EffectActor, SpawnTransform);
		}
	}
}

UParticleSystemComponent* AShooterWeapon::SpawnTrailEffect(const FVector& StartPoint, const FVector& EndPoint, bool SpawnTrailAttached)
{
	UParticleSystemComponent* TrailPSC;
	if (Effects[CurrentFireMode].TrailFX)
	{
		if (SpawnTrailAttached)
		{
			TrailPSC = UGameplayStatics::SpawnEmitterAttached(Effects[CurrentFireMode].TrailFX, GetWeaponMesh(), GetMuzzleName());
		}
		else
		{
			TrailPSC = UGameplayStatics::SpawnEmitterAtLocation(this, Effects[CurrentFireMode].TrailFX, StartPoint, (EndPoint - StartPoint).Rotation());
		}
		if (TrailPSC)
		{
			TrailPSC->SetVectorParameter(TrailTargetParam, EndPoint);
			return TrailPSC;
		}
	}
	return NULL;
}

// server only if bReplicateProjectiles == true
// everyone otherwise
void AShooterWeapon::FireProjectile(uint8 RandomSeed)
{
	//manually replicate projectile to other clients if necessary
	if (GetLocalRole() == ROLE_Authority && !GetGameState()->bReplicateProjectiles)
	{
		HitNotifySeed = RandomSeed;
	}

	//WeaponPreFireEvent() will be called in the projectile's initialization

	WeaponRandomStream.Initialize(RandomSeed);
	
	const float ConeHalfAngle = FMath::DegreesToRadians(GetFiringDispersion()  * 0.5f);
	FVector AimDir, Origin;
	
	const int32 NumProjectiles = ShotsPerTick[CurrentFireMode];
	for (int32 i=0; i < NumProjectiles && HasEnoughAmmo(); i++)
	{
		UseAmmo();
		GetAdjustedAim(AimDir, Origin);
		const FVector ShootDirFwd = WeaponRandomStream.VRandCone(ForwardVector, ConeHalfAngle, ConeHalfAngle);
		FVector ShootDir = AimDir.Rotation().RotateVector(ShootDirFwd);
		IncrementMuzzleIndex();

		FTransform SpawnTM(ShootDir.Rotation(), Origin);
		AShooterProjectile* Projectile = Cast<AShooterProjectile>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, ProjectileClass[CurrentFireMode], SpawnTM));
		if (Projectile)
		{
			Projectile->InitProjectile(GetInstigator(), RandomSeed, this, ShootDir);
			UGameplayStatics::FinishSpawningActor(Projectile, SpawnTM);
		}
	}
}

void AShooterWeapon::SetProjectileClass(uint8 FireModeIndex, TSubclassOf<class AShooterProjectile> NewClass) 
{
	ProjectileClass[FireModeIndex] = NewClass;
}

void AShooterWeapon::SetFiringMode(uint8 FireModeIndex, EFireMode NewFireMode)
{
	if (FireModeIndex >= NUM_FIRING_MODES || GetLocalRole() < ROLE_Authority)
	{
		return;
	}
	FiringMode[FireModeIndex] = NewFireMode;
}

USkeletalMeshComponent* AShooterWeapon::GetWeaponMesh() const
{
	return (MyPawn != NULL && MyPawn->IsFirstPerson()) ? Mesh1P : Mesh3P;
}

class AShooterCharacter* AShooterWeapon::GetPawnOwner() const
{
	return MyPawn;
}

bool AShooterWeapon::IsEquipped() const
{
	return bIsEquipped;
}

bool AShooterWeapon::IsAttachedToPawn() const
{
	return bIsEquipped || bPendingEquip;
}

EWeaponState::Type AShooterWeapon::GetCurrentState() const
{
	return CurrentState;
}

int32 AShooterWeapon::GetMaxAmmo(bool GetMaxClipAmmo) const
{
	if (GetMaxClipAmmo && !HasInfiniteClip())
	{
		return WeaponConfig.AmmoPerClip;
	}
	return WeaponConfig.MaxAmmo;
}

bool AShooterWeapon::HasInfiniteAmmo() const
{
	const AShooterPlayerController* MyPC = (MyPawn != NULL) ? Cast<const AShooterPlayerController>(MyPawn->Controller) : NULL;
	return WeaponConfig.bInfiniteAmmo || (MyPC && MyPC->HasInfiniteAmmo());
}

float AShooterWeapon::GetEquipStartedTime() const
{
	return EquipStartedTime;
}

float AShooterWeapon::GetEquipDuration() const
{
	return EquipDuration;
}

USkeletalMeshComponent* AShooterWeapon::GetWeaponMesh1P() const
{
	return Mesh1P;
}

USkeletalMeshComponent* AShooterWeapon::GetWeaponMesh3P() const
{
	return Mesh3P;
}

int32 AShooterWeapon::GetCurrentAmmo() const
{
	if (MyPawn)
	{
		return MyPawn->GetCurrentAmmo(GetClass(), false);
	}
	return 0;
}


int32 AShooterWeapon::GetCurrentTotalAmmo() const
{
	if (MyPawn)
	{
		return MyPawn->GetCurrentAmmo(GetClass(), true) + CurrentAmmoInClip;
	}
	return HasInfiniteClip() ? 0 : CurrentAmmoInClip;
}

int32 AShooterWeapon::GetInitialAmmo() const
{
	return WeaponConfig.InitialAmmo;
}

bool AShooterWeapon::HasInfiniteClip() const
{
	const AShooterPlayerController* MyPC = (MyPawn != NULL) ? MyPawn->GetController<AShooterPlayerController>() : NULL;
	return WeaponConfig.bInfiniteClip || (MyPC && MyPC->HasInfiniteClip());
}

float AShooterWeapon::GetFireRate(uint8 FireModeIndex) const
{
	return TimeBetweenShots[FireModeIndex];
}

void AShooterWeapon::SetFireRate(uint8 FireModeIndex, float NewTimeBetweenShots)
{
	TimeBetweenShots[FireModeIndex] = NewTimeBetweenShots;
}

void AShooterWeapon::SetCrosshair(UTexture2D* NewCrosshair)
{
	Crosshair = NewCrosshair;
	AShooterPlayerController* MyPC = (MyPawn != NULL) ? MyPawn->GetController<AShooterPlayerController>() : NULL;
	if (MyPC)
	{
		MyPC->CrosshairChanged(Crosshair);
	}
}

bool AShooterWeapon::WantsToFire() const
{
	return bWantsToFire;
}

void AShooterWeapon::IncrementFiringDispersion(float Increment)
{
	CurrentFiringDispersion = FMath::Clamp(CurrentFiringDispersion + Increment, WeaponConfig.BaseFiringDispersion, WeaponConfig.FiringDispersionMax);
}

void AShooterWeapon::SetCurrentFiringDispersion(float NewDispersion)
{
	CurrentFiringDispersion = FMath::Clamp(NewDispersion, WeaponConfig.BaseFiringDispersion, WeaponConfig.FiringDispersionMax);
}

float AShooterWeapon::GetFiringDispersion() const
{
	return CurrentFiringDispersion;
}

void AShooterWeapon::SetInstantHitDamage(uint8 FireMode, int32 NewDamage)
{
	check(FireMode < NUM_FIRING_MODES);
	InstantConfig[FireMode].HitDamage = NewDamage;
}

void AShooterWeapon::SetInstantHitDamageType(uint8 FireMode, TSubclassOf<UShooterDamageType> NewDamageType)
{
	check(FireMode < NUM_FIRING_MODES);
	InstantConfig[FireMode].DamageType = NewDamageType;
}

void AShooterWeapon::SetFireSound(USoundCue* NewSound, uint8 FireMode)
{
	check(FireMode < NUM_FIRING_MODES);
	Effects[FireMode].FireSound = NewSound;
}

void AShooterWeapon::SetTrailFX (UParticleSystem* NewTrailFX, uint8 FireMode)
{
	check(FireMode < NUM_FIRING_MODES);
	Effects[FireMode].TrailFX = NewTrailFX;
}

int32 AShooterWeapon::GetInstantHitDamage(uint8 FireMode) const
{
	check(FireMode < NUM_FIRING_MODES);
	return InstantConfig[FireMode].HitDamage;
}

TEnumAsByte<enum EFireMode> AShooterWeapon::GetFiringMode(uint8 FireModeIndex) const
{
	check(FireModeIndex < NUM_FIRING_MODES);
	return FiringMode[FireModeIndex];
}

FInstantWeaponData AShooterWeapon::GetInstantConfig(uint8 FireMode) const
{
	check(FireMode < NUM_FIRING_MODES);
	return InstantConfig[FireMode];
}

// everyone
void AShooterWeapon::HandleCharging()
{
	if (GetChargingTime() >= GetMaxChargingTime())
	{
		WeaponMaxChargeReachedEvent(CurrentFireMode, GetMaxChargingTime());
		GetWorldTimerManager().ClearTimer(HandleChargingHandle);
	}
	else if (HasEnoughAmmo())
	{
		UseAmmo();
	}
	else //no ammo to keep charging
	{
		StopCharging();
	}
}

// everyone
void AShooterWeapon::StartCharging(uint8 RandomSeed)
{
	WeaponRandomStream.Initialize(RandomSeed);
	ChargeRandomAdd = WeaponRandomStream.FRandRange(0.f, ChargeRandomDisturbance);
	TimeStartedCharging = GWorld->GetTimeSeconds();
	WeaponStartChargeEvent(CurrentFireMode);
	GetWorldTimerManager().SetTimer(HandleChargingHandle, this, &AShooterWeapon::HandleCharging, ChargeAmmoTimer, true);
}

// everyone
void AShooterWeapon::StopCharging()
{
	//if (GetWorldTimerManager().IsTimerActive(this, &AShooterWeapon::HandleCharging))
	{
		ChargingNotify.bIsCharging = false;
		WeaponStopChargeEvent(CurrentFireMode, GetChargingTime());
		TriggerCooldown();
		SimulateWeaponFire();
		GetWorldTimerManager().ClearTimer(HandleChargingHandle);
	}
}

float AShooterWeapon::GetChargingTime() const
{
	const float ChargeTime = GWorld->GetTimeSeconds() - TimeStartedCharging;
	return (ChargeTime > GetMaxChargingTime()) ? GetMaxChargingTime() : ChargeTime;
}

float AShooterWeapon::GetChargingPercent() const
{
	return GetChargingTime() / GetMaxChargingTime();
}

float AShooterWeapon::GetMaxChargingTime() const
{
	return MaxChargeTime + ChargeRandomAdd;
}

AController* AShooterWeapon::GetPawnOwnerController() const
{
	return MyPawn ? MyPawn->GetController() : NULL;
}

bool AShooterWeapon::ServerStartCharging_Validate(uint8 RandomSeed)
{
	return true;
}

void AShooterWeapon::ServerStartCharging_Implementation(uint8 RandomSeed)
{
	ChargingNotify.bIsCharging = true;
	ChargingNotify.RandomSeed = RandomSeed;
	StartCharging(RandomSeed);
}

// everyone
void AShooterWeapon::UpdateBeam()
{
	if (BeamPSC.Num() == 0)
	{
		return;
	}
	for (uint8 ShotIndex = 0; ShotIndex < ShotsPerTick[CurrentFireMode]; ShotIndex++)
	{
		FVector ShootDir, StartTrace;
		FHitResult Impact;
		FVector EndTrace;
		GetAdjustedAim(ShootDir, StartTrace);
		IncrementMuzzleIndex();
		EndTrace = StartTrace + ShootDir * InstantConfig[CurrentFireMode].WeaponRange;
		uint8  Bounce, BeamPSCIndex;
		BeamPSCIndex = (InstantConfig[CurrentFireMode].Bounces+1) * ShotIndex;
		for (Bounce=0; Bounce <= InstantConfig[CurrentFireMode].Bounces; ++Bounce)
		{
			if (BeamPSC[BeamPSCIndex])
			{
				BeamPSC[BeamPSCIndex]->SetHiddenInGame(true);
			}
			BeamPSCIndex += 1;
		}
		Bounce = 0;
		BeamPSCIndex = (InstantConfig[CurrentFireMode].Bounces+1) * ShotIndex;
		while (Bounce <= InstantConfig[CurrentFireMode].Bounces)
		{
			if (BeamPSC[BeamPSCIndex])
			{
				if (Bounce == 0)
				{
					Impact = WeaponTrace(StartTrace, EndTrace, GetInstigator(), InstantConfig[CurrentFireMode].WeaponRange);
				}
				else
				{
					Impact = WeaponTrace(StartTrace, EndTrace, this, InstantConfig[CurrentFireMode].WeaponRange);
				}
				BeamPSC[BeamPSCIndex]->SetHiddenInGame(false);
				BeamPSC[BeamPSCIndex]->SetVectorParameter(TrailSourceParam, StartTrace);
				if (Impact.bBlockingHit)
				{
					BeamPSC[BeamPSCIndex]->SetVectorParameter(TrailTargetParam, Impact.ImpactPoint);
					//DrawDebugSphere(GetWorld(), Impact.ImpactPoint, 50.f, 7, FColor::Red, false, 1 / 60.f);
				}
				else
				{
					BeamPSC[BeamPSCIndex]->SetVectorParameter(TrailTargetParam, EndTrace);
					//DrawDebugSphere(GetWorld(), EndTrace, 50.f, 7, FColor::Red, false, 1 / 60.f);
				}
				if (InstantConfig[CurrentFireMode].Bounces > 0 && Impact.bBlockingHit && Cast<AShooterCharacter>(Impact.GetActor()) == NULL )
				{
					FVector Velocity = Impact.ImpactPoint - StartTrace;
					Velocity.Normalize();
					ShootDir = -2 * FVector::DotProduct( Velocity, Impact.ImpactNormal )  * Impact.ImpactNormal + Velocity;
					StartTrace = Impact.ImpactPoint + Impact.ImpactNormal * InstantConfig[CurrentFireMode].ShotThickness;
					EndTrace = StartTrace + ShootDir * InstantConfig[CurrentFireMode].WeaponRange;
				}
				else
				{
					Bounce = MAX_uint8 - 1;
				}
			}
			Bounce += 1;
			BeamPSCIndex += 1;
		}
	}
}

void AShooterWeapon::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME(AShooterWeapon, FiringMode);
	DOREPLIFETIME(AShooterWeapon, HitNotifySeed);
	
	DOREPLIFETIME_CONDITION(AShooterWeapon, TimeBetweenShots, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AShooterWeapon, CurrentFiringDispersion, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AShooterWeapon, ShotsPerTick, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AShooterWeapon, CurrentAmmoInClip, COND_OwnerOnly);

	DOREPLIFETIME_CONDITION(AShooterWeapon, CurrentFireMode, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AShooterWeapon, BurstCounter, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AShooterWeapon, ChargingNotify, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AShooterWeapon, bPendingReload, COND_SkipOwner);
}

void AShooterWeapon::TriggerCooldown()
{
	const uint8 i = bIndependentFireModeCooldown ? 0 : CurrentFireMode;
	LastFireTime[i] = GetWorld()->GetTimeSeconds();
	LastFireCooldown[i] = TimeBetweenShots[CurrentFireMode];
}

bool AShooterWeapon::IsOnCooldown(uint8 FireMode)
{
	const uint8 i = bIndependentFireModeCooldown ? 0 : CurrentFireMode;
	return LastFireTime[i] + LastFireCooldown[i] > GetWorld()->GetTimeSeconds();
}

void AShooterWeapon::SetShotsPerTick(uint8 FireMode, uint8 NewShotsPerTick)
{
	if (FireMode >= NUM_FIRING_MODES)
	{
		UE_LOG(LogShooterWeapon, Warning, TEXT("Weapon %s called SetShotsPerTick() on invalid FireMode index (%d out of %d)."), *GetNameSafe(this), FireMode, NUM_FIRING_MODES-1);
		return;
	}
	ShotsPerTick[FireMode] = NewShotsPerTick;
}

uint8 AShooterWeapon::GetShotsPerTick(uint8 FireMode) const
{
	if (FireMode >= NUM_FIRING_MODES)
	{
		UE_LOG(LogShooterWeapon, Warning, TEXT("Weapon %s called GetShotsPerTick() on invalid FireMode index (%d out of %d)."), *GetNameSafe(this), FireMode, NUM_FIRING_MODES-1);
		return 1;
	}
	return ShotsPerTick[FireMode];
}

FWeaponEffects AShooterWeapon::GetWeaponEffects(uint8 FireMode) const
{
	if (FireMode >= NUM_FIRING_MODES)
	{
		UE_LOG(LogShooterWeapon, Warning, TEXT("Weapon %s called GetWeaponEffects() on invalid FireMode index (%d out of %d)."), *GetNameSafe(this), FireMode, NUM_FIRING_MODES-1);
		return FWeaponEffects();
	}
	return Effects[FireMode];
}

bool AShooterWeapon::IsLocallyControlled() const
{
	return MyPawn && MyPawn->IsLocallyControlled();
}

bool AShooterWeapon::IsFirstPerson() const
{
	return MyPawn && MyPawn->IsFirstPerson();
}

AShooterGameState* AShooterWeapon::GetGameState() const
{ 
	return GetWorld()->GetGameState<AShooterGameState>(); 
}