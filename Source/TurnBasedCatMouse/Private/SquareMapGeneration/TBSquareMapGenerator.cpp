// Fill out your copyright notice in the Description page of Project Settings.


#include "SquareMapGeneration/TBSquareMapGenerator.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Mammals/TBMammalBase.h"
#include "SquareMapGeneration/TBTile.h"

// Sets default values
ATBSquareMapGenerator::ATBSquareMapGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SquareMapSize = 8;

	InstancedStaticMeshComponent = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("InstancedStaticMeshComponent"));
	InstancedStaticMeshComponent->SetupAttachment(RootComponent);
	
	//TileScale = FVector(1,1,1);

}


// Called when the game starts or when spawned
void ATBSquareMapGenerator::BeginPlay()
{
	Super::BeginPlay();

	if(!TileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATBSquareMapGenerator::BeginPlay -> Tile class is not valid. Please set it and then restart the game!"));
		// maybe exit the game here
		return;
	}

	

	// get tile size by spawning tile actor and then destroy it
	if(ATBTile* Tile = GetWorld()->SpawnActor<ATBTile>(TileClass, GetActorLocation(), GetActorRotation(), GetActorSpawnParameters()))
	{
		FVector Origin;
		Tile->GetActorBounds(false, Origin, TileHalfExtents, true);
		Tile->Destroy();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ATBSquareMapGenerator::BeginPlay -> Couldn't spawned tile class named %s"), *TileClass->GetName());
	}

	
	if(TileHalfExtents.IsNearlyZero(0.01))
	{
		UE_LOG(LogTemp, Error, TEXT("ATBSquareMapGenerator::BeginPlay -> Tile size is too small!"));
		UKismetSystemLibrary::QuitGame(this,
			UGameplayStatics::GetPlayerController(this,0),
			EQuitPreference::Quit,
			false);
		return;
	}
	
}

bool ATBSquareMapGenerator::GenerateSquareMap_V2()
{
	if(!InstancedStaticMeshComponent->GetStaticMesh()) return false;

	const float halfExtentsX = InstancedStaticMeshComponent->GetStaticMesh()->GetBoundingBox().GetExtent().X;
	const float halfExtentsY = InstancedStaticMeshComponent->GetStaticMesh()->GetBoundingBox().GetExtent().Y;

	// clamp to min 8x8, max 99x99;
	SquareMapSize = FMath::Clamp(SquareMapSize, 2, 99);

	Tiles2D.Empty();

	
	// Resize the outer array to match the SquareMapSize
	Tiles2D.SetNum(SquareMapSize);

	FVector StartLoc = GetActorLocation();
	FVector CurrentLoc  = StartLoc;

	FActorSpawnParameters params = GetActorSpawnParameters();

	//spawn tiles
	for (int y = 0; y < SquareMapSize; y++)
	{
		// Resize each inner array to match the SquareMapSize
		Tiles2D[y].SetNum(SquareMapSize);

		CurrentLoc.X = StartLoc.X;
		
		for(int x = 0; x < SquareMapSize; x++)
		{
			FTransform Transform;
			Transform.SetLocation(CurrentLoc);
			InstancedStaticMeshComponent->AddInstance(Transform, true);

			// save the tile 
			Tiles2D[y][x] = FTileInfo(true, nullptr);
			AllTiles.Add(FTileInfo(true, nullptr));
			
			// move right by tile extents
			CurrentLoc.X += halfExtentsX * 2;
		}

		// move down by tile extents
		CurrentLoc.Y -= halfExtentsY * 2;
	}

	//spawn walls
	SpawnBorderWalls();
	
	return true;
}

bool ATBSquareMapGenerator::GenerateSquareMap()
{
	if(!TileClass || !WallClass) return false;
	
	// clamp to min 8x8, max 99x99;
	SquareMapSize = FMath::Clamp(SquareMapSize, 2, 99);

	SpawnedTiles2D.Empty();
	
	// Resize the outer array to match the SquareMapSize
	SpawnedTiles2D.SetNum(SquareMapSize);

	FVector StartLoc = GetActorLocation();
	FVector CurrentLoc  = StartLoc;

	FActorSpawnParameters params = GetActorSpawnParameters();

	//spawn tiles
	for (int y = 0; y < SquareMapSize; y++)
	{
		// Resize each inner array to match the SquareMapSize
		SpawnedTiles2D[y].SetNum(SquareMapSize);

		CurrentLoc.X = StartLoc.X;
		
		for(int x = 0; x < SquareMapSize; x++)
		{
			//Spawn the tile at current location
			ATBTile* Tile = GetWorld()->SpawnActor<ATBTile>(TileClass, CurrentLoc, FRotator::ZeroRotator, params);
			Tile->SetPosition2D(FVector2D(x,y));
			Tile->SquareMapGeneratorRef = this;
			
			// save the tile 
			SpawnedTiles2D[y][x] = Tile;
			AllSpawnedTiles.Add(Tile);
			
			// move right by tile extents
			CurrentLoc.X += TileHalfExtents.X * 2;

			
		}

		// move down by tile extents
		CurrentLoc.Y -= TileHalfExtents.Y * 2;
	}

	// spawn walls
	SpawnBorderWalls();

	// spawn mammals (3 cats and 50 mice)
	
	return true;
}

ATBTile* ATBSquareMapGenerator::GetRandomEmptyTile() const
{
	if(SpawnedTiles2D.Num() <= 0) return nullptr;

	TArray<ATBTile*> UncheckedTiles = AllSpawnedTiles;

	while (UncheckedTiles.Num() > 0)
	{
		int RandomIndex = UKismetMathLibrary::RandomIntegerInRange(0, UncheckedTiles.Num()-1);
		if(UncheckedTiles[RandomIndex]->IsTileEmpty())
		{
			return UncheckedTiles[RandomIndex];
		}

		UncheckedTiles.RemoveAt(RandomIndex);
	}

	return nullptr;
}

void ATBSquareMapGenerator::SpawnBorderWalls()
{
	const bool bIsEven = (SquareMapSize % 2) == 0;
	const int middleIndex = SquareMapSize / 2;
	const FVector totalHalfExtents = TileHalfExtents * SquareMapSize;
	const float ScaleMultiplierY = totalHalfExtents.Y / TileHalfExtents.Y;
	const float ScaleMultiplierX = totalHalfExtents.X / TileHalfExtents.X;
	
	if(middleIndex-1 < 0)
	{
		//@TODO: abort generating map
		return;
	}
	// middle position x
	FVector middleX = bIsEven ? (SpawnedTiles2D[0][middleIndex-1]->GetActorLocation() + SpawnedTiles2D[0][middleIndex]->GetActorLocation())/2 :
										 SpawnedTiles2D[0][middleIndex]->GetActorLocation();
	// middle position y
	FVector middleY = bIsEven ? (SpawnedTiles2D[middleIndex-1][0]->GetActorLocation() + SpawnedTiles2D[middleIndex][0]->GetActorLocation())/2 :
	 								 SpawnedTiles2D[middleIndex][0]->GetActorLocation();

	// combine and set middle position
	FVector middlePos = middleX;
	middlePos.Y = middleY.Y;
	middlePos.Z += TileHalfExtents.Z * 2;

	// save middle pos, it can be useful later
	SquareMapMiddle = middlePos;

	// calculate move deltas from middle to borders
	float MoveDeltaX = (TileHalfExtents.X * 2) * (SquareMapSize-middleIndex);
	MoveDeltaX += bIsEven ? TileHalfExtents.X : 0;

	float MoveDeltaY = (TileHalfExtents.Y * 2) * (SquareMapSize-middleIndex);
	MoveDeltaY += bIsEven ? TileHalfExtents.Y : 0;

	
	//spawn left wall
	FVector leftWallPos = middlePos;
	leftWallPos.X -= MoveDeltaX;
	ATBTile* leftWall = GetWorld()->SpawnActor<ATBTile>(WallClass, leftWallPos, FRotator::ZeroRotator, GetActorSpawnParameters());
	FVector oldScaleL = leftWall->GetActorScale3D();
	oldScaleL.Y *= ScaleMultiplierY;
	leftWall->SetActorScale3D(oldScaleL);

	//spawn right wall
	FVector rightWallPos = middlePos;
	rightWallPos.X += MoveDeltaX;
	ATBTile* rightWall = GetWorld()->SpawnActor<ATBTile>(WallClass, rightWallPos, FRotator::ZeroRotator, GetActorSpawnParameters());
	FVector oldScaleR = rightWall->GetActorScale3D();
	oldScaleR.Y *= ScaleMultiplierY;
	rightWall->SetActorScale3D(oldScaleR);

	//spawn up wall
	FVector upWallPos = middlePos;
	upWallPos.Y += MoveDeltaY;
	ATBTile* upWall = GetWorld()->SpawnActor<ATBTile>(WallClass, upWallPos, FRotator::ZeroRotator, GetActorSpawnParameters());
	FVector oldScaleU = upWall->GetActorScale3D();
	oldScaleU.X *= ScaleMultiplierX;
	upWall->SetActorScale3D(oldScaleU);

	//spawn down wall
	FVector downWallPos = middlePos;
	downWallPos.Y -= MoveDeltaY;
	ATBTile* downWall = GetWorld()->SpawnActor<ATBTile>(WallClass, downWallPos, FRotator::ZeroRotator, GetActorSpawnParameters());
	FVector oldScaleD = downWall->GetActorScale3D();
	oldScaleD.X *= ScaleMultiplierX;
	downWall->SetActorScale3D(oldScaleD);
}


FActorSpawnParameters ATBSquareMapGenerator::GetActorSpawnParameters()
{
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Parameters.Owner = this;

	return Parameters;
}

TArray<TArray<ATBTile*>> ATBSquareMapGenerator::GetSpawnedTiles2D()
{
	return SpawnedTiles2D;
}

// Called every frame
void ATBSquareMapGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FVector ATBSquareMapGenerator::GetSquareMapMiddle()
{
	return SquareMapMiddle;
}

