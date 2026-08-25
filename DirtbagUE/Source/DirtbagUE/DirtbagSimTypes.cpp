#include "DirtbagSimTypes.h"

// The mirrors cast by value between enum sets; if the sim reorders, these
// fail the build instead of silently corrupting data.
static_assert(static_cast<int>(EDirtbagHold::Crack) == static_cast<int>(dirtbag::HoldType::Crack), "Hold enums out of sync");
static_assert(static_cast<int>(EDirtbagRouteType::Crack) == static_cast<int>(dirtbag::RouteType::Crack), "RouteType enums out of sync");
static_assert(static_cast<int>(EDirtbagDiscipline::Trad) == static_cast<int>(dirtbag::Discipline::Trad), "Discipline enums out of sync");
static_assert(static_cast<int>(EDirtbagRackTier::Doubles) == static_cast<int>(dirtbag::RackTier::Doubles), "RackTier enums out of sync");
static_assert(static_cast<int>(EDirtbagHabit::SewsItUp) == static_cast<int>(dirtbag::Habit::SewsItUp), "Habit enums out of sync");
static_assert(static_cast<int>(EDirtbagQuirk::Quiet) == static_cast<int>(dirtbag::Quirk::Quiet), "Quirk enums out of sync");
static_assert(static_cast<int>(EDirtbagBeatKind::Topped) == static_cast<int>(dirtbag::BeatKind::Topped), "BeatKind enums out of sync");
static_assert(static_cast<int>(EDirtbagStyle::Fell) == static_cast<int>(dirtbag::Style::Fell), "Style enums out of sync");
static_assert(static_cast<int>(EDirtbagMorphology::Powerful) == static_cast<int>(dirtbag::Morphology::Powerful), "Morphology enums out of sync");
static_assert(static_cast<int>(EDirtbagRouteRead::NotThisYear) == static_cast<int>(dirtbag::RouteRead::NotThisYear), "RouteRead enums out of sync");
static_assert(static_cast<int>(EDirtbagAspect::West) == static_cast<int>(dirtbag::Aspect::West), "Aspect enums out of sync");
static_assert(static_cast<int>(EDirtbagSessionAdvice::Wrecked) == static_cast<int>(dirtbag::SessionAdvice::Wrecked), "SessionAdvice enums out of sync");
static_assert(static_cast<int>(EDirtbagVanPart::Clutch) == static_cast<int>(dirtbag::VanPart::Clutch), "VanPart enums out of sync");
static_assert(static_cast<int>(EDirtbagService::Work) == static_cast<int>(dirtbag::Service::Work), "Service enums out of sync");
static_assert(static_cast<int>(EDirtbagSponsorTier::Title) == static_cast<int>(dirtbag::SponsorTier::Title), "SponsorTier enums out of sync");
static_assert(static_cast<int>(EDirtbagEthicalAct::PulledOnGear) == static_cast<int>(dirtbag::EthicalAct::PulledOnGear), "EthicalAct enums out of sync");
static_assert(static_cast<int>(EDirtbagFaction::Stewardship) == static_cast<int>(dirtbag::Faction::Stewardship), "Faction enums out of sync");
static_assert(static_cast<int>(EDirtbagInjuryKind::Shoulder) == static_cast<int>(dirtbag::InjuryKind::Shoulder), "InjuryKind enums out of sync");

namespace DirtbagConvert
{

dirtbag::Climber ToSim(const FDirtbagClimber& In)
{
	dirtbag::Climber Out;
	Out.skills.power = In.Power;
	Out.skills.fingers = In.Fingers;
	Out.skills.technique = In.Technique;
	Out.skills.endurance = In.Endurance;
	Out.skills.head = In.Head;
	Out.morphology = static_cast<dirtbag::Morphology>(In.Morphology);
	Out.skin = In.Skin;
	Out.psyche = In.Psyche;
	Out.load = In.Load;
	Out.injury = ToSim(In.Injury);
	return Out;
}

dirtbag::Route ToSim(const FDirtbagRoute& In)
{
	dirtbag::Route Out;
	Out.name = TCHAR_TO_UTF8(*In.Name);
	Out.grade = In.Grade;
	Out.trueGrade = In.TrueGrade;
	Out.type = static_cast<dirtbag::RouteType>(In.Type);
	Out.discipline = static_cast<dirtbag::Discipline>(In.Discipline);
	Out.moves.reserve(In.Moves.Num());
	for (const FDirtbagMove& Move : In.Moves)
	{
		dirtbag::Move M;
		M.difficulty = Move.Difficulty;
		M.hold = static_cast<dirtbag::HoldType>(Move.Hold);
		M.restQuality = Move.RestQuality;
		M.reachBias = Move.ReachBias;
		M.crux = Move.bCrux;
		Out.moves.push_back(M);
	}
	return Out;
}

dirtbag::SessionState ToSim(const FDirtbagSessionState& In)
{
	dirtbag::SessionState Out;
	Out.skinLeft = In.SkinLeft;
	Out.warmth = In.Warmth;
	Out.psyche = In.Psyche;
	Out.attemptsMade = In.AttemptsMade;
	Out.padding = In.Padding;
	Out.shoeWear = In.ShoeWear;
	Out.rack = ToSim(In.Rack);
	Out.betaRate = In.BetaRate;
	Out.skinRate = In.SkinRate;
	return Out;
}

dirtbag::ProjectMemory ToSim(const FDirtbagProjectMemory& In)
{
	dirtbag::ProjectMemory Out;
	Out.routeName = TCHAR_TO_UTF8(*In.RouteName);
	Out.grade = In.Grade;
	Out.attempts = In.Attempts;
	Out.bestHighpoint = In.BestHighpoint;
	Out.beta = In.Beta;
	Out.sent = In.bSent;
	Out.firstSendStyle = static_cast<dirtbag::Style>(In.FirstSendStyle);
	Out.cleanliness = In.Cleanliness;
	Out.givenName = TCHAR_TO_UTF8(*In.GivenName);
	Out.firstAscent = In.bFirstAscent;
	Out.confirmedGrade = In.ConfirmedGrade;
	Out.discipline = static_cast<dirtbag::Discipline>(In.Discipline);
	return Out;
}

FDirtbagRoute FromSim(const dirtbag::Route& In)
{
	FDirtbagRoute Out;
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.Grade = In.grade;
	Out.TrueGrade = In.trueGrade;
	Out.Type = static_cast<EDirtbagRouteType>(In.type);
	Out.Discipline = static_cast<EDirtbagDiscipline>(In.discipline);
	Out.Moves.Reserve(In.moves.size());
	for (const dirtbag::Move& M : In.moves)
	{
		FDirtbagMove Move;
		Move.Difficulty = M.difficulty;
		Move.Hold = static_cast<EDirtbagHold>(M.hold);
		Move.RestQuality = M.restQuality;
		Move.ReachBias = M.reachBias;
		Move.bCrux = M.crux;
		Out.Moves.Add(Move);
	}
	return Out;
}

FDirtbagMoveResult FromSim(const dirtbag::MoveResult& In)
{
	FDirtbagMoveResult Out;
	Out.Index = In.index;
	Out.Odds = In.odds;
	Out.PumpAfter = In.pumpAfter;
	Out.bSuccess = In.success;
	return Out;
}

FDirtbagAttemptResult FromSim(const dirtbag::AttemptResult& In)
{
	FDirtbagAttemptResult Out;
	Out.bSent = In.sent;
	Out.Highpoint = In.highpoint;
	Out.Style = static_cast<EDirtbagStyle>(In.style);
	Out.SkinCost = In.skinCost;
	Out.PeakPump = In.peakPump;
	Out.Timeline.Reserve(In.timeline.size());
	for (const dirtbag::MoveResult& MR : In.timeline)
	{
		Out.Timeline.Add(FromSim(MR));
	}
	Out.Gear.Reserve(In.gear.quality.size());
	for (double Q : In.gear.quality)
	{
		Out.Gear.Add(Q);
	}
	return Out;
}

FDirtbagSessionState FromSim(const dirtbag::SessionState& In)
{
	FDirtbagSessionState Out;
	Out.ShoeWear = In.shoeWear;
	Out.SkinLeft = In.skinLeft;
	Out.Warmth = In.warmth;
	Out.Psyche = In.psyche;
	Out.AttemptsMade = In.attemptsMade;
	Out.Padding = In.padding;
	Out.Rack = FromSim(In.rack);
	Out.BetaRate = In.betaRate;
	Out.SkinRate = In.skinRate;
	return Out;
}

FDirtbagProjectMemory FromSim(const dirtbag::ProjectMemory& In)
{
	FDirtbagProjectMemory Out;
	Out.RouteName = UTF8_TO_TCHAR(In.routeName.c_str());
	Out.Grade = In.grade;
	Out.Attempts = In.attempts;
	Out.BestHighpoint = In.bestHighpoint;
	Out.Beta = In.beta;
	Out.bSent = In.sent;
	Out.FirstSendStyle = static_cast<EDirtbagStyle>(In.firstSendStyle);
	Out.Cleanliness = In.cleanliness;
	Out.GivenName = UTF8_TO_TCHAR(In.givenName.c_str());
	Out.bFirstAscent = In.firstAscent;
	Out.ConfirmedGrade = In.confirmedGrade;
	Out.Discipline = static_cast<EDirtbagDiscipline>(In.discipline);
	return Out;
}

dirtbag::PlayerState ToSim(const FDirtbagPlayerState& In)
{
	dirtbag::PlayerState Out;
	Out.name = TCHAR_TO_UTF8(*In.Name);
	Out.climber = ToSim(In.Climber);
	Out.character = ToSim(In.Character);
	Out.logbook = ToSim(In.Logbook);
	Out.quirks = ToSim(In.Quirks);
	Out.becameToday = static_cast<dirtbag::Quirk>(In.BecameToday);
	Out.life = ToSim(In.Life);
	Out.lostToday = static_cast<dirtbag::Thread>(In.LostToday);
	Out.locals = ToSim(In.Locals);
	Out.gym = ToSim(In.Gym);
	Out.floor = ToSim(In.Floor);
	Out.youth = ToSim(In.Youth);
	Out.gymLeague = ToSim(In.GymLeague);
	Out.living = ToSim(In.Living);
	Out.bivy = ToSim(In.Bivy);
	Out.gymNews = TCHAR_TO_UTF8(*In.GymNews);
	Out.rival = ToSim(In.Rival);
	Out.rankingPoints = In.RankingPoints;
	Out.rankingRecord.reserve(In.RankingRecord.Num());
	for (const FDirtbagRankingResult& R : In.RankingRecord)
	{
		Out.rankingRecord.push_back(ToSim(R));
	}
	Out.circuit = ToSim(In.Circuit);
	Out.worldCup = ToSim(In.WorldCup);
	Out.olympics = ToSim(In.Olympics);
	Out.league = ToSim(In.League);
	Out.medical = ToSim(In.Medical);
	Out.hand = ToSim(In.Hand);
	Out.sickness = ToSim(In.Sickness);
	Out.teeth = ToSim(In.Teeth);
	Out.upkeep = ToSim(In.Upkeep);
	Out.team = ToSim(In.Team);
	Out.pastRivals.reserve(In.PastRivals.Num());
	for (const FDirtbagPastRival& P : In.PastRivals)
	{
		Out.pastRivals.push_back(ToSim(P));
	}
	Out.cash = In.Cash;
	Out.hungerCarried = In.HungerCarried;
	Out.day = In.Day;
	Out.dog = ToSim(In.Dog);
	Out.shoes = ToSim(In.Shoes);
	Out.van = ToSim(In.Van);
	Out.owed = In.Owed;
	Out.job = ToSim(In.Job);
	Out.crew = ToSim(In.Crew);
	Out.dreams = ToSim(In.Dreams);
	Out.standing = ToSim(In.Standing);
	Out.kit = ToSim(In.Kit);
	Out.rack = ToSim(In.Rack);
	Out.sponsor = ToSim(In.Sponsor);
	Out.secrets.reserve(In.Secrets.Num());
	for (const FDirtbagSecret& S : In.Secrets)
	{
		Out.secrets.push_back(ToSim(S));
	}
	Out.lastPhysioDay = In.LastPhysioDay;
	Out.bonds.reserve(In.Bonds.Num());
	for (const FDirtbagPartnerBond& B : In.Bonds)
	{
		Out.bonds.push_back(ToSim(B));
	}
	Out.projects.reserve(In.Projects.Num());
	for (const FDirtbagProjectMemory& M : In.Projects)
	{
		Out.projects.push_back(ToSim(M));
	}
	return Out;
}

dirtbag::DayState ToSim(const FDirtbagDayState& In)
{
	dirtbag::DayState Out;
	Out.hour = In.Hour;
	Out.energy = In.Energy;
	Out.hunger = In.Hunger;
	Out.atGym = In.bAtGym;
	Out.hangboardDone = In.bHangboardDone;
	Out.indoors = In.bIndoors;
	Out.firstPullOnHour = In.FirstPullOnHour;
	Out.lastBurn = TCHAR_TO_UTF8(*In.LastBurn);
	Out.heard = TCHAR_TO_UTF8(*In.Heard);
	Out.session = ToSim(In.Session);
	return Out;
}

FDirtbagPlayerState FromSim(const dirtbag::PlayerState& In)
{
	FDirtbagPlayerState Out;
	Out.Name = FString(In.name.c_str());
	Out.Character = FromSim(In.character);
	Out.Logbook = FromSim(In.logbook);
	Out.Quirks = FromSim(In.quirks);
	Out.BecameToday = static_cast<EDirtbagQuirk>(In.becameToday);
	Out.Life = FromSim(In.life);
	Out.LostToday = static_cast<EDirtbagThread>(In.lostToday);
	Out.Locals = FromSim(In.locals);
	Out.Gym = FromSim(In.gym);
	Out.Floor = FromSim(In.floor);
	Out.Youth = FromSim(In.youth);
	Out.GymLeague = FromSim(In.gymLeague);
	Out.Living = FromSim(In.living);
	Out.Bivy = FromSim(In.bivy);
	Out.GymNews = UTF8_TO_TCHAR(In.gymNews.c_str());
	Out.Rival = FromSim(In.rival);
	Out.RankingPoints = In.rankingPoints;
	Out.RankingRecord.Reset(In.rankingRecord.size());
	for (const dirtbag::RankingResult& R : In.rankingRecord)
	{
		Out.RankingRecord.Add(FromSim(R));
	}
	Out.Circuit = FromSim(In.circuit);
	Out.WorldCup = FromSim(In.worldCup);
	Out.Olympics = FromSim(In.olympics);
	Out.League = FromSim(In.league);
	Out.Medical = FromSim(In.medical);
	Out.Hand = FromSim(In.hand);
	Out.Sickness = FromSim(In.sickness);
	Out.Teeth = FromSim(In.teeth);
	Out.Upkeep = FromSim(In.upkeep);
	Out.Team = FromSim(In.team);
	Out.PastRivals.Reset(In.pastRivals.size());
	for (const dirtbag::PastRival& P : In.pastRivals)
	{
		Out.PastRivals.Add(FromSim(P));
	}
	Out.Climber.Power = In.climber.skills.power;
	Out.Climber.Fingers = In.climber.skills.fingers;
	Out.Climber.Technique = In.climber.skills.technique;
	Out.Climber.Endurance = In.climber.skills.endurance;
	Out.Climber.Head = In.climber.skills.head;
	Out.Climber.Morphology = static_cast<EDirtbagMorphology>(In.climber.morphology);
	Out.Climber.Skin = In.climber.skin;
	Out.Climber.Psyche = In.climber.psyche;
	Out.Cash = In.cash;
	Out.HungerCarried = In.hungerCarried;
	Out.Day = In.day;
	Out.Dog = FromSim(In.dog);
	Out.Shoes = FromSim(In.shoes);
	Out.Van = FromSim(In.van);
	Out.Owed = In.owed;
	Out.Job = FromSim(In.job);
	Out.Crew = FromSim(In.crew);
	Out.Dreams = FromSim(In.dreams);
	Out.Standing = FromSim(In.standing);
	Out.Kit = FromSim(In.kit);
	Out.Rack = FromSim(In.rack);
	Out.Sponsor = FromSim(In.sponsor);
	Out.Secrets.Reserve(static_cast<int32>(In.secrets.size()));
	for (const dirtbag::Secret& S : In.secrets)
	{
		Out.Secrets.Add(FromSim(S));
	}
	Out.LastPhysioDay = In.lastPhysioDay;
	Out.Bonds.Reserve(static_cast<int32>(In.bonds.size()));
	for (const dirtbag::PartnerBond& B : In.bonds)
	{
		Out.Bonds.Add(FromSim(B));
	}
	Out.Projects.Reserve(In.projects.size());
	for (const dirtbag::ProjectMemory& M : In.projects)
	{
		Out.Projects.Add(FromSim(M));
	}
	return Out;
}

FDirtbagDayState FromSim(const dirtbag::DayState& In)
{
	FDirtbagDayState Out;
	Out.Hour = In.hour;
	Out.Energy = In.energy;
	Out.Hunger = In.hunger;
	Out.bAtGym = In.atGym;
	Out.bHangboardDone = In.hangboardDone;
	Out.bIndoors = In.indoors;
	Out.FirstPullOnHour = In.firstPullOnHour;
	Out.LastBurn = UTF8_TO_TCHAR(In.lastBurn.c_str());
	Out.Heard = UTF8_TO_TCHAR(In.heard.c_str());
	Out.Session = FromSim(In.session);
	return Out;
}

FDirtbagCareerSummary FromSim(const dirtbag::CareerSummary& In)
{
	FDirtbagCareerSummary Out;
	Out.AbilityGrade = In.abilityGrade;
	Out.HardestSendGrade = In.hardestSendGrade;
	Out.HardestSendName = UTF8_TO_TCHAR(In.hardestSendName.c_str());
	Out.HardestSendStyle = static_cast<EDirtbagStyle>(In.hardestSendStyle);
	Out.TotalSends = In.totalSends;
	Out.TotalAttempts = In.totalAttempts;
	Out.OpenProjects = In.openProjects;
	Out.Nemesis = UTF8_TO_TCHAR(In.nemesis.c_str());
	Out.NemesisAttempts = In.nemesisAttempts;
	return Out;
}

FDirtbagWeather FromSim(const dirtbag::Weather& In)
{
	FDirtbagWeather Out;
	Out.Day = In.day;
	Out.HighTempF = In.highTempF;
	Out.LowTempF = In.lowTempF;
	Out.Humidity = In.humidity;
	Out.Cloud = In.cloud;
	Out.Wind = In.wind;
	return Out;
}

FDirtbagVenue FromSim(const dirtbag::Venue& In)
{
	FDirtbagVenue Out;
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.Service = static_cast<EDirtbagService>(In.service);
	Out.OpensAt = In.opensAt;
	Out.ClosesAt = In.closesAt;
	Out.PriceFactor = In.priceFactor;
	Out.QualityFactor = In.qualityFactor;
	Out.TravelHours = In.travelHours;
	Out.Flavour = UTF8_TO_TCHAR(In.flavour.c_str());
	return Out;
}

dirtbag::Venue ToSim(const FDirtbagVenue& In)
{
	dirtbag::Venue Out;
	Out.name = TCHAR_TO_UTF8(*In.Name);
	Out.service = static_cast<dirtbag::Service>(In.Service);
	Out.opensAt = In.OpensAt;
	Out.closesAt = In.ClosesAt;
	Out.priceFactor = In.PriceFactor;
	Out.qualityFactor = In.QualityFactor;
	Out.travelHours = In.TravelHours;
	Out.flavour = TCHAR_TO_UTF8(*In.Flavour);
	return Out;
}

FDirtbagTown FromSim(const dirtbag::Town& In)
{
	FDirtbagTown Out;
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.Venues.Reserve(static_cast<int32>(In.venues.size()));
	for (const dirtbag::Venue& V : In.venues)
	{
		Out.Venues.Add(FromSim(V));
	}
	return Out;
}

FDirtbagInjury FromSim(const dirtbag::Injury& In)
{
	FDirtbagInjury Out;
	Out.bActive = In.active;
	Out.Kind = static_cast<EDirtbagInjuryKind>(In.kind);
	Out.Severity = In.severity;
	Out.DaysLeft = In.daysLeft;
	Out.bStaged = In.staged;
	return Out;
}

dirtbag::Injury ToSim(const FDirtbagInjury& In)
{
	dirtbag::Injury Out;
	Out.active = In.bActive;
	Out.kind = static_cast<dirtbag::InjuryKind>(In.Kind);
	Out.severity = In.Severity;
	Out.daysLeft = In.DaysLeft;
	Out.staged = In.bStaged;
	return Out;
}

FDirtbagSecret FromSim(const dirtbag::Secret& In)
{
	FDirtbagSecret Out;
	Out.Act = static_cast<EDirtbagEthicalAct>(In.act);
	Out.RouteKey = UTF8_TO_TCHAR(In.routeKey.c_str());
	Out.DayDone = In.dayDone;
	Out.bKnown = In.known;
	Out.DayFound = In.dayFound;
	return Out;
}

dirtbag::Secret ToSim(const FDirtbagSecret& In)
{
	dirtbag::Secret Out;
	Out.act = static_cast<dirtbag::EthicalAct>(In.Act);
	Out.routeKey = TCHAR_TO_UTF8(*In.RouteKey);
	Out.dayDone = In.DayDone;
	Out.known = In.bKnown;
	Out.dayFound = In.DayFound;
	return Out;
}

FDirtbagSponsorship FromSim(const dirtbag::Sponsorship& In)
{
	FDirtbagSponsorship Out;
	// mirror-skip: obligationsMetThisSeason -- within-season bookkeeping the
	// review reads and then clears. Blueprint has no use for a counter that
	// is zero every time a season boundary makes it interesting.
	// mirror-skip: obligationsMissedThisSeason -- same.
	Out.Tier = static_cast<EDirtbagSponsorTier>(In.tier);
	Out.SeasonsHeld = In.seasonsHeld;
	Out.GradeAtLastReview = In.gradeAtLastReview;
	Out.SeasonsWithoutProgress = In.seasonsWithoutProgress;
	Out.DaysHurtThisSeason = In.daysHurtThisSeason;
	return Out;
}

dirtbag::Sponsorship ToSim(const FDirtbagSponsorship& In)
{
	dirtbag::Sponsorship Out;
	// mirror-skip: obligationsMetThisSeason -- within-season bookkeeping the
	// review clears rather than reads, and the mirror does not carry it in
	// either direction, so there is nothing on the engine side to fill from.
	// mirror-skip: obligationsMissedThisSeason -- same.
	Out.tier = static_cast<dirtbag::SponsorTier>(In.Tier);
	Out.seasonsHeld = In.SeasonsHeld;
	Out.daysHurtThisSeason = In.DaysHurtThisSeason;
	Out.gradeAtLastReview = In.GradeAtLastReview;
	Out.seasonsWithoutProgress = In.SeasonsWithoutProgress;
	return Out;
}

FDirtbagBeat FromSim(const dirtbag::Beat& In)
{
	FDirtbagBeat Out;
	Out.Move = In.move;
	Out.Kind = static_cast<EDirtbagBeatKind>(In.kind);
	Out.Weight = In.weight;
	Out.Line = UTF8_TO_TCHAR(In.line.c_str());
	return Out;
}

FDirtbagLogbook FromSim(const dirtbag::Logbook& In)
{
	FDirtbagLogbook Out;
	Out.Count.Reserve(dirtbag::kDidCount);
	for (int i = 0; i < dirtbag::kDidCount; i++)
	{
		Out.Count.Add(In.count[i]);
	}
	Out.AsOfDay = In.asOfDay;
	Out.LifetimeBurns = In.lifetimeBurns;
	Out.LifetimeDays = In.lifetimeDays;
	return Out;
}

dirtbag::Logbook ToSim(const FDirtbagLogbook& In)
{
	dirtbag::Logbook Out;
	// A mirror arriving with the wrong number of lanes is a mirror from
	// another build; take what fits and leave the rest at zero rather than
	// reading off the end of it.
	const int Lanes = FMath::Min(In.Count.Num(), dirtbag::kDidCount);
	for (int i = 0; i < Lanes; i++)
	{
		Out.count[i] = In.Count[i];
	}
	Out.asOfDay = In.AsOfDay;
	Out.lifetimeBurns = In.LifetimeBurns;
	Out.lifetimeDays = In.LifetimeDays;
	return Out;
}

FDirtbagBivy FromSim(const dirtbag::Bivy& In)
{
	FDirtbagBivy Out;
	Out.Tonight = static_cast<EDirtbagSpot>(In.tonight);
	Out.LastSlept.Reserve(dirtbag::kSpotCount);
	for (int i = 0; i < dirtbag::kSpotCount; i++)
	{
		Out.LastSlept.Add(In.lastSlept[i]);
	}
	Out.LotNights = In.lotNights;
	Out.TicketsOwed = In.ticketsOwed;
	Out.bBooted = In.booted;
	return Out;
}

dirtbag::Bivy ToSim(const FDirtbagBivy& In)
{
	dirtbag::Bivy Out;
	Out.tonight = static_cast<dirtbag::Spot>(In.Tonight);
	const int Count = FMath::Min(In.LastSlept.Num(), dirtbag::kSpotCount);
	for (int i = 0; i < Count; i++)
	{
		Out.lastSlept[i] = In.LastSlept[i];
	}
	Out.lotNights = In.LotNights;
	Out.ticketsOwed = In.TicketsOwed;
	Out.booted = In.bBooted;
	return Out;
}

FDirtbagLiving FromSim(const dirtbag::Living& In)
{
	FDirtbagLiving Out;
	Out.Grime = In.grime;
	Out.Water = In.water;
	Out.Propane = In.propane;
	return Out;
}

dirtbag::Living ToSim(const FDirtbagLiving& In)
{
	dirtbag::Living Out;
	Out.grime = In.Grime;
	Out.water = In.Water;
	Out.propane = In.Propane;
	return Out;
}

FDirtbagGymStaffer FromSim(const dirtbag::GymStaffer& In)
{
	FDirtbagGymStaffer Out;
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.Trait = UTF8_TO_TCHAR(In.trait.c_str());
	Out.Wage = In.wage;
	Out.Quality = In.quality;
	Out.HiredDay = In.hiredDay;
	Out.NextAskDay = In.nextAskDay;
	Out.Refusals = In.refusals;
	return Out;
}

dirtbag::GymStaffer ToSim(const FDirtbagGymStaffer& In)
{
	dirtbag::GymStaffer Out;
	Out.name = TCHAR_TO_UTF8(*In.Name);
	Out.trait = TCHAR_TO_UTF8(*In.Trait);
	Out.wage = In.Wage;
	Out.quality = In.Quality;
	Out.hiredDay = In.HiredDay;
	Out.nextAskDay = In.NextAskDay;
	Out.refusals = In.Refusals;
	return Out;
}

FDirtbagLeagueChampion FromSim(const dirtbag::LeagueChampion& In)
{
	FDirtbagLeagueChampion Out;
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.Day = In.day;
	Out.Run = In.run;
	return Out;
}

dirtbag::LeagueChampion ToSim(const FDirtbagLeagueChampion& In)
{
	dirtbag::LeagueChampion Out;
	Out.name = TCHAR_TO_UTF8(*In.Name);
	Out.day = In.Day;
	Out.run = In.Run;
	return Out;
}

FDirtbagLeagueStanding FromSim(const dirtbag::LeagueStanding& In)
{
	FDirtbagLeagueStanding Out;
	Out.Who = static_cast<EDirtbagGymRegular>(In.who);
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.Points = In.points;
	Out.bSuited = In.suited;
	return Out;
}

FDirtbagGymLeague FromSim(const dirtbag::GymLeague& In)
{
	FDirtbagGymLeague Out;
	Out.bRunning = In.running;
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.Night = In.night;
	Out.Format = static_cast<EDirtbagLeagueFormat>(In.format);
	Out.Week = In.week;
	Out.Runs = In.runs;
	Out.Points.Reset();
	for (int32 i = 0; i < dirtbag::kGymRegularCount; i++)
	{
		Out.Points.Add(In.points[i]);
	}
	Out.LastNightDay = In.lastNightDay;
	Out.Champions.Reset();
	for (const dirtbag::LeagueChampion& Won : In.champions)
	{
		Out.Champions.Add(FromSim(Won));
	}
	return Out;
}

dirtbag::GymLeague ToSim(const FDirtbagGymLeague& In)
{
	dirtbag::GymLeague Out;
	Out.running = In.bRunning;
	Out.name = TCHAR_TO_UTF8(*In.Name);
	Out.night = In.Night;
	Out.format = static_cast<dirtbag::LeagueFormat>(In.Format);
	Out.week = In.Week;
	Out.runs = In.Runs;
	for (int32 i = 0; i < dirtbag::kGymRegularCount; i++)
	{
		Out.points[i] = In.Points.IsValidIndex(i) ? In.Points[i] : 0.0;
	}
	Out.lastNightDay = In.LastNightDay;
	for (const FDirtbagLeagueChampion& Won : In.Champions)
	{
		Out.champions.push_back(ToSim(Won));
	}
	return Out;
}

FDirtbagYouthKid FromSim(const dirtbag::YouthKid& In)
{
	FDirtbagYouthKid Out;
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.Tag = UTF8_TO_TCHAR(In.tag.c_str());
	Out.Level = In.level;
	Out.AgeAtJoin = In.ageAtJoin;
	Out.JoinedDay = In.joinedDay;
	return Out;
}

dirtbag::YouthKid ToSim(const FDirtbagYouthKid& In)
{
	dirtbag::YouthKid Out;
	Out.name = TCHAR_TO_UTF8(*In.Name);
	Out.tag = TCHAR_TO_UTF8(*In.Tag);
	Out.level = In.Level;
	Out.ageAtJoin = In.AgeAtJoin;
	Out.joinedDay = In.JoinedDay;
	return Out;
}

FDirtbagGraduate FromSim(const dirtbag::Graduate& In)
{
	FDirtbagGraduate Out;
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.Day = In.day;
	Out.Age = In.age;
	return Out;
}

dirtbag::Graduate ToSim(const FDirtbagGraduate& In)
{
	dirtbag::Graduate Out;
	Out.name = TCHAR_TO_UTF8(*In.Name);
	Out.day = In.Day;
	Out.age = In.Age;
	return Out;
}

FDirtbagYouth FromSim(const dirtbag::Youth& In)
{
	FDirtbagYouth Out;
	Out.bGoing = In.going;
	Out.FoundedDay = In.foundedDay;
	Out.Kids.Reset();
	for (const dirtbag::YouthKid& Kid : In.kids)
	{
		Out.Kids.Add(FromSim(Kid));
	}
	Out.bYouCoach = In.youCoach;
	Out.CoachName = UTF8_TO_TCHAR(In.coachName.c_str());
	Out.Sessions = In.sessions;
	Out.LastSessionDay = In.lastSessionDay;
	Out.Craft = In.craft;
	Out.Graduated.Reset();
	for (const dirtbag::Graduate& Gone : In.graduated)
	{
		Out.Graduated.Add(FromSim(Gone));
	}
	Out.SteppingUp.Reset();
	for (const std::string& Who : In.steppingUp)
	{
		Out.SteppingUp.Add(UTF8_TO_TCHAR(Who.c_str()));
	}
	return Out;
}

dirtbag::Youth ToSim(const FDirtbagYouth& In)
{
	dirtbag::Youth Out;
	Out.going = In.bGoing;
	Out.foundedDay = In.FoundedDay;
	for (const FDirtbagYouthKid& Kid : In.Kids)
	{
		Out.kids.push_back(ToSim(Kid));
	}
	Out.youCoach = In.bYouCoach;
	Out.coachName = TCHAR_TO_UTF8(*In.CoachName);
	Out.sessions = In.Sessions;
	Out.lastSessionDay = In.LastSessionDay;
	Out.craft = In.Craft;
	for (const FDirtbagGraduate& Gone : In.Graduated)
	{
		Out.graduated.push_back(ToSim(Gone));
	}
	for (const FString& Who : In.SteppingUp)
	{
		Out.steppingUp.push_back(TCHAR_TO_UTF8(*Who));
	}
	return Out;
}

FDirtbagGymFloor FromSim(const dirtbag::GymFloor& In)
{
	FDirtbagGymFloor Out;
	Out.Stage.Reset();
	for (int32 i = 0; i < dirtbag::kGymRegularCount; i++)
	{
		Out.Stage.Add(In.stage[i]);
	}
	Out.LastWalkDay = In.lastWalkDay;
	Out.LastCompDay = In.lastCompDay;
	Out.bWaveTwoArrived = In.waveTwoArrived;
	Out.WaveTwo = static_cast<EDirtbagGymSetMix>(In.waveTwo);
	return Out;
}

dirtbag::GymFloor ToSim(const FDirtbagGymFloor& In)
{
	dirtbag::GymFloor Out;
	for (int32 i = 0; i < dirtbag::kGymRegularCount; i++)
	{
		Out.stage[i] = In.Stage.IsValidIndex(i) ? In.Stage[i] : 0;
	}
	Out.lastWalkDay = In.LastWalkDay;
	Out.lastCompDay = In.LastCompDay;
	Out.waveTwoArrived = In.bWaveTwoArrived;
	Out.waveTwo = static_cast<dirtbag::GymSetMix>(In.WaveTwo);
	return Out;
}

FDirtbagGym FromSim(const dirtbag::Gym& In)
{
	FDirtbagGym Out;
	Out.bOwned = In.owned;
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.OwnedDay = In.ownedDay;
	Out.Price = static_cast<EDirtbagGymPrice>(In.price);
	Out.Mix = static_cast<EDirtbagGymSetMix>(In.mix);
	Out.Equip = static_cast<EDirtbagGymEquip>(In.equip);
	Out.Campaign = static_cast<EDirtbagGymCampaign>(In.campaign);
	Out.CampaignUntil = In.campaignUntil;
	Out.bFrontDesk = In.frontDesk;
	Out.bSetter = In.setter;
	Out.Desk = FromSim(In.desk);
	Out.RouteSetter = FromSim(In.routesetter);
	Out.Wings.Reset();
	for (int32 i = 0; i < dirtbag::kGymWingCount; i++)
	{
		Out.Wings.Add(In.wings[i]);
	}
	Out.bPassive = In.passive;
	Out.Incident = static_cast<EDirtbagGymIncident>(In.incident);
	Out.IncidentDay = In.incidentDay;
	Out.HostsSeason = In.hostsSeason;
	Out.AskedSeason = In.askedSeason;
	Out.RoundsRun = In.roundsRun;
	Out.Members = In.members;
	Out.Balance = In.balance;
	Out.DebtDays = In.debtDays;
	Out.LastTickDay = In.lastTickDay;
	return Out;
}

dirtbag::Gym ToSim(const FDirtbagGym& In)
{
	dirtbag::Gym Out;
	Out.owned = In.bOwned;
	Out.name = TCHAR_TO_UTF8(*In.Name);
	Out.ownedDay = In.OwnedDay;
	Out.price = static_cast<dirtbag::GymPrice>(In.Price);
	Out.mix = static_cast<dirtbag::GymSetMix>(In.Mix);
	Out.equip = static_cast<dirtbag::GymEquip>(In.Equip);
	Out.campaign = static_cast<dirtbag::GymCampaign>(In.Campaign);
	Out.campaignUntil = In.CampaignUntil;
	Out.frontDesk = In.bFrontDesk;
	Out.setter = In.bSetter;
	Out.desk = ToSim(In.Desk);
	Out.routesetter = ToSim(In.RouteSetter);
	for (int32 i = 0; i < dirtbag::kGymWingCount; i++)
	{
		Out.wings[i] = In.Wings.IsValidIndex(i) ? In.Wings[i] : false;
	}
	Out.passive = In.bPassive;
	Out.incident = static_cast<dirtbag::GymIncident>(In.Incident);
	Out.incidentDay = In.IncidentDay;
	Out.hostsSeason = In.HostsSeason;
	Out.askedSeason = In.AskedSeason;
	Out.roundsRun = In.RoundsRun;
	Out.members = In.Members;
	Out.balance = In.Balance;
	Out.debtDays = In.DebtDays;
	Out.lastTickDay = In.LastTickDay;
	return Out;
}

FDirtbagLocal FromSim(const dirtbag::Local& In)
{
	FDirtbagLocal Out;
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.Where = static_cast<EDirtbagService>(In.where);
	Out.Known = In.known;
	Out.EverKnew = In.everKnew;
	Out.Holds = static_cast<EDirtbagHeard>(In.holds);
	Out.About = UTF8_TO_TCHAR(In.about.c_str());
	Out.LastSeen = In.lastSeen;
	return Out;
}

dirtbag::Local ToSim(const FDirtbagLocal& In)
{
	dirtbag::Local Out;
	Out.name = TCHAR_TO_UTF8(*In.Name);
	Out.where = static_cast<dirtbag::Service>(In.Where);
	Out.known = In.Known;
	Out.everKnew = In.EverKnew;
	Out.holds = static_cast<dirtbag::Heard>(In.Holds);
	Out.about = TCHAR_TO_UTF8(*In.About);
	Out.lastSeen = In.LastSeen;
	return Out;
}

FDirtbagLocals FromSim(const dirtbag::Locals& In)
{
	FDirtbagLocals Out;
	Out.People.Reserve(In.people.size());
	for (const dirtbag::Local& P : In.people)
	{
		Out.People.Add(FromSim(P));
	}
	Out.KnownTitles = In.knownTitles;
	Out.KnownTier = In.knownTier;
	return Out;
}

dirtbag::Locals ToSim(const FDirtbagLocals& In)
{
	dirtbag::Locals Out;
	Out.people.reserve(In.People.Num());
	for (const FDirtbagLocal& P : In.People)
	{
		Out.people.push_back(ToSim(P));
	}
	Out.knownTitles = In.KnownTitles;
	Out.knownTier = In.KnownTier;
	return Out;
}

FDirtbagStrand FromSim(const dirtbag::Strand& In)
{
	FDirtbagStrand Out;
	Out.bGoing = In.going;
	Out.Warmth = In.warmth;
	Out.Depth = In.depth;
	Out.LastGivenDay = In.lastGivenDay;
	Out.DaysGiven = In.daysGiven;
	Out.bEnded = In.ended;
	Out.EndedDay = In.endedDay;
	return Out;
}

dirtbag::Strand ToSim(const FDirtbagStrand& In)
{
	dirtbag::Strand Out;
	Out.going = In.bGoing;
	Out.warmth = In.Warmth;
	Out.depth = In.Depth;
	Out.lastGivenDay = In.LastGivenDay;
	Out.daysGiven = In.DaysGiven;
	Out.ended = In.bEnded;
	Out.endedDay = In.EndedDay;
	return Out;
}

FDirtbagLife FromSim(const dirtbag::Life& In)
{
	FDirtbagLife Out;
	Out.Strands.Reserve(dirtbag::kThreadCount);
	for (int i = 0; i < dirtbag::kThreadCount; i++)
	{
		Out.Strands.Add(FromSim(In.strands[i]));
	}
	Out.Grieving = In.grieving;
	Out.GriefWeight = In.griefWeight;
	return Out;
}

dirtbag::Life ToSim(const FDirtbagLife& In)
{
	dirtbag::Life Out;
	const int Count = FMath::Min(In.Strands.Num(), dirtbag::kThreadCount);
	for (int i = 0; i < Count; i++)
	{
		Out.strands[i] = ToSim(In.Strands[i]);
	}
	Out.grieving = In.Grieving;
	Out.griefWeight = In.GriefWeight;
	return Out;
}

FDirtbagQuirks FromSim(const dirtbag::Quirks& In)
{
	FDirtbagQuirks Out;
	Out.Held.Reserve(In.held.size());
	for (dirtbag::Quirk Q : In.held)
	{
		Out.Held.Add(static_cast<EDirtbagQuirk>(Q));
	}
	Out.HeldFor.Reserve(dirtbag::kHabitCount);
	for (int i = 0; i < dirtbag::kHabitCount; i++)
	{
		Out.HeldFor.Add(In.heldFor[i]);
	}
	Out.Picked = static_cast<EDirtbagQuirk>(In.picked);
	return Out;
}

dirtbag::Quirks ToSim(const FDirtbagQuirks& In)
{
	dirtbag::Quirks Out;
	Out.held.reserve(In.Held.Num());
	for (EDirtbagQuirk Q : In.Held)
	{
		Out.held.push_back(static_cast<dirtbag::Quirk>(Q));
	}
	const int Lanes = FMath::Min(In.HeldFor.Num(), dirtbag::kHabitCount);
	for (int i = 0; i < Lanes; i++)
	{
		Out.heldFor[i] = In.HeldFor[i];
	}
	Out.picked = static_cast<dirtbag::Quirk>(In.Picked);
	return Out;
}

FDirtbagRack FromSim(const dirtbag::Rack& In)
{
	FDirtbagRack Out;
	Out.Pieces = In.pieces;
	Out.Quality = In.quality;
	return Out;
}

dirtbag::Rack ToSim(const FDirtbagRack& In)
{
	dirtbag::Rack Out;
	Out.pieces = In.Pieces;
	Out.quality = In.Quality;
	return Out;
}

FDirtbagKit FromSim(const dirtbag::Kit& In)
{
	FDirtbagKit Out;
	Out.Pads = In.pads;
	Out.bHangboard = In.hangboard;
	Out.MembershipDaysLeft = In.membershipDaysLeft;
	return Out;
}

dirtbag::Kit ToSim(const FDirtbagKit& In)
{
	dirtbag::Kit Out;
	Out.pads = In.Pads;
	Out.hangboard = In.bHangboard;
	Out.membershipDaysLeft = In.MembershipDaysLeft;
	return Out;
}

FDirtbagStanding FromSim(const dirtbag::Standing& In)
{
	FDirtbagStanding Out;
	Out.With.Reserve(dirtbag::kFactionCount);
	for (int32 i = 0; i < dirtbag::kFactionCount; i++)
	{
		Out.With.Add(In.with[i]);
	}
	Out.ClosedDays = In.closedDays;
	return Out;
}

dirtbag::Standing ToSim(const FDirtbagStanding& In)
{
	dirtbag::Standing Out;
	// A short array is a save from before factions existed, not a bug. The
	// missing camps have no opinion, which is what a default Standing says.
	for (int32 i = 0; i < In.With.Num() && i < dirtbag::kFactionCount; i++)
	{
		Out.with[i] = In.With[i];
	}
	Out.closedDays = In.ClosedDays;
	return Out;
}

FDirtbagJob FromSim(const dirtbag::Job& In)
{
	FDirtbagJob Out;
	Out.bSalaried = In.salaried;
	Out.DaysWorked = In.daysWorked;
	Out.WeeksSalaried = In.weeksSalaried;
	Out.DaysSinceSalary = In.daysSinceSalary;
	Out.DirtbagYears = In.dirtbagYears;
	Out.LongestStreak = In.longestStreak;
	return Out;
}

dirtbag::Job ToSim(const FDirtbagJob& In)
{
	dirtbag::Job Out;
	Out.salaried = In.bSalaried;
	Out.daysWorked = In.DaysWorked;
	Out.weeksSalaried = In.WeeksSalaried;
	Out.daysSinceSalary = In.DaysSinceSalary;
	Out.dirtbagYears = In.DirtbagYears;
	Out.longestStreak = In.LongestStreak;
	return Out;
}

FDirtbagBlackjack FromSim(const dirtbag::BlackjackHand& In)
{
	FDirtbagBlackjack Out;
	Out.Yours = In.yours;
	Out.DealerShows = In.dealerShows;
	Out.bBust = In.bust;
	Out.bFinished = In.finished;
	Out.Draws = In.draws;
	return Out;
}

dirtbag::BlackjackHand ToSim(const FDirtbagBlackjack& In)
{
	dirtbag::BlackjackHand Out;
	Out.yours = In.Yours;
	Out.dealerShows = In.DealerShows;
	Out.bust = In.bBust;
	Out.finished = In.bFinished;
	Out.draws = In.Draws;
	return Out;
}

FDirtbagDreams FromSim(const dirtbag::Dreams& In)
{
	FDirtbagDreams Out;
	Out.bRig = In.has[0];
	Out.bWarChest = In.has[1];
	Out.bHomeBase = In.has[2];
	Out.Chosen = static_cast<EDirtbagDream>(In.chosen);
	Out.SeasonOffDaysLeft = In.seasonOffDaysLeft;
	return Out;
}

dirtbag::Dreams ToSim(const FDirtbagDreams& In)
{
	dirtbag::Dreams Out;
	Out.has[0] = In.bRig;
	Out.has[1] = In.bWarChest;
	Out.has[2] = In.bHomeBase;
	Out.chosen = static_cast<dirtbag::Dream>(In.Chosen);
	Out.seasonOffDaysLeft = In.SeasonOffDaysLeft;
	return Out;
}

FDirtbagCrew FromSim(const dirtbag::Crew& In)
{
	FDirtbagCrew Out;
	Out.Name = FString(In.name.c_str());
	Out.NamedOnDay = In.namedOnDay;
	Out.DaysReadingAsACrew = In.daysReadingAsACrew;
	Out.MembersWhenNamed = In.membersWhenNamed;
	return Out;
}

dirtbag::Crew ToSim(const FDirtbagCrew& In)
{
	dirtbag::Crew Out;
	Out.name = TCHAR_TO_UTF8(*In.Name);
	Out.namedOnDay = In.NamedOnDay;
	Out.daysReadingAsACrew = In.DaysReadingAsACrew;
	Out.membersWhenNamed = In.MembersWhenNamed;
	return Out;
}

FDirtbagOddJob FromSim(const dirtbag::OddJob& In)
{
	FDirtbagOddJob Out;
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.Hours = In.hours;
	Out.Pay = In.pay;
	Out.Energy = In.energy;
	Out.bNeedsVan = In.needsVan;
	return Out;
}

dirtbag::Weather ToSim(const FDirtbagWeather& In)
{
	dirtbag::Weather Out;
	Out.day = In.Day;
	Out.highTempF = In.HighTempF;
	Out.lowTempF = In.LowTempF;
	Out.humidity = In.Humidity;
	Out.cloud = In.Cloud;
	Out.wind = In.Wind;
	return Out;
}

FDirtbagPrimeWindow FromSim(const dirtbag::PrimeWindow& In)
{
	FDirtbagPrimeWindow Out;
	Out.bExists = In.exists;
	Out.StartHour = In.startHour;
	Out.EndHour = In.endHour;
	Out.PeakHour = In.peakHour;
	Out.PeakFriction = In.peakFriction;
	return Out;
}

dirtbag::Aspect ToSim(EDirtbagAspect In)
{
	return static_cast<dirtbag::Aspect>(In);
}

FDirtbagCragLine FromSim(const dirtbag::CragLine& In)
{
	FDirtbagCragLine Out;
	Out.Route = FromSim(In.route);
	Out.Stars = In.stars;
	Out.bIsProject = In.isProject;
	Out.FirstAscentBy = UTF8_TO_TCHAR(In.firstAscentBy.c_str());
	Out.Description = UTF8_TO_TCHAR(In.description.c_str());
	// mirror-skip: displayName -- carried, but through DisplayName(), which
	// falls back to the description for a line nobody has named yet.
	Out.DisplayName = UTF8_TO_TCHAR(dirtbag::DisplayName(In).c_str());
	return Out;
}

FDirtbagCrag FromSim(const dirtbag::Crag& In)
{
	FDirtbagCrag Out;
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.Aspect = static_cast<EDirtbagAspect>(In.aspect);
	Out.ApproachHours = In.approachHours;
	Out.Lines.Reserve(static_cast<int32>(In.lines.size()));
	for (const dirtbag::CragLine& Line : In.lines)
	{
		Out.Lines.Add(FromSim(Line));
	}
	return Out;
}

FDirtbagClimber FromSim(const dirtbag::Climber& In)
{
	FDirtbagClimber Out;
	Out.Power = In.skills.power;
	Out.Fingers = In.skills.fingers;
	Out.Technique = In.skills.technique;
	Out.Endurance = In.skills.endurance;
	Out.Head = In.skills.head;
	Out.Morphology = static_cast<EDirtbagMorphology>(In.morphology);
	Out.Skin = In.skin;
	Out.Psyche = In.psyche;
	Out.Load = In.load;
	Out.Injury = FromSim(In.injury);
	return Out;
}

FDirtbagDog FromSim(const dirtbag::Dog& In)
{
	FDirtbagDog Out;
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.bAdopted = In.adopted;
	Out.Bond = In.bond;
	Out.Fed = In.fed;
	return Out;
}

dirtbag::Dog ToSim(const FDirtbagDog& In)
{
	dirtbag::Dog Out;
	Out.name = TCHAR_TO_UTF8(*In.Name);
	Out.adopted = In.bAdopted;
	Out.bond = In.Bond;
	Out.fed = In.Fed;
	return Out;
}

FDirtbagNationalTeam FromSim(const dirtbag::NationalTeam& In)
{
	FDirtbagNationalTeam Out;
	Out.Status = static_cast<EDirtbagTeamStatus>(In.status);
	Out.bEverNamed = In.everNamed;
	Out.Seasons = In.seasons;
	Out.Cuts = In.cuts;
	Out.NamedOnDay = In.namedOnDay;
	Out.Coach = FString(In.coach.c_str());
	Out.CoachKnownFor = FString(In.coachKnownFor.c_str());
	Out.Roster.Reset(In.roster.size());
	for (const dirtbag::Teammate& M : In.roster)
	{
		FDirtbagTeammate T;
		T.Name = FString(M.name.c_str());
		T.Role = FString(M.role.c_str());
		T.Points = M.points;
		Out.Roster.Add(T);
	}
	Out.Gone.Reset(In.gone.size());
	for (const std::string& G : In.gone) { Out.Gone.Add(FString(G.c_str())); }
	Out.Passed = FString(In.passed.c_str());
	Out.LastReviewPoints = In.lastReviewPoints;
	Out.LastReviewSeason = In.lastReviewSeason;
	Out.LastReviewDay = In.lastReviewDay;
	return Out;
}

dirtbag::NationalTeam ToSim(const FDirtbagNationalTeam& In)
{
	dirtbag::NationalTeam Out;
	Out.status = static_cast<dirtbag::TeamStatus>(In.Status);
	Out.everNamed = In.bEverNamed;
	Out.seasons = In.Seasons;
	Out.cuts = In.Cuts;
	Out.namedOnDay = In.NamedOnDay;
	Out.coach = TCHAR_TO_UTF8(*In.Coach);
	Out.coachKnownFor = TCHAR_TO_UTF8(*In.CoachKnownFor);
	Out.roster.reserve(In.Roster.Num());
	for (const FDirtbagTeammate& T : In.Roster)
	{
		dirtbag::Teammate M;
		M.name = TCHAR_TO_UTF8(*T.Name);
		M.role = TCHAR_TO_UTF8(*T.Role);
		M.points = T.Points;
		Out.roster.push_back(M);
	}
	Out.gone.reserve(In.Gone.Num());
	for (const FString& G : In.Gone) { Out.gone.push_back(TCHAR_TO_UTF8(*G)); }
	Out.passed = TCHAR_TO_UTF8(*In.Passed);
	Out.lastReviewPoints = In.LastReviewPoints;
	Out.lastReviewSeason = In.LastReviewSeason;
	Out.lastReviewDay = In.LastReviewDay;
	return Out;
}

FDirtbagCircuit FromSim(const dirtbag::Circuit& In)
{
	FDirtbagCircuit Out;
	Out.Season = In.season;
	Out.CompsDone = In.compsDone;
	Out.Schedule.Reset(In.schedule.size());
	for (int D : In.schedule) { Out.Schedule.Add(D); }
	Out.YourPoints = In.yourPoints;
	Out.RivalPoints = In.rivalPoints;
	Out.FieldPoints.Reset(In.fieldPoints.size());
	for (double P : In.fieldPoints) { Out.FieldPoints.Add(P); }
	Out.Titles = In.titles;
	return Out;
}

dirtbag::Circuit ToSim(const FDirtbagCircuit& In)
{
	dirtbag::Circuit Out;
	Out.season = In.Season;
	Out.compsDone = In.CompsDone;
	Out.schedule.reserve(In.Schedule.Num());
	for (int32 D : In.Schedule) { Out.schedule.push_back(D); }
	Out.yourPoints = In.YourPoints;
	Out.rivalPoints = In.RivalPoints;
	Out.fieldPoints.reserve(In.FieldPoints.Num());
	for (double P : In.FieldPoints) { Out.fieldPoints.push_back(P); }
	Out.titles = In.Titles;
	return Out;
}

FDirtbagCraftsman FromSim(const dirtbag::Craftsman& In)
{
	FDirtbagCraftsman Out;
	Out.Skill.Reset(dirtbag::kCraftCount);
	Out.Standing.Reset(dirtbag::kCraftCount);
	Out.Shifts.Reset(dirtbag::kCraftCount);
	Out.Sacked.Reset(dirtbag::kCraftCount);
	for (int32 i = 0; i < dirtbag::kCraftCount; i++)
	{
		Out.Skill.Add(In.skill[i]);
		Out.Standing.Add(In.standing[i]);
		Out.Shifts.Add(In.shifts[i]);
		Out.Sacked.Add(In.sacked[i]);
	}
	Out.MomentsTaken = In.momentsTaken;
	Out.MomentsBotched = In.momentsBotched;
	Out.MomentsDucked = In.momentsDucked;
	Out.Sackings = In.sackings;
	return Out;
}

dirtbag::Craftsman ToSim(const FDirtbagCraftsman& In)
{
	dirtbag::Craftsman Out;
	for (int32 i = 0; i < dirtbag::kCraftCount; i++)
	{
		if (In.Skill.IsValidIndex(i)) { Out.skill[i] = In.Skill[i]; }
		if (In.Standing.IsValidIndex(i)) { Out.standing[i] = In.Standing[i]; }
		if (In.Shifts.IsValidIndex(i)) { Out.shifts[i] = In.Shifts[i]; }
		if (In.Sacked.IsValidIndex(i)) { Out.sacked[i] = In.Sacked[i]; }
	}
	Out.momentsTaken = In.MomentsTaken;
	Out.momentsBotched = In.MomentsBotched;
	Out.momentsDucked = In.MomentsDucked;
	Out.sackings = In.Sackings;
	return Out;
}

FDirtbagSickness FromSim(const dirtbag::Sickness& In)
{
	FDirtbagSickness Out;
	Out.bActive = In.active;
	Out.DaysLeft = In.daysLeft;
	Out.Severity = In.severity;
	Out.bMedicated = In.medicated;
	Out.Caught = In.caught;
	return Out;
}

dirtbag::Sickness ToSim(const FDirtbagSickness& In)
{
	dirtbag::Sickness Out;
	Out.active = In.bActive;
	Out.daysLeft = In.DaysLeft;
	Out.severity = In.Severity;
	Out.medicated = In.bMedicated;
	Out.caught = In.Caught;
	return Out;
}

FDirtbagTeeth FromSim(const dirtbag::Teeth& In)
{
	FDirtbagTeeth Out;
	Out.Stage = static_cast<EDirtbagToothStage>(In.stage);
	Out.SinceDay = In.sinceDay;
	Out.Fixes = In.fixes;
	Out.WorstEver = In.worstEver;
	Out.Lost = In.lost;
	return Out;
}

dirtbag::Teeth ToSim(const FDirtbagTeeth& In)
{
	dirtbag::Teeth Out;
	Out.stage = static_cast<dirtbag::ToothStage>(In.Stage);
	Out.sinceDay = In.SinceDay;
	Out.fixes = In.Fixes;
	Out.worstEver = In.WorstEver;
	Out.lost = In.Lost;
	return Out;
}

FDirtbagUpkeep FromSim(const dirtbag::Upkeep& In)
{
	FDirtbagUpkeep Out;
	Out.LastPrehabDay = In.lastPrehabDay;
	Out.PrehabStreak = In.prehabStreak;
	Out.PrehabDays = In.prehabDays;
	Out.LastShrinkDay = In.lastShrinkDay;
	Out.ShrinkSessions = In.shrinkSessions;
	return Out;
}

dirtbag::Upkeep ToSim(const FDirtbagUpkeep& In)
{
	dirtbag::Upkeep Out;
	Out.lastPrehabDay = In.LastPrehabDay;
	Out.prehabStreak = In.PrehabStreak;
	Out.prehabDays = In.PrehabDays;
	Out.lastShrinkDay = In.LastShrinkDay;
	Out.shrinkSessions = In.ShrinkSessions;
	return Out;
}

FDirtbagMedical FromSim(const dirtbag::Medical& In)
{
	FDirtbagMedical Out;
	Out.Diagnosis = static_cast<EDirtbagDiagnosis>(In.diagnosis);
	Out.Treatment = static_cast<EDirtbagTreatment>(In.treatment);
	Out.Stage = static_cast<EDirtbagComeback>(In.stage);
	Out.StageStarted = In.stageStarted;
	Out.StageDays = In.stageDays;
	Out.ToldSeverity = In.toldSeverity;
	Out.Joints.Reset(dirtbag::kInjuryKindCount);
	Out.Shots.Reset(dirtbag::kInjuryKindCount);
	for (int32 i = 0; i < dirtbag::kInjuryKindCount; i++)
	{
		Out.Joints.Add(In.joints[i]);
		Out.Shots.Add(In.shots[i]);
	}
	Out.Scars.Reset(In.scars.size());
	for (const dirtbag::Scar& S : In.scars)
	{
		FDirtbagScar Sc;
		Sc.Kind = static_cast<EDirtbagInjuryKind>(S.kind);
		Sc.Weight = S.weight;
		Sc.FromDay = S.fromDay;
		Out.Scars.Add(Sc);
	}
	Out.bInsured = In.insured;
	Out.InsuredOnDay = In.insuredOnDay;
	Out.PremiumsPaid = In.premiumsPaid;
	Out.ClaimsPaid = In.claimsPaid;
	Out.Diagnoses = In.diagnoses;
	Out.ShotsTaken = In.shotsTaken;
	Out.Surgeries = In.surgeries;
	Out.RushedComebacks = In.rushedComebacks;
	Out.UntreatedInjuries = In.untreatedInjuries;
	Out.bTreatedThisTime = In.treatedThisTime;
	return Out;
}

dirtbag::Medical ToSim(const FDirtbagMedical& In)
{
	dirtbag::Medical Out;
	Out.diagnosis = static_cast<dirtbag::Diagnosis>(In.Diagnosis);
	Out.treatment = static_cast<dirtbag::Treatment>(In.Treatment);
	Out.stage = static_cast<dirtbag::Comeback>(In.Stage);
	Out.stageStarted = In.StageStarted;
	Out.stageDays = In.StageDays;
	Out.toldSeverity = In.ToldSeverity;
	for (int32 i = 0; i < dirtbag::kInjuryKindCount; i++)
	{
		if (In.Joints.IsValidIndex(i)) { Out.joints[i] = In.Joints[i]; }
		if (In.Shots.IsValidIndex(i)) { Out.shots[i] = In.Shots[i]; }
	}
	Out.scars.reserve(In.Scars.Num());
	for (const FDirtbagScar& S : In.Scars)
	{
		dirtbag::Scar Sc;
		Sc.kind = static_cast<dirtbag::InjuryKind>(S.Kind);
		Sc.weight = S.Weight;
		Sc.fromDay = S.FromDay;
		Out.scars.push_back(Sc);
	}
	Out.insured = In.bInsured;
	Out.insuredOnDay = In.InsuredOnDay;
	Out.premiumsPaid = In.PremiumsPaid;
	Out.claimsPaid = In.ClaimsPaid;
	Out.diagnoses = In.Diagnoses;
	Out.shotsTaken = In.ShotsTaken;
	Out.surgeries = In.Surgeries;
	Out.rushedComebacks = In.RushedComebacks;
	Out.untreatedInjuries = In.UntreatedInjuries;
	Out.treatedThisTime = In.bTreatedThisTime;
	return Out;
}

FDirtbagLeague FromSim(const dirtbag::League& In)
{
	FDirtbagLeague Out;
	Out.NextNight = In.nextNight;
	Out.Block = In.block;
	Out.WeeksDone = In.weeksDone;
	Out.YourPoints = In.yourPoints;
	Out.FieldPoints.Reset(In.fieldPoints.size());
	for (double P : In.fieldPoints) { Out.FieldPoints.Add(P); }
	Out.Best = In.best;
	Out.BestOnDay = In.bestOnDay;
	Out.Nights = In.nights;
	Out.BlockWins = In.blockWins;
	Out.LastClimbedNight = In.lastClimbedNight;
	return Out;
}

dirtbag::League ToSim(const FDirtbagLeague& In)
{
	dirtbag::League Out;
	Out.nextNight = In.NextNight;
	Out.block = In.Block;
	Out.weeksDone = In.WeeksDone;
	Out.yourPoints = In.YourPoints;
	Out.fieldPoints.reserve(In.FieldPoints.Num());
	for (double P : In.FieldPoints) { Out.fieldPoints.push_back(P); }
	Out.best = In.Best;
	Out.bestOnDay = In.BestOnDay;
	Out.nights = In.Nights;
	Out.blockWins = In.BlockWins;
	Out.lastClimbedNight = In.LastClimbedNight;
	return Out;
}

FDirtbagRankingResult FromSim(const dirtbag::RankingResult& In)
{
	FDirtbagRankingResult Out;
	Out.Day = In.day;
	Out.Points = In.points;
	return Out;
}

dirtbag::RankingResult ToSim(const FDirtbagRankingResult& In)
{
	dirtbag::RankingResult Out;
	Out.day = In.Day;
	Out.points = In.Points;
	return Out;
}

FDirtbagWorldCupSeason FromSim(const dirtbag::WorldCupSeason& In)
{
	FDirtbagWorldCupSeason Out;
	Out.Season = In.season;
	Out.Schedule.Reset(In.schedule.size());
	for (const dirtbag::WorldCupRound& R : In.schedule)
	{
		FDirtbagWorldCupRound Round;
		Round.Day = R.day;
		Round.Venue = R.venue;
		Round.bResolved = R.resolved;
		Round.bFlown = R.flown;
		Out.Schedule.Add(Round);
	}
	Out.YourPoints = In.yourPoints;
	Out.FieldPoints.Reset(In.fieldPoints.size());
	for (double P : In.fieldPoints) { Out.FieldPoints.Add(P); }
	Out.Starts = In.starts;
	Out.Missed = In.missed;
	Out.Finals = In.finals;
	Out.Podiums = In.podiums;
	Out.Wins = In.wins;
	Out.Titles = In.titles;
	Out.BestRank = In.bestRank;
	Out.LastRank = In.lastRank;
	Out.bClosed = In.closed;
	return Out;
}

dirtbag::WorldCupSeason ToSim(const FDirtbagWorldCupSeason& In)
{
	dirtbag::WorldCupSeason Out;
	Out.season = In.Season;
	Out.schedule.reserve(In.Schedule.Num());
	for (const FDirtbagWorldCupRound& R : In.Schedule)
	{
		dirtbag::WorldCupRound Round;
		Round.day = R.Day;
		Round.venue = R.Venue;
		Round.resolved = R.bResolved;
		Round.flown = R.bFlown;
		Out.schedule.push_back(Round);
	}
	Out.yourPoints = In.YourPoints;
	Out.fieldPoints.reserve(In.FieldPoints.Num());
	for (double P : In.FieldPoints) { Out.fieldPoints.push_back(P); }
	Out.starts = In.Starts;
	Out.missed = In.Missed;
	Out.finals = In.Finals;
	Out.podiums = In.Podiums;
	Out.wins = In.Wins;
	Out.titles = In.Titles;
	Out.bestRank = In.BestRank;
	Out.lastRank = In.LastRank;
	Out.closed = In.bClosed;
	return Out;
}

FDirtbagOlympics FromSim(const dirtbag::Olympics& In)
{
	FDirtbagOlympics Out;
	Out.NextDay = In.nextDay;
	Out.Appearances = In.appearances;
	Out.Gold = In.gold;
	Out.Silver = In.silver;
	Out.Bronze = In.bronze;
	Out.LastCompeted = In.lastCompeted;
	return Out;
}

dirtbag::Olympics ToSim(const FDirtbagOlympics& In)
{
	dirtbag::Olympics Out;
	Out.nextDay = In.NextDay;
	Out.appearances = In.Appearances;
	Out.gold = In.Gold;
	Out.silver = In.Silver;
	Out.bronze = In.Bronze;
	Out.lastCompeted = In.LastCompeted;
	return Out;
}

FDirtbagWorldCupVenue FromSim(const dirtbag::WorldCupVenue& In)
{
	FDirtbagWorldCupVenue Out;
	Out.City = FString(In.city);
	Out.Country = FString(In.country);
	Out.Discipline = static_cast<EDirtbagDiscipline>(In.discipline);
	Out.Travel = In.travel;
	Out.Blurb = FString(In.blurb);
	return Out;
}

FDirtbagFlightCheck FromSim(const dirtbag::FlightCheck& In)
{
	FDirtbagFlightCheck Out;
	Out.bCan = In.can;
	Out.Round = In.round;
	Out.Cost = In.cost;
	Out.Why = FString(In.why.c_str());
	return Out;
}

FDirtbagRival FromSim(const dirtbag::Rival& In)
{
	FDirtbagRival Out;
	Out.Name = FString(In.name.c_str());
	Out.Style = static_cast<EDirtbagRouteType>(In.style);
	Out.Vibe = static_cast<EDirtbagRivalVibe>(In.vibe);
	Out.Generation = In.generation;
	Out.BornOnDay = In.bornOnDay;
	Out.StartAge = In.startAge;
	Out.Grade = In.grade;
	Out.LastStepDay = In.lastStepDay;
	Out.PeakGrade = In.peakGrade;
	Out.Rivalry = In.rivalry;
	Out.bAllied = In.allied;
	Out.bOffered = In.offered;
	Out.bMet = In.met;
	Out.FirstAscents.Reset(In.firstAscents.size());
	for (const std::string& Key : In.firstAscents)
	{
		Out.FirstAscents.Add(FString(Key.c_str()));
	}
	Out.bRetired = In.retired;
	Out.Race.RouteName = FString(In.race.routeName.c_str());
	Out.Race.ByDay = In.race.byDay;
	Out.Race.bForFirstAscent = In.race.forFirstAscent;
	return Out;
}

dirtbag::Rival ToSim(const FDirtbagRival& In)
{
	dirtbag::Rival Out;
	Out.name = TCHAR_TO_UTF8(*In.Name);
	Out.style = static_cast<dirtbag::RouteType>(In.Style);
	Out.vibe = static_cast<dirtbag::RivalVibe>(In.Vibe);
	Out.generation = In.Generation;
	Out.bornOnDay = In.BornOnDay;
	Out.startAge = In.StartAge;
	Out.grade = In.Grade;
	Out.lastStepDay = In.LastStepDay;
	Out.peakGrade = In.PeakGrade;
	Out.rivalry = In.Rivalry;
	Out.allied = In.bAllied;
	Out.offered = In.bOffered;
	Out.met = In.bMet;
	Out.firstAscents.reserve(In.FirstAscents.Num());
	for (const FString& Key : In.FirstAscents)
	{
		Out.firstAscents.push_back(TCHAR_TO_UTF8(*Key));
	}
	Out.retired = In.bRetired;
	Out.race.routeName = TCHAR_TO_UTF8(*In.Race.RouteName);
	Out.race.byDay = In.Race.ByDay;
	Out.race.forFirstAscent = In.Race.bForFirstAscent;
	return Out;
}

FDirtbagPastRival FromSim(const dirtbag::PastRival& In)
{
	FDirtbagPastRival Out;
	Out.Name = FString(In.name.c_str());
	Out.Role = static_cast<EDirtbagRivalRole>(In.role);
	Out.Style = static_cast<EDirtbagRouteType>(In.style);
	Out.RetiredOnDay = In.retiredOnDay;
	Out.Age = In.age;
	Out.PeakGrade = In.peakGrade;
	Out.Generation = In.generation;
	return Out;
}

dirtbag::PastRival ToSim(const FDirtbagPastRival& In)
{
	dirtbag::PastRival Out;
	Out.name = TCHAR_TO_UTF8(*In.Name);
	Out.role = static_cast<dirtbag::RivalRole>(In.Role);
	Out.style = static_cast<dirtbag::RouteType>(In.Style);
	Out.retiredOnDay = In.RetiredOnDay;
	Out.age = In.Age;
	Out.peakGrade = In.PeakGrade;
	Out.generation = In.Generation;
	return Out;
}

FDirtbagCharacter FromSim(const dirtbag::Character& In)
{
	FDirtbagCharacter Out;
	Out.bBuilt = In.built;
	Out.Archetype = static_cast<EDirtbagArchetype>(In.build.archetype);
	Out.Origin = static_cast<EDirtbagOrigin>(In.build.origin);
	Out.Flaw = static_cast<EDirtbagFlaw>(In.build.flaw);
	Out.Temperament = static_cast<EDirtbagTemperament>(In.build.temperament);
	Out.Discipline = In.personality.discipline;
	Out.Boldness = In.personality.boldness;
	Out.Social = In.personality.social;
	Out.Purism = In.personality.purism;
	Out.Gift = static_cast<EDirtbagTalent>(In.gift);
	Out.AntiTalent = static_cast<EDirtbagTalent>(In.antiTalent);
	Out.bGiftKnown = In.giftKnown;
	Out.bAntiKnown = In.antiKnown;
	Out.Reps.Reset(dirtbag::kSkillCount);
	for (int32 i = 0; i < dirtbag::kSkillCount; i++)
	{
		Out.Reps.Add(In.reps[i]);
	}
	Out.StartingCash = In.startingCash;
	Out.AgePlus = In.agePlus;
	return Out;
}

dirtbag::Character ToSim(const FDirtbagCharacter& In)
{
	dirtbag::Character Out;
	Out.built = In.bBuilt;
	Out.build.archetype = static_cast<dirtbag::Archetype>(In.Archetype);
	Out.build.origin = static_cast<dirtbag::Origin>(In.Origin);
	Out.build.flaw = static_cast<dirtbag::Flaw>(In.Flaw);
	Out.build.temperament = static_cast<dirtbag::Temperament>(In.Temperament);
	Out.personality.discipline = In.Discipline;
	Out.personality.boldness = In.Boldness;
	Out.personality.social = In.Social;
	Out.personality.purism = In.Purism;
	Out.gift = static_cast<dirtbag::Talent>(In.Gift);
	Out.antiTalent = static_cast<dirtbag::Talent>(In.AntiTalent);
	Out.giftKnown = In.bGiftKnown;
	Out.antiKnown = In.bAntiKnown;
	// A mirror arriving with the wrong number of lanes is a mirror from
	// another version; take what is there and leave the rest at zero rather
	// than reading off the end of it.
	for (int32 i = 0; i < dirtbag::kSkillCount && i < In.Reps.Num(); i++)
	{
		Out.reps[i] = In.Reps[i];
	}
	Out.startingCash = In.StartingCash;
	Out.agePlus = In.AgePlus;
	return Out;
}

FDirtbagShoes FromSim(const dirtbag::Shoes& In)
{
	FDirtbagShoes Out;
	Out.Wear = In.wear;
	Out.Resoles = In.resoles;
	Out.PairsOwned = In.pairsOwned;
	return Out;
}

dirtbag::Shoes ToSim(const FDirtbagShoes& In)
{
	dirtbag::Shoes Out;
	Out.wear = In.Wear;
	Out.resoles = In.Resoles;
	Out.pairsOwned = In.PairsOwned;
	return Out;
}

FDirtbagVan FromSim(const dirtbag::Van& In)
{
	FDirtbagVan Out;
	Out.HoursDriven = In.hoursDriven;
	Out.bRig = In.rig;
	Out.Parts.Reserve(dirtbag::kVanPartCount);
	for (int i = 0; i < dirtbag::kVanPartCount; i++)
	{
		FDirtbagVanPart P;
		P.Wear = In.parts[i].wear;
		P.Patches = In.parts[i].patches;
		P.bFailed = In.parts[i].failed;
		Out.Parts.Add(P);
	}
	return Out;
}

dirtbag::Van ToSim(const FDirtbagVan& In)
{
	dirtbag::Van Out;
	Out.hoursDriven = In.HoursDriven;
	Out.rig = In.bRig;
	// A mirror arriving with the wrong number of parts would silently drop
	// or invent damage, so only copy what is actually there.
	const int32 n = FMath::Min(In.Parts.Num(), dirtbag::kVanPartCount);
	for (int32 i = 0; i < n; i++)
	{
		Out.parts[i].wear = In.Parts[i].Wear;
		Out.parts[i].patches = In.Parts[i].Patches;
		Out.parts[i].failed = In.Parts[i].bFailed;
	}
	return Out;
}

FDirtbagPartnerBond FromSim(const dirtbag::PartnerBond& In)
{
	FDirtbagPartnerBond Out;
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.Rapport = In.rapport;
	Out.EverKnew = In.everKnew;
	Out.FirstAscents.Reserve(static_cast<int32>(In.firstAscents.size()));
	for (const std::string& Key : In.firstAscents)
	{
		Out.FirstAscents.Add(UTF8_TO_TCHAR(Key.c_str()));
	}
	return Out;
}

dirtbag::PartnerBond ToSim(const FDirtbagPartnerBond& In)
{
	dirtbag::PartnerBond Out;
	Out.name = TCHAR_TO_UTF8(*In.Name);
	Out.rapport = In.Rapport;
	Out.everKnew = In.EverKnew;
	Out.firstAscents.reserve(static_cast<size_t>(In.FirstAscents.Num()));
	for (const FString& Key : In.FirstAscents)
	{
		Out.firstAscents.push_back(TCHAR_TO_UTF8(*Key));
	}
	return Out;
}

FDirtbagPartner FromSim(const dirtbag::Partner& In)
{
	FDirtbagPartner Out;
	// mirror-skip: ambition -- how hard they will chase a line of their own.
	// It decides what the Lot does behind your back and is deliberately not
	// something you can read off a person's face.
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.EverKnew = In.everKnew;
	Out.Tag = UTF8_TO_TCHAR(In.tag.c_str());
	Out.Climber = FromSim(In.climber);
	Out.bClimbs = In.climbs;
	Out.Rapport = In.rapport;
	Out.FirstAscents.Reserve(static_cast<int32>(In.firstAscents.size()));
	for (const std::string& Key : In.firstAscents)
	{
		Out.FirstAscents.Add(UTF8_TO_TCHAR(Key.c_str()));
	}
	return Out;
}

}  // namespace DirtbagConvert
