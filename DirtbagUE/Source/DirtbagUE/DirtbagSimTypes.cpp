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
	Out.SkinLeft = In.skinLeft;
	Out.Warmth = In.warmth;
	Out.Psyche = In.psyche;
	Out.AttemptsMade = In.attemptsMade;
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
	return Out;
}

dirtbag::PlayerState ToSim(const FDirtbagPlayerState& In)
{
	dirtbag::PlayerState Out;
	Out.climber = ToSim(In.Climber);
	Out.cash = In.Cash;
	Out.day = In.Day;
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
	Out.HighTempF = In.highTempF;
	Out.LowTempF = In.lowTempF;
	Out.Humidity = In.humidity;
	Out.Cloud = In.cloud;
	Out.Wind = In.wind;
	return Out;
}

dirtbag::Weather ToSim(const FDirtbagWeather& In)
{
	dirtbag::Weather Out;
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

}  // namespace DirtbagConvert
