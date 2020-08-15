// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once
#include "ShooterPersistentUser.h"
#include "ShooterTypes.h"
#include "ShooterCharacterMovement.h"
#include "ShooterCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBindableEvent_CharacterFired, AShooterWeapon*, Weapon, uint8, FireMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBindableEvent_EquippedWeapon, AShooterWeapon*, WeaponEquipped);

UCLASS(Abstract)
class AShooterCharacter : public ACharacter
{
	GENERATED_UCLASS_BODY()

public:

	/** setup initial variables, spawn inventory */
	virtual void BeginPlay() override;

	/** Update the character. (Running, health etc). */
	virtual void Tick(float DeltaSeconds) override;

	/** cleanup inventory */
	virtual void Destroyed() override;

	/** update mesh for first person view */
	virtual void PawnClientRestart() override;

	/** [server] update pawn meshes after possession */
	virtual void PossessedBy(class AController* C) override;
	/** [server] update pawn meshes after unpossession */
	virtual void UnPossessed() override;

	/** [client] perform PlayerState related setup */
	virtual void OnRep_PlayerState() override;
	
	/** called after a valid PlayerState is updated */
	UFUNCTION(BlueprintImplementableEvent, Category=Pawn)
	void OnPlayerStateReplicated(class AShooterPlayerState* NewPlayerState);
	
	/** called after this character was possessed by a controller */
	UFUNCTION(BlueprintImplementableEvent, Category=Pawn)
	void OnPawnPossessed(class AController* NewController);

	/** 
	 * Called after the camera updates.
	 *
	 *	@param	CameraLocation	Location of the Camera.
	 *	@param	CameraRotation	Rotation of the Camera.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category=Camera)
	void OnCameraUpdate(const FVector& CameraLocation, const FRotator& CameraRotation);

	/** overriding to allow bots to aim up/down */
	virtual void FaceRotation(FRotator NewRotation, float DeltaTime) override;

	/** get aim offsets */
	UFUNCTION(BlueprintCallable, Category=Aiming)
	FRotator GetAimOffsets() const;
	
	/** get character's aiming dispersion. Note: it returns only the character's aiming dispersion,
	*	without adding the weapon's firing dispersion. */
	UFUNCTION(BlueprintCallable, Category=Aiming)
	float GetAimingDispersion() const;
	
	/** defines whether aiming dispersion should be updated.
	*	Primarily intended for setting bots accuracy (for different difficulty levels). */
	UPROPERTY(EditDefaultsOnly, Category = Aiming)
	bool bUpdateAimingDispersion;
	
	/** Minimal aiming dispersion (degrees, lower = best accuracy, 0.0 = totally precise) */
	UPROPERTY(EditDefaultsOnly, Category = Aiming, Meta=(EditCondition = "bUpdateAimingDispersion", ClampMin="0.0", ClampMax="90.0", UIMin = "0.0", UIMax = "90.0"))
	float MinAimingDispersion;
	
	/** Maximum aiming dispersion (degrees, high values = worst accuracy) */
	UPROPERTY(EditDefaultsOnly, Category = Aiming, Meta=(EditCondition = "bUpdateAimingDispersion", ClampMin="0.0", ClampMax="90.0", UIMin = "0.0", UIMax = "90.0"))
	float MaxAimingDispersion;
	
	/** Aiming dispersion decrement per second */
	UPROPERTY(EditDefaultsOnly, Category = Aiming, Meta=(EditCondition = "bUpdateAimingDispersion"))
	float AimingDispersionDecrement;

	/** Aiming dispersion increment, modified by the character's movement speed and turn rate */
	UPROPERTY(EditDefaultsOnly, Category = Aiming, Meta=(EditCondition = "bUpdateAimingDispersion"))
	float AimingDispersionIncrement;
	
	/** call this when pawn moves or rotates to update aiming dispersion. Only does anything if bUpdateAimingDispersion,
	*	so no need to check it before calling. */
	void IncreaseAimingDispersion(float Factor);

	/** 
	 * Check if pawn is enemy if given controller.
	 *
	 * @param	TestPC	Controller to check against.
	 */
	UFUNCTION(BlueprintCallable, Category="Game|Pawn")
	bool IsEnemyFor(AController* TestPC) const;

	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** [server] add weapon to inventory. If AmmoAmount == -1, also gives the weapon's initial ammo amount */
	void AddWeapon(class AShooterWeapon* Weapon, int32 AmmoAmount = -1);
	
	/** [server] add item to inventory. */
	void AddItem(class AShooterItem* Item);

	UFUNCTION(Client, Reliable, WithValidation)
	void ClientWeaponAdded(TSubclassOf<class AShooterWeapon> WeaponClass);

	/** [client + server] called when a weapon was added to inventory. */
	UFUNCTION(BlueprintImplementableEvent, Category=Item)
	void OnWeaponAdded(TSubclassOf<class AShooterWeapon> WeaponClass);

	/** [server] remove item from inventory */
	void RemoveItem(class AShooterItem* Item);
	
	/** [server] remove all items from inventory */
	void ClearInventory();

	/** 
	 * Find in inventory
	 * @param WeaponClass	Class of weapon to find.
	 */
	UFUNCTION(BlueprintCallable, Category=Inventory)
	class AShooterWeapon* FindWeapon(TSubclassOf<class AShooterWeapon> WeaponClass) const;
	
	/** Notify sent from the weapon that it has no more ammo to shoot */
	void NotifyOutOfAmmo();

	/** Returns whether the player wants to automatically change weapon when the weapon he was using has ran out of ammo. */
	bool SwitchWeaponIfNoAmmo();

	/** Returns the first powerup found in this character's inventory, NULL if none */
	UFUNCTION(BlueprintCallable, Category=Inventory)
	class AShooterItem_Powerup* GetFirstPowerup();
	
	/** Whether a powerup is currently in effect. Currently, used for HUD only. */
	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	bool AnyPowerupActive;

	/** 
	 * [server + local] equips weapon from inventory 
	 *
	 * @param Weapon	Weapon to equip
	 */
	void EquipWeapon(class AShooterWeapon* Weapon);

	/** [server] add weapon ammo to inventory
	*	@return amount of ammo added. */
	int32 GiveAmmo(TSubclassOf<AShooterWeapon> WeaponClass, int32 Amount);
	
	/** can pickup ammo (not in max ammo limit)? */
	bool CanPickupAmmo(TSubclassOf<AShooterWeapon> WeaponClass);

	/** whether pawn can pickup pickups */
	UPROPERTY(EditDefaultsOnly, Category=Inventory)
	bool CanPickupPickups;

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local] starts weapon fire */
	void StartWeaponFire(uint8 FireMode);

	/** [local] stops weapon fire for given fire mode */
	void StopWeaponFire(uint8 FireMode);

	/** [local] stops weapon fire for all fire modes */
	void StopAllWeaponFire();

	/** check if pawn can fire weapon */
	bool CanFire() const;

	//////////////////////////////////////////////////////////////////////////
	// Movement

	/** [server + local] change running state */
	void SetRunning(bool bNewRunning, bool bToggle);
		
	/** Called on landing after falling has completed, to perform actions based on the Hit result. Triggers the OnLanded event. */
	virtual void Landed(const FHitResult& Hit) override;

	//virtual void SetBase(UPrimitiveComponent* NewBase, bool bNotifyActor = true) override;

	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* JumpSound;
	
	//////////////////////////////////////////////////////////////////////////
	// Animations
	
	/** play anim montage */
	virtual float PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate = 1.f, FName StartSectionName = NAME_None) override;

	/** stop playing montage */
	virtual void StopAnimMontage(class UAnimMontage* AnimMontage) override;

	/** stop playing all montages */
	void StopAllAnimMontages();

	//////////////////////////////////////////////////////////////////////////
	// Input handlers

	/** setup pawn specific input handlers */
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	/** 
	 * Move forward/back
	 *
	 * @param Val Movment input to apply
	 */
	void MoveForward(float Val);

	/** 
	 * Strafe right/left 
	 *
	 * @param Val Movment input to apply
	 */	
	void MoveRight(float Val);

	/** 
	 * Move Up/Down in allowed movement modes. 
	 *
	 * @param Val Movment input to apply
	 */	
	void MoveUp(float Val);

	/* Frame rate independent turn */
	void TurnAtRate(float Val);

	/* Frame rate independent lookup */
	void LookUpAtRate(float Val);
	/*
	virtual void AddControllerYawInput(float Val) override;

	virtual void AddControllerPitchInput(float Val) override;
	*/
	void OnStartFirePrimary();

	/** player pressed start fire action */
	void OnStartFireSecondary();
	
	/** player released start fire action */
	void OnStopFirePrimary();

	/** player released start fire action */
	void OnStopFireSecondary();

	/** player pressed reload action */
	void OnReload();

	/** player pressed next weapon action */
	void OnNextWeapon();

	/** player pressed prev weapon action */
	void OnPrevWeapon();

	/** player pressed jump action */
	void OnStartJump();

	/** player released jump action */
	void OnStopJump();
	
	/** player pressed crouch action */
	void OnStartCrouchInput();

	/** player released crouch action */
	void OnStopCrouchInput();

	/** player pressed run action */
	void OnStartRunning();

	/** player pressed toggled run action */
	void OnStartRunningToggle();

	/** player released run action */
	void OnStopRunning();
	
	void OnDodgeForward();
	
	void OnDodgeBackward();
	
	void OnDodgeLeft();
	
	void OnDodgeRight();
	
	void OnUse();
	
	AShooterWeapon* SwitchToWeaponCategory(uint8 Category);
	
	void SwitchToWeaponCategory1();
	
	void SwitchToWeaponCategory2();

	void SwitchToWeaponCategory3();

	void SwitchToWeaponCategory4();

	void SwitchToWeaponCategory5();

	void SwitchToWeaponCategory6();
	
	/** returns the best weapon (as defined by WeaponPriority) that has ammo and
	  * has AIWeaponRange >= AIMinWeaponRangeSquared */
	UFUNCTION()
	AShooterWeapon* GetBestWeapon(float AIMinWeaponRangeSquared = 0.f);

	/** Best weapons first */
	UPROPERTY(EditDefaultsOnly, Category=Inventory)
	TArray<TSubclassOf<class AShooterWeapon> > WeaponPriority;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestDodge(uint8 DodgeDirection);

	//////////////////////////////////////////////////////////////////////////
	// Reading data

	/** get "Mesh1P" component */
	UFUNCTION(BlueprintCallable, Category=Pawn)
	USkeletalMeshComponent* GetPawnMesh1P() const;
	
	/** get "GetMesh()" component (the one from APawn) */
	UFUNCTION(BlueprintCallable, Category=Pawn)
	USkeletalMeshComponent* GetPawnMesh3P() const;
	
	/** get currently equipped weapon */
	UFUNCTION(BlueprintPure, Category="Inventory")
	class AShooterWeapon* GetWeapon() const;

	/** returns all AShooterWeapons in Inventory */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<class AShooterWeapon*> GetAllWeapons() const;

	/** get weapon attach point */
	FName GetWeaponAttachPoint() const;

	/** get total number of inventory items */
	int32 GetInventoryCount() const;
	
	/** get current ammo for weapon. If GetInventoryAmmo = true, returns ammo amount in inventory, rather than current ammo in clip (if applicable). */
	UFUNCTION(BlueprintPure, Category="Inventory")
    int32 GetCurrentAmmo(TSubclassOf<AShooterWeapon> WeaponClass, bool GetInventoryAmmo = false) const;
	
	/** consume ammo for weapon (only reduces ammo in the AShooterItem_Ammo item, doesn't changes Weapon.CurrentAmmoInClip) */
    void UseAmmo(TSubclassOf<AShooterWeapon> WeaponClass, int32 AmmoToConsume);

	/** 
	 * get weapon from inventory at index. Index validity is not checked.
	 *
	 * @param Index Inventory index
	 */
	class AShooterWeapon* GetInventoryWeapon(int32 index) const;

	/** get firing state */
	UFUNCTION(BlueprintCallable, Category="Game|Weapon")
	bool IsFiring() const;

	/** get running state */
	UFUNCTION(BlueprintCallable, Category=Pawn)
	bool IsRunning() const;
	
	/** Called when character fires his weapon. */
	UPROPERTY(BlueprintAssignable, Category=Pickup)
	FBindableEvent_CharacterFired OnCharacterFired;
	
	/** Called when character equips a weapon. */
	UPROPERTY(BlueprintAssignable, Category=Pickup)
	FBindableEvent_EquippedWeapon OnWeaponEquipped;
	
	/** called when pawn dies (at the end of OnDeath()) */
	UFUNCTION(BlueprintImplementableEvent, Category=Health)
	void PawnDiedEvent(APawn* PawnInstigator, AActor* DamageCauser);
	
	/** called when pawn restarts (respawns). Called on owner only. */
	UFUNCTION(BlueprintImplementableEvent, Category=Pawn)
	void PawnRestartedEvent();
	
	/** called after pawn becomes a ragdoll */
	UFUNCTION(BlueprintImplementableEvent, Category=Health, Meta=(Keywords="died"))
	void RagdollEvent(AShooterCharacter* Killer, FHitResult HitInfo, FVector_NetQuantizeNormal ShotDirection);
	
	/** called when player leaves / disconnects from the game */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category=Game, Meta=(Keywords="disconnected"))
	void PlayerLeftGame();
	
	/** get camera view type */
	UFUNCTION(BlueprintCallable, Category=Mesh)
	virtual bool IsFirstPerson() const;
	

	//////////////////////////////////////////////////////////////////////////
	// Spells

	/** Triggers a spell cooldown, and spend all SpellCharge if bUseOvercast == true and SpellCharge == 1.0.
	*	Make sure to call this both on server and on local client. */
	UFUNCTION(BlueprintCallable, Category=Spells)
	void CastSpell(float CooldownTime, bool bUseOvercast);

	/** Adds spell charge, it goes from 0.0 to 1.0. 
	*	Make sure to call this both on server and on local client. */
	UFUNCTION(BlueprintCallable, Category=Spells)
	void AddSpellCharge(float Amount);
	
	/** returns true if no spell is on cooldown */
	UFUNCTION(BlueprintPure, Category=Spells)
	bool CanCastSpell() const;

	/** returns true if no spell is on cooldown and SpellCharge == 1.0 */
	UFUNCTION(BlueprintPure, Category = Spells)
	bool CanCastOvercastSpell() const;

	//////////////////////////////////////////////////////////////////////////
	// Health and armor

	/** get max health (non-boosted) */
	UFUNCTION(BlueprintPure, Category=Health)
	float GetMaxHealth() const;

	/** get max health (boosted) */
	UFUNCTION(BlueprintPure, Category = Health)
	float GetMaxBoostedHealth() const;

	/** get current health */
	UFUNCTION(BlueprintPure, Category = Health)
	float GetHealth() const;

	/** Adds health, limited to MaxHealth or MaxHealthBoosted, and returns how much health was added */
	UFUNCTION(BlueprintCallable, Category = Health)
	float GiveHealth(float Amount, bool bIsHealthBoost);

	/** check if pawn is still alive */
	UFUNCTION(BlueprintPure, Category=Health)
	bool IsAlive() const;

	/** get current armor */
	UFUNCTION(BlueprintPure, Category = Health)
	float GetArmor() const;

	/** returns percentage of health when low health effects should start */
	float GetLowHealthPercentage() const;

	/** Gives armor amount to pawn, up to MaxArmor.
	* @return the amount of armor that was added. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Health)
	float GiveArmor(float Amount);

	float GetMaxArmor() const;

	/** Adds shield, limited to MaxShield */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Health)
	void GiveShield(float Amount);

	/** event called when this character hits another character. If overriding, don't forget to call parent function. */
	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = Pawn)
	void OnHitAnotherCharacter(float DamageDealt, AShooterCharacter* HitCharacter);

	//////////////////////////////////////////////////////////////////////////
	// Misc

	UFUNCTION(BlueprintCallable, Category = Movement)
	UShooterCharacterMovement* GetShooterCharacterMovement() const;

	/** Modifier for all damage dealt by this character */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Replicated, Category=Pawn)
	float DamageScale;
	
	/** Head bone name(s) for this character. Used by weapons to determine if hit is a headshot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Pawn)
	TArray<FName> HeadBoneNames;
	
	inline struct FTakeHitInfo GetLastHitInfo() const { return LastTakeHitInfo; }

	/** if false, this pawn will never respawn after dying. Only checked for AI agents. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Pawn)
	bool bShouldRespawn;
	
	/** Performs a trace in the direction the character is looking */
	UFUNCTION(BlueprintCallable, Category=Collision)
	FHitResult ForwardTrace(float TraceDist, ECollisionChannel TraceChannel) const;
	
	UFUNCTION(BlueprintCallable, Category=Pawn)
	virtual void GetPlayerViewPointBP( FVector& Location, FRotator& Rotation ) const;

	//////////////////////////////////////////////////////////////////////////
	// AI	
	UPROPERTY(EditAnywhere, Category=Behavior)
	class UBehaviorTree* BotBehavior;

	//virtual void FaceRotation(FRotator NewRotation, float DeltaTime = 0.f) override;

	/** Update the color of all player meshes. */
	void UpdatePlayerColorsAllMIDs();

	void CreateMeshMIDs();

protected:

	FTimerHandle UpdatePlayerColorsAllMIDsHandle;

	/** spawns a Weapon pickup upon character death */
	void DropWeapon();

	UFUNCTION(Unreliable, WithValidation, NetMulticast)
	void PlaySoundReplicated(USoundCue* Sound);

	/** pawn mesh: 1st person view */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** pointer to all mesh components in this character */
	UPROPERTY()
	TArray<USkeletalMeshComponent*> AllMeshes;

	/** socket or bone name for attaching weapon mesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Inventory)
	FName WeaponAttachPoint;

	/** weapons given at game start */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Inventory)
	TArray<TSubclassOf<class AShooterWeapon> > StartingWeapons;

	/** weapons in inventory */
	UPROPERTY(Transient, Replicated)
	TArray<class AShooterItem*> Inventory;
	
	/** meant to be used for a character class (e.g. monsters), rather than a cheat */
	UPROPERTY(EditDefaultsOnly, Category = Inventory)
	bool bHasInfiniteAmmo;
	
	/** if true this pawn will never drop its inventory when it dies, even if the game mode requests it. */
	UPROPERTY(EditDefaultsOnly, Category = Inventory)
	bool bNeverDropInventory;

	/** currently equipped weapon */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_CurrentWeapon)
	class AShooterWeapon* CurrentWeapon;

	/** Replicate where this pawn was last hit and damaged */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_LastTakeHitInfo)
	struct FTakeHitInfo LastTakeHitInfo;

	/** current aiming dispersion (inaccuracy) */
	float CurrentAimingDispersion;

	/** Time at which point the last take hit info for the actor times out and won't be replicated; Used to stop join-in-progress effects all over the screen */
	float LastTakeHitTimeTimeout;

	/** current running state */
	UPROPERTY(Transient, Replicated)
	uint8 bWantsToRun : 1;

	/** from gamepad running is toggled */
	uint8 bWantsToRunToggled : 1;

	/** when low health effects should start */
	float LowHealthPercentage;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	float BaseTurnRate;

	/** Base lookup rate, in deg/sec. Other scaling may affect final lookup rate. */
	float BaseLookUpRate;

	/** material instances for setting parameters in all meshes */
	UPROPERTY(Transient, BlueprintReadOnly, Category=Pawn)
	TArray<UMaterialInstanceDynamic*> MeshMIDs;

	/** sets material instance parameter on all meshes (1st and 3rd person). */
	UFUNCTION(BlueprintCallable, Category = Pawn, Meta=(Keywords="color material instance"))
	void SetAllMeshesVectorParameter(FName ParameterName, FLinearColor NewColor);

	/** animation played on death */
	UPROPERTY(EditDefaultsOnly, Category=Animation)
	UAnimMontage* DeathAnim;

	/** sound played on death, local player only */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* DeathSound;
	
	/** effect played on respawn */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	UParticleSystem* RespawnFX;

	/** sound played on respawn */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* RespawnSound;

	/** sound played when health is low */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* LowHealthSound;

	/** sound played when running */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* RunLoopSound;

	/** sound played when stop running */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* RunStopSound;

	/** used to manipulate with run loop sound */
	UPROPERTY()
	UAudioComponent* RunLoopAC;

	/** hook to looped low health sound used to stop/adjust volume */
	UPROPERTY()
	UAudioComponent* LowHealthWarningPlayer;

	/** handles sounds for running */
	void UpdateRunSounds(bool bNewRunning);

	/** handle mesh visibility and updates */
	void UpdatePawnMeshes();
	
	/** BP hook for the UpdatePawnMeshes() function */
	UFUNCTION(BlueprintImplementableEvent, Category=Pawn)
	void UpdatePawnMeshes_BP();

	/** handle mesh colors on specified material instance */
	void UpdatePlayerColors(UMaterialInstanceDynamic* UseMID);

	/** Responsible for cleaning up bodies on clients. */
	virtual void TornOff();
	
	/** whether last damage dealt should gib this character */
	bool ShouldGib(float DamageTaken, struct FDamageEvent const& DamageEvent) const;
	
	/** splashes the character in a rain of blood and gore */
	void Gib(struct FDamageEvent const& DamageEvent);

	/** Z velocity that character starts taking damage, when he falls. */
	UPROPERTY(EditDefaultsOnly, Category = Jumping)
	float MinLandedDamageVelocity;

	/** Z velocity that character takes 100 points of damage, when he falls. */
	UPROPERTY(EditDefaultsOnly, Category = Jumping)
	float MaxLandedDamageVelocity;

	/** force to apply when dodging */
	UPROPERTY(EditDefaultsOnly, Category = Jumping)
	float DodgeMomentum;
	
	/** how high dodging takes you, [0..1] */
	UPROPERTY(EditDefaultsOnly, Category = Jumping)
	float DodgeZ;
	
	/** how high wall dodging takes you, [0..1] */
	UPROPERTY(EditDefaultsOnly, Category = Jumping)
	float WallDodgeZ;
	
	/** min interval between dodges */
	UPROPERTY(EditDefaultsOnly, Category = Jumping)
	float MinDodgeInterval;
	
	/** min interval between wall dodges */
	UPROPERTY(EditDefaultsOnly, Category = Jumping)
	float MinWallDodgeInterval;
	
	/** force to apply when wall dodging  */
	UPROPERTY(EditDefaultsOnly, Category = Jumping)
	float WallDodgeMomentum;
	
	float LastDodgeTime;

	float LastWallDodgeTime;
	
	/** Overriding to allow ragdolls to take damage */
	virtual bool ShouldTakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const override;

	virtual class AController* GetDamageInstigator(class AController* InstigatedBy, const class UDamageType& DamageType) const override;

	/** time when this character last took damage */
	float LastHitTime;

protected:
	
	//////////////////////////////////////////////////////////////////////////
	// Damage & death

	/** Identifies if pawn is in its dying state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Health)
	uint32 bIsDying:1;

	/** Current health of the Pawn. Its default is also used as Max health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category=Health)
	float Health;

	/** current armor amount */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = Health)
	float Armor;

	/** Max boosted health (via health vials, mega health powerup, etc). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Health)
	float MaxBoostedHealth;

	/** maximum armor amount */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Health)
	float MaxArmor;

	//////////////////////////////////////////////////////////////////////////
	// Spells

	/** Current spell charge of the character, goes from 0.0 to 1.0. When at 1.0, the character can perform an Overcast spell. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category=Spells)
	float SpellCharge;

	/** SpellCharge starts to decay after this many seconds. */
	UPROPERTY(EditDefaultsOnly, Category = Spells)
	float SpellChargeDecaysAfter;

	/** SpellCharge decays at this rate per second, after DecayTime was reached. */
	UPROPERTY(EditDefaultsOnly, Category = Spells)
	float SpellChargeDecayRate;

	/** when Charge was last gained */
	float LastSpellChargeGainTime;

	/** how many seconds is the cooldown of the last spell cast */
	float LastSpellCooldownTime;

	/** current spell cooldown */
	UPROPERTY(BlueprintReadOnly, Category = Spells)
	float CurrentSpellCooldownTime;

	/** current spell cooldown, normalized from 0 to 1 according to last spell's cooldown time */
	UPROPERTY(BlueprintReadOnly, Category = Spells)
	float CurrentSpellCooldownTimeNormalized;

	//////////////////////////////////////////////////////////////////////////
	// Shield

	/** Current shield level. Shield absorbs 100% of damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = Health)
	float Shield;

	/** Max shield level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Health)
	float MaxShield;

	/** How much Shield is gained when dealing damage, if distance to damaged pawn is <= ShieldGainDistanceMin. 
	*	Between ShieldGainDistanceMin and ShieldGainDistanceMax it is interpolated. Beyond ShieldGainDistanceMax it is 0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Health)
	float ShieldGainPerDamagePoint;
	
	/** If distance to damaged pawn is <= to this much, gains max amount of shield, according to ShieldGainPerDamagePoint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Health)
	float ShieldGainDistanceMin;
	float ShieldGainDistanceMinSquared;

	/** If distance to damaged pawn is > to ShieldGainDistanceMin, gains an interpolated amount of shield up to ShieldGainDistanceMax.
	*	Beyond ShieldGainDistanceMax, no shield is gained. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Health)
	float ShieldGainDistanceMax;	
	float ShieldGainDistanceMaxSquared;

	/** After gaining any amount of shield, how long (seconds) it takes to start decaying. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Health)
	float ShieldDecaysAfter;
	
	/** Game time when shield was last gained. */
	float LastShieldGainTime;

	/** Shield decay rate per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Health)
	float ShieldDecayRate;

public:

	/** Take damage, handle death */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	/** Pawn suicide */
	virtual void Suicide();

	/** Kill this pawn */
	virtual void KilledBy(class APawn* EventInstigator);

	/** Returns True if the pawn can die in the current state */
	virtual bool CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const;

	/**
	 * Kills pawn.  Server/authority only.
	 * @param KillingDamage - Damage amount of the killing blow
	 * @param DamageEvent - Damage event of the killing blow
	 * @param Killer - Who killed this pawn
	 * @param DamageCauser - the Actor that directly caused the damage (i.e. the Projectile that exploded, the Weapon that fired, etc)
	 * @returns true if allowed
	 */
	virtual bool Die(float KillingDamage, struct FDamageEvent const& DamageEvent, class AController* Killer, class AActor* DamageCauser);

	// Die when we fall out of the world.
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	/** Called on the actor right before replication occurs */
	virtual void PreReplication( IRepChangedPropertyTracker & ChangedPropertyTracker ) override;
	
	UFUNCTION(BlueprintCallable, Category=Physics)
	void AddMomentum( FVector Momentum, bool bMassIndependent );

	/** attempts to retrieve the ShooterPersistentUser. */
	UShooterPersistentUser* GetPersistentUser() const;

protected:
	/** notification when killed, for both the server and client. */
	virtual void OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, class APawn* InstigatingPawn, class AController* Killer, class AActor* DamageCauser);

	/** play effects on hit */
	virtual void PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser);

	/** switch to ragdoll */
	void SetRagdollPhysics();

	FTimerHandle SetRagdollPhysicsHandle;

	/** sets up the replication for taking a hit */
	void ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* InstigatingPawn, class AActor* DamageCauser, bool bKilled);

	/** play hit or death on client */
	UFUNCTION()
	void OnRep_LastTakeHitInfo();

	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** updates current weapon */
	void SetCurrentWeapon(class AShooterWeapon* NewWeapon, class AShooterWeapon* LastWeapon = NULL);

	/** current weapon rep handler */
	UFUNCTION()
	void OnRep_CurrentWeapon(class AShooterWeapon* LastWeapon);

	/** [server] spawns default inventory */
	void SpawnDefaultInventory();

	/** [server] remove all weapons from inventory and destroy them */
	void DestroyInventory();

	/** equip weapon */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEquipWeapon(class AShooterWeapon* NewWeapon);

	/** update running state */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetRunning(bool bNewRunning, bool bToggle);

	FTimerHandle DestroyInventoryHandle;

};

FORCEINLINE float AShooterCharacter::GetMaxArmor() const
{
	return MaxArmor;
}
