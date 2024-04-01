// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TBTurnedBasedManager.generated.h"

struct FTileInfo;
class ATBMammalBase;
class ATBSquareMapGenerator;

UCLASS()
class TURNBASEDCATMOUSE_API ATBTurnedBasedManager : public AActor
{
	GENERATED_BODY()
public:
	// Sets default values for this actor's properties
	ATBTurnedBasedManager();
public:	
	//Settings
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turned Based Manager")
	TSubclassOf<ATBSquareMapGenerator> SquareMapGenClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turned Based Manager")
	TSubclassOf<ATBMammalBase> CatClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turned Based Manager")
	TSubclassOf<ATBMammalBase> MouseClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turned Based Manager")
	int NumberOfCatsToSpawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turned Based Manager")
	int NumberOfMiceToSpawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turned Based Manager")
	float StartNextRoundTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Turned Based Manager")
	bool bAutoStartNextRound;

	//

	UPROPERTY(BlueprintReadOnly)
	ATBSquareMapGenerator* SquareMapGeneratorRef;
private:
	int CurrentRound;

	// Index of the next cat to move in current round. Resets to zero after each round
	int CurrentCatIndex;

	// Index of the next cat to move in current round. Resets to zero after each round
	int CurrentMouseIndex;

	// All living cats
	TArray<ATBMammalBase*> Cats;

	// All living mice
	TArray<ATBMammalBase*> Mice;

	/* Mammals in this list are going to breed after move turns finished.
	 * If breed was successful and finished for the mammal it will be removed from this list.
	 */
	TArray<ATBMammalBase*> AllMammalsToBreed;
	
	// Mammals in this list are going to starve after breeding finishes. 
	TArray<ATBMammalBase*> MammalsToStarve;

	bool bIsRoundOngoing;
	
	FTimerHandle TimerHandle_StartNextRound;

private:
	/**
	 * @brief Spawns a mammal on the given tile and sets references accordingly.
	 * @param MammalClass The class of the mammal to spawn.
	 * @param TargetTile The tile to spawn mammal on.
	 * @return Returns a pointer to the spawned mammal.
	 */
	ATBMammalBase* SpawnMammal(TSubclassOf<ATBMammalBase> MammalClass, FTileInfo* TargetTile) const;
	
	// Spawns cats and mouse at random tiles
	void InitSpawnMammals();

	// Tries to breed mammals in AllMammalsToBreed list at the end of each round.
	void TryBreedMammals();

	// Tries to starve mammals in MammalsToStarve list at the end of each round.
	void TryStarveMammals();
protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Turned Based Manager|Events")
	void OnCatsWin();

	UFUNCTION(BlueprintImplementableEvent, Category = "Turned Based Manager|Events")
	void OnMiceWin();
private:
	// EVENTS

	/**
	 * @brief Called after a mammal completes its turn.
	 * @param PlayedMammal The mammal that completed its turn.
	 * @param bWasSuccessful Indicates whether the mammal successfully completed its turn.
	 */
	UFUNCTION()
	void OnTurnFinished(ATBMammalBase* PlayedMammal, bool bWasSuccessful);

	/**
	 * @brief Called after a mammal killed by another mammal.
	 * @param KilledMammal The mammal that is killed.
	 */
	UFUNCTION()
	void OnKillRequested(ATBMammalBase* KilledMammal);

	/**
	 * @brief Called after a mammal starved in its turn.
	 * @param StarvedMammal The mammal that is starved.
	 */
	UFUNCTION()
	void OnStarved(ATBMammalBase* StarvedMammal);
	
	/**
	 * @brief Called after a mammal requested breed in its turn.
	 * @param BredMammal The mammal that is starved.
	 */
	UFUNCTION()
	void OnBred(ATBMammalBase* BredMammal);
	
	void OnRoundFinished();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/**
	* @brief Starts the square map generation and spawns mammals. 
	* Then starts the next round if bAutoStartNextRound is true.
	*/
	UFUNCTION(BlueprintCallable, Category = "Turned Based Manager")
	void StartTurnBasedGame();

	// If the round is not on going, starts the next round.
	UFUNCTION(BlueprintCallable, Category = "Turned Based Manager")
	void StartNextRound();

	UFUNCTION(BlueprintPure, Category = "Turn Based Manager") FORCEINLINE
	int GetCurrentRound() const { return CurrentRound; }

	UFUNCTION(BlueprintPure, Category = "Turn Based Manager") FORCEINLINE
	int GetAliveCatsCount() const {return Cats.Num(); }

	UFUNCTION(BlueprintPure, Category = "Turn Based Manager") FORCEINLINE
	int GetAliveMiceCount() const {return Mice.Num(); }

	UFUNCTION(BlueprintPure, Category = "Turn Based Manager") FORCEINLINE
	bool GetIsRoundOnGoing() const {return bIsRoundOngoing; }
	
};
