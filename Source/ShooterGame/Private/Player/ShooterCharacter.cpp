// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "ShooterCharacter.h"
#include "ShooterPickup.h"
#include "Animation/AnimMontage.h"
#include "ShooterLocalPlayer.h"
#include "ShooterAnimInstance.h"
#include "ShooterProjectile.h"

AShooterCharacter::AShooterCharacter(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UShooterCharacterMovement>(ACharacter::CharacterMovementComponentName))
{
	Mesh1P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("PawnMesh1P"));
	Mesh1P->SetupAttachment(GetCapsuleComponent());
	Mesh1P->bOnlyOwnerSee = true;
	Mesh1P->bOwnerNoSee = false;
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->bReceivesDecals = false;
	Mesh1P->ViewOwnerDepthPriorityGroup = SDPG_Foreground;
	Mesh1P->DepthPriorityGroup = SDPG_Foreground;
	Mesh1P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	//Mesh1P is attached to the camera, and should tick after its update
	Mesh1P->PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	Mesh1P->SetCollisionProfileName(FName("NoCollision"));
	Mesh1P->SetGenerateOverlapEvents(false);

	GetMesh()->SetupAttachment(GetCapsuleComponent());
	GetMesh()->bOnlyOwnerSee = false;
	GetMesh()->bOwnerNoSee = true;
	GetMesh()->bReceivesDecals = false;
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetIsReplicated(true);
	}

	bIsTargeting = false;
	bWantsToRun = false;
	LowHealthPercentage = 0.5f;
	MaxLandedDamageVelocity = 3000.0f;
	MinLandedDamageVelocity = 2000.f;
	LastWallDodgeTime = 0.f;
	LastDodgeTime = 0.f;

	Health = 100.f;
	Armor = 0.f;
	MaxArmor = 100.f;
	MaxBoostedHealth = 200;
	MaxMana = 100.f;
	Mana = MaxMana;

	WeaponAttachPoint = TEXT("WeaponPoint");
	WeaponPriority.SetNumUninitialized(0);
	DefaultInventoryClasses.SetNumUninitialized(0);
	bHasInfiniteAmmo = false;
	bNeverDropInventory = false;
	DodgeMomentum = 1024.f;
	DodgeZ = 0.4f;
	WallDodgeZ = 0.2f;
	MinDodgeInterval = 1.f;
	MinWallDodgeInterval = 0.3f;
	WallDodgeMomentum = 1024.f;
	AIControllerClass = AShooterAIController::StaticClass();
	DeathSound = NULL;
	bKicking = false;
	DamageScale = 1.f;
	AnyPowerupActive = false;
	HeadBoneNames.Add(FName("b_head"));
	HeadBoneNames.Add(FName("b_neck"));
	bShouldRespawn = true;

	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;
}

/*void AShooterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}*/

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() == ROLE_Authority)
	{
		Health = GetMaxHealth();
	}
	//initialize the AllMeshes array
	GetComponents<USkeletalMeshComponent>(AllMeshes);

	CreateMeshMIDs();

	// respawn effects
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (RespawnFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(this, RespawnFX, GetActorLocation(), GetActorRotation());
		}

		if (RespawnSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, RespawnSound, GetActorLocation());
		}
	}

	if (bUpdateAimingDispersion)
	{
		CurrentAimingDispersion = MinAimingDispersion;
	}
}

void AShooterCharacter::CreateMeshMIDs()
{
	for (USkeletalMeshComponent* TheMesh : AllMeshes)
	{
		for (int32 iMat = 0; iMat < TheMesh->GetNumMaterials(); iMat++)
		{
			if (TheMesh->GetMaterial(iMat) != NULL)
			{
				MeshMIDs.Add(TheMesh->CreateAndSetMaterialInstanceDynamic(iMat));
			}
		}
	}
}

void AShooterCharacter::Destroyed()
{
	Super::Destroyed();
	DestroyInventory();
}

void AShooterCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	// switch mesh to 1st person view
	UpdatePawnMeshes();

	// reattach weapon if needed
	SetCurrentWeapon(CurrentWeapon);

	// set team colors for 1st person view
	//UMaterialInstanceDynamic* Mesh1PMID = Mesh1P->CreateAndSetMaterialInstanceDynamic(0);
	//UpdatePlayerColors(Mesh1PMID);

	AShooterPlayerController* PC = Cast<AShooterPlayerController>(Controller);
	if (PC)
	{
		PC->SetZoomLevel(0.0, 10.0);
	}
	PawnRestartedEvent();
}

void AShooterCharacter::PossessedBy(class AController* InController)
{
	Super::PossessedBy(InController);
	UpdatePawnMeshes();
	SpawnDefaultInventory();
	OnPawnPossessed(InController);
}

void AShooterCharacter::UnPossessed()
{
	Super::UnPossessed();
	UpdatePawnMeshes();
}

bool PlayerColorsAreDefaults(AShooterPlayerState* PS)
{
	return PS->GetColor(0) == FColor::White && PS->GetColor(1) == FColor::White && PS->GetColor(2) == FColor::White;
}

void AShooterCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// [client] as soon as GetPlayerState<AShooterPlayerState>() is assigned, set colors of this pawn for local player
	if (GetPlayerState<AShooterPlayerState>() != NULL)
	{
		UpdatePlayerColorsAllMIDs();
		AShooterPlayerState* PS = GetPlayerState<AShooterPlayerState>();
		UShooterPersistentUser* PU = GetPersistentUser();
		if (PS)
		{
			//PS->SetPlayerName(PU->GetPlayerName());
			if (PlayerColorsAreDefaults(PS))
			{
				//player colors may have not been replicated yet, set a timer and try again
				GetWorldTimerManager().SetTimer(UpdatePlayerColorsAllMIDsHandle, this, &AShooterCharacter::UpdatePlayerColorsAllMIDs, 2.f, false);
			}
			OnPlayerStateReplicated(PS);
		}
	}
}

FRotator AShooterCharacter::GetAimOffsets() const
{
	const FVector AimDirWS = GetBaseAimRotation().Vector();
	const FVector AimDirLS = ActorToWorld().InverseTransformVectorNoScale(AimDirWS);
	const FRotator AimRotLS = AimDirLS.Rotation();

	return AimRotLS;
}

bool AShooterCharacter::IsEnemyFor(AController* TestPC) const
{
	if (TestPC == Controller || TestPC == NULL)
	{
		return false;
	}

	AShooterPlayerState* TestPlayerState = TestPC->GetPlayerState<AShooterPlayerState>();
	AShooterPlayerState* MyPlayerState = GetPlayerState<AShooterPlayerState>();

	if (GetWorld()->GetGameState() && GetWorld()->GetGameState()->GameModeClass)
	{
		const AShooterGameMode* DefGame = GetWorld()->GetGameState()->GameModeClass->GetDefaultObject<AShooterGameMode>();
		if (DefGame && MyPlayerState && TestPlayerState)
		{
			return DefGame->CanDealDamage(TestPlayerState, MyPlayerState);
		}
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////
// Meshes

void AShooterCharacter::UpdatePawnMeshes()
{
	//attach first person arms+weapon to the camera, to follow its movement
	AShooterPlayerController* PC = Cast<AShooterPlayerController>(Controller);
	if (PC && PC->PlayerCameraManager)
	{
		Mesh1P->AttachToComponent(PC->PlayerCameraManager->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		USkeletalMeshComponent* DefMesh1P = Cast<USkeletalMeshComponent>(GetClass()->GetDefaultSubobjectByName(TEXT("PawnMesh1P")));
		Mesh1P->SetRelativeLocation(DefMesh1P->GetRelativeLocation());
		Mesh1P->SetRelativeRotation(DefMesh1P->GetRelativeRotation());
	}

	if (GetController() && GetController()->IsLocalPlayerController())
	{
		UShooterAnimInstance* Animation;
		Animation = Cast<UShooterAnimInstance>(Mesh1P->GetAnimInstance());
		if (Animation)
		{
			Animation->bProcessAnimNotifies = false;
		}
	}
	UpdatePlayerColorsAllMIDs();
	UpdatePawnMeshes_BP();
}

void AShooterCharacter::UpdatePlayerColorsAllMIDs()
{
	for (int32 i = 0; i < MeshMIDs.Num(); ++i)
	{
		UpdatePlayerColors(MeshMIDs[i]);
	}
}

void AShooterCharacter::UpdatePlayerColors(UMaterialInstanceDynamic* UseMID)
{
	if (UseMID)
	{
		AShooterPlayerState* PS = GetPlayerState<AShooterPlayerState>();
		if (PS)
		{
			const int32 NumColors = PS->GetNumColors();
			for (int32 i=0; i < NumColors; i++)
			{
				const FName ParmName = FName(*(TEXT("Color") +  FString::FromInt(i+1)));
				UseMID->SetVectorParameterValue(ParmName, PS->GetColor(i));
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Damage & death


void AShooterCharacter::FellOutOfWorld(const class UDamageType& dmgType)
{
	if (IsAlive())
	{
		Die(Health, FDamageEvent(dmgType.GetClass()), NULL, NULL);
	}
}

void AShooterCharacter::Suicide()
{
	KilledBy(this);
}

void AShooterCharacter::KilledBy(APawn* EventInstigator)
{
	if (GetLocalRole() == ROLE_Authority && !bIsDying)
	{
		AController* Killer = NULL;
		if (EventInstigator != NULL)
		{
			Killer = EventInstigator->Controller;
			LastHitBy = NULL;
		}

		Die(Health, FDamageEvent(UDamageType::StaticClass()), Killer, NULL);
	}
}

float AShooterCharacter::GiveArmor(float Amount)
{
	const float PrevArmor = Armor;
	Armor = FMath::Min(Armor + Amount, MaxArmor);
	return Armor - PrevArmor;
}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->HasGodMode())
	{
		return 0.f;
	}
	AShooterGameMode* const Game = GetWorld()->GetAuthGameMode<AShooterGameMode>();
	if (Game && Game->GetMatchState() != MatchState::InProgress)
	{
		return 0.f;
	}
	// Modify based on game rules.
	Damage = Game ? Game->ModifyDamage(Damage, this, EventInstigator) : Damage;
	if (EventInstigator)
	{
		AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(EventInstigator->GetPawn());
		const bool bEnvironmentalDamage = DamageCauser == NULL;
		if (ShooterChar && !bEnvironmentalDamage)
		{
			Damage *= ShooterChar->DamageScale;
		}
	}

	float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	LastHitTime = GetWorld()->GetTimeSeconds();
	if (ActualDamage > 0.f || Health < 1.0f)
	{
		const float ArmorAbsorbedDamage = FMath::Min(ActualDamage * 0.5f, Armor);
		Armor = FMath::Max(0.f, Armor - ActualDamage);
		ActualDamage -= ArmorAbsorbedDamage;
		Health -= ActualDamage;

		if (Health < 1.0f && !bIsDying)
		{
			Die(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
		}
		else
		{
			PlayHit(ActualDamage, DamageEvent, EventInstigator ? EventInstigator->GetPawn() : NULL, DamageCauser);
		}

		MakeNoise(1.0f, EventInstigator ? EventInstigator->GetPawn() : this);
	}
	return ActualDamage;
}


bool AShooterCharacter::CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const
{
	if ( bIsDying										// already dying
		|| IsPendingKill()								// already destroyed
		|| GetLocalRole() < ROLE_Authority						// not authority
		|| GetWorld()->GetAuthGameMode<AShooterGameMode>() == NULL
		|| GetWorld()->GetAuthGameMode<AShooterGameMode>()->GetMatchState() == MatchState::LeavingMap)	// level transition occurring
	{
		return false;
	}

	return true;
}


bool AShooterCharacter::Die(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser)
{
	if (!CanDie(KillingDamage, DamageEvent, Killer, DamageCauser))
	{
		return false;
	}

	ApplyDamageMomentum(KillingDamage, DamageEvent, Killer? Killer->GetPawn() : NULL, DamageCauser);

	//make sure health is 0 or below (won't be in case of environmental death)
	Health = FMath::Min(Health, 0.f);
	Mana = 0.f;

	// if this is an environmental death then refer to the previous killer so that they receive credit (knocked into lava pits, etc)
	UDamageType const* const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	//try to determine what weapon killed me
	AShooterWeapon* Weapon = Cast<AShooterWeapon>(DamageCauser);
	if (Weapon == NULL)
	{
		AShooterProjectile* Proj = Cast<AShooterProjectile>(DamageCauser);
		if (Proj)
		{
			Weapon = Proj->OwnerWeapon;
		}
	}
	TSubclassOf<class AShooterWeapon> KillerWeaponClass;
	if (Weapon)
	{
		KillerWeaponClass = Weapon->GetClass();
	}
	TSubclassOf<class UShooterDamageType> KillerDmgType;
	if (DamageType)
	{
		KillerDmgType = DamageType->GetClass();
	}
	
	AController* const KilledPlayer = (Controller != NULL) ? Controller : Cast<AController>(GetOwner());
	GetWorld()->GetAuthGameMode<AShooterGameMode>()->Killed(Killer, KilledPlayer, this, KillerWeaponClass, KillerDmgType);
	
	NetUpdateFrequency = GetDefault<AShooterCharacter>()->NetUpdateFrequency;
	GetCharacterMovement()->ForceReplicationUpdate();

	OnDeath(KillingDamage, DamageEvent, Killer ? Killer->GetPawn() : NULL, Killer, DamageCauser);
	
	return true;
}

AController* AShooterCharacter::GetDamageInstigator(AController* InstigatedBy, const UDamageType& DamageType) const
{
	if ( (InstigatedBy != NULL) && (InstigatedBy != Controller) )
	{
		return InstigatedBy;
	}
	if ( DamageType.bCausedByWorld && (LastHitBy != NULL) && GetWorld()->GetTimeSeconds() - LastHitTime < 4.f )
	{
		return LastHitBy;
	}
	if (InstigatedBy != NULL)
	{
		return InstigatedBy;
	}
	//couldn't find a killer, consider it a suicide
	return GetController();
}

void AShooterCharacter::OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AController* Killer, class AActor* DamageCauser)
{
	if (bIsDying)
	{
		return;
	}
	
	ApplyDamageMomentum(KillingDamage, DamageEvent, PawnInstigator, DamageCauser);
	SetReplicateMovement(false);
	TearOff();
	bIsDying = true;

	if (GetLocalRole() == ROLE_Authority)
	{
		ReplicateHit(KillingDamage, DamageEvent, PawnInstigator, DamageCauser, true);
		
		//achievements must be checked after ReplicateHit(), which will update killed pawn's LastHitInfo
		AShooterGameMode* Game = GetWorld()->GetAuthGameMode<AShooterGameMode>();
		if (Game && Game->NotifyGameAchievements)
		{
			Game->CheckAndNotifyAchievements(Killer, GetController(), this, DamageCauser);
		}

		DropWeapon();

		// play the force feedback effect on the client player controller
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			UShooterDamageType *DamageType = Cast<UShooterDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->KilledForceFeedback)
			{
				PC->ClientPlayForceFeedback(DamageType->KilledForceFeedback, false, true, "Damage");
			}
		}
	}

	// cannot use IsLocallyControlled here, because even local client's controller may be NULL here
	if (GetNetMode() != NM_DedicatedServer && DeathSound && Mesh1P && Mesh1P->IsVisible())
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	// switch back to 3rd person view
	UpdatePawnMeshes();

	DetachFromControllerPendingDestroy();
	StopAllAnimMontages();
	
	// remove all weapons (give some time to run all functions, such as, explode all bullets (WeapRevolver))
	GetWeapon()->GetWeaponMesh3P()->SetHiddenInGame(true, true);
	GetWeapon()->GetWeaponMesh1P()->SetHiddenInGame(true, true);
	GetWeapon()->OwnerDied();
	GetWorldTimerManager().SetTimer(DestroyInventoryHandle, this, &AShooterCharacter::DestroyInventory, 1.0f, false);

	if (LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
	{
		LowHealthWarningPlayer->Stop();
	}

	if (RunLoopAC)
	{
		RunLoopAC->Stop();
	}

	// disable collisions on capsule
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

	if (GetMesh())
	{
		static FName CollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetCollisionProfileName(CollisionProfileName);
	}
	SetActorEnableCollision(true);

	if (ShouldGib(KillingDamage, DamageEvent))
	{
		Gib(DamageEvent);
	}
	//else
	{
		// Death anim
		float DeathAnimDuration = PlayAnimMontage(DeathAnim);

		// Ragdoll
		if (DeathAnimDuration > 0.f)
		{
			GetWorldTimerManager().SetTimer(SetRagdollPhysicsHandle, this, &AShooterCharacter::SetRagdollPhysics, FMath::Min(0.1f, DeathAnimDuration), false);
		}
		else
		{
			SetRagdollPhysics();
		}
	}
	PawnDiedEvent(PawnInstigator, DamageCauser);
}

void AShooterCharacter::PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
{
	if (GetLocalRole() == ROLE_Authority && !GetTearOff())
	{
		ReplicateHit(DamageTaken, DamageEvent, PawnInstigator, DamageCauser, false);

		// play the force feedback effect on the client player controller
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			UShooterDamageType *DamageType = Cast<UShooterDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->HitForceFeedback)
			{
				PC->ClientPlayForceFeedback(DamageType->HitForceFeedback, false, true, "Damage");
			}
		}
	}

	if (DamageTaken > 0.f)
	{
		ApplyDamageMomentum(DamageTaken, DamageEvent, PawnInstigator, DamageCauser);
	}
	
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	//AShooterHUD* MyHUD = MyPC ? Cast<AShooterHUD>(MyPC->GetHUD()) : NULL;
	//if (MyHUD)
	{
		FVector ImpulseDir;    
		FHitResult Hit; 
		DamageEvent.GetBestHitInfo(this, PawnInstigator, Hit, ImpulseDir);
//		MyHUD->NotifyHit(Hit.GetComponent(), DamageCauser, nullptr, false,  Hit.Location, Hit.Normal, ImpulseDir, Hit);
	}

	if (PawnInstigator && PawnInstigator != this && PawnInstigator->IsLocallyControlled() && Health > 0.f)
	{
		AShooterPlayerController* InstigatorPC = Cast<AShooterPlayerController>(PawnInstigator->Controller);
		//AShooterHUD* InstigatorHUD = InstigatorPC ? Cast<AShooterHUD>(InstigatorPC->GetHUD()) : NULL;
		//if (InstigatorHUD)
		{
//			InstigatorHUD->NotifyEnemyHit();
		}
	}

	if (ShouldGib(DamageTaken, DamageEvent))
	{
		Gib(DamageEvent);
	}
}

bool AShooterCharacter::ShouldGib(float DamageTaken, struct FDamageEvent const& DamageEvent) const
{
	//UDamageType* DmgType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass : GetDefault<UDamageType>();
	if (Health < -100.f || (Health < -10.f && DamageTaken > 20.f && DamageEvent.GetTypeID() == FRadialDamageEvent::ClassID) )
	{
		return true;
	}
	return false;	
}

void AShooterCharacter::Gib(struct FDamageEvent const& DamageEvent)
{
	return;
	TArray<FName> BonesToBreak;

	if (DamageEvent.GetTypeID() == FPointDamageEvent::ClassID)
	{
		FPointDamageEvent PointDmg = *((FPointDamageEvent const*)(&DamageEvent));
		BonesToBreak.Add(PointDmg.HitInfo.BoneName);
	}
	else if (DamageEvent.GetTypeID() == FRadialDamageEvent::ClassID)
	{
		FRadialDamageEvent RadDmg = *((FRadialDamageEvent const*)(&DamageEvent));
		for (FHitResult Hit : RadDmg.ComponentHits)
		{
			BonesToBreak.Add(Hit.BoneName);
		}
	}

	for (FName BoneName : BonesToBreak)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, BoneName.ToString());
		GetMesh()->HideBoneByName(BoneName, EPhysBodyOp::PBO_Term);
		//TODO: spawn the broken bits
	}
}

void AShooterCharacter::SetRagdollPhysics()
{
	bool bInRagdoll = false;

	if (IsPendingKill())
	{
		bInRagdoll = false;
	}
	else if (!GetMesh() || !GetMesh()->GetPhysicsAsset())
	{
		bInRagdoll = false;
	}
	else
	{
		// initialize physics/etc
		GetMesh()->SetAllBodiesSimulatePhysics(true);
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		GetMesh()->bBlendPhysics = true;

		//GetMesh()->SetNotifyRigidBodyCollision(true);
		
		//regenerate impulse from LastTakeHitInfo (it's not replicated in time on GetCharacterMovement())
		if (LastTakeHitInfo.PawnInstigator.Get() != NULL)
		{
			FHitResult HitInfo;
			FVector ImpulseDir;
			LastTakeHitInfo.GetDamageEvent().GetBestHitInfo(this, LastTakeHitInfo.PawnInstigator.Get(), HitInfo, ImpulseDir);
			FVector Impulse = ImpulseDir;

			const UDamageType* DmgTypeInst = Cast<UDamageType>(LastTakeHitInfo.DamageTypeClass->GetDefaultObject());
			AShooterWeapon* KilledByWeapon = Cast<AShooterWeapon>(LastTakeHitInfo.DamageCauser.Get());
			AShooterProjectile* KilledByProjectile = Cast<AShooterProjectile>(LastTakeHitInfo.DamageCauser.Get());
			//assume fire mode == 0, this has no big gameplay implications
			if (KilledByWeapon && KilledByWeapon->GetFiringMode(0) == EFireMode::FM_Instant)
			{
				int32 ShotsTaken = (int32) (LastTakeHitInfo.ActualDamage / KilledByWeapon->GetInstantHitDamage(0));
				if (ShotsTaken < 0)
					ShotsTaken = 1;
				//const UShooterDamageType* DmgTypeInst = Cast<UShooterDamageType>(KilledByWeapon->GetInstantConfig(0).DamageType->GetDefaultObject());
				const float TotalDamageImpulse = DmgTypeInst->DamageImpulse * ShotsTaken;
				Impulse = Impulse * TotalDamageImpulse;
			}
			else if (KilledByProjectile)
			{
				//const UShooterDamageType* DmgTypeInst = Cast<UShooterDamageType>(KilledByProjectile->DamageType->GetDefaultObject());
				Impulse = Impulse * DmgTypeInst->DamageImpulse;
			}
			else
			{
				
				Impulse = Impulse * DmgTypeInst->DamageImpulse;
			}

			GetMesh()->AddImpulseAtLocation(Impulse, HitInfo.Location);
		}

		bInRagdoll = true;
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	if (!bInRagdoll)
	{
		// hide and set short lifespan
		TurnOff();
		SetActorHiddenInGame(true);
		SetLifeSpan( 1.0f );
	}
	else
	{
		SetLifeSpan( 60.0f );
		FPointDamageEvent ev = LastTakeHitInfo.GetPointDamageEvent();
		RagdollEvent(LastTakeHitInfo.PawnInstigator.Get(), ev.HitInfo, ev.ShotDirection);
	}
}



void AShooterCharacter::ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser, bool bKilled)
{
	const float TimeoutTime = GetWorld()->GetTimeSeconds() + 0.5f;

	FDamageEvent const& LastDamageEvent = LastTakeHitInfo.GetDamageEvent();
	if ((PawnInstigator == LastTakeHitInfo.PawnInstigator.Get()) && (LastDamageEvent.DamageTypeClass == LastTakeHitInfo.DamageTypeClass) && (LastTakeHitTimeTimeout == TimeoutTime))
	{
		// same frame damage
		if (bKilled && LastTakeHitInfo.bKilled)
		{
			// Redundant death take hit, just ignore it
			return;
		}

		// otherwise, accumulate damage done this frame
		Damage += LastTakeHitInfo.ActualDamage;
	}

	LastTakeHitInfo.ActualDamage = Damage;
	LastTakeHitInfo.PawnInstigator = Cast<AShooterCharacter>(PawnInstigator);
	LastTakeHitInfo.DamageCauser = DamageCauser;
	LastTakeHitInfo.SetDamageEvent(DamageEvent);		
	LastTakeHitInfo.bKilled = bKilled;
	LastTakeHitInfo.EnsureReplication();

	LastTakeHitTimeTimeout = TimeoutTime;
}

void AShooterCharacter::OnRep_LastTakeHitInfo()
{
	if (LastTakeHitInfo.bKilled)
	{
		//clients don't need info about the Killer's controller, it's only used by server for checking achievements
		OnDeath(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), NULL, LastTakeHitInfo.DamageCauser.Get());
	}
	else
	{
		PlayHit(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
}

//Pawn::PlayDying sets this lifespan, but when that function is called on client, dead pawn's role is still SimulatedProxy despite bTearOff being true. 
void AShooterCharacter::TornOff()
{
	SetLifeSpan(60.f);
}


//////////////////////////////////////////////////////////////////////////
// Inventory

void AShooterCharacter::DropWeapon()
{
	if (bNeverDropInventory)
	{
		return;
	}
	const FTransform SpawnTM(GetActorRotation(), GetActorLocation());
	AShooterGameMode* GameMode = GetWorld()->GetAuthGameMode<AShooterGameMode>();
	const float PickupDuration = GameMode ? GameMode->DroppedPickupDuration : 10.f;
	const bool ShouldDropWeapon = GameMode ? GameMode->DropWeaponOnDeath : true;
	if (CurrentWeapon && ShouldDropWeapon && CurrentWeapon && CurrentWeapon->GetCurrentTotalAmmo() > 0)
	{
		AShooterPickup_Weapon* NewPickup = Cast<AShooterPickup_Weapon>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, CurrentWeapon->ItemPickup, SpawnTM));
		if (NewPickup)
		{
			NewPickup->overrideAmmoAmount = CurrentWeapon->GetCurrentTotalAmmo();
			NewPickup->WasDropped = true;
			NewPickup->SetLifeSpan(PickupDuration);
			NewPickup->RespawnTime = 1000.f;
			UGameplayStatics::FinishSpawningActor(NewPickup, SpawnTM);
			
			const FVector_NetQuantize10 TossVelocity = FVector_NetQuantize10(FMath::FRand() * 200.f, FMath::FRand() * 200.f, 500.f);
			NewPickup->SetVelocity(TossVelocity);
		}
	}

	AShooterItem_Powerup* Powerup = GetFirstPowerup();
	if (Powerup && Powerup->IsActive())
	{
		const bool ShouldDropPowerup = GameMode ? GameMode->DropPowerupOnDeath : true;
		if (ShouldDropPowerup)
		{
			AShooterPickup_Powerup* NewPickup = Cast<AShooterPickup_Powerup>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, Powerup->ItemPickup, SpawnTM));
			if (NewPickup)
			{
				NewPickup->overrideDuration = Powerup->GetRemainingDuration();
				NewPickup->WasDropped = true;
				NewPickup->SetLifeSpan(PickupDuration);
				NewPickup->RespawnTime = 1000.f;
				UGameplayStatics::FinishSpawningActor(NewPickup, SpawnTM);

				const FVector_NetQuantize10 TossVelocity = FVector_NetQuantize10(FMath::FRand() * 200.f, FMath::FRand() * 200.f, 500.f);
				NewPickup->SetVelocity(TossVelocity);
			}
		}
		Powerup->Deactivate();
	}
}
void AShooterCharacter::SpawnDefaultInventory()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		return;
	}

	AShooterWeapon* NewWeapon = NULL;
	for (int32 i = 0; i < DefaultInventoryClasses.Num(); i++)
	{
		if (DefaultInventoryClasses[i])
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			NewWeapon = GetWorld()->SpawnActor<AShooterWeapon>(DefaultInventoryClasses[i], SpawnInfo);
			AddWeapon(NewWeapon);
		}
	}

	if (NewWeapon)
	{
		EquipWeapon(NewWeapon);
	}
}

void AShooterCharacter::DestroyInventory()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		return;
	}

	// remove all items from inventory and destroy them
	for (int32 i = Inventory.Num() - 1; i >= 0; i--)
	{
		AShooterItem* Item = Inventory[i];
		if (Item)
		{
			RemoveItem(Item);
			Item->Destroy();
		}
	}
}

void AShooterCharacter::AddWeapon(AShooterWeapon* Weapon, int32 AmmoAmount)
{
	if (Weapon && GetLocalRole() == ROLE_Authority)
	{
		Weapon->SetOwningPawn(this);
		Inventory.AddUnique(Weapon);
		const int32 NewAmmo = AmmoAmount <= -1 ? Weapon->GetInitialAmmo() : AmmoAmount;
		GiveAmmo(Weapon->GetClass(), NewAmmo);
		//initial clip ammo
		if (!Weapon->HasInfiniteClip())
		{
			const float ClipAmmo = FMath::Min(Weapon->GetAmmoPerClip(), NewAmmo);
			Weapon->CurrentAmmoInClip = ClipAmmo;
			UseAmmo(Weapon->GetClass(), ClipAmmo);
		}
		ClientWeaponAdded(Weapon->GetClass());	
	}
}

void AShooterCharacter::AddItem(AShooterItem* Item)
{
	if (Item && GetLocalRole() == ROLE_Authority)
	{
		Item->SetOwningPawn(this);
		Inventory.AddUnique(Item);

		AShooterWeapon* Weapon = Cast<AShooterWeapon>(Item);
		if (Weapon)
		{
			GiveAmmo(Weapon->GetClass(), Weapon->GetInitialAmmo());
		}
	}
}

bool AShooterCharacter::ClientWeaponAdded_Validate(TSubclassOf<class AShooterWeapon> WeaponClass)
{
	return true;
}

void AShooterCharacter::ClientWeaponAdded_Implementation(TSubclassOf<class AShooterWeapon> WeaponClass)
{
	OnWeaponAdded(WeaponClass);
}

void AShooterCharacter::RemoveItem(AShooterItem* Item)
{
	if (Item && GetLocalRole() == ROLE_Authority)
	{
		Item->SetOwningPawn(NULL);
		AShooterWeapon* Weapon = Cast<AShooterWeapon>(Item);
		if (Weapon && Weapon == CurrentWeapon)
		{
			Weapon->OnUnEquip();
		}
		AShooterItem_Powerup* Powerup = Cast<AShooterItem_Powerup>(Item);
		if (Powerup && Powerup->IsActive())
		{
			Powerup->Deactivate();
		}
		Inventory.RemoveSingle(Item);
	}
}

void AShooterCharacter::ClearInventory()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		while (Inventory.Num() > 0)
		{
			RemoveItem(Inventory[0]);
		}
	}
}
AShooterWeapon* AShooterCharacter::FindWeapon(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i] && Inventory[i]->IsA(WeaponClass))
		{
			return Cast<AShooterWeapon>(Inventory[i]);
		}
	}
	return NULL;
}

AShooterItem_Powerup* AShooterCharacter::GetFirstPowerup()
{	
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i] && Inventory[i]->IsA(AShooterItem_Powerup::StaticClass()))
		{
			return Cast<AShooterItem_Powerup>(Inventory[i]);
		}
	}
	return NULL;
}

AShooterWeapon* AShooterCharacter::GetBestWeapon(float AIMinWeaponRangeSquared)
{
	for (uint8 i = 0; i < WeaponPriority.Num(); i++)
	{
		AShooterWeapon* BestWeapon = FindWeapon(WeaponPriority[i]);
		for (int32 j = 0; j < NUM_FIRING_MODES; j++)
		{
			if (BestWeapon && BestWeapon->HasEnoughAmmo() && BestWeapon->AIWeaponRange[j] * BestWeapon->AIWeaponRange[j] >= AIMinWeaponRangeSquared)
			{
				return BestWeapon;
			}
		}
	}
	return NULL;
}

int32 AShooterCharacter::GetCurrentAmmo(TSubclassOf<AShooterWeapon> WeaponClass, bool GetInventoryAmmo) const
{
	AShooterWeapon* Weap = FindWeapon(WeaponClass);
	if (!GetInventoryAmmo && Weap && !Weap->HasInfiniteClip())
	{
		return Weap->GetCurrentAmmoInClip();
	}
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i])
		{
			AShooterItem_Ammo* Ammo = Cast<AShooterItem_Ammo>(Inventory[i]);
			if (Ammo && Ammo->WeaponClass == WeaponClass)
			{
				return Ammo->GetAmmoAmount();
			}
		}
	}

	return 0;
}

void AShooterCharacter::UseAmmo(TSubclassOf<AShooterWeapon> WeaponClass, int32 AmmoToConsume)
{
	if (GetLocalRole() == ROLE_Authority && !bHasInfiniteAmmo)
	{
		for (int32 i = 0; i < Inventory.Num(); i++)
		{
			if (Inventory[i])
			{
				AShooterItem_Ammo* Ammo = Cast<AShooterItem_Ammo>(Inventory[i]);
				if (Ammo && Ammo->WeaponClass == WeaponClass)
				{
					Ammo->UseAmmo(AmmoToConsume);
					return;
				}
			}
		}
	}
}

int32 AShooterCharacter::GiveAmmo(TSubclassOf<AShooterWeapon> WeaponClass, int32 Amount)
{
	if (Amount > 0)
	{
		for (int32 i = 0; i < Inventory.Num(); i++)
		{
			if (Inventory[i])
			{
				AShooterItem_Ammo* Ammo = Cast<AShooterItem_Ammo>(Inventory[i]);
				if (Ammo && Ammo->WeaponClass == WeaponClass)
				{
					//already has ammo item. Update amount.
					return Ammo->AddAmmo(Amount);
				}
			}
		}
		//doesn't have ammo item yet, create it
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AShooterItem_Ammo* NewAmmoItem = GetWorld()->SpawnActor<AShooterItem_Ammo>(AShooterItem_Ammo::StaticClass(), SpawnInfo);
		NewAmmoItem->InitializeAmmo(WeaponClass, Amount);
		AddItem(NewAmmoItem);
		return Amount;
	}
	return 0;
}

bool AShooterCharacter::CanPickupAmmo(TSubclassOf<AShooterWeapon> WeaponClass)
{
	if (!CanPickupPickups)
	{
		return false;
	}
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i])
		{
			AShooterItem_Ammo* Ammo = Cast<AShooterItem_Ammo>(Inventory[i]);
			if (Ammo && Ammo->WeaponClass == WeaponClass)
			{
				//already has ammo item. Check ammo amount
				return !Ammo->IsMaxAmmo();
			}
		}
	}
	//doesn't have that ammo yet, so it is zero and can be picked up
	return true;
}
void AShooterCharacter::EquipWeapon(AShooterWeapon* Weapon)
{
	if (Weapon)
	{
		if (GetLocalRole() == ROLE_Authority)
		{
			SetCurrentWeapon(Weapon);
		}
		else
		{
			ServerEquipWeapon(Weapon);
		}
	}
}

bool AShooterCharacter::ServerEquipWeapon_Validate(AShooterWeapon* Weapon)
{
	return true;
}

void AShooterCharacter::ServerEquipWeapon_Implementation(AShooterWeapon* Weapon)
{
	EquipWeapon(Weapon);
}

void AShooterCharacter::OnRep_CurrentWeapon(AShooterWeapon* LastWeapon)
{
	SetCurrentWeapon(CurrentWeapon, LastWeapon);
}

void AShooterCharacter::SetCurrentWeapon(class AShooterWeapon* NewWeapon, class AShooterWeapon* LastWeapon)
{
	AShooterWeapon* LocalLastWeapon = NULL;
	
	if (LastWeapon != NULL)
	{
		LocalLastWeapon = LastWeapon;
	}
	else if (NewWeapon != CurrentWeapon)
	{
		LocalLastWeapon = CurrentWeapon;
	}

	// unequip previous
	if (LocalLastWeapon)
	{
		LocalLastWeapon->OnUnEquip();
	}

	CurrentWeapon = NewWeapon;

	// equip new one
	if (NewWeapon)
	{
		NewWeapon->SetOwningPawn(this);	// Make sure weapon's MyPawn is pointing back to us. During replication, we can't guarantee APawn::CurrentWeapon will rep after AWeapon::MyPawn!
		NewWeapon->OnEquip();
		OnWeaponEquipped.Broadcast(NewWeapon);
	}
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage

void AShooterCharacter::StartWeaponFire(uint8 FireMode)
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire(FireMode);
		if (CurrentWeapon->HasEnoughAmmo())
		{
			OnCharacterFired.Broadcast(CurrentWeapon, FireMode);
		}
	}
}

void AShooterCharacter::StopWeaponFire(uint8 FireMode)
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire(FireMode);
	}
}

void AShooterCharacter::StopAllWeaponFire()
{
	if (CurrentWeapon)
	{
		for (uint8 i = 0; i < NUM_FIRING_MODES; i++)
			CurrentWeapon->StopFire(i);
	}
}

bool AShooterCharacter::CanFire() const
{
	return IsAlive();
}

void AShooterCharacter::SetTargeting(bool bNewTargeting)
{
	bIsTargeting = bNewTargeting;

	if (TargetingSound)
	{
		UGameplayStatics::SpawnSoundAttached(TargetingSound, GetRootComponent());
	}

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerSetTargeting(bNewTargeting);
	}
}

bool AShooterCharacter::ServerSetTargeting_Validate(bool bNewTargeting)
{
	return true;
}

void AShooterCharacter::ServerSetTargeting_Implementation(bool bNewTargeting)
{
	SetTargeting(bNewTargeting);
}

//////////////////////////////////////////////////////////////////////////
// Movement

void AShooterCharacter::SetRunning(bool bNewRunning, bool bToggle)
{
	bWantsToRun = bNewRunning;
	bWantsToRunToggled = bNewRunning && bToggle;

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerSetRunning(bNewRunning, bToggle);
	}

	UpdateRunSounds(bNewRunning);
}

bool AShooterCharacter::ServerSetRunning_Validate(bool bNewRunning, bool bToggle)
{
	return true;
}

void AShooterCharacter::ServerSetRunning_Implementation(bool bNewRunning, bool bToggle)
{
	SetRunning(bNewRunning, bToggle);
}

void AShooterCharacter::UpdateRunSounds(bool bNewRunning)
{
	if (bNewRunning)
	{
		if (!RunLoopAC && RunLoopSound)
		{
			RunLoopAC = UGameplayStatics::SpawnSoundAttached(RunLoopSound, GetRootComponent());
			if (RunLoopAC)
			{
				RunLoopAC->bAutoDestroy = false;
			}
			
		}
		else if (RunLoopAC)
		{
			RunLoopAC->Play();
		}
	}
	else
	{
		if (RunLoopAC)
		{
			RunLoopAC->Stop();
		}

		if (RunStopSound)
		{
			UGameplayStatics::SpawnSoundAttached(RunStopSound, GetRootComponent());
		}
	}
}

void AShooterCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	UShooterCharacterMovement* MovementComp = Cast<UShooterCharacterMovement>(GetMovementComponent());
	if (GetLocalRole() == ROLE_Authority && MovementComp && MovementComp->MovementMode == MOVE_Falling && GetVelocity().Z < -MinLandedDamageVelocity)
	{
		const float FallingDamage = FMath::Lerp(0.0f, 100.0f, (FMath::Abs(GetVelocity().Z) - MinLandedDamageVelocity) / (MaxLandedDamageVelocity - MinLandedDamageVelocity));
		UGameplayStatics::ApplyDamage(this, FallingDamage, Controller, NULL, UShooterDamageType::StaticClass());
	}
}

//////////////////////////////////////////////////////////////////////////
// Animations

float AShooterCharacter::PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate, FName StartSectionName) 
{
	float AnimLength = 0.f;
	if (AnimMontage)
	{
		for (USkeletalMeshComponent* TheMesh : AllMeshes)
		{
			if (TheMesh && TheMesh->AnimScriptInstance && TheMesh->AnimScriptInstance->CurrentSkeleton && TheMesh->AnimScriptInstance->CurrentSkeleton->IsCompatible(AnimMontage->GetSkeleton()))
			{
				AnimLength = TheMesh->AnimScriptInstance->Montage_Play(AnimMontage, InPlayRate);
			}
		}
	}
	return AnimLength;
}

void AShooterCharacter::StopAnimMontage(class UAnimMontage* AnimMontage)
{
	if (AnimMontage)
	{
		for (USkeletalMeshComponent* TheMesh : AllMeshes)
		{
			if (TheMesh && TheMesh->AnimScriptInstance && TheMesh->AnimScriptInstance->Montage_IsPlaying(AnimMontage))
			{
				TheMesh->AnimScriptInstance->Montage_Stop(AnimMontage->BlendOut.GetBlendTime());
			}
		}
	}
}

void AShooterCharacter::StopAllAnimMontages()
{
	for (USkeletalMeshComponent* TheMesh : AllMeshes)
	{
		if (TheMesh && TheMesh->AnimScriptInstance)
		{
			TheMesh->AnimScriptInstance->Montage_Stop(0.0f);
		}
	}
}

void AShooterCharacter::OnRep_AnimRep()
{
	switch (AnimReplication.AnimToPlay)
	{
	case ECharacterAnimations::Kick:
		PlayAnimMontage(KickAnim);
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AShooterCharacter::SetupPlayerInputComponent(class UInputComponent* TheInputComponent)
{
	check(TheInputComponent);
	TheInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	TheInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	TheInputComponent->BindAxis("MoveUp", this, &AShooterCharacter::MoveUp);
	TheInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	TheInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	TheInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	TheInputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpAtRate);

	TheInputComponent->BindAction("StartFire", IE_Pressed, this, &AShooterCharacter::OnStartFirePrimary);
	TheInputComponent->BindAction("StopFire", IE_Released, this, &AShooterCharacter::OnStopFirePrimary);
	TheInputComponent->BindAction("StartFire2", IE_Pressed, this, &AShooterCharacter::OnStartFireSecondary);
	TheInputComponent->BindAction("StopFire2", IE_Released, this, &AShooterCharacter::OnStopFireSecondary);
	TheInputComponent->BindAction("Kick", IE_Released, this, &AShooterCharacter::OnKick);

	TheInputComponent->BindAction("NextWeapon", IE_Pressed, this, &AShooterCharacter::OnNextWeapon);
	TheInputComponent->BindAction("PrevWeapon", IE_Pressed, this, &AShooterCharacter::OnPrevWeapon);
	TheInputComponent->BindAction("Reload", IE_Pressed, this, &AShooterCharacter::OnReload);
	TheInputComponent->BindAction("WeaponCat1", IE_Pressed, this, &AShooterCharacter::SwitchToWeaponCategory1);
	TheInputComponent->BindAction("WeaponCat2", IE_Pressed, this, &AShooterCharacter::SwitchToWeaponCategory2);
	TheInputComponent->BindAction("WeaponCat3", IE_Pressed, this, &AShooterCharacter::SwitchToWeaponCategory3);
	TheInputComponent->BindAction("WeaponCat4", IE_Pressed, this, &AShooterCharacter::SwitchToWeaponCategory4);
	TheInputComponent->BindAction("WeaponCat5", IE_Pressed, this, &AShooterCharacter::SwitchToWeaponCategory5);
	TheInputComponent->BindAction("WeaponCat6", IE_Pressed, this, &AShooterCharacter::SwitchToWeaponCategory6);

	TheInputComponent->BindAction("StartJump", IE_Pressed, this, &AShooterCharacter::OnStartJump);
	TheInputComponent->BindAction("StopJump", IE_Released, this, &AShooterCharacter::OnStopJump);
	TheInputComponent->BindAction("StartCrouch", IE_Pressed, this, &AShooterCharacter::OnStartCrouchInput);
	TheInputComponent->BindAction("StopCrouch", IE_Released, this, &AShooterCharacter::OnStopCrouchInput);

	TheInputComponent->BindAction("Use", IE_Pressed, this, &AShooterCharacter::OnUse);

	TheInputComponent->BindAction("DodgeForward", IE_DoubleClick, this, &AShooterCharacter::OnDodgeForward);
	TheInputComponent->BindAction("DodgeBackward", IE_DoubleClick, this, &AShooterCharacter::OnDodgeBackward);
	TheInputComponent->BindAction("DodgeLeft", IE_DoubleClick, this, &AShooterCharacter::OnDodgeLeft);
	TheInputComponent->BindAction("DodgeRight", IE_DoubleClick, this, &AShooterCharacter::OnDodgeRight);

}


void AShooterCharacter::MoveForward(float Val)
{
	if (Controller && Val != 0.f)
	{
		// Limit pitch when walking or falling
		const bool bLimitRotation = (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling());
		const FRotator Rotation = bLimitRotation ? GetActorRotation() : Controller->GetControlRotation();
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis( EAxis::X );
		AddMovementInput(Direction, Val);
	}
}

void AShooterCharacter::MoveRight(float Val)
{
	if (Val != 0.f)
	{
		const FRotator Rotation = GetActorRotation();
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis( EAxis::Y );
		AddMovementInput(Direction, Val);
	}
}

void AShooterCharacter::MoveUp(float Val)
{
	if (Val != 0.f)
	{
		// Not when walking or falling.
		if (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling())
		{
			return;
		}

		AddMovementInput(FVector::UpVector, Val);
	}
}

void AShooterCharacter::TurnAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Val * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}
/*
void AShooterCharacter::AddControllerYawInput(float Val)
{
	Super::AddControllerYawInput(Val);
	IncreaseAimingDispersion(Val);
}

void AShooterCharacter::AddControllerPitchInput(float Val)
{
	Super::AddControllerPitchInput(Val);
	IncreaseAimingDispersion(Val);
}
*/
void AShooterCharacter::LookUpAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Val * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::OnStartFirePrimary()
{
	StartWeaponFire(0);
}

void AShooterCharacter::OnStartFireSecondary()
{
	StartWeaponFire(1);
}

void AShooterCharacter::OnStopFirePrimary()
{
	StopWeaponFire(0);
}

void AShooterCharacter::OnStopFireSecondary()
{
	StopWeaponFire(1);
}

void AShooterCharacter::OnStartTargeting()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsRunning())
		{
			SetRunning(false, false);
		}
		SetTargeting(true);
	}
}

void AShooterCharacter::OnStopTargeting()
{
	SetTargeting(false);
}

void AShooterCharacter::OnReload()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (CurrentWeapon)
		{
			CurrentWeapon->StartReload();
		}
	}
}

void AShooterCharacter::OnNextWeapon()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (Inventory.Num() >= 2)
		{
			int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon);
			AShooterWeapon* NextWeapon = NULL;
			while (NextWeapon == NULL)
			{
				++CurrentWeaponIdx;
				NextWeapon = Cast<AShooterWeapon>(Inventory[(CurrentWeaponIdx) % Inventory.Num()]);
			}
			EquipWeapon(NextWeapon);
		}
	}
}

void AShooterCharacter::OnPrevWeapon()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (Inventory.Num() >= 2)
		{
			int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon) + Inventory.Num();
			AShooterWeapon* NextWeapon = NULL;
			while (NextWeapon == NULL)
			{
				--CurrentWeaponIdx;
				NextWeapon = Cast<AShooterWeapon>(Inventory[(CurrentWeaponIdx) % Inventory.Num()]);
			}
			EquipWeapon(NextWeapon);
		}
	}
}

AShooterWeapon* AShooterCharacter::SwitchToWeaponCategory(uint8 Category)
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (Inventory.Num() >= 2)
		{
			int32 CurrentWeaponIdx = CurrentWeapon->WeaponCategory == Category ? Inventory.IndexOfByKey(CurrentWeapon) : 0;
			int32 InvSize = Inventory.Num();
			int32 Attempts=0;
			AShooterWeapon* NextWeapon = NULL;
			while (NextWeapon == NULL && Attempts <= InvSize)
			{
				++Attempts;
				++CurrentWeaponIdx;
				NextWeapon = Cast<AShooterWeapon>(Inventory[(CurrentWeaponIdx) % Inventory.Num()]);
				if (NextWeapon && NextWeapon->WeaponCategory != Category)
				{
					NextWeapon = NULL;
				}
			}
			if (NextWeapon && NextWeapon != CurrentWeapon)
			{
				EquipWeapon(NextWeapon);
				return NextWeapon;
			}
		}
	}
	return NULL;
}

void AShooterCharacter::SwitchToWeaponCategory1()
{
	SwitchToWeaponCategory(1);
}

void AShooterCharacter::SwitchToWeaponCategory2()
{
	SwitchToWeaponCategory(2);
}

void AShooterCharacter::SwitchToWeaponCategory3()
{
	SwitchToWeaponCategory(3);
}

void AShooterCharacter::SwitchToWeaponCategory4()
{
	SwitchToWeaponCategory(4);
}

void AShooterCharacter::SwitchToWeaponCategory5()
{
	SwitchToWeaponCategory(5);
}

void AShooterCharacter::SwitchToWeaponCategory6()
{
	SwitchToWeaponCategory(6);
}

void AShooterCharacter::OnStartRunning()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsTargeting())
		{
			SetTargeting(false);
		}
		StopAllWeaponFire();
		SetRunning(true, false);
	}
}

void AShooterCharacter::OnStartRunningToggle()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsTargeting())
		{
			SetTargeting(false);
		}
		StopAllWeaponFire();
		SetRunning(true, true);
	}
}

void AShooterCharacter::OnStopRunning()
{
	SetRunning(false, false);
}

void AShooterCharacter::OnDodgeForward()
{
	ServerRequestDodge(EDodgeDirection::Forward);
}

void AShooterCharacter::OnDodgeBackward()
{
	ServerRequestDodge(EDodgeDirection::Backward);
}

void AShooterCharacter::OnDodgeLeft()
{
	ServerRequestDodge(EDodgeDirection::Left);
}

void AShooterCharacter::OnDodgeRight()
{
	ServerRequestDodge(EDodgeDirection::Right);
}

bool AShooterCharacter::ServerRequestDodge_Validate(uint8 DodgeDirection)
{
	return true;
}
void AShooterCharacter::ServerRequestDodge_Implementation(uint8 DodgeDirection)
{
	if (GetCharacterMovement()->IsCrouching())
	{
		return;
	}
	FVector DodgeVector = GetActorRotation().Vector();
	switch (DodgeDirection)
	{
	case EDodgeDirection::Backward:
		DodgeVector = -DodgeVector;
		break;
	case EDodgeDirection::Left:
		DodgeVector = GetActorRotation().RotateVector(FVector(0.f, -1.f, 0.f));
		break;
	case EDodgeDirection::Right:
		DodgeVector = GetActorRotation().RotateVector(FVector(0.f, 1.f, 0.f));
		break;
	default: break;
	}

	//normal dodge
	if (GetCharacterMovement()->IsMovingOnGround() && GWorld && LastDodgeTime <= GWorld->GetTimeSeconds() - MinDodgeInterval )
	{
		LastDodgeTime = GWorld->GetTimeSeconds();
		DodgeVector.Z = DodgeZ;
		AddMomentum(DodgeVector * DodgeMomentum, true);
		PlaySoundReplicated(JumpSound);
	}
	//wall dodge
	else if ( !GetCharacterMovement()->IsMovingOnGround() && GWorld && LastWallDodgeTime <= GWorld->GetTimeSeconds() - MinWallDodgeInterval)
	{
		LastWallDodgeTime = GWorld->GetTimeSeconds();
		FVector TraceFrom = GetActorLocation();
		FVector TraceTo = GetActorLocation() - DodgeVector * 100.0f;

		FCollisionQueryParams TraceParams(TEXT("DodgeTrace"), false, this);
		FHitResult Hit(ForceInit);
		if (GetWorld()->LineTraceSingleByChannel(Hit, TraceFrom, TraceTo, ECC_WorldStatic, TraceParams))
		{
			DodgeVector.Z = WallDodgeZ;
			AddMomentum(DodgeVector * WallDodgeMomentum, true);
			PlaySoundReplicated(JumpSound);
		}
	}
}

bool AShooterCharacter::PlaySoundReplicated_Validate(USoundCue* Sound)
{
	return true;
}

void AShooterCharacter::PlaySoundReplicated_Implementation(USoundCue* Sound)
{
	UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
}

void AShooterCharacter::OnUse()
{
}

FHitResult AShooterCharacter::ForwardTrace(float TraceDist, ECollisionChannel TraceChannel) const
{
	static FName Tag = FName(TEXT("ForwardTrace"));

	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(Tag, true, this);
	//TraceParams.bTraceAsyncScene = true;

	FHitResult Hit(ForceInit);
	AShooterPlayerController* PC = Cast<AShooterPlayerController>(Controller);
	FVector TraceFrom = FVector::ZeroVector;
	FVector AimDir = FVector::ZeroVector;
	if (PC)
	{
		FRotator CamRot;
		PC->GetPlayerViewPoint(TraceFrom, CamRot);
		AimDir = CamRot.Vector();
	}
	else
	{
		TraceFrom = GetActorLocation();
		AimDir = GetBaseAimRotation().Vector();
	}
	const FVector TraceTo = TraceFrom + AimDir * TraceDist;
	GetWorld()->LineTraceSingleByChannel(Hit, TraceFrom, TraceTo, TraceChannel, TraceParams);

	return Hit;
}

void AShooterCharacter::OnKick()
{
	ServerKick();
}

bool AShooterCharacter::ServerKick_Validate()
{
	return true;
}

void AShooterCharacter::ServerKick_Implementation()
{
	if (GetCharacterMovement()->IsCrouching())
	{
		return;
	}
	
	const float KickInterval = 1.f;
	if (GWorld && LastKickTime <= GWorld->GetTimeSeconds() - KickInterval )
	{
		AnimReplication.AnimToPlay = ECharacterAnimations::Kick;
		AnimReplication.bForceUpdate = !AnimReplication.bForceUpdate;

		//play anim locally (also run on DedicatedServer because damage depends on anim notify)
		OnRep_AnimRep();

		LastKickTime = GWorld->GetTimeSeconds();
	}
}

bool AShooterCharacter::IsRunning() const
{	
	if (!GetCharacterMovement())
	{
		return false;
	}
	
	return (bWantsToRun || bWantsToRunToggled) && !GetVelocity().IsZero() && (GetVelocity().GetSafeNormal2D() | GetActorRotation().Vector()) > -0.1;
}

void AShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bWantsToRunToggled && !IsRunning())
	{
		SetRunning(false, false);
	}
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->HasHealthRegen())
	{
		if (this->Health < this->GetMaxHealth())
		{
			this->Health +=  5 * DeltaSeconds;
			if (Health > this->GetMaxHealth())
			{
				Health = this->GetMaxHealth();
			}
		}
	}
	
	if (LowHealthSound && GEngine->UseSound())
	{
		if ((this->Health > 0 && this->Health < this->GetMaxHealth() * LowHealthPercentage) && (!LowHealthWarningPlayer || !LowHealthWarningPlayer->IsPlaying()))
		{
			LowHealthWarningPlayer = UGameplayStatics::SpawnSoundAttached(LowHealthSound, GetRootComponent(),
				NAME_None, FVector(ForceInit), EAttachLocation::KeepRelativeOffset, true);
			LowHealthWarningPlayer->SetVolumeMultiplier(0.0f);
		} 
		else if ((this->Health > this->GetMaxHealth() * LowHealthPercentage || this->Health < 0) && LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
		{
			LowHealthWarningPlayer->Stop();
		}
		if (LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
		{
			const float MinVolume = 0.3f;
			const float VolumeMultiplier = (1.0f - (this->Health / (this->GetMaxHealth() * LowHealthPercentage)));
			LowHealthWarningPlayer->SetVolumeMultiplier(MinVolume + (1.0f - MinVolume) * VolumeMultiplier);
		}
	}

	if (bUpdateAimingDispersion)
	{
		CurrentAimingDispersion = FMath::Max(CurrentAimingDispersion - AimingDispersionDecrement * DeltaSeconds, MinAimingDispersion);
	}

#if !UE_BUILD_SHIPPING
	if (IsFirstPerson())
	{
		USkeletalMeshComponent* DefMesh1P = Cast<USkeletalMeshComponent>(GetClass()->GetDefaultSubobjectByName(TEXT("PawnMesh1P")));
		Mesh1P->SetRelativeLocation(DefMesh1P->GetRelativeLocation());
		Mesh1P->SetRelativeRotation(DefMesh1P->GetRelativeRotation());
	}
#endif
}

void AShooterCharacter::OnStartJump()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		bPressedJump = true;
	}
}

void AShooterCharacter::OnStopJump()
{
	bPressedJump = false;
}

void AShooterCharacter::OnStartCrouchInput()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		Crouch();
	}
}

void AShooterCharacter::OnStopCrouchInput()
{
	UnCrouch();
}

bool AShooterCharacter::IsFiring() const
{
	if (CurrentWeapon)
	{
		return CurrentWeapon->WantsToFire();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
// Replication

AShooterWeapon* AShooterCharacter::GetWeapon() const
{
	return CurrentWeapon;
}

TArray<AShooterWeapon*> AShooterCharacter::GetAllWeapons() const
{
	TArray<AShooterWeapon*> Weapons;
	for (AShooterItem* Item : Inventory)
	{
		AShooterWeapon* Weapon = Cast<AShooterWeapon>(Item);
		if (Weapon)
		{
			Weapons.Add(Weapon);
		}
	}
	return Weapons;
}
int32 AShooterCharacter::GetInventoryCount() const
{
	return Inventory.Num();
}

AShooterWeapon* AShooterCharacter::GetInventoryWeapon(int32 index) const
{
	return Cast<AShooterWeapon>(Inventory[index]);
}

FName AShooterCharacter::GetWeaponAttachPoint() const
{
	return WeaponAttachPoint;
}

bool AShooterCharacter::IsTargeting() const
{
	return bIsTargeting;
}

bool AShooterCharacter::IsFirstPerson() const
{
	AShooterPlayerController* SPC = Cast<AShooterPlayerController>(Controller);
	if (SPC == NULL || !IsAlive())
	{
		return false;
	}
	//@TODO: This doesn't works in splitscreen play (always returns true for both players)
	return IsLocallyControlled();
}

float AShooterCharacter::GetMaxHealth() const
{
	return GetClass()->GetDefaultObject<AShooterCharacter>()->Health;
}

bool AShooterCharacter::IsAlive() const
{
	return Health > 0.f;
}

float AShooterCharacter::GetLowHealthPercentage() const
{
	return LowHealthPercentage;
}

void AShooterCharacter::SetAllMeshesVectorParameter(FName ParameterName, FLinearColor NewColor)
{
	for (int32 i = 0; i < MeshMIDs.Num(); ++i)
	{
		if (MeshMIDs[i])
			MeshMIDs[i]->SetVectorParameterValue(ParameterName, NewColor);
	}
}

USkeletalMeshComponent* AShooterCharacter::GetPawnMesh1P() const
{
	return Mesh1P;
}

USkeletalMeshComponent* AShooterCharacter::GetPawnMesh3P() const
{
	return GetMesh();
}

void AShooterCharacter::AddMomentum( FVector Momentum, bool bMassIndependent )
{
	GetCharacterMovement()->AddImpulse(Momentum, bMassIndependent);
}

float AShooterCharacter::GetAimingDispersion() const
{
	return CurrentAimingDispersion;
}

void AShooterCharacter::IncreaseAimingDispersion(float Factor)
{
	if (bUpdateAimingDispersion)
	{
		const float DeltaTime = GetWorld()->GetDeltaSeconds();
		CurrentAimingDispersion = FMath::Min(CurrentAimingDispersion + AimingDispersionIncrement * DeltaTime * FMath::Abs(Factor), MaxAimingDispersion);
	}
}

UShooterPersistentUser* AShooterCharacter::GetPersistentUser() const
{
	AShooterPlayerController* PC = Cast<AShooterPlayerController>(Controller);
	if (PC)
	{
		UShooterLocalPlayer* const ShooterLP = Cast<UShooterLocalPlayer>(PC->Player);
		if (ShooterLP)
		{
			return ShooterLP->GetPersistentUser();
		}
	}
	return NULL;
}

void AShooterCharacter::NotifyOutOfAmmo()
{
	if (SwitchWeaponIfNoAmmo())
	{
		EquipWeapon(GetBestWeapon());
	}
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
//	AShooterHUD* MyHUD = MyPC ? Cast<AShooterHUD>(MyPC->GetHUD()) : NULL;
	//if (MyHUD)
	{
//		MyHUD->NotifyOutOfAmmo();
	}	
}

bool AShooterCharacter::SwitchWeaponIfNoAmmo()
{
	//if (Preferences.SwitchWeaponIfNoAmmo) @TODO
	return true;
}

UShooterCharacterMovement* AShooterCharacter::GetShooterCharacterMovement() const
{
	return Cast<UShooterCharacterMovement>(GetCharacterMovement());
}

void AShooterCharacter::FaceRotation(FRotator NewRotation, float DeltaTime)
{
	FRotator CurrentRotation = NewRotation;
	if (Controller && Controller->IsA(AShooterAIController::StaticClass()))
	{
		CurrentRotation = FMath::RInterpTo(GetActorRotation(), NewRotation, DeltaTime, 8.0f);
	}
	Super::FaceRotation(CurrentRotation, DeltaTime);
}

bool AShooterCharacter::ShouldTakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const
{
	if (GetTearOff())
	{
		return true;
	}
	return Super::ShouldTakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void AShooterCharacter::GetPlayerViewPointBP(FVector& Location, FRotator& Rotation) const
{
	GetActorEyesViewPoint(Location, Rotation);
	//The above's rotation doesn't updates pitch, on remote clients
	if (GetLocalRole() != ROLE_Authority)
	{
		Rotation = GetBaseAimRotation();
	}
}

void AShooterCharacter::PreReplication( IRepChangedPropertyTracker & ChangedPropertyTracker )
{
	Super::PreReplication( ChangedPropertyTracker );

	// Only replicate this property for a short duration after it changes so join in progress players don't get spammed with fx when joining late
	DOREPLIFETIME_ACTIVE_OVERRIDE( AShooterCharacter, LastTakeHitInfo, GetWorld() && GetWorld()->GetTimeSeconds() < LastTakeHitTimeTimeout );
}

void AShooterCharacter::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );
	
	DOREPLIFETIME_CONDITION( AShooterCharacter, LastTakeHitInfo,	COND_Custom );

	//owner only
	DOREPLIFETIME_CONDITION( AShooterCharacter, Inventory, COND_OwnerOnly );
	DOREPLIFETIME_CONDITION( AShooterCharacter, Mana, COND_OwnerOnly );
	DOREPLIFETIME_CONDITION( AShooterCharacter, MaxMana, COND_OwnerOnly );
	DOREPLIFETIME_CONDITION( AShooterCharacter, Armor, COND_OwnerOnly );
	
	// everyone except local owner: flag change is locally instigated
	DOREPLIFETIME_CONDITION( AShooterCharacter, bIsTargeting,		COND_SkipOwner );
	DOREPLIFETIME_CONDITION( AShooterCharacter, bWantsToRun,		COND_SkipOwner );
	
	// everyone
	DOREPLIFETIME( AShooterCharacter, CurrentWeapon );
	DOREPLIFETIME( AShooterCharacter, Health );
	DOREPLIFETIME(AShooterCharacter, AnimReplication);
	DOREPLIFETIME(AShooterCharacter, DamageScale);
}

bool AShooterCharacter::HasEnoughMana(float ManaRequired) const
{
	return Mana >= ManaRequired;
}

void AShooterCharacter::UseMana(float Amount)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		Mana = FMath::Max(0.f, Mana - Amount);
	}
}

void AShooterCharacter::RestoreMana(float Amount)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		Mana = FMath::Min(MaxMana, Mana + Amount);
	}
}

float AShooterCharacter::GetMaxMana() const
{
	return MaxMana;
}

float AShooterCharacter::GetMana() const
{
	return Mana;
}
