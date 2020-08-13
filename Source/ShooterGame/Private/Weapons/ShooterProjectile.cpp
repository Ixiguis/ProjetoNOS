// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#include "ShooterProjectile.h"

AShooterProjectile::AShooterProjectile()
{
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->AlwaysLoadOnClient = true;
	CollisionComp->AlwaysLoadOnServer = true;
	CollisionComp->bTraceComplexOnMove = true;

	//NoCollision to avoid colliding with owner upon spawn. Will be changed on InitProjectile().
	CollisionComp->SetCollisionProfileName(FName("NoCollision"));

	CollisionComp->CanCharacterStepUpOn = ECB_No;
	CollisionComp->BodyInstance.bUseCCD = true;
	RootComponent = CollisionComp;

	MainParticleComp = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("MainParticleComp"));
	MainParticleComp->bAutoActivate = true;
	MainParticleComp->bAutoDestroy = false;
	MainParticleComp->SetupAttachment(RootComponent);
	
	TrailParticleComp = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailParticleComp"));
	TrailParticleComp->bAutoActivate = true;
	TrailParticleComp->bAutoDestroy = false;
	TrailParticleComp->SetupAttachment(RootComponent);

	MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	MovementComp->UpdatedComponent = CollisionComp;
	MovementComp->InitialSpeed = 2000.0f;
	MovementComp->MaxSpeed = 2000.0f;
	MovementComp->bRotationFollowsVelocity = true;
	MovementComp->ProjectileGravityScale = 0.f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = false;
	SetReplicateMovement(true);
	bAlwaysReplicate = false;

	CanBeDeflected = true;
	ProjectileLife = 20.0f;
	ExplosionDamage = 30;
	ExplosionRadius = 128.0f;
	RandomSeed = 0;
	DamageType = UShooterDamageType::StaticClass();

	bExplodeOnImpact = true;
}

void AShooterProjectile::OnConstruction(const FTransform& Transform)
{
	if (bAlwaysReplicate)
	{
		SetReplicates(true);
	}
	else if (!GIsEditor && GetNetMode() != NM_Client)
	{
		SetReplicates(GetWorld()->GetAuthGameMode<AShooterGameMode>()->bReplicateProjectiles);
	}
	Super::OnConstruction(Transform);
}

void AShooterProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	MovementComp->OnProjectileStop.AddDynamic(this, &AShooterProjectile::OnImpact);
	MovementComp->OnProjectileBounce.AddDynamic(this, &AShooterProjectile::OnBounce);

	SetLifeSpan( ProjectileLife );
	MyController = GetInstigatorController();
}

void AShooterProjectile::InitProjectile(APawn* InInstigator, uint8& InRandomSeed, AShooterWeapon* InOwnerWeapon, FVector& ShootDirection)
{
	if (GetInstigator())
	{
		MyController = GetInstigator()->GetController();
	}
	
	CollisionComp->SetCollisionProfileName(FName("Projectile"));
	CollisionComp->MoveIgnoreActors.Add(GetInstigator());
	GetWorldTimerManager().SetTimer(StopIgnoringInstigatorHandle, this, &AShooterProjectile::StopIgnoringInstigator, 0.2f, false);

	RandomSeed = InRandomSeed;
	OwnerWeapon = InOwnerWeapon;
	InitVelocity(ShootDirection);

	if (OwnerWeapon)
	{
		//calling this event here instead of doing it on AShooterWeapon::FireProjectile.
		//if the server has set to replicate projectiles, AShooterWeapon::FireProjectile won't be called in clients.
		OwnerWeapon->WeaponPreFireEvent(OwnerWeapon->CurrentFireMode);
	}
}

void AShooterProjectile::StopIgnoringInstigator()
{
	CollisionComp->MoveIgnoreActors.Empty();
	//CollisionComp->SetCollisionProfileName(FName("Projectile"));
}

void AShooterProjectile::SetInstigator(AShooterCharacter* InstigatorCharacter)
{
	AActor::SetInstigator(InstigatorCharacter);
	MyController = InstigatorCharacter->Controller;
	OwnerWeapon = InstigatorCharacter->GetWeapon();
	SetOwner(OwnerWeapon);
}
void AShooterProjectile::InitVelocity(FVector& ShootDirection)
{
	if (MovementComp)
	{
		MovementComp->Velocity = ShootDirection * MovementComp->InitialSpeed;
	}
}

void AShooterProjectile::StopProjectile()
{
	if (MovementComp)
	{
		MovementComp->Velocity = FVector::ZeroVector;
	}
}

void AShooterProjectile::OnImpact(const FHitResult& HitResult)
{
	ProjectileImpact(HitResult);
	if (!bExploded && bExplodeOnImpact)
	{
		if (GetLocalRole() == ROLE_Authority)
		{
			Explode(HitResult);
		}
	}
}

void AShooterProjectile::OnBounce(const FHitResult& HitResult, const FVector& ImpactVelocity)
{
	ProjectileBounceEvent(HitResult, ImpactVelocity);
}

void AShooterProjectile::Explode(const FHitResult& Impact)
{
	if (MainParticleComp)
	{
		MainParticleComp->SetHiddenInGame(true);
		MainParticleComp->Deactivate();
	}
	if (TrailParticleComp)
	{
		TrailParticleComp->Deactivate();
	}

	ProjectileExploded(Impact);

	// effects and damage origin shouldn't be placed inside mesh at impact point
	const FVector NudgedImpactLocation = Impact.ImpactPoint + Impact.ImpactNormal * 10.0f;
	if (ExplosionTemplate)
	{
		FTransform const SpawnTransform(Impact.ImpactNormal.Rotation(), NudgedImpactLocation);
		AShooterExplosionEffect* const EffectActor = GetWorld()->SpawnActorDeferred<AShooterExplosionEffect>(ExplosionTemplate, SpawnTransform);
		if (EffectActor)
		{
			EffectActor->SurfaceHit = Impact;
			UGameplayStatics::FinishSpawningActor(EffectActor, SpawnTransform);
		}
	}

	if (ExplosionDamage > 0 && ExplosionRadius > 0 && DamageType)
	{
		UGameplayStatics::ApplyRadialDamage(this, ExplosionDamage, NudgedImpactLocation, ExplosionRadius, DamageType, TArray<AActor*>(), this, MyController.Get());
	}
	else if (ExplosionDamage > 0 && DamageType && Impact.GetActor())
	{
		UGameplayStatics::ApplyDamage(Impact.GetActor(), ExplosionDamage, MyController.Get(), this, DamageType);
	}
	bExploded = true;

	DisableAndDestroy();
}

void AShooterProjectile::DisableAndDestroy()
{
	UAudioComponent* ProjAudioComp = FindComponentByClass<UAudioComponent>();
	if (ProjAudioComp && ProjAudioComp->IsPlaying())
	{
		ProjAudioComp->FadeOut(0.1f, 0.f);
	}
	MovementComp->StopMovementImmediately();
	CollisionComp->SetCollisionProfileName(FName("NoCollision"));

	//for some reason this causes a crash sometimes, let the projectile live for its ProjectileLife for now
	//SetLifeSpan(6.0f);
}

void AShooterProjectile::OnRep_Exploded()
{
	//FVector ProjDirection = GetActorRotation().Vector();
	FVector ProjDirection = MovementComp->Velocity;
	if (!ProjDirection.Normalize())
	{
		ProjDirection = FVector(0.f, 0.f, -1.f);
	}
	static FName ProjTag = FName(TEXT("ProjectileTrace"));
	const AActor* IgnoreActor = MovementComp->bShouldBounce ? NULL : GetInstigator();
	FCollisionQueryParams TraceParams(ProjTag, true, IgnoreActor);

	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	const FVector StartTrace = GetActorLocation() - ProjDirection * (FMath::Max(MovementComp->MaxSpeed, MovementComp->InitialSpeed) * DeltaTime + 20);
	const FVector EndTrace = GetActorLocation() + ProjDirection * (FMath::Max(MovementComp->MaxSpeed, MovementComp->InitialSpeed) * DeltaTime + 20);
	const float CollisionSphereRadius = CollisionComp->GetScaledSphereRadius();
	FHitResult Impact;
	if ( !GetWorld()->SweepSingleByChannel(Impact, StartTrace, EndTrace, FQuat(ForceInit), COLLISION_PROJECTILE, FCollisionShape::MakeSphere(CollisionSphereRadius), TraceParams) )
	{
		// failsafe
		Impact.ImpactPoint = GetActorLocation();
		Impact.ImpactNormal = -ProjDirection;
	}
	Explode(Impact);
}

void AShooterProjectile::ExplodeProjectile()
{
	if (GetLocalRole() == ROLE_Authority && !bExploded)
	{
		OnRep_Exploded();
	}
}

void AShooterProjectile::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	if (MovementComp)
	{
		MovementComp->Velocity = NewVelocity;
	}
}

void AShooterProjectile::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );
	
	DOREPLIFETIME( AShooterProjectile, bExploded );
	DOREPLIFETIME( AShooterProjectile, OwnerWeapon );
}
