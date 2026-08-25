#include "DirtbagRival.h"

#include <algorithm>
#include <cmath>

#include "DirtbagSession.h"   // SkillToGrade / GradeToSkill

namespace dirtbag {
namespace {

// Twelve, because a career spans three or four of them and a pool that runs
// dry inside one save is a pool that reads as a list.
const char* kNames[] = {
    "Dex Calloway", "Silas Mott",  "Rafe Linden", "Knox Bauer",
    "Roman Tate",   "Gideon Pace", "Bex Hollis",  "Juno Hale",
    "Esme Vaughn",  "Lux Mercer",  "Vera Lyle",   "Mika Stroud",
};
constexpr int kNameCount = 12;

// What a climber who is soft *there* will be beaten by. The rival's style
// leans toward your weakest skill rather than rolling free -- they are built
// to exploit what you are bad at, which is what makes them feel personal
// instead of generated.
RouteType StyleAgainst(const Skills& s, Rng& rng) {
  double worst = s.power;
  int which = 0;
  const double vals[5] = {s.power, s.fingers, s.technique, s.endurance,
                          s.head};
  for (int i = 1; i < 5; i++) {
    if (vals[i] < worst) { worst = vals[i]; which = i; }
  }
  switch (which) {
    case 0:  // weak power -> they are strong, or they jump
      return rng.Chance(0.5) ? RouteType::Power : RouteType::Dyno;
    case 1:  // weak fingers -> small edges
      return RouteType::Crimp;
    case 2:  // weak technique -> footwork, or a crack you cannot read
      return rng.Chance(0.5) ? RouteType::Technical : RouteType::Crack;
    case 3:  // weak endurance -> they never pump out
      return RouteType::Endurance;
    default: // weak head -> they commit where you do not
      return rng.Chance(0.5) ? RouteType::Dyno : RouteType::Technical;
  }
}

}  // namespace

const char* VibeText(RivalVibe v) {
  switch (v) {
    case RivalVibe::Foil:    return "a friendly foil";
    case RivalVibe::Nemesis: return "a bitter nemesis";
    default:                 return "a quiet benchmark";
  }
}

const char* RoleText(RivalRole r) {
  switch (r) {
    case RivalRole::Coach:  return "coaching at the gym";
    case RivalRole::Author: return "writing the guidebook";
    default:                return "gone";
  }
}

const char* StyleText(RouteType t) {
  switch (t) {
    case RouteType::Crimp:      return "a crimp assassin";
    case RouteType::Power:      return "a power monster";
    case RouteType::Dyno:       return "a dyno wizard";
    case RouteType::Endurance:  return "an endurance machine";
    case RouteType::Technical:  return "a footwork technician";
    default:                    return "a crack fiend";
  }
}

Rival RollRival(const Rng& worldRng, const Skills& yours, int day,
                int generation, double yourGrade, const RivalDials& dials) {
  Rng rng = worldRng.Derive("rival#" + std::to_string(generation));
  Rival r;
  r.name = kNames[rng.IntRange(0, kNameCount - 1)];
  r.style = StyleAgainst(yours, rng);
  r.vibe = static_cast<RivalVibe>(rng.IntRange(0, kRivalVibeCount - 1));
  r.generation = generation;
  r.bornOnDay = day;
  r.startAge = dials.rivalStartAge;
  r.lastStepDay = day;
  // The first one was already here when you arrived, and already better.
  r.grade = std::min(dials.gradeCap, yourGrade + dials.lead);
  r.peakGrade = r.grade;
  return r;
}

double RivalAge(const Rival& r, int day, const RivalDials& dials) {
  const int days = std::max(0, day - r.bornOnDay);
  return r.startAge +
         static_cast<double>(days) / static_cast<double>(dials.daysPerYear);
}

bool CanImprove(const Rival& r, int day, const RivalDials& dials) {
  return !r.retired && RivalAge(r, day, dials) < dials.peakAge;
}

bool RivalDay(Rival& r, double yourGrade, int day, const RivalDials& dials) {
  if (r.retired) return false;
  // A successor climbs about twice as fast, because they always do.
  const int every =
      r.generation > 0 ? dials.successorStepEveryDays : dials.stepEveryDays;
  if (day - r.lastStepDay < every) return false;
  if (!CanImprove(r, day, dials)) return false;

  // They chase, they do not teleport. If you are already past them they
  // close the gap a step at a time like everybody else, which is what makes
  // going past them mean something -- you get a season of being ahead
  // rather than one frame of it.
  const double target = std::min(dials.gradeCap, yourGrade + dials.lead);
  if (r.grade >= target) return false;

  r.lastStepDay = day;
  r.grade = std::min(target, r.grade + dials.stepSize);
  r.peakGrade = std::max(r.peakGrade, r.grade);
  return true;
}

bool AreTheyAhead(const Rival& r, double yourGrade) {
  return !r.retired && r.grade > yourGrade;
}

bool ThinkingAboutIt(const Rival& r, const Rng& worldRng, int day,
                     const RivalDials& dials) {
  if (r.retired) return false;
  if (RivalAge(r, day, dials) < dials.retireAge) return false;
  // Once a season, on its own stream. Derived from the season index rather
  // than the day so that asking twice in the same season is the same answer
  // -- a caller that checks daily must not get 91 rolls at it.
  const int season = day / std::max(1, dials.seasonDays);
  Rng rng = worldRng.Derive("rival-retire#" + std::to_string(r.generation) +
                            "#" + std::to_string(season));
  return rng.Chance(dials.retireChancePerSeason);
}

PastRival Retire(const Rival& r, const Rng& worldRng, int day,
                 const RivalDials& dials) {
  PastRival p;
  p.name = r.name;
  p.style = r.style;
  p.retiredOnDay = day;
  p.age = RivalAge(r, day, dials);
  p.peakGrade = r.peakGrade;
  p.generation = r.generation;

  Rng rng = worldRng.Derive("rival-role#" + std::to_string(r.generation));
  const double roll = rng.NextDouble();
  if (r.allied) {
    // You two ended up friends. They stay close.
    p.role = roll < 0.65 ? RivalRole::Coach : RivalRole::Author;
  } else if (r.rivalry < -dials.allyAt) {
    // They beat you and left. That is the one that stings, and it is the
    // only branch where "gone" is likely.
    p.role = roll < 0.5 ? RivalRole::Author : RivalRole::Gone;
  } else {
    p.role = roll < 0.40   ? RivalRole::Coach
             : roll < 0.75 ? RivalRole::Author
                           : RivalRole::Gone;
  }
  return p;
}

Rival Succeed(const Rng& worldRng, const Skills& yours, double yourGrade,
              int day, int generation,
              const std::string& theyAlreadyHaveAName,
              const RivalDials& dials) {
  Rival r = RollRival(worldRng, yours, day, generation, yourGrade, dials);
  // **A name you already know.** Everything else about them is rolled the
  // same way a stranger's is -- you coached them to sixteen, not into a
  // style -- but the name on the board is one you chose off a list of
  // fourteen when they were eleven.
  if (!theyAlreadyHaveAName.empty()) r.name = theyAlreadyHaveAName;
  // The new one starts below you rather than ahead. **This is the moment a
  // career turns over**: for the first time somebody is chasing you, and
  // they are gaining twice as fast as the last one did.
  r.grade = std::max(0.0, yourGrade - dials.successorLag);
  r.peakGrade = r.grade;
  // Younger than the one who just went.
  r.startAge = dials.rivalStartAge - 2.0;
  return r;
}

void TheyGotThereFirst(Rival& r, const std::string& routeName,
                       const RivalDials& dials) {
  if (std::find(r.firstAscents.begin(), r.firstAscents.end(), routeName) !=
      r.firstAscents.end()) {
    return;   // already theirs; taking it twice is not two defeats
  }
  r.firstAscents.push_back(routeName);
  r.rivalry -= dials.faSwing;
}

void YouGotThereFirst(Rival& r, const RivalDials& dials) {
  r.rivalry += dials.faSwing;
}

bool WouldPartnerUp(const Rival& r, const RivalDials& dials) {
  return !r.retired && !r.allied && !r.offered && r.rivalry >= dials.allyAt;
}

bool RaceIsOn(const Rival& r) { return !r.race.routeName.empty(); }

bool StartARace(Rival& r, const Crag& crag, double yourGrade,
                const std::vector<std::string>& spokenFor, const Rng& worldRng,
                int day, const RivalDials& dials) {
  if (r.retired || r.allied) return false;
  if (RaceIsOn(r)) return false;
  if (yourGrade < dials.raceMinGrade) return false;

  // Its own stream, per day. A race starting must never shift the rng an
  // attempt resolves on, and asking twice in one day must be one answer.
  Rng rng = worldRng.Derive("race#" + std::to_string(day));
  if (!rng.Chance(dials.raceChancePerDay)) return false;

  const bool wantFa = rng.Chance(dials.raceForFaChance);

  // A line worth racing for is one **you could plausibly do** -- within a
  // grade of you either way. Racing you for something three grades up is
  // not a race, it is an announcement.
  std::vector<int> candidates;
  for (std::size_t i = 0; i < crag.lines.size(); i++) {
    const CragLine& line = crag.lines[i];
    if (line.isProject != wantFa) continue;
    if (wantFa &&
        std::find(spokenFor.begin(), spokenFor.end(), line.route.name) !=
            spokenFor.end()) {
      continue;
    }
    if (!wantFa && !line.firstAscentBy.empty() &&
        line.firstAscentBy == r.name) {
      continue;   // they are not racing you for their own line
    }
    const double g = static_cast<double>(line.route.grade);
    if (g > yourGrade + 1.0 || g < yourGrade - 1.5) continue;
    candidates.push_back(static_cast<int>(i));
  }
  if (candidates.empty()) return false;

  const CragLine& pick =
      crag.lines[candidates[rng.IntRange(
          0, static_cast<int>(candidates.size()) - 1)]];
  r.race.routeName = pick.route.name;
  r.race.byDay = day + dials.raceDays;
  r.race.forFirstAscent = pick.isProject;
  r.met = true;   // you cannot be raced by a stranger
  return true;
}

bool RaceRanOut(const Rival& r, int day) {
  return RaceIsOn(r) && day >= r.race.byDay;
}

void YouWonTheRace(Rival& r, const RivalDials& dials) {
  if (!RaceIsOn(r)) return;
  r.rivalry += dials.raceWinSwing;
  r.race = Race{};
}

void TheyWonTheRace(Rival& r, const RivalDials& dials) {
  if (!RaceIsOn(r)) return;
  // An open line is gone rather than merely climbed by somebody else first,
  // and the head-to-head says so.
  if (r.race.forFirstAscent) {
    r.rivalry -= dials.raceLoseFaSwing;
    TheyGotThereFirst(r, r.race.routeName, dials);
  } else {
    r.rivalry -= dials.raceLoseSwing;
  }
  r.race = Race{};
}

std::string RaceLine(const Rival& r, int day) {
  if (!RaceIsOn(r)) return std::string();
  const int left = r.race.byDay - day;
  std::string s = r.name;
  s += r.race.forFirstAscent ? " has been looking at " : " is working ";
  s += r.race.routeName;
  s += ".";
  // Days in words rather than a number, like everything else the HUD says
  // slowly. A countdown is a timer; this is a thing somebody told you.
  if (left <= 0) {
    s += " You are out of time.";
  } else if (left == 1) {
    s += " You have got today.";
  } else if (left == 2) {
    s += " A couple of days, at most.";
  } else {
    s += " Not for much longer.";
  }
  if (r.race.forFirstAscent) {
    s += " Nobody has done it yet.";
  }
  return s;
}

Partner AsAClimber(const Rival& r, const Rng& worldRng, int day,
                   const RivalDials& dials) {
  Partner p;
  p.name = r.name;
  p.tag = StyleText(r.style);
  p.climbs = true;
  // Their grade *is* their strength. Derived from the rival rather than
  // rolled again, so the person the book records and the person the HUD
  // reports are the same climber.
  p.ambition = 0.95;
  // **Their grade converted to a skill, not handed over as one.**
  // `PartnerOn` takes skill points, 0..100, and a grade is 0..18 -- the
  // first version passed the grade straight through, which made the rival a
  // beginner who took **zero lines in thirty years** while ageing and
  // retiring perfectly convincingly. Nothing in the suite noticed, because
  // every test asked whether the machinery ran rather than whether it did
  // anything.
  //
  // Day 1 rather than `day`, because `PartnerOn` adds its own creep for
  // ambitious partners and the rival's improvement is `RivalDay`'s job.
  // Letting both run would have them improving twice.
  p.climber = PartnerOn(worldRng, r.name, GradeToSkill(r.grade), p.ambition,
                        1);
  p.firstAscents = r.firstAscents;
  (void)dials;
  (void)day;   // see the note above: their creep is RivalDay's job
  return p;
}

std::string RivalLine(const Rival& r, double yourGrade, int day,
                      const RivalDials& dials) {
  // A number for somebody you have never been introduced to is a
  // leaderboard, not a rival.
  if (!r.met) return std::string();

  if (r.retired) {
    return r.name + " does not climb like that any more.";
  }
  if (r.allied) {
    return r.name + " is climbing with you now.";
  }

  const double gap = r.grade - yourGrade;
  std::string s = r.name;
  if (gap > 0.5) {
    s += " is still ahead of you.";
  } else if (gap > -0.5) {
    // The interesting state, and the one worth its own sentence.
    s += " is right there.";
  } else {
    s += " is behind you, for now.";
  }

  // Said only once they are past their best, because it is the thing that
  // changes what the chase means: you are no longer catching somebody, you
  // are outlasting them.
  if (!CanImprove(r, day, dials)) {
    s += " Not getting any better, either.";
  }
  return s;
}

}  // namespace dirtbag
