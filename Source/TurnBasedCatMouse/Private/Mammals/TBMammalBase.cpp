// Fill out your copyright notice in the Description page of Project Settings.


#include "Mammals/TBMammalBase.h"
#include "Kismet/KismetMathLibrary.h"
#include "SquareMapGeneration/TBTile.h"

// Sets default values
ATBMammalBase::ATBMammalBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MammalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MammalMeshComp"));
	RootComponent = MammalMesh;

	StarvationTurnCount = 3;
	StarveCounter = 0;
	bCanEat = false;
	bCanBreed = true;
}

// Called when the game starts or when spawned
void ATBMammalBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATBMammalBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// do smooth interpolation 
	if(bMoveStarted)
	{
		SetActorLocation(UKismetMathLibrary::VInterpTo(
			GetActorLocation(), CurrentMoveTargetPosition, DeltaTime, MoveInterpSpeed));

		if(GetActorLocation().Equals(CurrentMoveTargetPosition, 0.01))
		{
			OnMoveFinished(true);
		}
	}
}

void ATBMammalBase::SetCurrentTilePos(FVector2D TargetPos)
{
	CurrentTilePos = TargetPos;
}

FVector2D ATBMammalBase::GetCurrentTilePos()
{
	return CurrentTilePos;
}

void ATBMammalBase::ExecuteTurn()
{
	//@TODO: do these in tryeat func
	if(bCanEat)
	{
		if(GetRandomEatTarget(EatTarget))
		{
			StartEat(EatTarget);
			return;
		}
	}

	
	StartRandomMove();
	// move random direction 1 unit
	//TryMove();
}

bool ATBMammalBase::GetRandomEatTarget(FTileInfo& EatTargetTile) const
{
	// create current tile info, only the CurrentTilePos is important
	const FTileInfo CurrentTile = FTileInfo(CurrentTilePos, FVector::ZeroVector, false, nullptr);
	TArray<FTileInfo> AdjacentTiles = MapGeneratorRef->GetAllAdjacentTiles(CurrentTile);

	if(AdjacentTiles.Num() <= 0)
		return false;

	// find if there any eatable mammal within 1 unit
	TArray<FTileInfo> EatableMammalTiles;
	for(int i = 0 ; i < AdjacentTiles.Num() ; i++)
	{
		if(!AdjacentTiles[i].bIsEmptyTile && AdjacentTiles[i].MammalRef && AdjacentTiles[i].MammalRef->GetClass() == EatableMammalClass)
		{
			EatableMammalTiles.Add(AdjacentTiles[i]);
		}
	}

	// nothing eatable
	if(EatableMammalTiles.Num() <= 0)
		return false;
	
	// select random target mammal
	const int RandomIndex = UKismetMathLibrary::RandomIntegerInRange(0, EatableMammalTiles.Num()-1);
	EatTargetTile = EatableMammalTiles[RandomIndex];

	return true;
}

void ATBMammalBase::StartRandomMove()
{
	FTileInfo CurrentTile = FTileInfo(CurrentTilePos, FVector::ZeroVector, false, this);
	
	const TArray<FTileInfo> AdjacentEmptyTiles = MapGeneratorRef->GetAllAdjacentEmptyTiles(CurrentTile);

	if(AdjacentEmptyTiles.Num() <= 0)
	{
		OnMoveFinished(false);
		return;
	}

	// empty current tile
	MapGeneratorRef->ClearTile(CurrentTilePos.X, CurrentTilePos.Y);

	// select random target empty tile to move
	const int RandomIndex = UKismetMathLibrary::RandomIntegerInRange(0, AdjacentEmptyTiles.Num()-1);

	// set move target position that will be used to interpolate in Tick()
	CurrentMoveTargetPosition = AdjacentEmptyTiles[RandomIndex].WordLocation;
	CurrentMoveTargetPosition.Z = GetActorLocation().Z;

	// apply the move
	CurrentTile.WordLocation = AdjacentEmptyTiles[RandomIndex].WordLocation;
	CurrentTile.Pos2D = AdjacentEmptyTiles[RandomIndex].Pos2D;
	CurrentTilePos = CurrentTile.Pos2D;
	
	MapGeneratorRef->UpdateTileAt(CurrentTile, CurrentTilePos.X, CurrentTilePos.Y);

	// start the interpolation in Tick()
	bMoveStarted = true;
	PrimaryActorTick.bCanEverTick = true;
}

void ATBMammalBase::StartEat(const FTileInfo& EatTargetTile)
{
	FTileInfo CurrentTile = FTileInfo(CurrentTilePos, FVector::ZeroVector, false, this);

	// empty current tile
	MapGeneratorRef->ClearTile(CurrentTilePos.X, CurrentTilePos.Y);
	
	
	// set move target position that will be used to interpolate in Tick()
	CurrentMoveTargetPosition = EatTargetTile.WordLocation;
	CurrentMoveTargetPosition.Z = GetActorLocation().Z;

	bEatOnMoveFinished = true;
	
	// start the interpolation in Tick()
	bMoveStarted = true;
	PrimaryActorTick.bCanEverTick = true;
}

void ATBMammalBase::OnMoveFinished(const bool bWasSuccessful)
{
	// disable movement in tick
	bMoveStarted = false;
	PrimaryActorTick.bCanEverTick = false; 

	// if eat target is valid, we requested a move with eat 
	if(bEatOnMoveFinished)
	{
		bEatOnMoveFinished = false;
		
		//reset starve counter
		if(bCanStarve)
			StarveCounter = 0;

		
		//call on killed event for the victim
		EatTarget.MammalRef->OnKilled.Broadcast(EatTarget.MammalRef);
		
		//apply the move
		FTileInfo CurrentTile = FTileInfo(EatTarget.Pos2D, EatTarget.WordLocation, false, this);
		CurrentTilePos = CurrentTile.Pos2D;
		MapGeneratorRef->UpdateTileAt(CurrentTile, CurrentTilePos.X, CurrentTilePos.Y);
	}
	else if(bCanStarve)
	{
		TryStarve();
		StarveCounter++;
		//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Blue, FString::Printf(TEXT("Cat starve counter updated: %d"), StarveCounter));
	}

	if(bCanBreed)
	{
		TryBreed();
	}

	OnTurnFinished.Broadcast(this, bWasSuccessful);
	OnTurnFinished.Clear();
}


void ATBMammalBase::TryBreed()
{
	if(BreedCounter >= BreedTurnCount)
	{
		SavedBreedCounter++;
		BreedCounter = 0;
	}
	else
	{
		//increase for each turn 
		BreedCounter++;
	}

	if(SavedBreedCounter > 0)
	{
		// call on bred event to actually try spawn the mammal
		OnBred.Broadcast(this);
		return;
	}

	
}

void ATBMammalBase::TryStarve()
{
	if(StarveCounter >= StarvationTurnCount)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("Cat starve event called at : %d"), StarveCounter));
		OnStarved.Broadcast(this);
	}
}


