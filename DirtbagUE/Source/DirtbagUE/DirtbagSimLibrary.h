// The sim as Blueprint nodes. Thin by design: every function here converts,
// calls into Sim/, converts back. If a behavior question comes up, the
// answer lives in Sim/ (and its tests), never here.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/Object.h"

#include "DirtbagSimTypes.h"

#include "DirtbagSimLibrary.generated.h"

/**
 * One live attempt the minigame drives move by move (SETUP.md §4 step 4).
 * Create with UDirtbagSimLibrary::BeginLiveAttempt, then per move:
 * PeekOdds drives the tension UI, StepMove commits with HOLD TO CLIMB's
 * execution quality, ShakeOut is the release verb. Finish for the result,
 * CommitToSession to pay skin/warmth/psyche and update the project ledger.
 */
UCLASS(BlueprintType)
class UDirtbagLiveAttempt : public UObject
{
	GENERATED_BODY()

public:
	/** Odds the next move faces at this execution. Pure preview — no roll. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Attempt")
	double PeekOdds(double Execution) const;

	/** Commit to the next move. After it the attempt may be over. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Attempt")
	FDirtbagMoveResult StepMove(double Execution);

	/** Release to shake out at the current stance. Returns net pump
	 *  recovered; negative means the hang tax beat the stance. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Attempt")
	double ShakeOut();

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Attempt")
	bool IsOver() const;

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Attempt")
	double GetPump() const;

	/** Index of the next unclimbed move (== moves completed so far). */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Attempt")
	int32 GetNextMoveIndex() const;

	/** Styles, skin and the sent flag — call once the attempt is over. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Attempt")
	FDirtbagAttemptResult Finish() const;

	/** Pays the session (skin, warmth, psyche) and updates the project
	 *  ledger, exactly as the sim's batch path does. Call once per attempt,
	 *  after Finish. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Attempt")
	void CommitToSession(UPARAM(ref) FDirtbagSessionState& Session,
	                     UPARAM(ref) FDirtbagProjectMemory& Memory) const;

	// Internal state; set by BeginLiveAttempt only.
	dirtbag::LiveAttempt Live;
};

UCLASS()
class UDirtbagSimLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Deterministically builds a route's moves from its identity — same
	 *  seed + name always yields the same line (worldgen rng stream). */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	static FDirtbagRoute BuildRoute(const FString& WorldSeed,
	                                const FString& RouteName, int32 Grade,
	                                int32 TrueGrade, EDirtbagRouteType Type,
	                                EDirtbagDiscipline Discipline);

	/** A fresh session for this climber: full skin, stone cold. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	static FDirtbagSessionState StartSession(const FDirtbagClimber& Climber);

	/** One bot-driven burn (the staged-replay path, SETUP.md §4 step 3):
	 *  resolves the whole attempt and updates session + ledger in place.
	 *  The returned timeline is the staging script. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	static FDirtbagAttemptResult AttemptInSession(
	    const FString& SessionSeed, UPARAM(ref) FDirtbagSessionState& Session,
	    UPARAM(ref) FDirtbagProjectMemory& Memory,
	    const FDirtbagClimber& Climber, const FDirtbagRoute& Route,
	    double Friction = 0.5, double BotExecution = 0.72);

	/** One player-driven burn (the interactive path, SETUP.md §4 step 4).
	 *  Reads session + ledger but does not change them — CommitToSession on
	 *  the returned object does that when the attempt ends. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	static UDirtbagLiveAttempt* BeginLiveAttempt(
	    const FString& SessionSeed, const FDirtbagSessionState& Session,
	    const FDirtbagProjectMemory& Memory, const FDirtbagClimber& Climber,
	    const FDirtbagRoute& Route, double Friction = 0.5);

	/** "V7" / "5.12a" — the guidebook's ladder for UI text. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	static FString GradeName(int32 Grade, EDirtbagDiscipline Discipline);

	// --- The day loop ---------------------------------------------------

	/** The gym's route board: Count routes laddered V0 upward, deterministic
	 *  per seed, occasional in-house sandbag included. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Day")
	static TArray<FDirtbagRoute> GymBoard(const FString& WorldSeed,
	                                      int32 Count = 8);

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Day")
	static FDirtbagDayState WakeUp(const FDirtbagPlayerState& Player);

	/** Time is never free: hunger rides along. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Day")
	static void PassHours(UPARAM(ref) FDirtbagDayState& Day, double Hours);

	/** False when the wallet says no. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Day")
	static bool EatMeal(UPARAM(ref) FDirtbagPlayerState& Player,
	                    UPARAM(ref) FDirtbagDayState& Day);

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Day")
	static void WorkShift(UPARAM(ref) FDirtbagPlayerState& Player,
	                      UPARAM(ref) FDirtbagDayState& Day);

	/** The climber as they are right now — career skills plus today's
	 *  fatigue speaking through psyche. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Day")
	static FDirtbagClimber ClimberForSession(const FDirtbagPlayerState& Player,
	                                         const FDirtbagDayState& Day);

	/** Pull on: seeds the day's session from the current climber. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Day")
	static void StartGymSession(UPARAM(ref) FDirtbagPlayerState& Player,
	                            UPARAM(ref) FDirtbagDayState& Day);

	/** One bot-driven burn inside a day: resolves against the day's session
	 *  and the player's ledger for this route, then books the day-costs and
	 *  training creep. The one-stop replay node for the gym flow. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Day")
	static FDirtbagAttemptResult DayAttempt(
	    const FString& SessionSeed, UPARAM(ref) FDirtbagPlayerState& Player,
	    UPARAM(ref) FDirtbagDayState& Day, const FDirtbagRoute& Route,
	    double Friction = 0.5, double BotExecution = 0.72);

	/** One player-driven burn inside a day. Drive the returned attempt with
	 *  PeekOdds/StepMove/ShakeOut, then hand it to CommitLiveAttempt. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Day")
	static UDirtbagLiveAttempt* BeginDayLiveAttempt(
	    const FString& SessionSeed, const FDirtbagPlayerState& Player,
	    const FDirtbagDayState& Day, const FDirtbagRoute& Route,
	    double Friction = 0.5);

	/** Pays the session, the project ledger, and the day (time, energy,
	 *  training) for a finished live attempt. Call once, when it's over. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Day")
	static FDirtbagAttemptResult CommitLiveAttempt(
	    UDirtbagLiveAttempt* Attempt, UPARAM(ref) FDirtbagPlayerState& Player,
	    UPARAM(ref) FDirtbagDayState& Day);

	/** Lights out: skin regrows, psyche drifts home, day advances, bills
	 *  land on their morning. Day state resets to the next wake. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Day")
	static void SleepToNextDay(UPARAM(ref) FDirtbagPlayerState& Player,
	                           UPARAM(ref) FDirtbagDayState& Day);

	/** Reading the line from the ground: judged against the guidebook grade,
	 *  so a sandbag looks reasonable right until you are on it. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Day")
	static EDirtbagRouteRead ReadRoute(const FDirtbagClimber& Climber,
	                                   const FDirtbagRoute& Route);

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Day")
	static FString ReadRouteText(EDirtbagRouteRead Read);

	/** What the project ledgers add up to — ability, hardest send, nemesis. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Day")
	static FDirtbagCareerSummary SummarizeCareer(const FDirtbagPlayerState& Player);

	/** The career in one dry line, for a stats card. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Day")
	static FString CareerLine(const FDirtbagCareerSummary& Career);

	// --- Save / load ----------------------------------------------------
	// Serialization itself lives in Sim/DirtbagSave (versioned, migrated,
	// harness-tested); these nodes only move the bytes.

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Save")
	static FString SaveToText(const FString& Seed,
	                          const FDirtbagPlayerState& Player);

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Save")
	static EDirtbagLoadResult LoadFromText(const FString& Text, FString& OutSeed,
	                                       FDirtbagPlayerState& OutPlayer);

	/** Writes under <Project>/Saved/SaveGames/. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Save")
	static bool SaveToFile(const FString& Seed,
	                       const FDirtbagPlayerState& Player,
	                       const FString& Filename = FString(TEXT("dirtbag-save.txt")));

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Save")
	static EDirtbagLoadResult LoadFromFile(
	    const FString& Filename, FString& OutSeed,
	    FDirtbagPlayerState& OutPlayer);

	// --- Conditions ------------------------------------------------------
	// The day's weather and its prime window. All derived from world seed +
	// day, so none of it is saved and none of it can drift from a reload.

	/** This world's weather on this day. Deterministic; costs nothing to
	 *  ask twice. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	static FDirtbagWeather WeatherFor(const FString& WorldSeed, int32 Day);

	/** Friction 0..1 on this face at this hour — the number the resolver
	 *  eats. Below ~0.3 you are wasting skin. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	static double FrictionAt(const FDirtbagWeather& Weather,
	                         EDirtbagAspect Aspect, double Hour);

	/** What the holds actually feel like: air temperature plus the heat the
	 *  wall has banked from the sun, shed on a lag. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	static double RockTempF(const FDirtbagWeather& Weather,
	                        EDirtbagAspect Aspect, double Hour);

	/** The day's best span, or bExists false on a day that never comes good. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	static FDirtbagPrimeWindow PrimeWindowFor(const FDirtbagWeather& Weather,
	                                          EDirtbagAspect Aspect);

	/** "sticky - this is the day" / "greasy". */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	static FString ConditionsText(double Friction);

	/** "window 5:30pm to 7:15pm, best at 6:30pm". */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	static FString WindowText(const FDirtbagPrimeWindow& Window);

	// --- The crag --------------------------------------------------------

	/** The guidebook for this world's roadside crag. Authored lines, seeded
	 *  moves — same seed, same rock, forever. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Crag")
	static FDirtbagCrag RoadsideCrag(const FString& WorldSeed);

	/** "Diesel  V5  ***" / "project, the arete left of Diesel — ..." */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Crag")
	static FString GuidebookLine(const FDirtbagCragLine& Line);
};
