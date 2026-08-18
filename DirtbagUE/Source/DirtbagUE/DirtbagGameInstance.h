// The engine-side owner of career and day state. One PlayerState, one
// DayState, one seed — living here so they survive level transitions (the
// van→gym drive) and so every wall, HUD, and interact reads the same truth.
// Loads on boot, saves on sleep. All rules stay in Sim/; this class only
// holds state and forwards to it.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "DirtbagSimTypes.h"

#include "DirtbagGameInstance.generated.h"

/**
 * What the session wants drawn right now. Published by the wall, read by
 * the HUD — so the presentation layer shares one truth and a future UMG
 * widget binds to the same fields the debug HUD uses.
 */
USTRUCT(BlueprintType)
struct FDirtbagSessionReadout
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	bool bActive = false;

	/** "Volume Country  V5" */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	FString RouteLine;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	double Pump = 0.0;

	/** Best-case odds for the move you're on; < 0 when no move is pending. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	double Odds = -1.0;

	/** Grip charge 0..1; < 0 when not charging. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	double Grip = -1.0;

	/** The latch window, so the bar can show you where to let go. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	double WindowStart = 0.6;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	double WindowEnd = 0.95;
};

UCLASS()
class UDirtbagGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	/** The world's identity; every RNG stream derives from it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	FString Seed = TEXT("gym-1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	FString SaveFilename = TEXT("dirtbag-save.txt");

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	FDirtbagPlayerState Player;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	FDirtbagDayState Day;

	/** Live session readout for the HUD; the wall keeps this current. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	FDirtbagSessionReadout SessionReadout;

	/** True when the current Player came from disk rather than a fresh start. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	bool bLoadedFromSave = false;

	// --- Day verbs (mutating members must live here: Blueprint struct
	// --- access returns copies, so the owner does the mutating) ----------

	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	bool EatMeal();

	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	void WorkShift();

	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	void PassHours(double Hours);

	/** First pull-on of the day seeds the session; later calls no-op. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	void EnsureAtGym();

	/** Lights out: sim sleep, then save — the save file's one write point. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	void Sleep();

	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	bool SaveNow();

	// --- Conditions ------------------------------------------------------
	// Weather is derived from Seed + Day, so it is never saved and cannot
	// drift from a reload. Aspect is a property of the place you are
	// climbing; the gym is indoors and ignores all of this.

	/** Which way the current crag faces. Set from the crag itself the moment
	 *  one is loaded, so the guidebook and the shade line cannot disagree;
	 *  change it afterwards to feel out how aspect moves the window. Indoors
	 *  it is ignored — a gym has no shade line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Conditions")
	EDirtbagAspect CragAspect = EDirtbagAspect::North;

	/** True while the player is climbing indoors, where conditions are a
	 *  thermostat rather than a decision. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Conditions")
	bool bIndoors = true;

	/** The gym's flat, boring friction. Slightly under outdoor prime on
	 *  purpose: plastic in a warm room is never actually good. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Conditions")
	double IndoorFriction = 0.5;

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	FDirtbagWeather TodaysWeather() const;

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	FDirtbagPrimeWindow TodaysWindow() const;

	/** Friction right now — indoors, the thermostat; outdoors, the rock. This
	 *  is what every attempt this instance starts is resolved against. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	double CurrentFriction() const;

	/** "greasy - 61F on the rock. window 5:30pm to 7:15pm, best at 6:30pm" */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	FString ConditionsLine() const;

	// --- What you are climbing on ----------------------------------------
	// One index, two venues: indoors it selects a gym problem, outdoors a
	// line from the guidebook. Walls carry only the index, so moving a wall
	// between venues is a flag rather than a rebuild.

	/** The gym's board for this world, cached. Indoor venue only. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	TArray<FDirtbagRoute> GetBoard();

	/** The route at this index in whichever venue is live. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	FDirtbagRoute GetBoardRoute(int32 Index);

	/** This world's crag, cached. Outdoors only. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Crag")
	FDirtbagCrag GetCrag();

	/** The guidebook entry at this index — stars, project status, the lot.
	 *  Meaningless indoors, where a gym problem has no book. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Crag")
	FDirtbagCragLine GetCragLine(int32 Index);

	/** How many things there are to climb where you are standing. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	int32 NumRoutesHere();

	/** Bot-driven burn on a route, day-integrated (session, ledger, time,
	 *  energy, training all move together). */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	FDirtbagAttemptResult ReplayAttempt(const FDirtbagRoute& Route);

	/** What the ledgers add up to, and the one-line version of it. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	FDirtbagCareerSummary GetCareer() const;

	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	FString GetCareerLine() const;

	/** Lifetime attempts on a route, for "Attempt N" staging. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	int32 AttemptsOn(const FDirtbagRoute& Route) const;

	// --- C++-side live-attempt plumbing for ADirtbagClimbWall ------------
	// Not UFUNCTIONs: sim types don't cross into Blueprint.

	dirtbag::LiveAttempt BeginLiveFor(const FDirtbagRoute& Route);
	FDirtbagAttemptResult CommitLiveFor(const dirtbag::LiveAttempt& Live);

	/** Session seed for today: attempts are replayable per world+day. */
	FString TodaysSessionSeed() const;

private:
	void EnsureBoard();
	void EnsureCrag();

	// The forecast changes once a day; the HUD asks for it every frame.
	// Cheap either way (~23us), but there is no reason to re-hash the seed
	// and re-integrate a day of solar loading sixty times a second.
	mutable int32 CachedWeatherDay = -1;
	mutable FDirtbagWeather CachedWeather;
	mutable int32 CachedWindowDay = -1;
	mutable EDirtbagAspect CachedWindowAspect = EDirtbagAspect::North;
	mutable FDirtbagPrimeWindow CachedWindow;

	UPROPERTY()
	TArray<FDirtbagRoute> Board;

	UPROPERTY()
	FDirtbagCrag Crag;

	bool bCragLoaded = false;
};
