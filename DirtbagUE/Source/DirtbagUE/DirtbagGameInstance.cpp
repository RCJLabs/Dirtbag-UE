#include "DirtbagGameInstance.h"

#include "DirtbagSimLibrary.h"

void UDirtbagGameInstance::Init()
{
	Super::Init();

	FString LoadedSeed;
	FDirtbagPlayerState LoadedPlayer;
	std::vector<dirtbag::Legacy> LoadedLegacies;
	const EDirtbagLoadResult Result = DirtbagSaveIO::LoadGameFromFile(
	    SaveFilename, LoadedSeed, LoadedPlayer, LoadedLegacies);
	if (Result == EDirtbagLoadResult::Ok)
	{
		Seed = LoadedSeed;
		Player = LoadedPlayer;
		Legacies = LoadedLegacies;
		bLoadedFromSave = true;
	}
	// Anything else — missing, garbage, future — starts fresh; the bad file
	// is left on disk untouched for post-mortems, never overwritten silently
	// until the next sleep.
	else
	{
		// A fresh valley hands you a body, not a template. The mirror's
		// defaults are flat 50s and Average — the same climber in every
		// world — and NewClimber draws skills and morphology from the world
		// seed, so which lines suit you differs before a day is played.
		Player.Climber = DirtbagConvert::FromSim(dirtbag::NewClimber(
		    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed))));
	}

	Day = UDirtbagSimLibrary::WakeUp(Player);
}

bool UDirtbagGameInstance::EatMeal()
{
	return UDirtbagSimLibrary::EatMeal(Player, Day);
}

void UDirtbagGameInstance::PassHours(double Hours)
{
	UDirtbagSimLibrary::PassHours(Day, Hours);
}

void UDirtbagGameInstance::EnsureAtGym()
{
	if (!Day.bAtGym)
	{
		UDirtbagSimLibrary::StartGymSession(Player, Day);
	}
}

void UDirtbagGameInstance::Sleep()
{
	// The Lot's day resolves before yours ends, so waking to "Dev got the
	// arete" is news about yesterday rather than a thing that happened while
	// you were asleep in the same field as him.
	AdvanceTheLot();

	// The dog's day too: hungrier by one, and closer to you unless you
	// spent the day at a desk it could not come to.
	dirtbag::Dog SimDog = DirtbagConvert::ToSim(Player.Dog);
	dirtbag::DogDay(SimDog, !bWorkedToday);
	Player.Dog = DirtbagConvert::FromSim(SimDog);
	bWorkedToday = false;
	DogWorry.Reset();
	VanNews.Reset();
	SponsorNews.Reset();
	// Whoever stood at the bottom of the rope yesterday is fresh again.
	RopedBurnsToday = 0;
	// Yesterday's first ascent stops being news. It is in the book now,
	// which is where a thing you did goes once it stops being a moment.
	LastAscentLine.Reset();
	DirtbagYearNews.Reset();
	CrewNews.Reset();
	DreamNews.Reset();
	const bool bHadACrewName = !Player.Crew.Name.IsEmpty();

	// SleepToNextDay ticks the streak, so a year completing is visible as
	// the count going up across the call. Reading the count rather than
	// plumbing a return value through the Blueprint library keeps the news
	// here, where every other piece of overnight news already lives.
	const int32 YearsBefore = Player.Job.DirtbagYears;

	// The sim's copy of who you are, kept current before CrewDay runs
	// inside SleepToNextDay — the crew hash includes your name, so the sync
	// has to happen on this side of the call or the town would name
	// generation two after generation one.
	Player.Name = ClimberName;

	UDirtbagSimLibrary::SleepToNextDay(Seed, Player, Day);

	// The high-water mark, once a day.
	//
	// **This had never been written.** `PeakGradeEver` is read by
	// TimeToThinkAboutIt -- the one opinion this game ever offers about
	// stopping -- and set nowhere but the reset in RetireAndPassItOn, so it
	// was a permanent 0.0 and the "two grades off your best" arm of that
	// function could never fire. The whole retirement hint has been running
	// on the injury arm alone, and the injury arm is fed
	// ConsecutiveInjuries, which was never written either.
	//
	// Written-and-never-wired at a sixth layer, and the nastiest kind: not
	// a dead feature, but a **live function silently answering from
	// constants.** No checker in this project could see it -- they prove
	// declarations are reachable, not that fields are ever assigned.
	//
	// Stored rather than derived because it is a historical maximum: you
	// cannot recompute the best you ever were from the state of the body
	// that is left. In ability-grade units, because that is what
	// TimeToThinkAboutIt compares it against.
	const double AbilityToday = GetCareer().AbilityGrade;
	if (AbilityToday > PeakGradeEver)
	{
		PeakGradeEver = AbilityToday;
	}

	// And the streak of bodies giving up. Counted on the day an injury
	// starts rather than every day you are hurt -- three injuries in a row
	// is the signal, not one injury lasting three months.
	const bool bHurtToday = IsHurt();
	if (bHurtToday && !bWasHurtYesterday)
	{
		ConsecutiveInjuries++;
	}
	else if (!bHurtToday)
	{
		// Reset by a clean season, not by a clean day: healing from one
		// injury must not wipe the count that says your body keeps
		// breaking.
		DaysSinceHurt++;
		if (DaysSinceHurt >= 90)
		{
			ConsecutiveInjuries = 0;
		}
	}
	if (bHurtToday)
	{
		DaysSinceHurt = 0;
	}
	bWasHurtYesterday = bHurtToday;

	if (!bHadACrewName && !Player.Crew.Name.IsEmpty())
	{
		// Not "you are now called" — nobody announces a nickname to your
		// face. You hear it secondhand, which is how they always arrive.
		CrewNews = FString::Printf(
		    TEXT("You heard somebody at the fire call you %s."),
		    *Player.Crew.Name);
	}

	if (Player.Job.DirtbagYears > YearsBefore)
	{
		DirtbagYearNews =
		    Player.Job.DirtbagYears == 1
		        ? TEXT("A year today, and nobody has owned an hour of it.")
		        : FString::Printf(
		              TEXT("%d Dirtbag Years. Still nobody's."),
		              Player.Job.DirtbagYears);
	}

	// The salary owns its days whether or not you wanted them, and this is
	// where a day begins — SleepToNextDay ends by resetting the DayState to
	// the wake hour, so the new day starts here and the job takes it here.
	//
	// Until now the engine could ask SalariedToday() and had no way to work
	// one: the salaried-job trap, a named Phase 3 mechanic, existed only in
	// the probe. Doing it at dawn rather than offering it as an action is
	// the entire design — nine to five means the clock arrives at the far
	// side of the day having skipped everything the day was for, and a trap
	// you can decline is not a trap.
	if (SalariedToday())
	{
		WorkSalariedDay();
	}

	// The sponsor's side of the bargain. Both halves ran nowhere before
	// this: the stipend was never paid, so the $640-a-month title tier paid
	// $0, and the review was never run — not by the engine and not even by
	// the probe — so no rung was ever won or lost by anybody.
	//
	// Monthly rather than daily, because a stipend is a monthly thing and
	// because the offer check is already monthly. Day 1 is excluded: they
	// do not pay you for the day you signed.
	if (Player.Sponsor.Tier != EDirtbagSponsorTier::None &&
	    Player.Day > 1 && Player.Day % 30 == 1)
	{
		const double Paid = dirtbag::MonthlyStipend(
		    static_cast<dirtbag::SponsorTier>(Player.Sponsor.Tier));
		if (Paid > 0.0)
		{
			dirtbag::PlayerState Wallet = DirtbagConvert::ToSim(Player);
			dirtbag::Pay(Wallet, Paid);
			Player.Cash = Wallet.cash;
			Player.Owed = Wallet.owed;
			SponsorNews = FString::Printf(TEXT("%s paid: $%.0f."),
			                              *SponsorLine(), Paid);
		}
	}

	// And once a year they look at what you have actually done. Being hurt
	// pauses that clock rather than running it, which is why the day count
	// this reads had to become saved state.
	if (Player.Sponsor.Tier != EDirtbagSponsorTier::None &&
	    Player.Day > 1 && Player.Day % 365 == 1)
	{
		dirtbag::Sponsorship Deal = DirtbagConvert::ToSim(Player.Sponsor);
		const EDirtbagSponsorTier Was = Player.Sponsor.Tier;
		const dirtbag::SponsorTier Now = dirtbag::ReviewSeason(
		    Deal, GetCareer().HardestSendGrade, Deal.daysHurtThisSeason);
		Deal.daysHurtThisSeason = 0;
		Player.Sponsor = DirtbagConvert::FromSim(Deal);

		SponsorNews =
		    static_cast<EDirtbagSponsorTier>(Now) < Was
		        ? FString::Printf(TEXT("They stopped returning calls. %s"),
		                          *SponsorLine())
		        : FString::Printf(TEXT("They are keeping you on. %s"),
		                          *SponsorLine());
	}

	// Whether they own the day you have just woken into.
	//
	// **This is the entire mechanic and nothing was asking it.**
	// `SponsorOwnsToday` had no caller, so a deal paid $640 a month and
	// cost nothing at all -- the one money in this game whose price is
	// good days was, in the played game, free money.
	//
	// Taken at Sleep rather than offered as a choice, because the sim's
	// own header is unambiguous about what the trade is: *"Obligation days
	// a month, and they land on days with a window. This is the entire
	// mechanic: money that costs you the good days rather than the spare
	// ones."* The decision was made when you signed. You wake up and half
	// the day already belongs to somebody, which is what being sponsored
	// is.
	if (SponsorOwnsToday())
	{
		const dirtbag::SponsorDials Sp;
		PassHours(Sp.obligationHours);
		Day.Energy = FMath::Max(0.0, Day.Energy - Sp.obligationEnergy);
		// Appended rather than assigned: the money, the review and a shoot
		// can all land on the same morning, and overwriting would lose the
		// one that mattered.
		const FString Shoot = FString::Printf(
		    TEXT("They want you today. %.0f hours of standing on the same "
		         "move while somebody changes a lens."),
		    Sp.obligationHours);
		SponsorNews = SponsorNews.IsEmpty()
		                  ? Shoot
		                  : SponsorNews + TEXT("  ") + Shoot;
	}

	// And whether anybody put it together while you slept. This lives in
	// Sleep rather than being a call the day loop remembers, because five
	// separate per-day ticks have now been written and left uncalled in this
	// project — KitDay, the body roll, FactionDay, the legacy save path.
	// Anything that happens overnight happens here.
	EthicsNews = DoesAnybodyFindOutToday();

	SaveNow();
}

bool UDirtbagGameInstance::SaveNow()
{
	// The complete path: anything that drops Legacies here disinherits every
	// generation before this one, and nothing about the save would look
	// wrong until somebody opened the guidebook.
	return DirtbagSaveIO::SaveGameToFile(Seed, Player, Legacies, SaveFilename);
}

void UDirtbagGameInstance::EnsureBoard()
{
	if (Board.Num() == 0)
	{
		Board = UDirtbagSimLibrary::GymBoard(Seed);
	}
}

TArray<FDirtbagRoute> UDirtbagGameInstance::GetBoard()
{
	EnsureBoard();
	return Board;
}

FDirtbagRoute UDirtbagGameInstance::GetBoardRoute(int32 Index)
{
	return GetRouteAt(Venue, Index);
}

FDirtbagRoute UDirtbagGameInstance::GetRouteAt(EDirtbagVenue AtVenue,
                                               int32 Index)
{
	if (IsOutdoors(AtVenue))
	{
		FDirtbagRoute Route = GetCragLineAt(AtVenue, Index).Route;
		// A chipped hold is the one shortcut that changes the rock, so it
		// has to change the route rather than the ledger -- and it has to
		// keep changing it for good, for everyone, forever.
		//
		// Applied here because this is the single funnel every caller goes
		// through, and derived from the secret rather than stored on the
		// crag because **the crag is regenerated from the seed on every
		// load** and a mutation written into it would evaporate at the
		// next save. The secret persists; the rock follows from it.
		//
		// The send stands when this comes out (StripsTheAscent is false
		// for chipping) which is right: it happened, on a line that is no
		// longer what it was.
		for (const FDirtbagSecret& S : Player.Secrets)
		{
			if (S.Act == EDirtbagEthicalAct::ChippedAHold &&
			    S.RouteKey == Route.Name)
			{
				Route.TrueGrade = FMath::Max(0, Route.TrueGrade - 1);
			}
		}
		return Route;
	}
	EnsureBoard();
	if (Board.Num() == 0)
	{
		return FDirtbagRoute();
	}
	return Board[FMath::Clamp(Index, 0, Board.Num() - 1)];
}

void UDirtbagGameInstance::EnsureCrag()
{
	// The venue decides which rock. Arriving at the cave with Roadside still
	// loaded would serve boulder lines at a rope crag and compute the window
	// for the wrong aspect -- east-facing shade on a north-facing wall.
	// Which rock. Written as a switch rather than a chain of ternaries so
	// that a fourth venue cannot quietly fall through to Roadside the way a
	// `== Cave ? Cave : Crag` test would have.
	EDirtbagVenue Want = EDirtbagVenue::Crag;
	if (Venue == EDirtbagVenue::Cave) Want = EDirtbagVenue::Cave;
	else if (Venue == EDirtbagVenue::Terrace) Want = EDirtbagVenue::Terrace;
	if (bCragLoaded && LoadedCrag == Want)
	{
		return;
	}
	LoadedCrag = Want;
	Crag = Want == EDirtbagVenue::Cave
	           ? UDirtbagSimLibrary::ShadedCave(Seed)
	           : Want == EDirtbagVenue::Terrace
	                 ? UDirtbagSimLibrary::SunTerrace(Seed)
	                 : UDirtbagSimLibrary::RoadsideCrag(Seed);
	bCragLoaded = true;

	// Seed a ledger for every unclimbed line, at the filth it is actually
	// in. This has to happen on load rather than on first touch: every
	// attempt path creates its own ledger through MemoryFor if one is
	// missing, and that one would be clean — so walking up to a virgin line
	// and pulling on it would climb it as if somebody had already spent a
	// day brushing it, and the whole mechanic would be silently absent.
	for (const FDirtbagCragLine& Line : Crag.Lines)
	{
		if (!Line.bIsProject)
		{
			continue;   // everything in the book is clean; people climb it
		}
		bool bKnown = false;
		for (const FDirtbagProjectMemory& M : Player.Projects)
		{
			if (M.RouteName == Line.Route.Name)
			{
				bKnown = true;
				break;
			}
		}
		if (!bKnown)
		{
			dirtbag::CragLine SimLine;
			SimLine.route.name = TCHAR_TO_UTF8(*Line.Route.Name);
			SimLine.route.grade = Line.Route.Grade;
			SimLine.route.discipline =
			    static_cast<dirtbag::Discipline>(Line.Route.Discipline);
			SimLine.isProject = true;
			Player.Projects.Add(
			    DirtbagConvert::FromSim(dirtbag::NewProjectLedger(SimLine)));
		}
	}

	// Put the player's own ascents back on the page. Nothing about the crag
	// is saved — it is regenerated from the world seed every time the venue
	// changes — so without this a line you named reverts to a nameless
	// project the moment you walk to the cave and back.
	// Everybody who came before, then the person standing in it. Order
	// matters only in the sense that the living player's ledger should win
	// a tie, and it cannot: a line a predecessor named is no longer an open
	// project, so CanName refuses it.
	UDirtbagSimLibrary::WriteLegaciesIntoTheBook(Crag, Legacies);
	UDirtbagSimLibrary::WriteTheLotIntoTheBook(Crag, Player);
	UDirtbagSimLibrary::WriteIntoTheBook(Crag, Player, AscentSignature());

	// The guidebook owns which way its rock faces. Keeping a second copy of
	// that on the game instance is how a crag ends up climbing in one
	// aspect's shade while its window is computed for another.
	CragAspect = Crag.Aspect;
	CachedWindowDay = -1;
}

FDirtbagCrag UDirtbagGameInstance::GetCrag()
{
	EnsureCrag();
	return Crag;
}

FDirtbagCragLine UDirtbagGameInstance::GetCragLineAt(EDirtbagVenue AtVenue,
                                                     int32 Index)
{
	// Asking for one venue's rock while standing at another is legitimate —
	// a wall resolves its own line in BeginPlay, before anybody has arrived
	// anywhere — so this loads what was asked for rather than what is
	// underfoot, and puts back what was there.
	const EDirtbagVenue Standing = Venue;
	Venue = AtVenue;
	EnsureCrag();
	const FDirtbagCragLine Line =
	    Crag.Lines.Num() == 0
	        ? FDirtbagCragLine()
	        : Crag.Lines[FMath::Clamp(Index, 0, Crag.Lines.Num() - 1)];

	// Put the rock back. Restoring only the venue would leave the cave
	// loaded and CragAspect pointing north while the player stands at
	// east-facing Roadside — a crag climbing in one aspect's shade while its
	// window is computed for another, which is the exact failure EnsureCrag
	// exists to prevent.
	Venue = Standing;
	EnsureCrag();
	return Line;
}

double UDirtbagGameInstance::ApproachHoursFor(EDirtbagVenue AtVenue)
{
	if (!IsOutdoors(AtVenue))
	{
		return -1.0;   // no rock, no approach; the spot's own number stands
	}
	// Same save-and-restore as GetCragLineAt, and for the same reason: a
	// travel spot asks about the far end of the drive while the player is
	// still standing at this one, and leaving the wrong crag loaded would
	// point CragAspect at rock the player is nowhere near.
	const EDirtbagVenue Standing = Venue;
	Venue = AtVenue;
	EnsureCrag();
	const double Hours = Crag.ApproachHours;
	Venue = Standing;
	EnsureCrag();
	return Hours;
}

FDirtbagCragLine UDirtbagGameInstance::GetCragLine(int32 Index)
{
	EnsureCrag();
	if (Crag.Lines.Num() == 0)
	{
		return FDirtbagCragLine();
	}
	return Crag.Lines[FMath::Clamp(Index, 0, Crag.Lines.Num() - 1)];
}

int32 UDirtbagGameInstance::NumRoutesHere()
{
	if (!bIndoors)
	{
		EnsureCrag();
		return Crag.Lines.Num();
	}
	EnsureBoard();
	return Board.Num();
}

FDirtbagAttemptResult UDirtbagGameInstance::ReplayAttempt(
    const FDirtbagRoute& Route)
{
	EnsureAtGym();
	if (!bIndoors)
	{
		EnsureCrag();   // so a project's ledger exists, and is filthy
	}
	bClimbedToday = true;
	return UDirtbagSimLibrary::DayAttempt(TodaysSessionSeed(), Player, Day,
	                                      Route, CurrentFriction());
}

FDirtbagCareerSummary UDirtbagGameInstance::GetCareer() const
{
	return UDirtbagSimLibrary::SummarizeCareer(Player);
}

FString UDirtbagGameInstance::GetCareerLine() const
{
	return UDirtbagSimLibrary::CareerLine(GetCareer());
}

int32 UDirtbagGameInstance::AttemptsOn(const FDirtbagRoute& Route) const
{
	for (const FDirtbagProjectMemory& M : Player.Projects)
	{
		if (M.RouteName == Route.Name)
		{
			return M.Attempts;
		}
	}
	return 0;
}

dirtbag::LiveAttempt UDirtbagGameInstance::BeginLiveFor(
    const FDirtbagRoute& Route)
{
	EnsureAtGym();
	if (!bIndoors)
	{
		EnsureCrag();
	}
	bClimbedToday = true;
	UDirtbagLiveAttempt* Attempt = UDirtbagSimLibrary::BeginDayLiveAttempt(
	    TodaysSessionSeed(), Player, Day, Route, CurrentFriction());
	return Attempt->Live;
}

FDirtbagAttemptResult UDirtbagGameInstance::CommitLiveFor(
    const dirtbag::LiveAttempt& Live)
{
	const dirtbag::AttemptResult Result = dirtbag::FinishAttempt(Live);
	const dirtbag::Route& SimRoute = Live.input.route;
	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);

	dirtbag::CommitAttempt(SimDay.session,
	                       dirtbag::MemoryFor(SimPlayer, SimRoute), SimRoute,
	                       Result);
	dirtbag::ApplyAttemptToDay(SimPlayer, SimDay, SimRoute, Result, Live.rng);

	Player = DirtbagConvert::FromSim(SimPlayer);
	Day = DirtbagConvert::FromSim(SimDay);
	return DirtbagConvert::FromSim(Result);
}

FString UDirtbagGameInstance::TodaysSessionSeed() const
{
	return FString::Printf(TEXT("%s#day%d"), *Seed, Player.Day);
}

// --- Conditions --------------------------------------------------------------

FDirtbagWeather UDirtbagGameInstance::TodaysWeather() const
{
	if (CachedWeatherDay != Player.Day)
	{
		CachedWeather = UDirtbagSimLibrary::WeatherFor(Seed, Player.Day);
		CachedWeatherDay = Player.Day;
	}
	return CachedWeather;
}

FDirtbagPrimeWindow UDirtbagGameInstance::TodaysWindow() const
{
	const FDirtbagWeather Weather = TodaysWeather();
	if (CachedWindowDay != Player.Day || CachedWindowAspect != CragAspect)
	{
		CachedWindow = UDirtbagSimLibrary::PrimeWindowFor(Weather, CragAspect);
		CachedWindowDay = Player.Day;
		CachedWindowAspect = CragAspect;
	}
	return CachedWindow;
}

void UDirtbagGameInstance::Rest(double Hours)
{
	dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);
	dirtbag::Rest(SimDay, Hours);
	Day = DirtbagConvert::FromSim(SimDay);
}

double UDirtbagGameInstance::HoursUntilWindow()
{
	if (bIndoors)
	{
		return 0.0;   // a gym is always as good as it gets
	}
	const FDirtbagPrimeWindow Window = TodaysWindow();
	if (!Window.bExists || Day.Hour >= Window.StartHour)
	{
		return 0.0;   // open, missed, or never coming
	}
	return Window.StartHour - Day.Hour;
}

FString UDirtbagGameInstance::WaitAdvice()
{
	if (bIndoors)
	{
		return TEXT("Nothing to wait for. It is a gym.");
	}
	const FDirtbagPrimeWindow Window = TodaysWindow();
	if (!Window.bExists)
	{
		return TEXT("Today is not going to come good. Go and do something "
		            "else.");
	}
	if (Day.Hour > Window.EndHour)
	{
		return TEXT("You missed it. It was better an hour ago and it will be "
		            "better tomorrow.");
	}
	if (Day.Hour >= Window.StartHour)
	{
		return TEXT("It is on, right now.");
	}
	const double Wait = Window.StartHour - Day.Hour;
	return FString::Printf(TEXT("The rock comes good in %.0f minutes."),
	                       Wait * 60.0);
}

void UDirtbagGameInstance::SetVenue(EDirtbagVenue NewVenue)
{
	if (Venue == NewVenue)
	{
		return;
	}
	Venue = NewVenue;
	bIndoors = (NewVenue == EDirtbagVenue::Gym);

	// The window is computed per aspect, and arriving somewhere changes
	// which aspect applies.
	CachedWindowDay = -1;

	if (!bIndoors)
	{
		EnsureCrag();   // seeds the project ledgers, filthy, before any burn
	}
}

double UDirtbagGameInstance::CurrentFriction() const
{
	// A gym has no shade line: the whole mechanic is an outdoor one, and
	// pretending otherwise would make the Phase 1 gym behave differently
	// after this change for no reason the player could read.
	if (bIndoors)
	{
		return IndoorFriction;
	}
	return UDirtbagSimLibrary::FrictionAt(TodaysWeather(), CragAspect,
	                                      Day.Hour);
}

FString UDirtbagGameInstance::ConditionsLine() const
{
	if (bIndoors)
	{
		return TEXT("indoors - the holds are exactly as good as they ever are");
	}
	const FDirtbagWeather Weather = TodaysWeather();
	const double Friction = CurrentFriction();
	return FString::Printf(
	    TEXT("%s  -  %.0fF on the rock, %.0f%% humidity.  %s"),
	    *UDirtbagSimLibrary::ConditionsText(Friction),
	    UDirtbagSimLibrary::RockTempF(Weather, CragAspect, Day.Hour),
	    Weather.Humidity * 100.0,
	    *UDirtbagSimLibrary::WindowText(TodaysWindow()));
}

// --- First ascents -----------------------------------------------------------

FString UDirtbagGameInstance::AscentSignature() const
{
	return ClimberName.IsEmpty() ? FString(TEXT("you")) : ClimberName;
}

FDirtbagProjectMemory* UDirtbagGameInstance::LedgerFor(int32 BoardIndex)
{
	// Cleaning and naming only happen while you are standing at the wall,
	// so the live venue is the right one here — unlike route resolution,
	// which happens before anyone has arrived anywhere.
	const FDirtbagRoute Route = GetRouteAt(Venue, BoardIndex);
	if (Route.Name.IsEmpty())
	{
		return nullptr;
	}
	for (FDirtbagProjectMemory& M : Player.Projects)
	{
		if (M.RouteName == Route.Name)
		{
			return &M;
		}
	}

	// First touch of a line that is already in a book: clean, because people
	// climb it. Projects never reach here — EnsureCrag has already seeded
	// them filthy, so that no attempt path can invent a clean one first.
	dirtbag::CragLine SimLine;
	SimLine.route.name = TCHAR_TO_UTF8(*Route.Name);
	SimLine.route.grade = Route.Grade;
	SimLine.isProject = false;
	Player.Projects.Add(
	    DirtbagConvert::FromSim(dirtbag::NewProjectLedger(SimLine)));
	return &Player.Projects.Last();
}

double UDirtbagGameInstance::CleanLine(int32 BoardIndex, double Hours)
{
	FDirtbagProjectMemory* Ledger = LedgerFor(BoardIndex);
	if (!Ledger)
	{
		return 0.0;
	}
	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);
	dirtbag::ProjectMemory SimLedger = DirtbagConvert::ToSim(*Ledger);

	const double Gained =
	    dirtbag::CleanLine(SimPlayer, SimDay, SimLedger, Hours);

	// Write the ledger back before the player, or converting the player
	// would overwrite the row we just changed.
	*Ledger = DirtbagConvert::FromSim(SimLedger);
	Day = DirtbagConvert::FromSim(SimDay);
	return Gained;
}

FString UDirtbagGameInstance::ClaimText(int32 BoardIndex, double GainedThisPress)
{
	const FDirtbagProjectMemory* Ledger = LedgerFor(BoardIndex);
	if (!Ledger || Ledger->bFirstAscent)
	{
		return FString();
	}
	// Only at the crossing. Below the threshold you have tickled the line;
	// crossing it is somebody walking past, seeing the chalk gone and the
	// holds bare, and drawing the obvious conclusion.
	//
	// No project test is needed and none would be safe: an established line
	// starts at cleanliness 1.0, so `Was` is already above the threshold and
	// this returns empty for everything in the book. That is the same
	// default that made the bare `> 0.2` in SpokenFor fragile, working for
	// us here rather than against us.
	const dirtbag::PartnerDials Dials;
	const double Was = Ledger->Cleanliness - GainedThisPress;
	if (Was > Dials.brushedEnoughToBeYours ||
	    Ledger->Cleanliness <= Dials.brushedEnoughToBeYours)
	{
		return FString();
	}
	return TEXT("Word gets round. Nobody at the Lot will touch it now.");
}

bool UDirtbagGameInstance::IsWorkable(int32 BoardIndex)
{
	const FDirtbagProjectMemory* Ledger = LedgerFor(BoardIndex);
	return Ledger ? dirtbag::IsWorkable(DirtbagConvert::ToSim(*Ledger)) : true;
}

FString UDirtbagGameInstance::CleanlinessText(int32 BoardIndex)
{
	const FDirtbagProjectMemory* Ledger = LedgerFor(BoardIndex);
	if (!Ledger)
	{
		return FString();
	}
	return FString(UTF8_TO_TCHAR(
	    dirtbag::CleanlinessText(DirtbagConvert::ToSim(*Ledger)).c_str()));
}

bool UDirtbagGameInstance::CanNameLine(int32 BoardIndex)
{
	if (bIndoors)
	{
		return false;   // nobody names a gym problem; the setter already did
	}
	const FDirtbagProjectMemory* Ledger = LedgerFor(BoardIndex);
	if (!Ledger)
	{
		return false;
	}
	const FDirtbagCragLine Line = GetCragLine(BoardIndex);
	dirtbag::CragLine SimLine;
	SimLine.isProject = Line.bIsProject;
	SimLine.firstAscentBy = TCHAR_TO_UTF8(*Line.FirstAscentBy);
	return dirtbag::CanName(SimLine, DirtbagConvert::ToSim(*Ledger));
}

EDirtbagSessionAdvice UDirtbagGameInstance::ReadSession() const
{
	return static_cast<EDirtbagSessionAdvice>(
	    dirtbag::ReadSession(DirtbagConvert::ToSim(Day.Session),
	                         DirtbagConvert::ToSim(Player.Climber)));
}

FString UDirtbagGameInstance::SessionAdviceText() const
{
	return FString(UTF8_TO_TCHAR(dirtbag::SessionAdviceText(
	    dirtbag::ReadSession(DirtbagConvert::ToSim(Day.Session),
	                         DirtbagConvert::ToSim(Player.Climber)))));
}

// --- Gear and the van --------------------------------------------------------

double UDirtbagGameInstance::ShoeCostInGrades() const
{
	// Edging, because that is the hold dead rubber punishes hardest and so
	// the number a player would actually notice.
	return dirtbag::ShoePenalty(DirtbagConvert::ToSim(Player.Shoes), true);
}

FString UDirtbagGameInstance::ShoeLine() const
{
	return FString(UTF8_TO_TCHAR(
	    dirtbag::ShoeText(DirtbagConvert::ToSim(Player.Shoes)).c_str()));
}

// --- Ethics ------------------------------------------------------------------

void UDirtbagGameInstance::DoSomethingYouWouldNotAdmitTo(
    EDirtbagEthicalAct Act, const FString& OnRoute)
{
	Player.Secrets.Add(DirtbagConvert::FromSim(dirtbag::Commit(
	    static_cast<dirtbag::EthicalAct>(Act), TCHAR_TO_UTF8(*OnRoute),
	    Player.Day)));
}

bool UDirtbagGameInstance::CanTakeShortcut(EDirtbagEthicalAct Act,
                                          int32 BoardIndex)
{
	// Plastic has no ethics worth the name. You cannot chisel a resin
	// hold, nobody bolts a gym, and claiming a gym problem is not a lie
	// anybody in this game would tell.
	if (!IsOutdoors(Venue))
	{
		return false;
	}
	const FDirtbagProjectMemory* Ledger = nullptr;
	const FDirtbagRoute Route = GetRouteAt(Venue, BoardIndex);
	for (const FDirtbagProjectMemory& M : Player.Projects)
	{
		if (M.RouteName == Route.Name)
		{
			Ledger = &M;
			break;
		}
	}
	// Nothing to gain from a line you have already done.
	if (Ledger && Ledger->bSent)
	{
		return false;
	}

	switch (Act)
	{
	case EDirtbagEthicalAct::ChippedAHold:
		return true;
	case EDirtbagEthicalAct::ClaimedASend:
		return true;
	case EDirtbagEthicalAct::PulledOnGear:
		// "One hang nobody saw" needs you to have been on it. Claiming a
		// line you never tied into is the other act, and it costs more
		// when it comes out for exactly that reason.
		return Ledger && Ledger->Attempts > 0;
	case EDirtbagEthicalAct::StagedAPhoto:
		// Unblocked the day sponsorship got a door. There is nothing to
		// stage a shot *for* without somebody paying for it, and until now
		// there was no way to have a sponsor at all -- which is why this
		// act sat unoffered with the reason written where it would be
		// read.
		//
		// Only worth doing when they are about to notice you have stopped
		// climbing. A photo of a send that did not happen is a thing you
		// do the season before a review goes badly, not a thing you do for
		// fun.
		return Player.Sponsor.Tier != EDirtbagSponsorTier::None &&
		       Player.Sponsor.SeasonsWithoutProgress > 0;
	default:
		// **RetroBolted is deliberately not offered**, and saying so here
		// is better than offering an act that does nothing -- which would
		// be this project's own favourite bug wearing a new hat.
		//
		// It buys less runout, and runout is computed in `RunoutAt` rather
		// than carried on the route, so its benefit needs plumbing that
		// does not exist yet. A real act with a real cost dial (0.6)
		// waiting for it.
		return false;
	}
}

FString UDirtbagGameInstance::TakeShortcut(EDirtbagEthicalAct Act,
                                           int32 BoardIndex)
{
	if (!CanTakeShortcut(Act, BoardIndex))
	{
		return FString();
	}
	const FDirtbagRoute Route = GetRouteAt(Venue, BoardIndex);
	DoSomethingYouWouldNotAdmitTo(Act, Route.Name);

	// The benefit, and it is deliberately the exact mirror of what
	// stripping takes back: claiming and pulling on both write a send into
	// the ledger, and the day it comes out that send is what goes.
	if (Act == EDirtbagEthicalAct::ClaimedASend ||
	    Act == EDirtbagEthicalAct::PulledOnGear)
	{
		if (FDirtbagProjectMemory* Ledger = LedgerFor(BoardIndex))
		{
			Ledger->bSent = true;
		}
	}
	// A staged shot buys exactly one thing: the sponsor believes you have
	// been climbing. The review counts seasons without progress, so the
	// benefit is that clock going back to zero -- you did not send
	// anything, and they think you did.
	//
	// Deliberately not a fake send in the ledger. The lie is told to the
	// sponsor rather than to the book, which is why its Secret carries no
	// route key and why StripsTheAscent finds nothing to take: the ascent
	// never existed to be taken.
	if (Act == EDirtbagEthicalAct::StagedAPhoto)
	{
		Player.Sponsor.SeasonsWithoutProgress = 0;
	}

	// Chipping needs nothing here. Its benefit is applied in GetRouteAt,
	// derived from the secret, because the rock has to stay changed across
	// a save and the crag is rebuilt from the seed every load.

	switch (Act)
	{
	case EDirtbagEthicalAct::ChippedAHold:
		return FString::Printf(
		    TEXT("It goes now. It did not before, and it never will "
		         "again."));
	case EDirtbagEthicalAct::ClaimedASend:
		return FString::Printf(TEXT("%s. Ticked. Nobody was there."),
		                       *Route.Name);
	case EDirtbagEthicalAct::StagedAPhoto:
		return FString(
		    TEXT("Three moves up, hanging on the rope between shots. It "
		         "will look like the top."));
	default:
		return FString(
		    TEXT("One hang. Nobody saw it. You write it down clean."));
	}
}

double UDirtbagGameInstance::HowWatchedYouAre() const
{
	return dirtbag::VisibilityFrom(
	    DirtbagConvert::ToSim(Player.Standing),
	    static_cast<dirtbag::SponsorTier>(Player.Sponsor.Tier));
}

int32 UDirtbagGameInstance::ThingsNobodyKnows() const
{
	int32 N = 0;
	for (const FDirtbagSecret& S : Player.Secrets)
	{
		if (!S.bKnown) N++;
	}
	return N;
}

FString UDirtbagGameInstance::DoesAnybodyFindOutToday()
{
	std::vector<dirtbag::Secret> Secrets;
	Secrets.reserve(Player.Secrets.Num());
	for (const FDirtbagSecret& S : Player.Secrets)
	{
		Secrets.push_back(DirtbagConvert::ToSim(S));
	}

	const int Found = dirtbag::SomebodyFindsOut(
	    Secrets, dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)), Player.Day,
	    HowWatchedYouAre());
	if (Found < 0)
	{
		return FString();
	}

	dirtbag::Standing SimStanding = DirtbagConvert::ToSim(Player.Standing);
	double Psyche = Player.Climber.Psyche;
	dirtbag::ItComesOut(Secrets[Found], SimStanding, Psyche);
	Player.Standing = DirtbagConvert::FromSim(SimStanding);
	Player.Climber.Psyche = Psyche;

	// The ascent goes with it, when the lie was about what happened rather
	// than about the rock. This is what gives the whole system teeth: your
	// hardest send vanishing cascades straight into a sponsor's next review.
	if (dirtbag::StripsTheAscent(Secrets[Found].act))
	{
		const FString Key = UTF8_TO_TCHAR(Secrets[Found].routeKey.c_str());
		for (FDirtbagProjectMemory& M : Player.Projects)
		{
			if (M.RouteName == Key)
			{
				M.bSent = false;
				M.bFirstAscent = false;
			}
		}
	}

	Player.Secrets[Found] = DirtbagConvert::FromSim(Secrets[Found]);
	return UTF8_TO_TCHAR(
	    dirtbag::EthicsText(Secrets[Found], Player.Day).c_str());
}

// --- Sponsorship -------------------------------------------------------------

namespace
{
	int CountFirstAscents(const FDirtbagPlayerState& Player)
	{
		int N = 0;
		for (const FDirtbagProjectMemory& M : Player.Projects)
		{
			if (M.bFirstAscent) N++;
		}
		return N;
	}
}

EDirtbagSponsorTier UDirtbagGameInstance::OfferOnTheTable() const
{
	const FDirtbagCareerSummary Career = GetCareer();
	return static_cast<EDirtbagSponsorTier>(dirtbag::OfferFor(
	    Career.HardestSendGrade, CountFirstAscents(Player),
	    DirtbagConvert::ToSim(Player.Standing)));
}

FString UDirtbagGameInstance::WhatTheyAreOffering() const
{
	const EDirtbagSponsorTier Offer = OfferOnTheTable();
	if (Offer <= Player.Sponsor.Tier)
	{
		return FString();
	}
	const dirtbag::SponsorDials Sp;
	const double Pays = dirtbag::MonthlyStipend(
	    static_cast<dirtbag::SponsorTier>(Offer));
	const int32 Days =
	    Offer == EDirtbagSponsorTier::Title   ? Sp.obligationDaysTitle
	    : Offer == EDirtbagSponsorTier::Gear  ? Sp.obligationDaysGear
	                                          : Sp.obligationDaysShoes;

	// Both halves, every time. An offer that says what it pays and not what
	// it wants is an advert, and the whole point of this system is that the
	// price is days rather than money.
	FString What = FString(
	    dirtbag::SponsorTierName(static_cast<dirtbag::SponsorTier>(Offer)));
	What += Pays > 0.0 ? FString::Printf(TEXT(" - $%.0f a month"), Pays)
	                   : FString(TEXT(" - free rubber"));
	What += Days > 0
	            ? FString::Printf(TEXT(", and %d day%s a month that will not "
	                                   "be the rainy ones"),
	                              Days, Days == 1 ? TEXT("") : TEXT("s"))
	            : FString(TEXT(", and they want nothing"));
	return What;
}

bool UDirtbagGameInstance::SignWithSponsor()
{
	const EDirtbagSponsorTier Offered = OfferOnTheTable();
	if (Offered <= Player.Sponsor.Tier) return false;

	dirtbag::Standing SimStanding = DirtbagConvert::ToSim(Player.Standing);
	dirtbag::SignedWith(static_cast<dirtbag::SponsorTier>(Offered),
	                    SimStanding);
	Player.Standing = DirtbagConvert::FromSim(SimStanding);

	Player.Sponsor.Tier = Offered;
	Player.Sponsor.GradeAtLastReview = GetCareer().HardestSendGrade;
	return true;
}

FString UDirtbagGameInstance::SponsorLine() const
{
	return UTF8_TO_TCHAR(
	    dirtbag::SponsorText(DirtbagConvert::ToSim(Player.Sponsor)).c_str());
}

bool UDirtbagGameInstance::SponsorOwnsToday() const
{
	return dirtbag::ObligationToday(
	    DirtbagConvert::ToSim(Player.Sponsor),
	    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)), Player.Day,
	    TodaysWindow().bExists);
}

// --- Retiring ----------------------------------------------------------------

bool UDirtbagGameInstance::TimeToThinkAboutIt() const
{
	return dirtbag::TimeToThinkAboutIt(DirtbagConvert::ToSim(Player),
	                                   ConsecutiveInjuries, PeakGradeEver);
}

FString UDirtbagGameInstance::CareerEpitaph() const
{
	const dirtbag::Legacy L = dirtbag::TallyCareer(
	    DirtbagConvert::ToSim(Player), TCHAR_TO_UTF8(*ClimberName),
	    1 + Player.Day / 365);
	return UTF8_TO_TCHAR(dirtbag::LegacyText(L).c_str());
}

bool UDirtbagGameInstance::ToggleGuidebook()
{
	// Indoors there is no book. A gym has a setter's tag on the start hold
	// and a grade somebody made up on Tuesday, and EnsureCrag falls through
	// to Roadside at the Gym -- so without this the "guidebook" would
	// cheerfully show you the lines at a crag forty minutes away.
	if (!IsOutdoors(Venue))
	{
		Guidebook.bActive = false;
		return false;
	}

	// Every spot and every wall binds G, and the Lot has four spots inside
	// a few metres of each other. Without this the key toggles once per
	// overlapping trigger, so standing between the van and the fire opens
	// and immediately closes the book -- which looks exactly like the key
	// not working.
	const uint64 Now = GFrameCounter;
	if (Now == GuidebookToggledOnFrame)
	{
		return Guidebook.bActive;
	}
	GuidebookToggledOnFrame = Now;

	Guidebook.bActive = !Guidebook.bActive;
	if (Guidebook.bActive)
	{
		SetGuidebookView(Guidebook.View);
	}
	return Guidebook.bActive;
}

void UDirtbagGameInstance::SetGuidebookView(EDirtbagGuidebookView View)
{
	EnsureCrag();

	FDirtbagGuidebookReadout& B = Guidebook;
	B.View = View;
	B.CragName = Crag.Name;
	B.Rows.Reset();
	B.Hidden = 0;
	B.SentHere = 0;
	B.LinesHere = Crag.Lines.Num();

	for (const FDirtbagCragLine& Line : Crag.Lines)
	{
		// The ledger is looked up rather than created: reading the book
		// must not seed a project entry for every line at the crag, which
		// LedgerFor would do and which would quietly bloat the save with
		// thirty-one untouched rows the moment somebody pressed G.
		const FDirtbagProjectMemory* Ledger = nullptr;
		for (const FDirtbagProjectMemory& M : Player.Projects)
		{
			if (M.RouteName == Line.Route.Name)
			{
				Ledger = &M;
				break;
			}
		}
		const bool bSent = Ledger && Ledger->bSent;
		if (bSent)
		{
			B.SentHere++;
		}

		// Filters. Both come from the sim's own guidebook helpers in
		// spirit -- Projects is OpenProjects and InReach is LinesUpTo --
		// applied here because the engine holds the mirrored crag and
		// converting the whole thing back to sim types to filter it would
		// cost more than the filter.
		bool bShow = true;
		if (View == EDirtbagGuidebookView::Projects)
		{
			bShow = Line.bIsProject;
		}
		else if (View == EDirtbagGuidebookView::InReach)
		{
			// What you have actually climbed, not what you might. A book
			// that shows you everything you could theoretically do is the
			// book you already have.
			bShow = Line.Route.Grade <= FMath::RoundToInt(PeakGradeEver);
		}
		if (!bShow)
		{
			B.Hidden++;
			continue;
		}

		FDirtbagGuidebookRow Row;
		Row.bProject = Line.bIsProject;
		Row.bSent = bSent;
		Row.FirstAscent = Line.FirstAscentBy;

		// The entry, as the book would read it aloud.
		if (Line.bIsProject)
		{
			Row.Entry = FString::Printf(TEXT("project — %s"),
			                            *Line.Description);
		}
		else
		{
			Row.Entry = FString::Printf(
			    TEXT("%s   %s"), *Line.DisplayName,
			    *UDirtbagSimLibrary::GradeName(Line.Route.Grade,
			                                   Line.Route.Discipline));
			for (int32 i = 0; i < Line.Stars; i++)
			{
				Row.Entry += TEXT("*");
			}
		}

		// And what you have done on it. This is the whole reason the page
		// is worth drawing: the burn count has been tracked per line since
		// Phase 1 and has never been visible anywhere but the wall you
		// were standing at.
		if (bSent)
		{
			Row.Yours = Ledger->Attempts > 1
			                ? FString::Printf(TEXT("done, %d burns"),
			                                  Ledger->Attempts)
			                : FString(TEXT("done, first go"));
		}
		else if (Ledger && Ledger->Attempts > 0)
		{
			Row.Yours = FString::Printf(
			    TEXT("%d burn%s, high point %d of %d"), Ledger->Attempts,
			    Ledger->Attempts == 1 ? TEXT("") : TEXT("s"),
			    Ledger->BestHighpoint, Line.Route.Moves.Num());
		}
		else if (Line.bIsProject)
		{
			// A project's obstacle is the dirt, so that is what its line
			// says when you have not touched it.
			Row.Yours = Ledger && Ledger->Cleanliness < 0.99
			                ? FString::Printf(TEXT("%.0f%% cleaned"),
			                                  Ledger->Cleanliness * 100.0)
			                : FString(TEXT("filthy"));
		}
		B.Rows.Add(Row);
	}
}

TArray<FString> UDirtbagGameInstance::WhoCouldTurnUp() const
{
	TArray<FString> Out;
	for (const std::string& Name : dirtbag::ThreeWhoCouldTurnUp(
	         dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)),
	         GenerationsBefore()))
	{
		Out.Add(FString(Name.c_str()));
	}
	return Out;
}

void UDirtbagGameInstance::NameTheClimber(const FString& Name)
{
	if (Name.IsEmpty())
	{
		return;
	}
	ClimberName = Name;
	// Straight into the sim as well as the mirror. The sync that normally
	// carries this over happens at Sleep, and a climber named at a handover
	// may put up a first ascent before ever sleeping -- which would sign
	// the line with the name the sim still had, which is nobody.
	Player.Name = Name;
}

void UDirtbagGameInstance::RetireAndPassItOn(const FString& Name)
{
	const dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	const dirtbag::Legacy L = dirtbag::TallyCareer(
	    SimPlayer, TCHAR_TO_UTF8(*Name), 1 + Player.Day / 365);
	Legacies.push_back(L);

	Player = DirtbagConvert::FromSim(dirtbag::Inherit(
	    L, dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed))));
	// The next one arrives unnamed; the naming UI fills ClimberName and the
	// sync in Sleep carries it into the sim, where the crew hash reads it.
	Day = UDirtbagSimLibrary::WakeUp(Player);
	ConsecutiveInjuries = 0;
	PeakGradeEver = 0.0;
	ClimberName.Reset();
	SaveNow();
}

int32 UDirtbagGameInstance::GenerationsBefore() const
{
	return static_cast<int32>(Legacies.size());
}

TArray<FString> UDirtbagGameInstance::InheritedGuidebook() const
{
	TArray<FString> Out;
	for (const dirtbag::Legacy& L : Legacies)
	{
		for (const dirtbag::NamedLine& N : L.firstAscents)
		{
			Out.Add(UTF8_TO_TCHAR(dirtbag::GuidebookEntry(N).c_str()));
		}
	}
	return Out;
}

// --- Age ---------------------------------------------------------------------

double UDirtbagGameInstance::Age() const
{
	return dirtbag::AgeOn(Player.Day);
}

FString UDirtbagGameInstance::AgeLine() const
{
	return UTF8_TO_TCHAR(dirtbag::AgeText(Age()).c_str());
}

// --- Where you stand ---------------------------------------------------------

FString UDirtbagGameInstance::StandingLine() const
{
	return UTF8_TO_TCHAR(
	    dirtbag::StandingText(DirtbagConvert::ToSim(Player.Standing)).c_str());
}

double UDirtbagGameInstance::StandingWith(EDirtbagFaction Faction) const
{
	return dirtbag::StandingWith(DirtbagConvert::ToSim(Player.Standing),
	                             static_cast<dirtbag::Faction>(Faction));
}

bool UDirtbagGameInstance::CragIsOpen() const
{
	return dirtbag::CragIsOpen(DirtbagConvert::ToSim(Player.Standing));
}

// --- Work --------------------------------------------------------------------

TArray<FDirtbagOddJob> UDirtbagGameInstance::TodaysJobBoard() const
{
	return UDirtbagSimLibrary::OddJobBoard(Seed, Player.Day);
}

bool UDirtbagGameInstance::TakeOddJob(const FDirtbagOddJob& Job)
{
	dirtbag::OddJob SimJob;
	SimJob.name = TCHAR_TO_UTF8(*Job.Name);
	SimJob.hours = Job.Hours;
	SimJob.pay = Job.Pay;
	SimJob.energy = Job.Energy;
	SimJob.needsVan = Job.bNeedsVan;

	// Taken before the work, because working changes the day and the guilt
	// is about the van you left it in this morning.
	const double Guilt =
	    dirtbag::VanGuilt(DirtbagConvert::ToSim(Player.Dog), VanTempF());

	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);
	if (!dirtbag::WorkOddJob(SimPlayer, SimDay, SimJob)) return false;
	Player = DirtbagConvert::FromSim(SimPlayer);
	Day = DirtbagConvert::FromSim(SimDay);
	bWorkedToday = true;

	// The dog does not come to a gig any more than it comes to a shift.
	//
	// This was in `WorkShift` and not here, so the two ways of working
	// disagreed about whether the dog existed -- and the board was about to
	// become the only way to work, which would have quietly deleted the
	// whole van-guilt system by making its one caller unreachable.
	if (Guilt > 0.0)
	{
		Player.Climber.Psyche = FMath::Max(0.05, Player.Climber.Psyche - Guilt);
		DogWorry = FString::Printf(
		    TEXT("You could hear it from the far side of the car park. The "
		         "van hit %.0fF."),
		    VanTempF());
	}
	return true;
}

FDirtbagCampfireHand UDirtbagGameInstance::DealCampfireHand(int32 HandNumber)
{
	FDirtbagCampfireHand Out;
	const std::vector<dirtbag::Partner> Lot = LotToday();
	const dirtbag::CampfireHand Hand = dirtbag::DealPoker(
	    Lot, dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)), Player.Day,
	    HandNumber);

	Out.Yours = Hand.yours;
	Out.Pot = Hand.pot;
	for (int32 i = 0; i < static_cast<int32>(Hand.who.size()); i++)
	{
		FDirtbagCampfireRead R;
		R.Who = FString(Hand.who[i].c_str());
		// The name is put in front of the tell here rather than in the sim,
		// because the sim has no business writing a sentence with somebody's
		// name in it and the engine has to anyway.
		R.Tell = FString::Printf(
		    TEXT("%s %s."), *R.Who,
		    *FString(dirtbag::ReadText(Hand.reads[i]).c_str()));
		Out.Reads.Add(R);
	}
	return Out;
}

FString UDirtbagGameInstance::PlayCampfireHand(int32 HandNumber, double Stake,
                                               bool bFold)
{
	std::vector<dirtbag::Partner> Lot = LotToday();
	const dirtbag::CampfireHand Hand = dirtbag::DealPoker(
	    Lot, dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)), Player.Day,
	    HandNumber);

	double Cash = Player.Cash;
	double Psyche = Player.Climber.Psyche;
	const dirtbag::CampfireResult R =
	    dirtbag::PlayPoker(Hand, Cash, Psyche, Lot, Stake, bFold);

	Player.Cash = Cash;
	Player.Climber.Psyche = Psyche;
	LastHandCash = R.cashDelta;
	// Rapport was paid inside PlayHand, so the bonds have to go back or an
	// evening of cards would be forgotten by morning.
	StoreBonds(Lot);
	return FString(R.line.c_str());
}

FDirtbagLiarsDice UDirtbagGameInstance::DealLiarsDice(int32 RoundNumber)
{
	FDirtbagLiarsDice Out;
	const std::vector<dirtbag::Partner> Lot = LotToday();
	const dirtbag::LiarsDiceRound R = dirtbag::DealLiarsDice(
	    Lot, dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)), Player.Day,
	    RoundNumber);

	for (int Pip : R.yours)
	{
		Out.Yours.Add(Pip);
	}
	Out.Bidder = FString(R.bidder.c_str());
	Out.DiceOnTable = R.diceOnTable;
	if (!R.bidder.empty())
	{
		Out.Bid = FString::Printf(TEXT("%s says there are %d %ds."),
		                          *Out.Bidder, R.bidCount, R.bidFace);
		Out.Tell = FString::Printf(
		    TEXT("%s %s."), *Out.Bidder,
		    *FString(dirtbag::TellText(R.tell).c_str()));
	}
	return Out;
}

FString UDirtbagGameInstance::PlayLiarsDice(int32 RoundNumber, double Stake,
                                            bool bCall)
{
	std::vector<dirtbag::Partner> Lot = LotToday();
	const dirtbag::LiarsDiceRound R = dirtbag::DealLiarsDice(
	    Lot, dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)), Player.Day,
	    RoundNumber);

	double Cash = Player.Cash;
	double Psyche = Player.Climber.Psyche;
	const dirtbag::CampfireResult Res =
	    dirtbag::PlayLiarsDice(R, Cash, Psyche, Lot, Stake, bCall);

	Player.Cash = Cash;
	Player.Climber.Psyche = Psyche;
	LastHandCash = Res.cashDelta;
	StoreBonds(Lot);
	return FString(Res.line.c_str());
}

FDirtbagBlackjack UDirtbagGameInstance::DealBlackjack(int32 HandNumber)
{
	const dirtbag::BlackjackHand H = dirtbag::DealBlackjack(
	    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)), Player.Day, HandNumber);
	LastBlackjack = DirtbagConvert::FromSim(H);
	return LastBlackjack;
}

int32 UDirtbagGameInstance::HitBlackjack(int32 HandNumber)
{
	dirtbag::BlackjackHand H = DirtbagConvert::ToSim(LastBlackjack);
	const int32 Card = dirtbag::Hit(
	    H, dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)), Player.Day,
	    HandNumber);
	LastBlackjack = DirtbagConvert::FromSim(H);
	return Card;
}

FString UDirtbagGameInstance::StandBlackjack(int32 HandNumber, double Stake)
{
	dirtbag::BlackjackHand H = DirtbagConvert::ToSim(LastBlackjack);
	std::vector<dirtbag::Partner> Lot = LotToday();
	double Cash = Player.Cash;
	double Psyche = Player.Climber.Psyche;

	const dirtbag::CampfireResult Res = dirtbag::Stand(
	    H, Cash, Psyche, Lot, Stake,
	    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)), Player.Day, HandNumber);

	LastBlackjack = DirtbagConvert::FromSim(H);
	Player.Cash = Cash;
	Player.Climber.Psyche = Psyche;
	LastHandCash = Res.cashDelta;
	StoreBonds(Lot);
	return FString(Res.line.c_str());
}

void UDirtbagGameInstance::SetPrompt(AActor* Owner,
                                    const TArray<FDirtbagPromptLine>& Lines)
{
	PromptOwner = Owner;
	Prompt.Lines = Lines;
	Prompt.bActive = Lines.Num() > 0;
}

void UDirtbagGameInstance::ClearPrompt(AActor* Owner)
{
	// Only the holder may put it down. Triggers overlap -- the van, the
	// fire and the shop can share a corner of the Lot -- and a spot that
	// cleared unconditionally on the way out would wipe the prompt of the
	// one you just walked into.
	if (PromptOwner != Owner)
	{
		return;
	}
	PromptOwner = nullptr;
	Prompt = FDirtbagPrompt();
}

EDirtbagFiresideGame UDirtbagGameInstance::WhatsOutTonight() const
{
	return static_cast<EDirtbagFiresideGame>(
	    dirtbag::WhatsOutTonight(Player.Day));
}

FString UDirtbagGameInstance::FiresideGameName(EDirtbagFiresideGame Which) const
{
	return FString(
	    dirtbag::GameName(static_cast<dirtbag::FiresideGame>(Which)));
}

double UDirtbagGameInstance::FireStake(int32 Notch) const
{
	return dirtbag::StakeNotch(Notch);
}

double UDirtbagGameInstance::DreamCost(EDirtbagDream Which) const
{
	return dirtbag::CostOf(static_cast<dirtbag::Dream>(Which));
}

FString UDirtbagGameInstance::DreamName(EDirtbagDream Which) const
{
	return FString(dirtbag::DreamName(static_cast<dirtbag::Dream>(Which)));
}

FString UDirtbagGameInstance::DreamBlurb(EDirtbagDream Which) const
{
	return FString(dirtbag::DreamBlurb(static_cast<dirtbag::Dream>(Which)));
}

bool UDirtbagGameInstance::CanAffordDream(EDirtbagDream Which) const
{
	return dirtbag::CanAfford(DirtbagConvert::ToSim(Player.Dreams),
	                          static_cast<dirtbag::Dream>(Which), Player.Cash);
}

bool UDirtbagGameInstance::ChooseDream(EDirtbagDream Which)
{
	dirtbag::Dreams SimDreams = DirtbagConvert::ToSim(Player.Dreams);
	if (!dirtbag::ChooseDream(SimDreams, static_cast<dirtbag::Dream>(Which)))
	{
		return false;
	}
	Player.Dreams = DirtbagConvert::FromSim(SimDreams);
	return true;
}

bool UDirtbagGameInstance::BuyDream(EDirtbagDream Which)
{
	dirtbag::Dreams SimDreams = DirtbagConvert::ToSim(Player.Dreams);
	dirtbag::Van SimVan = DirtbagConvert::ToSim(Player.Van);
	double Cash = Player.Cash;
	if (!dirtbag::BuyDream(SimDreams, SimVan, Cash,
	                       static_cast<dirtbag::Dream>(Which)))
	{
		return false;
	}
	Player.Dreams = DirtbagConvert::FromSim(SimDreams);
	Player.Van = DirtbagConvert::FromSim(SimVan);
	Player.Cash = Cash;

	// Said plainly, because this is the largest sum the player will ever
	// hand over and the game should not be coy about what it just did to
	// their float.
	DreamNews = FString::Printf(
	    TEXT("%s. And %d dollars left to your name."),
	    *DreamName(Which), FMath::FloorToInt(Cash));
	return true;
}

FString UDirtbagGameInstance::FreeDayLine() const
{
	if (!dirtbag::NoNeedToWork(DirtbagConvert::ToSim(Player.Dreams)))
	{
		return FString();
	}
	return TEXT("Nothing needs doing today.");
}

FString UDirtbagGameInstance::DreamLine() const
{
	FString Line =
	    FString(dirtbag::DreamText(DirtbagConvert::ToSim(Player.Dreams))
	                .c_str());

	// What you are still after, with the distance attached — "saving for
	// the Rig" is a mood and "$3,240 of $9,000" is a decision about this
	// week. Under chosen-dreams this line carries more weight than it did:
	// it is the one thing you are allowed to want.
	if (Player.Dreams.Chosen != EDirtbagDream::None &&
	    !dirtbag::HasDream(DirtbagConvert::ToSim(Player.Dreams),
	                       static_cast<dirtbag::Dream>(Player.Dreams.Chosen)))
	{
		const double Cost = DreamCost(Player.Dreams.Chosen);
		const FString Saving = FString::Printf(
		    TEXT("Saving for %s — $%d of $%d."),
		    *DreamName(Player.Dreams.Chosen),
		    FMath::FloorToInt(FMath::Max(0.0, Player.Cash)),
		    FMath::FloorToInt(Cost));
		Line += Line.IsEmpty() ? Saving : TEXT("  ") + Saving;
	}
	return Line;
}

FString UDirtbagGameInstance::CrewLine() const
{
	return FString(dirtbag::CrewText(DirtbagConvert::ToSim(Player.Crew))
	                   .c_str());
}

FString UDirtbagGameInstance::DirtbagYearLine() const
{
	return FString(dirtbag::DirtbagYearText(DirtbagConvert::ToSim(Player.Job))
	                   .c_str());
}

int32 UDirtbagGameInstance::TakeSalariedJob()
{
	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	// Read before, because TakeSalariedJob is what zeroes it.
	const int32 Lost = SimPlayer.job.daysSinceSalary;
	dirtbag::TakeSalariedJob(SimPlayer);
	Player = DirtbagConvert::FromSim(SimPlayer);
	return Lost;
}

void UDirtbagGameInstance::QuitSalariedJob()
{
	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	dirtbag::QuitSalariedJob(SimPlayer);
	Player = DirtbagConvert::FromSim(SimPlayer);
}

bool UDirtbagGameInstance::SalariedToday() const
{
	return dirtbag::SalariedToday(DirtbagConvert::ToSim(Player).job,
	                              Player.Day);
}

void UDirtbagGameInstance::WorkSalariedDay()
{
	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);
	dirtbag::WorkSalariedDay(SimPlayer, SimDay);
	Player = DirtbagConvert::FromSim(SimPlayer);
	Day = DirtbagConvert::FromSim(SimDay);

	// The dog spent the day in the van, same as it does for a shift.
	bWorkedToday = true;
}

// --- The body ----------------------------------------------------------------

FString UDirtbagGameInstance::InjuryLine() const
{
	return UTF8_TO_TCHAR(
	    dirtbag::InjuryText(DirtbagConvert::ToSim(Player.Climber)).c_str());
}

FString UDirtbagGameInstance::LoadLine() const
{
	return UTF8_TO_TCHAR(
	    dirtbag::LoadText(DirtbagConvert::ToSim(Player.Climber)).c_str());
}

int32 UDirtbagGameInstance::LoadWarning() const
{
	return dirtbag::LoadWarning(DirtbagConvert::ToSim(Player.Climber));
}

bool UDirtbagGameInstance::IsHurt() const
{
	return Player.Climber.Injury.bActive;
}

bool UDirtbagGameInstance::SeeAPhysio()
{
	dirtbag::Climber SimClimber = DirtbagConvert::ToSim(Player.Climber);
	double Cash = Player.Cash;
	int LastDay = Player.LastPhysioDay;
	if (!dirtbag::Physio(SimClimber, Cash, LastDay, Player.Day)) return false;
	Player.Climber = DirtbagConvert::FromSim(SimClimber);
	Player.Cash = Cash;
	Player.LastPhysioDay = LastDay;
	return true;
}

// --- The kit -----------------------------------------------------------------

namespace
{
	// Every purchase is the same shape: hand the sim its own view of the kit
	// and the cash, let it decide, and write both back only if it said yes.
	template <typename Fn>
	bool BuyWith(FDirtbagKit& Kit, double& Cash, Fn&& Buy)
	{
		dirtbag::Kit SimKit = DirtbagConvert::ToSim(Kit);
		double Money = Cash;
		if (!Buy(SimKit, Money)) return false;
		Kit = DirtbagConvert::FromSim(SimKit);
		Cash = Money;
		return true;
	}
}

bool UDirtbagGameInstance::BuyCrashPad()
{
	return BuyWith(Player.Kit, Player.Cash,
	               [](dirtbag::Kit& K, double& M) { return dirtbag::BuyPad(K, M); });
}

bool UDirtbagGameInstance::BuyHangboard()
{
	return BuyWith(Player.Kit, Player.Cash, [](dirtbag::Kit& K, double& M) {
		return dirtbag::BuyHangboard(K, M);
	});
}

bool UDirtbagGameInstance::RenewGymMembership()
{
	return BuyWith(Player.Kit, Player.Cash, [](dirtbag::Kit& K, double& M) {
		return dirtbag::RenewMembership(K, M);
	});
}

bool UDirtbagGameInstance::IsGymMember() const
{
	return dirtbag::IsGymMember(DirtbagConvert::ToSim(Player.Kit));
}

FString UDirtbagGameInstance::PadOfferLine() const
{
	return UTF8_TO_TCHAR(
	    dirtbag::PadOfferText(DirtbagConvert::ToSim(Player.Kit)).c_str());
}

FString UDirtbagGameInstance::KitLine() const
{
	return UTF8_TO_TCHAR(
	    dirtbag::KitText(DirtbagConvert::ToSim(Player.Kit)).c_str());
}

bool UDirtbagGameInstance::GoToTheGym()
{
	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);
	if (!dirtbag::GoToTheGym(SimPlayer, SimDay)) return false;
	Player = DirtbagConvert::FromSim(SimPlayer);
	Day = DirtbagConvert::FromSim(SimDay);
	return true;
}

bool UDirtbagGameInstance::HangboardSession()
{
	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);
	if (!dirtbag::HangboardSession(SimPlayer, SimDay)) return false;
	Player = DirtbagConvert::FromSim(SimPlayer);
	Day = DirtbagConvert::FromSim(SimDay);
	return true;
}

bool UDirtbagGameInstance::ResoleShoes()
{
	dirtbag::Shoes S = DirtbagConvert::ToSim(Player.Shoes);
	double Cash = Player.Cash;
	// The bottom sponsorship rung is a shoe deal and nothing else, so this
	// is the only place it can ever be worth anything.
	const bool bSponsored =
	    dirtbag::CoversShoes(DirtbagConvert::ToSim(Player.Sponsor));
	if (!dirtbag::Resole(S, Cash, bSponsored)) return false;
	Player.Shoes = DirtbagConvert::FromSim(S);
	Player.Cash = Cash;
	return true;
}

bool UDirtbagGameInstance::BuyNewShoes()
{
	dirtbag::Shoes S = DirtbagConvert::ToSim(Player.Shoes);
	double Cash = Player.Cash;
	const bool bSponsored =
	    dirtbag::CoversShoes(DirtbagConvert::ToSim(Player.Sponsor));
	if (!dirtbag::BuyNewShoes(S, Cash, bSponsored)) return false;
	Player.Shoes = DirtbagConvert::FromSim(S);
	Player.Cash = Cash;
	return true;
}

FString UDirtbagGameInstance::VanLine() const
{
	return FString(UTF8_TO_TCHAR(
	    dirtbag::VanText(DirtbagConvert::ToSim(Player.Van)).c_str()));
}

bool UDirtbagGameInstance::VanRuns() const
{
	return dirtbag::VanRuns(DirtbagConvert::ToSim(Player.Van));
}

int32 UDirtbagGameInstance::WorstVanPart() const
{
	return dirtbag::WorstVanPart(DirtbagConvert::ToSim(Player.Van));
}

bool UDirtbagGameInstance::BodgeVan()
{
	const int32 Part = WorstVanPart();
	if (Part < 0) return false;
	dirtbag::Van V = DirtbagConvert::ToSim(Player.Van);
	double Hours = 0.0;
	dirtbag::BodgeVan(V, static_cast<dirtbag::VanPart>(Part), Hours);
	Player.Van = DirtbagConvert::FromSim(V);
	PassHours(Hours);
	return true;
}

bool UDirtbagGameInstance::PatchVan()
{
	const int32 Part = WorstVanPart();
	if (Part < 0) return false;
	dirtbag::Van V = DirtbagConvert::ToSim(Player.Van);
	double Cash = Player.Cash, Hours = 0.0;
	if (!dirtbag::PatchVan(V, static_cast<dirtbag::VanPart>(Part), Cash, Hours))
	{
		return false;
	}
	Player.Van = DirtbagConvert::FromSim(V);
	Player.Cash = Cash;
	PassHours(Hours);
	return true;
}

bool UDirtbagGameInstance::ReplaceVanPart()
{
	const int32 Part = WorstVanPart();
	if (Part < 0) return false;
	dirtbag::Van V = DirtbagConvert::ToSim(Player.Van);
	double Cash = Player.Cash, Hours = 0.0;
	if (!dirtbag::ReplaceVanPart(V, static_cast<dirtbag::VanPart>(Part), Cash,
	                             Hours))
	{
		return false;
	}
	Player.Van = DirtbagConvert::FromSim(V);
	Player.Cash = Cash;
	PassHours(Hours);
	return true;
}

int32 UDirtbagGameInstance::DriveVan(double Hours)
{
	dirtbag::Van V = DirtbagConvert::ToSim(Player.Van);
	const dirtbag::Rng World = dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed));

	// The radiator reads the air, not the rock: it is the engine that
	// overheats, and it does it on the drive rather than at the crag.
	const double AirF =
	    bIndoors ? 70.0
	             : dirtbag::TemperatureAt(
	                   DirtbagConvert::ToSim(TodaysWeather()), Day.Hour);

	const int Broke = dirtbag::DriveVan(V, World, Player.Day, Hours, AirF);
	Player.Van = DirtbagConvert::FromSim(V);

	// And the pump. Every drive in the game comes through here, which is
	// why the charge lives here rather than at each travel spot: fuel was
	// written, measured at $918-$1,224 a season, and billed to nobody,
	// because the one caller who could have charged it did not have to.
	//
	// Charged rather than refused. You cannot decline to have burned the
	// fuel you already burned, so a skint player arrives at the crag owing
	// for the drive, the same way the rent works.
	const double Fuel = dirtbag::FuelFor(Hours);
	dirtbag::PlayerState Wallet = DirtbagConvert::ToSim(Player);
	dirtbag::Charge(Wallet, Fuel);
	Player.Cash = Wallet.cash;
	Player.Owed = Wallet.owed;
	LastDriveFuel = Fuel;

	if (Broke >= 0)
	{
		VanNews = FString::Printf(
		    TEXT("The %s went."),
		    UTF8_TO_TCHAR(dirtbag::VanPartName(
		        static_cast<dirtbag::VanPart>(Broke))));
	}
	return Broke;
}

// --- The dog -----------------------------------------------------------------

double UDirtbagGameInstance::VanTempF()
{
	if (bIndoors)
	{
		return 70.0;   // a gym car park is not the story
	}
	const dirtbag::Weather W = DirtbagConvert::ToSim(TodaysWeather());
	return dirtbag::TemperatureAt(W, Day.Hour) + VanGreenhouseF;
}

bool UDirtbagGameInstance::VanIsSafeForTheDog()
{
	return dirtbag::VanIsSafe(VanTempF());
}

bool UDirtbagGameInstance::FeedTheDog()
{
	dirtbag::Dog SimDog = DirtbagConvert::ToSim(Player.Dog);
	double Cash = Player.Cash;
	const bool bWasStray = !SimDog.adopted;

	if (!dirtbag::FeedDog(SimDog, Cash))
	{
		return false;
	}
	Player.Dog = DirtbagConvert::FromSim(SimDog);
	Player.Cash = Cash;

	// The one moment worth writing down as it happens, same as a first
	// ascent: you fed a stray until it was yours.
	if (bWasStray && SimDog.adopted)
	{
		SaveNow();
	}
	return true;
}

FString UDirtbagGameInstance::DogLine()
{
	const dirtbag::Dog SimDog = DirtbagConvert::ToSim(Player.Dog);
	FString Line = UTF8_TO_TCHAR(dirtbag::DogText(SimDog).c_str());

	// The forecast, for the one who cannot read it.
	if (Player.Dog.bAdopted && !bIndoors && !VanIsSafeForTheDog())
	{
		Line += FString::Printf(TEXT("  —  the van is at %.0fF"), VanTempF());
	}
	return Line;
}

// --- The Lot -----------------------------------------------------------------

std::vector<dirtbag::Partner> UDirtbagGameInstance::LotToday()
{
	const dirtbag::Rng World = dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed));
	std::vector<dirtbag::Partner> Lot =
	    dirtbag::LotRegulars(World, Player.Day);

	std::vector<dirtbag::PartnerBond> Bonds;
	Bonds.reserve(static_cast<size_t>(Player.Bonds.Num()));
	for (const FDirtbagPartnerBond& B : Player.Bonds)
	{
		Bonds.push_back(DirtbagConvert::ToSim(B));
	}
	dirtbag::ApplyBonds(Lot, Bonds);
	return Lot;
}

void UDirtbagGameInstance::StoreBonds(
    const std::vector<dirtbag::Partner>& Lot)
{
	Player.Bonds.Reset();
	for (const dirtbag::PartnerBond& B : dirtbag::BondsFrom(Lot))
	{
		Player.Bonds.Add(DirtbagConvert::FromSim(B));
	}
}

// --- The rope ----------------------------------------------------------------

FString UDirtbagGameInstance::BelayLine() const
{
	const std::vector<dirtbag::Partner> Lot =
	    const_cast<UDirtbagGameInstance*>(this)->LotToday();
	return UTF8_TO_TCHAR(
	    dirtbag::BelayText(dirtbag::BestBelayer(Lot)).c_str());
}

bool UDirtbagGameInstance::HasABelayer() const
{
	const std::vector<dirtbag::Partner> Lot =
	    const_cast<UDirtbagGameInstance*>(this)->LotToday();
	return dirtbag::BestBelayer(Lot) != nullptr;
}

int32 UDirtbagGameInstance::BurnsHeldToday() const
{
	const std::vector<dirtbag::Partner> Lot =
	    const_cast<UDirtbagGameInstance*>(this)->LotToday();
	const dirtbag::Partner* Who = dirtbag::BestBelayer(Lot);
	return Who ? dirtbag::BurnsTheyWillHold(*Who) : 0;
}

bool UDirtbagGameInstance::CanTieIn() const
{
	return RopedBurnsToday < BurnsHeldToday();
}

FString UDirtbagGameInstance::RopeRefusal() const
{
	if (!HasABelayer())
	{
		// The sim owns how this reads; a null belayer is exactly the case
		// BelayText was written for.
		return BelayLine();
	}
	if (!CanTieIn())
	{
		return TEXT("They have been down there long enough. Tomorrow.");
	}
	return FString();
}

TArray<FDirtbagPartner> UDirtbagGameInstance::GetLot()
{
	TArray<FDirtbagPartner> Out;
	for (const dirtbag::Partner& P : LotToday())
	{
		Out.Add(DirtbagConvert::FromSim(P));
	}
	return Out;
}

TArray<FString> UDirtbagGameInstance::LotTalk()
{
	TArray<FString> Out;
	EnsureCrag();
	const dirtbag::Crag SimCrag = dirtbag::RoadsideCrag(
	    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)));
	for (const dirtbag::Partner& P : LotToday())
	{
		Out.Add(FString(UTF8_TO_TCHAR(
		    dirtbag::LotTalk(P, SimCrag, Player.Day).c_str())));
	}
	return Out;
}

FString UDirtbagGameInstance::SitAtTheFire(double Hours)
{
	// Sitting at the fire is resting with company: the same hours, the same
	// hunger, and the people are what you get for spending them here rather
	// than alone at the boulders.
	Rest(Hours);

	std::vector<dirtbag::Partner> Lot = LotToday();
	double Lift = 0.0;
	for (dirtbag::Partner& P : Lot)
	{
		// Rapport is per day, so an hour is a fraction of one — you cannot
		// befriend the whole Lot by sitting still for a week.
		dirtbag::PartnerDials Dials;
		P.rapport = FMath::Min(
		    1.0, P.rapport + Dials.rapportPerDay * (Hours / 8.0));

		// The best of them, not the sum. Summing would make crowding the
		// fire a strategy, and it is not one — an evening is lifted by the
		// person who lifts it, not by a headcount.
		Lift = FMath::Max(Lift, dirtbag::PsycheFrom(P, Dials));
	}
	StoreBonds(Lot);

	// Paid for the hours you actually sat, on the same fraction rapport uses.
	Player.Climber.Psyche =
	    FMath::Min(1.0, Player.Climber.Psyche + Lift * (Hours / 8.0));

	// Which voice you hear is picked by the clock, not by engine randomness:
	// sitting an hour longer should change the subject, and reloading the
	// same afternoon should not.
	const TArray<FString> Talk = LotTalk();
	if (Talk.Num() == 0)
	{
		return FString();
	}
	const int32 Which =
	    FMath::Abs(static_cast<int32>(Day.Hour * 2.0) + Player.Day) %
	    Talk.Num();
	return Talk[Which];
}

double UDirtbagGameInstance::AskForBeta(int32 BoardIndex, FString& OutWho)
{
	OutWho.Reset();
	FDirtbagProjectMemory* Ledger = LedgerFor(BoardIndex);
	if (!Ledger || bIndoors)
	{
		return 0.0;
	}
	const FDirtbagCragLine Line = GetCragLine(BoardIndex);
	dirtbag::CragLine SimLine;
	SimLine.route = DirtbagConvert::ToSim(Line.Route);
	SimLine.isProject = Line.bIsProject;

	std::vector<dirtbag::Partner> Lot = LotToday();

	// Whoever can actually help most, which is not always whoever you know
	// best — Trish is delighted to help and cannot.
	double Best = 0.0;
	dirtbag::ProjectMemory SimLedger = DirtbagConvert::ToSim(*Ledger);
	const dirtbag::Standing SimStanding = DirtbagConvert::ToSim(Player.Standing);
	for (const dirtbag::Partner& P : Lot)
	{
		// What you are to their crowd decides how much of the sequence they
		// bother to spell out. This is the first thing standing has ever
		// bought at the wall rather than on a screen — BetaMultiplierFor was
		// written, tested, and reachable from nothing.
		dirtbag::ProjectMemory Trial = SimLedger;
		const double Gained = dirtbag::ShareBeta(
		    P, SimLine, Trial,
		    dirtbag::BetaMultiplierFor(SimStanding, P.name));
		if (Gained > Best)
		{
			Best = Gained;
			SimLedger = Trial;
			OutWho = UTF8_TO_TCHAR(P.name.c_str());
		}
	}
	if (Best > 0.0)
	{
		*Ledger = DirtbagConvert::FromSim(SimLedger);
	}
	return Best;
}

void UDirtbagGameInstance::AdvanceTheLot()
{
	const dirtbag::Rng World = dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed));
	EnsureCrag();
	dirtbag::Crag SimCrag = dirtbag::RoadsideCrag(World);

	// Everything anybody has already claimed, the player included: a line
	// you have done is not still lying around for Dev to take.
	// Claimed, or visibly being worked. The second half is what stops the
	// Lot taking every project in the valley: a per-day roll over a
	// thirty-year career converges on certainty however small the dial is,
	// so commitment has to protect a line structurally rather than
	// probabilistically.
	std::vector<std::string> Taken =
	    dirtbag::SpokenFor(DirtbagConvert::ToSim(Player).projects);
	std::vector<dirtbag::Partner> Lot = LotToday();
	for (const dirtbag::Partner& P : Lot)
	{
		for (const std::string& Key : P.firstAscents)
		{
			Taken.push_back(Key);
		}
	}

	LotNews.Reset();
	for (dirtbag::Partner& P : Lot)
	{
		// Rapport is earned by turning up and climbing, not by existing.
		dirtbag::SpendDayWith(P, bClimbedToday);

		const int Line = dirtbag::PartnerTakesFirstAscent(World, P, SimCrag,
		                                                  Taken, Player.Day);
		if (Line < 0)
		{
			continue;
		}
		// Write it into the book, which is the half that was missing: the
		// claim used to go only into the partner's own list, so the line
		// stayed an open project and the player could still walk up and
		// take the first ascent of something Dev did last spring.
		dirtbag::CragLine& Got = SimCrag.lines[Line];
		dirtbag::TheyPutUpTheLine(Got, P.name);
		P.firstAscents.push_back(Got.route.name);
		Taken.push_back(Got.route.name);

		// Told plainly and without sympathy, which is how it happens.
		// Appended rather than assigned: two people getting lucky on the
		// same night is rare, and silently dropping one of them would be
		// worse than the crowded line it avoids.
		if (!LotNews.IsEmpty())
		{
			LotNews += TEXT("   ");
		}
		LotNews += FString::Printf(
		    TEXT("%s got %s. They are calling it %s."),
		    UTF8_TO_TCHAR(P.name.c_str()),
		    UTF8_TO_TCHAR(Got.description.c_str()),
		    UTF8_TO_TCHAR(dirtbag::DisplayName(Got).c_str()));
	}
	StoreBonds(Lot);
	bClimbedToday = false;
}

void UDirtbagGameInstance::OfferNaming(int32 BoardIndex)
{
	if (!CanNameLine(BoardIndex))
	{
		return;
	}
	bNamingPending = true;
	NamingBoardIndex = BoardIndex;
	const FDirtbagCragLine Line = GetCragLine(BoardIndex);
	NamingLineText =
	    Line.Description.IsEmpty() ? Line.Route.Name : Line.Description;
}

void UDirtbagGameInstance::DismissNaming()
{
	// Only the prompt goes away. CanNameLine still answers true tomorrow:
	// you did the first ascent, and that does not expire because you closed
	// a window.
	bNamingPending = false;
}

bool UDirtbagGameInstance::NameFirstAscent(int32 BoardIndex,
                                           const FString& Name)
{
	if (!CanNameLine(BoardIndex))
	{
		return false;
	}
	FDirtbagProjectMemory* Ledger = LedgerFor(BoardIndex);
	const FDirtbagCragLine Line = GetCragLine(BoardIndex);

	dirtbag::CragLine SimLine;
	SimLine.route = DirtbagConvert::ToSim(Line.Route);
	SimLine.isProject = Line.bIsProject;
	SimLine.firstAscentBy = TCHAR_TO_UTF8(*Line.FirstAscentBy);
	dirtbag::ProjectMemory SimLedger = DirtbagConvert::ToSim(*Ledger);

	// ClaimFirstAscent, not NameFirstAscent: naming is the ledger half, and
	// on its own it leaves the valley with no opinion about what you just
	// did. Doing a line nobody had done is the loudest thing a climber can
	// do here and it was worth exactly nothing to any faction.
	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	if (!dirtbag::ClaimFirstAscent(SimPlayer, SimLedger, SimLine,
	                               TCHAR_TO_UTF8(*Name)))
	{
		return false;
	}
	// Only the standing comes back. The ledger is written through the
	// pointer below, and round-tripping the whole player here would undo
	// anything the rest of this frame had already changed.
	Player.Standing = DirtbagConvert::FromSim(SimPlayer.standing);
	*Ledger = DirtbagConvert::FromSim(SimLedger);

	// The book is loaded and stale by one line. EnsureCrag would fix it on
	// the next venue change, which is far too late: the player is standing
	// in front of the thing they just named.
	UDirtbagSimLibrary::WriteIntoTheBook(Crag, Player, AscentSignature());

	// Say it back. Naming was silent until now — the widget closed and
	// nothing in the world acknowledged that the line was yours.
	LastAscentLine = FirstAscentLine(BoardIndex);

	bNamingPending = false;

	// A first ascent is the one thing in this game worth writing down the
	// moment it happens rather than at lights out.
	SaveNow();
	return true;
}

FString UDirtbagGameInstance::FirstAscentLine(int32 BoardIndex)
{
	const FDirtbagProjectMemory* Ledger = LedgerFor(BoardIndex);
	if (!Ledger)
	{
		return FString();
	}
	const FString By = AscentSignature();
	return FString(UTF8_TO_TCHAR(
	    dirtbag::FirstAscentLine(DirtbagConvert::ToSim(*Ledger),
	                             TCHAR_TO_UTF8(*By))
	        .c_str()));
}

TArray<FDirtbagProjectMemory> UDirtbagGameInstance::GetFirstAscents() const
{
	TArray<FDirtbagProjectMemory> Out;
	const dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	for (const dirtbag::ProjectMemory* M : dirtbag::FirstAscents(SimPlayer))
	{
		Out.Add(DirtbagConvert::FromSim(*M));
	}
	return Out;
}
