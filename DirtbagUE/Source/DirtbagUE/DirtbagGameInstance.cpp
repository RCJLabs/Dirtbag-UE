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

	// **A career opens with four questions.** Skipped for a save whose
	// climber has already answered them, which is the whole of the old-save
	// story: an unbuilt character is neutral in every lane, so a v20 save
	// loads and plays exactly as it did until the day it is rebuilt.
	BeginCreation();
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
	QuirkNews.Reset();
	const bool bHadACrewName = !Player.Crew.Name.IsEmpty();

	// SleepToNextDay ticks the streak, so a year completing is visible as
	// the count going up across the call. Reading the count rather than
	// plumbing a return value through the Blueprint library keeps the news
	// here, where every other piece of overnight news already lives.
	const int32 YearsBefore = Player.Job.DirtbagYears;

	// And the same trick for the World Cup, which closes its season inside
	// the same call. A season latches `bClosed` exactly once and then sits
	// closed through the whole off-season, so the transition is the news
	// and the flag is not.
	const bool bWorldClosedBefore = Player.WorldCup.bClosed;
	WorldCupNews.Reset();
	GamesNews.Reset();

	// And the same for the medical file, which heals inside the same call.
	// Read across it rather than plumbed through: an injury that cleared
	// overnight is the only overnight medical news there is, and the flag
	// going false is exactly that.
	const bool bWasHurt = Player.Climber.Injury.bActive;
	const int32 ScarsBefore = Player.Medical.Scars.Num();
	MedicalNews.Reset();

	// The sim's copy of who you are, kept current before CrewDay runs
	// inside SleepToNextDay — the crew hash includes your name, so the sync
	// has to happen on this side of the call or the town would name
	// generation two after generation one.
	Player.Name = ClimberName;

	UDirtbagSimLibrary::SleepToNextDay(Seed, Player, Day);

	// **What you turned into overnight**, if anything, which is almost
	// never. Read off the career rather than detected across the call like
	// the news above it: `HabitsDay` already decides which quirk landed and
	// hands it back, so re-deriving it here would be a second opinion about
	// a thing the sim has already answered.
	if (Player.BecameToday != EDirtbagQuirk::None)
	{
		QuirkNews = FString(
		    dirtbag::QuirkLanded(
		        static_cast<dirtbag::Quirk>(Player.BecameToday))
		        .c_str());
	}

	if (bWasHurt && !Player.Climber.Injury.bActive)
	{
		// **What it leaves is the news, not that it stopped hurting.** An
		// injury nobody looked at heals too; it just costs you something
		// you will not see for years.
		MedicalNews =
		    Player.Medical.Scars.Num() > ScarsBefore &&
		            !Player.Medical.bTreatedThisTime
		        ? FString(TEXT("It stopped hurting. You never did find out "
		                       "what it was."))
		        : FString(TEXT("Cleared to climb."));
	}

	// A World Cup year ended overnight. Rebuilt from the season rather than
	// returned through the Blueprint library, for the reason above.
	if (Player.WorldCup.bClosed && !bWorldClosedBefore)
	{
		WorldCupNews = FString(
		    dirtbag::WorldCupSeasonNews(DirtbagConvert::ToSim(Player.WorldCup))
		        .c_str());
	}

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

	// **The day a talent stops being a secret.** `ApplyAttemptToDay` books
	// the session's work in each lane and flips the flag when one becomes
	// obvious; nothing inside the sim says so out loud, because a sim
	// function that formats a sentence for a HUD is a sim function that
	// knows about a HUD. So it is noticed here, by the transition -- which
	// also means it can only ever be said once.
	const bool bKnewGift = SimPlayer.character.giftKnown;
	const bool bKnewAnti = SimPlayer.character.antiKnown;

	dirtbag::ApplyAttemptToDay(SimPlayer, SimDay, SimRoute, Result, Live.rng);

	if (!bKnewGift && SimPlayer.character.giftKnown)
	{
		TalentNews = FString(
		    dirtbag::TalentSurfaced(SimPlayer.character.gift).c_str());
	}
	else if (!bKnewAnti && SimPlayer.character.antiKnown)
	{
		TalentNews = FString(
		    dirtbag::TalentSurfaced(SimPlayer.character.antiTalent).c_str());
	}

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

namespace
{
// The question, in the game's voice. Not "Select Archetype": you are
// standing in a car park at the start of a life, and the game should sound
// like it knows that.
const TCHAR* CreationQuestion(EDirtbagCreationStep Step)
{
	switch (Step)
	{
	case EDirtbagCreationStep::Archetype:
		return TEXT("What kind of climber are you?");
	case EDirtbagCreationStep::Origin:
		return TEXT("How did you end up here?");
	case EDirtbagCreationStep::Flaw:
		return TEXT("And what is wrong with you?");
	case EDirtbagCreationStep::Quirk:
		return TEXT("One more thing about you.");
	default:
		return TEXT("What are you like?");
	}
}
}  // namespace

void UDirtbagGameInstance::RefreshCreation()
{
	FDirtbagCreationReadout& C = Creation;
	C.Options.Reset();
	C.Blurbs.Reset();
	if (!C.bActive || C.Step == EDirtbagCreationStep::Done)
	{
		return;
	}
	C.Question = CreationQuestion(C.Step);

	// The catalogue is the sim's, read rather than restated. A second copy
	// of six origins in engine code is two lists that disagree by Christmas.
	const auto Add = [&C](const TCHAR* Name, const TCHAR* Blurb)
	{
		C.Options.Add(FString(Name));
		C.Blurbs.Add(FString(Blurb));
	};
	switch (C.Step)
	{
	case EDirtbagCreationStep::Archetype:
		for (int32 i = 0; i < dirtbag::kArchetypeCount; i++)
		{
			const dirtbag::ArchetypeDef& D =
			    dirtbag::Describe(static_cast<dirtbag::Archetype>(i));
			Add(UTF8_TO_TCHAR(D.name), UTF8_TO_TCHAR(D.blurb));
		}
		break;
	case EDirtbagCreationStep::Origin:
		for (int32 i = 0; i < dirtbag::kOriginCount; i++)
		{
			const dirtbag::OriginDef& D =
			    dirtbag::Describe(static_cast<dirtbag::Origin>(i));
			// The perk is said with the blurb, because an origin that does
			// not say what it permanently buys you is a flavour text.
			Add(UTF8_TO_TCHAR(D.name),
			    *FString::Printf(TEXT("%s  %s"), UTF8_TO_TCHAR(D.blurb),
			                     UTF8_TO_TCHAR(D.perk)));
		}
		break;
	case EDirtbagCreationStep::Quirk:
		// Only the pickable ones, and read off the sim's own enum rather
		// than restated here -- a second list of six quirks in engine code
		// is two lists that disagree by Christmas.
		for (int32 i = static_cast<int32>(dirtbag::Quirk::LightSleeper);
		     i < dirtbag::kQuirkCount; i++)
		{
			const dirtbag::Quirk Q = static_cast<dirtbag::Quirk>(i);
			Add(UTF8_TO_TCHAR(dirtbag::QuirkName(Q)),
			    UTF8_TO_TCHAR(dirtbag::QuirkLine(Q)));
		}
		break;
	case EDirtbagCreationStep::Flaw:
		for (int32 i = 0; i < dirtbag::kFlawCount; i++)
		{
			const dirtbag::FlawDef& D =
			    dirtbag::Describe(static_cast<dirtbag::Flaw>(i));
			Add(UTF8_TO_TCHAR(D.name), UTF8_TO_TCHAR(D.blurb));
		}
		break;
	default:
		for (int32 i = 0; i < dirtbag::kTemperamentCount; i++)
		{
			const dirtbag::TemperamentDef& D =
			    dirtbag::Describe(static_cast<dirtbag::Temperament>(i));
			Add(UTF8_TO_TCHAR(D.name), UTF8_TO_TCHAR(D.blurb));
		}
		break;
	}
}

bool UDirtbagGameInstance::AcceptTheRival()
{
	if (RivalOffer.IsEmpty())
	{
		return false;
	}
	Player.Rival.bAllied = true;
	// Clearing the race with it: you do not keep racing somebody you have
	// just agreed to tie in with.
	Player.Rival.Race = FDirtbagRace{};
	RivalNews = FString::Printf(
	    TEXT("You are climbing with %s now. You are not sure when that "
	         "changed."),
	    *Player.Rival.Name);
	RivalOffer.Empty();
	return true;
}

bool UDirtbagGameInstance::DeclineTheRival()
{
	if (RivalOffer.IsEmpty())
	{
		return false;
	}
	// **Nothing is taken away.** Declining costs no standing and no
	// head-to-head -- it is a real answer rather than a worse one, and the
	// only thing it changes is that they go back to racing you. Said
	// without editorial, because the game does not have an opinion about
	// which of you was right.
	RivalNews = FString::Printf(TEXT("You said no. %s did not seem surprised."),
	                            *Player.Rival.Name);
	RivalOffer.Empty();
	return true;
}

namespace
{
// The live comp, kept sim-side. It is not on the player state because a
// comp does not survive a reload: you are in the gym for six hours and the
// save is written when you sleep, so a comp interrupted by an alt-F4 is a
// comp you did not finish. Storing it would mean deciding what a half-comp
// means on load, which is a worse answer than "you missed it".
dirtbag::CompState GLiveComp;
}  // namespace

int32 UDirtbagGameInstance::DaysUntilComp() const
{
	return dirtbag::DaysUntilComp(DirtbagConvert::ToSim(Player.Circuit),
	                              Player.Day);
}

bool UDirtbagGameInstance::CompIsToday() const
{
	return dirtbag::CompIsToday(DirtbagConvert::ToSim(Player.Circuit),
	                            Player.Day);
}

FString UDirtbagGameInstance::RankLine() const
{
	const dirtbag::RankTier T = dirtbag::RankFor(Player.RankingPoints);
	const double To = dirtbag::ToNextRank(Player.RankingPoints);
	FString Line = UTF8_TO_TCHAR(dirtbag::RankName(T));
	// The gap to the next one, in points, because that is the only number
	// on this ladder a player can do anything about.
	if (To > 0.0)
	{
		Line += FString::Printf(TEXT("  (%.0f to the next)"), To);
	}
	return Line;
}

FString UDirtbagGameInstance::TeamStandingLine() const
{
	return FString(
	    dirtbag::TeamLine(DirtbagConvert::ToSim(Player.Team)).c_str());
}

FString UDirtbagGameInstance::CircuitStandingLine() const
{
	return FString(
	    dirtbag::CircuitLine(DirtbagConvert::ToSim(Player.Circuit)).c_str());
}

FString UDirtbagGameInstance::CompLine() const
{
	const dirtbag::CompTier Tier = dirtbag::TierFor(Player.RankingPoints);
	if (CompIsToday())
	{
		return FString::Printf(
		    TEXT("The %s comp is today.  ($%.0f to enter)"),
		    UTF8_TO_TCHAR(dirtbag::TierName(Tier)),
		    dirtbag::CompDials{}.entryFee);
	}
	const int32 Days = DaysUntilComp();
	if (Days < 0)
	{
		return FString();
	}
	// Days in words, like everything else the game says slowly.
	return FString::Printf(
	    TEXT("There is a %s comp %s."), UTF8_TO_TCHAR(dirtbag::TierName(Tier)),
	    Days == 1 ? TEXT("tomorrow") : Days == 2 ? TEXT("the day after next")
	                                             : TEXT("this week"));
}

bool UDirtbagGameInstance::EnterComp()
{
	if (Comp.bActive || !CompIsToday())
	{
		return false;
	}
	const dirtbag::CompDials Dials;
	if (Player.Cash < Dials.entryFee)
	{
		return false;
	}
	Player.Cash -= Dials.entryFee;
	PassHours(Dials.hours);
	Day.Energy = FMath::Max(0.0, Day.Energy - Dials.energy);

	GLiveComp = dirtbag::SetTheBoard(
	    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)),
	    dirtbag::TierFor(Player.RankingPoints), AllroundGrade(), Player.Day);
	Comp = FDirtbagCompReadout{};
	Comp.bActive = true;
	RefreshComp();
	return true;
}

void UDirtbagGameInstance::RefreshComp()
{
	Comp.Tier = FString(dirtbag::TierName(GLiveComp.tier));
	Comp.Round = static_cast<EDirtbagCompRound>(GLiveComp.round);
	Comp.bHasRounds = Comp.Stage == EDirtbagStage::Domestic &&
	                  dirtbag::RunsRounds(GLiveComp.tier);
	Comp.StillIn = GLiveComp.stillIn.size();
	Comp.AttemptsLeft = GLiveComp.attemptsLeft;
	Comp.YourScore = dirtbag::YourScore(GLiveComp);
	Comp.Problems.Reset(GLiveComp.problems.size());
	for (std::size_t i = 0; i < GLiveComp.problems.size(); i++)
	{
		const dirtbag::CompProblem& P = GLiveComp.problems[i];
		const dirtbag::ProblemProgress& Pr = GLiveComp.progress[i];
		FDirtbagCompProblem Out;
		Out.Colour = FString(P.route.name.c_str());
		Out.Grade = UDirtbagSimLibrary::GradeName(
		    P.route.trueGrade, EDirtbagDiscipline::Boulder);
		Out.Points = P.topPoints;
		Out.FlashPoints = P.flashPoints;
		Out.Tries = Pr.tries;
		Out.bTopped = Pr.topped;
		Out.bFlashed = Pr.flashed;
		Out.Zone = Pr.zone;
		Comp.Problems.Add(Out);
	}
}

bool UDirtbagGameInstance::CompAttempt(int32 Which)
{
	if (!Comp.bActive || Comp.bSettled)
	{
		return false;
	}
	const int32 Before = GLiveComp.attemptsLeft;
	// **Everything the climber walked in carrying**, or a comp is the one
	// room in the game where a wrecked body climbs like a fresh one --
	// which it was, measured: a maxed-out finger joint, four cortisone
	// shots, a flu and an abscess scored identically to nothing at all.
	// See Sim/DirtbagBodyContext.h.
	dirtbag::AttemptProblem(
	    GLiveComp, Which, DirtbagConvert::ToSim(Player.Climber),
	    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed) +
	                           std::string("#comp#") +
	                           std::to_string(Player.Day)),
	    // The dials the live board was set under. Today every variant
	    // agrees on the two things `AttemptProblem` actually reads -- the
	    // pressure and the zone thresholds -- so this changes nothing yet.
	    // It is here because the board and the burns resolving under
	    // *different* dials is the same class of bug as the one this whole
	    // change closes, and it costs a branch to make impossible.
	    Comp.Stage == EDirtbagStage::League
	        ? dirtbag::LeagueCompDials()
	    : Comp.Stage == EDirtbagStage::WorldCup
	        ? dirtbag::WorldCupCompDials()
	    : Comp.Stage == EDirtbagStage::Games
	        ? dirtbag::OlympicCompDials()
	        : dirtbag::CompDials{},
	    TheBodyYouWalkedInWith());
	RefreshComp();
	// The go only counted if the sim took it -- a bad index or a problem
	// you have already topped spends nothing, which the caller needs to
	// know so it does not report a burn that never happened.
	return GLiveComp.attemptsLeft < Before;
}

bool UDirtbagGameInstance::SettleComp()
{
	if (!Comp.bActive || Comp.bSettled)
	{
		return false;
	}

	// **Where the result goes is decided by which board this was.**
	//
	// One comp engine, three ladders. The domestic path below banks into
	// the circuit, moves the ranking, and lets the committee sit; a World
	// Cup round banks into the season table; the Games bank a medal. Doing
	// all three through one branch here rather than three settle functions
	// keeps the attempt loop, the scoreboard and the placing text shared,
	// which is what stops two comps disagreeing about what a flash is
	// worth.
	if (Comp.Stage == EDirtbagStage::League)
	{
		return SettleTheLeague();
	}
	if (Comp.Stage != EDirtbagStage::Domestic)
	{
		return SettleTheWorldStage();
	}

	const dirtbag::Rival R = DirtbagConvert::ToSim(Player.Rival);
	const dirtbag::CompResult Res = dirtbag::Settle(
	    GLiveComp, AllroundGrade(), R.retired ? std::string() : R.name,
	    R.grade,
	    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed) +
	                           std::string("#settle#") +
	                           std::to_string(Player.Day) +
	                           std::string("#r") +
	                           std::to_string(static_cast<int>(
	                               GLiveComp.round))));

	// **A round is not the comp.** At Regional and above the scorecard you
	// just turned in decides who goes through rather than who won, and if
	// you are one of them the board underneath you is replaced: a fresh
	// five, half a grade up, a fresh set of goes, and the score does not
	// carry. Only the round you go out in -- or the final -- reaches the
	// banking below.
	if (dirtbag::RunsRounds(GLiveComp.tier) &&
	    GLiveComp.round != dirtbag::CompRound::Final)
	{
		const dirtbag::RoundOutcome Out = dirtbag::NextRound(
		    GLiveComp, Res,
		    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)), AllroundGrade(),
		    Player.Day);
		if (Out.through)
		{
			// Another round is another round's worth of pump. Charged per
			// round rather than once for the day, because that is what the
			// format costs.
			Day.Energy = FMath::Max(
			    0.0, Day.Energy - dirtbag::CompDials{}.roundEnergy);
			Comp.RoundNews = FString(Out.news.c_str());
			Comp.Board.Reset();
			Comp.Placing.Reset();
			RefreshComp();
			return true;
		}
		Comp.RoundNews = FString(Out.news.c_str());
	}

	Player.Cash += Res.cash;

	// **Into the season and onto the ladder.**
	//
	// Ranking points come from the placing rather than the prize rep: a win
	// is a hundred, the back of the field is five, and the finals are worth
	// half as much again. The prize `rep` is what the *scene* thinks and
	// belongs to standing; this is what the federation records, and the two
	// are different numbers on purpose.
	dirtbag::Circuit Season = DirtbagConvert::ToSim(Player.Circuit);
	const bool bFinals = dirtbag::FinalsToday(Season, Player.Day);
	dirtbag::BankResult(Season, Res, bFinals);
	// **Recorded, not added.** `RankingPoints` is derived: the night tick
	// recomputes it from the record, so a direct write here would survive
	// until the next morning and no further. The record is what a ranking
	// is -- the last year of results -- and it is why a rung can be lost.
	RecordResult(dirtbag::RankingPointsFor(Res.place, Res.fieldSize,
	                                       GLiveComp.tier, bFinals, false,
	                                       false, false));

	// And if that was the last one, the season is over and the podium gets
	// paid. Closed here rather than at Sleep because the table is complete
	// the moment the finals are turned in, and hearing about it tomorrow
	// morning would be the game telling you something you watched happen.
	if (dirtbag::SeasonOver(Season))
	{
		const dirtbag::SeasonEnd End = dirtbag::CloseSeason(Season);
		Player.Cash += End.cash;
		// A season's podium is a National-weight result whatever room the
		// comps were in: winning a year is winning a year.
		RecordResult(End.rankingPoints);
		if (End.title) { Season.titles++; }
		CompNews = FString::Printf(
		    TEXT("Season %d done - you finished %d%s.%s"), Season.season,
		    End.place,
		    End.place == 1   ? TEXT("st")
		    : End.place == 2 ? TEXT("nd")
		    : End.place == 3 ? TEXT("rd")
		                     : TEXT("th"),
		    End.title ? TEXT("  That is a title.") : TEXT(""));

		// **And the committee sits.** Here and nowhere else: a domestic
		// season is the unit a selection committee actually works in, and
		// reviewing you every night would make the team a thermostat.
		//
		// After the season's own podium points are banked, because they are
		// part of the year the committee is looking at.
		dirtbag::NationalTeam Team = DirtbagConvert::ToSim(Player.Team);
		const dirtbag::TeamReview Review = dirtbag::ReviewTheTeam(
		    Team, Player.RankingPoints, Season, Player.Day, Season.season);
		Player.Team = DirtbagConvert::FromSim(Team);
		Player.Cash += Review.stipend;
		if (Review.changed)
		{
			// Standing rather than ranking: being named is what the *scene*
			// makes of you, and the federation already had its say in the
			// number that got you there. The Scene is the faction this
			// belongs to by its own definition -- "comps, sponsors, media,
			// the gym".
			dirtbag::Standing S = DirtbagConvert::ToSim(Player.Standing);
			dirtbag::Shift(S, dirtbag::Faction::Scene, Review.rep);
			Player.Standing = DirtbagConvert::FromSim(S);
			TeamNews = FString(Review.news.c_str());
		}
	}
	Player.Circuit = DirtbagConvert::FromSim(Season);

	// Beating them is head-to-head, and it is the same currency an FA moves.
	if (Res.beatTheRival)
	{
		dirtbag::Rival Beat = R;
		dirtbag::YouGotThereFirst(Beat);
		Beat.met = true;
		Player.Rival = DirtbagConvert::FromSim(Beat);
	}

	Comp.bSettled = true;
	Comp.Placing = FString(dirtbag::PlacingText(Res).c_str());
	Comp.Board.Reset(Res.board.size());
	for (const dirtbag::CompEntrant& E : Res.board)
	{
		Comp.Board.Add(FString::Printf(TEXT("%s%s   %.0f"),
		                               E.isYou ? TEXT("> ") : TEXT("  "),
		                               UTF8_TO_TCHAR(E.name.c_str()),
		                               E.score));
	}
	RefreshComp();
	return true;
}

FString UDirtbagGameInstance::ZoneNameOf(EDirtbagZone Which) const
{
	return FString(UTF8_TO_TCHAR(
	    dirtbag::ZoneName(static_cast<dirtbag::Zone>(Which))));
}

FString UDirtbagGameInstance::ZoneBlurbOf(EDirtbagZone Which) const
{
	return FString(UTF8_TO_TCHAR(
	    dirtbag::ZoneBlurb(static_cast<dirtbag::Zone>(Which))));
}

// --- and the rest of what is wrong with you ----------------------------

FString UDirtbagGameInstance::SickLine() const
{
	return FString(
	    dirtbag::SickText(DirtbagConvert::ToSim(Player.Sickness)).c_str());
}

bool UDirtbagGameInstance::TakeSomethingForIt()
{
	dirtbag::Sickness S = DirtbagConvert::ToSim(Player.Sickness);
	double Cash = Player.Cash;
	if (!dirtbag::TakeSomethingForIt(S, Cash)) { return false; }
	Player.Cash = Cash;
	Player.Sickness = DirtbagConvert::FromSim(S);
	MedicalNews = SickLine();
	return true;
}

FString UDirtbagGameInstance::TeethLine() const
{
	return FString(
	    dirtbag::TeethText(DirtbagConvert::ToSim(Player.Teeth)).c_str());
}

double UDirtbagGameInstance::ToothPrice() const
{
	return dirtbag::ToothPrice(
	    DirtbagConvert::ToSim(Player.Teeth).stage);
}

bool UDirtbagGameInstance::FixTheTooth()
{
	dirtbag::Teeth T = DirtbagConvert::ToSim(Player.Teeth);
	double Cash = Player.Cash;
	if (!dirtbag::FixTheTooth(T, Cash, Player.Day)) { return false; }
	Player.Cash = Cash;
	Player.Teeth = DirtbagConvert::FromSim(T);
	// **What it was is remembered**, and the game says so once, because a
	// career remembers and this one nearly cost you a season.
	MedicalNews =
	    T.worstEver >= static_cast<int>(dirtbag::ToothStage::Abscess)
	        ? TEXT("Dealt with. It should never have got that far.")
	        : TEXT("Dealt with, and cheaply, which is the only time it "
	               "ever is.");
	return true;
}

bool UDirtbagGameInstance::DoPrehab()
{
	dirtbag::Upkeep U = DirtbagConvert::ToSim(Player.Upkeep);
	double Hour = Day.Hour;
	if (!dirtbag::DoPrehab(U, Hour, Player.Day)) { return false; }
	// Through PassHours rather than assigning the clock, so hunger rides
	// along exactly as it does for every other twenty minutes of a day.
	PassHours(Hour - Day.Hour);
	Player.Upkeep = DirtbagConvert::FromSim(U);
	return true;
}

FString UDirtbagGameInstance::UpkeepLine() const
{
	return FString(dirtbag::UpkeepText(
	                   DirtbagConvert::ToSim(Player.Upkeep), Player.Day)
	                   .c_str());
}

bool UDirtbagGameInstance::SeeTheShrink()
{
	dirtbag::Upkeep U = DirtbagConvert::ToSim(Player.Upkeep);
	dirtbag::Climber C = DirtbagConvert::ToSim(Player.Climber);
	double Cash = Player.Cash;
	if (!dirtbag::SeeTheShrink(U, C, Cash, Player.Day)) { return false; }
	Player.Cash = Cash;
	Player.Upkeep = DirtbagConvert::FromSim(U);
	Player.Climber.Psyche = C.psyche;
	MedicalNews = TEXT("An hour of saying it out loud. It helps, which is "
	                   "annoying.");
	return true;
}

// --- what is wrong with you --------------------------------------------

FString UDirtbagGameInstance::MedicalLine() const
{
	return FString(dirtbag::MedicalText(
	                   DirtbagConvert::ToSim(Player.Medical),
	                   DirtbagConvert::ToSim(Player.Climber), Player.Day)
	                   .c_str());
}

FString UDirtbagGameInstance::BodyHistoryLine() const
{
	return FString(dirtbag::HistoryText(
	                   DirtbagConvert::ToSim(Player.Medical), Player.Day)
	                   .c_str());
}

double UDirtbagGameInstance::PriceOf(EDirtbagTreatment What) const
{
	const dirtbag::MedicalDials MD;
	const dirtbag::Medical Med = DirtbagConvert::ToSim(Player.Medical);
	double List = 0.0;
	switch (What)
	{
	case EDirtbagTreatment::Cortisone: List = MD.cortisoneCost; break;
	case EDirtbagTreatment::Surgery: List = MD.surgeryCost; break;
	case EDirtbagTreatment::Physio: List = dirtbag::BodyDials{}.physioCost; break;
	case EDirtbagTreatment::Rest:
	default: List = 0.0; break;
	}
	return dirtbag::BillFor(Med, List, Player.Day, MD);
}

double UDirtbagGameInstance::PriceOfLook(bool bScan) const
{
	const dirtbag::MedicalDials MD;
	return dirtbag::BillFor(DirtbagConvert::ToSim(Player.Medical),
	                        bScan ? MD.scanCost : MD.guessCost, Player.Day,
	                        MD);
}

namespace
{
// One place that pushes a mutated medical file back onto the player, so a
// door cannot half-apply a change. Every verb below goes through it.
void PutBack(FDirtbagPlayerState& P, const dirtbag::Medical& Med,
             const dirtbag::Climber& C)
{
	P.Medical = DirtbagConvert::FromSim(Med);
	P.Climber.Injury = DirtbagConvert::FromSim(C.injury);
}
}  // namespace

bool UDirtbagGameInstance::SeeSomebody()
{
	dirtbag::Medical Med = DirtbagConvert::ToSim(Player.Medical);
	const dirtbag::Climber C = DirtbagConvert::ToSim(Player.Climber);
	double Cash = Player.Cash;
	if (!dirtbag::Diagnose(Med, C, Cash, dirtbag::Diagnosis::Guessed,
	                       dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)),
	                       Player.Day))
	{
		return false;
	}
	Player.Cash = Cash;
	PutBack(Player, Med, C);
	MedicalNews = MedicalLine();
	return true;
}

bool UDirtbagGameInstance::GetItScanned()
{
	dirtbag::Medical Med = DirtbagConvert::ToSim(Player.Medical);
	const dirtbag::Climber C = DirtbagConvert::ToSim(Player.Climber);
	double Cash = Player.Cash;
	if (!dirtbag::Diagnose(Med, C, Cash, dirtbag::Diagnosis::Scanned,
	                       dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)),
	                       Player.Day))
	{
		return false;
	}
	Player.Cash = Cash;
	PutBack(Player, Med, C);
	MedicalNews = MedicalLine();
	return true;
}

bool UDirtbagGameInstance::TakeTheShot()
{
	dirtbag::Medical Med = DirtbagConvert::ToSim(Player.Medical);
	dirtbag::Climber C = DirtbagConvert::ToSim(Player.Climber);
	double Cash = Player.Cash;
	if (!dirtbag::TakeTheShot(Med, C, Cash, Player.Day)) { return false; }
	Player.Cash = Cash;
	PutBack(Player, Med, C);
	MedicalNews = TEXT("It stops hurting almost at once. That is the "
	                   "problem with it.");
	return true;
}

bool UDirtbagGameInstance::BookTheSurgery()
{
	dirtbag::Medical Med = DirtbagConvert::ToSim(Player.Medical);
	dirtbag::Climber C = DirtbagConvert::ToSim(Player.Climber);
	double Cash = Player.Cash;
	if (!dirtbag::HaveSurgery(Med, C, Cash, Player.Day)) { return false; }
	Player.Cash = Cash;
	PutBack(Player, Med, C);
	MedicalNews = TEXT("Booked. That is most of a season, and it is the "
	                   "only thing that takes it off the joint.");
	return true;
}

bool UDirtbagGameInstance::ComebackStageIsDone() const
{
	return dirtbag::StageIsDone(DirtbagConvert::ToSim(Player.Medical),
	                            Player.Day);
}

bool UDirtbagGameInstance::PushOn()
{
	dirtbag::Medical Med = DirtbagConvert::ToSim(Player.Medical);
	dirtbag::Climber C = DirtbagConvert::ToSim(Player.Climber);
	if (Med.stage == dirtbag::Comeback::Clear) { return false; }
	const dirtbag::Comeback Was = Med.stage;
	const bool SetBack = dirtbag::NextStage(
	    Med, C, dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)), Player.Day);
	PutBack(Player, Med, C);
	MedicalNews =
	    SetBack
	        ? FString(TEXT("That was too soon. You are back where you "
	                       "started, and it is worse."))
	    : Med.stage == dirtbag::Comeback::Clear
	        ? FString(TEXT("Cleared to climb."))
	        : FString::Printf(TEXT("%s."),
	                          UTF8_TO_TCHAR(
	                              dirtbag::ComebackName(Med.stage)));
	(void)Was;
	return !SetBack;
}

bool UDirtbagGameInstance::BuyInsurance()
{
	dirtbag::Medical Med = DirtbagConvert::ToSim(Player.Medical);
	if (!dirtbag::BuyInsurance(Med, DirtbagConvert::ToSim(Player.Climber),
	                           Player.Day))
	{
		return false;
	}
	Player.Medical = DirtbagConvert::FromSim(Med);
	MedicalNews = TEXT("Covered, in a month. Not before.");
	return true;
}

void UDirtbagGameInstance::CancelInsurance()
{
	dirtbag::Medical Med = DirtbagConvert::ToSim(Player.Medical);
	dirtbag::CancelInsurance(Med);
	Player.Medical = DirtbagConvert::FromSim(Med);
}

FString UDirtbagGameInstance::InsuranceLine() const
{
	const dirtbag::Medical Med = DirtbagConvert::ToSim(Player.Medical);
	const dirtbag::MedicalDials MD;
	if (!Med.insured)
	{
		return Med.premiumsPaid > 0.0
		           ? FString::Printf(
		                 TEXT("No cover.  You paid in $%.0f and got $%.0f "
		                      "back."),
		                 Med.premiumsPaid, Med.claimsPaid)
		           : FString(TEXT("No cover."));
	}
	if (!dirtbag::CoverIsLive(Med, Player.Day, MD))
	{
		return FString::Printf(
		    TEXT("Covered in %d days."),
		    MD.waitingDays - (Player.Day - Med.insuredOnDay));
	}
	return FString::Printf(
	    TEXT("Covered.  $%.0f a fortnight; $%.0f paid in, $%.0f back."),
	    MD.premium, Med.premiumsPaid, Med.claimsPaid);
}

// --- the league --------------------------------------------------------

FString UDirtbagGameInstance::LeagueLine() const
{
	return FString(dirtbag::LeagueLine(DirtbagConvert::ToSim(Player.League),
	                                   Player.Day)
	                   .c_str());
}

bool UDirtbagGameInstance::LeagueIsTonight() const
{
	return dirtbag::LeagueTonight(DirtbagConvert::ToSim(Player.League),
	                              Player.Day);
}

FString UDirtbagGameInstance::LeagueStandingLine() const
{
	const dirtbag::League L = DirtbagConvert::ToSim(Player.League);
	if (L.nights <= 0)
	{
		return FString();
	}
	const std::vector<dirtbag::CircuitStanding> Table =
	    dirtbag::LeagueTable(L);
	int32 Place = 0;
	for (int32 i = 0; i < static_cast<int32>(Table.size()); i++)
	{
		if (Table[i].isYou) { Place = i + 1; }
	}
	FString Line = FString::Printf(
	    TEXT("League block %d: %d of %d after %d week%s.  Best: %.0f"),
	    L.block, Place, static_cast<int32>(Table.size()), L.weeksDone,
	    L.weeksDone == 1 ? TEXT("") : TEXT("s"), L.best);
	if (L.blockWins > 0)
	{
		Line += FString::Printf(TEXT("  (%d block%s won)"), L.blockWins,
		                        L.blockWins == 1 ? TEXT("") : TEXT("s"));
	}
	return Line;
}

bool UDirtbagGameInstance::EnterLeague()
{
	if (Comp.bActive || !LeagueIsTonight())
	{
		return false;
	}
	const dirtbag::LeagueDials LD;
	if (Player.Cash < LD.nightFee)
	{
		return false;
	}
	Player.Cash -= LD.nightFee;
	PassHours(LD.nightHours);
	Day.Energy = FMath::Max(0.0, Day.Energy - LD.nightEnergy);

	GLiveComp = dirtbag::SetTheLeagueBoard(
	    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)), AllroundGrade(),
	    Player.Day, LD);
	Comp = FDirtbagCompReadout{};
	Comp.bActive = true;
	Comp.Stage = EDirtbagStage::League;
	Comp.Where = TEXT("League night");
	RefreshComp();
	return true;
}

bool UDirtbagGameInstance::SettleTheLeague()
{
	const dirtbag::LeagueDials LD;
	dirtbag::League L = DirtbagConvert::ToSim(Player.League);
	const dirtbag::LeagueResult Res = dirtbag::SettleLeague(
	    L, GLiveComp, AllroundGrade(), Player.Day,
	    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed) +
	                           std::string("#league#") +
	                           std::to_string(Player.Day)),
	    LD);
	Player.League = DirtbagConvert::FromSim(L);
	Player.Cash += Res.cash;
	if (Res.rep > 0.0)
	{
		dirtbag::Standing S = DirtbagConvert::ToSim(Player.Standing);
		dirtbag::Shift(S, dirtbag::Faction::Scene, Res.rep);
		Player.Standing = DirtbagConvert::FromSim(S);
	}
	LeagueNews = FString(Res.news.c_str());

	// **No ranking points and no circuit points.** The whole reason a
	// league is not a small comp: what it moves is your own number.
	Comp.bSettled = true;
	Comp.Placing = FString::Printf(
	    TEXT("%d of %d, %.0f points."), Res.place, Res.fieldSize, Res.score);
	Comp.Board.Reset(Res.board.size());
	for (const dirtbag::CompEntrant& E : Res.board)
	{
		Comp.Board.Add(FString::Printf(TEXT("%s%s   %.0f"),
		                               E.isYou ? TEXT("> ") : TEXT("  "),
		                               UTF8_TO_TCHAR(E.name.c_str()),
		                               E.score));
	}
	RefreshComp();
	return true;
}

// --- the top of the ladder ---------------------------------------------

FString UDirtbagGameInstance::WorldCupLine() const
{
	return FString(dirtbag::WorldCupLine(
	                   DirtbagConvert::ToSim(Player.WorldCup), Player.Day)
	                   .c_str());
}

int32 UDirtbagGameInstance::DaysUntilWorldCupRound() const
{
	return dirtbag::DaysUntilRound(DirtbagConvert::ToSim(Player.WorldCup),
	                               Player.Day);
}

FDirtbagFlightCheck UDirtbagGameInstance::CanFlyToday() const
{
	return DirtbagConvert::FromSim(
	    dirtbag::CanFly(DirtbagConvert::ToSim(Player.WorldCup),
	                    DirtbagConvert::ToSim(Player.Team), Player.Cash,
	                    Player.Day));
}

FDirtbagWorldCupVenue UDirtbagGameInstance::RoundVenue() const
{
	const dirtbag::WorldCupSeason S = DirtbagConvert::ToSim(Player.WorldCup);
	const int Round = dirtbag::RoundToday(S, Player.Day);
	if (Round < 0)
	{
		return FDirtbagWorldCupVenue{};
	}
	return DirtbagConvert::FromSim(
	    dirtbag::TheVenues()[S.schedule[Round].venue]);
}

bool UDirtbagGameInstance::FlyToTheRound()
{
	if (Comp.bActive)
	{
		return false;
	}
	const dirtbag::WorldCupSeason S = DirtbagConvert::ToSim(Player.WorldCup);
	const dirtbag::FlightCheck Check =
	    dirtbag::CanFly(S, DirtbagConvert::ToSim(Player.Team), Player.Cash,
	                    Player.Day);
	if (!Check.can)
	{
		return false;
	}

	const dirtbag::WorldStageDials WD;
	// The ticket, and then the day. The federation covers the entry; it
	// does not cover the flight, which is the whole of what makes a season
	// a budget problem.
	Player.Cash -= Check.cost;
	PassHours(dirtbag::CompDials{}.hours + WD.travelHours);
	// Airports, time zones, a bad night on a hotel mattress -- on top of
	// what the comp itself takes.
	Day.Energy = FMath::Max(
	    0.0, Day.Energy - dirtbag::CompDials{}.energy - WD.tripEnergy);
	Day.Hunger = FMath::Min(100.0, Day.Hunger + WD.tripHunger);

	GLiveComp = dirtbag::SetTheWorldBoard(
	    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)), Player.Day, WD);
	Comp = FDirtbagCompReadout{};
	Comp.bActive = true;
	Comp.Stage = EDirtbagStage::WorldCup;
	const dirtbag::WorldCupVenue& V =
	    dirtbag::TheVenues()[S.schedule[Check.round].venue];
	Comp.Where = FString::Printf(TEXT("%s, %s"), UTF8_TO_TCHAR(V.city),
	                             UTF8_TO_TCHAR(V.country));
	RefreshComp();
	return true;
}

FString UDirtbagGameInstance::GamesLine() const
{
	return FString(dirtbag::GamesLine(DirtbagConvert::ToSim(Player.Olympics),
	                                  Player.RankingPoints, Player.Day)
	                   .c_str());
}

bool UDirtbagGameInstance::GamesAreToday() const
{
	return dirtbag::GamesToday(DirtbagConvert::ToSim(Player.Olympics),
	                           Player.Day);
}

FString UDirtbagGameInstance::WhyNotTheGames() const
{
	return FString(dirtbag::CanEnterTheGames(
	                   DirtbagConvert::ToSim(Player.Olympics),
	                   Player.RankingPoints, Player.Day)
	                   .why.c_str());
}

bool UDirtbagGameInstance::EnterTheGames()
{
	if (Comp.bActive)
	{
		return false;
	}
	const dirtbag::Olympics O = DirtbagConvert::ToSim(Player.Olympics);
	if (!dirtbag::CanEnterTheGames(O, Player.RankingPoints, Player.Day).can)
	{
		return false;
	}
	const dirtbag::WorldStageDials WD;
	// No ticket and no entry fee. **You are on the national team and this
	// is what the national team is for** -- the cost of being here was the
	// twelve hundred ranking points it took to qualify.
	PassHours(dirtbag::CompDials{}.hours + WD.travelHours);
	Day.Energy = FMath::Max(
	    0.0, Day.Energy - dirtbag::CompDials{}.energy - WD.tripEnergy);
	Day.Hunger = FMath::Min(100.0, Day.Hunger + WD.tripHunger);

	GLiveComp = dirtbag::SetTheOlympicBoard(
	    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)), Player.Day, WD);
	Comp = FDirtbagCompReadout{};
	Comp.bActive = true;
	Comp.Stage = EDirtbagStage::Games;
	Comp.Where = TEXT("The Games");
	RefreshComp();
	return true;
}

bool UDirtbagGameInstance::SettleTheWorldStage()
{
	const dirtbag::WorldStageDials WD;
	const bool bGames = Comp.Stage == EDirtbagStage::Games;
	const dirtbag::Rng SettleRng = dirtbag::Rng::FromSeed(
	    TCHAR_TO_UTF8(*Seed) + std::string(bGames ? "#games#" : "#wc#") +
	    std::to_string(Player.Day));

	const dirtbag::CompResult Res =
	    bGames ? dirtbag::SettleTheGames(GLiveComp, SettleRng, WD)
	           : dirtbag::SettleWorldRound(GLiveComp, SettleRng, WD);

	if (bGames)
	{
		dirtbag::Olympics O = DirtbagConvert::ToSim(Player.Olympics);
		const dirtbag::Medal M = dirtbag::MedalFor(Res.place);
		// Keyed on the day the Games were held, which is unique per cycle
		// and never zero -- so a Games cannot be entered twice and a
		// default `lastCompeted` of -1 never collides with one.
		dirtbag::BankTheGames(O, M, O.nextDay);
		Player.Olympics = DirtbagConvert::FromSim(O);
		GamesNews =
		    M.gold     ? TEXT("Gold. There is nothing above this.")
		    : M.silver ? TEXT("Silver. One person climbed better today.")
		    : M.bronze ? TEXT("Bronze. You are on the podium at the Games.")
		               : FString::Printf(
		                     TEXT("%d%s at the Games. You were there."),
		                     Res.place,
		                     Res.place == 1   ? TEXT("st")
		                     : Res.place == 2 ? TEXT("nd")
		                     : Res.place == 3 ? TEXT("rd")
		                                      : TEXT("th"));
	}
	else
	{
		dirtbag::WorldCupSeason S = DirtbagConvert::ToSim(Player.WorldCup);
		const int Round = dirtbag::RoundToday(S, Player.Day);
		dirtbag::BankRound(S, Round, &Res, SettleRng, WD);
		Player.WorldCup = DirtbagConvert::FromSim(S);
	}

	// **No prize money and no ranking points.** The World Cup pays in the
	// world table and the Games pay in a medal; the domestic ranking is
	// what got you here and is not what this is for. Said here rather than
	// left implicit, because the domestic branch does the opposite three
	// lines up and the difference is deliberate.

	Comp.bSettled = true;
	Comp.Placing = FString(dirtbag::PlacingText(Res).c_str());
	Comp.Board.Reset(Res.board.size());
	for (const dirtbag::CompEntrant& E : Res.board)
	{
		Comp.Board.Add(FString::Printf(TEXT("%s%s   %.0f"),
		                               E.isYou ? TEXT("> ") : TEXT("  "),
		                               UTF8_TO_TCHAR(E.name.c_str()),
		                               E.score));
	}
	RefreshComp();
	return true;
}

void UDirtbagGameInstance::RecordResult(double Points)
{
	dirtbag::PlayerState P = DirtbagConvert::ToSim(Player);
	dirtbag::Record(P.rankingRecord, Player.Day, Points);
	Player.RankingRecord.Reset(P.rankingRecord.size());
	for (const dirtbag::RankingResult& R : P.rankingRecord)
	{
		Player.RankingRecord.Add(DirtbagConvert::FromSim(R));
	}
	// Refreshed now as well as tonight, so the tier a comp just moved you
	// into is the tier the next line of text reads. The night tick is what
	// makes results *age out*; this is what makes them count today.
	Player.RankingPoints =
	    dirtbag::RankingFrom(P.rankingRecord, Player.Day);
}

dirtbag::BodyContext UDirtbagGameInstance::TheBodyYouWalkedInWith() const
{
	dirtbag::BodyContext Body;
	Body.who = DirtbagConvert::ToSim(Player.Character);
	Body.medical = DirtbagConvert::ToSim(Player.Medical);
	Body.sickness = DirtbagConvert::ToSim(Player.Sickness);
	Body.teeth = DirtbagConvert::ToSim(Player.Teeth);
	Body.shoeWear = Player.Shoes.Wear;
	Body.day = Player.Day;
	return Body;
}

double UDirtbagGameInstance::AllroundGrade() const
{
	const FDirtbagClimber& C = Player.Climber;
	return dirtbag::SkillToGrade(
	    (C.Power + C.Fingers + C.Technique + C.Endurance + C.Head) / 5.0);
}

double UDirtbagGameInstance::ShopPrice() const
{
	return dirtbag::ShopPriceMultiplier(
	    DirtbagConvert::ToSim(Player.Character));
}

void UDirtbagGameInstance::BeginCreation()
{
	// Already answered. A loaded career walks straight past this, and so
	// does a second call.
	if (Player.Character.bBuilt)
	{
		Creation.bActive = false;
		return;
	}
	Creation = FDirtbagCreationReadout{};
	Creation.bActive = true;
	Creation.Step = EDirtbagCreationStep::Archetype;
	RefreshCreation();
}

bool UDirtbagGameInstance::ChooseInCreation(int32 Which)
{
	FDirtbagCreationReadout& C = Creation;
	if (!C.bActive || C.Step == EDirtbagCreationStep::Done)
	{
		return false;
	}
	if (!C.Options.IsValidIndex(Which))
	{
		// The key belonged to creation even though it named nobody --
		// otherwise pressing 5 at a four-way question would fall through to
		// whatever else is listening.
		return true;
	}

	switch (C.Step)
	{
	case EDirtbagCreationStep::Archetype:
		Player.Character.Archetype = static_cast<EDirtbagArchetype>(Which);
		C.Step = EDirtbagCreationStep::Origin;
		break;
	case EDirtbagCreationStep::Origin:
		Player.Character.Origin = static_cast<EDirtbagOrigin>(Which);
		C.Step = EDirtbagCreationStep::Flaw;
		break;
	case EDirtbagCreationStep::Flaw:
		Player.Character.Flaw = static_cast<EDirtbagFlaw>(Which);
		C.Step = EDirtbagCreationStep::Temperament;
		break;
	case EDirtbagCreationStep::Temperament:
		Player.Character.Temperament =
		    static_cast<EDirtbagTemperament>(Which);
		C.Step = EDirtbagCreationStep::Quirk;
		break;
	default:
	{
		// The picked quirk. Through the sim's own Pick, which refuses an
		// earned one -- choosing to be obsessive at the counter is not the
		// same thing as becoming it over two seasons, and that difference
		// is the whole rule the habits file is built on.
		{
			dirtbag::Quirks Q = DirtbagConvert::ToSim(Player.Quirks);
			const int First = static_cast<int>(dirtbag::Quirk::LightSleeper);
			dirtbag::Pick(Q, static_cast<dirtbag::Quirk>(First + Which));
			Player.Quirks = DirtbagConvert::FromSim(Q);
		}

		// Everything answered, so the sim builds the person: the archetype's
		// shape, the origin's life, the temperament leaned by where you came
		// from, and **the two talents you do not get told about.**
		dirtbag::Build Build;
		Build.archetype =
		    static_cast<dirtbag::Archetype>(Player.Character.Archetype);
		Build.origin = static_cast<dirtbag::Origin>(Player.Character.Origin);
		Build.flaw = static_cast<dirtbag::Flaw>(Player.Character.Flaw);
		Build.temperament =
		    static_cast<dirtbag::Temperament>(Player.Character.Temperament);

		const dirtbag::Rng World =
		    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed));
		const dirtbag::Character Made = dirtbag::MakeCharacter(Build, World);
		Player.Character = DirtbagConvert::FromSim(Made);
		Player.Climber =
		    DirtbagConvert::FromSim(dirtbag::MakeClimber(Build, World));
		// The origin's money is what you arrive with, not a bonus on top of
		// the old flat start -- otherwise a Trust-Fund Kid gets $820.
		Player.Cash = Made.startingCash;

		C.WhoYouAre = FString(dirtbag::WhoYouAre(Made).c_str());
		C.Step = EDirtbagCreationStep::Done;
		C.Options.Reset();
		C.Blurbs.Reset();
		// Written down straight away: four answers is enough of a decision
		// that losing it to an alt-F4 would be a real annoyance.
		SaveNow();
		return true;
	}
	}
	RefreshCreation();
	return true;
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
	// **What is going in the valley, minus the trades that will not have
	// you.** A sacking has to show up as work you cannot take, or it is a
	// number in a menu -- see Sim/DirtbagCraft.h.
	TArray<FDirtbagOddJob> All =
	    UDirtbagSimLibrary::OddJobBoard(Seed, Player.Day);
	const dirtbag::Craftsman Hand = DirtbagConvert::ToSim(Player.Hand);
	TArray<FDirtbagOddJob> Open;
	Open.Reserve(All.Num());
	for (const FDirtbagOddJob& J : All)
	{
		if (dirtbag::WillTheyHireYou(
		        Hand, dirtbag::CraftForGig(TCHAR_TO_UTF8(*J.Name))))
		{
			Open.Add(J);
		}
	}
	return Open;
}

namespace
{
// The shift's decision, derived rather than stored -- it is deterministic
// on the day and the trade, so asking twice gives the same answer and a
// reload does not reroll it.
dirtbag::ShiftMoment MomentFor(const FString& Seed, int32 Day,
                               const FDirtbagOddJob& Job)
{
	return dirtbag::MomentOnShift(
	    dirtbag::CraftForGig(TCHAR_TO_UTF8(*Job.Name)),
	    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)), Day);
}
}  // namespace

FString UDirtbagGameInstance::ShiftMomentLine(const FDirtbagOddJob& Job) const
{
	const dirtbag::ShiftMoment M = MomentFor(Seed, Player.Day, Job);
	return M.happened ? FString(UTF8_TO_TCHAR(M.what)) : FString();
}

FString UDirtbagGameInstance::TheHardWayLine(const FDirtbagOddJob& Job) const
{
	const dirtbag::ShiftMoment M = MomentFor(Seed, Player.Day, Job);
	if (!M.happened) { return FString(); }
	// **What it needs is not shown as a number.** You know whether you can
	// do your job; a threshold on the prompt would turn a decision into a
	// skill check you can read off the wall.
	return FString(UTF8_TO_TCHAR(M.theHardWay));
}

void UDirtbagGameInstance::TakeTheGigTheHardWay()
{
	bTakeTheHardWay = true;
}

FString UDirtbagGameInstance::CraftLine() const
{
	return FString(
	    dirtbag::CraftText(DirtbagConvert::ToSim(Player.Hand)).c_str());
}

FString UDirtbagGameInstance::TradeLine() const
{
	return FString(dirtbag::TradeText(DirtbagConvert::ToSim(Player.Hand),
	                                  AllroundGrade())
	                   .c_str());
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
	const dirtbag::ShiftMoment Moment = MomentFor(Seed, Player.Day, Job);
	const int32 BotchedBefore = Player.Hand.MomentsBotched;
	const bool bWasSacked = Player.Hand.Sacked.IsValidIndex(
	                            static_cast<int32>(dirtbag::CraftForGig(
	                                TCHAR_TO_UTF8(*Job.Name)))) &&
	                        Player.Hand.Sacked[static_cast<int32>(
	                            dirtbag::CraftForGig(
	                                TCHAR_TO_UTF8(*Job.Name)))];
	// The shift's own decision, answered by the player -- see
	// `TakeTheGigTheHardWay`. Defaults to the easy answer, which is never
	// wrong and never gets you anywhere.
	if (!dirtbag::WorkOddJob(SimPlayer, SimDay, SimJob,
	                         dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)),
	                         bTakeTheHardWay))
	{
		return false;
	}
	const bool bWentHard = bTakeTheHardWay;
	bTakeTheHardWay = false;
	Player = DirtbagConvert::FromSim(SimPlayer);
	Day = DirtbagConvert::FromSim(SimDay);
	bWorkedToday = true;

	// How the shift went, read across the call rather than plumbed
	// through -- the same way every other piece of news in this game
	// works.
	WorkNews.Reset();
	if (Moment.happened && bWentHard)
	{
		const bool bBotched = Player.Hand.MomentsBotched > BotchedBefore;
		WorkNews = FString::Printf(
		    TEXT("%s  %s"), UTF8_TO_TCHAR(Moment.theHardWay),
		    bBotched ? TEXT("It did not go fine.") : TEXT("It went fine."));
		const int32 Which = static_cast<int32>(
		    dirtbag::CraftForGig(TCHAR_TO_UTF8(*Job.Name)));
		if (!bWasSacked && Player.Hand.Sacked.IsValidIndex(Which) &&
		    Player.Hand.Sacked[Which])
		{
			// **And it outlives the job.** The gig comes off your board.
			WorkNews += TEXT("  They will not be calling you again.");
		}
	}

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

FString UDirtbagGameInstance::SalaryLine() const
{
	const dirtbag::JobDials Jd;
	if (Player.Job.bSalaried)
	{
		return FString::Printf(
		    TEXT("You have the job.  (J) leaves it, and it will sting."));
	}
	// What it pays and what it takes, in one line, with neither half
	// softened. `DirtbagJobs.h`: *"Nothing in here punishes it -- the
	// numbers simply are what they are."* A warning would be the game
	// arguing with the player about their own life.
	FString Line = FString::Printf(
	    TEXT("Permanent position going.  (J)  -  $%.0f a week, %d days, "
	         "%.0f to %.0f."),
	    Jd.salaryPerWeek, Jd.salaryDaysPerWeek, Jd.salaryStartHour,
	    Jd.salaryStartHour + Jd.salaryHours);
	// And what it costs that money cannot buy back, when there is one.
	// Said as a fact about the streak rather than as advice.
	if (Player.Job.DaysSinceSalary >= 30)
	{
		Line += FString::Printf(TEXT("  It would end a run of %d days."),
		                        Player.Job.DaysSinceSalary);
	}
	return Line;
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

FString UDirtbagGameInstance::PhysioLine() const
{
	if (!IsHurt())
	{
		return FString();
	}
	const dirtbag::BodyDials Bd;
	const int32 Since = Player.Day - Player.LastPhysioDay;
	if (Player.LastPhysioDay > 0 && Since < Bd.physioDaysBetween)
	{
		// Said as the reason rather than as a refusal. You cannot buy your
		// way out of a season in an afternoon, and the game should sound
		// like a physio saying so rather than like a locked door.
		return FString::Printf(
		    TEXT("The physio wants %d more day%s before they see you again."),
		    Bd.physioDaysBetween - Since,
		    Bd.physioDaysBetween - Since == 1 ? TEXT("") : TEXT("s"));
	}
	if (Player.Cash < Bd.physioCost)
	{
		return FString::Printf(TEXT("A physio is $%.0f. You have $%.0f."),
		                       Bd.physioCost, Player.Cash);
	}
	// The trade stated in the only units that matter. Days, not "recovery".
	return FString::Printf(
	    TEXT("See a physio?  (P)  -  $%.0f, and about %d days off it."),
	    Bd.physioCost, Bd.physioDaysSaved);
}

bool UDirtbagGameInstance::SeeAPhysio()
{
	dirtbag::Climber SimClimber = DirtbagConvert::ToSim(Player.Climber);
	double Cash = Player.Cash;
	int LastDay = Player.LastPhysioDay;
	// What the physio charges *you*. The Late Bloomer's lane, and the only
	// origin that touches it.
	const double PhysioPrice =
	    dirtbag::PhysioPriceMultiplier(DirtbagConvert::ToSim(Player.Character));
	if (!dirtbag::Physio(SimClimber, Cash, LastDay, Player.Day,
	                     dirtbag::BodyDials{}, PhysioPrice))
	{
		return false;
	}
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
	// What the counter charges *you*. The Trust-Fund Kid's lane, and nobody
	// else's -- read once here rather than inside the sim, because a price
	// is a fact about the shop and who is standing at it.
	const double Price = ShopPrice();
	return BuyWith(Player.Kit, Player.Cash,
	               [Price](dirtbag::Kit& K, double& M)
	               { return dirtbag::BuyPad(K, M, dirtbag::KitDials{}, Price); });
}

bool UDirtbagGameInstance::BuyHangboard()
{
	const double Price = ShopPrice();
	return BuyWith(Player.Kit, Player.Cash,
	               [Price](dirtbag::Kit& K, double& M) {
		return dirtbag::BuyHangboard(K, M, dirtbag::KitDials{}, Price);
	});
}

bool UDirtbagGameInstance::BuyTradRack()
{
	const double Price = ShopPrice();
	dirtbag::Rack SimRack = DirtbagConvert::ToSim(Player.Rack);
	double Money = Player.Cash;
	if (!dirtbag::BuyRack(SimRack, Money, dirtbag::TradDials{}, Price))
	{
		return false;
	}
	Player.Rack = DirtbagConvert::FromSim(SimRack);
	Player.Cash = Money;
	return true;
}

FString UDirtbagGameInstance::HabitDoingLine() const
{
	return UTF8_TO_TCHAR(
	    dirtbag::DoingLabel(DirtbagConvert::ToSim(Player.Logbook), Player.Day)
	        .c_str());
}

FString UDirtbagGameInstance::HabitAreLine() const
{
	return UTF8_TO_TCHAR(
	    dirtbag::AreLabel(DirtbagConvert::ToSim(Player.Quirks)).c_str());
}

FString UDirtbagGameInstance::HowYouClimbLine() const
{
	return UTF8_TO_TCHAR(
	    dirtbag::HowYouClimb(DirtbagConvert::ToSim(Player.Quirks),
	                         DirtbagConvert::ToSim(Player.Logbook), Player.Day)
	        .c_str());
}

FString UDirtbagGameInstance::MembershipLine() const
{
	const dirtbag::KitDials Kd;
	const int32 Left = Player.Kit.MembershipDaysLeft;
	if (Left > 0)
	{
		return FString::Printf(
		    TEXT("Gym membership: %d day%s left.  (M) tops it up, $%.0f."),
		    Left, Left == 1 ? TEXT("") : TEXT("s"), Kd.membershipCost);
	}
	return FString::Printf(
	    TEXT("Not a member.  (M)  -  $%.0f for %d days of weather nobody "
	         "can take off you."),
	    Kd.membershipCost, Kd.membershipDays);
}

FString UDirtbagGameInstance::HangboardLine() const
{
	if (Player.Kit.bHangboard)
	{
		return FString();
	}
	const dirtbag::KitDials Kd;
	return FString::Printf(
	    TEXT("A hangboard?  (H)  -  $%.0f, once, and it lives in the van."),
	    Kd.hangboardCost);
}

FString UDirtbagGameInstance::RackOfferLine() const
{
	const dirtbag::TradDials Td;
	const dirtbag::Rack Have = DirtbagConvert::ToSim(Player.Rack);
	const dirtbag::RackTier Tier = dirtbag::TierOf(Have, Td);
	if (Tier == dirtbag::RackTier::Doubles)
	{
		return FString();
	}
	const dirtbag::RackTier Want =
	    static_cast<dirtbag::RackTier>(static_cast<int>(Tier) + 1);
	// The first one is the one that matters, because until you own a rack
	// there is a whole crag you cannot go to. After that it is an upgrade
	// like any other and the line says so.
	return Tier == dirtbag::RackTier::None
	           ? FString::Printf(
	                 TEXT("%s?  (G)  -  $%.0f, and the buttress opens."),
	                 UTF8_TO_TCHAR(dirtbag::RackTierName(Want)),
	                 dirtbag::RackPrice(Want, Td) * ShopPrice())
	           : FString::Printf(
	                 TEXT("%s?  (G)  -  $%.0f.  You lead on %s."),
	                 UTF8_TO_TCHAR(dirtbag::RackTierName(Want)),
	                 dirtbag::RackPrice(Want, Td) * ShopPrice(),
	                 UTF8_TO_TCHAR(dirtbag::RackTierName(Tier)));
}

bool UDirtbagGameInstance::RenewGymMembership()
{
	const double Price = ShopPrice();
	return BuyWith(Player.Kit, Player.Cash,
	               [Price](dirtbag::Kit& K, double& M) {
		return dirtbag::RenewMembership(K, M, dirtbag::KitDials{}, Price);
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

	// **And the rival, who goes for the hard ones.**
	//
	// Through the Lot's own machinery rather than a roll of their own:
	// `AsAClimber` hands back a `Partner` built from their grade, so the
	// same `PartnerTakesFirstAscent` and `TheyPutUpTheLine` that write a
	// neighbour's ascent into the book write theirs. Two paths to "somebody
	// got there first" would drift, and the drift shows up as a guidebook
	// that knows about one and not the other.
	{
		dirtbag::Rival R = DirtbagConvert::ToSim(Player.Rival);
		const bool bWasMet = R.met;
		if (!R.name.empty() && !R.retired && !R.allied)
		{
			dirtbag::Partner Them =
			    dirtbag::AsAClimber(R, World, Player.Day);
			const int Line = dirtbag::PartnerTakesFirstAscent(
			    World, Them, SimCrag, Taken, Player.Day);
			if (Line >= 0)
			{
				dirtbag::CragLine& Got = SimCrag.lines[Line];
				dirtbag::TheyPutUpTheLine(Got, R.name);
				dirtbag::TheyGotThereFirst(R, Got.route.name);
				// Losing a line to somebody is a way of meeting them.
				R.met = true;
				if (!LotNews.IsEmpty())
				{
					LotNews += TEXT("   ");
				}
				// Said differently from a neighbour's, because it is a
				// different thing: a neighbour got lucky, this one was
				// after it.
				LotNews += FString::Printf(
				    TEXT("%s got to %s first. It has their name on it now."),
				    UTF8_TO_TCHAR(R.name.c_str()),
				    UTF8_TO_TCHAR(Got.description.c_str()));
			}

			// **The clock, before the roll for a new one.** If it ran out
			// they finished it, and an open line is gone -- written into
			// the book by the same path as any other ascent of theirs, so
			// there is no version of this the guidebook does not know
			// about.
			if (dirtbag::RaceRanOut(R, Player.Day))
			{
				const FString Line =
				    FString(R.race.routeName.c_str());
				const bool bWasFa = R.race.forFirstAscent;
				if (bWasFa)
				{
					for (dirtbag::CragLine& L : SimCrag.lines)
					{
						if (L.route.name == R.race.routeName)
						{
							dirtbag::TheyPutUpTheLine(L, R.name);
							break;
						}
					}
				}
				dirtbag::TheyWonTheRace(R);
				RivalNews = FString::Printf(
				    bWasFa ? TEXT("%s did %s. It was never yours to lose, "
				                  "and it is theirs now.")
				           : TEXT("%s did %s. You had five days."),
				    UTF8_TO_TCHAR(R.name.c_str()), *Line);
			}
			else if (!dirtbag::RaceIsOn(R))
			{
				// **A line with a deadline on it**, which is the whole
				// answer to "does the rival change what you climb". Rolled
				// against the same off-limits list the FA path uses, so
				// they cannot race you for something already claimed.
				std::vector<std::string> Off = Taken;
				for (const std::string& S :
				     dirtbag::SpokenFor(DirtbagConvert::ToSim(Player).projects))
				{
					Off.push_back(S);
				}
				if (dirtbag::StartARace(R, SimCrag, AllroundGrade(), Off,
				                        World, Player.Day))
				{
					RivalNews = FString(
					    dirtbag::RaceLine(R, Player.Day).c_str());
				}
			}

			// They come around, once, when the head-to-head says you have
			// earned it. **Offered, not taken** -- the answer is C or F at
			// the van, like every other choice in this build.
			if (dirtbag::WouldPartnerUp(R))
			{
				R.offered = true;
				RivalOffer = FString::Printf(
				    TEXT("%s asked if you wanted to rope up."),
				    UTF8_TO_TCHAR(R.name.c_str()));
			}
		}
		// **Introduced.** Before this they were a name and a number; you
		// find out what they are by losing a line to them or taking one
		// off them, which is the only honest way to learn it. Said once,
		// ever, and prepended so the introduction reads before the thing
		// that caused it.
		if (!bWasMet && R.met)
		{
			const FString Intro = FString::Printf(
			    TEXT("%s. %s, and %s."), UTF8_TO_TCHAR(R.name.c_str()),
			    UTF8_TO_TCHAR(dirtbag::StyleText(R.style)),
			    UTF8_TO_TCHAR(dirtbag::VibeText(R.vibe)));
			RivalNews = RivalNews.IsEmpty()
			                ? Intro
			                : Intro + TEXT("   ") + RivalNews;
		}
		Player.Rival = DirtbagConvert::FromSim(R);
	}

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

	// **You got there first.** The head-to-head is what opens the ally arc,
	// and a first ascent is the highest stake this game currently has --
	// their side of it is in AdvanceTheLot, and this is yours.
	{
		dirtbag::Rival R = DirtbagConvert::ToSim(Player.Rival);
		dirtbag::YouGotThereFirst(R);
		// And if this was the line they were on, you beat them to it.
		if (dirtbag::RaceIsOn(R) && R.race.routeName == SimLine.route.name)
		{
			dirtbag::YouWonTheRace(R);
			RivalNews = FString::Printf(
			    TEXT("You got there first. %s will have heard by tonight."),
			    UTF8_TO_TCHAR(R.name.c_str()));
		}
		// Beating somebody to a line is a way of meeting them.
		R.met = true;
		Player.Rival = DirtbagConvert::FromSim(R);
	}

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
