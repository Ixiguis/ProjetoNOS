// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#ifndef __SHOOTERGAME_H__
#define __SHOOTERGAME_H__

#include "Engine.h"
#include "ParticleDefinitions.h"
#include "SoundDefinitions.h"
#include "Net/UnrealNetwork.h"
//#include "ShooterGameClasses.h"
#include "ShooterGameMode.h"
#include "ShooterGameState.h"
#include "ShooterCharacter.h"
#include "ShooterCharacterMovement.h"
#include "ShooterPlayerController.h"
#include "ShooterGameClasses.h"
/*#include "ShooterLocalPlayer.h"
#include "ShooterBlueprintLibrary.h"
#include "ShooterWeapon.h"
#include "ShooterGameSession.h"
*/
//includes to extend UserWidget
/*
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "Runtime/UMG/Public/UMG.h"
#include "Runtime/UMG/Public/UMGStyle.h"
#include "Runtime/UMG/Public/Slate/SObjectWidget.h"
#include "Runtime/UMG/Public/IUMGModule.h"
#include "Runtime/UMG/Public/Blueprint/UserWidget.h"
*/
class UBehaviorTreeComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogShooter, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogShooterWeapon, Log, All);

/** when you modify this, please note that this information can be saved with instances
 * also DefaultEngine.ini [/Script/Engine.CollisionProfile] should match with this list **/
#define COLLISION_WEAPON		ECC_GameTraceChannel1
#define COLLISION_PROJECTILE	ECC_GameTraceChannel2
#define COLLISION_PICKUP		ECC_GameTraceChannel3

#define PHYS_SURFACE_GLASS		EPhysicalSurface::SurfaceType7

#define MAX_PLAYER_NAME_LENGTH 16


/** Set to 1 to pretend we're building for console even on a PC, for testing purposes */
#define SHOOTER_SIMULATE_CONSOLE_UI	0

#if PLATFORM_PS4 || PLATFORM_XBOXONE || SHOOTER_SIMULATE_CONSOLE_UI
	#define SHOOTER_CONSOLE_UI 1
#else
	#define SHOOTER_CONSOLE_UI 0
#endif

#endif	// __SHOOTERGAME_H__
