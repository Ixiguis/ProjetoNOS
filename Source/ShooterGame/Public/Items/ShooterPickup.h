// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Engine/Canvas.h"
#include "ShooterPickup.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBindableEvent_PickupPickedup, AShooterPickup*, Pickup);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBindableEvent_PickupRespawned, AShooterPickup*, Pickup);

// Base class for pickup objects that can be placed in the world
UCLASS(Abstract, Blueprintable)
class AShooterPickup : public AActor
{
	GENERATED_BODY()

public:
	AShooterPickup();

	virtual void PostInitializeComponents() override;

	/** pickup on touch */
	virtual void NotifyActorBeginOverlap(class AActor* Other) override;

	/** check if pawn can use this pickup */
	virtual bool CanBePickedUp(class AShooterCharacter* TestPawn);

	/** initial setup */
	virtual void BeginPlay() override;
	
	/** how long it takes to respawn? 0 means no respawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Pickup)
	float RespawnTime;
	
	/** is it ready for interactions? */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_IsActive)
	uint32 bIsActive:1;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Collision)
	class UProjectileMovementComponent* MovementComp;
	
	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void SetVelocity(FVector_NetQuantize10 NewVelocity);
	
	/** if WasDropped, destroys itself on pickup and activates on BeginPlay even if SpawnAtGameStart == false */
	bool WasDropped;
	
	/** this pickup will destroy itself after being picked up */
	UPROPERTY(BlueprintReadWrite, Category = Pickup)
	bool DestroyOnPickup;

	/** if true, this item will stay on active after being picked up, but can only be picked up once per character */
	UPROPERTY(Replicated)
	bool PickupStay;
	
	/** Icon to draw when picked up. */
	UPROPERTY(EditAnywhere, Category=HUD)
	FCanvasIcon PickupIcon;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Pickup)
	void GetPickupMessage(FText& Text, class UTexture2D*& LeftImage, class UTexture2D*& RightImage) const;

protected:
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Collision)
	class UCapsuleComponent* CollisionComp;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Pickup)
	class USkeletalMeshComponent* PickupMeshComp;

	/** FX component */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Effects)
	class UParticleSystemComponent* PickupPSC;
	
	/** FX of active pickup */
	UPROPERTY(EditAnywhere, Category=Effects)
	class UParticleSystem* ActiveFX;
	
	/** If true, this pickup will simulate physics (will fall until it hits the ground). */
	UPROPERTY(EditAnywhere, Category = Effects)
	uint32 bSimulatePhysics : 1;

	/** FX of pickup on respawn timer */
	UPROPERTY(EditAnywhere, Category=Effects)
	class UParticleSystem* RespawningFX;

	/** sound played when player picks it up */
	UPROPERTY(EditAnywhere, Category=Effects)
	class USoundCue* PickupSound;

	/** sound played on respawn */
	UPROPERTY(EditAnywhere, Category=Effects)
	class USoundCue* RespawnSound;
	
	/** if false, this pickup will spawn only after its RespawnTime, on match start */
	UPROPERTY(EditAnywhere, Category=Pickup)
	bool SpawnAtGameStart;

	/** returns true if TestPawn already picked up this powerup. */
	bool PawnAlreadyPickedUp(class AShooterCharacter* TestPawn);

	/** characters that picked up this powerup. Only filled if PickupStay == true. */
	TArray<class AShooterCharacter*> Pickers;

	/** removes characters that are dead or no longer exist from Pickers array */
	void CleanupPickers();

	/* The character who has picked up this pickup */
	UPROPERTY(Transient)
	class AShooterCharacter* PickedUpBy;

	UFUNCTION()
	void OnRep_IsActive();

	/** what this pickup does (restore health, give ammo, etc...) */
	virtual void GivePickupTo(class AShooterCharacter* Pawn);
	
	/** [server] what this pickup does (restore health, give ammo, etc...), blueprint override. Called in addition to GivePickupTo(). */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category=Pickup)
	void GivePickupTo_BP(class AShooterCharacter* Pawn);

	/** handle touches */
	virtual void PickupOnTouch(class AShooterCharacter* Pawn);

	/** show and enable pickup */
	virtual void RespawnPickup();

	/** show effects when pickup disappears */
	virtual void OnPickedUp();

	/** show effects when pickup appears */
	virtual void OnRespawned();

	/** blueprint event: pickup disappears */
	UFUNCTION(BlueprintImplementableEvent, Category=Pickup)
	void OnPickedUpEvent();

	/** blueprint event: pickup appears */
	UFUNCTION(BlueprintImplementableEvent, Category=Pickup)
	void OnRespawnEvent();
	
	/** Called when pickup is picked up */
	UPROPERTY(BlueprintAssignable, Category=Pickup)
	FBindableEvent_PickupPickedup OnPickupPickedupDelegate;
	
	/** Called when pickup respawns */
	UPROPERTY(BlueprintAssignable, Category=Pickup)
	FBindableEvent_PickupRespawned OnPickupRespawnedDelegate;
	
	UPROPERTY(Transient, Replicated)
	float RespawnTimeMultipler;

	UPROPERTY(Transient, Replicated)
	float WarmupTime;

	UFUNCTION(BlueprintCallable, Category = Pickup)
	float GetRespawnTime() const;

	FTimerHandle RespawnPickupHandle;
	FTimerHandle CleanupPickersHandle;
};
