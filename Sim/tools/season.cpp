// Play a season headless.
//
// Phase 2's remaining gate is "a full season plays start to finish", and the
// honest way to test that without a person is to actually play it: a policy
// that wakes up, reads the forecast, decides between the crag, a shift and
// the fire, spends skin, feeds the dog, sleeps, and does it again. Every
// call below is the same function the game calls.
//
// This is a probe, not a test. It answers "does a year hold together, and
// what does it feel like from the numbers" — the things a harness assertion
// cannot ask.
//
// Build: Sim/tools/build-season.sh

#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>

#include "DirtbagConditions.h"
#include "DirtbagCore.h"
#include "DirtbagCrag.h"
#include "DirtbagDay.h"
#include "DirtbagDog.h"
#include "DirtbagFirstAscent.h"
#include "DirtbagGear.h"
#include "DirtbagVan.h"
#include "DirtbagJobs.h"
#include "DirtbagPartner.h"
#include "DirtbagSave.h"
#include "DirtbagSession.h"
#include "DirtbagSessionLoop.h"

using namespace dirtbag;

namespace {

struct LineTally { std::string name; int burns = 0, sends = 0, best = 0; int moves = 0; };

struct Tally {
  int daysClimbed = 0, daysWorked = 0, daysRested = 0, daysWashedOut = 0;
  int burns = 0, sends = 0, firstAscents = 0;
  int mealsEaten = 0, dogMeals = 0, brokeDays = 0, starvedNights = 0;
  double cashLow = 1e9, cashHigh = -1e9;
  int linesLostToTheLot = 0;
  std::vector<LineTally> perLine;
  double earned = 0.0, spentFood = 0.0, spentDog = 0.0, spentBills = 0.0;
  double spentShoes = 0.0;
  int resoles = 0, newPairs = 0, deadRubberDays = 0;
  double spentVan = 0.0;
  int breakdowns = 0, strandedDays = 0, bodges = 0;
  double vanHoursLost = 0.0;
  int movesClimbed = 0;
  double warmthSum = 0.0, skinSum = 0.0, oddsSum = 0.0;
  int oddsN = 0;
  int missedWindows = 0;
};

// Which line to point at today: the hardest thing that still reads as
// climbable, preferring an open project — a player chases the thing that
// could be theirs.
const CragLine* PickLine(const Crag& crag, const Climber& c,
                         const PlayerState& player) {
  const CragLine* best = nullptr;
  for (const CragLine& l : crag.lines) {
    const RouteRead read = ReadRoute(c, l.route);
    if (read == RouteRead::NotThisYear) continue;

    // Walk away from a line that is going nowhere. Real players do this;
    // the first run of this probe did not, and spent 519 burns getting one
    // move up a line that turned out to be two grades harder than the book
    // claimed. Warmth is earned by climbing moves, so a line you cannot
    // start is one you can never warm up on — the trap feeds itself.
    bool hopeless = false;
    for (const ProjectMemory& m : player.projects)
      if (m.routeName == l.route.name && m.attempts > 40 &&
          m.bestHighpoint * 3 < static_cast<int>(l.route.moves.size()))
        hopeless = true;   // a `continue` here would skip the ledger, not
    if (hopeless) continue;  // the line — which is not the same thing at all

    // Sent is sent. (The first run of this probe excluded projects from
    // that check, so after its one first ascent the player spent the rest
    // of the year re-sending the same line — twenty sends against a single
    // ledger, which is what gave the game away.)
    bool done = false;
    for (const ProjectMemory& m : player.projects)
      if (m.routeName == l.route.name && m.sent) done = true;
    if (done) continue;

    // Nobody spends a year on a line they cannot start. Prefer something at
    // the limit over something out of reach, and an open project over a
    // line that is already somebody's.
    const RouteRead r = ReadRoute(c, l.route);
    const int worth = (r == RouteRead::AtYourLimit   ? 3
                       : r == RouteRead::Project     ? 2
                       : r == RouteRead::Comfortable ? 1
                                                     : 0) +
                      (l.isProject ? 2 : 0);
    if (!best) { best = &l; continue; }
    const RouteRead br = ReadRoute(c, best->route);
    const int bestWorth = (br == RouteRead::AtYourLimit   ? 3
                           : br == RouteRead::Project     ? 2
                           : br == RouteRead::Comfortable ? 1
                                                          : 0) +
                          (best->isProject ? 2 : 0);
    if (worth > bestWorth ||
        (worth == bestWorth && l.route.trueGrade > best->route.trueGrade))
      best = &l;
  }
  return best;
}

ProjectMemory& LedgerFor(PlayerState& player, const CragLine& line) {
  for (ProjectMemory& m : player.projects)
    if (m.routeName == line.route.name) return m;
  player.projects.push_back(NewProjectLedger(line));
  return player.projects.back();
}

}  // namespace

int main(int argc, char** argv) {
  const int DAYS = argc > 1 ? std::atoi(argv[1]) : 365;
  const std::string seed = argc > 2 ? argv[2] : "crag-1";
  // Skin you insist on having before pulling on. 0 is the grinder who
  // climbs every day there is a window; higher is somebody who rests.
  const double restUntilSkin = argc > 3 ? std::atof(argv[3]) : 0.0;
  // Arg 4 is "q" for the one-line form, anything else (or absent) verbose.
  // Arg 5 is "salary" to take the job on day one and never quit.
  const bool quiet = argc > 4 && std::string(argv[4]) == "q";
  const bool takeTheSalary = argc > 5 && std::string(argv[5]) == "salary";

  const Rng world = Rng::FromSeed(seed);
  const Crag crag = RoadsideCrag(world);
  ConditionsDials cd;
  DayDials dd;
  FirstAscentDials fd;
  DogDials dog;

  PlayerState player;
  player.climber.skills.power = player.climber.skills.fingers =
      player.climber.skills.technique = player.climber.skills.endurance =
          player.climber.skills.head = 50.0;

  if (takeTheSalary) TakeSalariedJob(player);

  Tally t;
  std::vector<std::string> lotTaken;
  int lastGradeReport = 0;

  if (!quiet) {
    printf("=== a season at %s (%d days, seed %s, rest until skin %.1f) ===\n\n",
           crag.name.c_str(), DAYS, seed.c_str(), restUntilSkin);
    printf("%5s %6s %7s %6s %6s %5s %5s  %s\n", "day", "cash", "grade",
           "skin", "psyche", "sends", "FAs", "what happened");
  }

  for (int day = 1; day <= DAYS; day++) {
    DayState today = WakeUp(player, dd);
    const Weather w = GenerateWeather(world, player.day, cd);
    const PrimeWindow win = FindPrimeWindow(w, crag.aspect, cd);

    std::string note;

    // Eat when hungry — checked through the day rather than at dawn, when
    // nobody is. (The first run of this probe never ate once, because it
    // asked at wake-up: a probe bug, but it also proved hunger was never
    // biting hard enough to notice, which was worth knowing.)
    // Feed the dog every other day or so.
    if (player.dog.fed < 0.6) {
      const double before = player.cash;
      if (FeedDog(player.dog, player.cash, dog)) {
        t.dogMeals++;
        t.spentDog += before - player.cash;
      }
    }

    // The salary owns its days whether or not you wanted them.
    JobDials jd;
    bool needMoney = false;
    if (SalariedToday(player.job, player.day, jd)) {
      WorkSalariedDay(player, today, jd, dd);
      t.earned += SalaryDayPay(jd);
      t.daysWorked++;
      note = "work";
    } else {
      // Otherwise: rent comes first. Below a float, take a gig off the
      // board — the best-paying one you can actually do.
      needMoney = player.cash < 120.0 || player.owed > 0.0;
      if (needMoney) {
        const std::vector<OddJob> board = OddJobBoard(world, player.day, jd);
        const OddJob* best = nullptr;
        for (const OddJob& g : board) {
          if (g.needsVan && !VanRuns(player.van)) continue;
          if (!best || g.pay > best->pay) best = &g;
        }
        if (best) {
          // Earned is what the gig paid, not what reached the pocket —
          // debt takes its cut first and that is not lost income.
          if (WorkOddJob(player, today, *best, dd)) {
            t.earned += best->pay;
            t.daysWorked++;
            note = "gig";
          }
        }
      }
    }

    // Drive to the crag and back, if the van goes. Half an hour each way.
    VanDials vd;
    if (VanRuns(player.van) && win.exists) {
      const int broke = DriveVan(player.van, world, player.day, 1.0,
                                 TemperatureAt(w, 14.0, cd), vd);
      if (broke >= 0) {
        t.breakdowns++;
        note = std::string("the ") +
               VanPartName(static_cast<VanPart>(broke)) + " went";
      }
    }

    // Stranded: fix it the best way you can afford. Replace properly if
    // there is money, patch if not, and bodge if there is nothing at all —
    // which always works, and always costs the morning.
    if (!VanRuns(player.van)) {
      t.strandedDays++;
      const int bad = WorstVanPart(player.van, vd);
      if (bad >= 0) {
        const VanPart part = static_cast<VanPart>(bad);
        double hours = 0.0;
        const double before = player.cash;
        if (!ReplaceVanPart(player.van, part, player.cash, hours, vd) &&
            !PatchVan(player.van, part, player.cash, hours, vd)) {
          BodgeVan(player.van, part, hours, vd);
          t.bodges++;
        }
        t.spentVan += before - player.cash;
        t.vanHoursLost += hours;
        PassHours(today, hours, dd);
      }
    }

    const bool tooThin = player.climber.skin < restUntilSkin;
    if (tooThin && win.exists) {
      Rest(today, 4.0, dd);
      t.daysRested++;
      note = note.empty() ? "resting skin" : note + " + resting skin";
    }

    const bool stranded = !VanRuns(player.van);
    if (!win.exists || tooThin || stranded) {
      if (!win.exists) {
        if (!needMoney && !tooThin) {
          Rest(today, 4.0, dd);
          t.daysRested++;
          note = "washed out";
        }
        t.daysWashedOut++;
      }
    } else {
      // Wait for the window, then spend skin in it.
      // Wait for the window if it is still ahead. If it has already gone —
      // which is what a working day does to a winter window — you climb in
      // whatever is left, and that is the whole cost of having a job.
      if (today.hour < win.startHour) {
        Rest(today, win.startHour - today.hour, dd);
      }
      const bool missedIt = today.hour > win.endHour;
      if (missedIt) t.missedWindows++;

      StartGymSession(player, today, dd);
      const Climber body = ClimberForSession(player, today, dd);
      const CragLine* line = PickLine(crag, body, player);
      if (line) {
        ProjectMemory& mem = LedgerFor(player, *line);

        // Clean it if it needs it — the cost that comes before any chance.
        while (!IsWorkable(mem, fd) && today.hour < win.endHour)
          CleanLine(player, today, mem, 0.5, fd, dd);

        // Conditions where the clock actually is, not where the window was.
        const double climbAt =
            missedIt ? today.hour : win.peakHour;
        const Conditions cond = ConditionsAt(w, crag.aspect, climbAt, cd);
        Rng session = Rng::FromSeed(seed + "#day" + std::to_string(player.day));
        int burnsToday = 0;
        while (today.session.skinLeft > 0.5 &&
               burnsToday < static_cast<int>(win.hours() / 0.25) + 4) {
          // The two halves the engine's DayAttempt node wraps: resolve the
          // burn against the session, then let the day pay for it.
          const AttemptResult r =
              AttemptInSession(session, today.session, mem, body, line->route,
                               cond);
          ApplyAttemptToDay(player, today, line->route, r, dd);
          t.burns++;
          burnsToday++;

          LineTally* lt = nullptr;
          for (LineTally& x : t.perLine)
            if (x.name == line->route.name) lt = &x;
          if (!lt) {
            t.perLine.push_back(
                {line->route.name, 0, 0, 0,
                 static_cast<int>(line->route.moves.size())});
            lt = &t.perLine.back();
          }
          lt->burns++;
          lt->best = std::max(lt->best, r.highpoint);
          if (r.sent) lt->sends++;
          t.movesClimbed += static_cast<int>(r.timeline.size());
          t.warmthSum += today.session.warmth;
          t.skinSum += today.session.skinLeft;
          if (!r.timeline.empty()) {
            t.oddsSum += r.timeline[0].odds;
            t.oddsN++;
          }
          if (r.sent) {
            t.sends++;
            if (CanName(*line, mem)) {
              NameFirstAscent(mem, *line, "Line " + std::to_string(player.day));
              t.firstAscents++;
              note += (note.empty() ? "" : " + ");
              note += "FIRST ASCENT of " + line->description;
            }
            break;
          }
        }
        if (burnsToday > 0) { t.daysClimbed++; if (note.empty()) note = "climbed"; }
      }
    }

    // Rubber: resole while the uppers hold, replace when they do not.
    GearDials gd;
    if (player.shoes.wear > gd.noticeablyWorn) {
      const double before = player.cash;
      if (CanResole(player.shoes, gd)) {
        if (Resole(player.shoes, player.cash, gd)) {
          t.resoles++;
          t.spentShoes += before - player.cash;
        }
      } else if (BuyNewShoes(player.shoes, player.cash, gd)) {
        t.newPairs++;
        t.spentShoes += before - player.cash;
      }
    }
    if (player.shoes.wear > 0.8) t.deadRubberDays++;

    // The Lot lives its life.
    std::vector<Partner> lot = LotRegulars(world, player.day);
    ApplyBonds(lot, player.bonds);
    for (Partner& p : lot) {
      SpendDayWith(p, t.daysClimbed > 0);
      std::vector<std::string> taken = lotTaken;
      for (const ProjectMemory& m : player.projects)
        if (m.firstAscent) taken.push_back(m.routeName);
      const int got = PartnerTakesFirstAscent(world, p, crag, taken, player.day);
      if (got >= 0) {
        lotTaken.push_back(crag.lines[got].route.name);
        p.firstAscents.push_back(crag.lines[got].route.name);
        t.linesLostToTheLot++;
        note += (note.empty() ? "" : " + ");
        note += p.name + " got " + crag.lines[got].description;
      }
    }
    player.bonds = BondsFrom(lot);

    // Evening: eat if the day has made you hungry and you can afford it.
    if (today.hunger > 28.0) {
      const double before = player.cash;
      if (EatMeal(player, today, dd)) {
        t.mealsEaten++;
        t.spentFood += before - player.cash;
      } else {
        t.brokeDays++;
      }
    }

    DogDay(player.dog, !needMoney, dog);
    WeatherProjects(player, fd);
    if (today.hunger > dd.starvingHunger) t.starvedNights++;

    t.cashLow = std::min(t.cashLow, player.cash);
    t.cashHigh = std::max(t.cashHigh, player.cash);

    const int grade = static_cast<int>(SkillToGrade(player.climber.skills.power));
    const bool interesting =
        note.find("FIRST ASCENT") != std::string::npos ||
        note.find("got") != std::string::npos || grade != lastGradeReport ||
        day % 60 == 0 || day == 1;
    if (interesting && !quiet) {
      printf("%5d %6.0f %6.1f %6.1f %6.2f %5d %5d  %s\n", day, player.cash,
             SkillToGrade(player.climber.skills.power),
             player.climber.skin, player.climber.psyche, t.sends,
             t.firstAscents, note.c_str());
      lastGradeReport = grade;
    }
    SleepToNextDay(player, today, dd);
  }

  if (quiet) {
    printf("%6.1f %8d %8d %8d %8d %9.1f %7.0f\n", restUntilSkin,
           t.daysClimbed, t.burns, t.sends, t.firstAscents,
           SkillToGrade(player.climber.skills.power), player.cash);
    return 0;
  }

  printf("\n=== after %d days ===\n", DAYS);
  printf("  climbed %d days, worked %d, rested %d; %d days never came good\n",
         t.daysClimbed, t.daysWorked, t.daysRested, t.daysWashedOut);
  printf("  %d burns, %d sends, %d first ascents\n", t.burns, t.sends,
         t.firstAscents);
  printf("  grade V%.1f -> V%.1f\n", 5.0,
         SkillToGrade(player.climber.skills.power));
  printf("  cash %.0f (low %.0f, high %.0f), broke on %d days, %d hungry "
         "nights\n", player.cash, t.cashLow, t.cashHigh, t.brokeDays,
         t.starvedNights);
  printf("  %d meals, %d tins of dog food; dog %s (bond %.2f)\n", t.mealsEaten,
         t.dogMeals, player.dog.adopted ? "adopted" : "still a stray",
         player.dog.bond);
  printf("  the Lot took %d lines; %zu open lines remain\n",
         t.linesLostToTheLot, OpenProjects(crag).size() - t.linesLostToTheLot -
                                  t.firstAscents);

  const double bills = static_cast<double>(DAYS / dd.billsEveryDays) *
                       dd.billsAmount;
  printf("\n  the money, over %d days:\n", DAYS);
  printf("    earned  $%7.0f from %d shifts\n", t.earned, t.daysWorked);
  printf("    bills   $%7.0f\n", bills);
  printf("    food    $%7.0f (%d meals)\n", t.spentFood, t.mealsEaten);
  printf("    dog     $%7.0f (%d tins)\n", t.spentDog, t.dogMeals);
  printf("    shoes   $%7.0f (%d resoles, %d new pairs; %d days on dead "
         "rubber)\n", t.spentShoes, t.resoles, t.newPairs, t.deadRubberDays);
  printf("    van     $%7.0f (%d breakdowns, %d days stranded, %d bodged, "
         "%.0f hours under it)\n", t.spentVan, t.breakdowns, t.strandedDays,
         t.bodges, t.vanHoursLost);
  printf("    -> %d windows arrived at after they had gone\n",
         t.missedWindows);
  printf("    -> %.0f%% of days worked to stay level; %d moves climbed\n",
         100.0 * t.daysWorked / DAYS, t.movesClimbed);

  printf("\n  average at the moment of pulling on: warmth %.2f, skin left "
         "%.1f, first-move odds %.0f%%\n",
         t.burns ? t.warmthSum / t.burns : 0.0,
         t.burns ? t.skinSum / t.burns : 0.0,
         t.oddsN ? 100.0 * t.oddsSum / t.oddsN : 0.0);
  printf("  where the burns went:\n");
  std::sort(t.perLine.begin(), t.perLine.end(),
            [](const LineTally& a, const LineTally& b) {
              return a.burns > b.burns;
            });
  for (size_t i = 0; i < t.perLine.size() && i < 8; i++) {
    const LineTally& x = t.perLine[i];
    printf("    %-40s %4d burns, %d sends, best %d of %d moves\n",
           x.name.c_str(), x.burns, x.sends, x.best, x.moves);
  }

  // A season has to survive a save, which is the other half of the gate.
  SaveGame save;
  save.seed = seed;
  save.player = player;
  SaveGame back;
  const LoadResult r = DeserializeSave(SerializeSave(save), back);
  printf("  save round-trip: %s (%zu ledgers, %zu bonds)\n",
         r == LoadResult::Ok ? "ok" : "FAILED", back.player.projects.size(),
         back.player.bonds.size());
  return 0;
}
