// Fill out your copyright notice in the Description page of Project Settings.


#include "SquareMapGeneration/TBSquareMapGenerator.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
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

	
	// get tile extents
	if(const UStaticMesh* StaticMesh = InstancedStaticMeshComponent->GetStaticMesh())
	{
		TileHalfExtents = StaticMesh->GetBoundingBox().GetExtent();
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

bool ATBSquareMapGenerator::GenerateSquareMap()
{
	if(!InstancedStaticMeshComponent->GetStaticMesh()) return false;
	
	// clamp to min 8x8, max 99x99;
	SquareMapSize = FMath::Clamp(SquareMapSize, 2, 999);

	Tiles2D.Empty();

	
	// Resize the outer array to match the SquareMapSize
	Tiles2D.SetNum(SquareMapSize);

	const FVector StartLoc = GetActorLocation();
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
			Tiles2D[y][x] = FTileInfo(FVector2D(x,y), CurrentLoc, true, nullptr);
			AllTiles.Add(Tiles2D[y][x]);
			
			// move right by tile extents
			CurrentLoc.X += TileHalfExtents.X * 2;
		}

		// move down by tile extents
		CurrentLoc.Y -= TileHalfExtents.Y * 2;
	}

	//spawn walls
	SpawnBorderWalls();
	
	return true;
}

// bool ATBSquareMapGenerator::GenerateSquareMap()
// {
// 	if(!TileClass || !WallClass) return false;
// 	
// 	// clamp to min 8x8, max 99x99;
// 	SquareMapSize = FMath::Clamp(SquareMapSize, 2, 99);
//
// 	SpawnedTiles2D.Empty();
// 	
// 	// Resize the outer array to match the SquareMapSize
// 	SpawnedTiles2D.SetNum(SquareMapSize);
//
// 	FVector StartLoc = GetActorLocation();
// 	FVector CurrentLoc  = StartLoc;
//
// 	FActorSpawnParameters params = GetActorSpawnParameters();
//
// 	//spawn tiles
// 	for (int y = 0; y < SquareMapSize; y++)
// 	{
// 		// Resize each inner array to match the SquareMapSize
// 		SpawnedTiles2D[y].SetNum(SquareMapSize);
//
// 		CurrentLoc.X = StartLoc.X;
// 		
// 		for(int x = 0; x < SquareMapSize; x++)
// 		{
// 			//Spawn the tile at current location
// 			ATBTile* Tile = GetWorld()->SpawnActor<ATBTile>(TileClass, CurrentLoc, FRotator::ZeroRotator, params);
// 			Tile->SetPosition2D(FVector2D(x,y));
// 			Tile->SquareMapGeneratorRef = this;
// 			
// 			// save the tile 
// 			SpawnedTiles2D[y][x] = Tile;
// 			AllSpawnedTiles.Add(Tile);
// 			
// 			// move right by tile extents
// 			CurrentLoc.X += TileHalfExtents.X * 2;
//
// 			
// 		}
//
// 		// move down by tile extents
// 		CurrentLoc.Y -= TileHalfExtents.Y * 2;
// 	}
//
// 	// spawn walls
// 	SpawnBorderWalls();
//
// 	// spawn mammals (3 cats and 50 mice)
// 	
// 	return true;
// }


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
	FVector middleX = bIsEven ? (Tiles2D[0][middleIndex-1].WordLocation + Tiles2D[0][middleIndex].WordLocation)/2 :
										 Tiles2D[0][middleIndex].WordLocation;
	// middle position y
	FVector middleY = bIsEven ? (Tiles2D[middleIndex-1][0].WordLocation + Tiles2D[middleIndex][0].WordLocation)/2 :
	 								 Tiles2D[middleIndex][0].WordLocation;

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

bool ATBSquareMapGenerator::UpdateTileAt(FTileInfo NewTileInfo, int TargetX, int TargetY)
{
	if(TargetX < 0 || TargetY < 0) return false;

	if(TargetX >= Tiles2D.Num() || TargetY >=Tiles2D.Num()) return false;

	Tiles2D[TargetY][TargetX] = NewTileInfo;

	int index1D = (TargetY * SquareMapSize) + TargetX;
	AllTiles[index1D] = NewTileInfo;

	return true;
}

bool ATBSquareMapGenerator::GetTileAt(FTileInfo& TargetTile, int TargetX, int TargetY)
{
	if(TargetX < 0 || TargetY < 0) return false;

	if(TargetX >= Tiles2D.Num() || TargetY >=Tiles2D.Num()) return false;

	TargetTile = Tiles2D[TargetY][TargetX];
	return true;
}

bool ATBSquareMapGenerator::ClearTile(int TargetX, int TargetY)
{
	if(TargetX < 0 || TargetY < 0) return false;

	if(TargetX >= Tiles2D.Num() || TargetY >=Tiles2D.Num()) return false;

	Tiles2D[TargetY][TargetX].bIsEmptyTile = true;
	Tiles2D[TargetY][TargetX].MammalRef = nullptr;
	return true;
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



bool ATBSquareMapGenerator::GetRandomEmptyTile(FTileInfo& FoundTile) const
{
	if(Tiles2D.Num() <= 0) return false;

	TArray<FTileInfo> UncheckedTiles = AllTiles;

	while (UncheckedTiles.Num() > 0)
	{
		int RandomIndex = UKismetMathLibrary::RandomIntegerInRange(0, UncheckedTiles.Num()-1);
		if(UncheckedTiles[RandomIndex].bIsEmptyTile)
		{
			FoundTile = UncheckedTiles[RandomIndex];
			return true;
		}
		UncheckedTiles.RemoveAt(RandomIndex);
	}

	return false;
}

bool ATBSquareMapGenerator::GetTileAtDirection(FTileInfo& TileResult, const FTileInfo& SourceTile, const EDirectionType Direction) const
{
	if(Tiles2D.Num() <= 0) return false;
	FVector2D Position2D = SourceTile.Pos2D;
	
	if(Direction == EDirectionType::South)
	{
		// if south tile is valid
		if(Position2D.Y - 1 >= 0)
		{
			TileResult = Tiles2D[Position2D.Y-1][Position2D.X];
			return true;
		}
	}
	else if(Direction == EDirectionType::North)
	{
		// if north tile is valid
		if(Position2D.Y + 1 < Tiles2D.Num())
		{
			TileResult = Tiles2D[Position2D.Y+1][Position2D.X];
			return true;
		}
	}
	else if(Direction == EDirectionType::West)
	{
		// if west tile is valid
		if(Position2D.X - 1 >= 0)
		{
			TileResult = Tiles2D[Position2D.Y][Position2D.X - 1];
			return true;
		}
			
	}
	else if(Direction == EDirectionType::East)
	{
		// if east tile is valid
		if(Position2D.X + 1 < Tiles2D.Num())
		{
			TileResult = Tiles2D[Position2D.Y][Position2D.X + 1];
			return true;
		}
	}

	return false;
}

TArray<FTileInfo> ATBSquareMapGenerator::GetAllAdjacentEmptyTiles(const FTileInfo& SourceTile) const
{
	TArray<FTileInfo> AdjacentEmptyTiles;
	
	if(Tiles2D.Num() <= 0) return AdjacentEmptyTiles;
	
	FTileInfo TileResult;
	if(GetTileAtDirection(TileResult, SourceTile, EDirectionType::North))
	{
		if(TileResult.bIsEmptyTile)
		{
			AdjacentEmptyTiles.Add(TileResult);
		}
	}
	if(GetTileAtDirection(TileResult, SourceTile, EDirectionType::South))
	{
		if(TileResult.bIsEmptyTile)
		{
			AdjacentEmptyTiles.Add(TileResult);
		}
	}
	if(GetTileAtDirection(TileResult, SourceTile, EDirectionType::West))
	{
		if(TileResult.bIsEmptyTile)
		{
			AdjacentEmptyTiles.Add(TileResult);
		}
	}
	if(GetTileAtDirection(TileResult, SourceTile, EDirectionType::East))
	{
		if(TileResult.bIsEmptyTile)
		{
			AdjacentEmptyTiles.Add(TileResult);
		}
	}

	return AdjacentEmptyTiles;
}

TArray<FTileInfo> ATBSquareMapGenerator::GetAllAdjacentTiles(const FTileInfo& SourceTile) const
{
	TArray<FTileInfo> AdjacentEmptyTiles;
	
	if(Tiles2D.Num() <= 0) return AdjacentEmptyTiles;
	
	FTileInfo TileResult;
	if(GetTileAtDirection(TileResult, SourceTile, EDirectionType::North))
	{
		AdjacentEmptyTiles.Add(TileResult);
	}
	if(GetTileAtDirection(TileResult, SourceTile, EDirectionType::South))
	{
		AdjacentEmptyTiles.Add(TileResult);
	}
	if(GetTileAtDirection(TileResult, SourceTile, EDirectionType::West))
	{
		AdjacentEmptyTiles.Add(TileResult);
	}
	if(GetTileAtDirection(TileResult, SourceTile, EDirectionType::East))
	{
		AdjacentEmptyTiles.Add(TileResult);
	}

	return AdjacentEmptyTiles;
}

