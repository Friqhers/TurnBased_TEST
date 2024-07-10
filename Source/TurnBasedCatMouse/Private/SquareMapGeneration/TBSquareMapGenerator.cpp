// Fill out your copyright notice in the Description page of Project Settings.


#include "SquareMapGeneration/TBSquareMapGenerator.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

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

	
	// get tile extents
	if(const UStaticMesh* StaticMesh = InstancedStaticMeshComponent->GetStaticMesh())
	{
		TileHalfExtents = StaticMesh->GetBoundingBox().GetExtent();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ATBSquareMapGenerator::BeginPlay -> Please set the static mesh for InstancedStaticMeshComponent!"));
		UKismetSystemLibrary::QuitGame(this,
			UGameplayStatics::GetPlayerController(this,0),
			EQuitPreference::Quit,
			false);
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

bool ATBSquareMapGenerator::GenerateSquareMap()
{
	if(!InstancedStaticMeshComponent->GetStaticMesh()) return false;
	
	// clamp to min 8x8, max 99x99;
	SquareMapSize = FMath::Clamp(SquareMapSize, 2, 999);

	Tiles.Empty();

	
	// Resize the outer array to match the SquareMapSize
	Tiles.SetNum(SquareMapSize);

	const FVector StartLoc = GetActorLocation();
	FVector CurrentLoc  = StartLoc;
	

	//spawn tiles
	for (int y = 0; y < SquareMapSize; y++)
	{
		// Resize each inner array to match the SquareMapSize
		Tiles[y].SetNum(SquareMapSize);

		CurrentLoc.X = StartLoc.X;
		
		for(int x = 0; x < SquareMapSize; x++)
		{
			FTransform Transform;
			Transform.SetLocation(CurrentLoc);
			InstancedStaticMeshComponent->AddInstance(Transform, true);

			
			// save the tile 
			//Tiles2D[y][x] = FTileInfo(FVector2D(x,y), CurrentLoc, true, nullptr);
			
			

			FTileInfo* Tile = new FTileInfo(FVector2D(x,y), CurrentLoc, true, nullptr);
			Tiles[y][x] = Tile;
			AllTiles.Add(Tile);
			
			// move right by tile extents
			CurrentLoc.X += TileHalfExtents.X * 2;
		}

		// move down by tile extents
		CurrentLoc.Y -= TileHalfExtents.Y * 2;
	}
	// -Y is North, +X is East 

	//spawn walls
	SpawnBorderWalls();
	
	return true;
}


void ATBSquareMapGenerator::SpawnBorderWalls()
{
	const bool bIsEven = (SquareMapSize % 2) == 0;
	const int middleIndex = SquareMapSize / 2;
	const FVector totalHalfExtents = TileHalfExtents * SquareMapSize;
	const float ScaleMultiplierY = totalHalfExtents.Y / TileHalfExtents.Y;
	const float ScaleMultiplierX = totalHalfExtents.X / TileHalfExtents.X;
	
	if(middleIndex-1 < 0 || !WallClass)
	{
		return;
	}
	
	// middle position x
	FVector middleX = bIsEven ? (Tiles[0][middleIndex-1]->WordLocation + Tiles[0][middleIndex]->WordLocation)/2 :
										 Tiles[0][middleIndex]->WordLocation;
	// middle position y
	FVector middleY = bIsEven ? (Tiles[middleIndex-1][0]->WordLocation + Tiles[middleIndex][0]->WordLocation)/2 :
	 								 Tiles[middleIndex][0]->WordLocation;

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
	AActor* leftWall = GetWorld()->SpawnActor<AActor>(WallClass, leftWallPos, FRotator::ZeroRotator, GetActorSpawnParameters());
	FVector oldScaleL = leftWall->GetActorScale3D();
	oldScaleL.Y *= ScaleMultiplierY;
	leftWall->SetActorScale3D(oldScaleL);

	//spawn right wall
	FVector rightWallPos = middlePos;
	rightWallPos.X += MoveDeltaX;
	AActor* rightWall = GetWorld()->SpawnActor<AActor>(WallClass, rightWallPos, FRotator::ZeroRotator, GetActorSpawnParameters());
	FVector oldScaleR = rightWall->GetActorScale3D();
	oldScaleR.Y *= ScaleMultiplierY;
	rightWall->SetActorScale3D(oldScaleR);

	//spawn up wall
	FVector upWallPos = middlePos;
	upWallPos.Y += MoveDeltaY;
	AActor* upWall = GetWorld()->SpawnActor<AActor>(WallClass, upWallPos, FRotator::ZeroRotator, GetActorSpawnParameters());
	FVector oldScaleU = upWall->GetActorScale3D();
	oldScaleU.X *= ScaleMultiplierX;
	upWall->SetActorScale3D(oldScaleU);

	//spawn down wall
	FVector downWallPos = middlePos;
	downWallPos.Y -= MoveDeltaY;
	AActor* downWall = GetWorld()->SpawnActor<AActor>(WallClass, downWallPos, FRotator::ZeroRotator, GetActorSpawnParameters());
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

FVector ATBSquareMapGenerator::GetTileHalfExtents() const
{
	return TileHalfExtents;
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


FTileInfo* ATBSquareMapGenerator::GetTileAtDirection(const FTileInfo* SourceTile, const EDirectionType Direction) const
{
	FTileInfo* TileResult = nullptr;
	
	if(Tiles.Num() <= 0) return TileResult;
	
	const FVector2D Position2D = SourceTile->Pos2D;
	
	
	if(Direction == EDirectionType::South)
	{
		// if south tile is valid
		if(Position2D.Y - 1 >= 0)
		{
			TileResult = Tiles[Position2D.Y-1][Position2D.X];
			return TileResult;
		}
	}
	else if(Direction == EDirectionType::North)
	{
		// if north tile is valid
		if(Position2D.Y + 1 < Tiles.Num())
		{
			TileResult = Tiles[Position2D.Y+1][Position2D.X];
			return TileResult;
		}
	}
	else if(Direction == EDirectionType::West)
	{
		// if west tile is valid
		if(Position2D.X - 1 >= 0)
		{
			TileResult = Tiles[Position2D.Y][Position2D.X - 1];
			return TileResult;
		}
			
	}
	else if(Direction == EDirectionType::East)
	{
		// if east tile is valid
		if(Position2D.X + 1 < Tiles.Num())
		{
			TileResult = Tiles[Position2D.Y][Position2D.X + 1];
			return TileResult;
		}
	}

	return TileResult;
}

TArray<FTileInfo*> ATBSquareMapGenerator::GetAllAdjacentEmptyTiles(const FTileInfo* SourceTile) const
{
	TArray<FTileInfo*> AdjacentEmptyTiles;
	
	if(Tiles.Num() <= 0) return AdjacentEmptyTiles;
	
	
	if(FTileInfo* TileResult = GetTileAtDirection(SourceTile, EDirectionType::North))
	{
		if(TileResult->bIsEmptyTile)
		{
			AdjacentEmptyTiles.Add(TileResult);
		}
	}
	if(FTileInfo* TileResult = GetTileAtDirection(SourceTile, EDirectionType::South))
	{
		if(TileResult->bIsEmptyTile)
		{
			AdjacentEmptyTiles.Add(TileResult);
		}
	}
	if(FTileInfo* TileResult = GetTileAtDirection(SourceTile, EDirectionType::West))
	{
		if(TileResult->bIsEmptyTile)
		{
			AdjacentEmptyTiles.Add(TileResult);
		}
	}
	if(FTileInfo* TileResult = GetTileAtDirection(SourceTile, EDirectionType::East))
	{
		if(TileResult->bIsEmptyTile)
		{
			AdjacentEmptyTiles.Add(TileResult);
		}
	}

	return AdjacentEmptyTiles;
}

TArray<FTileInfo*> ATBSquareMapGenerator::GetAllAdjacentTiles(const FTileInfo* SourceTile) const
{
	TArray<FTileInfo*> AdjacentEmptyTiles;
	
	if(Tiles.Num() <= 0) return AdjacentEmptyTiles;
	
	
	if(FTileInfo* TileResult = GetTileAtDirection(SourceTile, EDirectionType::North))
	{
		AdjacentEmptyTiles.Add(TileResult);
	}
	if(FTileInfo* TileResult = GetTileAtDirection(SourceTile, EDirectionType::South))
	{
		AdjacentEmptyTiles.Add(TileResult);
	}
	if(FTileInfo* TileResult = GetTileAtDirection(SourceTile, EDirectionType::West))
	{
		AdjacentEmptyTiles.Add(TileResult);
	}
	if(FTileInfo* TileResult = GetTileAtDirection(SourceTile, EDirectionType::East))
	{
		AdjacentEmptyTiles.Add(TileResult);
	}

	return AdjacentEmptyTiles;
}

FTileInfo* ATBSquareMapGenerator::GetRandomEmptyTile() const
{
	FTileInfo* FoundTile = nullptr;
	if(AllTiles.Num() <= 0) return nullptr;

	TArray<FTileInfo*> UncheckedTiles = AllTiles;

	while (UncheckedTiles.Num() > 0)
	{
		const int RandomIndex = UKismetMathLibrary::RandomIntegerInRange(0, UncheckedTiles.Num()-1);
		if(UncheckedTiles[RandomIndex]->bIsEmptyTile)
		{
			FoundTile = UncheckedTiles[RandomIndex];
			return FoundTile;
		}
		UncheckedTiles.RemoveAt(RandomIndex);
	}

	return nullptr;
}
