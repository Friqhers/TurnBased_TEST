// Fill out your copyright notice in the Description page of Project Settings.


#include "SquareMapGeneration/TBTile.h"

#include "Mammals/TBMammalBase.h"
#include "SquareMapGeneration/TBSquareMapGenerator.h"

// Sets default values
ATBTile::ATBTile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	TileFloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileFloorMesh"));
	RootComponent = TileFloorMesh;

	// tile is not occupied by default
	bIsEmptyTile = true;
	
}

// Called when the game starts or when spawned
void ATBTile::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ATBTile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATBTile::ClearTile()
{
	// Mark the tile as empty
	SetTileEmpty(true);
	if(MammalRef)
	{
		MammalRef->Destroy();
	}
}

ATBMammalBase* ATBTile::SpawnMammal(TSubclassOf<ATBMammalBase> MammalClass)
{
	if(!MammalClass) return nullptr;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn the mammal at the tile's location
	MammalRef = GetWorld()->SpawnActor<ATBMammalBase>(MammalClass, GetActorLocation(), FRotator::ZeroRotator, Params);

	// Calculate mammal bounds and adjust its position to snap it to the tile
	FVector TileOrigin, TileHalfExtents;
	GetActorBounds(false, TileOrigin, TileHalfExtents, true);
	MammalRef->SetActorLocation(GetActorLocation() + FVector(0,0,TileHalfExtents.Z));
	//MammalRef->SetCurrentTile(this);

	// Mark the tile as not empty
	SetTileEmpty(false);
	
	return MammalRef;
}

ATBTile* ATBTile::GetTileAtDirection(const EDirectionType Direction, const TArray<TArray<ATBTile*>>& SpawnedTiles) const
{
	ATBTile* FoundTile = nullptr;
	
	if(!SquareMapGeneratorRef) return FoundTile;
	
	if(Direction == EDirectionType::South)
	{
		// if south tile is valid
		if(Position2D.Y - 1 >= 0)
		{
			FoundTile = SpawnedTiles[Position2D.Y-1][Position2D.X];
		}
	}
	else if(Direction == EDirectionType::North)
	{
		// if north tile is valid
		if(Position2D.Y + 1 < SpawnedTiles.Num())
		{
			FoundTile = SpawnedTiles[Position2D.Y+1][Position2D.X];
		}
	}
	else if(Direction == EDirectionType::West)
	{
		// if west tile is valid
		if(Position2D.X - 1 >= 0)
		{
			FoundTile = SpawnedTiles[Position2D.Y][Position2D.X - 1];
		}
			
	}
	else if(Direction == EDirectionType::East)
	{
		// if east tile is valid
		if(Position2D.X + 1 < SpawnedTiles.Num())
		{
			FoundTile = SpawnedTiles[Position2D.Y][Position2D.X + 1];
		}
	}

	return FoundTile;
}

TArray<ATBTile*> ATBTile::GetAllAdjacentTiles() const
{
	TArray<ATBTile*> AdjacentTiles;
	
	if(!SquareMapGeneratorRef) return AdjacentTiles;
	
	const TArray<TArray<ATBTile*>> SpawnedTiles = SquareMapGeneratorRef->GetSpawnedTiles2D();

	if(ATBTile* NorthTile = GetTileAtDirection(EDirectionType::North, SpawnedTiles))
	{
		AdjacentTiles.Add(NorthTile);
	}
	if(ATBTile* SouthTile = GetTileAtDirection(EDirectionType::South, SpawnedTiles))
	{
		AdjacentTiles.Add(SouthTile);
	}
	if(ATBTile* WestTile = GetTileAtDirection(EDirectionType::West, SpawnedTiles))
	{
		AdjacentTiles.Add(WestTile);
	}
	if(ATBTile* EastTile = GetTileAtDirection(EDirectionType::East, SpawnedTiles))
	{
		AdjacentTiles.Add(EastTile);
	}

	return AdjacentTiles;
}

TArray<ATBTile*> ATBTile::GetAllAdjacentEmptyTiles() const
{
	TArray<ATBTile*> AdjacentEmptyTiles;
	
	if(!SquareMapGeneratorRef) return AdjacentEmptyTiles;
	
	const TArray<TArray<ATBTile*>> SpawnedTiles = SquareMapGeneratorRef->GetSpawnedTiles2D();

	if(ATBTile* NorthTile = GetTileAtDirection(EDirectionType::North, SpawnedTiles))
	{
		if(NorthTile->bIsEmptyTile)
		{
			AdjacentEmptyTiles.Add(NorthTile);
		}
	}
	if(ATBTile* SouthTile = GetTileAtDirection(EDirectionType::South, SpawnedTiles))
	{
		if(SouthTile->bIsEmptyTile)
		{
			AdjacentEmptyTiles.Add(SouthTile);
		}
	}
	if(ATBTile* WestTile = GetTileAtDirection(EDirectionType::West, SpawnedTiles))
	{
		if(WestTile->bIsEmptyTile)
		{
			AdjacentEmptyTiles.Add(WestTile);
		}
	}
	if(ATBTile* EastTile = GetTileAtDirection(EDirectionType::East, SpawnedTiles))
	{
		if(EastTile->bIsEmptyTile)
		{
			AdjacentEmptyTiles.Add(EastTile);
		}
	}

	return AdjacentEmptyTiles;
}

void ATBTile::SetPosition2D(const FVector2D& TargetPos)
{
	Position2D = TargetPos;
}

FVector2D ATBTile::GetPosition2D() const
{
	return Position2D;
}

bool ATBTile::IsTileEmpty() const
{
	return bIsEmptyTile;
}

void ATBTile::SetTileEmpty(bool bIsEmpty)
{
	bIsEmptyTile = bIsEmpty;
	OnTileUpdated();
}

