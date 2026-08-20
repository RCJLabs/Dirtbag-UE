#include "DirtbagGameInstance.h"

#include "DirtbagSimLibrary.h"

void UDirtbagGameInstance::Init()
{
	Super::Init();

	FString LoadedSeed;
	FDirtbagPlayerState LoadedPlayer;
	TArray<dirtbag::Legacy> LoadedLegacies;
	const EDirtbagLoadResult Result = UDirtbagSimLibrary::LoadGameFromFile(
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

	Day = UDirtbagSimLibrary::WakeUp(Player);
}

bool UDirtbagGameInstance::EatMeal()
{
	return UDirtbagSimLibrary::EatMeal(Player, Day);
}

void UDirtbagGameInstance::WorkShift()
{
	// A dog does not come to the belay desk, so a shift is the one thing
	// that leaves it behind for four hours. On a cool day that costs
	// nothing; on a warm one the van is an oven and you knew it.
	const double Guilt =
	    dirtbag::VanGuilt(DirtbagConvert::ToSim(Player.Dog), VanTempF());

	UDirtbagSimLibrary::WorkShift(Player, Day);
	bWorkedToday = true;

	if (Guilt > 0.0)
	{
		Player.Climber.Psyche = FMath::Max(0.05, Player.Climber.Psyche - Guilt);
		DogWorry = FString::Printf(
		    TEXT("You could hear it from the desk. The van hit %.0fF."),
		    VanTempF());
	}
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

	UDirtbagSimLibrary::SleepToNextDay(Seed, Player, Day);
	SaveNow();
}

bool UDirtbagGameInstance::SaveNow()
{
	// The complete path: anything that drops Legacies here disinherits every
	// generation before this one, and nothing about the save would look
	// wrong until somebody opened the guidebook.
	return UDirtbagSimLibrary::SaveGameToFile(Seed, Player, Legacies,
	                                          SaveFilename);
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
		return GetCragLineAt(AtVenue, Index).Route;
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
	const EDirtbagVenue Want =
	    Venue == EDirtbagVenue::Cave ? EDirtbagVenue::Cave : EDirtbagVenue::Crag;
	if (bCragLoaded && LoadedCrag == Want)
	{
		return;
	}
	LoadedCrag = Want;
	Crag = Want == EDirtbagVenue::Cave
	           ? UDirtbagSimLibrary::ShadedCave(Seed)
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

FString UDirtbagGameInstance::ShoeLine() const
{
	return FString(UTF8_TO_TCHAR(
	    dirtbag::ShoeText(DirtbagConvert::ToSim(Player.Shoes)).c_str()));
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

void UDirtbagGameInstance::RetireAndPassItOn(const FString& Name)
{
	const dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	const dirtbag::Legacy L = dirtbag::TallyCareer(
	    SimPlayer, TCHAR_TO_UTF8(*Name), 1 + Player.Day / 365);
	Legacies.Add(L);

	Player = DirtbagConvert::FromSim(dirtbag::Inherit(L));
	Day = UDirtbagSimLibrary::WakeUp(Player);
	ConsecutiveInjuries = 0;
	PeakGradeEver = 0.0;
	ClimberName.Reset();
	SaveNow();
}

int32 UDirtbagGameInstance::GenerationsBefore() const
{
	return Legacies.Num();
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

	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);
	if (!dirtbag::WorkOddJob(SimPlayer, SimDay, SimJob)) return false;
	Player = DirtbagConvert::FromSim(SimPlayer);
	Day = DirtbagConvert::FromSim(SimDay);
	// A shift is a shift: the dog did not come to it either.
	bWorkedToday = true;
	return true;
}

void UDirtbagGameInstance::TakeSalariedJob()
{
	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	dirtbag::TakeSalariedJob(SimPlayer);
	Player = DirtbagConvert::FromSim(SimPlayer);
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
	if (!dirtbag::Resole(S, Cash)) return false;
	Player.Shoes = DirtbagConvert::FromSim(S);
	Player.Cash = Cash;
	return true;
}

bool UDirtbagGameInstance::BuyNewShoes()
{
	dirtbag::Shoes S = DirtbagConvert::ToSim(Player.Shoes);
	double Cash = Player.Cash;
	if (!dirtbag::BuyNewShoes(S, Cash)) return false;
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
	for (dirtbag::Partner& P : Lot)
	{
		// Rapport is per day, so an hour is a fraction of one — you cannot
		// befriend the whole Lot by sitting still for a week.
		dirtbag::PartnerDials Dials;
		P.rapport = FMath::Min(
		    1.0, P.rapport + Dials.rapportPerDay * (Hours / 8.0));
	}
	StoreBonds(Lot);

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
	for (const dirtbag::Partner& P : Lot)
	{
		dirtbag::ProjectMemory Trial = SimLedger;
		const double Gained = dirtbag::ShareBeta(P, SimLine, Trial);
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
	const dirtbag::Crag SimCrag = dirtbag::RoadsideCrag(World);

	// Everything anybody has already claimed, the player included: a line
	// you have done is not still lying around for Dev to take.
	std::vector<std::string> Taken;
	for (const FDirtbagProjectMemory& M : Player.Projects)
	{
		if (M.bFirstAscent)
		{
			Taken.push_back(TCHAR_TO_UTF8(*M.RouteName));
		}
	}
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
		const dirtbag::CragLine& Got = SimCrag.lines[Line];
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
		LotNews += FString::Printf(TEXT("%s got %s."),
		                           UTF8_TO_TCHAR(P.name.c_str()),
		                           UTF8_TO_TCHAR(Got.description.c_str()));
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

	if (!dirtbag::NameFirstAscent(SimLedger, SimLine, TCHAR_TO_UTF8(*Name)))
	{
		return false;
	}
	*Ledger = DirtbagConvert::FromSim(SimLedger);

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
	return FString(UTF8_TO_TCHAR(
	    dirtbag::FirstAscentLine(DirtbagConvert::ToSim(*Ledger), "you")
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
