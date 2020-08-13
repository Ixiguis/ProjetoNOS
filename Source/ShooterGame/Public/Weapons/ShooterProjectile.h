// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.
#pragma once

#include "ShooterWeapon.h"
#include "ShooterProjectile.generated.h"

// 
UCLASS(Abstract, Blueprintable)
class AShooterProjectile : public AActor
{
	GENERATED_BODY()

public:
	AShooterProjectile();

	/** initial setup */
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;

	/** setup velocity */
	void InitVelocity(FVector& ShootDirection);

	void InitProjectile(APawn* InInstigator, uint8& InRandomSeed, AShooterWeapon* InOwnerWeapon, FVector& ShootDirection);

	/** handle hit */
	UFUNCTION()
	void OnImpact(const FHitResult& HitResult);

	/** handle bounce */
	UFUNCTION()
	void OnBounce(const FHitResult& HitResult, const FVector& ImpactVelocity);
	
	/** damage at impact point */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=Projectile)
	int32 ExplosionDamage;

	/** radius of damage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=Projectile)
	float ExplosionRadius;
	
	/** [server] called when projectile hit something */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category=Projectile)
	void ProjectileImpact(const FHitResult& HitResult);
	
	UFUNCTION(BlueprintImplementableEvent, Category=Projectile)
	void ProjectileBounceEvent(const FHitResult& HitResult, const FVector& ImpactVelocity);

	/** [everyone] called when projectile exploded */
	UFUNCTION(BlueprintImplementableEvent, Category = Projectile)
	void ProjectileExploded(const FHitResult& HitResult);

	/** Call to explode this projectile. */
	UFUNCTION(BlueprintCallable, Category=Projectile)
	void ExplodeProjectile();
	
	/** call to explode this projectile */
	UFUNCTION(BlueprintCallable, Category=Projectile)
	void StopProjectile();
	
	/** set the proejctile's owner (useful if spawned using SpawnActor(), rather than created by Weapon::ServerFireProjectile() ) */
	UFUNCTION(BlueprintCallable, Category=Projectile)
	void SetInstigator(AShooterCharacter* InstigatorCharacter);
	
	/** type of damage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=Projectile)
	TSubclassOf<UShooterDamageType> DamageType;
	
	/** effects for explosion */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=Effects)
	TSubclassOf<class AShooterExplosionEffect> ExplosionTemplate;

	/** whether this projectile can be deflected with the sword. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Projectile)
	bool CanBeDeflected;
	
	/** If this projectile was created by a weapon, this is the random seed it used. Synced in all clients. */
	UPROPERTY(BlueprintReadOnly, Category = Projectile)
	uint8 RandomSeed;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category=Projectile)
	AShooterWeapon* OwnerWeapon;

protected:
	
	/** life time */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	float ProjectileLife;
	
	/** If true, then this projectile will always replicate from server to clients, even if server has the setting "bReplicateProjectiles" set to false. */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	bool bAlwaysReplicate;
	
	/** should automatically explode on impact? */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Projectile)
	bool bExplodeOnImpact;

	/** movement component */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Projectile)
	UProjectileMovementComponent* MovementComp;

	/** collisions */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Projectile)
	USphereComponent* CollisionComp;

	/** The projectile's particle component, e.g. the missile itself. Is deactivated and hidden immediately upon explosion. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Projectile)
	UParticleSystemComponent* MainParticleComp;
	
	/** The projectile's trail particle component, e.g. the missile's smoke trail. Is deactivated, but not hidden, upon explosion. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Projectile)
	UParticleSystemComponent* TrailParticleComp;

	/** controller that fired me (cache for damage calculations). 
	*	Use this to determine damage instigator, rather than GetInstigatorController, because instigator's controller is NULL when pawn dies (and the projectile may still damage players). */
	UPROPERTY(BlueprintReadOnly, Category = Projectile, Meta = (Keywords = "instigator"))
	TWeakObjectPtr<AController> MyController;

	/** did it explode? */
	UPROPERTY(Transient, BlueprintReadOnly, ReplicatedUsing=OnRep_Exploded, Category=Projectile)
	bool bExploded;

	/** [client] explosion happened */
	UFUNCTION()
	void OnRep_Exploded();

	/** trigger explosion */
	void Explode(const FHitResult& Impact);

	/** shutdown projectile and prepare for destruction */
	void DisableAndDestroy();

	/** update velocity on client */
	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) override;

	virtual void StopIgnoringInstigator() final;

	FTimerHandle StopIgnoringInstigatorHandle;

	/** reference to the level's DirectionalLight */
	ADirectionalLight* Sun;
};
