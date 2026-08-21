#include "DirtbagSimTypes.h"

// The mirrors cast by value between enum sets; if the sim reorders, these
// fail the build instead of silently corrupting data.
static_assert(static_cast<int>(EDirtbagHold::Crack) == static_cast<int>(dirtbag::HoldType::Crack), "Hold enums out of sync");
static_assert(static_cast<int>(EDirtbagRouteType::Crack) == static_cast<int>(dirtbag::RouteType::Crack), "RouteType enums out of sync");
static_assert(static_cast<int>(EDirtbagDiscipline::Sport) == static_cast<int>(dirtbag::Discipline::Sport), "Discipline enums out of sync");
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
	Out.climber = ToSim(In.Climber);
	Out.cash = In.Cash;
	Out.day = In.Day;
	Out.dog = ToSim(In.Dog);
	Out.shoes = ToSim(In.Shoes);
	Out.van = ToSim(In.Van);
	Out.owed = In.Owed;
	Out.job = ToSim(In.Job);
	Out.crew = ToSim(In.Crew);
	Out.standing = ToSim(In.Standing);
	Out.kit = ToSim(In.Kit);
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
	Out.session = ToSim(In.Session);
	return Out;
}

FDirtbagPlayerState FromSim(const dirtbag::PlayerState& In)
{
	FDirtbagPlayerState Out;
	Out.Climber.Power = In.climber.skills.power;
	Out.Climber.Fingers = In.climber.skills.fingers;
	Out.Climber.Technique = In.climber.skills.technique;
	Out.Climber.Endurance = In.climber.skills.endurance;
	Out.Climber.Head = In.climber.skills.head;
	Out.Climber.Morphology = static_cast<EDirtbagMorphology>(In.climber.morphology);
	Out.Climber.Skin = In.climber.skin;
	Out.Climber.Psyche = In.climber.psyche;
	Out.Cash = In.cash;
	Out.Day = In.day;
	Out.Dog = FromSim(In.dog);
	Out.Shoes = FromSim(In.shoes);
	Out.Van = FromSim(In.van);
	Out.Owed = In.owed;
	Out.Job = FromSim(In.job);
	Out.Crew = FromSim(In.crew);
	Out.Standing = FromSim(In.standing);
	Out.Kit = FromSim(In.kit);
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
	return Out;
}

dirtbag::Injury ToSim(const FDirtbagInjury& In)
{
	dirtbag::Injury Out;
	Out.active = In.bActive;
	Out.kind = static_cast<dirtbag::InjuryKind>(In.Kind);
	Out.severity = In.Severity;
	Out.daysLeft = In.DaysLeft;
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
