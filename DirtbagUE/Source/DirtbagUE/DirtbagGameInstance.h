// The engine-side owner of career and day state. One PlayerState, one
// DayState, one seed — living here so they survive level transitions (the
// van→gym drive) and so every wall, HUD, and interact reads the same truth.
// Loads on boot, saves on sleep. All rules stay in Sim/; this class only
// holds state and forwards to it.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "DirtbagSimTypes.h"

// The sim's own container, deliberately: UHT parses every declaration in a
// UCLASS body and cannot resolve a plain namespaced C++ type inside a
// TArray. See the note in DirtbagSimLibrary.h.
#include <vector>

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
	Crag,
	/** The Shaded Cave: rope routes, north-facing, forty minutes up the
	 *  hill. Outdoors like the Crag in every respect that matters -- the
	 *  window, the guidebook, cleaning, beta -- and different only in which
	 *  rock it loads and which way that rock faces. */
	Cave
};

/** Is this venue rock rather than plastic? Everything outdoors shares the
 *  weather, the guidebook and the brush; only the gym does not. Written
 *  once so that adding the third venue could not leave a `== Crag` test
 *  behind that quietly means "not the cave either". */
inline bool IsOutdoors(EDirtbagVenue Venue)
{
	return Venue != EDirtbagVenue::Gym;
}

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

	/** Sit it out. Hours pass, hunger with them, and a little energy comes
	 *  back — the shade, not a night's sleep. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	void Rest(double Hours);

	/** Hours until today's window opens: 0 if it is already open, has been
	 *  and gone, or the day never comes good at all. The thing waiting is
	 *  actually for. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	double HoursUntilWindow();

	/** "the rock comes good at 3:45pm" / "it is on, right now" / "today is
	 *  not going to happen". */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	FString WaitAdvice();

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

	/** A line at a named outdoor venue, so a wall can ask for cave rock
	 *  while the player is still standing at Roadside. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Crag")
	FDirtbagCragLine GetCragLineAt(EDirtbagVenue AtVenue, int32 Index);

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

	/** "Bouncin  V4  FA you" — the line as the book will print it, set the
	 *  moment a name is given and carried for the rest of the day. Not
	 *  saved: it is an acknowledgement, and the ascent itself lives in the
	 *  ledger. Empty when nothing has been named today. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|FirstAscent")
	FString LastAscentLine;

	/** Raised by the wall when a line nobody had done goes. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	void OfferNaming(int32 BoardIndex);

	/** Walk away without naming it. The line stays yours to name later —
	 *  the ascent happened, and nothing about it expires. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	void DismissNaming();

	/** Where this session stands: warm enough, skinned enough, or done.
	 *  Meaningful once you have pulled on today. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	EDirtbagSessionAdvice ReadSession() const;

	/** "still cold — pull on something easy first" */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	FString SessionAdviceText() const;

	// --- Gear and the van ------------------------------------------------

	/** "the rubber is going; worth a resole" */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Gear")
	FString ShoeLine() const;

	/** What the rubber is costing you right now, in grades on edging holds.
	 *  0 on a new pair.
	 *
	 *  Worth showing because it is worth a lot and has never been visible:
	 *  measured over ten seasons, a climber who never replaces their shoes
	 *  sends 1.0 against 3.7 on the same burns, for about $9 of the year's
	 *  cash. The cheapest thing in the game that matters. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Gear")
	double ShoeCostInGrades() const;

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Gear")
	bool ResoleShoes();

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Gear")
	bool BuyNewShoes();

	// --- Ethics ----------------------------------------------------------
	// An ethical shortcut is not a transaction, it is a secret with a fuse.
	// You take the benefit now and carry a thing that can come out — and
	// the more people are watching you, the more likely it is that somebody
	// noticed. Success is what exposes you.

	/** Do it. Records only that it happened; the caller applies whatever
	 *  the act buys, because the benefit differs per act and belongs where
	 *  it is felt. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Ethics")
	void DoSomethingYouWouldNotAdmitTo(EDirtbagEthicalAct Act,
	                                   const FString& OnRoute);

	/** How closely you are watched, 0..1 — Scene standing plus what a
	 *  sponsor has made of you. This is the number that decides how long
	 *  you get away with it. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Ethics")
	double HowWatchedYouAre() const;

	/** How many things you are carrying that nobody knows about. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Ethics")
	int32 ThingsNobodyKnows() const;

	/** Roll for the day. Returns the line to show when something surfaces,
	 *  empty otherwise — "Everyone knows you chipped a hold on Chalk Ghost
	 *  now. It was nine years ago. It does not matter that it was."
	 *
	 *  Called from Sleep, so a secret comes out the way a player meets one:
	 *  by waking up to it. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Ethics")
	FString DoesAnybodyFindOutToday();

	// --- Sponsorship -----------------------------------------------------
	// The only money in the game that arrives because you climbed rather
	// than instead of it — and the only money whose cost is measured in
	// good days, because you cannot shoot climbing photos in the rain.

	/** What would be offered right now, given what people can see: what you
	 *  have sent, what you have put up, and what the Scene thinks. Ability
	 *  is not the currency — a crusher nobody has heard of gets nothing. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Sponsor")
	EDirtbagSponsorTier OfferOnTheTable() const;

	/** Take it. Says something about you: the Scene likes a sponsored
	 *  climber and the old guard has opinions. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Sponsor")
	bool SignWithSponsor();

	/** "free shoes, and they want nothing" / "$640 a month, and they want
	 *  three days — and they will not be the rainy ones" */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Sponsor")
	FString SponsorLine() const;

	/** Do they own today? Only ever a day with a window. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Sponsor")
	bool SponsorOwnsToday() const;

	// --- Retiring --------------------------------------------------------

	/** Whose career this is. Set when the player names themselves; used on
	 *  the epitaph and on every line they put up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Legacy")
	FString ClimberName;

	/** Injuries back to back, which says it before the numbers do. Reset by
	 *  a season that does not end in one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Legacy")
	int32 ConsecutiveInjuries = 0;

	/** The best this career ever was, so decline is measured against it
	 *  rather than against an age. Plenty of people climb their hardest at
	 *  forty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Legacy")
	double PeakGradeEver = 0.0;
	// Offered, never forced. Deciding when to stop is the last real choice a
	// climbing career contains, and taking it away would be the one
	// unforgivable thing to do to one.

	/** Has the game got something honest to say about stopping? Never a
	 *  command — a body that keeps breaking, or two grades off your best and
	 *  past the age. Age alone is never the reason. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Legacy")
	bool TimeToThinkAboutIt() const;

	/** "9 seasons. Hardest: The Guidebook Lied, V7. Two lines that are yours
	 *  now. Chalk Ghost never went, after 210 tries. Retired at 47, with the
	 *  crag open." */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Legacy")
	FString CareerEpitaph() const;

	/** End it. Tallies what the career was, files it with the ones before,
	 *  and hands the valley to somebody twenty-four with nothing in their
	 *  fingers — because the world remembers and the body does not. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Legacy")
	void RetireAndPassItOn(const FString& RetiringAs);

	/** How many came before. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Legacy")
	int32 GenerationsBefore() const;

	/** The guidebook lines earlier generations put up: "Cattle Grid Arete,
	 *  V7. FA Evan". These are the whole of what survives a retirement. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Legacy")
	TArray<FString> InheritedGuidebook() const;

	// --- Age -------------------------------------------------------------
	// Derived from the day counter, never stored — which is why adding it
	// needed no save version, and why a save can never disagree with a
	// birthday.

	UFUNCTION(BlueprintPure, Category = "Dirtbag|Body")
	double Age() const;

	/** "31 — still going up" / "44 — the good years, if you are careful" */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Body")
	FString AgeLine() const;

	// --- Where you stand -------------------------------------------------

	/** "the crag is closed. The signs went up on the gate." — or empty when
	 *  nobody has an opinion about you yet. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Standing")
	FString StandingLine() const;

	/** -1 they will not have you .. +1 you are one of theirs. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Standing")
	double StandingWith(EDirtbagFaction Faction) const;

	/** False when the signs are up. Push the stewards far enough and access
	 *  gets pulled — for nine days, which is a season and not a sentence. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Standing")
	bool CragIsOpen() const;

	// --- Work ------------------------------------------------------------

	/** Today's board: three gigs, deterministic per world and day. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Work")
	TArray<FDirtbagOddJob> TodaysJobBoard() const;

	/** Take a gig. Costs the hours and the energy, pays into debt first,
	 *  and says something about you — the best-paying gig on the board is
	 *  shooting guidebook photos, and it costs you the old guard and the
	 *  stewards both. False if the van is dead and the gig needed it. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Work")
	bool TakeOddJob(const FDirtbagOddJob& Job);

	/** Nine to five, five days a week. The hours are the point, not the
	 *  money — and in midwinter the light is gone before you clock off. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Work")
	void TakeSalariedJob();

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Work")
	void QuitSalariedJob();

	/** Does the salary own today? */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Work")
	bool SalariedToday() const;

	/** Work it. Called from Sleep on any day the salary owns, not offered
	 *  as an action — a trap you can decline is not a trap. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Work")
	void WorkSalariedDay();

	// --- The body --------------------------------------------------------

	/** "a pulley in the ring finger — 3 weeks, if you are sensible", or
	 *  empty when nothing is wrong. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Body")
	FString InjuryLine() const;

	/** "carrying a load" / "everything aches; this is the warning". The
	 *  warning is worth listening to: measured over twelve seasons, a
	 *  climber who stops training at it sends 38 against 24 for one who
	 *  does not, and spends 55 days hurt against 1,467. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Body")
	FString LoadLine() const;

	/** How loudly to say it: 0 nothing worth saying, 1 worth noticing, 2 the
	 *  warning before an injury. The bands are the sim's, so the colour and
	 *  the sentence can never disagree. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Body")
	int32 LoadWarning() const;

	UFUNCTION(BlueprintPure, Category = "Dirtbag|Body")
	bool IsHurt() const;

	/** Money for time: takes days off an injury, once a week at most. The
	 *  one thing money buys that hands climbing back rather than moving it
	 *  around. False if you are not hurt, cannot afford it, or saw one too
	 *  recently. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Body")
	bool SeeAPhysio();

	// --- The kit ---------------------------------------------------------
	// Things worth money that buy you climbing. Each returns false if you
	// cannot afford it, and changes nothing when it does.

	/** A second pad. Measured across twelve seasons, both pads against one
	 *  is +37% sends at identical skin spend — it does not give you more
	 *  climbing, it makes the climbing you already had worth more. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Kit")
	bool BuyCrashPad();

	/** Plywood above the van door. Worth an hour on a day the weather has
	 *  already taken, and a mistake on any other — it spends the skin the
	 *  crag was waiting for. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Kit")
	bool BuyHangboard();

	/** A month of plastic. The only climbing that ignores the weather. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Kit")
	bool RenewGymMembership();

	UFUNCTION(BlueprintPure, Category = "Dirtbag|Kit")
	bool IsGymMember() const;

	/** "one pad, a board in the van, and the gym until the month runs out" */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Kit")
	FString KitLine() const;

	/** "A second pad, $260. Better landings, and one less reason to be
	 *  brave." Empty once you own the pads that matter.
	 *
	 *  The pad is the purchase measured to move a season most, and its
	 *  price in head was invisible — a player found out months later that
	 *  they had stopped getting braver, with nothing having said so. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Kit")
	FString PadOfferLine() const;

	/** A day on plastic. False if you are not a member — the gym is the one
	 *  place in this game that checks. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Kit")
	bool GoToTheGym();

	/** An hour on the board. False without one, or on skin already gone. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Kit")
	bool HangboardSession();

	/** "the belt is on borrowed time" — empty when there is nothing to say. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Van")
	FString VanLine() const;

	/** Is the van going anywhere? */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Van")
	bool VanRuns() const;

	/** The part that needs attention, or -1 when nothing does. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Van")
	int32 WorstVanPart() const;

	/** Free, four hours, and it will not hold. Always available — being
	 *  broke must never end a save. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Van")
	bool BodgeVan();

	/** Cash, twice per part, and it holds a while. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Van")
	bool PatchVan();

	/** More cash, and actually fixed. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Van")
	bool ReplaceVanPart();

	/** Drive somewhere: wears the van, charges the fuel, and may break it.
	 *  Returns the part that went, or -1. Called by the travel spot.
	 *
	 *  Every drive in the game comes through here. That is deliberate: fuel
	 *  is the only cost that goes up the more you climb, and the way it came
	 *  to be free was that each travel spot could have charged it and none
	 *  had to. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Van")
	int32 DriveVan(double Hours);

	/** What the last drive cost at the pump. Read by the travel spot so the
	 *  toast can say it — a cost the player never sees is a cost that feels
	 *  like a bug. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Van")
	double LastDriveFuel = 0.0;

	/** Set when something let go on a drive. Cleared at lights out. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Van")
	FString VanNews;

	// --- The dog ---------------------------------------------------------

	/** Feed it. Costs cash; enough meals and the stray is yours, with no
	 *  ceremony because the life does not provide one. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Dog")
	bool FeedTheDog();

	/** "the dog is asleep under the van" / "there is a stray at the Lot" */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Dog")
	FString DogLine();

	/** What a parked van is running at right now. Vans are ovens; the
	 *  conditions system already knows the air temperature, so this is that
	 *  plus what a metal box in the sun adds to it. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Dog")
	double VanTempF();

	/** True when the van is somewhere a dog can be left today. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Dog")
	bool VanIsSafeForTheDog();

	/** How hot a parked van runs above the air around it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Dog")
	double VanGreenhouseF = 18.0;

	/** Set when you left it somewhere you should not have. Cleared at
	 *  lights out, like everything else you get to stop thinking about. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Dog")
	FString DogWorry;

	/** What came out overnight, if anything did. Set by Sleep and cleared
	 *  by the next one, exactly like DogWorry and VanNews — a secret should
	 *  reach the player the way it reaches a real climber, which is that you
	 *  wake up and everyone already knows. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Ethics")
	FString EthicsNews;

	/** What the sponsor did overnight: the month's money, or the once-a-year
	 *  verdict on whether they are keeping you. Empty on any night neither
	 *  happened. Not saved — it is news, and news is for the morning it
	 *  arrives. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Sponsor")
	FString SponsorNews;

	// --- The Lot ---------------------------------------------------------

	/** The Lot's people as they are today: strength derived from seed and
	 *  date, rapport and claims from the career. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Lot")
	TArray<FDirtbagPartner> GetLot();

	/** An hour at the fire. Time passes, rapport grows with everyone there,
	 *  and you hear what people are working — which is the answer to
	 *  waiting for a window being lonely. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Lot")
	FString SitAtTheFire(double Hours);

	/** What the fire has to say right now, without spending anything. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Lot")
	TArray<FString> LotTalk();

	/** Ask whoever knows this line best for beta. Returns how much you
	 *  learned, 0 if nobody there can help. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Lot")
	double AskForBeta(int32 BoardIndex, FString& OutWho);

	/** News from overnight: "Dev got the arete left of Diesel." Stays up for
	 *  the day and is replaced at the next lights-out, because losing a line
	 *  is not a thing to glance at once and lose. Empty when nothing
	 *  happened, which is most nights. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Lot")
	FString LotNews;

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

	/** Who signs an ascent. A climber who has not named themselves is still
	 *  allowed to do first ascents; the book calls them "you" and means it. */
	FString AscentSignature() const;

	/** Today's Lot, with the career's bonds folded in. */
	std::vector<dirtbag::Partner> LotToday();

	/** Write rapport and claims back into the career. */
	void StoreBonds(const std::vector<dirtbag::Partner>& Lot);

	/** Overnight: the Lot climbs too, and rapport moves. */
	void AdvanceTheLot();

	/** True once the player has pulled on anything today — what rapport is
	 *  actually earned by. */
	bool bClimbedToday = false;

	/** True once a shift has been worked today: the dog was on its own for
	 *  four hours, which is what its bond is priced on. */
	bool bWorkedToday = false;

	/** The ones that came before, oldest first. Not a UPROPERTY: these are
	 *  sim types, saved and loaded through DirtbagSave with everything else,
	 *  and Blueprint reads them through InheritedGuidebook(). */
	std::vector<dirtbag::Legacy> Legacies;

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

	/** Which crag `Crag` currently holds. Roadside and the cave are
	 *  different rock with different aspects, so arriving at one has to
	 *  reload rather than keep serving the other's lines. */
	EDirtbagVenue LoadedCrag = EDirtbagVenue::Crag;
};
