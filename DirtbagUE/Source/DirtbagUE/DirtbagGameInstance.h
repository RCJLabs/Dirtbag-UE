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

/**
 * Where you are climbing. A gym has a thermostat and somebody else's brush;
 * a crag has a shade line, dirt, and lines nobody has done.
 *
 * This is a property of the place, set by the walls and travel spots you
 * actually walk up to, rather than a switch on the game instance — a game
 * instance is not an actor, has no details panel, and cannot be selected in
 * the level, so anything that has to be flipped by hand there is a setting
 * nobody can find.
 */
UENUM(BlueprintType)
enum class EDirtbagVenue : uint8
{
	Gym,
	Crag
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
	 *  thermostat rather than a decision. Set by SetVenue, which the walls
	 *  and travel spots call — do not expect to find this in a details
	 *  panel, because a game instance does not have one. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Conditions")
	bool bIndoors = true;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Conditions")
	EDirtbagVenue Venue = EDirtbagVenue::Gym;

	/** Arrive somewhere. Cheap and idempotent — the walls call it every time
	 *  you walk up to one, so where you are can never drift from what you
	 *  are standing in front of. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	void SetVenue(EDirtbagVenue NewVenue);

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

	/** The route at this index in whichever venue is live. Prefer
	 *  GetRouteAt from anything that knows its own venue: this one reads
	 *  global state, which is only correct once the player has arrived. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	FDirtbagRoute GetBoardRoute(int32 Index);

	/** The route at this index in a named venue, independent of where the
	 *  player currently is. A wall resolves its route at BeginPlay, long
	 *  before anyone has walked up to it, so asking "where am I?" at that
	 *  moment gives the wrong answer — and gave a crag wall a gym problem. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	FDirtbagRoute GetRouteAt(EDirtbagVenue AtVenue, int32 Index);

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

	// --- First ascents ---------------------------------------------------
	// clean -> work -> send -> name. No locks: a virgin line is simply
	// filthy, and filthy rock climbs about four grades harder than it will
	// once you have spent an afternoon on it with a brush.

	/** An hour on the brush at the line this wall points at. Costs the day's
	 *  time and energy, so cleaning competes with climbing. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	double CleanLine(int32 BoardIndex, double Hours);

	/** Whether it is worth pulling on yet. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	bool IsWorkable(int32 BoardIndex);

	/** "filthy; you can find the holds but not use them" */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	FString CleanlinessText(int32 BoardIndex);

	/** True when you have done a line nobody had done, and the naming is
	 *  therefore yours. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	bool CanNameLine(int32 BoardIndex);

	/** Name it. Records the claim and confirms what it really went at —
	 *  which nobody, including the guidebook, knew until now. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	bool NameFirstAscent(int32 BoardIndex, const FString& Name);

	/** "Roadside Rites  V8  FA you (the book said V7)" — empty if unnamed. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	FString FirstAscentLine(int32 BoardIndex);

	// --- The naming prompt -----------------------------------------------
	// Typing a name needs a text box, and a text box means UMG. Rather than
	// bind C++ to one widget class, the wall raises a flag here and a widget
	// blueprint watches it — so the look of the prompt stays entirely in the
	// editor and this layer only says "there is a line waiting to be named".

	/** True from the moment a nameable line goes until the name is given or
	 *  the prompt is dismissed. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|FirstAscent")
	bool bNamingPending = false;

	/** Which line is waiting. Pass this straight back to NameFirstAscent. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|FirstAscent")
	int32 NamingBoardIndex = 0;

	/** "the arete left of Diesel" — what to show above the text box. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|FirstAscent")
	FString NamingLineText;

	/** Raised by the wall when a line nobody had done goes. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	void OfferNaming(int32 BoardIndex);

	/** Walk away without naming it. The line stays yours to name later —
	 *  the ascent happened, and nothing about it expires. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	void DismissNaming();

	/** Every first ascent in this career, hardest first. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	TArray<FDirtbagProjectMemory> GetFirstAscents() const;

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

	/** The ledger for a line, created filthy the first time a virgin line is
	 *  touched. Returns null only when there is nothing at that index. */
	FDirtbagProjectMemory* LedgerFor(int32 BoardIndex);

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
