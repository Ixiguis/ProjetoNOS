// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "ShooterDamageType.h"
#include "ShooterItem.h"
#include "ShooterWeapon.generated.h"

#define NUM_FIRING_MODES 2

namespace EWeaponState
{
	enum Type
	{
		Idle,
		Firing,
		Reloading,
		Equipping,
	};
}

/** This determines which animation to play on pawn owner. 
 *	After adding an anim, assign its animation on the anim blueprint's AnimGraph (look for the node "Blend Poses (EWeaponAnim)"). */
UENUM(BlueprintType)
namespace EWeaponAnim
{
	enum Type
	{
		Melee,
		Small,
		Rifle,
		Gastraphetes,
		OnShoulder,
		ArmAttached,
		Spell,
		Chaingun,
	};
}

UENUM()
enum EFireMode
{
	//does nothing
	FM_NotAvailable,
	//fires an instant-hit shot
	FM_Instant,
	//fires a physical projectile
	FM_Projectile,
	/** Custom behavior.
	*	Does not interrupts other firing modes (i.e. if this is set on secondary FireMode, it will not interrupt primary fire if the user presses primary fire and secondary fire simultaneously).
	*	Use the events StartCustomFire and StopCustomFire to define behavior, in Blueprint.
	*	Note: does not calls HandleFiring(), and does not sets CurrentFireMode or bWantsToFire. Also does not checks if weapon has ammo, and does not consumes ammo. */
	FM_Custom,
	//click and hold to charge the weapon. Does nothing other than calling WeaponStartChargeEvent and WeaponStopChargeEvent. Prevents other firing modes while charging.
	FM_Charge,
	//continually draws the trail fx to target, but otherwise functions just like FM_Instant. Configure parameters on InstantConfig. Be sure to use an infinite TrailFX particle, or the game will crash.
	FM_Beam,
};

USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_USTRUCT_BODY()

	/** infinite ammo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Ammo)
	bool bInfiniteAmmo;

	/** infinite ammo in clip, no reload required. AmmoPerClip and InitialClips are ignored if true. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Ammo)
	bool bInfiniteClip;

	/** max ammo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Ammo)
	int32 MaxAmmo;

	/** Initial ammo (weapon just picked up) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Ammo)
	int32 InitialAmmo;
	
	/** amount of ammo to consume after shooting, for each firing mode. If fire mode is Charge, this is the ammo consumed per second. */
	UPROPERTY(EditDefaultsOnly, Category=Ammo)
	uint8 AmmoToConsume[NUM_FIRING_MODES];

	/** clip size */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Ammo, Meta=(EditCondition = "!bInfiniteClip"))
	int32 AmmoPerClip;
	
	/** failsafe reload duration if weapon doesn't have any animation for it */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Ammo)
	float NoAnimReloadDuration;
	
	/** base (initial) weapon Dispersion (degrees) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Accuracy, meta=(ClampMin="0.0", ClampMax="90.0", UIMin = "0.0", UIMax = "90.0"))
	float BaseFiringDispersion;

	/** firing dispersion increment for each firing mode; incremented with each shot. */
	UPROPERTY(EditDefaultsOnly, Category=Accuracy, meta=(ClampMin="0.0", ClampMax="90.0", UIMin = "0.0", UIMax = "20.0"))
	float FiringDispersionIncrement[NUM_FIRING_MODES];
	
	/** continuous firing: Dispersion decrement per second while not firing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Accuracy)
	float FiringDispersionDecrement;
	
	/** continuous firing: max increment */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Accuracy, meta=(ClampMin="0.0", ClampMax="90.0", UIMin = "0.0", UIMax = "90.0"))
	float FiringDispersionMax;
	
	/** whether firing dispersion should be overridden (will not increase by firing or decrease by not firing; adjust behaviour in blueprints) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Accuracy)
	bool OverrideDispersion;

	/** defaults */
	FWeaponData()
	{
		bInfiniteAmmo = false;
		MaxAmmo = 100;
		bInfiniteClip = true;
		BaseFiringDispersion = 0.0f;
		FiringDispersionMax = 0.0f;
		OverrideDispersion = false;
		for (uint8 i=0; i<NUM_FIRING_MODES; i++)
		{
			FiringDispersionIncrement[i] = 0.0f;
			AmmoToConsume[i] = 1;
		}
		FiringDispersionDecrement = 1.f;
		AmmoPerClip = 20;
		NoAnimReloadDuration = 1.0f;
	}
};

USTRUCT(BlueprintType)
struct FWeaponAnim
{
	GENERATED_USTRUCT_BODY()

	/** animation played on pawn (1st person view) */
	UPROPERTY(EditDefaultsOnly, Category=Animation)
	UAnimMontage* Pawn1P;

	/** animation played on pawn (3rd person view) */
	UPROPERTY(EditDefaultsOnly, Category=Animation)
	UAnimMontage* Pawn3P;
};

UENUM(BlueprintType)
namespace EMuzzleChooser
{
	enum Type
	{
		/** picks a random muzzle attach point each time the weapon shoots */
		Random,
		/** picks muzzle attach point sequentially with each shot */
		Sequential,
		/** uses MuzzleAttachPoint[0] for fire mode 0, MuzzleAttachPoint[1] for fire mode 1, etc. */
		FireMode,
	};
}

/** firing effects for each fire mode */
USTRUCT(BlueprintType)
struct FWeaponEffects
{
	GENERATED_USTRUCT_BODY()

	/** FX for muzzle flash */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	UParticleSystem* MuzzleFX;

	/** camera shake on firing */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	TSubclassOf<UCameraShake> FireCameraShake;

	/** single fire sound (or start fire sound if bLoopedFireSound is true) */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* FireSound;
	
	/** looped fire sound (bLoopedFireSound set) */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* FireLoopSound;
	
	/** finished burst sound (bLoopedFireSound set) */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* FireLoopFinishSound;
	
	/** fire animations */
	UPROPERTY(EditDefaultsOnly, Category=Animation)
	FWeaponAnim FireAnim;
	
	/** is muzzle FX looped? */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	bool bLoopedMuzzleFX;

	/** is fire sound looped? */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	bool bLoopedFireSound;
	
	/** is fire animation looped? */
	UPROPERTY(EditDefaultsOnly, Category=Animation)
	bool bLoopedFireAnim;
	
	/** impact effects */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	TSubclassOf<class AShooterImpactEffect> ImpactTemplate;

	/** tracer effect */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	UParticleSystem* TrailFX;
	
	/** if true, the trail will spawn attached to weapon's muzzle attach point (follow it when the weapon moves), otherwise stays where it spawned */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	bool SpawnTrailAttached;
	
	/** define how to use the MuzzleAttachPoints */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	TEnumAsByte<EMuzzleChooser::Type> MuzzleChooseMethod;
	
	/** controls current muzzle index, for EMuzzleChooser::Sequential */
	uint8 CurrentMuzzleIndex;

	FWeaponEffects()
	{
		bLoopedMuzzleFX = false;
		bLoopedFireAnim = false;
		SpawnTrailAttached = false;
		MuzzleChooseMethod = EMuzzleChooser::Sequential;
		CurrentMuzzleIndex = 0;
	}
};

USTRUCT()
struct FChargingInfo
{
	GENERATED_USTRUCT_BODY()
		
	UPROPERTY()
	uint32 bIsCharging : 1;
	
	UPROPERTY()
	uint8 RandomSeed;
	
	FChargingInfo()
	{
		bIsCharging = false;
		RandomSeed = 0;
	}
};

USTRUCT(BlueprintType)
struct FInstantWeaponData
{
	GENERATED_USTRUCT_BODY()

	/** weapon range (max 1048576.0) */
	UPROPERTY(EditDefaultsOnly, Category=WeaponStat)
	float WeaponRange;
	
	/** shot thickness (bullet caliber; diameter of the trace) */
	UPROPERTY(EditDefaultsOnly, Category=WeaponStat)
	float ShotThickness;

	/** damage amount */
	UPROPERTY(EditDefaultsOnly, Category=WeaponStat)
	int32 HitDamage;
	
	/** how many bullets to spawn with each shot (does not consume additional ammo, use e.g. for shotgun pellets) */
	UPROPERTY(EditDefaultsOnly, Category=WeaponStat)
	uint8 BulletsToSpawn;
	
	/** type of damage */
	UPROPERTY(EditDefaultsOnly, Category=WeaponStat)
	TSubclassOf<UShooterDamageType> DamageType;
	
	/** number of bounces */
	UPROPERTY(EditDefaultsOnly, Category=WeaponStat)
	uint8 Bounces;

	/** for each bounce, how much to multiply damage. For instance, if BounceDamageMultiplier is 0.5, 
	 *	a direct hit will do 100% damage, if bounced once it will do 50%, if bounced twice it will do 25%, and so on. */
	UPROPERTY(EditDefaultsOnly, Category=WeaponStat)
	float BounceDamageMultiplier;

	/** Damage multiplier in case of a headshot */
	UPROPERTY(EditDefaultsOnly, Category=WeaponStat)
	float HeadshotDamageMultiplier;
	
	/** hit verification: scale for bounding box of hit actor */
	UPROPERTY(EditDefaultsOnly, Category=HitVerification)
	float ClientSideHitLeeway;

	/** hit verification: threshold for dot product between view direction and hit direction */
	UPROPERTY(EditDefaultsOnly, Category=HitVerification)
	float AllowedViewDotHitDir;

	/** defaults */
	FInstantWeaponData()
	{
		WeaponRange = 1048576.0f; //max FVector_NetQuantize size. Do not use larger values (unless you modify FHitResult).
		ShotThickness = 1.f;
		HitDamage = 10;
		DamageType = UShooterDamageType::StaticClass();
		ClientSideHitLeeway = 200.0f;
		Bounces=0;
		BounceDamageMultiplier = 1.0f;
		HeadshotDamageMultiplier = 1.3f;
		AllowedViewDotHitDir = 0.8f;
		BulletsToSpawn = 1;
	}
};

UCLASS(Abstract, Blueprintable, NotPlaceable)
class AShooterWeapon : public AShooterItem
{
	GENERATED_BODY()

public:
	AShooterWeapon();

	/** perform initial setup */
	virtual void PostInitializeComponents() override;

	virtual void Destroyed() override;

	virtual void Tick(float DeltaSeconds) override;
	
	//////////////////////////////////////////////////////////////////////////
	// Ammo
	
	/** consume a bullet */
	void UseAmmo();

	/** check if weapon has infinite ammo (include owner's cheats) */
	bool HasInfiniteAmmo() const;

	/** gives this weapon's ammo to character owner */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Ammo)
	void GiveAmmo(int32 Amount);

	//////////////////////////////////////////////////////////////////////////
	// Inventory
	
	/** weapon is being equipped by owner pawn */
	virtual void OnEquip();

	/** weapon is now equipped by owner pawn */
	virtual void OnEquipFinished();

	FTimerHandle OnEquipFinishedHandle;

	/** weapon is holstered by owner pawn */
	virtual void OnUnEquip();

	/** check if it's currently equipped */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	bool IsEquipped() const;

	/** check if mesh is already attached */
	bool IsAttachedToPawn() const;
	
	/** gets last time when this weapon was switched to */
	float GetEquipStartedTime() const;

	/** gets the duration of equipping weapon*/
	float GetEquipDuration() const;
	
	/** set the weapon's owning pawn */
	void SetOwningPawn(AShooterCharacter* AShooterCharacter);
	
	/** get current ammo amount (ammo in clip, if weapon requires reloading) */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	int32 GetCurrentAmmo() const;
	
	/** get current total ammo amount (ammo in inventory + ammo in weapon clip, if weapon requires reloading) */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	int32 GetCurrentTotalAmmo() const;

	//////////////////////////////////////////////////////////////////////////
	// Input

	/** [local + server] start weapon fire */
	virtual void StartFire(uint8 FireMode);

	/** [local + server] stop weapon fire */
	virtual void StopFire(uint8 FireMode);

	/** [everyone] start weapon reload */
	UFUNCTION(BlueprintCallable, Category=Ammo)
	virtual void StartReload(bool bFromReplication = false);

	/** [local + server] interrupt weapon reload */
	virtual void StopReload();

	FTimerHandle StopReloadHandle;

	/** [server] performs actual reload */
	virtual void ReloadWeapon();
	
	FTimerHandle ReloadWeaponHandle;
	//////////////////////////////////////////////////////////////////////////
	// Control

	/** check if weapon can fire */
	bool CanFire() const;

	/** check if weapon can be reloaded */
	UFUNCTION(BlueprintCallable, Category=Ammo)
	bool CanReload() const;
	
	/** fire mode being currently used (if firing) */
	UPROPERTY(Transient, Replicated)
	uint8 CurrentFireMode;

	/** get current ammo amount (clip) */
	int32 GetCurrentAmmoInClip() const;

	/** get clip size */
	int32 GetAmmoPerClip() const;
	
	/** directly modifies current firing Dispersion, limited by base BaseFiringDispersion and BaseFiringDispersionMax. Be sure to check WeaponConfig.OverrideDispersion if using this. 
	*	Clients can modify this, but the server will replicate its value to owning client. */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	void IncrementFiringDispersion(float Increment);

	/** directly modifies current firing Dispersion, limited by base BaseFiringDispersion and BaseFiringDispersionMax. Be sure to check WeaponConfig.OverrideDispersion if using this. 
	*	Clients can modify this, but the server will replicate its value to owning client. */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	void SetCurrentFiringDispersion(float NewDispersion);
	
	/** returns the current weapon's firing dispersion. */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	float GetFiringDispersion() const;
	
	/** returns the base (initial) firing dispersion. */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	float GetBaseFiringDispersion() const;
	
	//////////////////////////////////////////////////////////////////////////
	// Reading data

	/** get current weapon state */
	EWeaponState::Type GetCurrentState() const;
	
	UFUNCTION(BlueprintCallable, Category=Weapon)
	int32 GetInstantHitDamage(uint8 FireMode) const;
	
	/** get max ammo amount. If GetMaxClipAmmo, returns max clip size, instead of global max ammo. */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	int32 GetMaxAmmo(bool GetMaxClipAmmo = false) const;

	/** get initial ammo amount */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	int32 GetInitialAmmo() const;
	
	/** returns true if weapon has enough ammo to shoot */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	bool HasEnoughAmmo() const;

	/** get weapon mesh (needs pawn owner to determine variant) */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	USkeletalMeshComponent* GetWeaponMesh() const;
	
	/** get first person weapon mesh */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	USkeletalMeshComponent* GetWeaponMesh1P() const;

	/** get third person weapon mesh*/
	UFUNCTION(BlueprintCallable, Category=Weapon)
	USkeletalMeshComponent* GetWeaponMesh3P() const;
	
	/** Returns whether owning pawn is in first person view */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	bool IsFirstPerson() const;

	/** get pawn owner */
	UFUNCTION(BlueprintCallable, Category="Game|Weapon")
	class AShooterCharacter* GetPawnOwner() const;
	
	/** get pawn owner's controller */
	UFUNCTION(BlueprintCallable, Category="Game|Weapon")
	class AController* GetPawnOwnerController() const;
	
	/** returns whether this weapon is currently charging (if fire mode is FM_Charge). Synced in all clients. */
	UFUNCTION(BlueprintCallable, Category = "Charge")
	bool IsCharging() const;

	/** amount of time passed since user started charging */
	UFUNCTION(BlueprintCallable, Category=Charge)
	float GetChargingTime() const;
	
	/** returns ChargingTime / MaxChargeTime */
	UFUNCTION(BlueprintCallable, Category=Charge)
	float GetChargingPercent() const;
	
	/** returns MaxChargeTime + ChargeRandomAdd, if applicable. */
	UFUNCTION(BlueprintCallable, Category=Charge)
	float GetMaxChargingTime() const;

	/** name of bone/socket for holding the weapon with the right hand. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Visuals)
	FName RightHandAttachPoint;

	/** name of bone/socket for holding the weapon with the left hand. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Visuals)
	FName LeftHandAttachPoint;

	//////////////////////////////////////////////////////////////////////////
	// HUD
	/** weapon icon, displayed on the hud */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = HUD)
	UTexture2D* WeaponIcon;

	/** crosshair texture */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = HUD)
	UTexture2D* Crosshair;

	UFUNCTION(BlueprintCallable, Category = HUD)
	void SetCrosshair(UTexture2D* NewCrosshair);

	/** Weapon category, used for key bindings and for showing weapons on the HUD */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=HUD)
	uint8 WeaponCategory;

	//////////////////////////////////////////////////////////////////////////
	// Misc
	
	bool WantsToFire() const;
	
	/** set current TimeBetweenShots. Clients can call this too, but the server will replicate its value to the owner. */
	UFUNCTION(BlueprintCallable, Category=Weapon, Meta=(Keywords="TimeBetweenShots"))
	void SetFireRate(uint8 FireModeIndex, float NewTimeBetweenShots);
	
	/** get current TimeBetweenShots */
	UFUNCTION(BlueprintCallable, Category=Weapon, Meta=(Keywords="TimeBetweenShots"))
	float GetFireRate(uint8 FireModeIndex) const;
	
	/** sets how many shots/projectiles are spawned whenever the weapon fires */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	void SetShotsPerTick(uint8 FireMode, uint8 NewShotsPerTick);
	
	/** returns how many shots/projectiles are spawned whenever the weapon fires */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	uint8 GetShotsPerTick(uint8 FireMode) const;
	
	/** returns weapon effects for given fire mode */
	UFUNCTION(BlueprintCallable, Category=Effects)
	FWeaponEffects GetWeaponEffects(uint8 FireMode) const;

	/** modify the fire mode (projectile, instant, custom, etc) for the specified FireModeIndex */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Weapon)
	void SetFiringMode(uint8 FireModeIndex, EFireMode NewFireMode);
	
	/** get the fire mode (projectile, instant, custom) for the specified FireModeIndex */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	TEnumAsByte<enum EFireMode> GetFiringMode(uint8 FireModeIndex) const;
	
	UFUNCTION(BlueprintCallable, Category=Visuals)
	void SetFireSound(USoundCue* NewSound, uint8 FireMode);

	//////////////////////////////////////////////////////////////////////////
	// Instant hit
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Weapon)
	void SetInstantHitDamage(uint8 FireMode, int32 NewDamage);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Weapon)
	void SetInstantHitDamageType(uint8 FireMode, TSubclassOf<UShooterDamageType> NewDamageType);
	
	UFUNCTION(BlueprintCallable, Category=Weapon)
	TSubclassOf<UShooterDamageType> GetInstantHitDamageType(uint8 FireMode) const;
	
	FInstantWeaponData GetInstantConfig(uint8 FireMode) const;
	
	UFUNCTION(BlueprintCallable, Category=Visuals)
	void SetTrailFX (UParticleSystem* NewTrailFX, uint8 FireMode);
	
	/** Whether players should be awarded by killing with a headshot with this weapon (typically true for sniper rifles); instant-hit only */
	UPROPERTY(EditDefaultsOnly, Category=Weapon)
	bool bRewardHeadshots;

	//////////////////////////////////////////////////////////////////////////
	// Projectile

	/** set projectile class (from Blueprint) */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	void SetProjectileClass(uint8 FireModeIndex, TSubclassOf<class AShooterProjectile> NewClass);
	
	//////////////////////////////////////////////////////////////////////////
	// Blueprint events
	
	/** [everyone] just before the weapon fires */
	UFUNCTION(BlueprintImplementableEvent, Category=Weapon)
	void WeaponPreFireEvent(uint8 FireMode);
	
	/** [local + server] called once when user presses the fire button */
	UFUNCTION(BlueprintImplementableEvent, Category=Weapon)
	void UserStartedFiringEvent(uint8 FireMode);

	/** [local + server] called once when user releases the fire button */
	UFUNCTION(BlueprintImplementableEvent, Category=Weapon)
	void UserStoppedFiringEvent(uint8 FireMode);
	
	/** [local + server] called once when user presses the fire button and fire mode is FM_Custom */
	UFUNCTION(BlueprintImplementableEvent, Category=Weapon)
	void StartCustomFire(uint8 FireMode);

	/** [local + server] called once when user releases the fire button and fire mode is FM_Custom */
	UFUNCTION(BlueprintImplementableEvent, Category=Weapon)
	void StopCustomFire(uint8 FireMode);
	
	/** [everyone] called when weapon is being equipped (not ready to fire yet) */
	UFUNCTION(BlueprintImplementableEvent, Category=Weapon)
	void WeaponBeingEquippedEvent();

	/** [everyone] called when weapon is equipped and ready to fire */
	UFUNCTION(BlueprintImplementableEvent, Category=Weapon)
	void WeaponEquippedEvent();

	/** [everyone] called when weapon is unequipped */
	UFUNCTION(BlueprintImplementableEvent, Category=Weapon)
	void WeaponUnequippedEvent();
	
	/** [everyone] called when weapon is fired and a TracerFX is created. Returns the particle component. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = Weapon)
	void TracerCreatedEvent(UParticleSystemComponent* ParticleComponent, const uint8 FireMode, const uint8 BounceNum, const FVector StartPoint, const FVector EndPoint);
	
	/** [server] called when weapon dealt damage to an actor (instant hit only). */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category=Weapon)
	void WeaponDealtDamageEvent(FHitResult HitResult, FPointDamageEvent DamageInfo, FVector ShootDir, uint8 FireMode, uint8 BounceNumber);
	
	/** [everyone] called when weapon hit anything (instant hit only) */
	UFUNCTION(BlueprintImplementableEvent, Category=Weapon)
	void WeaponInstantHitEvent(uint8 FireMode, FHitResult Impact, AShooterCharacter* InstigatorCharacter);
	
	/** [everyone] called when weapon tracer bounced (if InstantConfig.NumBounces > 1) */
	UFUNCTION(BlueprintImplementableEvent, Category=Weapon)
	void InstantHitBounceEvent(FHitResult Impact, uint8 BounceNum);

	/** [everyone] started charging and has ammo to charge */
	UFUNCTION(BlueprintImplementableEvent, Category=Weapon)
	void WeaponStartChargeEvent(uint8 FireMode);
	
	/** [everyone] stopped charging */
	UFUNCTION(BlueprintImplementableEvent, Category=Weapon)
	void WeaponStopChargeEvent(uint8 FireMode, float TotalChargingTime);
	
	/** [everyone] max charge time reached */
	UFUNCTION(BlueprintImplementableEvent, Category=Weapon)
	void WeaponMaxChargeReachedEvent(uint8 FireMode, float TotalChargingTime);
	
	/** [server] called when user scores a kill */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category=Weapon)
	void KilledEvent(APawn* KilledPawn, AController* KilledPlayer, TSubclassOf<class AShooterWeapon> KillerWeaponClass, TSubclassOf<class UShooterDamageType> KillerDmgType);
		
	/** [everyone] called on SimulateWeaponFire. Gameplay stuff should not go here (not called on dedicated servers). */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category=Effects)
	void SimulateWeaponFireEvent(uint8 FireMode, UParticleSystemComponent* InMuzzlePSC);
	
	/** [everyone] called on StopSimulatingWeaponFire. Gameplay stuff should not go here (not called on dedicated servers). */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category=Effects)
	void StopSimulatingWeaponFireEvent(uint8 FireMode);
	
	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** attaches weapon mesh to pawn's mesh */
	void AttachMeshToPawn();

	/** detaches weapon mesh from pawn */
	void DetachMeshFromPawn();

	/** called by Character when pawn owner dies */
	void OwnerDied();

	/** How far the AI thinks the weapon can shoot. While a shotgun may have a very long range, in practice
	  * it shouldn't be used from afar, so this is where you should configure the weapon range for the AI, for each fire mode. */
	UPROPERTY(EditDefaultsOnly, Category=AI)
	float AIWeaponRange[NUM_FIRING_MODES];
	
	/** check if weapon has infinite clip (include owner's cheats) */
	bool HasInfiniteClip() const;
	
	/** current ammo - inside clip */
	UPROPERTY(Transient, Replicated)
	uint16 CurrentAmmoInClip;
	
protected:
	
	//////////////////////////////////////////////////////////////////////////
	// Basic config
	/** weapon data */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Config)
	FWeaponData WeaponConfig;
	
	/** time between two consecutive shots (for each fire mode) */
	UPROPERTY(EditDefaultsOnly, Replicated, Category=Config)
	float TimeBetweenShots[NUM_FIRING_MODES];
	
	/** projectile spawned if fire mode is Projectile */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AShooterProjectile> ProjectileClass[NUM_FIRING_MODES];

	/** weapon instant hit / Beam config */
	UPROPERTY(EditDefaultsOnly, Category=Config)
	FInstantWeaponData InstantConfig[NUM_FIRING_MODES];
	
	/** how many shots/projectiles to spawn every time HandleFiring() is called (consumes extra ammo) */
	UPROPERTY(EditAnywhere, Replicated, Category=Config)
	uint8 ShotsPerTick[NUM_FIRING_MODES];
	
	/** If fire mode is Charge, this is the max time the weapon can charge. Calls MaxChargeEvent if reached this limit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Charge)
	float MaxChargeTime;
	
	/** If fire mode is Charge, this is the frequency that it consumes ammo. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Charge)
	float ChargeAmmoTimer;
	
	/** Each time when weapon starts charging, add or subtract a random number between [0..ChargeRandomDisturbance] to/from MaxChargeTime. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Charge)
	float ChargeRandomDisturbance;
	
	/** The generated random disturbance for this charge */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Charge)
	float ChargeRandomAdd;

	/** random number generator stream */
	FRandomStream WeaponRandomStream;
	
	//////////////////////////////////////////////////////////////////////////
	// Appearance / sound
	/** weapon mesh: 1st person view */
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** weapon mesh: 3rd person view */
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh3P;

	/** firing audio component (bLoopedFireSound set) */
	UPROPERTY(Transient)
	UAudioComponent* FireAC;

	/** name of bone/socket for muzzle in weapon mesh. If >1, will use a random attach point each shot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Visuals)
	TArray<FName> MuzzleAttachPoint;

	/** spawned component for muzzle FX */
	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzlePSC;

	/** animation owner should play while holding this weapon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Animation)
	TEnumAsByte<EWeaponAnim::Type> CharacterAnim;

	/** weapon firing effects, specific for each fire mode */
	UPROPERTY(EditDefaultsOnly, Category=Visuals)
	FWeaponEffects Effects[NUM_FIRING_MODES];
	
	/** reload sound */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* ReloadSound;

	/** reload animations */
	UPROPERTY(EditDefaultsOnly, Category=Animation)
	FWeaponAnim ReloadAnim;

	/** out of ammo sound */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* OutOfAmmoSound;

	/** equip sound */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* EquipSound;

	/** equip animations */
	UPROPERTY(EditDefaultsOnly, Category=Animation)
	FWeaponAnim EquipAnim;
	
	/** param name for beam target in smoke trail */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Effects)
	FName TrailTargetParam;
	
	/** param name for beam source in smoke trail (only used if bounces > 0) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Effects)
	FName TrailSourceParam;
	
	/** spawn effects for impact */
	UFUNCTION(BlueprintCallable, Category=Effects)
	void SpawnImpactEffects(const FHitResult& Impact);

	/** spawn trail effect
	 *  @param SpawnTrailAttached Whether to spawn trail attached to weapon's muzzle, or use StartPoint */
	UParticleSystemComponent* SpawnTrailEffect(const FVector& StartPoint, const FVector& EndPoint, bool SpawnTrailAttached);

	//////////////////////////////////////////////////////////////////////////
	// Control variables
	
	/** is weapon currently equipped? */
	uint32 bIsEquipped : 1;
	
	/** is fire animation playing? */
	uint32 bPlayingFireAnim : 1;
	
	/** is weapon fire active? */
	uint32 bWantsToFire : 1;

	/** [everyone] is reload animation playing? */
	UPROPERTY(BlueprintReadOnly, Transient, ReplicatedUsing=OnRep_Reload, Category=Ammo)
	uint32 bPendingReload : 1;

	/** is equip animation playing? */
	uint32 bPendingEquip : 1;

	/** weapon is refiring */
	uint32 bRefiring;

	/** current weapon state */
	EWeaponState::Type CurrentState;

	/** Firing mode for primary/secondary fire buttons */
	UPROPERTY(Replicated, EditDefaultsOnly, Category=Weapon)
	TEnumAsByte<enum EFireMode> FiringMode[NUM_FIRING_MODES];

	/** False: each fire mode has its own cooldown, true: cooldown is shared among all fire modes. */
	UPROPERTY(Replicated, EditDefaultsOnly, Category = Weapon)
	bool bIndependentFireModeCooldown;

	/** triggers a cooldown for current fire mode */
	void TriggerCooldown();

	/** returns true if weapon is on cooldown for FireMode (ie. not ready to fire) */
	bool IsOnCooldown(uint8 FireMode);

	/** time of last successful weapon fire -- used to determine HandleFiring() timer */
	float LastFireTime[NUM_FIRING_MODES];
	
	/** time between shots for the last shot fired -- used to determine HandleFiring() timer */
	float LastFireCooldown[NUM_FIRING_MODES];

	/** last time when this weapon was switched to */
	float EquipStartedTime;

	/** how much time weapon needs to be equipped */
	float EquipDuration;

	/** current Dispersion from continuous firing */
	UPROPERTY(Transient, Replicated)
	float CurrentFiringDispersion;
	
	/** game time when user started charging weapon (FM_Charge) */
	float TimeStartedCharging;
	
	//////////////////////////////////////////////////////////////////////////
	// Projectile

	/** [server or everyone] spawn projectile */
	void FireProjectile(uint8 RandomSeed);
	
	//////////////////////////////////////////////////////////////////////////
	// Input - server side

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartFire(uint8 FireMode);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStopFire(uint8 FireMode);

	//////////////////////////////////////////////////////////////////////////
	// Replication

	UFUNCTION()
	void OnRep_BurstCounter();

	/** Called in network play to do the cosmetic fx for firing */
	void SimulateWeaponFire();

	/** Called in network play to stop cosmetic fx (e.g. for a looping shot). */
	void StopSimulatingWeaponFire();
	
	/** instant hit notify for replication */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_HitNotify)
	uint8 HitNotifySeed;
	
	uint8 PreviousSeed;

	/** burst counter, used for replicating fire events to remote clients */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_BurstCounter)
	uint8 BurstCounter;
	
	/** whether the client should notify the server that he hit something with gameplay implications */
	bool ClientShouldNotifyHit(AActor* TestActor) const;

	/** server notified of instant hit from client to verify and deal damage */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerNotifyInstantHit(FHitResult Impact, uint8 RandomSeed, uint8 ShotIndex, uint8 BounceNumber);

	/** server notified of instant hit shot to show trail FX on server and other clients */
	UFUNCTION(Server, Unreliable, WithValidation)
	void ServerNotifyShot(uint8 RandomSeed);
	
	UFUNCTION()
	void OnRep_HitNotify();

	/** called in network play to do the cosmetic fx  */
	void SimulateInstantHit(uint8 RandomSeed);

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerStartReload();

	UFUNCTION(Reliable, Server, WithValidation)
	void ServerStopReload();

	UFUNCTION()
	void OnRep_Reload();

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local + server] weapon specific fire implementation */
	void FireWeapon();

	/** [local + server] handle weapon fire */
	void HandleFiring();

	FTimerHandle HandleFiringHandle;
	
	/** [local + server] firing started */
	virtual void OnBurstStarted();

	/** [local + server] firing finished */
	virtual void OnBurstFinished();

	/** update weapon state */
	void SetWeaponState(EWeaponState::Type NewState);

	/** determine current weapon state */
	void DetermineWeaponState();
	
	/** Returns true if owner's pawn is locally controlled */
	bool IsLocallyControlled() const;
	
	//////////////////////////////////////////////////////////////////////////
	// Instant hit and Beam modes

	/** process the instant hit and notify the server if necessary */
	void ProcessInstantHit(uint8 RandomSeed);

	/** check if weapon should deal damage to actor */
	bool ShouldDealDamage(AActor* TestActor) const;

	/** handle damage */
	void DealDamage(const FHitResult& Impact, const FVector& ShootDir, uint8 BounceNumber);
	
	/** find hit. If TraceFrom == Zero or TraceTo == Zero, finds these values using GetAdjustedAim and TraceDist. */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	FHitResult WeaponTrace(FVector TraceFrom = FVector::ZeroVector, FVector TraceTo = FVector::ZeroVector, AActor* IgnoreActor = NULL, float TraceDist = 100000.0f) const;

	/** find hit, using a multi line trace. If TraceFrom == Zero or TraceTo == Zero, finds these values using GetAdjustedAim and TraceDist. */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	TArray<FHitResult> WeaponTraceMulti(FVector TraceFrom = FVector::ZeroVector, FVector TraceTo = FVector::ZeroVector, AActor* IgnoreActor = NULL, float TraceDist = 100000.0f) const;

	/** update Beam trail FX */
	void UpdateBeam();

	/** Beam particle component, for each bounce */
	TArray<UParticleSystemComponent*> BeamPSC;

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage helpers

	/** play weapon sounds */
	UAudioComponent* PlayWeaponSound(USoundCue* Sound);

	/** play weapon animations */
	float PlayWeaponAnimation(const FWeaponAnim& Animation);

	/** stop playing weapon animations */
	void StopWeaponAnimation(const FWeaponAnim& Animation);

	/** Get the aim of the weapon, allowing for adjustments to be made by the weapon, with OutStartTrace starting at weapon's muzzle if not blocked. */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	void GetAdjustedAim(FVector& OutAimDir, FVector& OutStartTrace) const;

	/** Get the aim of the camera (camera facing direction) */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	FVector GetCameraAim() const;

	/** get the originating location for camera damage (middle of the screen, in first person) */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	FVector GetCameraDamageStartLocation(const FVector& AimDir) const;

	/** get muzzle location of the weapon, depending on MuzzleChooseMethod and CurrentMuzzleIndex. */
	UFUNCTION(BlueprintCallable, Category = Weapon)
	FVector GetMuzzleLocation() const;
	
	/** get a muzzle socket name of the weapon */
	FName GetMuzzleName() const;

	void IncrementMuzzleIndex();

	inline AShooterGameState* GetGameState() const { return GetWorld()->GetGameState<AShooterGameState>(); }

	//////////////////////////////////////////////////////////////////////////
	// FM_Charge mode

	/** [everyone] calls the blueprint event and sets up the HandleCharging() timer */
	void StartCharging(uint8 RandomSeed);

	/** [everyone] calls the blueprint event and stops the HandleCharging() timer */
	void StopCharging();

	/** repnotify used to replicate Charging status to other clients */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_ChargingNotify)
	FChargingInfo ChargingNotify;
	
	/** [everyone] handle weapon charging */
	void HandleCharging();
	
	FTimerHandle HandleChargingHandle;
	UFUNCTION()
	void OnRep_ChargingNotify();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartCharging(uint8 RandomSeed);
	
};

FORCEINLINE bool AShooterWeapon::IsCharging() const
{
	return ChargingNotify.bIsCharging;
}

FORCEINLINE int32 AShooterWeapon::GetAmmoPerClip() const
{
	return WeaponConfig.AmmoPerClip;
}

FORCEINLINE int32 AShooterWeapon::GetCurrentAmmoInClip() const
{
	return CurrentAmmoInClip;
}

FORCEINLINE class AController* AShooterWeapon::GetPawnOwnerController() const
{
	return MyPawn ? MyPawn->GetController() : NULL;
}

FORCEINLINE TSubclassOf<UShooterDamageType> AShooterWeapon::GetInstantHitDamageType(uint8 FireMode) const
{
	check(FireMode < NUM_FIRING_MODES);
	return InstantConfig[FireMode].DamageType;
}

FORCEINLINE float AShooterWeapon::GetBaseFiringDispersion() const
{
	return WeaponConfig.BaseFiringDispersion;
}