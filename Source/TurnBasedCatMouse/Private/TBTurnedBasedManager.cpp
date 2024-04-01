// Fill out your copyright notice in the Description page of Project Settings.


#include "TBTurnedBasedManager.h"
#include "SquareMapGeneration/TBSquareMapGenerator.h"
#include "Mammals/TBMammalBase.h"
#include "Async/Async.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ATBTurnedBasedManager::ATBTurnedBasedManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	NumberOfCatsToSpawn = 3;
	NumberOfMiceToSpawn= 50;
	bIsRoundOngoing = false;
	bAutoStartNextRound = true;
	StartNextRoundTime = 1;
	CurrentRound = 0;

}


// Called when the game starts or when spawned
void ATBTurnedBasedManager::BeginPlay()
{
	Super::BeginPlay();

	StartTurnBasedGame();
}

// Called every frame
void ATBTurnedBasedManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATBTurnedBasedManager::StartTurnBasedGame()
{
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// spawn map generator
	SquareMapGeneratorRef = GetWorld()->SpawnActor<ATBSquareMapGenerator>(SquareMapGenClass, GetActorLocation(), FRotator::ZeroRotator, Params);

	// generate the map
	SquareMapGeneratorRef->GenerateSquareMap();

	// spawn mammals
	InitSpawnMammals();

	
	OnRoundFinished();

	
	// AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this]
	// {
	// 	// generate the map
	// 	SquareMapGeneratorRef->GenerateSquareMap();
	//
	// 	// after generation is finished, spawn the mammals
	// 	SpawnMammals();
	// });
}


ATBMammalBase* ATBTurnedBasedManager::SpawnMammal(TSubclassOf<ATBMammalBase> MammalClass, FTileInfo* TargetTile) const
{
	if(!MammalClass || !TargetTile->bIsEmptyTile) return nullptr;

	
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn the mammal at the tile's location
	ATBMammalBase* MammalRef = GetWorld()->SpawnActor<ATBMammalBase>(MammalClass, TargetTile->WordLocation, FRotator::ZeroRotator, Params);

	// Calculate mammal bounds and adjust its position to snap it to the tile
	MammalRef->SetActorLocation(TargetTile->WordLocation + FVector(0,0,SquareMapGeneratorRef->GetTileHalfExtents().Z));

	// update the tile
	TargetTile->bIsEmptyTile = false;
	TargetTile->MammalRef = MammalRef;
	//SquareMapGeneratorRef->UpdateTileAt(TargetTile, TargetTile.Pos2D.X, TargetTile.Pos2D.Y);

	// update mammal tile 
	MammalRef->SetCurrentTile(TargetTile);
	
	
	return MammalRef;
}

void ATBTurnedBasedManager::InitSpawnMammals()
{
	//spawn cats
	for(int i= 0 ; i < NumberOfCatsToSpawn; i++)
	{
		
		if(FTileInfo* TileResult = SquareMapGeneratorRef->GetRandomEmptyTile())
		{
			ATBMammalBase* SpawnedCat = SpawnMammal(CatClass, TileResult);
			SpawnedCat->OnStarved.AddDynamic(this, &ATBTurnedBasedManager::OnStarved);
			SpawnedCat->OnBred.AddDynamic(this, &ATBTurnedBasedManager::OnBred);
			SpawnedCat->MapGeneratorRef = SquareMapGeneratorRef;
			SpawnedCat->UpdateDebugWidget(SquareMapGeneratorRef->ShowDebug);
			Cats.Add(SpawnedCat);
		}
		else
		{
			break; // if cant find any empty tile exit the loop
		}
		
	}
	
	//spawn mice
	for(int i= 0 ; i < NumberOfMiceToSpawn; i++)
	{
		if(FTileInfo* TileResult = SquareMapGeneratorRef->GetRandomEmptyTile())
		{
			ATBMammalBase* SpawnedMouse = SpawnMammal(MouseClass, TileResult);
			SpawnedMouse->OnKilled.AddDynamic(this, &ATBTurnedBasedManager::OnKillRequested);
			SpawnedMouse->OnBred.AddDynamic(this, &ATBTurnedBasedManager::OnBred);
			SpawnedMouse->MapGeneratorRef = SquareMapGeneratorRef;
			SpawnedMouse->UpdateDebugWidget(SquareMapGeneratorRef->ShowDebug);
			Mice.Add(SpawnedMouse);
		}
		else
		{
			break; // if cant find any empty tile exit the loop
		}
	}

}

void ATBTurnedBasedManager::StartNextRound()
{
	if(bIsRoundOngoing) return;
	
	if(Cats.Num() <= 0)
	{
		OnCatsWin();
		return;
	}

	if(Mice.Num() <= 0)
	{
		OnMiceWin();
		return;
	}
	
	CurrentRound++;
	
	bIsRoundOngoing = true;
	CurrentCatIndex = 0;
	CurrentMouseIndex = 0;

	
	// start the round by selecting first cat in the list
	Cats[CurrentCatIndex]->OnTurnFinished.AddDynamic(this, &ATBTurnedBasedManager::OnTurnFinished);
	Cats[CurrentCatIndex]->ExecuteTurn();
}

void ATBTurnedBasedManager::OnTurnFinished(ATBMammalBase* PlayedMammal, bool bWasSuccessful)
{
	PlayedMammal->OnTurnFinished.Clear();
	
	// move next cat
	if(CurrentCatIndex + 1 < Cats.Num())
	{
		CurrentCatIndex++;
		Cats[CurrentCatIndex]->OnTurnFinished.AddDynamic(this, &ATBTurnedBasedManager::OnTurnFinished);
		Cats[CurrentCatIndex]->ExecuteTurn();
		return;
	}
	
	// then mice move
	if(CurrentMouseIndex < Mice.Num())
	{
		CurrentMouseIndex++;
		Mice[CurrentMouseIndex-1]->OnTurnFinished.AddDynamic(this, &ATBTurnedBasedManager::OnTurnFinished);
		Mice[CurrentMouseIndex-1]->ExecuteTurn();
		return;
	}

	//after all of the mammals moved, try breeding
	TryBreedMammals();
	
	TryStarveMammals();
	
	// round finished
	OnRoundFinished();
	
}

void ATBTurnedBasedManager::OnKillRequested(ATBMammalBase* KilledMammal)
{
	KilledMammal->OnTurnFinished.Clear();
	KilledMammal->OnKilled.Clear();
	
	Mice.Remove(KilledMammal);
	AllMammalsToBreed.Remove(KilledMammal); // remove it from breeding list

	//KilledMammal->GetCurrentTile()->ClearTile();
	FTileInfo* Tile = KilledMammal->GetCurrentTile();
	Tile->bIsEmptyTile = true;
	Tile->MammalRef = nullptr;
	//SquareMapGeneratorRef->ClearTile(TilePos.X,TilePos.Y);

	KilledMammal->Destroy();
}

void ATBTurnedBasedManager::OnStarved(ATBMammalBase* StarvedMammal)
{
	MammalsToStarve.AddUnique(StarvedMammal);
}

void ATBTurnedBasedManager::OnBred(ATBMammalBase* BredMammal)
{
	if(!BredMammal) return;

	//Add BredMammal to breed list if it is not already in the breed list. 
	AllMammalsToBreed.AddUnique(BredMammal);
}


void ATBTurnedBasedManager::OnRoundFinished()
{
	bIsRoundOngoing = false;
	GetWorldTimerManager().ClearTimer(TimerHandle_StartNextRound);
	
	if(bAutoStartNextRound)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_StartNextRound, this, &ATBTurnedBasedManager::StartNextRound, StartNextRoundTime, false);
	}
}


void ATBTurnedBasedManager::TryBreedMammals()
{
	int i = 0;
	while(i < AllMammalsToBreed.Num())
	{
		ATBMammalBase* MammalToBreed = AllMammalsToBreed[i];
		if(!MammalToBreed)
		{
			AllMammalsToBreed.RemoveAt(i);
			continue;
		}
		
		int SavedBreedCount = MammalToBreed->GetSavedBreedCounter();
		if(SavedBreedCount <= 0)
		{
			MammalToBreed->SetSavedBreedCounter(0);
			AllMammalsToBreed.RemoveAt(i);
			continue;
		}
		
		const FTileInfo* MammalTile = MammalToBreed->GetCurrentTile();
		if(!MammalTile)
		{
			MammalToBreed->SetSavedBreedCounter(0);
			AllMammalsToBreed.RemoveAt(i);
			continue;
		}
		
		// try find empty tiles to spawn 
		TArray<FTileInfo*> EmptyTiles = SquareMapGeneratorRef->GetAllAdjacentEmptyTiles(MammalTile);

		//if there is empty tile to spawn and still remaining breeds
		while (EmptyTiles.Num() > 0 && SavedBreedCount > 0)
		{
			// select random empty tile 
			const int RandomIndex = UKismetMathLibrary::RandomIntegerInRange(0, EmptyTiles.Num()-1);

			//spawn the mammal on the empty tile
			ATBMammalBase* SpawnedMammal = SpawnMammal(MammalToBreed->GetClass(), EmptyTiles[RandomIndex]);

			// bind events
			if(MammalToBreed->GetClass() == MouseClass)
			{
				SpawnedMammal->OnKilled.AddDynamic(this, &ATBTurnedBasedManager::OnKillRequested);
				SpawnedMammal->OnBred.AddDynamic(this, &ATBTurnedBasedManager::OnBred);
				Mice.Add(SpawnedMammal);
			}
			else if(MammalToBreed->GetClass() == CatClass)
			{
				SpawnedMammal->OnStarved.AddDynamic(this, &ATBTurnedBasedManager::OnStarved);
				SpawnedMammal->OnBred.AddDynamic(this, &ATBTurnedBasedManager::OnBred);
				Cats.Add(SpawnedMammal);
			}
			SpawnedMammal->MapGeneratorRef = SquareMapGeneratorRef;
			SpawnedMammal->UpdateDebugWidget(SquareMapGeneratorRef->ShowDebug);
			
			// remove the tile that we spawned on
			EmptyTiles.RemoveAt(RandomIndex);

			// decrease saved breeds
			SavedBreedCount--;

			// update saved breeds on the mammal that bred
			MammalToBreed->SetSavedBreedCounter(SavedBreedCount);
		}
		
		i++;
	}
}

void ATBTurnedBasedManager::TryStarveMammals()
{
	for(int i = 0; i< MammalsToStarve.Num(); i++)
	{
		ATBMammalBase* MammalToStarve = MammalsToStarve[i];
		
		//remove the mammal from the list
		MammalsToStarve.RemoveAt(i);

		if(!MammalToStarve)
		{
			continue;
		}
		
		//remove the mammal from cats or mice
		if(MammalToStarve->GetClass() == CatClass)
		{
			Cats.Remove(MammalToStarve);
		}
		else if(MammalToStarve->GetClass() == MouseClass)
		{
			Mice.Remove(MammalToStarve);
		}
		
		// also remove it from breeding list, if possible
		AllMammalsToBreed.Remove(MammalToStarve);

		// destroy the mammal and clear the tile
		FTileInfo* Tile = MammalToStarve->GetCurrentTile();
		Tile->bIsEmptyTile = true;
		Tile->MammalRef = nullptr;
		MammalToStarve->Destroy();
	}
}

