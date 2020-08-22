// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved.

#pragma once

#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "GameFramework/DamageType.h"
#include "ShooterTypes.generated.h"

/** when you modify this, please note that this information can be saved with instances
 * also DefaultEngine.ini [/Script/Engine.CollisionProfile] should match with this list **/
#define COLLISION_WEAPON		ECC_GameTraceChannel1
#define COLLISION_PROJECTILE	ECC_GameTraceChannel2
#define COLLISION_PICKUP		ECC_GameTraceChannel3

#define PHYS_SURFACE_GLASS		EPhysicalSurface::SurfaceType7

#define MAX_PLAYER_NAME_LENGTH 16


namespace EDodgeDirection
{
	enum Type
	{
		Forward=0,
		Backward=1,
		Left=2,
		Right=3,
	};
}

/** keep in sync with ShooterImpactEffect */
UENUM()
namespace EShooterPhysMaterialType
{
	enum Type
	{
		Unknown,
		Concrete,
		Dirt,
		Water,
		Metal,
		Wood,
		Grass,
		Glass,
		Flesh,
	};
}

#define SHOOTER_SURFACE_Default		SurfaceType_Default
#define SHOOTER_SURFACE_Concrete	SurfaceType1
#define SHOOTER_SURFACE_Dirt		SurfaceType2
#define SHOOTER_SURFACE_Water		SurfaceType3
#define SHOOTER_SURFACE_Metal		SurfaceType4
#define SHOOTER_SURFACE_Wood		SurfaceType5
#define SHOOTER_SURFACE_Grass		SurfaceType6
#define SHOOTER_SURFACE_Glass		SurfaceType7
#define SHOOTER_SURFACE_Flesh		SurfaceType8

#define ECC_Weapon		ECC_GameTraceChannel1
#define ECC_Projectile	ECC_GameTraceChannel2
#define ECC_Pickup		ECC_GameTraceChannel3

/** Number of customizable player colors */
const int32 GNumPlayerColors = 7;


USTRUCT()
struct FDecalData
{
	GENERATED_USTRUCT_BODY()

	/** material */
	UPROPERTY(EditDefaultsOnly, Category=Decal)
	UMaterial* DecalMaterial;

	/** quad size (width & height) */
	UPROPERTY(EditDefaultsOnly, Category=Decal)
	float DecalSize;

	/** lifespan */
	UPROPERTY(EditDefaultsOnly, Category=Decal)
	float LifeSpan;

	/** defaults */
	FDecalData()
		: DecalSize(256.f)
		, LifeSpan(10.f)
	{
	}
};

USTRUCT(BlueprintType)
struct FMapInfo
{
	GENERATED_USTRUCT_BODY()
		
	/** friendly map name, to be displayed on the menu */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=MapInfo)
	FString MapName;

	/** map author */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=MapInfo)
	FString MapAuthor;

	/** path to map, to pass on the travel command */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=MapInfo)
	FString MapPath;

	/** map prefix (DM, CTF, etc.) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=MapInfo)
	FString MapPrefix;
	
	/** Minimum number of teams supported in this map, if it's for a team-based game mode. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=MapInfo)
	uint8 MinTeams;

	/** Maximum number of teams supported in this map, if it's for a team-based game mode. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=MapInfo)
	uint8 MaxTeams;

	/** Whether this map supports arena game modes (Deathmatch, Invasion, CTF, etc.). If true then it will appear on the menu's map list. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=MapInfo)
	bool bSupportsArenaGameModes;

	FMapInfo()
	{
		MinTeams = MaxTeams = 0;
		bSupportsArenaGameModes = true;
	}
	void ReadMapData(const FString& MapFullPath)
	{
		const FString CleanFilename = FPaths::GetBaseFilename(MapFullPath);
		MapPath = CleanFilename;
		const int32 DashIndex = CleanFilename.Find(TEXT("-"));
		MapPrefix = CleanFilename.LeftChop(CleanFilename.Len() - DashIndex);
		FString DataFilePath = FPaths::ProjectContentDir() + TEXT("Maps/info/") + CleanFilename + TEXT(".json");
		FArchive* SaveFile = IFileManager::Get().CreateFileReader(*DataFilePath);
		if (!SaveFile)
		{
			//couldn't find map data file, attempt to determine some things from file name
			MapAuthor = TEXT("Unknown");
			MapName = CleanFilename.RightChop(DashIndex + 1);
			return;
		}
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<> > JsonReader = TJsonReaderFactory<>::Create(SaveFile);

		if (FJsonSerializer::Deserialize(JsonReader, JsonObject) &&
			JsonObject.IsValid())
		{
			MapName = JsonObject->GetStringField("Title");
			MapAuthor = JsonObject->GetStringField("Author");
			MinTeams = JsonObject->GetNumberField("MinTeams");
			MaxTeams = JsonObject->GetNumberField("MaxTeams");
			bSupportsArenaGameModes = JsonObject->GetBoolField("bSupportsArenaGameModes");
		}
		delete SaveFile;
	}
};

USTRUCT(BlueprintType)
struct FGameModeInfo
{
	GENERATED_USTRUCT_BODY()

	/** friendly game mode name, to be displayed on the menu */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=GameModeInfo)
	FText GameModeName;

	/** game mode prefix (DM, TDM, etc.), will set game mode accordingly. Ensure it is the same both in here and in GameModeList (DefaultGame.ini). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=GameModeInfo)
	FString GameModePrefix;
	
	/** Class name for this game type (e.g. "ShooterGameMode_TeamDeathMatch") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=GameModeInfo)
	FString GameClassName;
	
	/** The minimum number of teams this game type supports. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=GameModeInfo)
	uint8 MinTeams;

	/** The maximum number of teams this game type supports. Maps can also override this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=GameModeInfo)
	uint8 MaxTeams;
	
	/** Whether this game mode can ignore number of teams restrictions specified per map, 
	*	e.g. if true, and the map says it supports 2-4 teams but the game mode supports 1-10 teams, then the UI will allow you to pick 1-10 teams. */
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category=GameMode)
	bool bIgnoreMapTeamCountRestriction;

	/** If true then this game mode requires maps whose filename start with this gamemode's short name. For example, if true then CTF game mode would require
	*	maps to be called CTF-Something. If false, any map (regardless of prefix) will be available for this game mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=GameModeInfo)
	bool bRequiresMapPrefix;
	
	/** Whether this game mode should be added to the main menu's list of game modes */
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category=GameMode)
	bool bAddToMenu;

	FGameModeInfo()
	{}
	
	FGameModeInfo(FText Name, FString Prefix)
	{
		GameModePrefix = Prefix;
		GameModeName = Name;
	}
};

USTRUCT(BlueprintType)
struct FServerDetails
{
	GENERATED_USTRUCT_BODY()
		
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=ServerEntry)
	FString ServerName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=ServerEntry)
	FString CurrentPlayers;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=ServerEntry)
	FString MaxPlayers;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=ServerEntry)
	FString GameType;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=ServerEntry)
	FString MapName;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=ServerEntry)
	FString Ping;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=ServerEntry)
	int32 SearchResultsIndex;

	FServerDetails() {}
};

/** replicated information on a hit we've taken */
USTRUCT()
struct FTakeHitInfo
{
	GENERATED_USTRUCT_BODY()

	/** The amount of damage actually applied */
	UPROPERTY()
	float ActualDamage;

	/** The damage type we were hit with. */
	UPROPERTY()
	UClass* DamageTypeClass;

	/** Who hit us */
	UPROPERTY()
	TWeakObjectPtr<class AShooterCharacter> PawnInstigator;

	/** What actually caused the damage (e.g. the weapon (if hitscan) or the projectile) */
	UPROPERTY()
	TWeakObjectPtr<class AActor> DamageCauser;

	/** Specifies which DamageEvent below describes the damage received. */
	UPROPERTY()
	int32 DamageEventClassID;

	/** Whether this was a kill */
	UPROPERTY()
	uint32 bKilled:1;

private:

	/** A rolling counter used to ensure the struct is dirty and will replicate. */
	UPROPERTY()
	uint8 EnsureReplicationByte;

	/** Describes general damage. */
	UPROPERTY()
	FDamageEvent GeneralDamageEvent;

	/** Describes point damage, if that is what was received. */
	UPROPERTY()
	FPointDamageEvent PointDamageEvent;

	/** Describes radial damage, if that is what was received. */
	UPROPERTY()
	FRadialDamageEvent RadialDamageEvent;

public:
	FTakeHitInfo()
		: ActualDamage(0)
		, DamageTypeClass(NULL)
		, PawnInstigator(NULL)
		, DamageCauser(NULL)
		, DamageEventClassID(0)
		, bKilled(false)
		, EnsureReplicationByte(0)
	{}

	FDamageEvent& GetDamageEvent()
	{
		switch (DamageEventClassID)
		{
		case FPointDamageEvent::ClassID:
			if (PointDamageEvent.DamageTypeClass == NULL)
			{
				PointDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
			}
			return PointDamageEvent;

		case FRadialDamageEvent::ClassID:
			if (RadialDamageEvent.DamageTypeClass == NULL)
			{
				RadialDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
			}
			return RadialDamageEvent;

		default:
			if (GeneralDamageEvent.DamageTypeClass == NULL)
			{
				GeneralDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
			}
			return GeneralDamageEvent;
		}
	}
	FPointDamageEvent	 GetPointDamageEvent()
	{
		if (DamageEventClassID == FPointDamageEvent::ClassID)
		{
			if (PointDamageEvent.DamageTypeClass == NULL)
			{
				PointDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
			}
			return PointDamageEvent;
		}
		return FPointDamageEvent();
	}

	void SetDamageEvent(const FDamageEvent& DamageEvent)
	{
		DamageEventClassID = DamageEvent.GetTypeID();
		switch (DamageEventClassID)
		{
		case FPointDamageEvent::ClassID:
			PointDamageEvent = *((FPointDamageEvent const*)(&DamageEvent));
			break;
		case FRadialDamageEvent::ClassID:
			RadialDamageEvent = *((FRadialDamageEvent const*)(&DamageEvent));
			break;
		default:
			GeneralDamageEvent = DamageEvent;
		}

		DamageTypeClass = DamageEvent.DamageTypeClass;
	}

	void EnsureReplication()
	{
		EnsureReplicationByte++;
	}
};