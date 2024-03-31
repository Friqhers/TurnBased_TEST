// Fill out your copyright notice in the Description page of Project Settings.


#include "TBTurnedBasedManager.h"
#include "SquareMapGeneration/TBSquareMapGenerator.h"
#include "SquareMapGeneration/TBTile.h"
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
	SquareMapGeneratorRef->GenerateSquareMap_V2();

	return;
	
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


void ATBTurnedBasedManager::InitSpawnMammals()
{
	//spawn cats
	for(int i= 0 ; i < NumberOfCatsToSpawn; i++)
	{
		if(ATBTile* EmptyTile = SquareMapGeneratorRef->GetRandomEmptyTile())
		{
			ATBMammalBase* SpawnedCat = EmptyTile->SpawnMammal(CatClass);
			SpawnedCat->OnStarved.AddDynamic(this, &ATBTurnedBasedManager::OnStarved);
			SpawnedCat->OnBred.AddDynamic(this, &ATBTurnedBasedManager::OnBred);
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
		if(ATBTile* EmptyTile = SquareMapGeneratorRef->GetRandomEmptyTile())
		{
			ATBMammalBase* SpawnedMouse = EmptyTile->SpawnMammal(MouseClass);
			SpawnedMouse->OnKilled.AddDynamic(this, &ATBTurnedBasedManager::OnKillRequested);
			SpawnedMouse->OnBred.AddDynamic(this, &ATBTurnedBasedManager::OnBred);
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
		//@TODO: cat lose
		return;
	}

	if(Mice.Num() <= 0)
	{
		//@TODO: mouse lose
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

	KilledMammal->GetCurrentTile()->ClearTile();
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
	for (int i = 0; i< AllMammalsToBreed.Num(); i++)
	{
		ATBMammalBase* MammalToBreed = AllMammalsToBreed[i];
		int SavedBreedCount = MammalToBreed->GetSavedBreedCounter();
		if(SavedBreedCount <= 0)
		{
			AllMammalsToBreed[i]->SetSavedBreedCounter(0);
			AllMammalsToBreed.RemoveAt(i);
			continue;
		}

		if(!MammalToBreed->GetCurrentTile())
		{
			continue;
		}

		// try find empty tiles to spawn 
		TArray<ATBTile*> EmptyTiles = MammalToBreed->GetCurrentTile()->GetAllAdjacentEmptyTiles();

		//if there is empty tile to spawn and still remaining breeds
		while (EmptyTiles.Num() > 0 && SavedBreedCount > 0)
		{
			// select random empty tile 
			const int RandomIndex = UKismetMathLibrary::RandomIntegerInRange(0, EmptyTiles.Num()-1);

			//spawn the mammal on empty tile
			ATBMammalBase* SpawnedMammal = EmptyTiles[RandomIndex]->SpawnMammal(MammalToBreed->GetClass());

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
			
			// remove the tile that we spawned on
			EmptyTiles.RemoveAt(RandomIndex);

			// decrease saved breeds
			SavedBreedCount--;

			// update saved breeds on the mammal that bred
			MammalToBreed->SetSavedBreedCounter(SavedBreedCount);
		}
	}
}

void ATBTurnedBasedManager::TryStarveMammals()
{
	for(int i = 0; i< MammalsToStarve.Num(); i++)
	{
		ATBMammalBase* MammalToStarve = MammalsToStarve[i];

		if(!MammalToStarve)
		{
			MammalsToStarve.RemoveAt(i);
			continue;
		}
		
		//remove the mammal from the list
		MammalsToStarve.RemoveAt(i);

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
		MammalToStarve->GetCurrentTile()->ClearTile();
	}
}

