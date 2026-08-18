#include "DirtbagGameInstance.h"

#include "DirtbagSimLibrary.h"

void UDirtbagGameInstance::Init()
{
	Super::Init();

	FString LoadedSeed;
	FDirtbagPlayerState LoadedPlayer;
	const EDirtbagLoadResult Result =
	    UDirtbagSimLibrary::LoadFromFile(SaveFilename, LoadedSeed, LoadedPlayer);
	if (Result == EDirtbagLoadResult::Ok)
	{
		Seed = LoadedSeed;
		Player = LoadedPlayer;
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
	UDirtbagSimLibrary::WorkShift(Player, Day);
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
	UDirtbagSimLibrary::SleepToNextDay(Player, Day);
	SaveNow();
}

bool UDirtbagGameInstance::SaveNow()
{
	return UDirtbagSimLibrary::SaveToFile(Seed, Player, SaveFilename);
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
	if (!bIndoors)
	{
		return GetCragLine(Index).Route;
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
	if (bCragLoaded)
	{
		return;
	}
	Crag = UDirtbagSimLibrary::RoadsideCrag(Seed);
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
	dirtbag::ApplyAttemptToDay(SimPlayer, SimDay, SimRoute, Result);

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
	const FDirtbagRoute Route = GetBoardRoute(BoardIndex);
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
