// Copyright 2013-2014 Rampaging Blue Whale Games. All Rights Reserved. 

#include "System/ShooterWorldSettings.h"
#include "UObject/ConstructorHelpers.h"
#include "Serialization/JsonSerializer.h"
#if WITH_EDITOR
#include "Editor.h"
#endif

AShooterWorldSettings::AShooterWorldSettings()
{
	static ConstructorHelpers::FClassFinder<UDamageType> DmgTypeEnvOb(TEXT("/Game/Blueprints/DamageTypes/DmgType_ShooterEnvironmental.DmgType_ShooterEnvironmental_C"));
	KillZDamageType = DmgTypeEnvOb.Class;

	bSupportsArenaGameModes = true;
	MinTeams = MaxTeams = 0;

#if WITH_EDITOR
	FEditorDelegates::PostSaveWorld.AddUObject(this, &AShooterWorldSettings::WriteMetaData);
#endif
}

#if WITH_EDITOR
void AShooterWorldSettings::WriteMetaData(uint32 SaveFlags, UWorld* World, bool bSuccess)
{
	//sometimes this is called for invalid (uninitialized) WorldSettings, so check if some properties are at their defaults before overwriting the txt file
	if (Author.IsEmpty() && Title.IsEmpty())
	{
		return;
	}
	FString Filename = Cast<UPackage>(World->GetOuter())->FileName.ToString();
	if (Filename.Find(TEXT("_")) != INDEX_NONE)
	{
		//do not write metadata on levels _gameplay, _lights, etc
		return;
	}
	Filename = Filename.Replace(TEXT("/Game/"), TEXT("")) + TEXT(".json");
	Filename = Filename.Replace(TEXT(".umap"), TEXT(""));
	Filename = Filename.Replace(TEXT("Maps/"), TEXT("Maps/info/"));
	FString FilePath = FPaths::ProjectContentDir() + Filename;
	FArchive* SaveFile = IFileManager::Get().CreateFileWriter(*FilePath);
	if (!SaveFile)
	{
		return;
	}
	TSharedRef<TJsonWriter<> > JsonWriter = TJsonWriterFactory<>::Create(SaveFile);

	JsonWriter->WriteObjectStart();

	JsonWriter->WriteValue("Title", Title);
	JsonWriter->WriteValue("Author", Author);
	JsonWriter->WriteValue("bSupportsArenaGameModes", bSupportsArenaGameModes);
	JsonWriter->WriteValue("MinTeams", MinTeams);
	JsonWriter->WriteValue("MaxTeams", MaxTeams);	

	JsonWriter->WriteObjectEnd();
	JsonWriter->Close();
	delete SaveFile;
}
#endif //WITH_EDITOR