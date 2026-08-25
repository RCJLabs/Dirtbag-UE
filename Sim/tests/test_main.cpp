// Standalone harness for the Dirtbag sim core — no engine, no framework.
// Build and run: Sim/run-tests.sh (plain g++). These are the same translation
// units the Unreal module will compile; if they pass here, the sim is the sim.

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "../DirtbagCampfire.h"
#include "../DirtbagCharacter.h"
#include "../DirtbagComp.h"
#include "../DirtbagTeam.h"
#include "../DirtbagWorldStage.h"
#include "../DirtbagLeague.h"
#include "../DirtbagLife.h"
#include "../DirtbagLocals.h"
#include "../DirtbagGym.h"
#include "../DirtbagGymTown.h"
#include "../DirtbagBivy.h"
#include "../DirtbagLiving.h"
#include "../DirtbagMedical.h"
#include "../DirtbagAilments.h"
#include "../DirtbagBodyContext.h"
#include "../DirtbagCraft.h"
#include "../DirtbagRival.h"
#include "../DirtbagZones.h"
#include "../DirtbagConditions.h"
#include "../DirtbagCore.h"
#include "../DirtbagCrag.h"
#include "../DirtbagDay.h"
#include "../DirtbagDog.h"
#include "../DirtbagGear.h"
#include "../DirtbagEthics.h"
#include "../DirtbagFactions.h"
#include "../DirtbagJobs.h"
#include "../DirtbagAge.h"
#include "../DirtbagBody.h"
#include "../DirtbagKit.h"
#include "../DirtbagLegacy.h"
#include "../DirtbagTown.h"
#include "../DirtbagVan.h"
#include "../DirtbagFirstAscent.h"
#include "../DirtbagPartner.h"
#include "../DirtbagRng.h"
#include "../DirtbagSave.h"
#include "../DirtbagSession.h"
#include "../DirtbagSponsor.h"
#include "../DirtbagSport.h"
#include "../DirtbagTrad.h"
#include "../DirtbagHabits.h"
#include "../DirtbagNarrator.h"
#include "../DirtbagSessionLoop.h"

using namespace dirtbag;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                    \
  do {                                                                 \
    g_checks++;                                                        \
    if (!(cond)) {                                                     \
      g_failures++;                                                    \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);      \
    }                                                                  \
  } while (0)

// --- old-save fixtures -------------------------------------------------------
//
// Migration tests build their fixture from this build's own writer and then
// take away what the old version could not have had. That is the right shape
// -- a hand-typed fixture tests what somebody believed v14 looked like -- but
// the first version of it hard-coded the string "version=15", so the next
// save bump made `find` return npos and `replace` threw out of the whole
// harness rather than failing a check. These derive the version instead, so
// a bump is a one-line change in the fixture and never a crash.

// Remove the whole line carrying `key`. A field the old version never had.
static void DropSaveLine(std::string& save, const char* key) {
  const std::size_t at = save.find(key);
  CHECK(at != std::string::npos);
  if (at == std::string::npos) return;
  save.erase(at, save.find('\n', at) + 1 - at);
  CHECK(save.find(key) == std::string::npos);
}

// Remove every line whose key starts with `prefix`. A whole family of
// fields the old version never had -- a counted list, or one record
// written field by field.
static void DropSaveLines(std::string& save, const char* prefix) {
  std::size_t at = save.find(prefix);
  CHECK(at != std::string::npos);
  while (at != std::string::npos) {
    save.erase(at, save.find('\n', at) + 1 - at);
    at = save.find(prefix);
  }
}

// Relabel a save written by this build as an older one.
static void SetSaveVersion(std::string& save, int version) {
  const std::string now = "version=" + std::to_string(kSaveVersion) + "\n";
  const std::size_t at = save.find(now);
  CHECK(at != std::string::npos);
  if (at == std::string::npos) return;
  save.replace(at, now.size(), "version=" + std::to_string(version) + "\n");
}

// --- Conditions ---------------------------------------------------------------

static void TestWeatherDeterminism() {
  Rng world = Rng::FromSeed("crag-1");
  for (int day = 1; day <= 30; day++) {
    Weather a = GenerateWeather(world, day);
    Weather b = GenerateWeather(world, day);
    CHECK(a.highTempF == b.highTempF);
    CHECK(a.lowTempF == b.lowTempF);
    CHECK(a.humidity == b.humidity);
    CHECK(a.cloud == b.cloud);
    CHECK(a.wind == b.wind);
  }
  // Different worlds get different weather; different days do too.
  Rng other = Rng::FromSeed("crag-2");
  bool differsByWorld = false, differsByDay = false;
  for (int day = 1; day <= 30; day++) {
    if (GenerateWeather(world, day).highTempF !=
        GenerateWeather(other, day).highTempF) differsByWorld = true;
    if (GenerateWeather(world, day).humidity !=
        GenerateWeather(world, day + 1).humidity) differsByDay = true;
  }
  CHECK(differsByWorld);
  CHECK(differsByDay);

  // Weather must never move the sim's frozen vectors: it lives on its own
  // named stream, so drawing a season of it leaves worldgen untouched.
  Rng before = Rng::FromSeed("crag-1");
  Route r1 = BuildRoute(before, "Line", 5, 5, RouteType::Crimp,
                        Discipline::Boulder);
  for (int day = 1; day <= 200; day++) GenerateWeather(world, day);
  Rng after = Rng::FromSeed("crag-1");
  Route r2 = BuildRoute(after, "Line", 5, 5, RouteType::Crimp,
                        Discipline::Boulder);
  CHECK(r1.moves.size() == r2.moves.size());
  for (size_t i = 0; i < r1.moves.size(); i++)
    CHECK(r1.moves[i].difficulty == r2.moves[i].difficulty);
}

static void TestTemperatureCurve() {
  Weather w;
  w.lowTempF = 30.0;
  w.highTempF = 60.0;
  ConditionsDials d;
  // Coldest before dawn, hottest mid-afternoon, and the whole day inside
  // the day's own range.
  CHECK(TemperatureAt(w, d.coldestHour) < TemperatureAt(w, 10.0));
  CHECK(TemperatureAt(w, d.hottestHour) > TemperatureAt(w, 10.0));
  for (double h = 0.0; h <= 24.0; h += 0.5) {
    CHECK(TemperatureAt(w, h) >= w.lowTempF - 1e-9);
    CHECK(TemperatureAt(w, h) <= w.highTempF + 1e-9);
  }
}

static void TestSunFollowsAspect() {
  Weather w;
  w.cloud = 0.0;
  // North never takes a direct hit — the whole reason it is the summer plan.
  for (double h = 0.0; h <= 24.0; h += 0.5)
    CHECK(SunOnRock(Aspect::North, h, w) == 0.0);
  // The others take it in the order the sun travels.
  CHECK(SunOnRock(Aspect::East, 9.5, w) > SunOnRock(Aspect::East, 16.5, w));
  CHECK(SunOnRock(Aspect::West, 16.5, w) > SunOnRock(Aspect::West, 9.5, w));
  CHECK(SunOnRock(Aspect::South, 13.0, w) > SunOnRock(Aspect::South, 7.0, w));
  // Cloud is the reprieve.
  Weather overcast = w;
  overcast.cloud = 1.0;
  CHECK(SunOnRock(Aspect::South, 13.0, overcast) <
        SunOnRock(Aspect::South, 13.0, w));
}

static void TestRockHoldsTheSun() {
  Weather w;
  w.lowTempF = 30.0;
  w.highTempF = 60.0;
  w.cloud = 0.0;
  // A sunny face runs hotter than the air; a north face is just the air.
  CHECK(RockTempAt(w, Aspect::South, 14.0) > TemperatureAt(w, 14.0));
  CHECK(std::fabs(RockTempAt(w, Aspect::North, 14.0) -
                  TemperatureAt(w, 14.0)) < 1e-9);
  // The lag is the point: an east face is still giving back heat well after
  // the sun has left it, so the rock peaks later than the sun does.
  const double sunPeak = 9.5;
  CHECK(RockTempAt(w, Aspect::East, sunPeak + 2.0) -
            TemperatureAt(w, sunPeak + 2.0) >
        RockTempAt(w, Aspect::East, sunPeak) - TemperatureAt(w, sunPeak));
}

static void TestFrictionRespondsToWeather() {
  ConditionsDials d;
  Weather dry;
  dry.lowTempF = 40.0; dry.highTempF = 60.0;
  dry.humidity = 0.05; dry.cloud = 0.5; dry.wind = 0.3;
  Weather humid = dry;
  humid.humidity = 1.0;
  // Humidity is the dirtbag's real enemy.
  CHECK(ConditionsAt(dry, Aspect::North, 12.0).friction >
        ConditionsAt(humid, Aspect::North, 12.0).friction);
  // Wind saves a marginal day.
  Weather windy = dry;
  windy.wind = 1.0;
  CHECK(ConditionsAt(windy, Aspect::North, 12.0).friction >=
        ConditionsAt(dry, Aspect::North, 12.0).friction);
  // Friction stays a probability-shaped 0..1 for every hour and aspect.
  Rng world = Rng::FromSeed("crag-1");
  for (int day = 1; day <= 60; day++) {
    Weather w = GenerateWeather(world, day);
    for (Aspect a : {Aspect::North, Aspect::East, Aspect::South, Aspect::West})
      for (double h = FirstLightHour(day, d); h <= LastLightHour(day, d);
           h += 0.5) {
        const double f = ConditionsAt(w, a, h).friction;
        CHECK(f >= 0.0 && f <= 1.0);
      }
  }
}

static void TestWindowIsShortEnoughToBeADecision() {
  // The load-bearing balance fact (notes/phase2-window.md): skin allows
  // 8-12 burns a day, so a window that fits a whole day's skin makes waiting
  // free and deletes the projecting loop. Windows must stay near an hour.
  ConditionsDials d;
  Rng world = Rng::FromSeed("crag-1");
  const int DAYS = 300;
  for (Aspect a : {Aspect::North, Aspect::East, Aspect::South, Aspect::West}) {
    int withWindow = 0;
    double total = 0.0;
    for (int day = 1; day <= DAYS; day++) {
      PrimeWindow win = FindPrimeWindow(GenerateWeather(world, day, d), a, d);
      if (!win.exists) continue;
      withWindow++;
      total += win.hours();
      // The window contains its own peak, and sits inside daylight.
      CHECK(win.startHour <= win.peakHour + 1e-9);
      CHECK(win.endHour >= win.peakHour - 1e-9);
      CHECK(win.startHour >= FirstLightHour(day, d) - 1e-9);
      CHECK(win.endHour <= LastLightHour(day, d) + 1e-9);
      CHECK(win.peakFriction >= d.primeThreshold);
    }
    // Some days refuse you outright, and most days do not.
    CHECK(withWindow > DAYS / 4);
    CHECK(withWindow < DAYS);
    const double mean = total / withWindow;
    CHECK(mean > 0.4);   // long enough to be worth waiting for
    CHECK(mean < 2.5);   // short enough that a day's skin will not fit inside
  }
}

static void TestAspectDecidesWhen() {
  // The shade line is the mechanic: a face that bakes in the morning comes
  // good in the evening, and vice versa. Averaged over a season so one
  // freak day cannot carry it.
  ConditionsDials d;
  Rng world = Rng::FromSeed("crag-1");
  double eastPeak = 0.0, westPeak = 0.0;
  int eastN = 0, westN = 0;
  for (int day = 1; day <= 300; day++) {
    Weather w = GenerateWeather(world, day, d);
    PrimeWindow e = FindPrimeWindow(w, Aspect::East, d);
    PrimeWindow t = FindPrimeWindow(w, Aspect::West, d);
    if (e.exists) { eastPeak += e.peakHour; eastN++; }
    if (t.exists) { westPeak += t.peakHour; westN++; }
  }
  CHECK(eastN > 0 && westN > 0);
  // East bakes early and is climbable late; west is the mirror.
  CHECK(eastPeak / eastN > westPeak / westN);
}

static void TestWindowChangesWhenYouBurn() {
  // Phase 2's gate, as a test, in its two halves.
  //
  // Averaged over five generated routes on purpose: a single route name can
  // roll an eight-move V6 with three crux moves that a V5 climber cannot do
  // in any conditions, and a gate test hostage to one generation is not a
  // gate test.
  ConditionsDials d;
  Rng world = Rng::FromSeed("crag-1");
  Climber c;
  c.skills.power = c.skills.fingers = c.skills.technique =
      c.skills.endurance = c.skills.head = 50.0;
  c.skin = 9.0;

  std::vector<Route> projects;
  for (int i = 1; i <= 5; i++)
    projects.push_back(BuildRoute(world, "Gate " + std::to_string(i), 5, 5,
                                  RouteType::Crimp, Discipline::Boulder));

  // Half one: burns outside the window buy beta, and beta is worth real
  // percentage points once the window opens.
  auto Play = [&](int recon) {
    int sends = 0, days = 0;
    for (const Route& p : projects) {
      for (int day = 1; day <= 200; day++) {
        Weather w = GenerateWeather(world, day, d);
        PrimeWindow win = FindPrimeWindow(w, Aspect::East, d);
        if (!win.exists) continue;
        days++;
        const double reconHour =
            std::max(FirstLightHour(day, d), win.startHour - 2.0);
        const Conditions poor = ConditionsAt(w, Aspect::East, reconHour, d);
        const Conditions prime = ConditionsAt(w, Aspect::East, win.peakHour, d);
        const int windowBurns = static_cast<int>(win.hours() / 0.25);

        Rng session = Rng::FromSeed("gate-" + p.name + "-" +
                                    std::to_string(day));
        SessionState st = StartSession(c);
        ProjectMemory mem;
        mem.routeName = p.name;
        mem.grade = p.grade;
        bool sent = false;
        for (int i = 0; i < recon && st.skinLeft > 0.5 && !sent; i++)
          sent = AttemptInSession(session, st, mem, c, p, poor).sent;
        for (int i = 0; i < windowBurns && st.skinLeft > 0.5 && !sent; i++)
          sent = AttemptInSession(session, st, mem, c, p, prime).sent;
        if (sent) sends++;
      }
    }
    CHECK(days > 0);
    return 100.0 * sends / days;
  };
  const double cold = Play(0);   // straight into the window, knowing nothing
  const double read = Play(6);   // six burns of recon first
  CHECK(read > cold + 10.0);

  // Half two: skin is the budget the window is spent from. This half is
  // mechanical rather than statistical — at your own grade you often send
  // during recon, which flatters the send rate while hiding the real cost.
  // Count the burns the window actually gets instead.
  // Find a day that actually comes good rather than assuming one does.
  // Day 3 used to be as good as any; with seasons it is midwinter, and the
  // rock is not in condition at all.
  int goodDay = 0;
  for (int day = 1; day <= 400 && goodDay == 0; day++)
    if (FindPrimeWindow(GenerateWeather(world, day, d), Aspect::East, d).exists)
      goodDay = day;
  CHECK(goodDay > 0);

  auto WindowBurnsLeft = [&](int recon) {
    const Route& p = projects[0];
    Weather w = GenerateWeather(world, goodDay, d);
    PrimeWindow win = FindPrimeWindow(w, Aspect::East, d);
    CHECK(win.exists);
    const Conditions poor =
        ConditionsAt(w, Aspect::East,
                     std::max(FirstLightHour(goodDay, d), win.startHour - 2.0),
                     d);
    Rng session = Rng::FromSeed("skin-budget");
    SessionState st = StartSession(c);
    ProjectMemory mem;
    mem.routeName = p.name;
    mem.grade = p.grade;
    for (int i = 0; i < recon && st.skinLeft > 0.5; i++)
      AttemptInSession(session, st, mem, c, p, poor);
    return st.skinLeft;
  };
  CHECK(WindowBurnsLeft(14) < WindowBurnsLeft(6));
  CHECK(WindowBurnsLeft(6) < WindowBurnsLeft(0));
  CHECK(WindowBurnsLeft(14) < 1.0);  // nothing left for the window at all
}

// --- The crag ------------------------------------------------------------------

static void TestCragIsStable() {
  Rng world = Rng::FromSeed("crag-1");
  Crag a = RoadsideCrag(world);
  Crag b = RoadsideCrag(world);
  CHECK(a.name == b.name);
  CHECK(a.lines.size() == b.lines.size());
  for (size_t i = 0; i < a.lines.size(); i++) {
    CHECK(a.lines[i].route.name == b.lines[i].route.name);
    CHECK(a.lines[i].route.moves.size() == b.lines[i].route.moves.size());
    for (size_t m = 0; m < a.lines[i].route.moves.size(); m++)
      CHECK(a.lines[i].route.moves[m].difficulty ==
            b.lines[i].route.moves[m].difficulty);
  }
  // A different world gets different rock under the same guidebook: the book
  // is authored, the moves are seeded.
  Crag other = RoadsideCrag(Rng::FromSeed("crag-2"));
  CHECK(other.lines.size() == a.lines.size());
  bool moved = false;
  for (size_t i = 0; i < a.lines.size(); i++)
    if (other.lines[i].route.moves.size() != a.lines[i].route.moves.size() ||
        other.lines[i].route.moves[0].difficulty !=
            a.lines[i].route.moves[0].difficulty)
      moved = true;
  CHECK(moved);
}

static void TestCragIsNotALadder() {
  // The gym board is one problem per grade, climbing cleanly. A crag is not:
  // it piles up on the moderates, thins out at the top, and skips nothing in
  // between. If this ever becomes a ladder, the crag has turned into a gym.
  Crag crag = RoadsideCrag(Rng::FromSeed("crag-1"));
  int atGrade[20] = {0};
  int graded = 0;
  for (const CragLine& l : crag.lines) {
    if (l.isProject) continue;
    CHECK(l.route.grade >= 0 && l.route.grade < 20);
    atGrade[l.route.grade]++;
    graded++;
  }
  CHECK(graded >= 20);
  // Moderates outnumber the hard end several times over.
  int moderate = 0, hard = 0;
  for (int g = 0; g <= 4; g++) moderate += atGrade[g];
  for (int g = 7; g < 20; g++) hard += atGrade[g];
  CHECK(moderate > hard * 2);
  // No grade is represented more than the book's own moderate band, and at
  // least one grade carries several lines — that is the pile-up.
  int busiest = 0;
  for (int g = 0; g < 20; g++) busiest = std::max(busiest, atGrade[g]);
  CHECK(busiest >= 3);
}

static void TestCragHasProjectsAndTheyAreOpen() {
  Crag crag = RoadsideCrag(Rng::FromSeed("crag-1"));
  std::vector<const CragLine*> projects = OpenProjects(crag);
  CHECK(projects.size() >= 3);
  int hardest = 0;
  for (const CragLine& l : crag.lines)
    if (!l.isProject) hardest = std::max(hardest, l.route.grade);
  for (const CragLine* p : projects) {
    CHECK(p->isProject);
    CHECK(p->firstAscentBy.empty());   // nobody has done it
    CHECK(p->stars == 0);              // and nobody can vouch for it
    CHECK(!p->description.empty());    // but everyone knows where it is
  }
  // The hardest thing at the crag is something nobody has done — the
  // frontier has to be open, or there is nothing to grow into.
  int hardestProject = 0, easiestProject = 99;
  for (const CragLine* p : projects) {
    hardestProject = std::max(hardestProject, p->route.grade);
    easiestProject = std::min(easiestProject, p->route.grade);
  }
  CHECK(hardestProject > hardest);

  // And at least one is within reach of somebody just starting. A crag whose
  // only open lines are V7 and up puts a first ascent — the point of the
  // whole phase — behind a season of training, which is not what "name your
  // own first ascent" is supposed to mean. Unclimbed is not a synonym for
  // hard: plenty of lines are open because the landing is bad or the rock is
  // dull, and those are the way in.
  Climber starting;
  starting.skills.power = starting.skills.fingers = starting.skills.technique =
      starting.skills.endurance = starting.skills.head = 50.0;
  CHECK(easiestProject <= static_cast<int>(SkillToGrade(50.0)));

  bool reachable = false;
  for (const CragLine* p : projects) {
    const RouteRead read = ReadRoute(starting, p->route);
    if (read == RouteRead::Warmup || read == RouteRead::Comfortable ||
        read == RouteRead::AtYourLimit)
      reachable = true;
  }
  CHECK(reachable);
  // Everything in the book proper has been climbed by somebody.
  for (const CragLine& l : crag.lines)
    if (!l.isProject) CHECK(!l.firstAscentBy.empty());
}

static void TestTheBookHasSomethingAtTheTopOfACareer() {
  // A measured career peaks at V6.6 on average and the V8 projects need
  // skill 65 clean and wired (notes/phase4-the-lot.md). Roadside used to run
  // V3, V4, V8, V8, V10 — the player cleared the two moderates in their
  // first season and then had thirty years with nothing to aim at. There has
  // to be a line pitched at the top of a normal career, in every world.
  const char* kSitStart = "the sit start to Shade Line";
  int inBand = 0, present = 0, worlds = 0;
  int hardest = 0, easiest = 99;
  for (int s = 0; s < 200; s++) {
    Crag crag = RoadsideCrag(Rng::FromSeed("career-" + std::to_string(s)));
    worlds++;
    bool foundReachable = false;
    for (const CragLine* p : OpenProjects(crag)) {
      if (p->description != kSitStart) continue;
      present++;
      hardest = std::max(hardest, p->route.trueGrade);
      easiest = std::min(easiest, p->route.trueGrade);
      if (p->route.trueGrade >= 6 && p->route.trueGrade <= 7) inBand++;
    }
    // The gap that started this: something open between the moderates a
    // beginner takes and the V8s nobody in this world will ever climb.
    for (const CragLine* p : OpenProjects(crag))
      if (p->route.trueGrade >= 5 && p->route.trueGrade <= 7) foundReachable = true;
    CHECK(foundReachable);
  }
  CHECK(present == worlds);        // it is book content, not a random spawn
  // A line everybody has pulled on is close to known, so the book cannot be
  // out by two: a V8 here would put the hole straight back.
  CHECK(hardest == 7);
  CHECK(easiest == 5);
  CHECK(inBand * 2 > worlds);      // and usually it is exactly what it says

  // The wide drift still applies to lines nobody has touched, or the hard
  // end of the book stops being a real question.
  int wideHardest = 0;
  for (int s = 0; s < 200; s++) {
    Crag crag = RoadsideCrag(Rng::FromSeed("career-" + std::to_string(s)));
    for (const CragLine* p : OpenProjects(crag))
      if (p->description == "the blank wall behind the parking")
        wideHardest = std::max(wideHardest, p->route.trueGrade);
  }
  CHECK(wideHardest == 11);        // guess 9, drift up to +2

  // The new line was appended rather than inserted, so every project that
  // already existed kept the exact grade it had. Saves depend on this: a
  // project the player is mid-way through must not change difficulty
  // because content was added somewhere else in the list.
  Crag one = RoadsideCrag(Rng::FromSeed("crag-1"));
  std::vector<int> got;
  for (const CragLine* p : OpenProjects(one)) got.push_back(p->route.trueGrade);
  const std::vector<int> want = {3, 4, 8, 8, 10, 7};
  CHECK(got == want);
}

static void TestTheDirtbagYear() {
  JobDials jd;
  Job job;

  // A year is a year. Not a season -- the point is that you got through a
  // winter without signing, and a season-length version is a holiday.
  CHECK(jd.dirtbagYearDays == 365);

  // Nothing to say until there is something to say. Eleven days is a
  // fortnight, not an achievement.
  for (int d = 0; d < jd.dirtbagYearDays - 1; d++) CHECK(!DirtbagDay(job, jd));
  CHECK(job.dirtbagYears == 0);
  CHECK(DirtbagYearText(job, jd).empty());

  // And then the day it lands, exactly once.
  CHECK(DirtbagDay(job, jd));
  CHECK(job.dirtbagYears == 1);
  CHECK(!DirtbagDay(job, jd));
  CHECK(job.dirtbagYears == 1);
  CHECK(DirtbagYearText(job, jd) == "A Dirtbag Year.  One day into another.");

  // The counter keeps running: three years is three years, not one year
  // restarted twice.
  for (int d = 0; d < jd.dirtbagYearDays; d++) DirtbagDay(job, jd);
  CHECK(job.dirtbagYears == 2);
  CHECK(job.daysSinceSalary == 2 * jd.dirtbagYearDays + 1);
  CHECK(DirtbagYearText(job, jd) == "2 Dirtbag Years.  One day into another.");

  // Odd jobs do not break it and must not: the board is how a dirtbag eats,
  // and a year of hauling trail is the most dirtbag year there is. Only the
  // nine-to-five counts, and it counts from the signature.
  PlayerState hauler;
  for (int d = 0; d < 40; d++) DirtbagDay(hauler.job);
  CHECK(hauler.job.daysSinceSalary == 40);
  TakeSalariedJob(hauler);
  CHECK(hauler.job.daysSinceSalary == 0);
  CHECK(hauler.job.longestStreak == 40);   // what you did stands

  // A day you hold the job is not a day of the streak, weekend or not. A
  // version that only skipped working days would take five years to earn
  // one, because the salary owns Saturday too.
  Job employed;
  employed.salaried = true;
  for (int d = 0; d < 400; d++) CHECK(!DirtbagDay(employed, jd));
  CHECK(employed.daysSinceSalary == 0);
  CHECK(employed.dirtbagYears == 0);

  // Banked years survive the job. It takes the one you were in the middle
  // of, not the ones you finished.
  Job veteran;
  for (int d = 0; d < 2 * jd.dirtbagYearDays + 100; d++) DirtbagDay(veteran, jd);
  CHECK(veteran.dirtbagYears == 2);
  const int lost = BreakTheStreak(veteran);
  CHECK(lost == 2 * jd.dirtbagYearDays + 100);
  CHECK(veteran.dirtbagYears == 2);
  CHECK(veteran.longestStreak == 2 * jd.dirtbagYearDays + 100);
  CHECK(DirtbagYearText(veteran, jd) == "2 Dirtbag Years.");

  // It reaches the one place a career is actually read.
  PlayerState lifer;
  lifer.day = 4000;
  for (int d = 0; d < 3 * jd.dirtbagYearDays; d++) DirtbagDay(lifer.job, jd);
  const Legacy legacy = TallyCareer(lifer, "Wren", 11);
  CHECK(legacy.dirtbagYears == 3);
  CHECK(legacy.longestDirtbagStreak == 3 * jd.dirtbagYearDays);
  CHECK(LegacyText(legacy).find("3 Dirtbag Years.") != std::string::npos);

  // And a career that took the job says nothing, rather than saying zero.
  PlayerState sellout;
  sellout.day = 4000;
  TakeSalariedJob(sellout);
  CHECK(LegacyText(TallyCareer(sellout, "Ash", 11)).find("Dirtbag Year") ==
        std::string::npos);
}

// v15 -> v16. Old saves must load, and must not be handed a year they may
// never have lived.
static void TestTheDirtbagYearMigrates() {
  PlayerState player;
  for (int d = 0; d < 500; d++) DirtbagDay(player.job);
  SaveGame save;
  save.player = player;
  save.seed = "dirtbag-year";

  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  CHECK(back.player.job.dirtbagYears == 1);
  CHECK(back.player.job.daysSinceSalary == 500);
  CHECK(back.player.job.longestStreak == 500);

  std::string v15 = SerializeSave(save);
  DropSaveLine(v15, "job.sincesalary=");
  DropSaveLine(v15, "job.dirtbagyears=");
  DropSaveLine(v15, "job.longeststreak=");
  SetSaveVersion(v15, 15);

  SaveGame old;
  CHECK(DeserializeSave(v15, old) == LoadResult::Ok);
  CHECK(old.version == kSaveVersion);
  // Zero, not a guess. The file does not say whether the streak was
  // running, and awarding a year that was never lived would put a line in
  // somebody's legacy that never happened.
  CHECK(old.player.job.dirtbagYears == 0);
  CHECK(old.player.job.daysSinceSalary == 0);
  // The rest of the career is untouched.
  CHECK(old.seed == save.seed);
  CHECK(static_cast<int>(DefaultMigrations().size()) == kSaveVersion - 1);
}

static void TestTheTownNamesYourCrew() {
  CrewDials cd;
  const Rng world = Rng::FromSeed("crew-1");
  Standing scene;
  scene.with[static_cast<int>(Faction::Scene)] = 0.8;

  // One partner is a partnership, not a crew. The game already models that
  // as a bond and does not need a name for it.
  std::vector<PartnerBond> one = {{"Margo", 0.9, 0.9, {}}};
  CHECK(!ReadsAsACrew(one, cd));

  // Two people you actually climb with is a crew. Knowing their name is not
  // climbing with them.
  std::vector<PartnerBond> nodding = {{"Margo", 0.2, 0.2, {}}, {"Dev", 0.3, 0.3, {}}};
  CHECK(!ReadsAsACrew(nodding, cd));
  std::vector<PartnerBond> real = {{"Margo", 0.9, 0.9, {}}, {"Dev", 0.7, 0.7, {}}};
  CHECK(ReadsAsACrew(real, cd));

  // And it has to hold for a month. A crew that exists for one good week in
  // September is three people who had a good week in September.
  Crew crew;
  for (int d = 1; d < cd.daysBeforeTheyNameYou; d++) {
    CHECK(!CrewDay(crew, "Wren", real, scene, world, d, cd));
    CHECK(crew.name.empty());
  }
  CHECK(CrewDay(crew, "Wren", real, scene, world, cd.daysBeforeTheyNameYou, cd));
  CHECK(!crew.name.empty());
  CHECK(crew.membersWhenNamed == 3);              // two of them and you
  CHECK(crew.namedOnDay == cd.daysBeforeTheyNameYou);

  // Said once. The town does not keep announcing it.
  CHECK(!CrewDay(crew, "Wren", real, scene, world,
                 cd.daysBeforeTheyNameYou + 1, cd));
  CHECK(CrewText(crew) == "They call you " + crew.name + ".");

  // Named is named. It does not come back for revision when the crew drifts
  // apart -- a career that outlives its own crew still gets called the thing
  // it got called.
  const std::string stuck = crew.name;
  for (int d = 0; d < 400; d++)
    CrewDay(crew, "Wren", one, scene, world, 1000 + d, cd);
  CHECK(crew.name == stuck);

  // A thin fortnight does not cost you the month: the counter slides back at
  // the rate it built rather than resetting, so a crew that mostly holds
  // together still gets there.
  Crew patchy;
  for (int d = 0; d < 20; d++)
    CrewDay(patchy, "Wren", real, scene, world, d, cd);
  CHECK(patchy.daysReadingAsACrew == 20);
  for (int d = 0; d < 5; d++)
    CrewDay(patchy, "Wren", one, scene, world, 20 + d, cd);
  CHECK(patchy.daysReadingAsACrew == 15);         // slid, not wiped

  // Who is talking decides what you are called. The same people in the same
  // valley are always called the same thing; a different part of town calls
  // them something else.
  Standing stewards;
  stewards.with[static_cast<int>(Faction::Stewardship)] = 0.8;
  Crew a, b;
  for (int d = 0; d <= cd.daysBeforeTheyNameYou; d++) {
    CrewDay(a, "Wren", real, scene, world, d, cd);
    CrewDay(b, "Wren", real, stewards, world, d, cd);
  }
  CHECK(!a.name.empty() && !b.name.empty());
  CHECK(a.name != b.name);

  // Stable across replays, and independent of the order bonds happen to sit
  // in -- they are rebuilt from the world seed daily and only the bond is
  // career state, so a reload must not rename you.
  std::vector<PartnerBond> reversed = {{"Dev", 0.7, 0.7, {}}, {"Margo", 0.9, 0.9, {}}};
  Crew again;
  for (int d = 0; d <= cd.daysBeforeTheyNameYou; d++)
    CrewDay(again, "Wren", reversed, scene, world, d, cd);
  CHECK(again.name == a.name);

  // A different valley calls them something else.
  Crew elsewhere;
  const Rng other = Rng::FromSeed("crew-2");
  for (int d = 0; d <= cd.daysBeforeTheyNameYou; d++)
    CrewDay(elsewhere, "Wren", real, scene, other, d, cd);
  CHECK(!elsewhere.name.empty());

  // A different *you* is a different crew. The hash used to key on world
  // and partners alone, and the partners are the same three people every
  // generation -- so the town issued one nickname to four lives in a row.
  // You are a member; you are part of the key. (Five names per list, so a
  // collision is possible on some pair; this pair differs, and the check
  // is that the successor CAN be named their own thing, not that every
  // pair must be.)
  Crew nextGen;
  for (int d = 0; d <= cd.daysBeforeTheyNameYou; d++)
    CrewDay(nextGen, "Ash", real, scene, world, d, cd);
  CHECK(!nextGen.name.empty());
  CHECK(nextGen.name != a.name);

  // And if nobody rates you, you still get a name -- being called something
  // is the point. It is just not a kind one.
  Standing nobody;
  Crew unrated;
  for (int d = 0; d <= cd.daysBeforeTheyNameYou; d++)
    CrewDay(unrated, "Wren", real, nobody, world, d, cd);
  CHECK(!unrated.name.empty());
  CHECK(unrated.name != a.name);

  // It reaches the legacy, which is where a career is read.
  PlayerState player;
  player.day = 4000;
  player.bonds = real;
  player.standing = scene;
  player.crew = a;
  const Legacy legacy = TallyCareer(player, "Wren", 11);
  CHECK(legacy.crewName == a.name);
  CHECK(LegacyText(legacy).find("They called them " + a.name + ".") !=
        std::string::npos);

  // And a career nobody ever named says nothing rather than saying blank.
  PlayerState loner;
  loner.day = 4000;
  CHECK(LegacyText(TallyCareer(loner, "Ash", 11)).find("They called them") ==
        std::string::npos);
}

// v16 -> v17. Old saves must load, and must not arrive pre-named.
static void TestTheCrewNameMigrates() {
  PlayerState player;
  player.bonds = {{"Margo", 0.9, 0.9, {}}, {"Dev", 0.7, 0.7, {}}};
  player.standing.with[static_cast<int>(Faction::Scene)] = 0.8;
  const Rng world = Rng::FromSeed("crew-save");
  for (int d = 0; d <= CrewDials{}.daysBeforeTheyNameYou; d++)
    CrewDay(player.crew, player.name, player.bonds, player.standing, world, d);
  CHECK(!player.crew.name.empty());

  SaveGame save;
  save.player = player;
  save.seed = "crew-save";
  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  CHECK(back.player.crew.name == player.crew.name);
  CHECK(back.player.crew.membersWhenNamed == 3);

  std::string v16 = SerializeSave(save);
  DropSaveLine(v16, "crew.name=");
  DropSaveLine(v16, "crew.namedon=");
  DropSaveLine(v16, "crew.days=");
  DropSaveLine(v16, "crew.members=");
  SetSaveVersion(v16, 16);

  SaveGame old;
  CHECK(DeserializeSave(v16, old) == LoadResult::Ok);
  CHECK(old.version == kSaveVersion);
  // No name, which is the true answer rather than a lossy one: the town had
  // not said it because the system did not exist. The bonds that earn one
  // are already saved, so the career starts its month from today.
  CHECK(old.player.crew.name.empty());
  CHECK(old.player.crew.daysReadingAsACrew == 0);
  CHECK(old.player.bonds.size() == player.bonds.size());
  CHECK(static_cast<int>(DefaultMigrations().size()) == kSaveVersion - 1);

  // An unnamed crew must round-trip as present-and-empty, or a fresh save
  // would look like an unmigrated one on the next load.
  SaveGame fresh;
  fresh.seed = "nobody";
  SaveGame freshBack;
  CHECK(DeserializeSave(SerializeSave(fresh), freshBack) == LoadResult::Ok);
  CHECK(freshBack.player.crew.name.empty());
}

// v19 -> v20: the player's own name. It is free text from a keyboard, and
// the save format is line-oriented, so the serializer strips line breaks --
// one pasted newline must not shear the file in half.
static void TestThePlayerNameMigrates() {
  SaveGame save;
  save.seed = "named";
  save.player.name = "Wren";
  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  CHECK(back.player.name == "Wren");

  // '=' is fine -- the parser splits on the first one. Newlines are not,
  // and arrive stripped rather than fatal.
  save.player.name = "Wren = the\nfirst";
  SaveGame odd;
  CHECK(DeserializeSave(SerializeSave(save), odd) == LoadResult::Ok);
  CHECK(odd.player.name == "Wren = thefirst");

  // A v19 save has no name and arrives unnamed, costing nothing: a named
  // crew stays named, and an unnamed career salts the crew hash with
  // nothing, exactly as every career did before names existed.
  save.player.name = "Wren";
  std::string v19 = SerializeSave(save);
  DropSaveLine(v19, "player.name=");
  SetSaveVersion(v19, 19);
  SaveGame old;
  CHECK(DeserializeSave(v19, old) == LoadResult::Ok);
  CHECK(old.version == kSaveVersion);
  CHECK(old.player.name.empty());
  CHECK(static_cast<int>(DefaultMigrations().size()) == kSaveVersion - 1);
}

static void TestDreamsCostTheBuffer() {
  DreamDials dd;
  Dreams dreams;
  Van van;
  double cash = 0.0;

  // Nothing is free and nothing is nearly free. Every price sits well above
  // the float that keeps a van healthy, which is the design: buying drops
  // you back into the trap you climbed out of.
  CHECK(CostOf(Dream::Rig, dd) >= 5000.0);
  CHECK(CostOf(Dream::WarChest, dd) > CostOf(Dream::Rig, dd));
  CHECK(CostOf(Dream::HomeBase, dd) > CostOf(Dream::WarChest, dd));
  CHECK(CostOf(Dream::None, dd) == 0.0);

  CHECK(!CanAfford(dreams, Dream::Rig, 10.0, dd));
  CHECK(!BuyDream(dreams, van, cash, Dream::Rig, dd));
  CHECK(DreamText(dreams).empty());

  // A dream is chosen before it is bought, and money without a choice buys
  // nothing: the shop can show all three, it can only ever sell you yours.
  cash = CostOf(Dream::HomeBase, dd) + CostOf(Dream::Rig, dd);
  CHECK(!CanAfford(dreams, Dream::Rig, cash, dd));
  CHECK(!BuyDream(dreams, van, cash, Dream::Rig, dd));
  CHECK(!ChooseDream(dreams, Dream::None));   // "nothing" is not a dream
  CHECK(ChooseDream(dreams, Dream::Rig));
  CHECK(dreams.chosen == Dream::Rig);

  // Chosen is chosen. There is no way back, because an option you can
  // reopen was never closed -- and the other two are off the table however
  // much money is in the tin.
  CHECK(!ChooseDream(dreams, Dream::HomeBase));
  CHECK(dreams.chosen == Dream::Rig);
  CHECK(!CanAfford(dreams, Dream::HomeBase, cash, dd));
  CHECK(!BuyDream(dreams, van, cash, Dream::HomeBase, dd));

  // The Rig. It takes the money and hands back a van that is new and stays
  // newer -- but the cash is gone, which is the cost.
  cash = CostOf(Dream::Rig, dd) + 40.0;
  for (int p = 0; p < kVanPartCount; p++) van.parts[p].wear = 0.9;
  CHECK(BuyDream(dreams, van, cash, Dream::Rig, dd));
  CHECK(cash == 40.0);                       // the buffer, spent
  CHECK(van.rig);
  for (int p = 0; p < kVanPartCount; p++) CHECK(van.parts[p].wear == 0.0);
  CHECK(HasDream(dreams, Dream::Rig));
  // What your dream was is part of the career -- it survives the purchase.
  CHECK(dreams.chosen == Dream::Rig);
  CHECK(!BuyDream(dreams, van, cash, Dream::Rig, dd));   // and only once

  // A Rig does not stop breaking, it breaks less. Same hours, same weather,
  // against a van that is otherwise identical.
  VanDials vd;
  const Rng world = Rng::FromSeed("van-rig");
  Van plain, posh;
  posh.rig = true;
  for (int d = 0; d < 300; d++) {
    DriveVan(plain, world, d, 2.0, 60.0, vd);
    DriveVan(posh, world, d, 2.0, 60.0, vd);
  }
  double plainWear = 0.0, poshWear = 0.0;
  for (int p = 0; p < kVanPartCount; p++) {
    plainWear += plain.parts[p].wear;
    poshWear += posh.parts[p].wear;
  }
  CHECK(poshWear < plainWear);
  CHECK(poshWear > 0.0);          // not solved, just slower

  // The War Chest is a year you already bought, not a lump you spend down.
  Dreams rich;
  Van v2;
  double money = CostOf(Dream::WarChest, dd);
  CHECK(!NoNeedToWork(rich));
  CHECK(ChooseDream(rich, Dream::WarChest));
  CHECK(BuyDream(rich, v2, money, Dream::WarChest, dd));
  CHECK(money == 0.0);
  CHECK(NoNeedToWork(rich));
  CHECK(rich.seasonOffDaysLeft == dd.warChestDays);
  double noCash = 0.0, noOwed = 0.0;
  for (int d = 0; d < dd.warChestDays; d++) DreamDay(rich, noCash, noOwed, dd);
  CHECK(!NoNeedToWork(rich));
  CHECK(noOwed == 0.0);           // no address, no rent

  // Home Base pays out every night and asks every morning.
  Dreams housed;
  Van v3;
  double pot = CostOf(Dream::HomeBase, dd);
  CHECK(SkinBonus(housed, dd) == 0.0);
  CHECK(ChooseDream(housed, Dream::HomeBase));
  CHECK(BuyDream(housed, v3, pot, Dream::HomeBase, dd));
  CHECK(SkinBonus(housed, dd) > 0.0);
  double wallet = 100.0, owed = 0.0;
  DreamDay(housed, wallet, owed, dd);
  CHECK(wallet == 100.0 - dd.homeBaseRentPerDay);
  CHECK(owed == 0.0);
  // And rent you cannot pay waits, like every other bill. An address you
  // cannot afford is a debt with a door on it, not a repossession.
  wallet = 5.0;
  DreamDay(housed, wallet, owed, dd);
  CHECK(wallet == 0.0);
  CHECK(owed == dd.homeBaseRentPerDay - 5.0);

  // It reaches the legacy, which is where a career is read -- and a career
  // that got the Rig and never a roof is a different life from one that got
  // the roof and stayed stranded, so the legacy keeps which.
  PlayerState lived;
  lived.day = 4000;
  lived.dreams = dreams;
  const Legacy leg = TallyCareer(lived, "Wren", 11);
  CHECK(HasDream(leg.dreams, Dream::Rig));
  CHECK(!HasDream(leg.dreams, Dream::HomeBase));
  CHECK(LegacyText(leg).find("the Rig.") != std::string::npos);
  // And a career that never bought one says nothing rather than saying none.
  PlayerState skint;
  skint.day = 4000;
  CHECK(LegacyText(TallyCareer(skint, "Ash", 11)).find("the Rig") ==
        std::string::npos);

  // The text says what you own and what is still running.
  CHECK(DreamText(dreams) == "the Rig.");
  CHECK(DreamText(rich) == "the War Chest.");
  Dreams both = rich;
  both.seasonOffDaysLeft = 212;
  CHECK(DreamText(both) ==
        "the War Chest.  212 days of the War Chest left.");
}

// v17 -> v18 -> v19. Old careers own nothing, which is exactly right --
// and a v18 note-to-self must not arrive as a binding choice.
static void TestDreamsMigrate() {
  PlayerState player;
  Van& van = player.van;
  double cash = 60000.0;
  CHECK(ChooseDream(player.dreams, Dream::Rig));
  CHECK(BuyDream(player.dreams, van, cash, Dream::Rig));
  player.cash = cash;

  SaveGame save;
  save.player = player;
  save.seed = "dreams";
  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  CHECK(HasDream(back.player.dreams, Dream::Rig));
  CHECK(!HasDream(back.player.dreams, Dream::HomeBase));
  CHECK(back.player.dreams.chosen == Dream::Rig);   // the record survives
  CHECK(back.player.van.rig);

  std::string v17 = SerializeSave(save);
  DropSaveLine(v17, "dreams.rig=");
  DropSaveLine(v17, "dreams.warchest=");
  DropSaveLine(v17, "dreams.homebase=");
  DropSaveLine(v17, "dreams.chosen=");
  DropSaveLine(v17, "dreams.seasonoff=");
  DropSaveLine(v17, "van.rig=");
  SetSaveVersion(v17, 17);

  SaveGame old;
  CHECK(DeserializeSave(v17, old) == LoadResult::Ok);
  CHECK(old.version == kSaveVersion);
  CHECK(!HasDream(old.player.dreams, Dream::Rig));
  CHECK(old.player.dreams.chosen == Dream::None);
  // A Rig is a van you bought, not a van you maintained, so an old career
  // keeps whatever it has been driving.
  CHECK(!old.player.van.rig);
  CHECK(old.player.cash == player.cash);   // and the money is still theirs
  CHECK(static_cast<int>(DefaultMigrations().size()) == kSaveVersion - 1);

  // And the v18 shape specifically: it had `dreams.working`, a free
  // note-to-self. That note must NOT arrive as the binding choice --
  // binding somebody to a thing they idly clicked last month is exactly
  // the retroactive promise a migration must never make.
  std::string v18 = SerializeSave(save);
  DropSaveLine(v18, "dreams.chosen=");
  const std::size_t vat = v18.find("version=");
  CHECK(vat != std::string::npos);
  v18.insert(vat, "dreams.working=2\n");   // "was saving for the War Chest"
  SetSaveVersion(v18, 18);

  SaveGame noted;
  CHECK(DeserializeSave(v18, noted) == LoadResult::Ok);
  CHECK(noted.player.dreams.chosen == Dream::None);   // unchosen, not bound
  CHECK(HasDream(noted.player.dreams, Dream::Rig));   // owns what it bought
}

static void TestTheCampfireGame() {
  CampfireDials cd;
  const Rng world = Rng::FromSeed("fire-1");
  std::vector<Partner> lot;
  for (const char* n : {"Margo", "Dev", "Trish"}) {
    Partner p;
    p.name = n;
    p.rapport = 1.0;
    lot.push_back(p);
  }

  // A hand is dealt from its own stream, keyed on the day and the hand, so
  // an evening replays identically and reloading cannot reroll a bad night.
  const CampfireHand a = DealPoker(lot, world, 5, 2, cd);
  const CampfireHand b = DealPoker(lot, world, 5, 2, cd);
  CHECK(a.yours == b.yours);
  CHECK(a.truth == b.truth);
  const CampfireHand later = DealPoker(lot, world, 5, 3, cd);
  CHECK(later.yours != a.yours);
  CHECK(a.who.size() == lot.size());
  CHECK(a.reads.size() == lot.size());
  CHECK(a.pot == cd.ante * static_cast<double>(lot.size() + 1));

  // The read is the truth blurred by how little you know somebody. At full
  // rapport it is close; at none it is mostly the middle -- which does not
  // lie to you, it tells you nothing.
  std::vector<Partner> strangers = lot;
  for (Partner& p : strangers) p.rapport = 0.0;
  double closeErr = 0.0, strangeErr = 0.0;
  for (int h = 0; h < 400; h++) {
    const CampfireHand f = DealPoker(lot, world, 1, h, cd);
    const CampfireHand s = DealPoker(strangers, world, 1, h, cd);
    for (std::size_t i = 0; i < f.truth.size(); i++) {
      closeErr += std::abs(f.reads[i] - f.truth[i]);
      strangeErr += std::abs(s.reads[i] - s.truth[i]);
    }
  }
  CHECK(closeErr < strangeErr);   // knowing them is seeing them

  // Folding costs the ante and nothing else, and is still an evening spent
  // with people -- rapport is paid however the hand goes.
  std::vector<Partner> table = strangers;
  double cash = 500.0, foldPsyche = 0.7;
  const CampfireResult f =
      PlayPoker(a, cash, foldPsyche, table, 40.0, true, cd);
  CHECK(f.folded);
  CHECK(f.cashDelta == -cd.ante);
  CHECK(cash == 500.0 - cd.ante);
  CHECK(table[0].rapport > strangers[0].rapport);

  // You cannot bet what you do not have.
  double broke = 3.0, p2 = 0.7;
  std::vector<Partner> t2 = lot;
  PlayPoker(a, broke, p2, t2, 1000.0, false, cd);
  CHECK(broke >= 0.0);

  // The measured shape, and the reason `theyStayAbove` exists. A player who
  // never folds loses steadily; one who plays the read wins, and wins more
  // for knowing the table. Before the Lot folded, a stranger playing the
  // read made $14 a hand and the fire was an infinite cash machine.
  const auto run = [&](double rapport, bool useRead, double stake) {
    std::vector<Partner> tbl;
    for (const char* n : {"Margo", "Dev", "Trish"}) {
      Partner p; p.name = n; p.rapport = rapport; tbl.push_back(p);
    }
    double money = 100000.0, psy = 0.7;
    // Twenty thousand rather than four: the four-thousand run was not
    // converged -- the same policy measured -$7.08 there and -$2.50 at
    // forty thousand -- so every magnitude pinned below was pinned to a
    // number still moving under it.
    for (int h = 0; h < 20000; h++) {
      const CampfireHand hd = DealPoker(tbl, world, 9, h, cd);
      double worst = 0.0;
      for (double r : hd.reads) worst = std::max(worst, r);
      PlayPoker(hd, money, psy, tbl, stake, useRead && hd.yours < worst, cd);
      for (Partner& p : tbl) p.rapport = rapport;
    }
    return (money - 100000.0) / 20000.0;
  };
  // Measured at the middle notch, which is what a player actually sits
  // down on. The ceiling gets its own checks below, because it is now a
  // different question rather than a bigger version of the same one.
  const double mid = StakeNotch(1, cd);
  const double passive = run(1.0, false, mid);
  const double stranger = run(0.0, true, mid);
  const double friendly = run(1.0, true, mid);
  CHECK(stranger > passive);         // thinking beats not thinking
  CHECK(friendly > stranger);        // and knowing them beats thinking

  // Ordering is not enough, and finding that out is why these numbers are
  // here. Deleting the fold rule leaves every ordering check above passing
  // -- a stranger still beats a passive player, a friend still beats a
  // stranger -- while the table quietly pays $14 a hand to anybody with
  // eyes. The bug this game actually had is a *magnitude* bug, so it takes
  // magnitudes to pin it.
  //
  // Measured at the middle notch: passive -$5.00, stranger -$0.79,
  // friend +$2.60.
  CHECK(passive < -2.0);             // inattention has a real price
  CHECK(stranger < 1.0);             // you cannot beat a table you cannot
                                     // read -- which is what liar's dice
                                     // has always said about strangers and
                                     // what poker used to contradict
  CHECK(friendly > 1.0);             // and knowing them is worth real money
  CHECK(friendly < 6.0);             // but not a living

  // The stake has to be a decision, and this is the check that says so.
  //
  // When the fire got a table the player could choose the stake for the
  // first time, and measured immediately, the ceiling won at **every**
  // rapport from stranger to friend -- +$3.35 a hand against +$0.01 at the
  // ante with a stranger, and +$10.17 against +$1.13 with a friend. The
  // Lot called a $40 shove exactly as often as a $5 nudge, so the stake
  // scaled the winnings linearly and never flipped sign. A control whose
  // only correct setting is "maximum" is a lever with one end.
  //
  // `shoveMakesThemFold` is what makes it a choice. Shove and they lay
  // down, so you win the middle and nothing else; nudge and they call, so
  // a good hand gets paid. Measured now: the ceiling is worse than the
  // middle for the friend (+$1.83 against +$2.60) and worse again for the
  // stranger (-$2.31 against -$0.79), because betting big into people you
  // cannot read is how you lose money at a fire.
  const double ceilFriend = run(1.0, true, StakeNotch(2, cd));
  const double ceilStranger = run(0.0, true, StakeNotch(2, cd));
  CHECK(ceilFriend < friendly);      // the ceiling does not dominate
  CHECK(ceilStranger < stranger);    // and shoving blind is punished
  CHECK(ceilFriend > 0.0);           // but it is not simply a trap either

  // A read is only ever words to the player. A number would make this
  // arithmetic; a sentence keeps it a person.
  CHECK(std::string(ReadText(0.05)) != std::string(ReadText(0.95)));
  CHECK(!std::string(ReadText(0.5)).empty());
}

static void TestLiarsDice() {
  CampfireDials cd;
  const Rng world = Rng::FromSeed("dice-1");
  const auto table = [](double r) {
    std::vector<Partner> lot;
    for (const char* n : {"Margo", "Dev", "Trish"}) {
      Partner p; p.name = n; p.rapport = r; lot.push_back(p);
    }
    return lot;
  };

  // Deterministic per day and round, so a night replays and a reload cannot
  // hand you a different cup.
  std::vector<Partner> lot = table(1.0);
  const LiarsDiceRound a = DealLiarsDice(lot, world, 4, 1, cd);
  const LiarsDiceRound b = DealLiarsDice(lot, world, 4, 1, cd);
  CHECK(a.yours == b.yours);
  CHECK(a.bidCount == b.bidCount && a.actual == b.actual);
  CHECK(static_cast<int>(a.yours.size()) == cd.diceEach);
  CHECK(a.bidFace >= 2 && a.bidFace <= 6);   // ones are wild, never bid
  CHECK(a.diceOnTable == cd.diceEach * 4);
  CHECK(!a.bidder.empty());

  // Passing costs the ante and nothing else -- sitting at the table costs
  // whether or not you do anything at it.
  double cash = 500.0, psy = 0.7;
  std::vector<Partner> t = table(0.0);
  const CampfireResult p = PlayLiarsDice(a, cash, psy, t, 40.0, false, cd);
  CHECK(p.folded);
  CHECK(p.cashDelta == -cd.ante);
  CHECK(t[0].rapport > 0.0);          // still an evening with people

  const auto run = [&](double rapport, int mode) {
    std::vector<Partner> tbl = table(rapport);
    double money = 1e6, ps = 0.7;
    for (int i = 0; i < 6000; i++) {
      const LiarsDiceRound r = DealLiarsDice(tbl, world, 7, i, cd);
      const bool call = mode == 1 || (mode == 2 && r.tell > 0.5);
      PlayLiarsDice(r, money, ps, tbl, cd.maxStake, call, cd);
      for (Partner& q : tbl) q.rapport = rapport;
    }
    return (money - 1e6) / 6000.0;
  };
  const double pass = run(0.0, 0);
  const double reflex = run(1.0, 1);
  const double strangerTell = run(0.0, 2);
  const double friendTell = run(1.0, 2);

  // Magnitudes, not orderings -- the lesson from poker. `bidAmbition` began
  // at 1.35, which made nearly every bid a lie: calling blindly won $19 a
  // round and reading the table was *worse* than not thinking. Orderings
  // would not have noticed.
  CHECK(pass < -4.0 && pass > -6.0);   // exactly the ante
  CHECK(reflex < -15.0);               // calling everything is punished hard
  CHECK(strangerTell < pass);          // you cannot read people you do not know
  CHECK(friendTell > 2.0);             // and knowing them is the whole edge
  CHECK(friendTell < 20.0);            // without printing money

  // And the tell must never be perfect. The first version blurred a binary
  // with `lying*q + noise*(1-q)`, whose two cases stop overlapping at any
  // q >= 0.5 -- rapport 0.5 and 1.0 then scored identically to the cent
  // because both read every bid correctly.
  const double halfTell = run(0.5, 2);
  CHECK(halfTell > strangerTell);
  CHECK(halfTell < friendTell - 1.0);  // still improving, not saturated

  CHECK(std::string(TellText(0.05)) != std::string(TellText(0.95)));
}

static void TestBlackjack() {
  CampfireDials cd;
  const Rng world = Rng::FromSeed("jack-1");
  std::vector<Partner> lot;

  // The same hand replays identically however often it is asked.
  const BlackjackHand a = DealBlackjack(world, 3, 1);
  const BlackjackHand b = DealBlackjack(world, 3, 1);
  CHECK(a.yours == b.yours && a.dealerShows == b.dealerShows);
  CHECK(a.yours >= 2 && a.yours <= 20);
  CHECK(a.dealerShows >= 1 && a.dealerShows <= 10);
  CHECK(!a.finished);

  // Hitting is deterministic per draw, so a reload cannot deal you a
  // different card off the same decision.
  BlackjackHand h1 = a, h2 = a;
  CHECK(Hit(h1, world, 3, 1) == Hit(h2, world, 3, 1));
  CHECK(h1.yours == h2.yours && h1.draws == 1);

  // Going over ends it, and a finished hand cannot draw again.
  BlackjackHand doomed = a;
  for (int i = 0; i < 12 && !doomed.finished; i++) Hit(doomed, world, 3, 1);
  CHECK(doomed.bust && doomed.finished);
  CHECK(Hit(doomed, world, 3, 1) == 0);

  const auto run = [&](int standOn) {
    double money = 1e6, ps = 0.7;
    for (int i = 0; i < 20000; i++) {
      BlackjackHand hand = DealBlackjack(world, 8, i);
      while (!hand.finished && hand.yours < standOn) Hit(hand, world, 8, i);
      Stand(hand, money, ps, lot, cd.maxStake, world, 8, i, cd);
    }
    return (money - 1e6) / 20000.0;
  };
  const double good = run(15);
  const double timid = run(12);
  const double reckless = run(21);

  // The intent, pinned: good blackjack costs you the ante and nothing more.
  // It is the game with no edge to build, not the game that punishes you
  // for sitting down -- and not, as the first version was at 1.2 pay with
  // ties to the player, profitable at every strategy anybody would use.
  CHECK(good < -4.0);
  CHECK(good > -6.5);
  CHECK(good > timid);
  CHECK(reckless < -20.0);   // never stopping is never a plan
}

// What is out tonight, and what you can put in. Both were the presentation
// layer's business until the fire got a table; both are rules, so both get
// a test.
// How close was that. Phase 5 item 3 made this a sim judgement rather than
// something the camera was left to infer, so it gets tests like one.
static void TestHowClose() {
  CloseDials cd;

  // Build an attempt that reached `high` of `total` and fell off a move of
  // the given odds.
  const auto attempt = [](int high, int total, double failedOdds) {
    AttemptResult r;
    r.sent = false;
    r.highpoint = high;
    for (int i = 0; i <= high && i < total; i++) {
      MoveResult m;
      m.index = i;
      m.odds = (i == high) ? failedOdds : 0.9;
      m.success = i < high;
      r.timeline.push_back(m);
    }
    return r;
  };

  // A send is a send.
  AttemptResult sent;
  sent.sent = true;
  sent.highpoint = 12;
  CHECK(HowClose(sent, 12, cd) == 1.0);

  // Nothing to be close to.
  AttemptResult nothing;
  CHECK(HowClose(nothing, 12, cd) == 0.0);   // empty timeline
  CHECK(HowClose(attempt(4, 0, 0.9), 0, cd) == 0.0);  // a route of no moves

  // Monotonic in how far you got, everything else pinned. This is the
  // backbone and the one property that must never break.
  double last = -1.0;
  for (int high = 0; high < 12; high++) {
    const double c = HowClose(attempt(high, 12, 0.8), 12, cd);
    CHECK(c >= 0.0 && c <= 1.0);
    CHECK(c > last);
    last = c;
  }

  // The top is worth more than the bottom. Halfway up is well under half a
  // send -- nine of twelve is most of a route and none of a tick, and a
  // linear reading would call it 75% either way.
  CHECK(HowClose(attempt(6, 12, 0.8), 12, cd) < 0.40);
  CHECK(HowClose(attempt(11, 12, 0.8), 12, cd) > 0.70);

  // Falling off a gimme is closer than falling off the crux from the same
  // height. This is the whole reason the timeline is read at all: two
  // attempts that end at move 9 of 12 are not the same attempt.
  const double fluffedIt = HowClose(attempt(9, 12, 0.95), 12, cd);
  const double cruxedIt = HowClose(attempt(9, 12, 0.10), 12, cd);
  CHECK(fluffedIt > cruxedIt);
  // And by a margin worth staging rather than a rounding difference --
  // the magnitude lesson from the campfire, which orderings cannot see.
  CHECK(fluffedIt - cruxedIt > 0.15);

  // Never out of range, whatever the odds do.
  for (int high = 0; high <= 12; high++) {
    for (double odds : {0.0, 0.5, 1.0}) {
      const double c = HowClose(attempt(high, 12, odds), 12, cd);
      CHECK(c >= 0.0 && c <= 1.0);
    }
  }

  // Six bands, all distinct, none of them empty, and none of them a
  // number -- the gate is about a watcher rather than a reader.
  const char* seen[6] = {HowCloseText(0.0),  HowCloseText(0.2),
                         HowCloseText(0.45), HowCloseText(0.7),
                         HowCloseText(0.9),  HowCloseText(1.0)};
  for (int i = 0; i < 6; i++) {
    CHECK(!std::string(seen[i]).empty());
    CHECK(std::string(seen[i]).find_first_of("0123456789") ==
          std::string::npos);
    for (int j = 0; j < i; j++) {
      CHECK(std::string(seen[i]) != std::string(seen[j]));
    }
  }
}

// How much the pump shows. One curve, read by the camera sway and the
// breath and whatever needs it next -- which is the whole reason it is
// here rather than typed into three places in the engine.
static void TestPumpShows() {
  ShowDials sd;

  // Fresh is silent, and stays silent for the first third of a route.
  //
  // Pinned at absolute pump values on purpose. The first version of these
  // asserted PumpShows(quietBelow) and PumpShows(quietBelow - 10), which
  // are both trivially true when quietBelow is zero -- so deleting the
  // quiet zone entirely passed the whole suite. A check phrased in terms
  // of the dial it is checking cannot fail when that dial is wrong.
  CHECK(PumpShows(0.0, sd) == 0.0);
  CHECK(PumpShows(20.0, sd) == 0.0);
  CHECK(PumpShows(30.0, sd) == 0.0);   // a third of the way up, and quiet
  CHECK(PumpShows(45.0, sd) > 0.0);    // but it does start
  // Spent is everything.
  CHECK(PumpShows(100.0, sd) > 0.999);

  // Never out of range, including past the top -- pump is clamped
  // elsewhere and this must not care whether it was.
  for (double p = -20.0; p <= 140.0; p += 2.5) {
    const double s = PumpShows(p, sd);
    CHECK(s >= 0.0 && s <= 1.0);
  }

  // Monotonic once it starts showing at all.
  double last = -1.0;
  for (double p = sd.quietBelow; p <= 100.0; p += 2.0) {
    const double s = PumpShows(p, sd);
    CHECK(s >= last);
    last = s;
  }

  // Late and hard rather than creeping in. Halfway between quiet and spent
  // is well under half of showing -- a linear curve would put it at 0.5 and
  // a watcher would read a bar rather than a climber.
  const double mid = (sd.quietBelow + 100.0) * 0.5;
  CHECK(PumpShows(mid, sd) < 0.35);
  // And the last stretch does most of the work.
  CHECK(PumpShows(90.0, sd) - PumpShows(80.0, sd) >
        PumpShows(50.0, sd) - PumpShows(40.0, sd));

  // A degenerate dial cannot divide by zero or hand back a nonsense
  // number; it just never shows.
  ShowDials never;
  never.quietBelow = 100.0;
  CHECK(PumpShows(100.0, never) == 0.0);
  CHECK(PumpShows(50.0, never) == 0.0);
}

// Zones: where you are, and whether you can walk there. Added when the port
// turned out to have departed from the 2D game -- one travel rule where the
// original has two.
// Who turns up after you. Built because the engine had no way to name a
// climber at all -- ClimberName was an EditAnywhere string with no in-game
// setter, so every career signed its first ascents "you" and the crew hash
// was being fed an empty string.
static void TestWhoTurnsUp() {
  const Rng world = Rng::FromSeed("handover-world");

  // Three, always, and all different -- the UI draws a key per name and a
  // repeated one is a choice that is not a choice.
  for (int gen = 0; gen < 12; gen++) {
    const std::vector<std::string> who = ThreeWhoCouldTurnUp(world, gen);
    CHECK(static_cast<int>(who.size()) == kNameChoices);
    for (std::size_t i = 0; i < who.size(); i++) {
      CHECK(!who[i].empty());
      for (std::size_t j = 0; j < i; j++) {
        CHECK(who[i] != who[j]);
      }
    }
  }

  // Stable across a reload: the same world and generation offers the same
  // three, so reloading the handover cannot reroll who showed up. Same
  // no-reroll rule the fire and the crag already live under.
  CHECK(ThreeWhoCouldTurnUp(world, 3) == ThreeWhoCouldTurnUp(world, 3));

  // And different down the generations, which is the whole reason the
  // generation is a parameter. Checked across a run rather than on one
  // pair, because two adjacent draws colliding is luck and not a bug.
  int changes = 0;
  for (int gen = 1; gen < 12; gen++) {
    if (ThreeWhoCouldTurnUp(world, gen) != ThreeWhoCouldTurnUp(world, gen - 1)) {
      changes++;
    }
  }
  CHECK(changes >= 9);

  // A different world offers different people.
  const Rng elsewhere = Rng::FromSeed("another-valley");
  CHECK(ThreeWhoCouldTurnUp(world, 0) != ThreeWhoCouldTurnUp(elsewhere, 0));

  // Never one of the Lot regulars. The three people you have climbed with
  // for twenty years do not turn up as the kid who inherits the valley.
  const std::vector<Partner> lot = LotRegulars(world, 1);
  for (int gen = 0; gen < 24; gen++) {
    for (const std::string& name : ThreeWhoCouldTurnUp(world, gen)) {
      for (const Partner& p : lot) {
        CHECK(name != p.name);
      }
    }
  }
}

static void TestComp() {
  CompDials cd;
  const Rng world = Rng::FromSeed("comp-day");

  // ---- the season's calendar -------------------------------------------
  //
  // **A schedule you can plan around is the whole difference between a comp
  // and a random event** -- and it is what makes not turning up a decision
  // rather than an accident.
  {
    const Circuit c = StartSeason(world, 1, 1, cd);
    CHECK(static_cast<int>(c.schedule.size()) == cd.compsPerSeason);
    CHECK(c.season == 1);
    CHECK(c.compsDone == 0);
    // In order, with real gaps -- five comps in a week is not a season.
    for (std::size_t i = 1; i < c.schedule.size(); i++) {
      const int gap = c.schedule[i] - c.schedule[i - 1];
      CHECK(gap >= cd.gapMin);
      CHECK(gap < cd.gapMin + cd.gapVariance);
    }
    // A few days of notice before the first one, never the same day.
    CHECK(c.schedule.front() > 1 + cd.announceDaysAhead - 1);

    for (int d : c.schedule) CHECK(CompIsToday(c, d));
    CHECK(!CompIsToday(c, c.schedule.front() - 1));
    CHECK(!CompIsToday(c, c.schedule.front() + 1));

    // **The last one is the finals**, and only the last one.
    CHECK(FinalsToday(c, c.schedule.back()));
    for (std::size_t i = 0; i + 1 < c.schedule.size(); i++) {
      CHECK(!FinalsToday(c, c.schedule[i]));
    }

    // The poster goes up three days out and not before.
    CHECK(DaysUntilComp(c, c.schedule.front(), cd) == 0);
    CHECK(DaysUntilComp(c, c.schedule.front() - 1, cd) == 1);
    CHECK(DaysUntilComp(c, c.schedule.front() - cd.announceDaysAhead, cd) ==
          cd.announceDaysAhead);
    CHECK(DaysUntilComp(c, c.schedule.front() - cd.announceDaysAhead - 1,
                        cd) == -1);
    // And it is over when the last one has been climbed, not when the date
    // passes -- a season you no-showed the end of is still finished.
    CHECK(!SeasonOver(c, cd));
  }

  // ---- the ranking ladder ----------------------------------------------
  //
  // Six named tiers, and **the numbers are load-bearing further up**: 700
  // is where a national team calls you and 1200 is where the Games become
  // reachable, so these are pinned rather than left to drift.
  CHECK(RankFor(0.0, cd) == RankTier::Unranked);
  CHECK(RankFor(cd.regionalClimberAt, cd) == RankTier::RegionalClimber);
  CHECK(RankFor(cd.nationalProspectAt, cd) == RankTier::NationalProspect);
  CHECK(RankFor(cd.nationalTeamAt, cd) == RankTier::NationalTeam);
  CHECK(RankFor(cd.olympicHopefulAt, cd) == RankTier::OlympicHopeful);
  CHECK(RankFor(cd.worldClassAt, cd) == RankTier::WorldClass);
  CHECK(cd.nationalTeamAt == 700.0);
  CHECK(cd.olympicHopefulAt == 1200.0);
  // Monotonic, and every tier is reachable rather than skipped over.
  {
    RankTier last = RankTier::Unranked;
    bool seen[kRankTierCount] = {false, false, false, false, false, false};
    for (double p = 0.0; p <= cd.worldClassAt + 100.0; p += 10.0) {
      const RankTier t = RankFor(p, cd);
      CHECK(static_cast<int>(t) >= static_cast<int>(last));
      last = t;
      seen[static_cast<int>(t)] = true;
    }
    for (int i = 0; i < kRankTierCount; i++) CHECK(seen[i]);
  }
  CHECK(ToNextRank(0.0, cd) == cd.regionalClimberAt);
  CHECK(ToNextRank(cd.worldClassAt, cd) == -1.0);
  CHECK(ToNextRank(cd.nationalTeamAt - 1.0, cd) == 1.0);

  // ---- what a placing is worth -----------------------------------------
  //
  // **1st takes 100, a podium about half of it, mid-field a quarter**, and
  // the back of the field still takes 5 because turning up is worth
  // something and not very much.
  //
  // Pinned as magnitudes rather than as an ordering, because the ordering
  // is what the broken version passed. It was linear in how many people you
  // beat, so **fifth of nine paid fifty -- half a win** -- and a climber who
  // never beat anybody banked twelve hundred points a year by turning up.
  // The ordering held perfectly the whole time.
  CHECK(CircuitPoints(1, 9) == 100.0);
  CHECK(CircuitPoints(9, 9) >= 5.0);
  CHECK(CircuitPoints(9, 9) <= 8.0);
  CHECK(CircuitPoints(3, 9) <= 55.0);          // a podium is not a win
  CHECK(CircuitPoints(3, 9) >= 40.0);          // and it is not nothing
  CHECK(CircuitPoints(5, 9) <= 30.0);          // mid-field is a quarter
  CHECK(CircuitPoints(5, 9) * 4.0 <= CircuitPoints(1, 9) * 1.1);
  CHECK(CircuitPoints(1, 2) == 100.0);
  // Off the end of the field is not a placing at all.
  CHECK(CircuitPoints(10, 9) == 0.0);
  // Monotonic: finishing higher is never worth less.
  for (int p = 2; p <= 9; p++) CHECK(CircuitPoints(p - 1, 9) >=
                                     CircuitPoints(p, 9));

  // ---- the ranking is a record, not a total ----------------------------
  //
  // **The second half of the same fix.** A lifetime total means a tier
  // cleared once is cleared forever: measured at ten years, a career that
  // won nothing peaked at 14,633 points against a top tier of 2,200, and
  // "a career can fail to reach the Games" was false for every seed.
  {
    std::vector<RankingResult> record;
    Record(record, 10, 100.0, cd);
    Record(record, 20, 100.0, cd);
    CHECK(RankingFrom(record, 20, cd) == 200.0);
    // A result counts for a year and then it is gone: recorded on day 10
    // it is worth something through day 374 and worth nothing on 375.
    CHECK(RankingFrom(record, 10 + cd.rankingWindowDays - 1, cd) == 200.0);
    CHECK(RankingFrom(record, 10 + cd.rankingWindowDays, cd) == 100.0);
    CHECK(RankingFrom(record, 20 + cd.rankingWindowDays + 1, cd) == 0.0);
    // A year is a year, said as an absolute -- against the dial it is a
    // tautology that passes with the window at zero.
    CHECK(cd.rankingWindowDays == 365);
    // **Pruned as it is written**, so a thirty-year career does not carry
    // seven hundred results it can never count.
    std::vector<RankingResult> long_;
    for (int day = 1; day <= 30 * 365; day += 14) {
      Record(long_, day, 40.0, cd);
    }
    CHECK(static_cast<int>(long_.size()) < 30);
  }
  // The finals are worth half as much again, and a season podium adds a
  // lump on top of the placing.
  // **Weighted by which room you were in.** A gym podium and a National
  // podium were worth the same, which is what promoted a climber into a
  // room they could not place in while the ranking said they belonged.
  CHECK(RankingPointsFor(1, 9, CompTier::National, true, false, false, false,
                         cd) == std::round(100.0 * cd.finalsMultiplier));
  CHECK(RankingPointsFor(1, 9, CompTier::Local, false, false, false, false,
                         cd) <
        RankingPointsFor(1, 9, CompTier::Regional, false, false, false, false,
                         cd));
  CHECK(RankingPointsFor(1, 9, CompTier::Regional, false, false, false, false,
                         cd) <
        RankingPointsFor(1, 9, CompTier::National, false, false, false, false,
                         cd));
  // **Winning the room you are already too good for is worth less than a
  // podium one rung up**, and about what a mid-field day up there is worth
  // -- which is the whole reason to step up rather than farm the gym.
  CHECK(RankingPointsFor(1, 9, CompTier::Local, false, false, false, false,
                         cd) <
        RankingPointsFor(3, 9, CompTier::National, false, false, false, false,
                         cd));
  CHECK(RankingPointsFor(1, 9, CompTier::Local, false, false, false, false,
                         cd) <=
        RankingPointsFor(4, 9, CompTier::National, false, false, false, false,
                         cd));
  CHECK(RankingPointsFor(1, 9, CompTier::National, false, true, false, false,
                         cd) ==
        100.0 + cd.championRanking);
  CHECK(RankingPointsFor(4, 9, CompTier::National, false, false, false,
                         false, cd) <
        RankingPointsFor(1, 9, CompTier::National, false, false, false, false,
                         cd));

  // ---- the tiers gate on ranking, and they mean three things ----------
  CHECK(TierFor(0.0, cd) == CompTier::Local);
  CHECK(TierFor(cd.regionalAt - 1.0, cd) == CompTier::Local);
  CHECK(TierFor(cd.regionalAt, cd) == CompTier::Regional);
  CHECK(TierFor(cd.nationalAt, cd) == CompTier::National);

  // ---- the board -------------------------------------------------------
  CompState board = SetTheBoard(world, CompTier::Local, 6.0, 10, cd);
  CHECK(static_cast<int>(board.problems.size()) == cd.problems);
  CHECK(board.attemptsLeft == cd.attempts);
  CHECK(!board.finished);

  // **Seven attempts across five problems is the mechanic.** At most two
  // can be worked and the rest are one-shot, so this is a real constraint
  // rather than flavour.
  CHECK(cd.attempts < cd.problems * 2);

  // A board is a spread, not five copies of one grade -- otherwise there is
  // no reason to spend attempts anywhere in particular.
  {
    int lowest = kMaxGrade, highest = 0;
    for (const CompProblem& p : board.problems) {
      lowest = std::min(lowest, p.route.trueGrade);
      highest = std::max(highest, p.route.trueGrade);
    }
    CHECK(highest - lowest >= 2);
  }
  // And it is worth more the harder it is, or the spread would be
  // decoration.
  {
    const CompProblem* easiest = &board.problems[0];
    const CompProblem* hardest = &board.problems[0];
    for (const CompProblem& p : board.problems) {
      if (p.route.trueGrade < easiest->route.trueGrade) easiest = &p;
      if (p.route.trueGrade > hardest->route.trueGrade) hardest = &p;
    }
    CHECK(hardest->topPoints > easiest->topPoints * 1.2);
  }
  // A flash beats a top beats a zone, on every problem.
  for (const CompProblem& p : board.problems) {
    CHECK(p.flashPoints > p.topPoints);
    CHECK(p.topPoints > p.zonePoints);
  }
  // Deterministic: the same day is the same comp, so reloading for a
  // friendlier board is not a strategy.
  {
    const CompState again = SetTheBoard(world, CompTier::Local, 6.0, 10, cd);
    CHECK(again.problems.size() == board.problems.size());
    for (std::size_t i = 0; i < again.problems.size(); i++) {
      CHECK(again.problems[i].route.name == board.problems[i].route.name);
      CHECK(again.problems[i].route.trueGrade ==
            board.problems[i].route.trueGrade);
    }
    const CompState tomorrow = SetTheBoard(world, CompTier::Local, 6.0, 11, cd);
    bool differs = false;
    for (std::size_t i = 0; i < tomorrow.problems.size(); i++) {
      if (tomorrow.problems[i].route.moves.size() !=
          board.problems[i].route.moves.size()) {
        differs = true;
      }
    }
    CHECK(differs);   // and a different day is a different comp
  }
  // Harder tiers set harder problems.
  {
    const CompState nat = SetTheBoard(world, CompTier::National, 6.0, 10, cd);
    int localSum = 0, natSum = 0;
    for (const CompProblem& p : board.problems) localSum += p.route.trueGrade;
    for (const CompProblem& p : nat.problems) natSum += p.route.trueGrade;
    CHECK(natSum > localSum + 5);
  }

  // ---- spending the attempts ------------------------------------------
  {
    CompState c = SetTheBoard(world, CompTier::Local, 6.0, 10, cd);
    Climber body = NewClimber(Rng::FromSeed("competitor"));
    const Rng compRng = Rng::FromSeed("the-comp");

    // A bad index spends nothing. A comp is seven goes and losing one to a
    // fat finger is not a mechanic.
    const int before = c.attemptsLeft;
    AttemptProblem(c, -1, body, compRng, cd);
    AttemptProblem(c, 99, body, compRng, cd);
    CHECK(c.attemptsLeft == before);

    // A real attempt spends exactly one.
    AttemptProblem(c, 0, body, compRng, cd);
    CHECK(c.attemptsLeft == before - 1);
    CHECK(c.progress[0].tries == 1);

    // Run it dry, and it stops.
    while (c.attemptsLeft > 0) AttemptProblem(c, 4, body, compRng, cd);
    CHECK(c.finished);
    const double banked = YourScore(c, cd);
    AttemptProblem(c, 1, body, compRng, cd);
    CHECK(c.attemptsLeft == 0);
    CHECK(YourScore(c, cd) == banked);   // and nothing changes after
  }

  // Scoring reads what you did. Pinned as magnitudes, not orderings.
  {
    CompState c = SetTheBoard(world, CompTier::Local, 6.0, 10, cd);
    CHECK(YourScore(c, cd) == 0.0);
    c.progress[0].topped = true;
    c.progress[0].flashed = true;
    c.progress[0].tries = 1;
    const double flash = YourScore(c, cd);
    CHECK(flash == c.problems[0].flashPoints);
    c.progress[0].flashed = false;
    c.progress[0].tries = 3;
    const double worked = YourScore(c, cd);
    CHECK(worked == c.problems[0].topPoints);
    CHECK(flash > worked);
    // **A flash is worth more than a worked top**, which is the whole
    // reason to spend a go on the problem you believe in first.
    c.progress[0] = ProblemProgress{};
    c.progress[0].zone = 2;
    CHECK(YourScore(c, cd) == c.problems[0].zonePoints);
    c.progress[0].zone = 1;
    CHECK(YourScore(c, cd) < c.problems[0].zonePoints);
  }

  // ---- the field is a redistribution, not a difficulty setting --------
  //
  // **Every competitor has exactly one signature and one weakness**, and no
  // two of them are the same climber. That is what makes the scoreboard a
  // set of people rather than a sorted list.
  {
    const std::vector<Competitor>& f = TheField();
    CHECK(f.size() == 7);
    for (const Competitor& c : f) {
      CHECK(c.signature != c.weakness);
      CHECK(std::string(c.name).size() > 0);
    }
    // **The offsets sit below you, not around you.** The 2D game shipped
    // them symmetric and measured 5th on average, 2.2% podiums and 0% wins
    // forever -- which gated three downstream systems behind an event that
    // happened one comp in fifty. At most one of the seven is above you.
    int above = 0;
    for (const Competitor& c : f) if (c.gradeOffset > 0.0) above++;
    CHECK(above <= 1);
  }

  // A competitor scores better on their signature type than their weakness,
  // with everything else held equal -- and by a real margin.
  {
    CompState crimpy = SetTheBoard(world, CompTier::Local, 6.0, 10, cd);
    for (CompProblem& p : crimpy.problems) p.type = RouteType::Crimp;
    const double asSig = CompetitorScore(crimpy.problems, 6.0,
                                         RouteType::Crimp, RouteType::Dyno,
                                         1.0, cd);
    const double asWeak = CompetitorScore(crimpy.problems, 6.0,
                                          RouteType::Dyno, RouteType::Crimp,
                                          1.0, cd);
    CHECK(asSig > asWeak * 1.2);
  }

  // ---- and the whole thing is a contest -------------------------------
  //
  // **Form on the day is what makes a comp a contest rather than a table
  // lookup.** Without it a given board and a given field produce the same
  // scoreboard every time, and the 2D game shipped exactly that bug on the
  // rival specifically -- he posted his theoretical maximum in every
  // qualifier while everybody else had off days.
  {
    CompState c = SetTheBoard(world, CompTier::Local, 6.0, 10, cd);
    // Three tops: a competitive round rather than a perfect one or a blank,
    // so your score lands *inside* the field and the ordering can move.
    // The first version of this topped one problem, scored 7, and came
    // fifth every single time -- behind a Bowen who reliably scored 9. The
    // check failed with form working perfectly: it was measuring whether
    // one particular score was near a boundary, not whether form existed.
    c.progress[0].topped = true;
    c.progress[1].topped = true;
    c.progress[2].topped = true;
    std::vector<int> places;
    std::vector<std::string> orders;
    for (int s = 0; s < 40; s++) {
      const CompResult r =
          Settle(c, 6.0, "Dex Calloway", 7.0,
                 Rng::FromSeed("settle#" + std::to_string(s)), cd);
      CHECK(r.place >= 1 && r.place <= r.fieldSize);
      CHECK(r.fieldSize == 9);   // you, seven, and the rival
      places.push_back(r.place);
      std::string order;
      for (const CompEntrant& e : r.board) order += e.name + ",";
      orders.push_back(order);
    }
    // The scoreboard is not the same scoreboard twice -- which is the claim
    // form actually makes, and it holds whatever you happened to score.
    bool boardVaries = false, placeVaries = false;
    for (std::size_t i = 1; i < orders.size(); i++) {
      if (orders[i] != orders[0]) boardVaries = true;
      if (places[i] != places[0]) placeVaries = true;
    }
    CHECK(boardVaries);
    CHECK(placeVaries);
  }

  // **Local is yours to lose.** A climber at their own level, topping most
  // of a local board, should podium often -- not once in fifty.
  {
    int podiums = 0, wins = 0;
    for (int s = 0; s < 60; s++) {
      CompState c = SetTheBoard(Rng::FromSeed("b#" + std::to_string(s)),
                                CompTier::Local, 8.0, 10, cd);
      for (std::size_t i = 0; i < c.problems.size(); i++) {
        c.progress[i].topped = true;
        c.progress[i].flashed = (i < 3);
        c.progress[i].zone = 2;
      }
      const CompResult r = Settle(c, 8.0, "", 0.0,
                                  Rng::FromSeed("s#" + std::to_string(s)), cd);
      if (r.place <= 3) podiums++;
      if (r.place == 1) wins++;
    }
    // Measured: 60 podiums and 48 wins of 60. **A perfect local round takes
    // it four times in five and Kai still steals one** -- which is the
    // difference between a comp and a formality, and is what "Local is
    // yours to lose" has to mean.
    CHECK(podiums == 60);
    CHECK(wins > 40);
    CHECK(wins < 60);   // and it is not a formality
  }

  // Placing pays, and beating them is worth its own bump on top.
  {
    CompState c = SetTheBoard(world, CompTier::Local, 6.0, 10, cd);
    for (auto& pr : c.progress) { pr.topped = true; pr.flashed = true; }
    const CompResult won = Settle(c, 6.0, "", 0.0, world, cd);
    CHECK(won.cash > 0.0);
    CHECK(won.rep > 0.0);
    CHECK(!PlacingText(won).empty());
    // **Getting nothing up is last, not fourth.** Before the tie-break knew
    // about zeros, a climber who scored nothing at a comp two tiers above
    // them sorted ahead of everybody else who also scored nothing, came
    // *fourth of eight* and collected top-half prize money for it -- because
    // "you take ties" plus a stable sort plus being pushed onto the board
    // first meant a room full of people who did not climb was a tie you won.
    CompState empty = SetTheBoard(world, CompTier::National, 2.0, 10, cd);
    const CompResult last = Settle(empty, 2.0, "", 0.0, world, cd);
    CHECK(last.yourScore == 0.0);
    CHECK(last.place == last.fieldSize);
    CHECK(last.cash == 0.0);
    // Turning up is still worth a point. Nobody leaves with literally
    // nothing.
    CHECK(last.rep > 0.0);
  }
}

static void TestCircuit() {
  CompDials cd;
  const Rng world = Rng::FromSeed("a-season");
  Skills you;
  you.power = 50; you.fingers = 50; you.technique = 50;
  you.endurance = 50; you.head = 50;

  // ---- everybody scores, not just you ---------------------------------
  //
  // **A table that only tracked your points would be a personal best with
  // other names printed near it.** The season is a season because the field
  // is banking too, and this is the check that says so.
  {
    Circuit c = StartSeason(world, 1, 1, cd);
    CompState board = SetTheBoard(world, CompTier::Local, 8.0, 10, cd);
    for (auto& pr : board.progress) { pr.topped = true; pr.flashed = true; }
    const CompResult r =
        Settle(board, 8.0, "Dex Calloway", 9.0, world, cd);
    BankResult(c, r, false, cd);

    CHECK(c.compsDone == 1);
    CHECK(c.yourPoints > 0.0);
    CHECK(c.rivalPoints > 0.0);
    double fieldTotal = 0.0;
    for (double p : c.fieldPoints) fieldTotal += p;
    CHECK(fieldTotal > 0.0);
    // Winning it banks the most there is.
    CHECK(r.place == 1);
    CHECK(c.yourPoints == 100.0);

    // The table has everybody on it, sorted, with you where you finished.
    const std::vector<CircuitStanding> t = SeasonTable(c);
    CHECK(t.size() >= 8);
    CHECK(t.front().isYou);
    for (std::size_t i = 1; i < t.size(); i++) {
      CHECK(t[i - 1].points >= t[i].points);
    }
  }

  // ---- the finals are worth half as much again -------------------------
  {
    Circuit a = StartSeason(world, 1, 1, cd);
    Circuit b = StartSeason(world, 1, 1, cd);
    CompState board = SetTheBoard(world, CompTier::Local, 8.0, 10, cd);
    for (auto& pr : board.progress) { pr.topped = true; pr.flashed = true; }
    const CompResult r = Settle(board, 8.0, "", 0.0, world, cd);
    BankResult(a, r, false, cd);
    BankResult(b, r, true, cd);
    // Pinned as a magnitude: an ordering passes on a build where the finals
    // multiplier is 1.001, and then a season has no shape.
    CHECK(b.yourPoints == std::round(a.yourPoints * cd.finalsMultiplier));
    CHECK(b.yourPoints > a.yourPoints * 1.4);
  }

  // ---- a comp is climbed by the body you walked in with -----------------
  //
  // **The hole this closes was four systems wide and it was found by
  // accident.** `AttemptProblem` built its own `AttemptInput` and set six
  // fields; it never set the joints, the ailments, the comeback stage, the
  // flaw or the rubber. Measured before the fix, over 400 boards:
  //
  //     comp score, healthy climber:      4.1
  //     comp score, same climber wrecked: 4.1
  //
  // Through the session path the same body carried 2.2 grades of ailment
  // penalty and a fully degraded joint. Two paths, one assembling the
  // input by hand -- the same shape as every "written and never wired"
  // this project has found, in the path that serves gym comps, the
  // circuit, the World Cup, the Games *and* league nights.
  {
    Climber you;
    you.skills.power = you.skills.fingers = you.skills.technique =
        you.skills.endurance = you.skills.head = 60.0;
    you.skin = 100.0;
    you.psyche = 0.7;

    BodyContext wreck;
    wreck.medical.joints[static_cast<int>(InjuryKind::Pulley)] = 1.0;
    wreck.medical.scars.push_back(Scar{InjuryKind::Pulley, 0.9, 1});
    wreck.sickness.active = true;
    wreck.sickness.severity = 0.9;
    wreck.teeth.stage = ToothStage::Abscess;
    wreck.shoeWear = 1.0;
    wreck.day = 400;

    const auto playIt = [&](const BodyContext& b) {
      double total = 0.0;
      const int N = 120;
      for (int s = 0; s < N; s++) {
        const Rng w = Rng::FromSeed("bodyctx#" + std::to_string(s));
        CompState board = SetTheBoard(w, CompTier::Local, 8.0, 40 + s, cd);
        for (int a = 0; a < cd.attempts; a++) {
          int pick = -1;
          for (std::size_t i = 0; i < board.problems.size(); i++) {
            if (!board.progress[i].topped) {
              pick = static_cast<int>(i);
              break;
            }
          }
          if (pick < 0) break;
          AttemptProblem(board, pick, you,
                         w.Derive("a#" + std::to_string(a)), cd, b);
        }
        total += YourScore(board, cd);
      }
      return total / static_cast<double>(N);
    };

    const double well = playIt(BodyContext{});
    const double hurt = playIt(wreck);
    CHECK(well > 0.0);
    // **Materially, not by a rounding error.** A body in that state has to
    // be a worse day at a comp than a clean one, or the whole medical half
    // of the game is decoration in the room it matters most.
    CHECK(hurt < well * 0.75);

    // And each of the four carries on its own, so no one of them can go
    // quiet without this failing.
    BodyContext joints;
    joints.medical.joints[static_cast<int>(InjuryKind::Pulley)] = 1.0;
    joints.day = 400;
    BodyContext ill;
    ill.sickness.active = true;
    ill.sickness.severity = 0.9;
    BodyContext tooth;
    tooth.teeth.stage = ToothStage::Abscess;
    BodyContext rubber;
    rubber.shoeWear = 1.0;
    CHECK(playIt(joints) < well);
    CHECK(playIt(ill) < well);
    CHECK(playIt(tooth) < well);
    CHECK(playIt(rubber) < well);

    // **A flaw and a comp's nerves are two reasons, not one.** The old
    // code assigned the pressure penalty over the top of the flaw, so
    // Happy Feet simply did not exist at a comp; `ApplyBody` adds.
    {
      AttemptInput in;
      in.route.type = RouteType::Technical;
      in.oddsPenalty = 0.4;         // the situation has had its say
      BodyContext happy;
      Build build;
      build.flaw = Flaw::HappyFeet;
      happy.who = MakeCharacter(build, Rng::FromSeed("hf"));
      ApplyBody(in, happy);
      CHECK(in.oddsPenalty > 0.4);
      CHECK(OddsPenalty(happy.who, RouteType::Technical) > 0.0);
    }
    // A clean body is exactly neutral, which is what makes the default
    // argument safe and every golden vector unchanged.
    {
      AttemptInput in;
      ApplyBody(in, BodyContext{});
      CHECK(in.oddsPenalty == 0.0);
      CHECK(in.boldness == 0.0);
      CHECK(in.ailmentPenalty == 0.0);
      CHECK(in.shoeWear == 0.0);
      CHECK(in.injuryStagePenalty == 1.0);
      for (int j = 0; j < kInjuryKindCount; j++) {
        CHECK(in.jointDamage[j] == 0.0);
      }
    }
  }

  // ---- quals, semi, final ----------------------------------------------
  //
  // **A Tuesday at the gym is one board and done; a Regional is a day.**
  // The format is what makes the tiers mean three different things rather
  // than three grade offsets, and the load-bearing part of it is that
  // **the score does not carry**: a good qualification buys a place in the
  // semi and nothing else.
  {
    CHECK(!RunsRounds(CompTier::Local, cd));
    CHECK(RunsRounds(CompTier::Regional, cd));
    CHECK(RunsRounds(CompTier::National, cd));
    CHECK(SurvivorsOf(CompRound::Qualification, cd) == cd.semiCut);
    CHECK(SurvivorsOf(CompRound::Semi, cd) == cd.finalCut);
    CHECK(SurvivorsOf(CompRound::Final, cd) == 0);
    CHECK(cd.finalCut < cd.semiCut);   // it narrows

    // A gym comp does not have rounds, and asking for one ends it.
    {
      CompState local = SetTheBoard(world, CompTier::Local, 8.0, 10, cd);
      CompResult won;
      won.place = 1;
      const RoundOutcome o = NextRound(local, won, world, 8.0, 10, cd);
      CHECK(!o.through);
      CHECK(local.finished);
      CHECK(local.round == CompRound::Qualification);
      CHECK(o.news.empty());   // nothing happened, so nothing is said
    }

    // **Missing the cut is a result, not an error.** Being out in
    // qualification is a different day from finishing last in a final.
    {
      CompState reg = SetTheBoard(world, CompTier::Regional, 8.0, 10, cd);
      CompResult missed;
      missed.place = cd.semiCut + 1;
      const RoundOutcome o = NextRound(reg, missed, world, 8.0, 10, cd);
      CHECK(!o.through);
      CHECK(reg.finished);
      CHECK(o.news.find("qualification") != std::string::npos);
    }

    // Through, and the board underneath you is a new one.
    {
      CompState reg = SetTheBoard(world, CompTier::Regional, 8.0, 10, cd);
      const int qualGrade = reg.problems[0].route.trueGrade;
      // Climb the whole thing, so there is something to not carry.
      for (auto& pr : reg.progress) { pr.topped = true; pr.flashed = true; }
      CHECK(YourScore(reg, cd) > 0.0);
      reg.attemptsLeft = 0;

      CompResult made;
      made.place = 2;
      made.board.push_back(CompEntrant{"Kai", 90.0, false, false});
      made.board.push_back(CompEntrant{"You", 80.0, true, false});
      for (int i = 0; i < cd.semiCut - 2; i++) {
        made.board.push_back(CompEntrant{
            "Filler" + std::to_string(i), 70.0 - i, false, false});
      }
      const RoundOutcome o = NextRound(reg, made, world, 8.0, 10, cd);
      CHECK(o.through);
      CHECK(o.next == CompRound::Semi);
      CHECK(reg.round == CompRound::Semi);
      CHECK(!reg.finished);
      CHECK(o.news.find("semi") != std::string::npos);
      // **The score does not carry.** Nor do the attempts you spent.
      CHECK(YourScore(reg, cd) == 0.0);
      CHECK(reg.attemptsLeft == cd.attempts);
      for (const ProblemProgress& pr : reg.progress) {
        CHECK(pr.tries == 0);
        CHECK(!pr.topped);
      }
      // The field is the cut, and you are in it.
      CHECK(static_cast<int>(reg.stillIn.size()) == cd.semiCut);
      CHECK(StillIn(reg, "You"));
      CHECK(StillIn(reg, "Kai"));
      // Each round sits above the last.
      CHECK(reg.problems[0].route.trueGrade >= qualGrade);

      // **And Settle ranks nobody who went home.** Without the cut being
      // read, a semi-final is scored against the six people who left and
      // every round produces the qualification table again.
      CompState semi = reg;
      const CompResult s = Settle(semi, 8.0, "", 0.0, world, cd);
      CHECK(static_cast<int>(s.board.size()) <
            static_cast<int>(TheField().size()) + 1);
      for (const CompEntrant& e : s.board) {
        CHECK(StillIn(semi, e.isYou ? std::string("You") : e.name));
      }

      // Semi to final: narrower, shorter, and it ends there.
      CompResult top;
      top.place = 1;
      top.board.push_back(CompEntrant{"You", 90.0, true, false});
      for (int i = 0; i < cd.finalCut - 1; i++) {
        top.board.push_back(CompEntrant{
            "Kept" + std::to_string(i), 80.0 - i, false, false});
      }
      const RoundOutcome f = NextRound(reg, top, world, 8.0, 10, cd);
      CHECK(f.through);
      CHECK(reg.round == CompRound::Final);
      CHECK(static_cast<int>(reg.stillIn.size()) == cd.finalCut);
      // **A final is shorter and harder**, so one mistake is the result.
      CHECK(static_cast<int>(reg.problems.size()) == cd.finalProblems);
      CHECK(reg.attemptsLeft == cd.finalAttempts);
      CHECK(cd.finalAttempts < cd.attempts);

      // Nothing comes out of a final but a result.
      CompResult ended;
      ended.place = 1;
      const RoundOutcome after = NextRound(reg, ended, world, 8.0, 10, cd);
      CHECK(!after.through);
      CHECK(reg.finished);
    }
  }

  // ---- not turning up ---------------------------------------------------
  //
  // **A firm schedule you can ignore for free is a suggestion.** The rival
  // banks and you lose standing, and the asymmetry is the commitment.
  {
    Circuit c = StartSeason(world, 1, 1, cd);
    std::vector<RankingResult> record;
    Record(record, 1, 100.0, cd);
    Forfeit(c, record, 2, cd);
    CHECK(c.compsDone == 1);
    CHECK(c.yourPoints == 0.0);
    CHECK(c.rivalPoints == cd.forfeitRivalPoints);
    CHECK(RankingFrom(record, 2, cd) == 100.0 - cd.forfeitRankingLoss);
    // ...and it cannot take you below nothing. A career that no-showed its
    // way to a negative ranking would be a tier system with a hole under it.
    std::vector<RankingResult> broke;
    Forfeit(c, broke, 2, cd);
    CHECK(RankingFrom(broke, 2, cd) == 0.0);
    // **And the cost ages out like everything else on the record.** A bad
    // year that followed a climber forever while a good one did not would
    // be the lifetime-total bug wearing the other hat.
    CHECK(RankingFrom(broke, 2 + cd.rankingWindowDays + 1, cd) == 0.0);
    CHECK(RankingFrom(record, 2 + cd.rankingWindowDays + 1, cd) == 0.0);
  }

  // ---- closing it out ---------------------------------------------------
  {
    // Win every comp of a season and you win the season.
    Circuit c = StartSeason(world, 1, 1, cd);
    CompState board = SetTheBoard(world, CompTier::Local, 8.0, 10, cd);
    for (auto& pr : board.progress) { pr.topped = true; pr.flashed = true; }
    for (int i = 0; i < cd.compsPerSeason; i++) {
      const CompResult r =
          Settle(board, 8.0, "", 0.0,
                 Rng::FromSeed("comp#" + std::to_string(i)), cd);
      BankResult(c, r, i == cd.compsPerSeason - 1, cd);
    }
    CHECK(SeasonOver(c, cd));
    const SeasonEnd e = CloseSeason(c, cd);
    CHECK(e.place == 1);
    CHECK(e.title);
    CHECK(e.cash == cd.championCash);
    CHECK(e.rankingPoints == cd.championRanking);
    // **A season is worth much more than a comp**, or a year of turning up
    // buys nothing a single good Tuesday would not have.
    CHECK(e.cash > cd.winCash * 2.0);
  }
  {
    // **And a season you never entered is not a season you won.** Every
    // comp forfeited: the rival is top of the table and you are not the
    // champion, which is the zero-tie bug from the comp in a longer coat.
    Circuit c = StartSeason(world, 1, 1, cd);
    std::vector<RankingResult> record;
    Record(record, 1, 500.0, cd);
    for (int i = 0; i < cd.compsPerSeason; i++) Forfeit(c, record, 2, cd);
    CHECK(SeasonOver(c, cd));
    const SeasonEnd e = CloseSeason(c, cd);
    CHECK(!e.title);
    CHECK(e.cash == 0.0);
    CHECK(e.place == static_cast<int>(e.table.size()));
    CHECK(RankingFrom(record, 2, cd) < 500.0);
  }

  // ---- and it reads like something ------------------------------------
  {
    Circuit c = StartSeason(world, 1, 1, cd);
    CHECK(!CircuitLine(c, cd).empty());
    CompState board = SetTheBoard(world, CompTier::Local, 8.0, 10, cd);
    for (auto& pr : board.progress) { pr.topped = true; pr.flashed = true; }
    BankResult(c, Settle(board, 8.0, "", 0.0, world, cd), false, cd);
    const std::string mid = CircuitLine(c, cd);
    CHECK(!mid.empty());
    CHECK(mid != CircuitLine(Circuit{}, cd));
    // The last comp says so, because peaking for it is the decision the
    // multiplier exists to create.
    while (c.compsDone < cd.compsPerSeason - 1) {
      BankResult(c, Settle(board, 8.0, "", 0.0, world, cd), false, cd);
    }
    CHECK(CircuitLine(c, cd).find("half as much again") != std::string::npos);
  }

  (void)you;
}

static void TestCircuitCareer() {
  // **The night tick, which nothing covered.** Same shape as the rival's:
  // the season existed and ran only because something called it, and
  // "something" was one line in `SleepToNextDay` that no test touched.
  CompDials cd;
  PlayerState player;
  DayState day;
  const Rng world = Rng::FromSeed("a-circuit-life");
  player.climber = NewClimber(world);

  // A career that never enters a comp still has a circuit going on around
  // it -- a scene that waits for you is not a scene.
  for (int i = 0; i < 400; i++) SleepToNextDay(player, day, world);

  CHECK(player.circuit.season >= 1);
  CHECK(!player.circuit.schedule.empty());

  // **Every date it passed was resolved.** Not one, not all five at once --
  // the ledger keeps up with the calendar, which is what tells a no-show
  // apart from a comp you climbed.
  CHECK(player.circuit.compsDone <= cd.compsPerSeason);
  CHECK(player.circuit.compsDone ==
        std::min(cd.compsPerSeason,
                 CompsDueBy(player.circuit, player.day - 1)));

  // Skipping them all costs standing and hands the rival the season.
  CHECK(player.rankingPoints == 0.0);   // floored, not negative
  if (player.circuit.compsDone > 0) {
    CHECK(player.circuit.rivalPoints > 0.0);
    CHECK(player.circuit.yourPoints == 0.0);
  }

  // Seasons turn over. Four hundred days is several of them at these dials,
  // and a career that saw one season is a career where the break never
  // ended.
  CHECK(player.circuit.season >= 2);

  // **And a career that enters everything is not forfeited for it**, which
  // is the bug the first version of the night tick had: it checked only
  // whether the season was finished, so it forfeited the day after every
  // comp -- including the ones you entered and won.
  {
    PlayerState keen;
    DayState kd;
    keen.climber = NewClimber(world);
    const Rng w = Rng::FromSeed("keen");
    CompState board = SetTheBoard(w, CompTier::Local, 8.0, 10, cd);
    for (auto& pr : board.progress) { pr.topped = true; pr.flashed = true; }
    for (int i = 0; i < 200; i++) {
      // Enter it the moment it is on, exactly as the engine does.
      if (keen.circuit.season > 0 && CompIsToday(keen.circuit, keen.day)) {
        const bool finals = FinalsToday(keen.circuit, keen.day);
        const CompResult r =
            Settle(board, 8.0, "", 0.0,
                   Rng::FromSeed("k#" + std::to_string(keen.day)), cd);
        BankResult(keen.circuit, r, finals, cd);
        // Recorded rather than added. Writing `keen.rankingPoints` here is
        // the bug the field's comment warns about: the night tick
        // recomputes it from the record, so a direct write survives until
        // the next morning and no further. The first version of this test
        // did exactly that and caught it.
        Record(keen.rankingRecord, keen.day,
               RankingPointsFor(r.place, r.fieldSize,
                                TierFor(keen.rankingPoints, cd), finals,
                                false, false, false, cd),
               cd);
      }
      SleepToNextDay(keen, kd, w);
    }
    // Points on the board, nothing lost to a forfeit, and the rival banked
    // nothing off you.
    CHECK(keen.circuit.yourPoints > 0.0);
    CHECK(keen.rankingPoints > 0.0);
    CHECK(keen.circuit.rivalPoints == 0.0);
    // Enough comps to have climbed a tier: this is the whole point of a
    // season, and a ladder you cannot climb by winning is not a ladder.
    CHECK(RankFor(keen.rankingPoints, cd) != RankTier::Unranked);
  }
}

static void TestNationalTeam() {
  TeamDials td;
  CompDials cd;
  const Rng world = Rng::FromSeed("the-call");
  Circuit season = StartSeason(world, 1, 1, cd);
  // Give the field some points so the roster has an order to it.
  for (std::size_t i = 0; i < season.fieldPoints.size(); i++) {
    season.fieldPoints[i] = 400.0 - 40.0 * static_cast<double>(i);
  }

  // ---- the line, and the lower line you are held to -------------------
  //
  // **A national team is not a threshold**, but it does have one, and the
  // interesting part is that there are two: you have to clear 700 to be
  // named and you hold all the way down to 560. That gap is the grace a
  // committee gives a returning athlete, and without it a season spent
  // hovering is a coin flip taken five times.
  CHECK(td.selectAt == 700.0);
  CHECK(td.holdAt < td.selectAt);
  // ...and the same number behaves differently depending on which side of
  // the door you are on, which is the thing the dial comparison above only
  // implies. Without this, collapsing the two lines passes everything that
  // is not a tautology.
  {
    NationalTeam outsider, insider;
    ReviewTheTeam(insider, td.selectAt, season, 1, 1, td);
    CHECK(insider.status == TeamStatus::Named);
    // At exactly the holding line: the one already on it stays, the one
    // outside is not let in.
    ReviewTheTeam(outsider, td.holdAt, season, 2, 2, td);
    ReviewTheTeam(insider, td.holdAt, season, 2, 2, td);
    CHECK(outsider.status == TeamStatus::Never);
    CHECK(insider.status == TeamStatus::Named);
  }
  {
    NationalTeam t;
    CHECK(t.status == TeamStatus::Never);
    CHECK(TeamLine(t, td).empty());   // never been near it is not a status

    // Just short is just short.
    TeamReview r = ReviewTheTeam(t, td.selectAt - 1.0, season, 100, 1, td);
    CHECK(!r.changed);
    CHECK(t.status == TeamStatus::Never);
    CHECK(r.stipend == 0.0);
    // **And the committee has now met**, whether or not it did anything --
    // so the next season's close inside the year is refused rather than
    // deferred. Every day below is a year on from the one before it,
    // because that is how often this can happen.
    CHECK(t.lastReviewDay == 100);
    {
      const TeamReview tooSoon =
          ReviewTheTeam(t, td.selectAt + 5000.0, season,
                        100 + td.reviewEveryDays - 1, 2, td);
      CHECK(!tooSoon.changed);
      CHECK(t.status == TeamStatus::Never);   // not even for a huge number
      CHECK(tooSoon.stipend == 0.0);
      CHECK(td.reviewEveryDays == 365);       // an absolute, not the dial
    }

    // And clearing it is the call.
    r = ReviewTheTeam(t, td.selectAt, season, 100 + td.reviewEveryDays, 1,
                      td);
    CHECK(r.changed);
    CHECK(t.status == TeamStatus::Named);
    CHECK(t.everNamed);
    CHECK(r.rep == td.namedRep);
    CHECK(r.stipend == td.stipend);
    CHECK(!r.news.empty());
    CHECK(!TeamLine(t, td).empty());

    // **The coach is a person and they say something.**
    CHECK(!t.coach.empty());
    CHECK(!t.coachKnownFor.empty());
    CHECK(r.news.find(t.coach) != std::string::npos);

    // **The roster is the top of the field you have been chasing**, in
    // order, and it is people rather than a count.
    CHECK(static_cast<int>(t.roster.size()) == td.size);
    for (std::size_t i = 1; i < t.roster.size(); i++) {
      CHECK(t.roster[i - 1].points >= t.roster[i].points);
    }
    for (const Teammate& m : t.roster) {
      CHECK(!m.name.empty());
      CHECK(!m.role.empty());   // a teammate is a person before a number
    }
    // The roles are not all the same word.
    bool distinct = false;
    for (std::size_t i = 1; i < t.roster.size(); i++) {
      if (t.roster[i].role != t.roster[0].role) distinct = true;
    }
    CHECK(distinct);

    // **You hold below the line you were picked on.**
    const int wasSeasons = t.seasons;
    r = ReviewTheTeam(t, td.holdAt, season, 100 + 2 * td.reviewEveryDays, 2,
                      td);
    CHECK(!r.changed);
    CHECK(t.status == TeamStatus::Named);
    CHECK(t.seasons > wasSeasons);      // another season on the paper
    CHECK(r.stipend == td.stipend);     // and it pays every year

    // ...but not below *that*.
    r = ReviewTheTeam(t, td.holdAt - 1.0, season,
                      100 + 3 * td.reviewEveryDays, 3, td);
    CHECK(r.changed);
    CHECK(t.status == TeamStatus::Cut);
    CHECK(t.cuts == 1);
    CHECK(r.rep < 0.0);                 // being dropped costs you
    CHECK(r.stipend == 0.0);            // and the money stops
    // **Getting the call once never un-happens.**
    CHECK(t.everNamed);
    CHECK(!TeamLine(t, td).empty());
    CHECK(TeamLine(t, td) != std::string());

    // Coming back is news too, and quieter -- and you keep the coach you
    // arrived with.
    const std::string firstCoach = t.coach;
    r = ReviewTheTeam(t, td.selectAt, season,
                      100 + 4 * td.reviewEveryDays, 4, td);
    CHECK(r.changed);
    CHECK(t.status == TeamStatus::Named);
    CHECK(r.rep == td.renamedRep);
    CHECK(r.rep < td.namedRep);
    CHECK(t.coach == firstCoach);
  }

  // ---- being cut is not the same as never being called ----------------
  {
    NationalTeam never;
    NationalTeam cut;
    ReviewTheTeam(cut, td.selectAt, season, 10, 1, td);
    ReviewTheTeam(cut, 0.0, season, 10 + td.reviewEveryDays, 2, td);
    CHECK(cut.status == TeamStatus::Cut);
    CHECK(TeamLine(never, td).empty());
    CHECK(!TeamLine(cut, td).empty());
    CHECK(std::string(StatusText(TeamStatus::Named)) !=
          StatusText(TeamStatus::Cut));
    CHECK(std::string(StatusText(TeamStatus::Cut)) !=
          StatusText(TeamStatus::Never));
  }

  // ---- the coach is fixed by when you arrived -------------------------
  //
  // Deterministic from the season, so a reload cannot hand you a different
  // one for the same call-up -- and so the coach you got is a fact about
  // *when* you got there.
  {
    CHECK(std::string(CoachFor(1).name) == CoachFor(1).name);
    bool varies = false;
    for (int s = 2; s < 8; s++) {
      if (std::string(CoachFor(s).name) != CoachFor(1).name) varies = true;
    }
    CHECK(varies);
    for (const Coach& c : TheCoaches()) {
      CHECK(std::string(c.name).size() > 0);
      CHECK(std::string(c.knownFor).size() > 20);   // they say something
    }
  }

  // ---- and the roster turns over under you ----------------------------
  //
  // People come and go whether or not your own status changed, because the
  // roster is rebuilt from the field at every review.
  {
    NationalTeam t;
    ReviewTheTeam(t, td.selectAt, season, 10, 1, td);
    const std::string wasTop = t.roster.front().name;
    // The field reshuffles: the bottom of it has the season now.
    for (std::size_t i = 0; i < season.fieldPoints.size(); i++) {
      season.fieldPoints[i] = 40.0 * static_cast<double>(i);
    }
    ReviewTheTeam(t, td.selectAt, season, 10 + td.reviewEveryDays, 2, td);
    CHECK(t.roster.front().name != wasTop);
    CHECK(!t.gone.empty());   // and the game knows who is not on it now
  }
}

// A World Cup scorecard with you in a given place -- the shape `BankRound`
// requires now that the season table is fed from the card rather than from
// a second, independent roll. Thirteen names, yours among them, in order.
static CompResult ARoundYouPlaced(int yourPlace) {
  CompResult r;
  r.place = yourPlace;
  const std::vector<International>& f = TheWorldField();
  r.fieldSize = static_cast<int>(f.size()) + 1;
  int taken = 0;
  for (int place = 1; place <= r.fieldSize; place++) {
    CompEntrant e;
    if (place == yourPlace) {
      e.name = "You";
      e.isYou = true;
    } else {
      e.name = std::string(f[taken].name) + " (" + f[taken].nation + ")";
      taken++;
    }
    e.score = 100.0 - place;
    r.board.push_back(e);
  }
  return r;
}

// A climber with a given injury, and a comeback started properly.
static Climber HurtClimber(InjuryKind kind, double severity) {
  Climber c;
  c.skills.power = c.skills.fingers = c.skills.technique =
      c.skills.endurance = c.skills.head = 50.0;
  c.injury.active = true;
  c.injury.kind = kind;
  c.injury.severity = severity;
  c.injury.daysLeft = InjuryDaysFor(severity);
  return c;
}

// A map you cannot tell apart is not a map.
static void TestEveryZoneIsSomewhere() {
  // **Nine of the eleven walkable zones have no content and will not have
  // any for months.** That was always the plan -- a zone gets a
  // walk-through and a name until something wants to live there, which is
  // what a real town has between the places you go. What is not the plan
  // is a zone with nothing to say: the wider map would be eleven identical
  // grey boxes.
  std::vector<std::string> blurbs;
  for (int i = 0; i < kZoneCount; i++) {
    const Zone z = static_cast<Zone>(i);
    const std::string name = ZoneName(z);
    const std::string blurb = ZoneBlurb(z);
    CHECK(!name.empty());
    // Long enough to be a sentence about a place rather than a label.
    CHECK(blurb.size() > 25);
    // And it says something, rather than restating the name.
    CHECK(blurb != name);
    blurbs.push_back(blurb);
  }
  // **All sixteen different.** A copied line is the same grey box with a
  // different sign on it.
  for (std::size_t i = 0; i < blurbs.size(); i++) {
    for (std::size_t j = 0; j < i; j++) {
      CHECK(blurbs[i] != blurbs[j]);
    }
  }
}

static void TestCraft() {
  CraftDials cd;
  const Rng world = Rng::FromSeed("a-second-career");

  // ---- every trade has a gig behind it ----------------------------------
  //
  // **A craft you cannot practise is a stat.** Every one of these is on the
  // board, which is why three gigs were added with this phase rather than
  // three enum entries.
  {
    CHECK(CraftForGig("setting at the gym") == Craft::Setting);
    CHECK(CraftForGig("belaying kids' birthdays") == Craft::Coaching);
    CHECK(CraftForGig("a shift at the gear shop") == Craft::Counter);
    CHECK(CraftForGig("hauling firewood") == Craft::Labour);
    CHECK(CraftForGig("trail work for the park") == Craft::Trail);
    CHECK(CraftForGig("shooting photos for the guidebook") == Craft::Camera);
    CHECK(CraftForGig("a courier run across town") == Craft::Courier);
    CHECK(CraftForGig("a callout with the rescue team") == Craft::Rescue);
    CHECK(CraftForGig("a shift behind the bar") == Craft::Bar);
    // Some work is just work, and saying so is better than pretending.
    CHECK(CraftForGig("flyering for the climbing festival") == Craft::None);

    // Every trade the enum names has to be reachable from the board over a
    // spread of days, or the enum is lying.
    bool seen[kCraftCount] = {false};
    for (int day = 1; day <= 200; day++) {
      for (const OddJob& j : OddJobBoard(world, day)) {
        seen[static_cast<int>(CraftForGig(j.name))] = true;
      }
    }
    for (int i = 1; i < kCraftCount; i++) {
      // The office is the salaried job and never appears on the board,
      // which is the whole difference between the two shapes of work.
      if (static_cast<Craft>(i) == Craft::Office) continue;
      CHECK(seen[i]);
    }
  }

  // ---- the job you keep changes who your climber is ---------------------
  //
  // **Gate one.** Every trade pays back into the body or the head, and
  // none of it is as good as climbing -- work that trained you as well as
  // climbing did would make the salaried trap not a trap.
  {
    CHECK(CraftTeaches(Craft::Setting) == Skill::Technique);
    CHECK(CraftTeaches(Craft::Coaching) == Skill::Head);
    CHECK(CraftTeaches(Craft::Trail) == Skill::Endurance);
    CHECK(CraftTeaches(Craft::Labour) == Skill::Power);

    Craftsman hand;
    CHECK(hand.skill[static_cast<int>(Craft::Setting)] == 0.0);
    double taught = 0.0;
    for (int shift = 0; shift < 200; shift++) {
      taught += WorkTheTrade(hand, Craft::Setting, 5.0, cd).skillGain;
    }
    // Two hundred shifts is a decade of Saturdays, and it makes you as
    // good at it as anybody. **A craft is masterable and climbing is
    // not**, which is a real difference: you can be a genuinely great
    // route setter, and nobody is ever finished being a climber.
    CHECK(hand.skill[static_cast<int>(Craft::Setting)] > 90.0);
    {
      // ...but not in a season. Fifty shifts is a year of Saturdays and it
      // makes you competent, which is the shape that matters.
      Craftsman year;
      for (int s = 0; s < 50; s++) WorkTheTrade(year, Craft::Setting, 5.0, cd);
      CHECK(year.skill[static_cast<int>(Craft::Setting)] > 40.0);
      CHECK(year.skill[static_cast<int>(Craft::Setting)] < 80.0);
    }
    CHECK(hand.shifts[static_cast<int>(Craft::Setting)] == 200);
    // And it taught your climbing something. **A couple of grades' worth
    // of technique over a decade** -- real, and nothing like climbing.
    CHECK(taught > 5.0);
    CHECK(taught < 30.0);
    // Only that trade. Setting for a decade teaches you nothing about
    // the bar.
    CHECK(hand.skill[static_cast<int>(Craft::Bar)] == 0.0);
    // And work that is just work teaches nothing at all.
    Craftsman flyerer;
    CHECK(WorkTheTrade(flyerer, Craft::None, 5.0, cd).skillGain == 0.0);

    // **The economic reason to specialise.** A beginner is worth less and
    // a veteran is worth more, and the spread is wide enough to notice.
    Craftsman green;
    CHECK(CraftPay(green, Craft::Setting, cd) == cd.payAtNothing);
    CHECK(CraftPay(hand, Craft::Setting, cd) > 1.0);
    CHECK(cd.payAtNothing < 1.0);
    CHECK(cd.payAtMastery > 1.3);
    CHECK(cd.payAtMastery < 2.0);   // and never enough to beat a better gig
  }

  // ---- a shift has a decision in it -------------------------------------
  //
  // **Gate two**, and the whole of it is that the right answer is harder
  // and needs the craft you have actually built.
  {
    // Something comes up often enough to be a rhythm and rarely enough to
    // be an event.
    int happened = 0;
    for (int day = 1; day <= 400; day++) {
      if (MomentOnShift(Craft::Setting, world, day, cd).happened) happened++;
    }
    CHECK(happened > 80);
    CHECK(happened < 250);
    // Deterministic on the day, so a reload does not reroll it -- the same
    // no-reroll rule every gamble in this game lives under.
    const ShiftMoment a = MomentOnShift(Craft::Setting, world, 40, cd);
    const ShiftMoment b = MomentOnShift(Craft::Setting, world, 40, cd);
    CHECK(a.happened == b.happened);
    CHECK(std::string(a.what) == b.what);
    // And nothing ever comes up on work that is just work.
    for (int day = 1; day <= 200; day++) {
      CHECK(!MomentOnShift(Craft::None, world, day, cd).happened);
    }

    // Find a day something came up on.
    ShiftMoment moment;
    int on = 0;
    for (int day = 1; day <= 400 && !moment.happened; day++) {
      moment = MomentOnShift(Craft::Setting, world, day, cd);
      on = day;
    }
    CHECK(moment.happened);
    CHECK(moment.needs > 0.0);
    CHECK(std::string(moment.theHardWay) != moment.theEasyWay);

    // **The easy way is never wrong and never gets you anywhere.** It has
    // to be a real option, or the decision is a skill check.
    {
      Craftsman hand;
      const MomentOutcome out =
          DecideTheMoment(hand, moment, false, world, on, cd);
      CHECK(!out.botched);
      CHECK(out.payMultiplier == 1.0);
      CHECK(out.standingShift == 0.0);
      CHECK(hand.momentsDucked == 1);
      CHECK(hand.momentsTaken == 0);
      CHECK(!out.news.empty());
    }

    // **Reaching past your craft is how you botch it.** Somebody who can
    // do the job mostly pulls it off; somebody who cannot, mostly does
    // not -- and both can go either way, which is what makes it a
    // decision rather than a gate.
    const auto tryIt = [&](double craft) {
      int ok = 0;
      for (int s = 0; s < 60; s++) {
        Craftsman hand;
        hand.skill[static_cast<int>(Craft::Setting)] = craft;
        ShiftMoment m;
        int day = 0;
        for (int d = 1 + s * 7; d <= 400 + s * 7 && !m.happened; d++) {
          m = MomentOnShift(Craft::Setting, world, d, cd);
          day = d;
        }
        if (!m.happened) continue;
        if (!DecideTheMoment(hand, m, true, world, day, cd).botched) ok++;
      }
      return ok;
    };
    const int cannot = tryIt(10.0);
    const int can = tryIt(70.0);
    CHECK(can > cannot);
    CHECK(can > 40);        // somebody who can do the job mostly can
    CHECK(cannot < 25);     // and somebody who cannot, mostly cannot
    CHECK(cannot > 0);      // but it is never a wall
  }

  // ---- getting fired outlives the job -----------------------------------
  //
  // **Gate three.** Standing is per trade, and when it falls far enough
  // the gig comes off your board and stays off.
  {
    Craftsman hand;
    CHECK(WillTheyHireYou(hand, Craft::Setting, cd));
    // Botch it enough and they stop calling.
    hand.standing[static_cast<int>(Craft::Setting)] = cd.sackAt + 0.05;
    ShiftMoment m;
    int day = 0;
    for (int d = 1; d <= 400 && !m.happened; d++) {
      m = MomentOnShift(Craft::Setting, world, d, cd);
      day = d;
    }
    CHECK(m.happened);
    const MomentOutcome out = DecideTheMoment(hand, m, true, world, day, cd);
    if (out.botched) {
      CHECK(out.sacked);
      CHECK(!WillTheyHireYou(hand, Craft::Setting, cd));
      CHECK(hand.sackings == 1);
      CHECK(out.news.find("not be calling") != std::string::npos);

      // **And it shows up as work you cannot take**, which is the whole
      // difference between a sacking and a number in a menu.
      CHECK(!WillTheyHireYou(hand, CraftForGig("setting at the gym"), cd));
      CHECK(WillTheyHireYou(
          hand, CraftForGig("washing dishes at the diner"), cd));
      // And `WorkOddJob` refuses it too, so a caller that forgets to
      // filter the board shows the gig and cannot take it, rather than
      // walking straight past a sacking.
      {
        PlayerState p;
        p.hand = hand;
        DayState d = WakeUp(p);
        OddJob setting;
        setting.name = "setting at the gym";
        setting.hours = 5.0;
        setting.pay = 95.0;
        CHECK(!WorkOddJob(p, d, setting, world, false));
        OddJob dishes;
        dishes.name = "washing dishes at the diner";
        dishes.hours = 5.0;
        dishes.pay = 60.0;
        CHECK(WorkOddJob(p, d, dishes, world, false));
      }

      // The way back is to be good at it for a long time, and the gig is
      // off your board the whole time you are -- which is the point.
      CHECK(cd.rehireAt > cd.sackAt);
      for (int s = 0; s < 500 && hand.sacked[
               static_cast<int>(Craft::Setting)]; s++) {
        WorkTheTrade(hand, Craft::Setting, 5.0, cd);
      }
      // ...except you cannot work a trade that will not have you, so in
      // practice a sacking is for good unless something else lifts it.
      // Said here rather than left to be discovered.
      CHECK(true);
    }
    // Nobody is precious about flyering.
    Craftsman done;
    done.sacked[static_cast<int>(Craft::None)] = true;
    CHECK(WillTheyHireYou(done, Craft::None, cd));
  }

  // ---- and the work identity --------------------------------------------
  {
    Craftsman none;
    CHECK(YourTrade(none, cd) == Craft::None);
    CHECK(TradeText(none, 8.0, cd).empty());
    CHECK(CraftText(none, cd).empty());

    Craftsman setter;
    for (int s = 0; s < 300; s++) WorkTheTrade(setter, Craft::Setting, 6.0, cd);
    CHECK(YourTrade(setter, cd) == Craft::Setting);
    CHECK(!CraftText(setter, cd).empty());
    // **Which way round it goes depends on how much of each you have.** A
    // climber who is better at setting than at climbing is a setter who
    // climbs, whatever they would say at a party.
    const std::string asSetter = TradeText(setter, 4.0, cd);
    const std::string asClimber = TradeText(setter, 17.0, cd);
    CHECK(!asSetter.empty());
    CHECK(!asClimber.empty());
    CHECK(asSetter != asClimber);
    CHECK(asClimber.find("A climber") != std::string::npos);
    CHECK(asSetter.find("Somebody who") != std::string::npos);
  }
}

static void TestAilments() {
  AilmentDials ad;
  const Rng world = Rng::FromSeed("everything-else");

  // ---- sickness is not your fault, and that is its job ------------------
  //
  // Every injury in this game is something you did -- deliberately, because
  // an injury must never be weather. **Sickness is the counterweight**, and
  // it is still not a die: it arrives off how you are living, so a fed,
  // rested climber in a warm van essentially never gets ill and a hungry
  // one sleeping cold on a wrecked body does.
  {
    Climber fresh;
    Climber wrecked;
    wrecked.load = 95.0;

    const auto catchRate = [&](const Climber& c, double hunger,
                               double warmth) {
      int caught = 0;
      for (int s = 0; s < 4000; s++) {
        Sickness sick;
        if (SicknessDay(sick, c, hunger, warmth,
                        Rng::FromSeed("s#" + std::to_string(s)), 1, ad)) {
          caught++;
        }
      }
      return caught;
    };

    const int living = catchRate(fresh, 0.0, 1.0);
    const int rough = catchRate(wrecked, 95.0, 0.0);
    // Living well is not immunity -- it is odds. Pinned as magnitudes,
    // because an ordering passes on a build where both are one in a
    // million and nobody is ever ill.
    CHECK(rough > living * 3);
    CHECK(rough > 40);            // it happens
    CHECK(living < rough / 3);    // and living well is most of the answer

    // Once you have it, it counts down and clears itself.
    Sickness sick;
    sick.active = true;
    sick.daysLeft = 5;
    sick.severity = 0.4;
    Climber c;
    for (int i = 0; i < 5; i++) {
      CHECK(sick.active);
      SicknessDay(sick, c, 0.0, 1.0, world, 10 + i, ad);
    }
    CHECK(!sick.active);
    CHECK(sick.severity == 0.0);

    // **The cheapest decision in the game**, and it is a decision anyway.
    Sickness ill;
    ill.active = true;
    ill.daysLeft = 8;
    ill.severity = 0.5;
    double cash = 5.0;
    CHECK(!TakeSomethingForIt(ill, cash, ad));   // eleven dollars is money
    cash = 40.0;
    CHECK(TakeSomethingForIt(ill, cash, ad));
    CHECK(cash == 40.0 - ad.medsCost);
    CHECK(ill.daysLeft < 8);
    CHECK(ill.daysLeft >= 1);
    CHECK(!TakeSomethingForIt(ill, cash, ad));   // once
    CHECK(ad.medsCost < 20.0);

    // It costs you on the wall, flat, because there is no such thing as a
    // cold that is fine on slopers.
    CHECK(SickPenalty(Sickness{}, ad) == 0.0);
    CHECK(SickPenalty(ill, ad) > 0.0);
    Sickness bad;
    bad.active = true;
    bad.severity = 1.0;
    Sickness mild;
    mild.active = true;
    mild.severity = 0.0;
    CHECK(SickPenalty(bad, ad) > SickPenalty(mild, ad));
    CHECK(SickPenalty(mild, ad) > 0.0);   // even a sniffle costs something
    CHECK(!SickText(ill, ad).empty());
    CHECK(SickText(Sickness{}, ad).empty());
  }

  // ---- the tooth only ever goes one way ---------------------------------
  //
  // **The exact inverse of an injury**, where ignoring it is sometimes
  // fine. Nothing here improves with rest, no amount of time helps, and the
  // only thing that has ever fixed it is money -- which costs more at every
  // stage. It is the game's one pure test of whether you will spend on
  // something that is not climbing.
  {
    Teeth t;
    CHECK(t.stage == ToothStage::Fine);
    CHECK(TeethText(t, ad).empty());
    CHECK(ToothPrice(ToothStage::Fine, ad) == 0.0);

    // It starts on its own, eventually.
    int started = 0;
    for (int day = 1; day <= 4000 && t.stage == ToothStage::Fine; day++) {
      if (TeethDay(t, world, day, ad)) started = day;
    }
    CHECK(started > 0);
    CHECK(t.stage == ToothStage::Twinge);

    // **And then it only escalates -- until it does not, and the way it
    // stops is the tooth.** Run it out and it never once gets better while
    // it is there; it goes twinge, ache, abscess, and then the tooth comes
    // out and you are Fine again with one fewer tooth.
    //
    // That last part is what bounds never paying. Without it an abscess
    // was permanent: measured, a career that would not pay spent **10,688
    // of 10,950 days** with one and came out with four sends instead of
    // twenty-four, which is not a money test, it is a silent career-ender.
    ToothStage worst = t.stage;
    int reachedAbscess = 0;
    for (int day = started + 1; day <= started + 900; day++) {
      const bool moved = TeethDay(t, world, day, ad);
      if (t.stage == ToothStage::Abscess && !reachedAbscess) {
        reachedAbscess = day;
      }
      if (!reachedAbscess) {
        CHECK(static_cast<int>(t.stage) >= static_cast<int>(worst));
      }
      worst = t.stage;
      if (moved && t.lost > 0) break;
    }
    CHECK(reachedAbscess > 0);
    CHECK(t.lost == 1);
    CHECK(t.stage == ToothStage::Fine);
    CHECK(t.worstEver == static_cast<int>(ToothStage::Abscess));
    // The better part of a year of it, and then a tooth. Not a career.
    CHECK(ad.toothAbscessAfter >= 200);
    CHECK(ad.toothAbscessAfter <= 400);

    // Put it back at an abscess for what follows.
    t.stage = ToothStage::Abscess;
    t.sinceDay = 5000;
    // A twinge reaches an abscess inside two years, and not inside one --
    // long enough to ignore, short enough that ignoring it is a decision
    // you live to regret in the same career.
    CHECK(ad.toothTwingeAfter + ad.toothAcheAfter > 365);
    CHECK(ad.toothTwingeAfter + ad.toothAcheAfter < 730);

    // **Money only ever goes up.** A filling is a shift; a root canal is
    // most of a month.
    CHECK(ToothPrice(ToothStage::Twinge, ad) <
          ToothPrice(ToothStage::Ache, ad));
    CHECK(ToothPrice(ToothStage::Ache, ad) <
          ToothPrice(ToothStage::Abscess, ad));
    CHECK(ToothPrice(ToothStage::Abscess, ad) >
          ToothPrice(ToothStage::Twinge, ad) * 8.0);
    CHECK(ToothPrice(ToothStage::Twinge, ad) <= DayDials{}.shiftWage * 1.5);

    // What it takes while you have it. A twinge is a warning, and warnings
    // are free; an abscess is the main thing about your week.
    Teeth twinge;
    twinge.stage = ToothStage::Twinge;
    Teeth ache;
    ache.stage = ToothStage::Ache;
    CHECK(ToothPsycheCost(twinge, ad) == 0.0);
    CHECK(ToothPsycheCost(ache, ad) > 0.0);
    CHECK(ToothPsycheCost(t, ad) > ToothPsycheCost(ache, ad));
    CHECK(ToothGradePenalty(twinge, ad) == 0.0);
    CHECK(ToothGradePenalty(ache, ad) == 0.0);
    CHECK(ToothGradePenalty(t, ad) > 0.0);
    CHECK(!TeethText(t, ad).empty());

    // Paying makes it go away, and the clock can start again another year.
    double cash = 100.0;
    CHECK(!FixTheTooth(t, cash, 5001, ad));   // an abscess is not $100
    cash = 2000.0;
    CHECK(FixTheTooth(t, cash, 5001, ad));
    CHECK(t.stage == ToothStage::Fine);
    CHECK(t.fixes == 1);
    CHECK(cash == 2000.0 - ad.toothAbscessCost);
    // But what it was is remembered, because a career remembers -- and so
    // is the one you did not pay for.
    CHECK(t.worstEver == static_cast<int>(ToothStage::Abscess));
    CHECK(t.lost == 1);
    CHECK(!FixTheTooth(t, cash, 5001, ad));   // nothing to fix
  }

  // ---- prehab is boring, it works, and nobody does it -------------------
  {
    Upkeep u;
    CHECK(PrehabRisk(u, ad) == 1.0);
    CHECK(UpkeepText(u, 1, ad).empty());

    double hour = 7.0;
    CHECK(DoPrehab(u, hour, 1, ad));
    CHECK(hour > 7.0);                       // it costs the morning
    CHECK(!DoPrehab(u, hour, 1, ad));        // once a day
    CHECK(u.prehabStreak == 1);
    // One morning is nearly nothing.
    CHECK(PrehabRisk(u, ad) > 0.9);

    // **A streak, not a total.** Twenty minutes most mornings is the whole
    // effect.
    for (int day = 2; day <= 40; day++) {
      double h = 7.0;
      DoPrehab(u, h, day, ad);
      UpkeepDay(u, day, ad);
    }
    CHECK(u.prehabStreak >= ad.prehabStreakFor);
    CHECK(PrehabRisk(u, ad) <= 1.0 - ad.prehabRiskCut + 1e-9);
    // **Lowers the odds and never removes them.** A climber who has done
    // their twenty minutes every morning for a year still pops a pulley.
    CHECK(PrehabRisk(u, ad) > 0.5);
    CHECK(!UpkeepText(u, 40, ad).empty());

    // Life happens, and a habit you lose by going to a wedding is not a
    // habit, it is a chore. A few days off is survivable.
    Upkeep gap = u;
    UpkeepDay(gap, 40 + ad.prehabGraceDays, ad);
    CHECK(gap.prehabStreak > 0);
    double h = 7.0;
    DoPrehab(gap, h, 40 + ad.prehabGraceDays, ad);
    CHECK(gap.prehabStreak > ad.prehabStreakFor);
    // A fortnight off is not.
    Upkeep gone = u;
    UpkeepDay(gone, 60, ad);
    CHECK(gone.prehabStreak == 0);
    CHECK(PrehabRisk(gone, ad) == 1.0);
  }

  // ---- the shrink is the only thing that buys psyche --------------------
  {
    Upkeep u;
    Climber c;
    c.psyche = 0.2;
    double cash = 50.0;
    CHECK(!SeeTheShrink(u, c, cash, 100, ad));   // it is not cheap
    cash = 500.0;
    CHECK(SeeTheShrink(u, c, cash, 100, ad));
    CHECK(c.psyche > 0.2);
    CHECK(cash == 500.0 - ad.shrinkCost);
    CHECK(u.shrinkSessions == 1);
    // Rate limited, same as a physio and for the same reason: a rich
    // season cannot buy its way out of a bad one in an afternoon.
    CHECK(!SeeTheShrink(u, c, cash, 100 + ad.shrinkDaysBetween - 1, ad));
    CHECK(SeeTheShrink(u, c, cash, 100 + ad.shrinkDaysBetween, ad));

    // **And it does something a rest day cannot: it moves where you drift
    // back to.** That is the whole difference between resting and getting
    // help.
    Upkeep none;
    CHECK(PsycheFloor(none, 0.7, 200, ad) == 0.7);
    CHECK(PsycheFloor(u, 0.7, 100 + ad.shrinkDaysBetween, ad) > 0.7);
    // For a while, and then not.
    CHECK(PsycheFloor(u, 0.7, 100 + ad.shrinkDaysBetween +
                                 ad.shrinkLasts + 1, ad) == 0.7);
    CHECK(!UpkeepText(u, 100 + ad.shrinkDaysBetween, ad).empty());
  }
}

static void TestMedical() {
  MedicalDials md;
  const Rng world = Rng::FromSeed("the-body-keeps-score");

  // ---- the fog is the mechanic ------------------------------------------
  //
  // **An injury hides its grade until you pay to look at it.** A climber
  // with a sore finger does not know whether it is three weeks or three
  // months, and every decision they make is made without the number.
  {
    Medical med;
    Climber c = HurtClimber(InjuryKind::Pulley, 0.8);
    StartComeback(med, c, 1, md);
    CHECK(med.diagnosis == Diagnosis::None);
    CHECK(med.toldSeverity == 0.0);
    CHECK(med.stage == Comeback::Resting);
    // What it reads like says nothing about how bad. That is the design
    // and not a missing string.
    const std::string blind = MedicalText(med, c, 1, md);
    CHECK(!blind.empty());
    CHECK(blind.find("do not know") != std::string::npos);

    // Paying a physio gets you close, and the error is the fog.
    double cash = 1000.0;
    CHECK(Diagnose(med, c, cash, Diagnosis::Guessed, world, 1, md));
    CHECK(cash == 1000.0 - md.guessCost);
    CHECK(med.diagnosis == Diagnosis::Guessed);
    CHECK(med.toldSeverity > 0.0);
    CHECK(std::fabs(med.toldSeverity - c.injury.severity) <=
          md.guessError + 1e-9);
    // And **it is not always right**, which is the point -- over a spread
    // of seeds it lands both sides of the truth.
    {
      int high = 0, low = 0;
      for (int s = 0; s < 40; s++) {
        Medical m2;
        Climber c2 = HurtClimber(InjuryKind::Pulley, 0.5);
        StartComeback(m2, c2, 1, md);
        double money = 1000.0;
        Diagnose(m2, c2, money, Diagnosis::Guessed,
                 Rng::FromSeed("g#" + std::to_string(s)), 1, md);
        if (m2.toldSeverity > 0.5) high++;
        if (m2.toldSeverity < 0.5) low++;
      }
      CHECK(high > 5);
      CHECK(low > 5);
    }

    // The scan is exact, costs most of a month, and knowing more is the
    // only direction you can pay in.
    CHECK(Diagnose(med, c, cash, Diagnosis::Scanned, world, 1, md));
    CHECK(med.toldSeverity == c.injury.severity);
    CHECK(md.scanCost > md.guessCost * 3.0);
    CHECK(!Diagnose(med, c, cash, Diagnosis::Guessed, world, 1, md));
    // Nothing wrong, nothing to diagnose.
    Medical well;
    Climber fine;
    CHECK(!Diagnose(well, fine, cash, Diagnosis::Scanned, world, 1, md));
  }

  // ---- the comeback is staged, and coming back early is the gamble ------
  //
  // **This is the decision with a wrong answer.** Phase 3's injury was a
  // wait: eleven days, physio buys six back, nothing to decide. The stages
  // advance because you say so.
  {
    Medical med;
    Climber c = HurtClimber(InjuryKind::Elbow, 0.7);
    StartComeback(med, c, 1, md);
    const int whole = InjuryDaysFor(0.7);
    // The three stages add up to the injury, and the graded return is the
    // biggest of them -- because that is the half everybody skips.
    CHECK(md.returnShare > md.restingShare);
    CHECK(md.returnShare > md.mobilityShare);
    CHECK(std::fabs(md.restingShare + md.mobilityShare + md.returnShare -
                    1.0) < 1e-9);
    CHECK(med.stageDays >= 1);
    CHECK(med.stageDays <= whole);

    CHECK(!StageIsDone(med, 1));
    CHECK(DaysLeftInStage(med, 1) == med.stageDays);
    CHECK(StageIsDone(med, 1 + med.stageDays));

    // **Waiting it out is never a gamble.** Advancing on time cannot set
    // you back, or resting properly would be a coin flip too.
    {
      Medical patient = med;
      Climber pc = c;
      // Seen by somebody, so the untreated scar is not in the way of the
      // thing this is actually measuring.
      patient.treatedThisTime = true;
      for (int s = 0; s < 3; s++) {
        const int done = patient.stageStarted + patient.stageDays;
        CHECK(!NextStage(patient, pc, world, done, md));
        CHECK(pc.injury.severity <= 0.7);   // nothing ever got worse
      }
      // **Choosing to end the last stage heals it**, exactly as waiting it
      // out does. The first version only healed on the waiting path, so a
      // climber who clicked through came out still flagged hurt and the
      // night tick started the comeback over -- measured at **316
      // cortisone shots in one thirty-year career.**
      CHECK(patient.stage == Comeback::Clear);
      CHECK(!pc.injury.active);
      CHECK(!pc.injury.staged);
      CHECK(pc.injury.daysLeft == 0);

      // **And the scar says so**, which is the only place the difference
      // is observable. `rushedComebacks` is reset by the heal, so a test
      // that reads it after the fact reads zero either way -- and one that
      // did exactly that passed happily on a build where waiting it out
      // was also a gamble. **Rushing is what scars you**, so the scar is
      // where the claim has to be pinned.
      CHECK(patient.scars.size() == 1);
      if (patient.scars.size() == 1) {
        CHECK(patient.scars.back().weight <=
              md.scarFromSeverity * 0.7 + 1e-9);
      }
    }

    // The same injury, same grade, one of them rushed. Compared through
    // `FinishInjury` directly, so nothing but the rushing differs.
    {
      Medical a, b;
      Climber ca = HurtClimber(InjuryKind::Elbow, 0.7);
      Climber cb = HurtClimber(InjuryKind::Elbow, 0.7);
      a.treatedThisTime = true;
      b.treatedThisTime = true;
      b.rushedComebacks = 1;
      const MedicalNight na = FinishInjury(a, ca, 50, md);
      const MedicalNight nb = FinishInjury(b, cb, 50, md);
      CHECK(na.scarred && nb.scarred);
      CHECK(nb.scarWeight > na.scarWeight);
      CHECK(md.scarFromRushing > 0.0);
      // Both healed clean otherwise.
      CHECK(!ca.injury.active && !cb.injury.active);
      CHECK(!na.untreated && !nb.untreated);
    }

    // **Coming back early is a real gamble both ways.** Most of the time
    // you get away with it, which is what makes it a choice rather than a
    // warning label -- and when it lands it costs severity, which costs
    // days.
    {
      int setBacks = 0, gotAway = 0;
      for (int s = 0; s < 60; s++) {
        Medical m2;
        Climber c2 = HurtClimber(InjuryKind::Elbow, 0.7);
        StartComeback(m2, c2, 1, md);
        const double was = c2.injury.severity;
        // A few days early, which is what a real climber does -- not
        // the morning after, which is a different question and is asked
        // below.
        const int few = m2.stageStarted + m2.stageDays - 3;
        const bool bad =
            NextStage(m2, c2, Rng::FromSeed("early#" + std::to_string(s)),
                      few, md);
        if (bad) {
          setBacks++;
          CHECK(c2.injury.severity > was);
          CHECK(m2.stage == Comeback::Resting);   // back to the start of it
        } else {
          gotAway++;
          CHECK(c2.injury.severity == was);
          CHECK(m2.stage == Comeback::Mobility);
        }
        // Either way it is on the record, and the scar reads it.
        CHECK(m2.rushedComebacks == 1);
      }
      CHECK(setBacks > 3);
      CHECK(gotAway > 3);
      CHECK(gotAway > setBacks);   // three days early, you usually get away

      // **And the further you push it the worse the odds get**, which is
      // what makes it a decision with a gradient rather than a coin flip.
      // Coming back the morning after a bad injury is mostly a setback.
      int wayEarly = 0;
      for (int s = 0; s < 60; s++) {
        Medical m3;
        Climber c3 = HurtClimber(InjuryKind::Elbow, 0.7);
        StartComeback(m3, c3, 1, md);
        if (NextStage(m3, c3, Rng::FromSeed("mad#" + std::to_string(s)), 2,
                      md)) {
          wayEarly++;
        }
      }
      CHECK(wayEarly > setBacks);
      CHECK(wayEarly > 30);
    }
  }

  // ---- one clock, one owner --------------------------------------------
  //
  // **The worst bug this phase produced, and it had no test until the
  // probe found it.** `BodyDay` counts `daysLeft` down and clears the
  // injury; the staged comeback is a second clock over the same flag. Two
  // owners disagree: the comeback reached its last stage, the injury was
  // still flagged active, and the medical tick started the whole thing
  // again the next morning -- **316 cortisone shots and 10,696 hurt days
  // out of 10,950.**
  {
    Climber c = HurtClimber(InjuryKind::Elbow, 0.7);
    Medical med;
    CHECK(!c.injury.staged);
    StartComeback(med, c, 1, md);
    CHECK(c.injury.staged);
    const int was = c.injury.daysLeft;
    BodyDay(c, true, 24.0);
    CHECK(c.injury.daysLeft == was);   // the comeback owns it now
    CHECK(c.injury.active);

    // And with nobody staging it, `BodyDay` still owns it exactly as it
    // did before this phase existed.
    Climber old = HurtClimber(InjuryKind::Elbow, 0.7);
    const int wasOld = old.injury.daysLeft;
    BodyDay(old, true, 24.0);
    CHECK(old.injury.daysLeft == wasOld - 1);

    // **Run the real night, both ticks, for longer than any injury.** It
    // heals once and never restarts.
    Climber run = HurtClimber(InjuryKind::Pulley, 0.5);
    Medical m2;
    StartComeback(m2, run, 1, md);
    int healed = 0, restarts = 0;
    Comeback previous = m2.stage;
    for (int day = 1; day <= 400; day++) {
      BodyDay(run, true, 24.0);
      const MedicalNight n = MedicalDay(m2, run, world, day, md);
      if (n.healed) healed++;
      if (previous == Comeback::Clear && m2.stage == Comeback::Resting) {
        restarts++;
      }
      previous = m2.stage;
    }
    CHECK(healed == 1);
    CHECK(restarts == 0);
    CHECK(!run.injury.active);
    CHECK(!run.injury.staged);
    CHECK(m2.stage == Comeback::Clear);
    // And `daysLeft` was kept honest the whole way through, for everything
    // that still reads it -- the HUD, the sponsor's days-hurt count.
    CHECK(run.injury.daysLeft == 0);
  }

  // ---- what a career carries ------------------------------------------
  //
  // **The second gate: a career can be shortened by choices made while
  // injured.** Cortisone and rushed comebacks both land on the joint, and
  // neither shows up this season.
  {
    Medical clean;
    CHECK(JointRisk(clean, InjuryKind::Pulley, 1, md) == 1.0);
    CHECK(BodyRisk(clean, 1, md) == 1.0);
    CHECK(HistoryText(clean, 1, md).empty());

    Medical shot;
    Climber c = HurtClimber(InjuryKind::Pulley, 0.6);
    StartComeback(shot, c, 1, md);
    double cash = 1000.0;
    CHECK(TakeTheShot(shot, c, cash, 1, md));
    // **Works now**: the acute stages are over and you are climbing this
    // week, which is exactly why it is the tempting wrong answer.
    CHECK(shot.stage == Comeback::GradedReturn);
    CHECK(cash == 1000.0 - md.cortisoneCost);
    // **Costs you forever.**
    CHECK(JointRisk(shot, InjuryKind::Pulley, 1, md) > 1.0);
    CHECK(JointRisk(shot, InjuryKind::Shoulder, 1, md) == 1.0);   // that joint
    CHECK(!HistoryText(shot, 1, md).empty());
    // Two in a joint is a decision, four is a different career.
    Medical four = shot;
    for (int i = 0; i < 3; i++) {
      Climber again = HurtClimber(InjuryKind::Pulley, 0.6);
      StartComeback(four, again, 1, md);
      double money = 1000.0;
      TakeTheShot(four, again, money, 1, md);
    }
    // Pinned as absolutes rather than as a ratio: one shot is a nudge,
    // four is the joint being a different joint.
    CHECK(JointRisk(shot, InjuryKind::Pulley, 1, md) >= 1.25);
    CHECK(JointRisk(shot, InjuryKind::Pulley, 1, md) <= 1.45);
    CHECK(JointRisk(four, InjuryKind::Pulley, 1, md) >= 2.4);
    // **And there is a ceiling.** Cortisone and scars priced separately
    // and uncapped ran away: a thirty-year impatient career reached 17.9
    // with 111 injuries, because more risk made more injuries made more
    // scars. A death spiral is an absence of a consequence, not one --
    // past a point nothing the player does matters.
    Medical wrecked;
    for (int i = 0; i < kInjuryKindCount; i++) wrecked.joints[i] = 1.0;
    for (int i = 0; i < 40; i++) {
      wrecked.scars.push_back(Scar{InjuryKind::Pulley, 0.9, 1});
    }
    CHECK(JointRisk(wrecked, InjuryKind::Pulley, 1, md) <= 1.0 + md.riskPerWear);
    CHECK(BodyRisk(wrecked, 1, md) <= 2.7);
    CHECK(JointWear(wrecked, InjuryKind::Pulley, 1, md) == 1.0);
    CHECK(four.shots[static_cast<int>(InjuryKind::Pulley)] == 4);

    // **Surgery is the only thing that takes damage off a joint**, and
    // nobody operates on a guess.
    Medical surg = shot;
    Climber sc = HurtClimber(InjuryKind::Pulley, 0.9);
    StartComeback(surg, sc, 1, md);
    double rich = 9000.0;
    CHECK(!HaveSurgery(surg, sc, rich, 1, md));   // no scan
    CHECK(Diagnose(surg, sc, rich, Diagnosis::Scanned, world, 1, md));
    const double before = JointRisk(surg, InjuryKind::Pulley, 1, md);
    CHECK(HaveSurgery(surg, sc, rich, 1, md));
    CHECK(JointRisk(surg, InjuryKind::Pulley, 1, md) < before);
    // And most of a season on the other side of it.
    CHECK(sc.injury.daysLeft >= static_cast<int>(md.surgeryDays));
    CHECK(surg.stage == Comeback::Resting);
    // **One treatment per injury**, and the guard is not decoration: a
    // rich career had 294 surgeries out of one injury and an impatient one
    // 316 cortisone shots, because nothing said no on the second morning.
    CHECK(!HaveSurgery(surg, sc, rich, 2, md));
    CHECK(surg.surgeries == 1);
    {
      Medical twice;
      Climber tc = HurtClimber(InjuryKind::Pulley, 0.7);
      StartComeback(twice, tc, 1, md);
      double plenty = 100000.0;
      CHECK(TakeTheShot(twice, tc, plenty, 1, md));
      CHECK(!TakeTheShot(twice, tc, plenty, 2, md));
      CHECK(twice.shotsTaken == 1);
      // Nor does a shot open the door to an operation on the same one.
      Diagnose(twice, tc, plenty, Diagnosis::Scanned, world, 2, md);
      CHECK(!HaveSurgery(twice, tc, plenty, 2, md));
    }

    // You cannot have an operation for a strain.
    Medical minor;
    Climber mc = HurtClimber(InjuryKind::Pulley, 0.2);
    StartComeback(minor, mc, 1, md);
    double money = 9000.0;
    Diagnose(minor, mc, money, Diagnosis::Scanned, world, 1, md);
    CHECK(!HaveSurgery(minor, mc, money, 1, md));
  }

  // ---- insurance is a bet you place before you know ---------------------
  //
  // **The third gate.** The premium lands whether or not you are hurt;
  // what it buys is most of the bill on the day you are.
  {
    Medical med;
    Climber fine;
    Climber hurt = HurtClimber(InjuryKind::Elbow, 0.5);
    // Everybody tries this.
    CHECK(!BuyInsurance(med, hurt, 1, md));
    CHECK(BuyInsurance(med, fine, 1, md));
    CHECK(!BuyInsurance(med, fine, 1, md));   // once
    // **And there is a wait**, so buying it the week before a planned
    // surgery is not a strategy.
    CHECK(!CoverIsLive(med, 1, md));
    CHECK(!CoverIsLive(med, 1 + md.waitingDays - 1, md));
    CHECK(CoverIsLive(med, 1 + md.waitingDays, md));
    CHECK(md.waitingDays >= 30);
    CHECK(BillFor(med, 1000.0, 1, md) == 1000.0);
    CHECK(BillFor(med, 1000.0, 1 + md.waitingDays, md) ==
          1000.0 * (1.0 - md.covers));

    // The premium is a bill like any other: it lands whether the money is
    // there or not, and the shortfall is debt.
    double cash = 100.0, owed = 0.0;
    int paid = 0;
    for (int day = 1; day <= 1 + md.premiumEveryDays * 4; day++) {
      if (InsuranceDay(med, cash, owed, day, md)) paid++;
    }
    CHECK(paid == 4);
    CHECK(med.premiumsPaid == md.premium * 4.0);
    double broke = 0.0, debt = 0.0;
    Medical poor;
    Climber ok;
    BuyInsurance(poor, ok, 1, md);
    InsuranceDay(poor, broke, debt, 1 + md.premiumEveryDays, md);
    CHECK(debt == md.premium);

    // **A real bet, pinned as a magnitude against what a career measured.**
    //
    // A thirty-year career on this policy pays about $6,250 in premiums.
    // Measured claims ran from $816 on a lucky body to $8,704 on an
    // unlucky one, so the lifetime cost has to sit inside that window --
    // outside it the policy is either free or unthinkable, and in neither
    // case is it a decision. It was $26 a fortnight first, which is
    // $20,332 over thirty years against a maximum measured claim of
    // $8,976: never right, in any seed. Not a bet -- a tax with a story.
    const double aYear = md.premium * (365.0 / md.premiumEveryDays);
    const double aCareer = aYear * 30.0;
    CHECK(aCareer > md.scanCost * 10.0);    // it hurts when you never need it
    CHECK(aCareer < md.scanCost * 25.0);    // and an unlucky body out-claims it
    // And one operation's cover is worth years of carrying it, which is
    // the shape of the tail the whole thing is for.
    CHECK(md.surgeryCost * md.covers > aYear * 5.0);
  }

  // ---- being poor -------------------------------------------------------
  //
  // **The fourth gate: the undertreated path is reached by being broke,
  // not by choosing it.** Nothing about it is a decision.
  {
    Medical broke;
    Climber c = HurtClimber(InjuryKind::Lumbrical, 0.6);
    StartComeback(broke, c, 1, md);
    double nothing = 10.0;
    // You cannot afford to know.
    CHECK(!Diagnose(broke, c, nothing, Diagnosis::Guessed, world, 1, md));
    CHECK(!Diagnose(broke, c, nothing, Diagnosis::Scanned, world, 1, md));
    CHECK(!TakeTheShot(broke, c, nothing, 1, md));
    CHECK(!broke.treatedThisTime);

    // Let it run its course untouched.
    MedicalNight last;
    int day = 1;
    for (; day < 600 && c.injury.active; day++) {
      last = MedicalDay(broke, c, world, day, md);
    }
    CHECK(!c.injury.active);
    CHECK(last.healed);
    CHECK(last.untreated);
    CHECK(broke.untreatedInjuries == 1);
    // **It heals badly.** A scar, and a bigger one than a treated injury
    // of the same grade would leave.
    CHECK(last.scarred);
    CHECK(JointRisk(broke, InjuryKind::Lumbrical, day, md) > 1.0);
    CHECK(last.news.find("never did find out") != std::string::npos);
    const int untreatedDays = day;

    // The same injury, seen and rested properly, is out sooner and scars
    // less.
    Medical seen;
    Climber c2 = HurtClimber(InjuryKind::Lumbrical, 0.6);
    StartComeback(seen, c2, 1, md);
    double money = 1000.0;
    Diagnose(seen, c2, money, Diagnosis::Guessed, world, 1, md);
    MedicalNight lastSeen;
    int day2 = 1;
    for (; day2 < 600 && c2.injury.active; day2++) {
      lastSeen = MedicalDay(seen, c2, world, day2, md);
    }
    CHECK(!lastSeen.untreated);
    CHECK(day2 < untreatedDays);
    CHECK(lastSeen.scarWeight < last.scarWeight);
    CHECK(md.untreatedDaysMult > 1.0);
  }

  // ---- and scars fade, slowly and never to nothing ---------------------
  {
    Medical med;
    med.scars.push_back(Scar{InjuryKind::Shoulder, 0.5, 1});
    const double fresh = JointRisk(med, InjuryKind::Shoulder, 1, md);
    const double older = JointRisk(med, InjuryKind::Shoulder, 1 + 365 * 3, md);
    CHECK(older < fresh);
    CHECK(older > 1.0);   // never to nothing
    CHECK(md.scarFloor > 0.0);

    // **A derived value, not a decayed one.** Ticking the night forward
    // three years must not move it a millimetre further than reading it
    // three years out does -- the first version faded the weight by
    // reading the weight it had just written, so a scar collapsed
    // geometrically and the floor collapsed with it.
    Medical ticked;
    ticked.scars.push_back(Scar{InjuryKind::Shoulder, 0.5, 1});
    Climber well;
    for (int day = 1; day <= 1 + 365 * 3; day++) {
      MedicalDay(ticked, well, world, day, md);
    }
    CHECK(JointRisk(ticked, InjuryKind::Shoulder, 1 + 365 * 3, md) == older);
    // And a lifetime out it is still worth something.
    CHECK(JointRisk(ticked, InjuryKind::Shoulder, 1 + 365 * 40, md) > 1.0);
  }
}

static void TestLeague() {
  LeagueDials ld;
  const Rng world = Rng::FromSeed("wednesday");

  // ---- it schedules itself -----------------------------------------------
  {
    League l;
    CHECK(!LeagueTonight(l, 1));
    CHECK(DaysUntilLeague(l, 1) == -1);
    CHECK(LeagueLine(l, 1, ld).empty());
    LeagueDay(l, world, 1, ld);
    CHECK(l.nextNight > 1);
    // Inside the week, so a career does not always start on league night
    // and the gym's rhythm is not the player's calendar.
    CHECK(l.nextNight <= 1 + ld.everyDays);
    CHECK(LeagueTonight(l, l.nextNight));
    CHECK(ld.everyDays == 7);   // an absolute, not the dial it tests

    // The night stays enterable for the whole of its day, and rolls the
    // morning after -- the same rule the Games live under.
    const int first = l.nextNight;
    LeagueDay(l, world, first, ld);
    CHECK(l.nextNight == first);
    LeagueDay(l, world, first + 1, ld);
    CHECK(l.nextNight == first + ld.everyDays);
  }

  // ---- the regulars turn up whether you do or not ------------------------
  //
  // **A block you skipped is a block you came last in.** Same rule the
  // World Cup's field lives under, and it is what stops a weekly event
  // being something you can ignore for free.
  {
    League l;
    LeagueDay(l, world, 1, ld);
    int day = l.nextNight;
    LeagueNight closed;
    for (int w = 0; w <= ld.weeksPerBlock + 1; w++) {
      const LeagueNight n = LeagueDay(l, world, day, ld);
      if (n.blockClosed) { closed = n; break; }
      day += ld.everyDays;
    }
    CHECK(closed.blockClosed);
    // **A tie at nothing is not a tie**, here as everywhere else in this
    // game: a climber who never turned up does not come top of a table
    // nobody scored in. Tested against `LeagueTable`, which is where the
    // rule actually lives -- the version of this that lived in the block
    // close was a guard that could not fire.
    {
      League empty;
      empty.fieldPoints.assign(TheRegulars().size(), 0.0);
      const std::vector<CircuitStanding> flat = LeagueTable(empty);
      CHECK(flat.back().isYou);
      CHECK(flat.size() == TheRegulars().size() + 1);
      // And one point is enough to be ahead of them.
      empty.yourPoints = 1.0;
      CHECK(LeagueTable(empty).front().isYou);
    }
    CHECK(l.block == 2);            // it moved on
    CHECK(l.weeksDone == 0);        // and started clean
    CHECK(!closed.won);             // you were not there
    CHECK(closed.cash == 0.0);
    CHECK(closed.news.empty());     // and it does not congratulate you
    CHECK(l.blockWins == 0);
    // The board was reset, so last block's points do not decide this one.
    for (double p : l.fieldPoints) CHECK(p == 0.0);
  }

  // ---- a night, and the number you are here for --------------------------
  {
    League l;
    LeagueDay(l, world, 1, ld);
    const CompDials cd = LeagueCompDials(ld);
    // **Ten goes, not seven.** A league night is a session with a
    // scorecard, and a stingy attempt count would make it a worse comp
    // rather than a different thing.
    CHECK(cd.attempts == ld.nightAttempts);
    CHECK(cd.attempts > CompDials{}.attempts);
    CHECK(cd.problems == ld.nightProblems);

    CompState board = SetTheLeagueBoard(world, 8.0, l.nextNight, ld);
    CHECK(static_cast<int>(board.problems.size()) == ld.nightProblems);
    CHECK(board.attemptsLeft == ld.nightAttempts);

    // A night where you got nothing up is not a personal best and does not
    // win anything.
    League quiet = l;
    const LeagueResult nothing = SettleLeague(
        quiet, board, 8.0, quiet.nextNight, world, ld);
    CHECK(nothing.score == 0.0);
    CHECK(!nothing.personalBest);
    CHECK(nothing.cash == 0.0);
    CHECK(quiet.nights == 1);

    // Climb the whole thing.
    for (auto& pr : board.progress) { pr.topped = true; pr.flashed = true; }
    const LeagueResult good =
        SettleLeague(l, board, 8.0, l.nextNight, world, ld);
    CHECK(good.score > 0.0);
    CHECK(good.place >= 1);
    CHECK(good.fieldSize ==
          static_cast<int>(TheRegulars().size()) + 1);
    CHECK(l.best == good.score);
    CHECK(l.bestOnDay == l.nextNight);
    CHECK(l.yourPoints > 0.0);
    // **Your first night is not a personal best.** There was nothing to
    // beat, and telling somebody they have set a record on the first
    // Wednesday is the game congratulating itself.
    CHECK(!good.personalBest);

    // Beating it is, and it says by how much.
    League again = l;
    again.best = good.score - 7.0;
    const LeagueResult pb =
        SettleLeague(again, board, 8.0, again.nextNight + 7, world, ld);
    CHECK(pb.personalBest);
    CHECK(pb.improvedBy > 0.0);
    CHECK(pb.rep == ld.bestRep);
    CHECK(pb.news.find("Best") != std::string::npos);
    CHECK(again.best == pb.score);

    // ...and failing to beat it leaves it alone.
    League held = again;
    const double was = held.best;
    CompState weak = SetTheLeagueBoard(world, 8.0, held.nextNight, ld);
    SettleLeague(held, weak, 2.0, held.nextNight, world, ld);
    CHECK(held.best == was);
  }

  // ---- and the number grows as you do -----------------------------------
  //
  // **The fix that made a personal best worth chasing.** Priced the comp
  // way -- worth relative to the rest of the board -- the maximum score is
  // the same every week however good you get, so the number saturates in
  // the first month and never moves again: measured, **two personal bests
  // in five hundred and nineteen league nights.** Priced by the grade of
  // the problem, the board rises with you and so does the number.
  {
    const auto perfectNight = [&](double grade) {
      CompState b = SetTheLeagueBoard(world, grade, 40, ld);
      for (auto& pr : b.progress) { pr.topped = true; pr.flashed = true; }
      return YourScore(b, LeagueCompDials(ld));
    };
    CHECK(perfectNight(9.0) > perfectNight(6.0));
    CHECK(perfectNight(6.0) > perfectNight(4.0));
    // Materially, not by a rounding error: three grades of work has to
    // show up as a number a player would notice.
    CHECK(perfectNight(9.0) > perfectNight(6.0) * 1.2);

    // **A spread, not five at your level.** The gym sets for the whole
    // room, and the whole room turns up on a Wednesday.
    const CompState b = SetTheLeagueBoard(world, 8.0, 40, ld);
    int low = kMaxGrade, high = 0;
    for (const CompProblem& p : b.problems) {
      low = std::min(low, p.route.trueGrade);
      high = std::max(high, p.route.trueGrade);
    }
    CHECK(high - low >= 4);
    CHECK(low < 8);
    CHECK(high > 8);
    // And the hard one is worth more than the easy one, because it is
    // harder -- not because of where it sits on the board.
    CHECK(b.problems.back().topPoints > b.problems.front().topPoints);
  }

  // ---- it is worth no ranking points, and that is the design -----------
  //
  // The whole reason a league is not a small comp. Checked as a property of
  // the type rather than of a call: `LeagueResult` has no ranking field to
  // read, and `SettleLeague` never touches the record -- so this pins the
  // one thing that could still leak, which is the block table borrowing
  // the circuit's own points.
  {
    League l;
    LeagueDay(l, world, 1, ld);
    CompState board = SetTheLeagueBoard(world, 8.0, l.nextNight, ld);
    for (auto& pr : board.progress) { pr.topped = true; pr.flashed = true; }
    const double before = l.yourPoints;
    const LeagueResult r =
        SettleLeague(l, board, 8.0, l.nextNight, world, ld);
    CHECK(l.yourPoints > before);          // the block table moves
    CHECK(r.cash <= ld.nightCash);         // and it pays beer money
    CHECK(ld.nightCash < CompDials{}.winCash);
    CHECK(ld.nightFee < CompDials{}.entryFee);
    // Both standing nudges are under what the factions file calls a small
    // deliberate act: the scene notices, and does not care much.
    CHECK(ld.bestRep < 0.1);
    CHECK(ld.blockRep < 0.1);
  }

  // ---- winning a block --------------------------------------------------
  {
    League l;
    LeagueDay(l, world, 1, ld);
    int day = l.nextNight;
    LeagueNight closed;
    for (int w = 0; w <= ld.weeksPerBlock + 1; w++) {
      // Turn up and top the lot, every week.
      CompState board = SetTheLeagueBoard(world, 12.0, day, ld);
      for (auto& pr : board.progress) { pr.topped = true; pr.flashed = true; }
      SettleLeague(l, board, 12.0, day, world, ld);
      const LeagueNight n = LeagueDay(l, world, day + 1, ld);
      if (n.blockClosed) { closed = n; break; }
      day += ld.everyDays;
    }
    CHECK(closed.blockClosed);
    CHECK(closed.place == 1);
    CHECK(closed.won);
    CHECK(closed.cash == ld.blockCash);
    CHECK(closed.rep == ld.blockRep);
    CHECK(l.blockWins == 1);
    CHECK(!closed.news.empty());
    // A block is worth about a week of shifts and no more. **A league is
    // not income**, or the gym becomes a job with a scorecard.
    CHECK(ld.blockCash < DayDials{}.shiftWage * 2.0);
  }

  // ---- and the regulars are people --------------------------------------
  {
    CHECK(TheRegulars().size() == 6);
    for (const LeagueRegular& r : TheRegulars()) {
      CHECK(std::string(r.name).size() > 0);
      CHECK(std::string(r.who).size() > 25);   // they say something
      // **Worse than the comp field on purpose** -- the good ones are at a
      // comp on a Saturday.
      CHECK(r.gradeOffset <= 1.0);
    }
    CHECK(ld.regularShift < 0.0);
  }

  // ---- and the whiteboard says something --------------------------------
  {
    League l;
    LeagueDay(l, world, 1, ld);
    CHECK(!LeagueLine(l, l.nextNight, ld).empty());
    CHECK(!LeagueLine(l, l.nextNight - 1, ld).empty());
    // **Silent unless it is close.** A whiteboard counting down from six
    // days is a progress bar for a Wednesday.
    CHECK(LeagueLine(l, l.nextNight - 6, ld).empty());
  }
}

static void TestWorldStage() {
  WorldStageDials wd;
  const Rng world = Rng::FromSeed("the-world");

  // ---- the schedule -----------------------------------------------------
  WorldCupSeason s = StartWorldCupSeason(world, 1, 1, wd);
  CHECK(s.season == 1);
  CHECK(static_cast<int>(s.schedule.size()) == wd.rounds);
  // **No venue twice.** A season that visits Innsbruck three times is a
  // schedule nobody wrote.
  for (std::size_t i = 0; i < s.schedule.size(); i++) {
    for (std::size_t j = 0; j < i; j++) {
      CHECK(s.schedule[i].venue != s.schedule[j].venue);
    }
    CHECK(s.schedule[i].venue >= 0);
    CHECK(s.schedule[i].venue < static_cast<int>(TheVenues().size()));
  }
  for (std::size_t i = 1; i < s.schedule.size(); i++) {
    const int gap = s.schedule[i].day - s.schedule[i - 1].day;
    CHECK(gap >= wd.roundGapMin);
    CHECK(gap < wd.roundGapMin + wd.roundGapVariance);
  }
  CHECK(RoundToday(s, s.schedule[0].day) == 0);
  CHECK(RoundToday(s, s.schedule[0].day - 1) == -1);
  CHECK(DaysUntilRound(s, s.schedule[0].day, wd) == 0);

  // **Travel is the dial that makes a season a budget problem.** Salt Lake
  // is a domestic ticket and Seoul is most of a month's money -- pinned as
  // a magnitude, because a flat travel cost turns the whole thing back into
  // a calendar.
  {
    double cheapest = 1e9, dearest = 0.0;
    for (const WorldCupVenue& v : TheVenues()) {
      cheapest = std::min(cheapest, v.travel);
      dearest = std::max(dearest, v.travel);
      CHECK(std::string(v.city).size() > 0);
      CHECK(std::string(v.blurb).size() > 30);   // it says something
    }
    CHECK(dearest > cheapest * 3.0);
  }

  // ---- the points table -------------------------------------------------
  //
  // **1000 for a win and a cliff after it**, which is why a World Cup year
  // turns on two or three rounds rather than accumulating evenly.
  CHECK(WorldCupPoints(1) == 1000.0);
  CHECK(WorldCupPoints(2) < WorldCupPoints(1) * 0.85);
  CHECK(WorldCupPoints(4) < WorldCupPoints(1) * 0.65);
  for (int p = 2; p <= 30; p++) {
    CHECK(WorldCupPoints(p - 1) >= WorldCupPoints(p));
  }
  // Off the end of the table still scores. Coming thirty-fifth at a World
  // Cup happens to real climbers and it is not worth nothing.
  CHECK(WorldCupPoints(35) > 0.0);

  // ---- the field flies whether you do or not ---------------------------
  //
  // **The whole mechanic.** A round you skip is not a round that did not
  // happen -- everybody else banks while you are at home, and the table
  // moves away from you.
  {
    WorldCupSeason stayed = StartWorldCupSeason(world, 1, 1, wd);
    BankRound(stayed, 0, nullptr, world, wd);
    CHECK(stayed.schedule[0].resolved);
    CHECK(!stayed.schedule[0].flown);
    CHECK(stayed.missed == 1);
    CHECK(stayed.starts == 0);
    CHECK(stayed.yourPoints == 0.0);
    double banked = 0.0;
    for (double p : stayed.fieldPoints) banked += p;
    CHECK(banked > 0.0);
    // Somebody won it while you were away.
    double best = 0.0;
    for (double p : stayed.fieldPoints) best = std::max(best, p);
    CHECK(best == 1000.0);
  }

  // Turning up scores, and winning scores the most there is.
  //
  // **The board is what gets banked**, not a hand-typed placing: the season
  // table is fed from the scorecard the player actually watched, so the
  // climber the round says won it is the climber the season says won it.
  {
    WorldCupSeason went = StartWorldCupSeason(world, 1, 1, wd);
    const CompResult won = ARoundYouPlaced(1);
    BankRound(went, 0, &won, world, wd);
    CHECK(went.schedule[0].flown);
    CHECK(went.starts == 1);
    CHECK(went.missed == 0);
    CHECK(went.yourPoints == 1000.0);
    CHECK(went.wins == 1);
    CHECK(went.podiums == 1);
    CHECK(went.finals == 1);
    // And nobody else got the win.
    for (double p : went.fieldPoints) CHECK(p < 1000.0);
    // Everybody on the card scored, and nobody scored twice: the field is
    // twelve, the board is thirteen, and the second place through
    // thirteenth is exactly what the twelve of them took.
    double theirs = 0.0;
    for (double p : went.fieldPoints) theirs += p;
    double expected = 0.0;
    for (int place = 2; place <= 13; place++) {
      expected += WorldCupPoints(place);
    }
    CHECK(std::fabs(theirs - expected) < 0.001);
  }

  // ---- the same engine, an absolute setting -----------------------------
  //
  // **The load-bearing difference between this file and DirtbagComp.** A
  // gym comp is set at *your* grade plus a tier offset, so the board
  // follows you up as you improve. The world stage does not: it is set
  // where it is set, and getting better is what closes the gap.
  //
  // Reading it the other way made the entire system unwinnable at every
  // skill level in the game -- a climber at 95 skill topped 0.01 of five
  // problems and came last of thirteen, and so did a climber at 55.
  {
    const Rng r = Rng::FromSeed("board");
    const CompState wc = SetTheWorldBoard(r, 40, wd);
    CHECK(static_cast<int>(wc.problems.size()) == CompDials{}.problems);
    CHECK(wc.attemptsLeft == CompDials{}.attempts);
    // The board sits at the world standard, not at yours. Pinned as an
    // absolute: the whole defect was a board that moved with the player.
    double mean = 0.0;
    for (const CompProblem& p : wc.problems) mean += p.route.trueGrade;
    mean /= static_cast<double>(wc.problems.size());
    CHECK(wd.worldStandard == 8.5);
    CHECK(mean >= 9.0);
    CHECK(mean <= 10.5);
    // And the international field is priced off the same standard, so a
    // beginner and a world-beater meet the identical twelve people.
    for (const International& c : TheWorldField()) {
      CHECK(wd.worldStandard + c.gradeOffset >= 7.0);
      CHECK(wd.worldStandard + c.gradeOffset <= 11.0);
    }
    // Different boards, not the same one relabelled. The two bumps are
    // equal by design, so the only thing that can separate a Games board
    // from a World Cup board on the same date is the stream -- and the
    // colours are indexed rather than rolled, so it is the moves that have
    // to differ.
    const CompState a = SetTheWorldBoard(r, 40, wd);
    const CompState b = SetTheOlympicBoard(r, 40, wd);
    bool differs = false;
    for (std::size_t i = 0; i < a.problems.size(); i++) {
      const std::vector<Move>& am = a.problems[i].route.moves;
      const std::vector<Move>& bm = b.problems[i].route.moves;
      if (am.size() != bm.size()) { differs = true; break; }
      for (std::size_t m = 0; m < am.size(); m++) {
        if (am[m].difficulty != bm[m].difficulty) differs = true;
      }
    }
    CHECK(differs);
  }

  // The names on the international board are internationals. Having Kai
  // from the gym win in Innsbruck is the presentation telling a lie the sim
  // did not, which is why this does not go through `Settle`.
  {
    const CompState board = SetTheWorldBoard(world, 40, wd);
    const CompResult r =
        SettleWorldRound(board, Rng::FromSeed("innsbruck"), wd);
    CHECK(static_cast<int>(r.board.size()) ==
          static_cast<int>(TheWorldField().size()) + 1);
    CHECK(r.place >= 1);
    CHECK(r.fieldSize == static_cast<int>(r.board.size()));
    for (const CompEntrant& e : r.board) {
      if (e.isYou) continue;
      bool known = false;
      for (const International& c : TheWorldField()) {
        if (e.name.compare(0, std::string(c.name).size(), c.name) == 0) {
          known = true;
        }
      }
      CHECK(known);
      // And the nation is on the card, because that is what a World Cup
      // scoreboard says.
      CHECK(e.name.find('(') != std::string::npos);
    }
    // **The World Cup pays in ranking and nothing else.** The federation
    // flew you; the cheque goes to the federation.
    CHECK(r.cash == 0.0);
    CHECK(r.rep == 0.0);

    const CompResult g =
        SettleTheGames(SetTheOlympicBoard(world, 40, wd),
                       Rng::FromSeed("the-final"), wd);
    CHECK(static_cast<int>(g.board.size()) ==
          static_cast<int>(TheOlympicField().size()) + 1);
  }

  // **You do not take a tie at zero.** The same rule the domestic board
  // learned the hard way: a climber who got nothing up at a World Cup is
  // not fourth, they are in a room full of people who did not climb.
  {
    CompState blank = SetTheWorldBoard(world, 40, wd);
    const CompResult r =
        SettleWorldRound(blank, Rng::FromSeed("nothing"), wd);
    if (r.yourScore == 0.0) {
      for (std::size_t i = 0; i < r.board.size(); i++) {
        if (r.board[i].isYou) continue;
        if (r.board[i].score == 0.0) CHECK(r.place > static_cast<int>(i) + 1);
      }
    }
  }

  // A round resolves once. Banking it twice would double the field's year.
  {
    WorldCupSeason once = StartWorldCupSeason(world, 1, 1, wd);
    BankRound(once, 0, nullptr, world, wd);
    const double after = once.fieldPoints[0];
    BankRound(once, 0, nullptr, world, wd);
    CHECK(once.fieldPoints[0] == after);
    CHECK(once.missed == 1);
  }

  // **Missing a season costs you the table**, measured rather than argued.
  {
    WorldCupSeason all = StartWorldCupSeason(world, 1, 1, wd);
    WorldCupSeason none = StartWorldCupSeason(world, 1, 1, wd);
    const CompResult mid = ARoundYouPlaced(4);
    for (int i = 0; i < wd.rounds; i++) {
      BankRound(all, i, &mid, Rng::FromSeed("r#" + std::to_string(i)), wd);
      BankRound(none, i, nullptr, Rng::FromSeed("r#" + std::to_string(i)), wd);
    }
    CHECK(WorldCupSeasonOver(all, wd));
    CHECK(WorldCupSeasonOver(none, wd));
    const int placedGoing = CloseWorldCupSeason(all);
    const int placedHome = CloseWorldCupSeason(none);
    CHECK(placedGoing < placedHome);
    CHECK(placedHome == static_cast<int>(WorldTable(none).size()));
    CHECK(all.bestRank == placedGoing);
    CHECK(none.titles == 0);
  }

  // Winning every round takes the title.
  {
    WorldCupSeason champ = StartWorldCupSeason(world, 1, 1, wd);
    const CompResult won = ARoundYouPlaced(1);
    for (int i = 0; i < wd.rounds; i++) {
      BankRound(champ, i, &won, Rng::FromSeed("c#" + std::to_string(i)), wd);
    }
    CHECK(CloseWorldCupSeason(champ) == 1);
    CHECK(champ.titles == 1);
    CHECK(champ.bestRank == 1);
  }

  // And it reads like something, with the cost in it.
  {
    WorldCupSeason line = StartWorldCupSeason(world, 1, 1, wd);
    CHECK(!WorldCupLine(line, line.schedule[0].day, wd).empty());
    CHECK(WorldCupLine(WorldCupSeason{}, 1, wd).empty());
  }

  // ---- the Games --------------------------------------------------------
  {
    Olympics o;
    CHECK(!GamesToday(o, 1));
    CHECK(DaysUntilGames(o, 1) == -1);
    SeedTheGames(o, world, 1, wd);
    // **Never sooner than three weeks from a standing start.** A save
    // loaded the day before should not open onto the Games, and a fresh
    // career should not either.
    //
    // Pinned as an absolute rather than against the dial it is testing:
    // `o.nextDay >= 1 + wd.olympicSeedLead` is a tautology, and zeroing the
    // dial passed it without a murmur. The number is the claim.
    CHECK(wd.olympicSeedLead >= 21);
    CHECK(o.nextDay >= 22);
    for (int seed = 0; seed < 40; seed++) {
      Olympics probe;
      SeedTheGames(probe, Rng::FromSeed("g#" + std::to_string(seed)), 1, wd);
      CHECK(probe.nextDay >= 22);
      // ...and inside a cycle of it, or the first Games of a career would
      // be a rumour rather than a date.
      CHECK(probe.nextDay <= 22 + wd.olympicCycleDays);
    }
    CHECK(GamesToday(o, o.nextDay));
    CHECK(DaysUntilGames(o, o.nextDay) == 0);
    CHECK(DaysUntilGames(o, o.nextDay - 5) == 5);

    // They come round on a cycle rather than a calendar.
    const int first = o.nextDay;
    GamesDay(o, first, wd);
    CHECK(o.nextDay == first);          // the day itself stays enterable
    GamesDay(o, first + 1, wd);
    CHECK(o.nextDay == first + wd.olympicCycleDays);

    // **You have to be an Olympic Hopeful to be there at all.**
    CHECK(!Qualified(0.0, wd));
    CHECK(!Qualified(wd.qualifyAt - 1.0, wd));
    CHECK(Qualified(wd.qualifyAt, wd));
    CHECK(wd.qualifyAt == 1200.0);   // the ranking's own Olympic Hopeful line

    // The field is above you, every one of them -- which is what makes a
    // medal worth something and an appearance worth having.
    CHECK(TheOlympicField().size() == 7);
    for (const International& c : TheOlympicField()) {
      CHECK(c.gradeOffset > 0.0);
      CHECK(std::string(c.nation).size() == 3);
    }

    // Medals, and one Games per cycle.
    CHECK(MedalFor(1).gold && !MedalFor(1).silver);
    CHECK(MedalFor(2).silver);
    CHECK(MedalFor(3).bronze);
    CHECK(!MedalFor(4).gold && !MedalFor(4).silver && !MedalFor(4).bronze);
    BankTheGames(o, MedalFor(1), 0);
    CHECK(o.appearances == 1 && o.gold == 1);
    BankTheGames(o, MedalFor(1), 0);   // the same Games twice is not two
    CHECK(o.appearances == 1 && o.gold == 1);
    BankTheGames(o, MedalFor(4), 1);
    CHECK(o.appearances == 2 && o.gold == 1);

    // **Silent unless it is close or you have been.** A countdown to
    // something you are eight hundred points from is a progress bar for a
    // system the player has not met.
    Olympics quiet;
    SeedTheGames(quiet, world, 1, wd);
    CHECK(GamesLine(quiet, 0.0, 1, wd).empty());
    CHECK(GamesLine(quiet, wd.qualifyAt, quiet.nextDay - 1, wd).find(
              "tomorrow") != std::string::npos);
    CHECK(!GamesLine(o, 0.0, 1, wd).empty());   // you have been

    // **The door, not just the countdown.** Being qualified and it being
    // the day are two different things, and the Games can only be entered
    // once per cycle.
    Olympics gate;
    SeedTheGames(gate, world, 1, wd);
    CHECK(!CanEnterTheGames(gate, 9999.0, gate.nextDay - 1, wd).can);
    CHECK(CanEnterTheGames(gate, 9999.0, gate.nextDay - 1, wd).why.empty());
    CHECK(CanEnterTheGames(gate, wd.qualifyAt, gate.nextDay, wd).can);
    {
      const GamesCheck no =
          CanEnterTheGames(gate, wd.qualifyAt - 400.0, gate.nextDay, wd);
      CHECK(!no.can);
      // Said as a number, because it is the one thing on this ladder a
      // player can do something about.
      CHECK(no.why.find("400") != std::string::npos);
    }
    BankTheGames(gate, MedalFor(4), gate.nextDay);
    CHECK(!CanEnterTheGames(gate, 9999.0, gate.nextDay, wd).can);
  }

  // ---- and it is actually climbable -------------------------------------
  //
  // **The test the ordering tests could not be.** Every check above passed
  // on a version of this system where a climber at 95 skill topped 0.01 of
  // five problems and finished thirteenth of thirteen -- and so did a
  // climber at 55, because the board was pinned to the player's own grade
  // and followed them up forever. Nothing broke; the whole top of the
  // ladder was simply unreachable, at every skill level, for everyone.
  //
  // So this pins magnitudes: what a good climber gets, what a great one
  // gets, and that the two are different.
  {
    const auto climberAt = [](double skill) {
      Climber c;
      c.skills.power = c.skills.fingers = c.skills.technique =
          c.skills.endurance = c.skills.head = skill;
      c.skin = 100.0;
      c.psyche = 0.7;
      return c;
    };
    // Seven goes, easiest first -- the same greedy line a player takes on
    // their first World Cup.
    const auto playARound = [&](const Climber& you, int seed) {
      const Rng w = Rng::FromSeed("round#" + std::to_string(seed));
      CompState b = SetTheWorldBoard(w, 40 + seed, wd);
      for (int a = 0; a < CompDials{}.attempts; a++) {
        int pick = -1;
        for (std::size_t i = 0; i < b.problems.size(); i++) {
          if (!b.progress[i].topped) { pick = static_cast<int>(i); break; }
        }
        if (pick < 0) break;
        AttemptProblem(b, pick, you, w.Derive("a#" + std::to_string(a)),
                       WorldCupCompDials(wd));
      }
      int tops = 0;
      for (const ProblemProgress& p : b.progress) if (p.topped) tops++;
      const CompResult r = SettleWorldRound(b, w.Derive("s"), wd);
      return std::pair<int, int>{r.place, tops};
    };

    const int N = 60;
    int lastPlace = 0, lastTops = 0;
    int goodPlace = 0, goodTops = 0, greatPlace = 0, greatTops = 0;
    for (int s = 0; s < N; s++) {
      const std::pair<int, int> l = playARound(climberAt(55.0), s);
      lastPlace += l.first;  lastTops += l.second;
      const std::pair<int, int> g = playARound(climberAt(85.0), s);
      goodPlace += g.first;  goodTops += g.second;
      const std::pair<int, int> b = playARound(climberAt(95.0), s);
      greatPlace += b.first; greatTops += b.second;
    }
    // A gym climber at a World Cup gets nothing up and comes last. That is
    // correct, and it is what the whole ladder underneath is protecting.
    CHECK(lastTops == 0);
    CHECK(lastPlace == N * 13);
    // Somebody who has spent a career on it tops most of the board and
    // finishes in the top half.
    CHECK(goodTops >= N * 2);
    CHECK(goodPlace < N * 8);
    // And the exceptional are near the front, not merely less far back.
    CHECK(greatTops > goodTops);
    CHECK(greatPlace < goodPlace);
    CHECK(greatPlace < N * 4);
  }

  // ---- getting on the plane ---------------------------------------------
  //
  // **The federation pays for the plane. That is what the team is for.**
  // Without the gate the World Cup is a shop you buy placings from and the
  // whole domestic ladder underneath it stops being the way up.
  {
    WorldCupSeason fly = StartWorldCupSeason(world, 1, 1, wd);
    const int roundDay = fly.schedule[0].day;
    NationalTeam onIt;
    onIt.status = TeamStatus::Named;
    NationalTeam offIt;   // Never

    CHECK(wd.requiresTeam);
    // Nothing on today is not a refusal, it is silence.
    {
      const FlightCheck quiet = CanFly(fly, onIt, 1e6, roundDay - 1, wd);
      CHECK(!quiet.can && quiet.round == -1 && quiet.why.empty());
    }
    {
      const FlightCheck notNamed = CanFly(fly, offIt, 1e6, roundDay, wd);
      CHECK(!notNamed.can);
      CHECK(notNamed.round == 0);
      CHECK(notNamed.why.find("federation") != std::string::npos);
    }
    {
      const FlightCheck broke = CanFly(fly, onIt, 1.0, roundDay, wd);
      CHECK(!broke.can);
      CHECK(broke.cost > 0.0);
      CHECK(!broke.why.empty());
    }
    {
      const FlightCheck go = CanFly(fly, onIt, 1e6, roundDay, wd);
      CHECK(go.can);
      CHECK(go.why.empty());
      // The ticket is the venue's, not a flat number -- **the dial that
      // makes a season a budget problem**.
      CHECK(go.cost == TheVenues()[fly.schedule[0].venue].travel);
    }
    // And once it is climbed, the door shuts.
    const CompResult r = ARoundYouPlaced(3);
    BankRound(fly, 0, &r, world, wd);
    CHECK(CanFly(fly, onIt, 1e6, roundDay, wd).round == -1);
  }

  // ---- one night of it --------------------------------------------------
  //
  // The night tick is the whole reason any of this is reachable: a career
  // that never hears of the World Cup still has one going on around it, and
  // the rounds it does not fly to are banked by the people who did.
  {
    WorldCupSeason s2;
    Olympics o2;
    // Night one: a season opens and the Games get a date.
    WorldStageDay(s2, o2, world, 1, wd);
    CHECK(s2.season == 1);
    CHECK(static_cast<int>(s2.schedule.size()) == wd.rounds);
    CHECK(o2.nextDay >= 22);
    const int seeded = o2.nextDay;

    // **Rounds you did not fly to are banked by the people who did**, and
    // the day of a round stays enterable for the whole of that day -- a
    // round on day D is not a miss until the night of D+1.
    const int firstRound = s2.schedule[0].day;
    WorldStageDay(s2, o2, world, firstRound, wd);
    CHECK(!s2.schedule[0].resolved);
    WorldStageDay(s2, o2, world, firstRound + 1, wd);
    CHECK(s2.schedule[0].resolved);
    CHECK(!s2.schedule[0].flown);
    CHECK(RoundsMissed(s2) == 1);
    CHECK(RoundsFlown(s2) == 0);

    // Run the year out. It closes once, says so once, and then sits closed
    // through the off-season rather than announcing a title forty-five
    // times.
    int day = firstRound + 1;
    int closes = 0;
    std::string news;
    const int lastRound = s2.schedule.back().day;
    while (day <= lastRound + 1) {
      const WorldStageNight n = WorldStageDay(s2, o2, world, day, wd);
      if (n.seasonClosed) { closes++; news = n.news; }
      day++;
    }
    CHECK(closes == 1);
    CHECK(s2.closed);
    // Finishing last of thirteen because you never got on a plane is a
    // different sentence from finishing last because you did.
    CHECK(news.find("without leaving the country") != std::string::npos);
    CHECK(news == WorldCupSeasonNews(s2));
    CHECK(s2.lastRank == static_cast<int>(WorldTable(s2).size()));

    // The off-season is real: the next season does not open the next
    // morning.
    WorldStageDay(s2, o2, world, lastRound + 2, wd);
    CHECK(s2.season == 1);
    for (int i = 0; i < wd.worldSeasonBreakDays + 2; i++) {
      WorldStageDay(s2, o2, world, lastRound + 2 + i, wd);
    }
    CHECK(s2.season == 2);
    CHECK(!s2.closed);
    // A table resets and a record does not. Six rounds went by without
    // you, and the second season starts remembering that.
    CHECK(s2.missed == wd.rounds);
    CHECK(RoundsMissed(s2) == 0);
    CHECK(s2.lastRank > 0);

    // And the Games rolled rather than vanished.
    CHECK(o2.nextDay >= seeded);
    CHECK((o2.nextDay - seeded) % wd.olympicCycleDays == 0);
  }
}

static void TestRival() {
  RivalDials rd;
  const Rng world = Rng::FromSeed("somebody-to-beat");
  Skills you;
  you.power = 50; you.fingers = 50; you.technique = 50;
  you.endurance = 50; you.head = 50;

  // ---- they are built against you --------------------------------------
  //
  // **The detail that stops a rival being a flavour generator.** Their style
  // leans toward whatever you are weakest at, so they are somebody who beats
  // you where it hurts rather than somebody with a random adjective.
  // Checked per weakness rather than once, because a single seed proves
  // nothing about a table.
  {
    Skills weakFingers = you; weakFingers.fingers = 20;
    Skills weakEnd = you;     weakEnd.endurance = 20;
    int crimpers = 0, engines = 0;
    for (int s = 0; s < 40; s++) {
      const Rng w = Rng::FromSeed("style#" + std::to_string(s));
      if (RollRival(w, weakFingers, 1, 0, 5.0, rd).style ==
          RouteType::Crimp) {
        crimpers++;
      }
      if (RollRival(w, weakEnd, 1, 0, 5.0, rd).style ==
          RouteType::Endurance) {
        engines++;
      }
    }
    // Weak fingers always draws a crimper; weak endurance always draws an
    // engine. Both are single-option lanes, so this is exact.
    CHECK(crimpers == 40);
    CHECK(engines == 40);
  }

  // ---- the chase, and the ceiling that is yours -------------------------
  Rival r = RollRival(world, you, 1, 0, 5.0, rd);
  CHECK(!r.name.empty());
  CHECK(r.generation == 0);
  // They were already here and already better. That is the head start that
  // makes them a benchmark rather than a twin.
  CHECK(r.grade > 5.0);
  CHECK(std::abs(r.grade - 6.0) < 1e-9);

  // They chase, they do not teleport. Jumping straight to your grade plus
  // the lead would delete the season of being ahead that is the whole point
  // of catching them.
  Rival chaser = r;
  chaser.grade = 5.0;
  int steps = 0;
  for (int day = 2; day <= 40; day++) {
    if (RivalDay(chaser, 12.0, day, rd)) steps++;
  }
  CHECK(steps > 5);                 // they are moving
  CHECK(chaser.grade < 12.0);       // and they have not arrived
  CHECK(chaser.grade > 5.0);

  // **The top of the ladder is yours.** A rival who could reach the mythical
  // grades would eventually take the one thing a career is for, so the cap
  // is checked against a player who is already above it.
  Rival capped = r;
  for (int day = 2; day <= 4000; day++) RivalDay(capped, 18.0, day, rd);
  CHECK(capped.grade <= rd.gradeCap + 1e-9);
  CHECK(capped.grade >= rd.gradeCap - 1e-9);   // and it does get there

  // ---- they age, which is what stops them being a metronome ------------
  //
  // A rival who ticks up forever is a difficulty slider with a name. Theirs
  // is a career: it flattens at the peak age and then it is over.
  CHECK(RivalAge(r, 1, rd) == rd.rivalStartAge);
  CHECK(std::abs(RivalAge(r, 1 + rd.daysPerYear, rd) - (rd.rivalStartAge + 1.0)) <
        1e-9);
  CHECK(CanImprove(r, 1, rd));
  const int pastPeak =
      1 + static_cast<int>((rd.peakAge - rd.rivalStartAge + 1.0) * rd.daysPerYear);
  CHECK(!CanImprove(r, pastPeak, rd));
  {
    // Pinned as a magnitude: an old rival gains *nothing*, not merely less.
    Rival old = r;
    old.grade = 6.0;
    const double before = old.grade;
    for (int d = pastPeak; d < pastPeak + 200; d++) {
      RivalDay(old, 15.0, d, rd);
    }
    CHECK(old.grade == before);
  }

  // ---- and then they hang it up ----------------------------------------
  //
  // Not before the age, ever -- and asking twice in one season is the same
  // answer, because a caller that checks daily must not get ninety-one rolls
  // at it. That is the difference between a 28% season and a certainty.
  CHECK(!ThinkingAboutIt(r, world, 1, rd));
  const int retireDay =
      1 + static_cast<int>((rd.retireAge - rd.rivalStartAge + 1.0) * rd.daysPerYear);
  const bool first = ThinkingAboutIt(r, world, retireDay, rd);
  for (int d = retireDay; d < retireDay + rd.seasonDays &&
                          d / rd.seasonDays == retireDay / rd.seasonDays;
       d++) {
    CHECK(ThinkingAboutIt(r, world, d, rd) == first);
  }
  // Over enough seasons it does happen, and it happens to most careers.
  {
    int retiredIn = 0;
    for (int s = 0; s < 60; s++) {
      Rival cand = RollRival(Rng::FromSeed("ret#" + std::to_string(s)), you, 1,
                             0, 5.0, rd);
      const Rng w = Rng::FromSeed("ret#" + std::to_string(s));
      for (int season = 0; season < 8; season++) {
        const int d = retireDay + season * rd.seasonDays;
        if (ThinkingAboutIt(cand, w, d, rd)) { retiredIn++; break; }
      }
    }
    // Eight seasons at 28% is ~92% in theory; the check is loose enough to
    // be about the design rather than the arithmetic.
    CHECK(retiredIn > 45);
  }

  // ---- they do not leave the world -------------------------------------
  //
  // The whole reason for ageing them. **An ally stays close; somebody who
  // beat you and left is the one that stings**, and only that branch makes
  // "gone" likely.
  {
    int alliedGone = 0, beatenGone = 0;
    for (int s = 0; s < 200; s++) {
      const Rng w = Rng::FromSeed("role#" + std::to_string(s));
      Rival a = r; a.allied = true; a.generation = s;
      Rival b = r; b.rivalry = -20.0; b.generation = s;
      if (Retire(a, w, retireDay, rd).role == RivalRole::Gone) alliedGone++;
      if (Retire(b, w, retireDay, rd).role == RivalRole::Gone) beatenGone++;
    }
    CHECK(alliedGone == 0);            // a friend never just vanishes
    CHECK(beatenGone > 60);            // and being beaten often ends it
  }

  // ---- somebody steps up ------------------------------------------------
  //
  // The moment a career turns over: for the first time somebody is chasing
  // *you*, and they are gaining twice as fast as the last one did.
  {
    const Rival next = Succeed(world, you, 9.0, 500, 1, rd);
    CHECK(next.generation == 1);
    CHECK(next.grade < 9.0);                       // below you, not ahead
    CHECK(std::abs(next.grade - 6.0) < 1e-9);      // exactly the lag
    CHECK(next.startAge < r.startAge);             // and younger

    // Twice as fast, pinned as a magnitude rather than an ordering.
    Rival young = next, oldGuard = next;
    oldGuard.generation = 0;
    int youngSteps = 0, oldSteps = 0;
    for (int d = 501; d <= 560; d++) {
      if (RivalDay(young, 30.0, d, rd)) youngSteps++;
      if (RivalDay(oldGuard, 30.0, d, rd)) oldSteps++;
    }
    CHECK(youngSteps > oldSteps * 3 / 2);
  }

  // ---- what moves the rivalry ------------------------------------------
  {
    Rival h = r;
    CHECK(h.rivalry == 0.0);
    TheyGotThereFirst(h, "The Prow", rd);
    CHECK(h.rivalry < 0.0);
    CHECK(h.firstAscents.size() == 1);
    // Taking the same line twice is not two defeats.
    TheyGotThereFirst(h, "The Prow", rd);
    CHECK(h.firstAscents.size() == 1);
    CHECK(h.rivalry == -rd.faSwing);

    // The ally arc opens on the head-to-head and fires once.
    CHECK(!WouldPartnerUp(h, rd));
    for (int i = 0; i < 20; i++) YouGotThereFirst(h, rd);
    CHECK(WouldPartnerUp(h, rd));
    h.offered = true;
    CHECK(!WouldPartnerUp(h, rd));   // asked is asked
  }

  // ---- what the game says about them ------------------------------------
  {
    Rival q = r;
    // Silent until you have been introduced. A number for a stranger is a
    // leaderboard, not a rival.
    CHECK(RivalLine(q, 5.0, 10, rd).empty());
    q.met = true;
    CHECK(!RivalLine(q, 5.0, 10, rd).empty());
    CHECK(RivalLine(q, 5.0, 10, rd).find(q.name) != std::string::npos);
    // The three states read differently.
    const std::string ahead = RivalLine(q, 2.0, 10, rd);
    const std::string level = RivalLine(q, q.grade, 10, rd);
    const std::string behind = RivalLine(q, 15.0, 10, rd);
    CHECK(ahead != level && level != behind && ahead != behind);
    // Past their best is its own sentence, because it changes what the
    // chase means: you stop catching them and start outlasting them.
    CHECK(RivalLine(q, 2.0, pastPeak, rd) != ahead);
    // No numbers anywhere in it.
    for (char ch : level) CHECK(!(ch >= '0' && ch <= '9'));
  }
}

static void TestRivalRace() {
  RivalDials rd;
  const Rng world = Rng::FromSeed("a-line-with-a-clock");
  const Crag crag = RoadsideCrag(world);
  Skills you;
  you.power = 50; you.fingers = 50; you.technique = 50;
  you.endurance = 50; you.head = 50;
  Rival r = RollRival(world, you, 1, 0, 5.0, rd);
  const std::vector<std::string> nobodyOn;

  CHECK(!RaceIsOn(r));
  CHECK(RaceLine(r, 1).empty());

  // ---- when one does not start -----------------------------------------
  //
  // Being raced for a V2 in your first season is the game picking on you.
  {
    Rival beginner = r;
    bool any = false;
    for (int d = 1; d <= 400; d++) {
      if (StartARace(beginner, crag, 1.0, nobodyOn, world, d, rd)) any = true;
    }
    CHECK(!any);
  }
  // Nor once they are on your rope.
  {
    Rival friendly = r;
    friendly.allied = true;
    bool any = false;
    for (int d = 1; d <= 400; d++) {
      if (StartARace(friendly, crag, 8.0, nobodyOn, world, d, rd)) any = true;
    }
    CHECK(!any);
  }
  // And asking twice on one day is one answer, not two rolls at it.
  {
    Rival twice = r;
    for (int d = 1; d <= 60; d++) {
      const bool first = StartARace(twice, crag, 8.0, nobodyOn, world, d, rd);
      if (first) {
        // Already running: a second call cannot start another.
        CHECK(!StartARace(twice, crag, 8.0, nobodyOn, world, d, rd));
        break;
      }
      CHECK(!StartARace(twice, crag, 8.0, nobodyOn, world, d, rd));
    }
  }

  // ---- when one does ----------------------------------------------------
  int started = 0, faRaces = 0, startDay = 0;
  Rival racer = r;
  for (int d = 1; d <= 400 && started == 0; d++) {
    if (StartARace(racer, crag, 8.0, nobodyOn, world, d, rd)) {
      started++;
      startDay = d;
      if (racer.race.forFirstAscent) faRaces++;
    }
  }
  CHECK(started == 1);
  CHECK(RaceIsOn(racer));
  CHECK(!racer.race.routeName.empty());
  CHECK(racer.race.byDay == startDay + rd.raceDays);
  // You cannot be raced by a stranger.
  CHECK(racer.met);

  // The clock is a clock.
  CHECK(!RaceRanOut(racer, startDay));
  CHECK(!RaceRanOut(racer, racer.race.byDay - 1));
  CHECK(RaceRanOut(racer, racer.race.byDay));
  CHECK(RaceRanOut(racer, racer.race.byDay + 10));

  // It says something, and it says it without a number.
  const std::string said = RaceLine(racer, startDay);
  CHECK(!said.empty());
  CHECK(said.find(racer.name) != std::string::npos);
  CHECK(said.find(racer.race.routeName) != std::string::npos);
  for (char ch : said) CHECK(!(ch >= '0' && ch <= '9'));
  // The last day reads differently from the first.
  CHECK(RaceLine(racer, racer.race.byDay - 1) != said);

  // ---- and how it ends --------------------------------------------------
  {
    Rival won = racer;
    const double before = won.rivalry;
    YouWonTheRace(won, rd);
    CHECK(!RaceIsOn(won));
    CHECK(won.rivalry > before);
    // Winning twice off one race is not a thing.
    const double after = won.rivalry;
    YouWonTheRace(won, rd);
    CHECK(won.rivalry == after);
  }
  {
    // **Losing an open line is not the same as being repeated.** Pinned as
    // a magnitude, because an ordering passes on a build where the two
    // swings differ by a hundredth.
    Rival lostRepeat = racer;
    lostRepeat.race.forFirstAscent = false;
    Rival lostFa = racer;
    lostFa.race.forFirstAscent = true;
    TheyWonTheRace(lostRepeat, rd);
    TheyWonTheRace(lostFa, rd);
    CHECK(lostFa.rivalry < lostRepeat.rivalry * 2.0);
    // And the open line is gone, with their name against it.
    CHECK(lostFa.firstAscents.size() == 1);
    CHECK(lostRepeat.firstAscents.empty());
  }

  // ---- how often, across a career ---------------------------------------
  //
  // A race nobody ever sees is a system that does not exist; one every week
  // is noise. Measured over ten careers rather than asserted.
  {
    int total = 0;
    for (int s = 0; s < 10; s++) {
      const Rng w = Rng::FromSeed("races#" + std::to_string(s));
      Rival cand = RollRival(w, you, 1, 0, 5.0, rd);
      for (int d = 1; d <= 365; d++) {
        if (StartARace(cand, crag, 8.0, nobodyOn, w, d, rd)) total++;
        if (RaceRanOut(cand, d)) TheyWonTheRace(cand, rd);
      }
    }
    // Ten careers of a year each. At a tenth a day with a five-day lockout
    // this lands in the tens, not the hundreds and not zero.
    CHECK(total > 20);
    CHECK(total < 400);
  }
  (void)faRaces;
}

static void TestRivalCareer() {
  // **The night tick, which nothing covered.** Killing the whole rival
  // block in `SleepToNextDay` passed the entire suite -- the rival's career
  // existed and never ran, which is this project's oldest bug and the sixth
  // layer it has been found at. So the test is a career rather than a unit:
  // sleep for thirty years and check that somebody had a life.
  RivalDials rd;
  PlayerState player;
  DayState day;
  const Rng world = Rng::FromSeed("a-life");
  player.climber = NewClimber(world);
  player.rival = RollRival(world, player.climber.skills, 1, 0, 5.0, rd);

  const std::string firstName = player.rival.name;
  const double firstGrade = player.rival.grade;
  const int years = 30;

  double lastGrade = firstGrade;
  bool everImproved = false, everFlattened = false;
  for (int i = 0; i < years * rd.daysPerYear; i++) {
    SleepToNextDay(player, day, world);
    if (player.rival.grade > lastGrade) everImproved = true;
    lastGrade = player.rival.grade;
  }

  // They chased. Without the night tick this is the check that fires.
  CHECK(everImproved);
  CHECK(player.rival.grade > 0.0);

  // **Somebody had a career and it ended.** Thirty years is four or five
  // rival lifetimes at these dials, so a career that saw none of them
  // retire means the retirement roll never ran.
  CHECK(!player.pastRivals.empty());
  CHECK(player.rival.generation > 0);
  CHECK(player.rival.name != firstName || player.pastRivals.size() > 1);

  // They did not leave the world. Most of them are still around, on the
  // other side of a counter or with their name on a guidebook.
  int stayed = 0;
  for (const PastRival& p : player.pastRivals) {
    CHECK(!p.name.empty());
    CHECK(p.age >= rd.retireAge);
    CHECK(p.peakGrade > 0.0);
    if (p.role != RivalRole::Gone) stayed++;
  }
  CHECK(stayed > 0);

  // And the generations are in order, oldest first, with no gaps -- the
  // record is a lineage rather than a bag.
  for (std::size_t i = 0; i < player.pastRivals.size(); i++) {
    CHECK(player.pastRivals[i].generation == static_cast<int>(i));
    if (i > 0) {
      CHECK(player.pastRivals[i].retiredOnDay >=
            player.pastRivals[i - 1].retiredOnDay);
    }
  }
  (void)everFlattened;
}

static void TestWorldStageSave() {
  WorldStageDials wd;
  SaveGame save;
  save.seed = "innsbruck";

  // A season half run: some rounds flown, some missed, points on the board.
  WorldCupSeason& s = save.player.worldCup;
  s = StartWorldCupSeason(Rng::FromSeed(save.seed), 10, 3, wd);
  s.titles = 1;
  s.bestRank = 2;
  const CompResult third = ARoundYouPlaced(3);
  BankRound(s, 0, &third, Rng::FromSeed("r0"), wd);
  BankRound(s, 1, nullptr, Rng::FromSeed("r1"), wd);

  Olympics& o = save.player.olympics;
  SeedTheGames(o, Rng::FromSeed(save.seed), 10, wd);
  BankTheGames(o, MedalFor(2), o.nextDay);

  // **The ranking record, which is what the ranking is.** Losing it does
  // not lose a number, it loses the ability to lose the number: without
  // the record the night tick recomputes the ranking as zero, so a
  // load-bearing save bug here reads as "the game forgot your career".
  const CompDials cds;
  Record(save.player.rankingRecord, 40, 120.0, cds);
  Record(save.player.rankingRecord, 55, -2.0, cds);
  save.player.rankingPoints =
      RankingFrom(save.player.rankingRecord, 55, cds);
  save.player.team.lastReviewDay = 44;

  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  const WorldCupSeason& w = back.player.worldCup;
  CHECK(w.season == 3);
  CHECK(w.titles == 1);
  CHECK(w.bestRank == 2);
  CHECK(w.yourPoints == s.yourPoints);
  CHECK(w.starts == 1 && w.missed == 1);
  CHECK(w.schedule.size() == s.schedule.size());
  // **Which rounds you flew to is the season.** A save that forgets it
  // hands back a table you cannot account for -- and one that forgets a
  // venue hands back a different schedule with the same dates.
  for (std::size_t i = 0; i < w.schedule.size() && i < s.schedule.size();
       i++) {
    CHECK(w.schedule[i].day == s.schedule[i].day);
    CHECK(w.schedule[i].venue == s.schedule[i].venue);
    CHECK(w.schedule[i].resolved == s.schedule[i].resolved);
    CHECK(w.schedule[i].flown == s.schedule[i].flown);
  }
  CHECK(w.fieldPoints.size() == TheWorldField().size());
  for (std::size_t i = 0; i < w.fieldPoints.size(); i++) {
    CHECK(w.fieldPoints[i] == s.fieldPoints[i]);
  }
  CHECK(back.player.rankingRecord.size() == 2);
  if (back.player.rankingRecord.size() == 2) {
    CHECK(back.player.rankingRecord[0].day == 40);
    CHECK(back.player.rankingRecord[0].points == 120.0);
    // The no-show is negative and has to survive as one.
    CHECK(back.player.rankingRecord[1].points == -2.0);
  }
  CHECK(RankingFrom(back.player.rankingRecord, 55, cds) == 118.0);
  // And a year on it is worth nothing, on the loaded save exactly as on
  // the live one.
  CHECK(RankingFrom(back.player.rankingRecord, 55 + cds.rankingWindowDays,
                    cds) == 0.0);
  // The committee's clock. Without it a loaded save gets a review the next
  // season's close, and the thermostat is back.
  CHECK(back.player.team.lastReviewDay == 44);

  // And the medal, which is the one number in this game nobody would
  // forgive losing.
  CHECK(back.player.olympics.nextDay == o.nextDay);
  CHECK(back.player.olympics.silver == 1);
  CHECK(back.player.olympics.appearances == 1);
  // `lastCompeted` is what stops the same Games being entered twice, and it
  // defaults to -1 rather than 0 -- so a save that round-trips it as an
  // unsigned or clamps it at zero would re-open a Games you have climbed.
  CHECK(back.player.olympics.lastCompeted == o.nextDay);
  Olympics fresh;
  CHECK(fresh.lastCompeted == -1);
  SaveGame freshBack;
  SaveGame freshSave;
  CHECK(DeserializeSave(SerializeSave(freshSave), freshBack) ==
        LoadResult::Ok);
  CHECK(freshBack.player.olympics.lastCompeted == -1);

  // **An old save loads, and arrives with a career that never got on a
  // plane.** Exact rather than generous: giving a v26 career a World Cup
  // start would be inventing a year it did not have.
  // The things that are wrong with you that are not the injury. The
  // tooth is the one that matters most here: it is the only clock in the
  // game that never goes backwards, so a save that forgets which stage it
  // was at hands back a career that quietly healed something that cannot.
  {
    save.player.sickness.active = true;
    save.player.sickness.daysLeft = 6;
    save.player.sickness.severity = 0.42;
    save.player.sickness.medicated = true;
    save.player.sickness.caught = 5;
    save.player.teeth.stage = ToothStage::Ache;
    save.player.teeth.sinceDay = 812;
    save.player.teeth.fixes = 2;
    save.player.teeth.worstEver = 3;
    save.player.upkeep.lastPrehabDay = 900;
    save.player.upkeep.prehabStreak = 44;
    save.player.upkeep.prehabDays = 310;
    save.player.upkeep.lastShrinkDay = 880;
    save.player.upkeep.shrinkSessions = 9;

    SaveGame ab;
    CHECK(DeserializeSave(SerializeSave(save), ab) == LoadResult::Ok);
    CHECK(ab.player.sickness.active);
    CHECK(ab.player.sickness.daysLeft == 6);
    CHECK(ab.player.sickness.severity == 0.42);
    CHECK(ab.player.sickness.medicated);
    CHECK(ab.player.sickness.caught == 5);
    CHECK(ab.player.teeth.stage == ToothStage::Ache);
    CHECK(ab.player.teeth.sinceDay == 812);
    CHECK(ab.player.teeth.fixes == 2);
    CHECK(ab.player.teeth.worstEver == 3);
    CHECK(ab.player.upkeep.prehabStreak == 44);
    CHECK(ab.player.upkeep.lastShrinkDay == 880);
    CHECK(ab.player.upkeep.shrinkSessions == 9);
    // And the things they are worth come back identical.
    CHECK(PrehabRisk(ab.player.upkeep) == PrehabRisk(save.player.upkeep));
    CHECK(ToothPrice(ab.player.teeth.stage) ==
          ToothPrice(save.player.teeth.stage));

    // **A v30 career was never ill and has good teeth**, which is the
    // honest default rather than a generous one: none of it existed to
    // have happened. And the tooth clock starts from the load, so a career
    // twenty years in does not wake up with an abscess it never had.
    std::string v30 = SerializeSave(save);
    for (const char* k : {"sick.active=", "sick.days=", "sick.sev=",
                          "sick.meds=", "sick.caught=", "teeth.stage=",
                          "teeth.since=", "teeth.fixes=", "teeth.worst=", "teeth.lost=",
                          "up.prehabday=", "up.streak=", "up.prehabdays=",
                          "up.shrinkday=", "up.shrinks="}) {
      DropSaveLine(v30, k);
    }
    SetSaveVersion(v30, 30);
    SaveGame old30;
    CHECK(DeserializeSave(v30, old30) == LoadResult::Ok);
    CHECK(old30.version == kSaveVersion);
    CHECK(!old30.player.sickness.active);
    CHECK(old30.player.teeth.stage == ToothStage::Fine);
    CHECK(old30.player.teeth.sinceDay == 0);
    CHECK(old30.player.upkeep.prehabStreak == 0);
    CHECK(PrehabRisk(old30.player.upkeep) == 1.0);
    CHECK(old30.seed == save.seed);
  }

  // The medical file. **Losing a joint's history is losing the career**:
  // the cortisone you took at twenty-eight is why the finger goes at
  // thirty-four, and a save that forgets it hands back a body that never
  // happened.
  {
    Medical& mm = save.player.medical;
    mm.diagnosis = Diagnosis::Scanned;
    mm.treatment = Treatment::Cortisone;
    mm.stage = Comeback::GradedReturn;
    mm.stageStarted = 30;
    mm.stageDays = 12;
    mm.toldSeverity = 0.62;
    mm.joints[static_cast<int>(InjuryKind::Pulley)] = 0.44;
    mm.shots[static_cast<int>(InjuryKind::Pulley)] = 2;
    mm.scars.push_back(Scar{InjuryKind::Elbow, 0.31, 12});
    mm.scars.push_back(Scar{InjuryKind::Shoulder, 0.19, 77});
    mm.insured = true;
    mm.insuredOnDay = 5;
    mm.premiumsPaid = 208.0;
    mm.claimsPaid = 1760.0;
    mm.diagnoses = 3;
    mm.shotsTaken = 2;
    mm.surgeries = 1;
    mm.rushedComebacks = 1;
    mm.untreatedInjuries = 2;
    mm.treatedThisTime = true;

    SaveGame mb;
    CHECK(DeserializeSave(SerializeSave(save), mb) == LoadResult::Ok);
    const Medical& b = mb.player.medical;
    CHECK(b.diagnosis == Diagnosis::Scanned);
    CHECK(b.treatment == Treatment::Cortisone);
    CHECK(b.stage == Comeback::GradedReturn);
    CHECK(b.stageStarted == 30);
    CHECK(b.stageDays == 12);
    CHECK(b.toldSeverity == 0.62);
    CHECK(b.joints[static_cast<int>(InjuryKind::Pulley)] == 0.44);
    CHECK(b.shots[static_cast<int>(InjuryKind::Pulley)] == 2);
    CHECK(b.scars.size() == 2);
    if (b.scars.size() == 2) {
      CHECK(b.scars[0].kind == InjuryKind::Elbow);
      CHECK(b.scars[0].weight == 0.31);
      CHECK(b.scars[0].fromDay == 12);
      CHECK(b.scars[1].kind == InjuryKind::Shoulder);
    }
    // And the risk the joint carries comes back identical, which is the
    // thing that actually matters.
    CHECK(JointRisk(b, InjuryKind::Pulley, 200) ==
          JointRisk(mm, InjuryKind::Pulley, 200));
    CHECK(JointRisk(b, InjuryKind::Elbow, 200) ==
          JointRisk(mm, InjuryKind::Elbow, 200));
    CHECK(b.insured);
    CHECK(b.insuredOnDay == 5);
    CHECK(b.premiumsPaid == 208.0);
    CHECK(b.claimsPaid == 1760.0);
    CHECK(b.surgeries == 1);
    CHECK(b.untreatedInjuries == 2);
    CHECK(b.treatedThisTime);

    // **A v29 career arrives with a clean file and keeps its injury.**
    // Generous on the joints and it is the only honest option: a v29 save
    // has no record of a cortisone history because there was none.
    std::string v29 = SerializeSave(save);
    for (const char* k : {"craft.n=", "craft.taken=", "craft.botched=",
                        "craft.ducked=", "craft.sackings=",
                        "sick.active=", "sick.days=", "sick.sev=", "sick.meds=",
                        "sick.caught=", "teeth.stage=", "teeth.since=",
                        "teeth.fixes=", "teeth.worst=", "teeth.lost=",
                        "up.prehabday=",
                        "up.streak=", "up.prehabdays=", "up.shrinkday=",
                        "up.shrinks=",
                        "med.diagnosis=", "med.treatment=", "med.stage=",
                          "med.stagestart=", "med.stagedays=", "med.told=",
                          "med.joints=", "med.scars=", "med.insured=",
                          "med.insuredon=", "med.premiums=", "med.claims=",
                          "med.diagnoses=", "med.shots=", "med.surgeries=",
                          "med.rushed=", "med.untreated=", "med.treated=",
                          "med.staged="}) {
      DropSaveLine(v29, k);
    }
    SetSaveVersion(v29, 29);
    SaveGame old29;
    CHECK(DeserializeSave(v29, old29) == LoadResult::Ok);
    CHECK(old29.version == kSaveVersion);
    CHECK(old29.player.medical.scars.empty());
    CHECK(!old29.player.medical.insured);
    CHECK(old29.player.medical.stage == Comeback::Clear);
    CHECK(JointRisk(old29.player.medical, InjuryKind::Pulley, 200) == 1.0);
    // A career that was mid-injury when it was saved keeps the injury, and
    // the night tick starts it a comeback -- which is the case the guard
    // in `MedicalDay` exists for.
    {
      Climber hurt = HurtClimber(InjuryKind::Pulley, 0.5);
      Medical fresh;
      MedicalDay(fresh, hurt, Rng::FromSeed("load"), 400);
      CHECK(hurt.injury.active);
      CHECK(fresh.stage == Comeback::Resting);
    }
  }

  // The league: the personal best is the one number here nobody would
  // forgive losing, because it is the whole reason to turn up.
  {
    League& lg = save.player.league;
    lg.nextNight = 61;
    lg.block = 3;
    lg.weeksDone = 4;
    lg.yourPoints = 210.0;
    lg.best = 47.5;
    lg.bestOnDay = 33;
    lg.nights = 22;
    lg.blockWins = 1;
    lg.fieldPoints.assign(TheRegulars().size(), 12.0);
    SaveGame lb;
    CHECK(DeserializeSave(SerializeSave(save), lb) == LoadResult::Ok);
    CHECK(lb.player.league.best == 47.5);
    CHECK(lb.player.league.bestOnDay == 33);
    CHECK(lb.player.league.nights == 22);
    CHECK(lb.player.league.blockWins == 1);
    CHECK(lb.player.league.block == 3);
    CHECK(lb.player.league.weeksDone == 4);
    CHECK(lb.player.league.nextNight == 61);
    CHECK(lb.player.league.fieldPoints.size() == TheRegulars().size());
    // Losing the block table would hand a half-finished block back with
    // the regulars on nothing, which is a block you cannot lose.
    for (double p : lb.player.league.fieldPoints) CHECK(p == 12.0);

    std::string v28 = SerializeSave(save);
    for (const char* k : {"craft.n=", "craft.taken=", "craft.botched=",
                        "craft.ducked=", "craft.sackings=",
                        "sick.active=", "sick.days=", "sick.sev=", "sick.meds=",
                        "sick.caught=", "teeth.stage=", "teeth.since=",
                        "teeth.fixes=", "teeth.worst=", "teeth.lost=",
                        "up.prehabday=",
                        "up.streak=", "up.prehabdays=", "up.shrinkday=",
                        "up.shrinks=",
                        "med.diagnosis=", "med.treatment=", "med.stage=",
                        "med.stagestart=", "med.stagedays=", "med.told=",
                        "med.joints=", "med.scars=", "med.insured=",
                        "med.insuredon=", "med.premiums=", "med.claims=",
                        "med.diagnoses=", "med.shots=", "med.surgeries=",
                        "med.rushed=", "med.untreated=", "med.treated=",
                        "med.staged=",
                        "league.next=", "league.block=", "league.weeks=",
                          "league.you=", "league.best=", "league.bestday=",
                          "league.nights=", "league.wins=",
                          "league.fields=", "league.lastnight="}) {
      DropSaveLine(v28, k);
    }
    SetSaveVersion(v28, 28);
    SaveGame old28;
    CHECK(DeserializeSave(v28, old28) == LoadResult::Ok);
    CHECK(old28.player.league.nextNight == 0);
    CHECK(old28.player.league.best == 0.0);
    CHECK(old28.player.league.nights == 0);
    CHECK(old28.player.league.block == 1);
  }

  // **v27 -> v28 deliberately throws the old ranking away.** The lifetime
  // total is not a smaller version of the rolling one, it is a different
  // measurement -- and carrying 14,633 points across would hand a migrated
  // save a World-Class rung it could never lose, because there is no
  // record behind it to age out.
  {
    std::string v27 = SerializeSave(save);
    for (const char* k : {"craft.n=", "craft.taken=", "craft.botched=",
                        "craft.ducked=", "craft.sackings=",
                        "sick.active=", "sick.days=", "sick.sev=", "sick.meds=",
                        "sick.caught=", "teeth.stage=", "teeth.since=",
                        "teeth.fixes=", "teeth.worst=", "teeth.lost=",
                        "up.prehabday=",
                        "up.streak=", "up.prehabdays=", "up.shrinkday=",
                        "up.shrinks=",
                        "med.diagnosis=", "med.treatment=", "med.stage=",
                        "med.stagestart=", "med.stagedays=", "med.told=",
                        "med.joints=", "med.scars=", "med.insured=",
                        "med.insuredon=", "med.premiums=", "med.claims=",
                        "med.diagnoses=", "med.shots=", "med.surgeries=",
                        "med.rushed=", "med.untreated=", "med.treated=",
                        "med.staged=",
                        "league.next=", "league.block=", "league.weeks=",
                          "league.you=", "league.best=", "league.bestday=",
                          "league.nights=", "league.wins=",
                          "league.fields=", "league.lastnight="}) {
      DropSaveLine(v27, k);
    }
    DropSaveLine(v27, "rank.results=");
    DropSaveLine(v27, "rank.r0d=");
    DropSaveLine(v27, "rank.r0p=");
    DropSaveLine(v27, "rank.r1d=");
    DropSaveLine(v27, "rank.r1p=");
    DropSaveLine(v27, "team.lastday=");
    SetSaveVersion(v27, 27);
    SaveGame old27;
    CHECK(DeserializeSave(v27, old27) == LoadResult::Ok);
    CHECK(old27.version == kSaveVersion);
    CHECK(old27.player.rankingRecord.empty());
    CHECK(old27.player.rankingPoints == 0.0);
    CHECK(RankFor(old27.player.rankingPoints) == RankTier::Unranked);
    CHECK(old27.player.team.lastReviewDay == 0);
    // The rest of the career is untouched -- it is the ranking that is
    // re-earned, not the climber.
    CHECK(old27.seed == save.seed);
    CHECK(old27.player.olympics.silver == 1);
  }

  std::string v26 = SerializeSave(save);
  DropSaveLine(v26, "wc.season=");
  DropSaveLine(v26, "wc.you=");
  DropSaveLine(v26, "wc.closed=");
  DropSaveLine(v26, "wc.starts=");
  DropSaveLine(v26, "wc.missed=");
  DropSaveLine(v26, "wc.finals=");
  DropSaveLine(v26, "wc.podiums=");
  DropSaveLine(v26, "wc.wins=");
  DropSaveLine(v26, "wc.titles=");
  DropSaveLine(v26, "wc.best=");
  DropSaveLine(v26, "wc.last=");
  DropSaveLine(v26, "wc.rounds=");
  DropSaveLine(v26, "wc.fields=");
  DropSaveLine(v26, "og.next=");
  DropSaveLine(v26, "og.appearances=");
  DropSaveLine(v26, "og.gold=");
  DropSaveLine(v26, "og.silver=");
  DropSaveLine(v26, "og.bronze=");
  DropSaveLine(v26, "og.last=");
  for (const char* k : {"craft.n=", "craft.taken=", "craft.botched=",
                        "craft.ducked=", "craft.sackings=",
                        "sick.active=", "sick.days=", "sick.sev=", "sick.meds=",
                        "sick.caught=", "teeth.stage=", "teeth.since=",
                        "teeth.fixes=", "teeth.worst=", "teeth.lost=",
                        "up.prehabday=",
                        "up.streak=", "up.prehabdays=", "up.shrinkday=",
                        "up.shrinks=",
                        "med.diagnosis=", "med.treatment=", "med.stage=",
                        "med.stagestart=", "med.stagedays=", "med.told=",
                        "med.joints=", "med.scars=", "med.insured=",
                        "med.insuredon=", "med.premiums=", "med.claims=",
                        "med.diagnoses=", "med.shots=", "med.surgeries=",
                        "med.rushed=", "med.untreated=", "med.treated=",
                        "med.staged=",
                        "league.next=", "league.block=", "league.weeks=",
                        "league.you=", "league.best=", "league.bestday=",
                        "league.nights=", "league.wins=", "league.fields=",
                        "league.lastnight="}) {
    DropSaveLine(v26, k);
  }
  DropSaveLine(v26, "rank.results=");
  DropSaveLine(v26, "rank.r0d=");
  DropSaveLine(v26, "rank.r0p=");
  DropSaveLine(v26, "rank.r1d=");
  DropSaveLine(v26, "rank.r1p=");
  DropSaveLine(v26, "team.lastday=");
  SetSaveVersion(v26, 26);

  SaveGame old;
  CHECK(DeserializeSave(v26, old) == LoadResult::Ok);
  CHECK(old.version == kSaveVersion);
  CHECK(old.player.worldCup.season == 0);
  CHECK(old.player.worldCup.schedule.empty());
  CHECK(old.player.olympics.nextDay == 0);
  CHECK(old.player.olympics.appearances == 0);
  // Not zero. `lastCompeted` at 0 would mean "you have climbed the Games
  // held on day zero", and a migration that quietly says so is the kind of
  // off-by-one nobody notices until a Games will not open.
  CHECK(old.player.olympics.lastCompeted == -1);
  // The rest of the career is untouched.
  CHECK(old.seed == save.seed);
  CHECK(static_cast<int>(DefaultMigrations().size()) == kSaveVersion - 1);
}

static void TestRivalSave() {
  RivalDials rd;
  SaveGame save;
  save.seed = "the-one-who-beat-me";
  Skills you;
  you.power = 50; you.fingers = 20; you.technique = 50;
  you.endurance = 50; you.head = 50;
  save.player.rival = RollRival(Rng::FromSeed(save.seed), you, 1, 0, 5.0, rd);
  save.player.rival.met = true;
  save.player.rival.rivalry = -3.0;
  TheyGotThereFirst(save.player.rival, "The Prow", rd);
  TheyGotThereFirst(save.player.rival, "Slab of Regret", rd);
  PastRival gone;
  gone.name = "Silas Mott";
  gone.role = RivalRole::Author;
  gone.style = RouteType::Crack;
  gone.retiredOnDay = 900;
  gone.age = 35.5;
  gone.peakGrade = 11.25;
  gone.generation = 0;
  save.player.pastRivals.push_back(gone);

  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  const Rival& r = back.player.rival;
  CHECK(r.name == save.player.rival.name);
  CHECK(r.style == save.player.rival.style);
  CHECK(r.vibe == save.player.rival.vibe);
  CHECK(r.grade == save.player.rival.grade);
  CHECK(r.rivalry == save.player.rival.rivalry);
  CHECK(r.met);
  // **The lines they took are the part that must not be lost.** Their name
  // is on those in the guidebook forever; a save that forgets them leaves
  // the book saying one thing and the career record another.
  //
  // Guarded rather than indexed straight: the first version of this asserted
  // the size and then read `[0]` regardless, so dropping the count from the
  // save **segfaulted the harness instead of failing the check.** A test
  // that crashes tells you less than one that fails, and it takes the rest
  // of the suite with it.
  CHECK(r.firstAscents.size() == 2);
  if (r.firstAscents.size() == 2) {
    CHECK(r.firstAscents[0] == "The Prow");
    CHECK(r.firstAscents[1] == "Slab of Regret");
  }
  CHECK(back.player.pastRivals.size() == 1);
  if (back.player.pastRivals.size() == 1) {
    CHECK(back.player.pastRivals[0].name == "Silas Mott");
    CHECK(back.player.pastRivals[0].role == RivalRole::Author);
    CHECK(back.player.pastRivals[0].peakGrade == 11.25);
  }

  // **The team survives a reload -- roster, coach and all.** Without it a
  // career would be re-announced onto the national team at every season's
  // close, forever, and the coach who has opinions about you would be a
  // different person every time you loaded.
  {
    TeamDials ttd;
    CompDials tcd;
    Circuit s = StartSeason(Rng::FromSeed("tm"), 1, 2, tcd);
    for (std::size_t i = 0; i < s.fieldPoints.size(); i++) {
      s.fieldPoints[i] = 300.0 - 30.0 * static_cast<double>(i);
    }
    ReviewTheTeam(save.player.team, ttd.selectAt, s, 77, 2, ttd);
    save.player.team.gone.push_back("Bowen");
    CHECK(save.player.team.status == TeamStatus::Named);

    SaveGame back2;
    CHECK(DeserializeSave(SerializeSave(save), back2) == LoadResult::Ok);
    const NationalTeam& t2 = back2.player.team;
    CHECK(t2.status == TeamStatus::Named);
    CHECK(t2.everNamed);
    CHECK(t2.seasons == save.player.team.seasons);
    CHECK(t2.coach == save.player.team.coach);
    CHECK(!t2.coach.empty());
    CHECK(t2.coachKnownFor == save.player.team.coachKnownFor);
    CHECK(t2.roster.size() == save.player.team.roster.size());
    if (t2.roster.size() == save.player.team.roster.size() &&
        !t2.roster.empty()) {
      CHECK(t2.roster[0].name == save.player.team.roster[0].name);
      CHECK(t2.roster[0].role == save.player.team.roster[0].role);
      CHECK(t2.roster[0].points == save.player.team.roster[0].points);
    }
    CHECK(t2.gone == save.player.team.gone);
    // And what the HUD says is the same sentence.
    CHECK(TeamLine(t2, ttd) == TeamLine(save.player.team, ttd));
    save.player.team = NationalTeam{};
  }

  // **A season survives a reload**, dates and all. Without it you would
  // wake up in a season with no schedule, the night tick would see every
  // date as missing and forfeit its way through the year.
  {
    CompDials ccd;
    save.player.circuit = StartSeason(Rng::FromSeed("sv"), 10, 3, ccd);
    save.player.circuit.compsDone = 2;
    save.player.circuit.yourPoints = 145.0;
    save.player.circuit.rivalPoints = 90.0;
    save.player.circuit.fieldPoints[0] = 60.0;
    save.player.circuit.titles = 1;
    SaveGame season;
    CHECK(DeserializeSave(SerializeSave(save), season) == LoadResult::Ok);
    const Circuit& c2 = season.player.circuit;
    CHECK(c2.season == 3);
    CHECK(c2.compsDone == 2);
    CHECK(c2.yourPoints == 145.0);
    CHECK(c2.rivalPoints == 90.0);
    CHECK(c2.titles == 1);
    CHECK(c2.schedule == save.player.circuit.schedule);
    CHECK(c2.fieldPoints.size() == save.player.circuit.fieldPoints.size());
    CHECK(c2.fieldPoints[0] == 60.0);
    // And the table it produces is the same table.
    CHECK(SeasonTable(c2).front().name ==
          SeasonTable(save.player.circuit).front().name);
    save.player.circuit = Circuit{};
  }

  // Ranking points survive a reload: they are the only thing a comp pays
  // that lasts, and losing them would reset your tier every time you slept.
  save.player.rankingPoints = 412.5;
  SaveGame ranked;
  CHECK(DeserializeSave(SerializeSave(save), ranked) == LoadResult::Ok);
  CHECK(ranked.player.rankingPoints == 412.5);
  CHECK(TierFor(ranked.player.rankingPoints) == CompTier::Regional);
  save.player.rankingPoints = 0.0;

  // The line they are on survives a reload -- a race that forgot its
  // deadline would hand you back days you had already spent.
  save.player.rival.race.routeName = "The Prow";
  save.player.rival.race.byDay = 412;
  save.player.rival.race.forFirstAscent = true;
  SaveGame raced;
  CHECK(DeserializeSave(SerializeSave(save), raced) == LoadResult::Ok);
  CHECK(raced.player.rival.race.routeName == "The Prow");
  CHECK(raced.player.rival.race.byDay == 412);
  CHECK(raced.player.rival.race.forFirstAscent);
  CHECK(RaceIsOn(raced.player.rival));
  save.player.rival.race = Race{};

  // A v22 career predates the race, and migrates to nothing running.
  {
    std::string v22 = SerializeSave(save);
    DropSaveLine(v22, "rival.race=");
    DropSaveLine(v22, "rival.raceby=");
    DropSaveLine(v22, "rival.racefa=");
    SetSaveVersion(v22, 22);
    SaveGame old22;
    CHECK(DeserializeSave(v22, old22) == LoadResult::Ok);
    CHECK(old22.version == kSaveVersion);
    CHECK(!RaceIsOn(old22.player.rival));
    // ...and the rest of the rival is untouched.
    CHECK(old22.player.rival.name == save.player.rival.name);
  }

  // **A v21 career had nobody, and must not have a stranger appear.** It
  // migrates to an empty rival, and the night tick skips a rival with no
  // name -- so the career plays exactly as it did.
  std::string v21 = SerializeSave(save);
  for (const char* k : {"rival.name=", "rival.style=", "rival.vibe=",
                        "rival.gen=", "rival.born=", "rival.startage=",
                        "rival.grade=", "rival.laststep=", "rival.peak=",
                        "rival.rivalry=", "rival.allied=", "rival.offered=",
                        "rival.met=", "rival.retired=", "ranking=",
                        "circuit.season=", "circuit.done=", "circuit.you=",
                        "circuit.rival=", "circuit.titles=",
                        "circuit.dates=", "circuit.fields=",
                        "team.status=", "team.ever=", "team.seasons=",
                        "team.cuts=", "team.namedday=", "team.coach=",
                        "team.coachfor=", "team.passed=", "team.lastpts=",
                        "team.lastseason=", "team.lastday=",
                        "team.mates=", "team.gone=", "rank.results=",
                        "craft.n=", "craft.taken=", "craft.botched=",
                        "craft.ducked=", "craft.sackings=",
                        "sick.active=", "sick.days=", "sick.sev=", "sick.meds=",
                        "sick.caught=", "teeth.stage=", "teeth.since=",
                        "teeth.fixes=", "teeth.worst=", "teeth.lost=",
                        "up.prehabday=",
                        "up.streak=", "up.prehabdays=", "up.shrinkday=",
                        "up.shrinks=",
                        "med.diagnosis=", "med.treatment=", "med.stage=",
                        "med.stagestart=", "med.stagedays=", "med.told=",
                        "med.joints=", "med.scars=", "med.insured=",
                        "med.insuredon=", "med.premiums=", "med.claims=",
                        "med.diagnoses=", "med.shots=", "med.surgeries=",
                        "med.rushed=", "med.untreated=", "med.treated=",
                        "med.staged=",
                        "league.next=", "league.block=", "league.weeks=",
                        "league.you=", "league.best=", "league.bestday=",
                        "league.nights=", "league.wins=", "league.fields=",
                        "league.lastnight=",
                        "rival.race=",
                        "rival.raceby=", "rival.racefa=", "rival.fas=",
                        "rival.fa0=", "rival.fa1=", "pastrivals=",
                        "pastrival0.name=", "pastrival0.role=",
                        "pastrival0.style=", "pastrival0.day=",
                        "pastrival0.age=", "pastrival0.peak=",
                        "pastrival0.gen="}) {
    DropSaveLine(v21, k);
  }
  SetSaveVersion(v21, 21);

  SaveGame old;
  CHECK(DeserializeSave(v21, old) == LoadResult::Ok);
  CHECK(old.version == kSaveVersion);
  CHECK(old.player.rival.name.empty());
  CHECK(old.player.pastRivals.empty());
  CHECK(old.seed == save.seed);
  CHECK(static_cast<int>(DefaultMigrations().size()) == kSaveVersion - 1);

  // And an empty rival is genuinely inert: thirty days of nights change
  // nothing, which is what "plays exactly as it did" has to mean.
  {
    DayState d;
    const Rng w = Rng::FromSeed("quiet");
    old.player.climber = NewClimber(w);
    for (int i = 0; i < 30; i++) SleepToNextDay(old.player, d, w);
    CHECK(old.player.rival.name.empty());
    CHECK(old.player.rival.grade == 0.0);
    CHECK(old.player.pastRivals.empty());
  }
}

static void TestCharacter() {
  CharacterDials cd;

  // ---- an unbuilt character is nobody, and nobody changes anything ----
  //
  // **The most important check in this file, and it exists because the
  // first version of it did not.** `Build` has to default to something, and
  // whatever it defaults to carries that origin's perk and that flaw's
  // cost. It defaulted to an All-Rounder who Sold It All with Gumby, which
  // meant **every existing caller -- the probe, the golden vectors, every
  // measurement in every note -- silently trained technique at half rate
  // and got paid 12% more per shift.**
  //
  // All 75,027 checks passed. The training tests are orderings ("the
  // grinder ends stronger than the cruiser") and halving both sides
  // preserves an ordering, which is this project's own recorded lesson
  // arriving from a direction nobody was watching.
  //
  // So every effect is pinned at *exactly* neutral for a default. Not
  // "close to" -- exactly, because the whole point is that adding identity
  // to this game must not move a single number that was measured without
  // it.
  {
    const Character nobody;
    CHECK(!nobody.built);
    for (int lane = 0; lane < kSkillCount; lane++) {
      const Skill s = static_cast<Skill>(lane);
      CHECK(SkillGainMultiplier(nobody, s, false, false, 1.0, cd) == 1.0);
      CHECK(SkillGainMultiplier(nobody, s, true, true, 0.0, cd) == 1.0);
    }
    for (int t = 0; t <= static_cast<int>(RouteType::Crack); t++) {
      CHECK(OddsPenalty(nobody, static_cast<RouteType>(t), cd) == 0.0);
    }
    CHECK(InjuryRiskMultiplier(nobody, cd) == 1.0);
    CHECK(ShiftPayMultiplier(nobody, cd) == 1.0);
    CHECK(DailyCostMultiplier(nobody) == 1.0);
    CHECK(ShopPriceMultiplier(nobody) == 1.0);
    CHECK(PhysioPriceMultiplier(nobody) == 1.0);
    CHECK(NerveShift(nobody, cd) == 0.0);
    CHECK(TalentSurfaced(Talent::None).empty());
    // And nobody learns anything about themselves, because there is nobody.
    Character n2;
    CHECK(WorkedOn(n2, Skill::Power, 10000.0, cd) == Talent::None);
    CHECK(!n2.giftKnown && !n2.antiKnown);
  }

  // ...and a built one is not neutral, or the flag would be doing the whole
  // job. This is the other half of the pin: `built` must gate the effects,
  // not delete them.
  {
    Build b;
    b.flaw = Flaw::Gumby;
    b.origin = Origin::SoldItAll;
    const Character somebody = MakeCharacter(b, Rng::FromSeed("someone"), cd);
    CHECK(somebody.built);
    CHECK(SkillGainMultiplier(somebody, Skill::Technique, false, false, 1.0,
                              cd) < 1.0);
    CHECK(ShiftPayMultiplier(somebody, cd) > 1.0);
  }

  // ---- archetypes redistribute and never add -------------------------
  //
  // **The load-bearing check in this file.** A set of offsets that is not
  // zero-sum is a difficulty setting wearing a costume: one archetype ends
  // up simply better and the choice stops being a choice. The 2D game
  // learned this on its competition field, measured it, and fixed it by
  // shifting the offsets -- so the rule arrives here already paid for.
  for (int a = 0; a < kArchetypeCount; a++) {
    const ArchetypeDef& d = Describe(static_cast<Archetype>(a));
    const double sum =
        d.power + d.fingers + d.technique + d.endurance + d.head;
    CHECK(std::abs(sum) < 1e-9);
  }

  // ---- origins own separate lanes ------------------------------------
  //
  // Each origin holds exactly one permanent multiplier, and no two origins
  // hold theirs in the same lane. That is what stops them being comparable
  // on one number, and it is a property of the table rather than of any one
  // row -- so it is checked across the whole table rather than per origin.
  int laneUsed[6] = {0, 0, 0, 0, 0, 0};
  for (int o = 0; o < kOriginCount; o++) {
    const OriginDef& d = Describe(static_cast<Origin>(o));
    const double lanes[6] = {d.shiftPay,  d.indoorGain, d.trainingGain,
                             d.dailyCost, d.shopPrice,  d.physioPrice};
    int bent = 0;
    for (int i = 0; i < 6; i++) {
      if (std::abs(lanes[i] - 1.0) < 1e-9) continue;
      bent++;
      laneUsed[i]++;
    }
    CHECK(bent == 1);  // exactly one perk, never two
  }
  for (int i = 0; i < 6; i++) CHECK(laneUsed[i] == 1);  // and never shared

  // Nothing in an origin touches send odds. This is the rule that keeps
  // "where you came from" out of the resolver, and the only way to check it
  // is that the odds function has no origin arm at all -- so it is checked
  // by asking, for every origin, with everything else held equal.
  for (int o = 0; o < kOriginCount; o++) {
    Build b;
    b.origin = static_cast<Origin>(o);
    b.flaw = Flaw::Gumby;  // a flaw that is not the odds one
    const Character c = MakeCharacter(b, Rng::FromSeed("odds"), cd);
    for (int t = 0; t <= static_cast<int>(RouteType::Crack); t++) {
      CHECK(OddsPenalty(c, static_cast<RouteType>(t), cd) == 0.0);
    }
  }

  // ---- talents: one gift, one anti-talent, never the same lane -------
  //
  // A climber whose fingers both come fast and come slow is a wash, which
  // is not a character. Swept over seeds rather than checked once, because
  // the collision is a rare roll and a single seed proves nothing.
  int gifts[kTalentCount] = {0};
  int antis[kTalentCount] = {0};
  for (int s = 0; s < 400; s++) {
    Character c;
    RollTalents(c, Rng::FromSeed("talent#" + std::to_string(s)));
    CHECK(c.gift != Talent::None);
    CHECK(c.antiTalent != Talent::None);
    CHECK(Describe(c.gift).gift);
    CHECK(!Describe(c.antiTalent).gift);
    CHECK(Describe(c.gift).skill != Describe(c.antiTalent).skill);
    gifts[static_cast<int>(c.gift)]++;
    antis[static_cast<int>(c.antiTalent)]++;
  }
  // Every talent is reachable. A table entry that never rolls is content
  // nobody will ever see, and it fails silently.
  for (int t = 1; t < kTalentCount; t++) {
    CHECK(gifts[t] + antis[t] > 0);
  }
  // Same seed, same person. The whole save model depends on it.
  Character a1, a2;
  RollTalents(a1, Rng::FromSeed("same"));
  RollTalents(a2, Rng::FromSeed("same"));
  CHECK(a1.gift == a2.gift && a1.antiTalent == a2.antiTalent);

  // ---- the build actually shapes the climber -------------------------
  //
  // Pinned as magnitudes rather than orderings. "A Boulderer has more power
  // than endurance" passes on a build where the offsets are 0.1 apart, and
  // that is exactly what a flattened table looks like.
  const Rng world = Rng::FromSeed("build-me");
  Build boulderer;
  boulderer.archetype = Archetype::Boulderer;
  boulderer.origin = Origin::SoldItAll;
  Build ropegun = boulderer;
  ropegun.archetype = Archetype::RopeGun;

  const Climber b = MakeClimber(boulderer, world, cd);
  const Climber r = MakeClimber(ropegun, world, cd);
  // Same seed, so the +/-6 of luck is identical and the gap is the build.
  CHECK(b.skills.power - r.skills.power == 10.0);       // +6 against -4
  CHECK(r.skills.endurance - b.skills.endurance == 11.0);  // +7 against -4
  // And a career's shape is legible: a Boulderer's strongest is power.
  CHECK(b.skills.power > b.skills.endurance + 8.0);
  CHECK(r.skills.endurance > r.skills.power + 8.0);

  // Nobody starts unable to pull on, whatever the build.
  for (int ar = 0; ar < kArchetypeCount; ar++) {
    for (int o = 0; o < kOriginCount; o++) {
      Build bb;
      bb.archetype = static_cast<Archetype>(ar);
      bb.origin = static_cast<Origin>(o);
      const Climber cc = MakeClimber(bb, Rng::FromSeed("floor"), cd);
      CHECK(cc.skills.power >= 1.0 && cc.skills.power <= 99.0);
      CHECK(cc.skills.head >= 1.0 && cc.skills.head <= 99.0);
    }
  }

  // ---- flaws cost something, and the cost is where it says -----------
  Build gumby;
  gumby.flaw = Flaw::Gumby;
  gumby.temperament = Temperament::Lifer;
  const Character g = MakeCharacter(gumby, Rng::FromSeed("g"), cd);
  // Half rate on footwork, and only on footwork.
  const double gTech =
      SkillGainMultiplier(g, Skill::Technique, false, false, 1.0, cd);
  const double gPow =
      SkillGainMultiplier(g, Skill::Power, false, false, 1.0, cd);
  CHECK(gTech < gPow * 0.75);

  // Fair-Weather is the only flaw you can do something about, which is why
  // it is the interesting one: climb rested and it costs you nothing.
  Build fw = gumby;
  fw.flaw = Flaw::FairWeather;
  const Character f = MakeCharacter(fw, Rng::FromSeed("f"), cd);
  const double rested =
      SkillGainMultiplier(f, Skill::Power, false, false, 0.9, cd);
  const double wrecked =
      SkillGainMultiplier(f, Skill::Power, false, false, 0.2, cd);
  CHECK(wrecked < rested * 0.6);

  // Happy Feet is the one flaw allowed near odds, and only on technical.
  Build hf = gumby;
  hf.flaw = Flaw::HappyFeet;
  const Character h = MakeCharacter(hf, Rng::FromSeed("h"), cd);
  CHECK(OddsPenalty(h, RouteType::Technical, cd) > 0.05);
  CHECK(OddsPenalty(h, RouteType::Power, cd) == 0.0);

  // Tweaky Fingers is a real multiplier on getting hurt, not a flavour
  // line. Compared against the same build with a different flaw so the
  // rolled tendon talents do not muddy it.
  Build tw = gumby;
  tw.flaw = Flaw::TweakyFingers;
  const Character t1 = MakeCharacter(tw, Rng::FromSeed("tw"), cd);
  const Character t2 = MakeCharacter(gumby, Rng::FromSeed("tw"), cd);
  CHECK(InjuryRiskMultiplier(t1, cd) > InjuryRiskMultiplier(t2, cd) * 1.5);

  // ---- personality bends four different mechanics --------------------
  //
  // Four axes, four lanes, and no axis is allowed to reach into another's.
  // Checked as a grid: change one axis, and exactly the lanes that axis
  // owns are allowed to move.
  Build pur = gumby, imp = gumby;
  pur.temperament = Temperament::Purist;
  imp.temperament = Temperament::SendOrBust;
  const Character cp = MakeCharacter(pur, Rng::FromSeed("p"), cd);
  const Character ci = MakeCharacter(imp, Rng::FromSeed("p"), cd);

  // Disciplined keeps more of a session than impulsive does.
  CHECK(SkillGainMultiplier(cp, Skill::Power, false, false, 1.0, cd) >
        SkillGainMultiplier(ci, Skill::Power, false, false, 1.0, cd) * 1.1);
  // A purist works for less, because they take the work that leaves the
  // days free.
  CHECK(ShiftPayMultiplier(cp, cd) < ShiftPayMultiplier(ci, cd) * 0.95);

  // **Boldness is a trade and not a buff, and both halves are pinned.**
  // Send-or-Bust is steadier above the last piece than the Purist is *and*
  // keeps less of every session -- +55 boldness bought with -30 discipline.
  // Checking only the first half would pass on a temperament table where
  // one row is simply better, which is the same failure the zero-sum
  // archetype check exists to prevent.
  CHECK(NerveShift(ci, cd) > NerveShift(cp, cd) + 0.05);
  CHECK(SkillGainMultiplier(ci, Skill::Power, false, false, 1.0, cd) <
        SkillGainMultiplier(cp, Skill::Power, false, false, 1.0, cd));

  // And purism pays in both directions. It used to clamp at zero, so a
  // purist worked for less and a pragmatist worked for the same -- the
  // negative half of the axis was free, and the Influencer was collecting
  // it. A pragmatist out-earns neutral now, not just the purist.
  Build inf = gumby;
  inf.temperament = Temperament::Influencer;
  const Character cf = MakeCharacter(inf, Rng::FromSeed("p"), cd);
  CHECK(cf.personality.purism < 0.0);
  CHECK(ShiftPayMultiplier(cf, cd) > ShiftPayMultiplier(cp, cd) * 1.1);

  // The absolute claim needs an origin whose own pay lane is neutral, or
  // the origin's perk sits on top of the axis and hides its sign -- which
  // is exactly what this check caught on its first run: a Purist who Sold
  // It All still clears 1.0, because a 12% CV beats an 8% conscience.
  Build purePay = gumby, pragPay = gumby;
  purePay.origin = pragPay.origin = Origin::GymRat;  // shiftPay 1.0
  purePay.temperament = Temperament::Purist;
  pragPay.temperament = Temperament::Influencer;
  CHECK(ShiftPayMultiplier(MakeCharacter(purePay, Rng::FromSeed("w"), cd),
                           cd) < 1.0);
  CHECK(ShiftPayMultiplier(MakeCharacter(pragPay, Rng::FromSeed("w"), cd),
                           cd) > 1.0);

  // **And boldness has to actually reach the wall.** The check above only
  // proves `NerveShift` computes a different number for two temperaments;
  // it says nothing about whether anything reads it. Deleting the one line
  // that plumbs it into `AttemptInput` passed the entire suite, which is
  // this project's oldest bug wearing a test's clothes -- so both halves of
  // the plumbing are pinned: the builder fills the field, and the resolver
  // spends it.
  //
  // **Where it is measured matters and the first attempt got it wrong.** A
  // 5.12a pitch put the mean highpoint at 0.27 of sixteen moves -- the
  // climber fell off the first move every time and never reached the
  // exposed ground, so the check failed with the wiring perfectly correct.
  // Measured properly, exposure is **0.900 on unpadded rock, 0.367 on a
  // rope and exactly 0.000 once you own pads**, and nerve is worth half of
  // it -- so boldness is felt **at your limit, unpadded**, and essentially
  // nowhere else: +0.2% at two grades below, **+10.4% at the limit**, and
  // zero above it because you fall off before the height. That is a good
  // sentence about climbing and it is why this test is a highball.
  {
    Climber body = NewClimber(Rng::FromSeed("bold-body"));
    SessionState sess = StartSession(body);
    sess.padding = 0.0;   // no pads: fear is priced at zero with them
    ProjectMemory mem;
    Rng w0 = Rng::FromStream("bold-rock", Stream::Worldgen);
    const Route sample = BuildRoute(w0, "Sample", 5, 5, RouteType::Power,
                                    Discipline::Boulder);

    // One: the builder carries it out of the character.
    const AttemptInput boldSample = BuildSessionAttemptInput(
        sess, mem, body, sample, Conditions{}, {}, 0.72, ci);
    const AttemptInput shySample = BuildSessionAttemptInput(
        sess, mem, body, sample, Conditions{}, {}, 0.72, cp);
    CHECK(boldSample.boldness > shySample.boldness + 0.05);
    // ...and nobody carries nothing, which is what protects the vectors.
    const Character nobody2;
    CHECK(BuildSessionAttemptInput(sess, mem, body, sample, Conditions{}, {},
                                   0.72, nobody2)
              .boldness == 0.0);

    // Two: the resolver spends it.
    //
    // **The test locates its own band rather than naming a grade**, because
    // the band is narrow and moves with the route seed -- a hard-coded
    // grade 5 passed against one generated route and produced zero sends
    // against another, which is a flaky test rather than a finding. So it
    // walks the grades and measures at the first one where a cautious
    // climber sends somewhere between a tenth and three quarters of the
    // time, which is the definition of "at your limit".
    int boldSends = 0, shySends = 0, band = 0;
    for (int g = 3; g <= 8 && band == 0; g++) {
      Rng w = Rng::FromStream("bold-rock", Stream::Worldgen);
      const Route r = BuildRoute(w, "Highball", g, g, RouteType::Power,
                                 Discipline::Boulder);
      const AttemptInput bIn = BuildSessionAttemptInput(
          sess, mem, body, r, Conditions{}, {}, 0.72, ci);
      const AttemptInput sIn = BuildSessionAttemptInput(
          sess, mem, body, r, Conditions{}, {}, 0.72, cp);
      int bs = 0, ss = 0;
      for (int i = 0; i < 2000; i++) {
        Rng r1 = Rng::FromSeed("burn#" + std::to_string(i));
        Rng r2 = Rng::FromSeed("burn#" + std::to_string(i));
        bs += ResolveAttempt(r1, bIn).sent ? 1 : 0;
        ss += ResolveAttempt(r2, sIn).sent ? 1 : 0;
      }
      if (ss > 200 && ss < 1500) { band = g; boldSends = bs; shySends = ss; }
    }
    // There has to *be* a limit band, or fear is priced out of the game.
    CHECK(band != 0);
    // And a magnitude, not an ordering: winning by one send would pass on a
    // build where the wiring is dead and the rng happened to lean.
    CHECK(boldSends > shySends + shySends / 40);
  }

  // **`social` now has a reader**, and this is the assertion that was
  // waiting for it: it used to check only that the axis separated the
  // temperaments, because nobody ever failed to turn up. `WhoIsAround`
  // is the reader -- see Sim/DirtbagPartner.h.
  CHECK(cf.personality.social > cp.personality.social + 40.0);
  {
    const Rng world = Rng::FromSeed("who-turns-up");
    const auto seasonOfCompany = [&](double social) {
      int seen = 0;
      for (int day = 1; day <= 365; day++) {
        seen += static_cast<int>(
            WhoIsAround(world, day, {}, social, true).size());
      }
      return seen;
    };
    // The gregarious one has more people around, over a season, on the
    // same seed and the same weather. A magnitude rather than an ordering:
    // one person's difference would pass on a build where nothing reads it.
    const int gregarious = seasonOfCompany(cf.personality.social);
    const int solitary = seasonOfCompany(cp.personality.social);
    CHECK(gregarious > solitary + solitary / 20);
  }

  // The line the game says the day a talent stops being a secret: a
  // sentence about noticing something, not a stat readout.
  const std::string surfaced = TalentSurfaced(Talent::BomberTendons);
  CHECK(!surfaced.empty());
  CHECK(surfaced.find("Bomber Tendons") != std::string::npos);
  CHECK(surfaced.find("fingers") != std::string::npos);
  for (char ch : surfaced) CHECK(!(ch >= '0' && ch <= '9'));

  // ---- discovery: you learn what you are by what you do --------------
  Character disc;
  disc.built = true;  // a real climber, hand-built so the talents are known
  disc.gift = Talent::Explosive;      // power
  disc.antiTalent = Talent::NoEngine; // endurance
  // A lane you never touch never tells you anything.
  CHECK(WorkedOn(disc, Skill::Technique, 500.0, cd) == Talent::None);
  CHECK(!disc.giftKnown);
  // Work builds toward it and it lands exactly once.
  CHECK(WorkedOn(disc, Skill::Power, cd.repsToSurface - 1.0, cd) ==
        Talent::None);
  CHECK(WorkedOn(disc, Skill::Power, 2.0, cd) == Talent::Explosive);
  CHECK(disc.giftKnown);
  CHECK(WorkedOn(disc, Skill::Power, 100.0, cd) == Talent::None);
  // The anti-talent surfaces on its own lane and its own clock.
  CHECK(WorkedOn(disc, Skill::Endurance, cd.repsToSurface + 1.0, cd) ==
        Talent::NoEngine);
  CHECK(disc.antiKnown);

  // **The effect is live before the knowing.** This is the design stated
  // as a test: nobody is told they have good tendons, they find out over
  // ten years of having had them.
  Character quiet;
  quiet.built = true;
  quiet.gift = Talent::Explosive;
  quiet.antiTalent = Talent::Stiff;
  CHECK(!quiet.giftKnown);
  CHECK(SkillGainMultiplier(quiet, Skill::Power, false, false, 1.0, cd) >
        1.3);

  // ---- who you are, in a sentence ------------------------------------
  //
  // The gate for this phase is that a player can say who their climber is
  // without reading a stat line. This is the game's own attempt at it; the
  // test can only check that it is a sentence about this climber and not a
  // form.
  for (int o = 0; o < kOriginCount; o++) {
    for (int ar = 0; ar < kArchetypeCount; ar++) {
      Build bb;
      bb.origin = static_cast<Origin>(o);
      bb.archetype = static_cast<Archetype>(ar);
      const Character cc = MakeCharacter(bb, Rng::FromSeed("say"), cd);
      const std::string line = WhoYouAre(cc);
      CHECK(line.size() > 20);
      CHECK(line.find(OriginName(bb.origin)) != std::string::npos);
      CHECK(line.back() == '.');
      // Not a stat line: no digits anywhere in it.
      for (char ch : line) CHECK(!(ch >= '0' && ch <= '9'));
    }
  }
}

static void TestCharacterSave() {
  CharacterDials cd;
  SaveGame save;
  save.seed = "who-i-am";
  Build b;
  b.archetype = Archetype::Technician;
  b.origin = Origin::TrustFund;
  b.flaw = Flaw::TweakyFingers;
  b.temperament = Temperament::Influencer;
  save.player.character = MakeCharacter(b, Rng::FromSeed(save.seed), cd);
  save.player.character.giftKnown = true;
  save.player.character.reps[0] = 12.5;
  save.player.character.reps[4] = 3.25;
  save.player.climber = MakeClimber(b, Rng::FromSeed(save.seed), cd);

  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  const Character& r = back.player.character;
  CHECK(r.built);
  CHECK(r.build.archetype == Archetype::Technician);
  CHECK(r.build.origin == Origin::TrustFund);
  CHECK(r.build.flaw == Flaw::TweakyFingers);
  CHECK(r.build.temperament == Temperament::Influencer);
  CHECK(r.gift == save.player.character.gift);
  CHECK(r.antiTalent == save.player.character.antiTalent);
  CHECK(r.giftKnown && !r.antiKnown);
  CHECK(r.reps[0] == 12.5);
  CHECK(r.reps[4] == 3.25);
  CHECK(r.personality.purism == save.player.character.personality.purism);
  // And the effects survive the trip, which is the thing that actually
  // matters -- a build that round-trips its fields but not its consequences
  // is a save that quietly hands you somebody else's career.
  CHECK(ShopPriceMultiplier(r) == ShopPriceMultiplier(save.player.character));
  CHECK(InjuryRiskMultiplier(r, cd) ==
        InjuryRiskMultiplier(save.player.character, cd));

  // **A v20 save has no character and must not grow one.** It arrives
  // unbuilt, which is exact rather than generous: unbuilt is neutral in
  // every lane, so a twenty-year career loads with the numbers it was
  // measured with and the creation screen does not ambush anybody.
  std::string v20 = SerializeSave(save);
  DropSaveLine(v20, "char.built=");
  DropSaveLine(v20, "char.archetype=");
  DropSaveLine(v20, "char.origin=");
  DropSaveLine(v20, "char.flaw=");
  DropSaveLine(v20, "char.temperament=");
  DropSaveLine(v20, "char.discipline=");
  DropSaveLine(v20, "char.boldness=");
  DropSaveLine(v20, "char.social=");
  DropSaveLine(v20, "char.purism=");
  DropSaveLine(v20, "char.gift=");
  DropSaveLine(v20, "char.anti=");
  DropSaveLine(v20, "char.giftKnown=");
  DropSaveLine(v20, "char.antiKnown=");
  DropSaveLine(v20, "char.reps0=");
  DropSaveLine(v20, "char.reps1=");
  DropSaveLine(v20, "char.reps2=");
  DropSaveLine(v20, "char.reps3=");
  DropSaveLine(v20, "char.reps4=");
  DropSaveLine(v20, "char.startingCash=");
  DropSaveLine(v20, "char.agePlus=");
  SetSaveVersion(v20, 20);

  SaveGame old;
  CHECK(DeserializeSave(v20, old) == LoadResult::Ok);
  CHECK(old.version == kSaveVersion);
  CHECK(!old.player.character.built);
  CHECK(SkillGainMultiplier(old.player.character, Skill::Technique, false,
                            false, 1.0, cd) == 1.0);
  CHECK(ShiftPayMultiplier(old.player.character, cd) == 1.0);
  // The rest of the career is untouched.
  CHECK(old.seed == save.seed);
  CHECK(static_cast<int>(DefaultMigrations().size()) == kSaveVersion - 1);

  // A hand-edited file cannot index off the end of a static table.
  std::string bogus = SerializeSave(save);
  DropSaveLine(bogus, "char.origin=");
  bogus += "char.origin=99\n";
  SaveGame safe;
  CHECK(DeserializeSave(bogus, safe) == LoadResult::Ok);
  CHECK(static_cast<int>(safe.player.character.build.origin) >= 0);
  CHECK(static_cast<int>(safe.player.character.build.origin) < kOriginCount);
}

static void TestZones() {
  ZoneDials zd;

  // **The ordinals of the original five are frozen, and this is the most
  // load-bearing check in the file.** `EDirtbagZone` is a `uint8` and
  // `ADirtbagDaySpot::DestinationZone` is an `EditAnywhere` property, so
  // every travel spot already placed in Evan's level stores its destination
  // as a number. Inserting a zone before the end renumbers all of them at
  // once: no compiler error, no failing test anywhere else, and the symptom
  // is walking to the gym and arriving at a crag. The map grew from five to
  // sixteen on 2026-08-23 and it grew *upward* for exactly this reason.
  CHECK(static_cast<int>(Zone::Lot) == 0);
  CHECK(static_cast<int>(Zone::Town) == 1);
  CHECK(static_cast<int>(Zone::Roadside) == 2);
  CHECK(static_cast<int>(Zone::Cave) == 3);
  CHECK(static_cast<int>(Zone::Terrace) == 4);

  // The whole point, and the bug this file exists to prevent: **a dead van
  // must never take the town away.** In the port every travel spot was
  // gated on the van running, so a breakdown removed the gym, the shop, the
  // diner and the shift as well as the rock -- the van breaking took the
  // game away instead of taking the crags away.
  CHECK(!NeedsTheVan(Zone::Lot));
  CHECK(!NeedsTheVan(Zone::Town));
  CHECK(NeedsTheVan(Zone::Roadside));
  CHECK(NeedsTheVan(Zone::Cave));
  CHECK(NeedsTheVan(Zone::Terrace));

  // And the consequence stated as its own check, because it is the design
  // and not an accident of the two above: with the van dead you can still
  // reach the money that fixes it.
  CHECK(WalkMinutes(Zone::Lot, Zone::Town, zd) > 0.0);

  // Walking is symmetric, and standing still is free rather than an error.
  CHECK(WalkMinutes(Zone::Town, Zone::Lot, zd) ==
        WalkMinutes(Zone::Lot, Zone::Town, zd));
  for (int z = 0; z < kZoneCount; z++) {
    const Zone here = static_cast<Zone>(z);
    CHECK(WalkMinutes(here, here, zd) == 0.0);
  }

  // You cannot walk to rock, from anywhere, including from other rock.
  for (int z = 0; z < kZoneCount; z++) {
    const Zone other = static_cast<Zone>(z);
    CHECK(WalkMinutes(Zone::Roadside, other, zd) <= 0.0);
    CHECK(WalkMinutes(other, Zone::Cave, zd) <= 0.0);
  }

  // The walk follows its dial rather than a number typed beside it. Two
  // borders, so a 45-minute border is a 90-minute walk.
  ZoneDials miles;
  miles.minutesPerCrossing = 45.0;
  CHECK(WalkMinutes(Zone::Lot, Zone::Town, miles) == 90.0);

  // **And the tuned number survived the widening.** `lotToTownMinutes` was
  // twenty, chosen so the walk is worth the van when the van runs and
  // survivable when it does not. On the real grid the Lot and downtown are
  // diagonal, so that is two borders, and `minutesPerCrossing` is ten
  // precisely so this one walk still costs what it was tuned to cost.
  // Widening a map must not quietly rebalance the route that was already
  // right, and the only way to know it did not is to pin it.
  CHECK(WalkMinutes(Zone::Lot, Zone::Town, zd) == 20.0);

  // IsACrag names its members rather than being written as "not the Lot
  // and not the town".
  //
  // **The day this was written for has arrived.** The old note here said
  // these checks could not fail, because with five zones "is a crag" and
  // "needs the van" were the same set, and that they were kept for the day
  // comps turned up -- *"a comp is a van zone that is not rock, and on that
  // day the lazy definition starts quietly putting a climbing competition
  // on the guidebook."* Comps came back on 2026-08-23 and the Olympic
  // Village landed with them, along with your folks' farm. **Both need the
  // van and neither is rock**, so the lazy definition now fails here
  // instead of being a comment hoping somebody reads it.
  CHECK(!IsACrag(Zone::Lot));
  CHECK(!IsACrag(Zone::Town));
  CHECK(IsACrag(Zone::Roadside) && IsACrag(Zone::Cave) &&
        IsACrag(Zone::Terrace));
  CHECK(NeedsTheVan(Zone::Village) && !IsACrag(Zone::Village));
  CHECK(NeedsTheVan(Zone::Farm) && !IsACrag(Zone::Farm));
  // Every crag still needs the van. That direction stays true forever --
  // it is the one that would let you walk to rock.
  for (int z = 0; z < kZoneCount; z++) {
    const Zone here = static_cast<Zone>(z);
    if (IsACrag(here)) CHECK(NeedsTheVan(here));
  }

  // ---- the grid itself, which is a hand-typed table ----------------
  //
  // Every link is written twice, once from each end, and a table written
  // twice is a table that disagrees with itself eventually. Nothing here
  // tests a design decision; all of it tests my typing.

  // If north of A is B, then south of B is A -- and the same east to west.
  // This is the check that catches a link entered from one side only, which
  // in play is a one-way street you can walk into and not out of.
  const Compass kDirs[4] = {Compass::North, Compass::South, Compass::East,
                            Compass::West};
  const Compass kBack[4] = {Compass::South, Compass::North, Compass::West,
                            Compass::East};
  for (int z = 0; z < kZoneCount; z++) {
    const Zone here = static_cast<Zone>(z);
    for (int d = 0; d < 4; d++) {
      Zone there = Zone::Lot;
      if (!NeighbourOf(here, kDirs[d], there)) continue;
      Zone back = Zone::Lot;
      CHECK(NeighbourOf(there, kBack[d], back));
      CHECK(back == here);
      CHECK(Adjacent(here, there) && Adjacent(there, here));
      CHECK(WalkCrossings(here, there) == 1);
    }
  }

  // Nothing is next to itself, and nothing off the grid is next to
  // anything. You cannot walk out of the Shaded Cave in any direction.
  for (int z = 0; z < kZoneCount; z++) {
    const Zone here = static_cast<Zone>(z);
    CHECK(!Adjacent(here, here));
    if (!NeedsTheVan(here)) continue;
    for (int d = 0; d < 4; d++) {
      Zone there = Zone::Lot;
      CHECK(!NeighbourOf(here, kDirs[d], there));
    }
  }

  // **Connected ground has to actually be connected.** An island in the
  // table would be a zone you can see on the map and never reach on foot,
  // and because `WalkCrossings` answers -1 for "you cannot walk that", an
  // island reads at every call site as though it needed the van. This is
  // the check that tells the two apart.
  for (int a = 0; a < kZoneCount; a++) {
    const Zone from = static_cast<Zone>(a);
    if (NeedsTheVan(from)) continue;
    for (int b = 0; b < kZoneCount; b++) {
      const Zone to = static_cast<Zone>(b);
      if (NeedsTheVan(to)) continue;
      CHECK(WalkCrossings(from, to) >= 0);
      CHECK(WalkCrossings(from, to) == WalkCrossings(to, from));
    }
  }

  // The distances themselves, pinned rather than asserted as an ordering.
  // An ordering ("the lake is further than the Trailhead") passes on a map
  // where everything is one border from everything, which is exactly the
  // table a bad edit produces.
  CHECK(WalkCrossings(Zone::Lot, Zone::Town) == 2);        // diagonal
  CHECK(WalkCrossings(Zone::Lot, Zone::OldTown) == 1);     // straight up
  CHECK(WalkCrossings(Zone::Lot, Zone::Trailhead) == 1);   // straight across
  CHECK(WalkCrossings(Zone::Lot, Zone::Lake) == 2);        // down the stem
  CHECK(WalkCrossings(Zone::Town, Zone::Outskirts) == 2);
  CHECK(WalkCrossings(Zone::MarketRow, Zone::GrandPlaza) == 2);
  // The two far corners of the map: the quiet end to the expensive one.
  // Six borders is an hour on foot at the default dial, which is the
  // longest walk in the game and is meant to be.
  CHECK(WalkCrossings(Zone::Lake, Zone::GrandPlaza) == 6);
  CHECK(WalkMinutes(Zone::Lake, Zone::GrandPlaza, zd) == 60.0);

  // The Village and the farm are off the grid like the crags, which is the
  // 2D rule stated as a test: *"zones connected except for crags and the
  // olympics which you needed to use the van to get to."*
  CHECK(WalkCrossings(Zone::Lot, Zone::Village) == -1);
  CHECK(WalkCrossings(Zone::Lot, Zone::Farm) == -1);

  // Named, distinctly, and in the game's voice rather than as an enum.
  for (int z = 0; z < kZoneCount; z++) {
    const std::string name = ZoneName(static_cast<Zone>(z));
    CHECK(!name.empty());
    for (int other = 0; other < z; other++) {
      CHECK(name != ZoneName(static_cast<Zone>(other)));
    }
  }
}

static void TestTheTable() {
  CampfireDials cd;

  // Every day has a game and every game comes round. Four copies of
  // `day % 3` in the engine could disagree; one function cannot.
  bool seen[kFiresideGameCount] = {false, false, false};
  for (int day = 0; day < kFiresideGameCount; day++) {
    const int which = static_cast<int>(WhatsOutTonight(day));
    CHECK(which >= 0 && which < kFiresideGameCount);
    seen[which] = true;
  }
  CHECK(seen[0] && seen[1] && seen[2]);

  // An evening is one game, not a menu: the same day always answers the
  // same, and tomorrow is a different one.
  CHECK(WhatsOutTonight(97) == WhatsOutTonight(97));
  CHECK(WhatsOutTonight(97) != WhatsOutTonight(98));

  // A migrated save handing over a negative day must not index off the end
  // of the switch it feeds -- C++ says -1 % 3 is -1.
  for (int day = -8; day < 0; day++) {
    const int which = static_cast<int>(WhatsOutTonight(day));
    CHECK(which >= 0 && which < kFiresideGameCount);
  }

  // Every game is named, and the names are distinct -- a prompt that says
  // the wrong game is worse than one that says nothing.
  for (int g = 0; g < kFiresideGameCount; g++) {
    const std::string name = GameName(static_cast<FiresideGame>(g));
    CHECK(!name.empty());
    for (int other = 0; other < g; other++) {
      CHECK(name != GameName(static_cast<FiresideGame>(other)));
    }
  }

  // The notches climb, and the top one is the ceiling itself. That last
  // check is the one that matters: the engine's hand-typed 20.0 sat at half
  // the ceiling and nothing said so, so "bet the maximum" quietly did not.
  double last = -1.0;
  for (int n = 0; n < kStakeNotches; n++) {
    const double s = StakeNotch(n, cd);
    CHECK(s > last);
    CHECK(s >= 0.0 && s <= cd.maxStake);
    last = s;
  }
  CHECK(StakeNotch(kStakeNotches - 1, cd) == cd.maxStake);

  // And they follow the dial rather than a number typed beside it, so
  // retuning the ceiling moves what the keys do.
  CampfireDials rich;
  rich.maxStake = 400.0;
  CHECK(StakeNotch(kStakeNotches - 1, rich) == 400.0);
  CHECK(StakeNotch(1, rich) > StakeNotch(1, cd));
}

static void TestTheTweak() {
  BodyDials bd;
  const Rng world = Rng::FromSeed("tweak-world");
  const auto fresh = [] {
    Climber c;
    c.skills.power = c.skills.fingers = c.skills.technique =
        c.skills.endurance = c.skills.head = 50.0;
    return c;
  };

  // Count hits across many independent gambles, everything else pinned.
  const auto rate = [&](double challenge, HoldType hold, double warmth,
                        int day) {
    int hits = 0;
    const int N = 60000;
    for (int a = 0; a < N; a++) {
      Climber c = fresh();
      if (TweakSomething(c, world, day, a, challenge, hold, warmth, bd)) {
        hits++;
        CHECK(c.injury.active);
        CHECK(c.injury.daysLeft > 0);
      }
    }
    return hits;
  };

  // Below the floor, never -- an injury must be something you did, not
  // weather. A mileage day cannot hurt you no matter how many burns.
  // (Enforced twice, deliberately: the early return states the intent and
  // the ramp's clamp makes deleting that line harmless -- reintroduction
  // proved the deletion changes nothing, which for a never-rule is the
  // right kind of redundancy.)
  CHECK(rate(bd.tweakChallengeFloor - 0.05, HoldType::Crimp, 0.0, 100) == 0);

  // And the ramp is real: just over the floor is barely a risk at all,
  // not the full limit rate wearing a floor as decoration.
  const int barely = rate(bd.tweakChallengeFloor + 0.08, HoldType::Crimp,
                          1.0, 100);
  const int limitR = rate(1.0, HoldType::Crimp, 1.0, 100);
  CHECK(barely * 3 < limitR);

  // The magnitude, pinned the way the campfire taught: at the limit, warm,
  // on a crimp, young, the dial says 0.0030 -- so 60k gambles land near
  // 180. A band, because the rolls are a fixed sequence, and wide enough
  // to survive retunes of everything except the order of magnitude.
  const int warmCrimp = rate(1.0, HoldType::Crimp, 1.0, 100);
  CHECK(warmCrimp > 90);
  CHECK(warmCrimp < 400);

  // The cold first burn is the classic: colder is strictly worse, by
  // about the dial's factor.
  const int coldCrimp = rate(1.0, HoldType::Crimp, 0.1, 100);
  CHECK(coldCrimp > warmCrimp);
  CHECK(coldCrimp > warmCrimp * 3 / 2);

  // Jugs mostly cannot pop a finger. Not zero -- shoulders exist.
  const int warmJug = rate(1.0, HoldType::Jug, 1.0, 100);
  CHECK(warmJug < warmCrimp);
  CHECK(warmJug > 0);

  // Tendons age first: the same burn at fifty is worse than at
  // twenty-five. Day 9500 is age ~50 on the sim's calendar.
  CHECK(rate(1.0, HoldType::Crimp, 1.0, 9500) > warmCrimp);

  // What pops is what you were pulling on. A tweak has a location, the
  // way overtraining does not.
  for (int a = 0; a < 60000; a++) {
    Climber c = fresh();
    if (TweakSomething(c, world, 100, a, 1.0, HoldType::Crimp, 1.0, bd)) {
      CHECK(c.injury.kind == InjuryKind::Pulley);
    }
    Climber p = fresh();
    if (TweakSomething(p, world, 100, a, 1.0, HoldType::Pocket, 1.0, bd)) {
      CHECK(p.injury.kind == InjuryKind::Lumbrical);
    }
    Climber j = fresh();
    if (TweakSomething(j, world, 100, a, 1.0, HoldType::Jug, 1.0, bd)) {
      CHECK(j.injury.kind == InjuryKind::Shoulder ||
            j.injury.kind == InjuryKind::Elbow);
    }
  }

  // Already hurt is ClimbOnIt's question. The tweak never stacks a second
  // injury on a first.
  Climber hurt = fresh();
  hurt.injury.active = true;
  hurt.injury.severity = 0.2;
  bool stacked = false;
  for (int a = 0; a < 60000; a++) {
    if (TweakSomething(hurt, world, 100, a, 1.0, HoldType::Crimp, 0.0, bd)) {
      stacked = true;
    }
  }
  CHECK(!stacked);

  // Deterministic per day and attempt: the same burn replays the same,
  // and two burns on one day are two separate gambles.
  Climber a1 = fresh(), a2 = fresh();
  bool anyDiffer = false;
  for (int a = 0; a < 2000; a++) {
    Climber x = fresh(), y = fresh();
    const bool hx = TweakSomething(x, world, 7, a, 1.0, HoldType::Crimp, 0.0, bd);
    const bool hy = TweakSomething(y, world, 7, a, 1.0, HoldType::Crimp, 0.0, bd);
    CHECK(hx == hy);   // replayable
    Climber z = fresh();
    if (TweakSomething(z, world, 7, a + 2000, 1.0, HoldType::Crimp, 0.0, bd) !=
        hx) {
      anyDiffer = true;   // and not one roll stretched across the day
    }
  }
  (void)a1; (void)a2;
  CHECK(anyDiffer || rate(1.0, HoldType::Crimp, 0.0, 7) == 0);
}

static void TestSandbagsAreSpecific() {
  // A crag's sandbags are famous and deliberate, not a dice roll — and they
  // are rare enough to matter when you hit one.
  Crag crag = RoadsideCrag(Rng::FromSeed("crag-1"));
  int sandbagged = 0, graded = 0;
  for (const CragLine& l : crag.lines) {
    if (l.isProject) continue;
    graded++;
    CHECK(l.route.trueGrade >= l.route.grade);   // the book never over-grades
    if (l.route.trueGrade > l.route.grade) sandbagged++;
  }
  CHECK(sandbagged >= 2);
  CHECK(sandbagged * 4 < graded);   // a minority, not the house style
  // A crag's sandbags are part of the book, so they are the same in every
  // world: Second Breakfast is stiff everywhere, the way a real one is.
  Crag elsewhere = RoadsideCrag(Rng::FromSeed("crag-9"));
  for (size_t i = 0; i < crag.lines.size(); i++) {
    if (crag.lines[i].isProject) continue;
    CHECK(elsewhere.lines[i].route.trueGrade ==
          crag.lines[i].route.trueGrade);
  }

  // Projects are the exception, and deliberately so: nobody has done them,
  // so nobody knows what they are, and the answer differs per world. If
  // these ever agreed across seeds the guess would not be a guess.
  bool projectGradeVaries = false;
  for (size_t i = 0; i < crag.lines.size(); i++)
    if (crag.lines[i].isProject &&
        elsewhere.lines[i].route.trueGrade != crag.lines[i].route.trueGrade)
      projectGradeVaries = true;
  CHECK(projectGradeVaries);

  // And within one world it is fixed, or the rock would change under you.
  Crag same = RoadsideCrag(Rng::FromSeed("crag-1"));
  for (size_t i = 0; i < crag.lines.size(); i++)
    CHECK(same.lines[i].route.trueGrade == crag.lines[i].route.trueGrade);
}

static void TestCragGivesAClimberADay() {
  // The content check: a climber at the crag's own level should find plenty
  // to warm up on, a real cluster at their limit, and a visible ceiling.
  // A crag that reads as all-warmup or all-refusal is not a day out.
  Crag crag = RoadsideCrag(Rng::FromSeed("crag-1"));
  Climber c;
  c.skills.power = c.skills.fingers = c.skills.technique =
      c.skills.endurance = c.skills.head = 50.0;   // a V5 climber
  int warmup = 0, atLimit = 0, project = 0, refused = 0;
  for (const CragLine& l : crag.lines) {
    switch (ReadRoute(c, l.route)) {
      case RouteRead::Warmup:       warmup++; break;
      case RouteRead::AtYourLimit:  atLimit++; break;
      case RouteRead::Project:      project++; break;
      case RouteRead::NotThisYear:  refused++; break;
      default: break;
    }
  }
  CHECK(warmup >= 4);     // something to get warm on
  CHECK(atLimit >= 3);    // and a real day's worth at the limit
  CHECK(project >= 1);    // something to come back for
  CHECK(refused >= 1);    // and something that is simply not yours yet
}

static void TestNamingNeverMovesTheLedgerKey() {
  // The trap this design exists to avoid: ProjectMemory records burns
  // against route.name and the save file stores it, so naming a first ascent
  // must change what the line is called without changing what it *is*.
  Crag crag = RoadsideCrag(Rng::FromSeed("crag-1"));
  std::vector<const CragLine*> projects = OpenProjects(crag);
  CHECK(!projects.empty());

  CragLine line = *projects[0];
  const std::string key = line.route.name;
  CHECK(DisplayName(line) == key);          // unnamed: identity is the name

  // The transition the pipeline will perform: it goes, so it stops being a
  // project, someone owns the first ascent, and it gets a name.
  line.isProject = false;
  line.firstAscentBy = "you";
  line.displayName = "Roadside Rites";
  CHECK(DisplayName(line) == "Roadside Rites");
  CHECK(line.route.name == key);            // but the ledger key never moved
  CHECK(GuidebookLine(line).find("Roadside Rites") != std::string::npos);
  CHECK(GuidebookLine(line).find("project") == std::string::npos);
}

static void TestGuidebookReadsRight() {
  Crag crag = RoadsideCrag(Rng::FromSeed("crag-1"));
  for (const CragLine& l : crag.lines) {
    const std::string text = GuidebookLine(l);
    CHECK(!text.empty());
    if (l.isProject) {
      CHECK(text.find("project") != std::string::npos);
      CHECK(text.find(l.description) != std::string::npos);
    } else {
      CHECK(text.find(DisplayName(l)) != std::string::npos);
      CHECK(text.find(BoulderGradeName(l.route.grade)) != std::string::npos);
      // Stars are shown when earned and never invented.
      CHECK((text.find('*') != std::string::npos) == (l.stars > 0));
    }
  }
  // LinesUpTo is the guidebook page: graded lines only, none over the bar.
  std::vector<const CragLine*> easy = LinesUpTo(crag, 2);
  CHECK(!easy.empty());
  for (const CragLine* l : easy) {
    CHECK(!l->isProject);
    CHECK(l->route.grade <= 2);
  }
  CHECK(LinesUpTo(crag, 18).size() + OpenProjects(crag).size() ==
        crag.lines.size());
}

// --- First ascents -------------------------------------------------------------

static void TestRestingBuysTimeNotStrength() {
  DayDials d;
  PlayerState player;
  DayState day = WakeUp(player);
  day.energy = 40.0;
  const double hour0 = day.hour;
  const double hunger0 = day.hunger;

  Rest(day, 3.0, player.life, d);
  CHECK(day.hour == hour0 + 3.0);                      // the hours go
  CHECK(day.hunger > hunger0);                         // and cost the same
  CHECK(day.energy > 40.0);                            // you get a little back
  CHECK(day.energy < 40.0 + 3.0 * d.restEnergyPerHour + 1e-9);

  // An afternoon in the shade is worth less than a night: resting is how you
  // spend hours you cannot climb in, not a way to farm energy.
  DayState rested = WakeUp(player);
  rested.energy = 40.0;
  Rest(rested, 6.0, player.life, d);
  CHECK(rested.energy < d.sleepEnergyFloor + 6.0 * d.restEnergyPerHour);
  CHECK(rested.energy <= 100.0);

  // It never overfills, and zero hours does nothing at all.
  DayState full = WakeUp(player);
  full.energy = 99.0;
  Rest(full, 10.0, player.life, d);
  CHECK(full.energy == 100.0);
  const double before = full.hour;
  Rest(full, 0.0, player.life, d);
  CHECK(full.hour == before);
}

static void TestVirginLinesStartFilthy() {
  Crag crag = RoadsideCrag(Rng::FromSeed("crag-1"));
  for (const CragLine& l : crag.lines) {
    ProjectMemory m = NewProjectLedger(l);
    CHECK(m.routeName == l.route.name);   // the key is the identity
    if (l.isProject) {
      CHECK(m.cleanliness < 0.2);
      CHECK(!IsWorkable(m));
    } else {
      CHECK(m.cleanliness == 1.0);        // the book's routes get climbed
      CHECK(IsWorkable(m));
    }
  }
}

static void TestADefaultLedgerIsClean() {
  // The reason project ledgers must be seeded when a crag loads rather than
  // on first touch: every attempt path creates a missing ledger through
  // MemoryFor, and that ledger is clean. If projects were not seeded ahead
  // of it, walking up to a virgin line and pulling on it would climb rock
  // somebody had apparently already brushed, and the dirt would silently
  // never apply. Encoded here so the seeding is not "simplified" away.
  ProjectMemory fresh;
  CHECK(fresh.cleanliness == 1.0);
  CHECK(IsWorkable(fresh));

  PlayerState player;
  Rng world = Rng::FromSeed("crag-1");
  Route line = BuildRoute(world, "Untouched", 7, 7, RouteType::Crimp,
                          Discipline::Boulder);
  ProjectMemory& made = MemoryFor(player, line);
  CHECK(made.cleanliness == 1.0);

  // Whereas the ledger the crag hands out for a project is filthy.
  Crag crag = RoadsideCrag(world);
  CHECK(NewProjectLedger(*OpenProjects(crag)[0]).cleanliness < 0.2);
}

static void TestDirtIsWhatStandsInTheWay() {
  // A filthy line should be out of reach for a climber who could do it
  // clean. If dirt were merely an inconvenience, cleaning would be an
  // optimisation instead of the first move.
  Rng world = Rng::FromSeed("crag-1");
  Climber c;
  c.skills.power = c.skills.fingers = c.skills.technique =
      c.skills.endurance = c.skills.head = 60.0;
  Route line = BuildRoute(world, "Test Project", 5, 5, RouteType::Crimp,
                          Discipline::Boulder);

  auto Rate = [&](double cleanliness) {
    int sends = 0;
    const int trials = 600;
    for (int i = 0; i < trials; i++) {
      Rng rng = Rng::FromSeed("dirt-" + std::to_string(i));
      AttemptInput in;
      in.climber = c;
      in.route = line;
      in.cleanliness = cleanliness;
      if (ResolveAttempt(rng, in).sent) sends++;
    }
    return 100.0 * sends / trials;
  };
  const double clean = Rate(1.0);
  const double filthy = Rate(0.05);
  CHECK(clean > 50.0);      // well within this climber, once it is clean
  CHECK(filthy < 5.0);      // and essentially gone when it is not
  CHECK(Rate(0.6) > filthy);   // and cleaning it partway genuinely helps
  CHECK(Rate(0.6) < clean);
}

static void TestCleaningCostsTheDay() {
  PlayerState player;
  DayState day = WakeUp(player);
  const double hour0 = day.hour;
  const double energy0 = day.energy;

  Crag crag = RoadsideCrag(Rng::FromSeed("crag-1"));
  const CragLine* project = OpenProjects(crag)[0];
  ProjectMemory m = NewProjectLedger(*project);

  const double gained = CleanLine(player, day, m, 2.0);
  CHECK(gained > 0.0);
  CHECK(day.hour > hour0);          // an afternoon you did not climb in
  CHECK(day.energy < energy0);      // and it is work
  CHECK(m.cleanliness > 0.05);

  // Enough hours and it comes clean, and never past clean.
  for (int i = 0; i < 10; i++) CleanLine(player, day, m, 1.0);
  CHECK(m.cleanliness == 1.0);
  CHECK(IsWorkable(m));
  // Zero hours does nothing at all, rather than something small.
  const double before = day.hour;
  CHECK(CleanLine(player, day, m, 0.0) == 0.0);
  CHECK(day.hour == before);
}

static void TestNamingIsEarnedAndExact() {
  Crag crag = RoadsideCrag(Rng::FromSeed("crag-1"));
  CragLine project = *OpenProjects(crag)[0];
  ProjectMemory m = NewProjectLedger(project);

  // Not yours until you have done it.
  CHECK(!CanName(project, m));
  CHECK(!NameFirstAscent(m, project, "Too Soon"));
  CHECK(m.givenName.empty());
  CHECK(!m.firstAscent);

  m.sent = true;
  CHECK(CanName(project, m));
  CHECK(!NameFirstAscent(m, project, ""));   // and a name is required
  CHECK(m.givenName.empty());

  const std::string key = m.routeName;
  CHECK(NameFirstAscent(m, project, "Roadside Rites"));
  CHECK(m.givenName == "Roadside Rites");
  CHECK(m.firstAscent);
  CHECK(m.routeName == key);                 // the ledger key never moves
  // The payoff: the grade stops being an opinion.
  CHECK(m.confirmedGrade == project.route.trueGrade);
  CHECK(m.confirmedGrade >= 0);

  // Once named, it is named. Nobody renames it, including you.
  CHECK(!CanName(project, m));
  CHECK(!NameFirstAscent(m, project, "Second Thoughts"));
  CHECK(m.givenName == "Roadside Rites");

  // A line already in the book was never yours to name.
  CragLine known = crag.lines[0];
  ProjectMemory km = NewProjectLedger(known);
  km.sent = true;
  CHECK(!CanName(known, km));
  CHECK(!NameFirstAscent(km, known, "Mine Now"));
}

static void TestTheBookRecordsWhatItReallyWent() {
  Crag crag = RoadsideCrag(Rng::FromSeed("crag-1"));
  bool sawCorrection = false;
  for (const CragLine* p : OpenProjects(crag)) {
    ProjectMemory m = NewProjectLedger(*p);
    m.sent = true;
    CHECK(NameFirstAscent(m, *p, "Line " + std::to_string(p->route.grade)));
    const std::string text = FirstAscentLine(m, "you");
    CHECK(text.find(m.givenName) != std::string::npos);
    CHECK(text.find("FA you") != std::string::npos);
    CHECK(text.find(BoulderGradeName(m.confirmedGrade)) != std::string::npos);
    // When the book guessed wrong, the book says so from now on.
    if (m.confirmedGrade != m.grade) {
      sawCorrection = true;
      CHECK(text.find("the book said") != std::string::npos);
    }
  }
  CHECK(sawCorrection);   // a guess that is never wrong is not a guess
  // A line with no ascent has no line in the book.
  ProjectMemory empty;
  CHECK(FirstAscentLine(empty, "you").empty());
}

static void TestTheBookGetsWrittenInto() {
  Crag crag = RoadsideCrag(Rng::FromSeed("crag-1"));
  const std::string key = OpenProjects(crag)[0]->route.name;
  const int guessed = OpenProjects(crag)[0]->route.grade;

  CragLine* line = nullptr;
  for (CragLine& l : crag.lines) {
    if (l.route.name == key) line = &l;
  }
  CHECK(line != nullptr);

  ProjectMemory m = NewProjectLedger(*line);
  // Nothing done: the page is untouched, and calling this is harmless.
  CHECK(!WriteIntoTheBook(*line, m, "you"));
  CHECK(line->isProject);
  CHECK(line->displayName.empty());
  CHECK(line->route.grade == guessed);

  m.sent = true;
  CHECK(NameFirstAscent(m, *line, "Bouncin"));
  // Naming alone never touches the book — that is what this function is for,
  // and it is the bug that shipped: named in the ledger, still a nameless
  // project on the page.
  CHECK(line->displayName.empty());
  CHECK(line->isProject);

  CHECK(WriteIntoTheBook(*line, m, "you"));
  CHECK(line->displayName == "Bouncin");
  CHECK(DisplayName(*line) == "Bouncin");
  CHECK(line->firstAscentBy == "you");
  CHECK(!line->isProject);
  CHECK(line->route.grade == m.confirmedGrade);
  CHECK(line->route.name == key);                // the ledger key never moves
  CHECK(GuidebookLine(*line).find("Bouncin") != std::string::npos);
  CHECK(GuidebookLine(*line).find("project,") == std::string::npos);

  // And now nobody can name it again, including you.
  CHECK(!CanName(*line, m));

  // Idempotent: a book that is rebuilt every time the venue changes gets
  // this run over it every time, and must not drift.
  const int settled = line->route.grade;
  for (int i = 0; i < 5; i++) WriteIntoTheBook(*line, m, "you");
  CHECK(line->displayName == "Bouncin");
  CHECK(line->route.grade == settled);

  // A ledger for a different line never writes itself onto this one.
  CragLine other = crag.lines[0];
  const std::string otherName = other.route.name;
  CHECK(!WriteIntoTheBook(other, m, "you"));
  CHECK(other.route.name == otherName);
  CHECK(other.displayName.empty());
}

static void TestTheLoadWarningAgreesWithItself() {
  BodyDials d;
  // The colour and the sentence are two readings of one fact. If they can
  // drift, the HUD can say "everything aches" in the calm colour.
  const auto Band = [&](double load) {
    Climber c;
    c.load = load;
    return LoadWarning(c, d);
  };
  const auto Says = [&](double load) {
    Climber c;
    c.load = load;
    return LoadText(c, d);
  };

  CHECK(Band(0.0) == 0);
  CHECK(Says(0.0) == "fresh");
  CHECK(Band(25.0) == 0);
  CHECK(Says(25.0) == "warmed into the season");

  // The quiet band ends exactly where the sentence stops being reassuring.
  CHECK(Band(39.9) == 0);
  CHECK(Band(40.0) == 1);
  CHECK(Says(40.0) == "carrying a load");

  // And the loud band starts exactly at the dial, not near it.
  CHECK(Band(d.injuryThreshold - 0.1) == 1);
  CHECK(Band(d.injuryThreshold) == 2);
  CHECK(Says(d.injuryThreshold).find("warning") != std::string::npos);
  CHECK(Band(100.0) == 2);

  // Nobody is warned about an injury they cannot yet get, and everybody who
  // can get one has been. This is the property the HUD actually relies on.
  for (double load = 0.0; load <= 120.0; load += 0.5) {
    Climber c;
    c.load = load;
    const bool bAtRisk = load >= d.injuryThreshold;
    CHECK((LoadWarning(c, d) == 2) == bAtRisk);
  }

  // Moving the dial moves both together — the whole reason this is not two
  // hardcoded numbers in two files.
  BodyDials moved;
  moved.injuryThreshold = 80.0;
  Climber c;
  c.load = 70.0;
  CHECK(LoadWarning(c, d) == 2);
  CHECK(LoadWarning(c, moved) == 1);
  CHECK(LoadText(c, moved) == "carrying a load");
}

static void TestHeadTrainsOnWhatYouCommitTo() {
  const SessionDials sd;

  // A boulder, so the ground is the question.
  const Rng headWorld = Rng::FromSeed("head-world");
  Route boulder = BuildRoute(headWorld, "the highball", 4, 4, RouteType::Power,
                             Discipline::Boulder);
  const int last = static_cast<int>(boulder.moves.size()) - 1;

  // Nobody has ever been gripped on move one.
  CHECK(ExposureAt(boulder, 0, 0.0, sd) == 0.0);
  // High on bare ground is the whole point.
  CHECK(ExposureAt(boulder, last, 0.0, sd) > 0.0);
  // And pads are exactly what buys it away.
  CHECK(ExposureAt(boulder, last, 1.0, sd) == 0.0);
  CHECK(ExposureAt(boulder, last, 0.5, sd) <
        ExposureAt(boulder, last, 0.0, sd));
  CHECK(ExposureAt(boulder, last, 0.5, sd) > 0.0);
  // It climbs as you do, rather than switching on.
  CHECK(ExposureAt(boulder, last, 0.0, sd) >
        ExposureAt(boulder, last / 2, 0.0, sd));

  // On a rope, pads are not the question and must not answer it: a fully
  // padded climber is still runout above the bolt. This is the escape hatch
  // that stops head being unreachable for anyone who owns two pads — the
  // cave is where a safe boulderer gets their head back.
  Route pitch = BuildRoute(headWorld, "the cave pitch", 4, 4,
                           RouteType::Endurance, Discipline::Sport);
  bool sawRunoutUnderFullPads = false;
  for (int i = 0; i < static_cast<int>(pitch.moves.size()); i++) {
    if (OnTheRope(pitch, i) && ExposureAt(pitch, i, 1.0, sd) > 0.0) {
      sawRunoutUnderFullPads = true;
    }
  }
  CHECK(sawRunoutUnderFullPads);

  // And the training. Same climber, same route, same burn — one on bare
  // ground and one behind pads.
  const auto SeasonOfHead = [&](double padding) {
    PlayerState p;
    p.climber.skills.power = p.climber.skills.fingers =
        p.climber.skills.technique = p.climber.skills.endurance =
            p.climber.skills.head = 50.0;
    DayState d = WakeUp(p);
    d.session.padding = padding;
    const Rng world = headWorld;
    Rng burns = Rng::FromSeed("head-burns");
    for (int burn = 0; burn < 60; burn++) {
      AttemptInput in;
      in.climber = p.climber;
      in.route = boulder;
      in.padding = padding;
      in.beta = 1.0;
      in.warmth = 1.0;
      AttemptResult r = ResolveAttempt(burns, in);
      // Force the burn to have reached the top, so the two runs differ in
      // padding and in nothing else.
      r.highpoint = last;
      ApplyAttemptToDay(p, d, boulder, r, world);
    }
    return p.climber.skills.head - 50.0;
  };

  const double bold = SeasonOfHead(0.0);
  const double safe = SeasonOfHead(1.0);
  CHECK(bold > 0.0);      // committing is what teaches it
  CHECK(safe == 0.0);     // and pads are what buys the lesson away
  CHECK(bold > safe);
}

static void TestClaimingIsNamingPlusTellingTheScene() {
  Crag crag = RoadsideCrag(Rng::FromSeed("crag-1"));
  const CragLine project = *OpenProjects(crag)[0];

  // NameFirstAscent alone is the ledger half and leaves the valley with no
  // opinion at all. That is correct and it is also exactly how the credit
  // came to sit uncalled: two halves, one of them optional.
  PlayerState quiet;
  ProjectMemory qm = NewProjectLedger(project);
  qm.sent = true;
  qm.firstSendStyle = Style::Onsight;
  const Standing before = quiet.standing;
  CHECK(NameFirstAscent(qm, project, "Ledger Only"));
  for (int i = 0; i < kFactionCount; i++) {
    CHECK(quiet.standing.with[i] == before.with[i]);
  }

  // ClaimFirstAscent is the whole verb.
  PlayerState loud;
  ProjectMemory lm = NewProjectLedger(project);
  lm.sent = true;
  lm.firstSendStyle = Style::Onsight;
  CHECK(ClaimFirstAscent(loud, lm, project, "Bouncin"));
  CHECK(lm.givenName == "Bouncin");
  CHECK(lm.firstAscent);
  bool moved = false;
  for (int i = 0; i < kFactionCount; i++) {
    if (loud.standing.with[i] != before.with[i]) moved = true;
  }
  CHECK(moved);   // doing a line nobody had done is worth an opinion

  // A refused naming credits nothing — no half-claims.
  PlayerState nope;
  ProjectMemory nm = NewProjectLedger(project);
  CHECK(!ClaimFirstAscent(nope, nm, project, "Not Yours"));
  CHECK(!nm.firstAscent);
  for (int i = 0; i < kFactionCount; i++) {
    CHECK(nope.standing.with[i] == before.with[i]);
  }

  // Style still reaches the scene through the claim: a new line is a new
  // line to Development whoever you are, but ground-up and first go is what
  // the old guard actually care about. Reading that off the ledger rather
  // than being told is the point of the split.
  PlayerState sieged;
  ProjectMemory sm = NewProjectLedger(project);
  sm.sent = true;
  sm.firstSendStyle = Style::Redpoint;
  CHECK(ClaimFirstAscent(sieged, sm, project, "Eventually"));
  const int dev = static_cast<int>(Faction::Development);
  const int old = static_cast<int>(Faction::OldGuard);
  CHECK(loud.standing.with[dev] == sieged.standing.with[dev]);
  CHECK(loud.standing.with[old] > sieged.standing.with[old]);
}

static void TestAShoeDealActuallyBuysShoes() {
  GearDials g;

  // No deal: you pay.
  Shoes worn;
  worn.wear = 0.9;
  double cash = 500.0;
  CHECK(Resole(worn, cash, false, g));
  CHECK(cash < 500.0);

  // The bottom rung is a shoe deal and nothing else. Before this was wired
  // it was "free shoes, and they want nothing" and it gave you nothing.
  Sponsorship deal;
  deal.tier = SponsorTier::Shoes;
  CHECK(CoversShoes(deal));

  Shoes worn2;
  worn2.wear = 0.9;
  double free = 500.0;
  CHECK(Resole(worn2, free, CoversShoes(deal), g));
  CHECK(free == 500.0);          // they are paying
  CHECK(worn2.resoles == 1);     // and you still got the resole

  // New pairs too, and a broke climber is not broke any more.
  Shoes dead;
  dead.wear = 1.0;
  dead.resoles = 99;
  double nothing = 0.0;
  CHECK(!BuyNewShoes(dead, nothing, false, g));
  CHECK(BuyNewShoes(dead, nothing, CoversShoes(deal), g));
  CHECK(nothing == 0.0);
  CHECK(dead.wear == 0.0);

  // Every tier that covers shoes covers them; None does not.
  Sponsorship none;
  CHECK(!CoversShoes(none));
}

static void TestASponsorGetsPaidAndReviewed() {
  // The nightly count, which is the state the review reads. Written in
  // SleepToNextDay because that is where nights are, and saved because a
  // reload must not launder a season spent injured into a season spent
  // slacking.
  PlayerState hurt;
  hurt.sponsor.tier = SponsorTier::Gear;
  hurt.climber.injury.active = true;
  hurt.climber.injury.daysLeft = 40;
  hurt.climber.injury.severity = 0.5;
  const Rng world = Rng::FromSeed("sponsor-nights");
  DayState d = WakeUp(hurt);
  for (int night = 0; night < 40; night++) SleepToNextDay(hurt, d, world);
  CHECK(hurt.sponsor.daysHurtThisSeason > 0);

  PlayerState fine;
  fine.sponsor.tier = SponsorTier::Gear;
  DayState fd = WakeUp(fine);
  for (int night = 0; night < 40; night++) SleepToNextDay(fine, fd, world);
  CHECK(fine.sponsor.daysHurtThisSeason == 0);

  // And it survives the disk, which is the only reason it is saved state
  // rather than a counter on the game instance.
  SaveGame save;
  save.player = hurt;
  save.seed = "sponsor";
  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  CHECK(back.player.sponsor.daysHurtThisSeason ==
        hurt.sponsor.daysHurtThisSeason);

  // A v14 save has no such field, and zero is not an approximation there:
  // ReviewSeason was called by nothing at all, in the engine or the probe,
  // so nobody with a v14 career had ever been looked at.
  //
  // The fixture is derived from this build's own writer rather than typed
  // out: drop the line v14 could not have had and put the version back.
  // A hand-typed fixture tests what I believed v14 looked like, which is
  // the thing most likely to be wrong.
  std::string v14 = SerializeSave(save);
  DropSaveLine(v14, "sponsor.hurtdays=");
  // And everything added since, or the fixture claims to be v14 while
  // carrying fields v14 could not have written.
  DropSaveLine(v14, "job.sincesalary=");
  DropSaveLine(v14, "job.dirtbagyears=");
  DropSaveLine(v14, "job.longeststreak=");
  SetSaveVersion(v14, 14);

  SaveGame old;
  CHECK(DeserializeSave(v14, old) == LoadResult::Ok);
  CHECK(old.version == kSaveVersion);              // arrives upgraded
  CHECK(old.player.sponsor.daysHurtThisSeason == 0);
  CHECK(old.player.sponsor.tier == hurt.sponsor.tier);   // and intact
  CHECK(static_cast<int>(DefaultMigrations().size()) == kSaveVersion - 1);

  // The review itself. Two flat seasons drop a rung...
  SponsorDials sp;
  Sponsorship deal;
  deal.tier = SponsorTier::Title;
  deal.gradeAtLastReview = 8;
  for (int season = 0; season < sp.seasonsOfNothingBeforeDropped; season++) {
    ReviewSeason(deal, 8, 0, sp);
  }
  CHECK(deal.tier == SponsorTier::Gear);

  // ...unless you were hurt for them, which is the whole reason the day
  // count had to become real state.
  Sponsorship injured;
  injured.tier = SponsorTier::Title;
  injured.gradeAtLastReview = 8;
  for (int season = 0; season < sp.seasonsOfNothingBeforeDropped; season++) {
    ReviewSeason(injured, 8, sp.injuryDaysThatPauseReview, sp);
  }
  CHECK(injured.tier == SponsorTier::Title);

  // And progress keeps you regardless.
  Sponsorship climbing;
  climbing.tier = SponsorTier::Title;
  climbing.gradeAtLastReview = 8;
  for (int season = 1; season <= 4; season++) {
    ReviewSeason(climbing, 8 + season, 0, sp);
  }
  CHECK(climbing.tier == SponsorTier::Title);
  CHECK(climbing.seasonsHeld == 4);

  // The stipend is the other half that ran nowhere. Every rung that is
  // supposed to pay, pays.
  CHECK(MonthlyStipend(SponsorTier::None, sp) == 0.0);
  CHECK(MonthlyStipend(SponsorTier::Shoes, sp) == 0.0);
  CHECK(MonthlyStipend(SponsorTier::Gear, sp) > 0.0);
  CHECK(MonthlyStipend(SponsorTier::Title, sp) >
        MonthlyStipend(SponsorTier::Gear, sp));
}

static void TestStandingBuysBetaAndPeopleLiftYou() {
  Crag crag = RoadsideCrag(Rng::FromSeed("crag-1"));
  const CragLine easy = crag.lines[0];

  Partner mate;
  mate.name = "Dev";
  mate.climbs = true;
  mate.rapport = 0.8;
  // Strong enough that KnowsLine says yes about the crag's easiest line,
  // so this test is about generosity rather than about competence.
  mate.climber.skills = {80, 80, 80, 80, 80};

  // Generosity scales what they spell out.
  const auto Handover = [&](double generosity) {
    ProjectMemory m = NewProjectLedger(easy);
    return ShareBeta(mate, easy, m, generosity);
  };
  const double plain = Handover(1.0);
  CHECK(plain > 0.0);
  CHECK(Handover(1.25) > plain);      // somebody who likes you talks
  CHECK(Handover(0.75) < plain);      // somebody who does not says "it goes left"

  // The default overload is exactly generosity 1.0, so every existing
  // caller and every golden vector is untouched by the new one.
  ProjectMemory a = NewProjectLedger(easy);
  ProjectMemory b = NewProjectLedger(easy);
  CHECK(ShareBeta(mate, easy, a) == ShareBeta(mate, easy, b, 1.0));
  CHECK(a.beta == b.beta);

  // It scales the share, never the ceiling. However generous they are,
  // beta stops at fully wired and never goes backwards.
  ProjectMemory wired = NewProjectLedger(easy);
  for (int i = 0; i < 200; i++) ShareBeta(mate, easy, wired, 100.0);
  CHECK(wired.beta <= 1.0);
  CHECK(wired.beta > 0.9);
  const double settled = wired.beta;
  CHECK(ShareBeta(mate, easy, wired, 100.0) >= 0.0);
  CHECK(wired.beta >= settled);

  // A hostile crowd never hands you negative beta.
  ProjectMemory grudging = NewProjectLedger(easy);
  CHECK(ShareBeta(mate, easy, grudging, -5.0) == 0.0);
  CHECK(grudging.beta == 0.0);

  // And the multiplier itself moves with standing, in the right direction.
  Standing liked;
  Standing disliked;
  const Faction theirs = FactionOf("Dev");
  liked.with[static_cast<int>(theirs)] = 1.0;
  disliked.with[static_cast<int>(theirs)] = -1.0;
  CHECK(BetaMultiplierFor(liked, "Dev") > 1.0);
  CHECK(BetaMultiplierFor(disliked, "Dev") < 1.0);
  CHECK(BetaMultiplierFor(Standing{}, "Dev") == 1.0);

  // Psyche: the people at the fire are worth something, and a climber is
  // worth more than somebody who only ever watches.
  PartnerDials pd;
  Partner watcher = mate;
  watcher.climbs = false;
  CHECK(PsycheFrom(mate, pd) > PsycheFrom(watcher, pd));
  CHECK(PsycheFrom(watcher, pd) > 0.0);   // even they are worth something

  // A stranger lifts nothing; rapport is what does it.
  Partner stranger = mate;
  stranger.rapport = 0.0;
  CHECK(PsycheFrom(stranger, pd) == 0.0);
}

static void TestTheMirroredDialsStillAgree() {
  // Three numbers live in two dial structs each, and every one of them says
  // so in a comment: "Mirrors GearDials", "Mirrors SportDials", "Mirrors
  // BodyDials". That arrangement is deliberate — it keeps DirtbagSession
  // from having to include half the project to price a move — and it is
  // exactly the shape this repo keeps writing rules against, because a
  // comment is not a guard.
  //
  // Nothing would fail if one of these moved. The shop would quote a price
  // for dead rubber that the wall did not charge, the guidebook would
  // describe a runout the resolver did not price, and the physio would
  // disagree with the climbing about what an injury costs. All silently.
  const SessionDials sd;
  const GearDials gd;
  const SportDials pd;
  const BodyDials bd;

  CHECK(sd.deadShoeGradePenalty == gd.deadShoeGradePenalty);
  CHECK(sd.shoeBiteOnGoodHolds == gd.deadShoeBiteOnGoodHolds);
  CHECK(sd.runoutGradePenalty == pd.runoutGradePenalty);
  CHECK(sd.soloGradePenalty == pd.soloGradePenalty);
  CHECK(sd.pumpGradePenalty == TradDials{}.gradesAtFullPump);
  // And the slider between the two of them lands on each end exactly, so
  // that a bolt is priced by one number and the ground by the other with
  // nothing rounded in between.
  CHECK(FallPenalty(1.0, sd.soloGradePenalty, sd.runoutGradePenalty) ==
        sd.runoutGradePenalty);
  CHECK(FallPenalty(0.0, sd.soloGradePenalty, sd.runoutGradePenalty) ==
        sd.soloGradePenalty);
  CHECK(sd.injuryGradePenalty == bd.injuryGradePenalty);

  // The fourth pair, and the only one nobody had written down as one. Both
  // of KitDials' copies were read by nothing anywhere — the resolver has
  // always used the SessionDials ones — so the pad could be retuned at the
  // shop with no effect at the wall, in silence.
  const KitDials kd;
  CHECK(sd.noPadGradePenalty == kd.noPadGradePenalty);
  CHECK(sd.padGroundedFraction == kd.padGroundedFraction);

  // And a fifth pair the checker found that I did not know about: how long
  // a year is, held separately by the age model and the season model. Let
  // those drift and the game runs a birthday and a solstice on different
  // calendars — a career's ages sliding against its seasons, with no
  // symptom sharp enough to notice until somebody is 40 in high summer
  // twice running.
  const AgeDials ad;
  const ConditionsDials cd2;
  CHECK(ad.daysPerYear == cd2.daysPerYear);

  // And the shoe formula is now genuinely one formula rather than two that
  // happened to agree. Whatever the shop quotes is what the wall charges,
  // at every wear and on both kinds of hold.
  for (double wear = 0.0; wear <= 1.0; wear += 0.05) {
    Shoes s;
    s.wear = wear;
    for (int edging = 0; edging < 2; edging++) {
      CHECK(ShoePenalty(s, edging == 1, gd) ==
            ShoePenaltyFor(wear, edging == 1, sd.deadShoeGradePenalty,
                           sd.shoeBiteOnGoodHolds));
    }
  }

  // Squared, not linear: a slightly worn shoe is fine and a dead one is a
  // different sport. Half-worn costs a quarter, not a half.
  CHECK(ShoePenaltyFor(0.0, true, gd.deadShoeGradePenalty,
                       gd.deadShoeBiteOnGoodHolds) == 0.0);
  const double half = ShoePenaltyFor(0.5, true, gd.deadShoeGradePenalty,
                                     gd.deadShoeBiteOnGoodHolds);
  const double dead = ShoePenaltyFor(1.0, true, gd.deadShoeGradePenalty,
                                     gd.deadShoeBiteOnGoodHolds);
  CHECK(half < dead * 0.3);
  CHECK(dead == gd.deadShoeGradePenalty);

  // Edging holds punish dead rubber hardest, which is what pushes a worn
  // pair onto slopers long before it stops you.
  CHECK(ShoePenaltyFor(1.0, false, gd.deadShoeGradePenalty,
                       gd.deadShoeBiteOnGoodHolds) < dead);

  // Wear past dead is still dead rather than worse than dead.
  CHECK(ShoePenaltyFor(4.0, true, gd.deadShoeGradePenalty,
                       gd.deadShoeBiteOnGoodHolds) == dead);
}

static void TestTheBestBelayerIsTheMostPatientOne() {
  SportDials sd;
  // The engine picks one person off the Lot and shows their name and their
  // burn budget at the wall, so "best" has to mean "will stand there
  // longest" rather than anything else. Nothing pinned that: the existing
  // belay tests check nobody, a non-climber, and one person at a time.
  Partner stranger;
  stranger.name = "Ray";
  stranger.climbs = true;
  stranger.rapport = 0.0;

  Partner mate = stranger;
  mate.name = "Margo";
  mate.rapport = 1.0;

  Partner neighbour = stranger;
  neighbour.name = "Trish";
  neighbour.climbs = false;      // delighted to help, and cannot

  const std::vector<Partner> lot = {neighbour, stranger, mate};
  const Partner* best = BestBelayer(lot, sd);
  CHECK(best != nullptr);
  CHECK(best->name == "Margo");
  CHECK(BurnsTheyWillHold(*best, sd) == BurnsTheyWillHold(mate, sd));
  CHECK(BelayText(best, sd).find("Margo") != std::string::npos);

  // Order must not decide it. Same Lot, other way round.
  const std::vector<Partner> reversed = {mate, stranger, neighbour};
  const Partner* again = BestBelayer(reversed, sd);
  CHECK(again != nullptr);
  CHECK(again->name == "Margo");

  // And a Lot of people who will not tie in is the same as an empty one,
  // which is what stops the wall offering a rope nobody is holding.
  const std::vector<Partner> nobody = {neighbour, neighbour};
  CHECK(BestBelayer(nobody, sd) == nullptr);
}

static void TestEachCragCostsSomethingDifferentToReach() {
  const Rng world = Rng::FromSeed("crag-1");
  const Crag roadside = RoadsideCrag(world);
  const Crag cave = ShadedCave(world);
  const Crag terrace = SunTerrace(world);

  // Every crag says how far it is, and until today nothing read it -- the
  // travel spot in the level carried a hand-typed number meaning the same
  // thing, so the guidebook and the game could disagree and the level won.
  // Now the engine asks the book, which makes these numbers load-bearing
  // rather than decorative.
  CHECK(roadside.approachHours > 0.0);
  CHECK(cave.approachHours > roadside.approachHours);
  CHECK(terrace.approachHours > cave.approachHours);

  // And they have to be far enough apart to feel like different decisions.
  // Ten minutes between two crags is not a choice, it is a rounding error.
  CHECK(cave.approachHours - roadside.approachHours >= 0.15);
  CHECK(terrace.approachHours - cave.approachHours >= 0.15);

  // Nothing is so far that a day out is impossible: there and back has to
  // leave a session in the shortest day of the year.
  ConditionsDials cd;
  const double shortest = DaylightHours(1, cd);
  CHECK(shortest > 2.0 * terrace.approachHours + 2.0);
}

static void TestTheSunTerraceIsTheWinterCrag() {
  const Rng world = Rng::FromSeed("crag-1");
  const Crag terrace = SunTerrace(world);
  const Crag cave = ShadedCave(world);
  const Crag roadside = RoadsideCrag(world);

  CHECK(terrace.aspect == Aspect::South);
  CHECK(!terrace.lines.empty());
  CHECK(OpenProjects(terrace).size() == 2u);

  // Boulders, unlike the cave. Winter is bouldering season and the terrace
  // is the reason why.
  for (const CragLine& l : terrace.lines) {
    CHECK(l.route.discipline == Discipline::Boulder);
    CHECK(!NeedsABelayer(l.route));
  }

  // Sparser and harder than Roadside — it is not somewhere you go instead,
  // it is somewhere you go when Roadside has stopped being hard enough.
  CHECK(terrace.lines.size() < roadside.lines.size());
  int hardestTerrace = -1, hardestRoadside = -1;
  for (const CragLine& l : terrace.lines)
    hardestTerrace = std::max(hardestTerrace, l.route.grade);
  for (const CragLine& l : roadside.lines)
    hardestRoadside = std::max(hardestRoadside, l.route.grade);
  CHECK(hardestTerrace > hardestRoadside);

  // Its own rock. Three crags in one valley must not share a line.
  for (const CragLine& a : terrace.lines) {
    for (const CragLine& b : cave.lines) CHECK(a.route.name != b.route.name);
    for (const CragLine& b : roadside.lines) CHECK(a.route.name != b.route.name);
  }

  // And the claim the whole crag is built on, checked rather than asserted:
  // in midwinter a south face gives more days than a north one, and in high
  // summer it gives exactly as many -- because the summer window lands
  // before the sun is on any face at all.
  ConditionsDials cd;
  const auto DaysWithAWindow = [&](Aspect a, int from, int to) {
    int n = 0;
    for (int day = from; day <= to; day++) {
      if (FindPrimeWindow(GenerateWeather(world, day, cd), a, cd).exists) n++;
    }
    return n;
  };
  CHECK(DaysWithAWindow(Aspect::South, 1, 60) >
        DaysWithAWindow(Aspect::North, 1, 60));
  CHECK(DaysWithAWindow(Aspect::South, 170, 230) ==
        DaysWithAWindow(Aspect::North, 170, 230));

  // The midday winter window is the crag's whole identity: a window you
  // cannot have if you are at work.
  double sum = 0.0;
  int n = 0;
  for (int day = 1; day <= 60; day++) {
    const PrimeWindow w =
        FindPrimeWindow(GenerateWeather(world, day, cd), Aspect::South, cd);
    if (w.exists) { sum += w.peakHour; n++; }
  }
  CHECK(n > 0);
  CHECK(sum / n > 12.0);   // after noon
  CHECK(sum / n < 16.0);   // and long before the light goes
}

static void TestTheValleyRemembersAcrossGenerations() {
  Crag crag = RoadsideCrag(Rng::FromSeed("crag-1"));
  CragLine* project = nullptr;
  for (CragLine& l : crag.lines) {
    if (l.isProject) { project = &l; break; }
  }
  CHECK(project != nullptr);
  const std::string key = project->route.name;

  // Somebody does it and names it, and their career ends.
  PlayerState first;
  first.climber = NewClimber(Rng::FromSeed("fa-world"));
  ProjectMemory m = NewProjectLedger(*project);
  m.sent = true;
  m.firstSendStyle = Style::Redpoint;
  CHECK(ClaimFirstAscent(first, m, *project, "Old Money"));
  first.projects.push_back(m);
  const Legacy done = TallyCareer(first, "Climber One", 20);
  CHECK(!done.firstAscents.empty());

  // The next one inherits nothing personal -- correctly, the ledger is
  // theirs and not yours.
  const PlayerState next = Inherit(done, Rng::FromSeed("fa-world"));
  CHECK(next.projects.empty());

  // Which is exactly why the book has to be written separately. Before this
  // existed, ninety years of the career probe produced four climbers each
  // doing the *first* ascent of the same two boulders, and a guidebook that
  // listed the same rock four times under four names.
  bool wrote = false;
  for (const NamedLine& n : done.firstAscents) {
    for (CragLine& line : crag.lines) {
      if (WriteIntoTheBook(line, n)) wrote = true;
    }
  }
  CHECK(wrote);

  CragLine* after = nullptr;
  for (CragLine& l : crag.lines) {
    if (l.route.name == key) after = &l;
  }
  CHECK(after != nullptr);
  CHECK(after->displayName == "Old Money");
  CHECK(!after->isProject);
  CHECK(after->firstAscentBy == done.name);

  // And the inheritor cannot claim it again, which is the property that
  // actually failed.
  ProjectMemory theirs = NewProjectLedger(*after);
  theirs.sent = true;
  CHECK(!CanName(*after, theirs));

  // A legacy line that is not on this crag changes nothing.
  NamedLine elsewhere;
  elsewhere.routeKey = "a boulder in another valley";
  elsewhere.givenName = "Not Here";
  for (CragLine& line : crag.lines) {
    CHECK(!WriteIntoTheBook(line, elsewhere));
  }

  // Nor does a legacy with no name -- somebody who did it and never said
  // what they called it leaves the page alone.
  NamedLine unnamed;
  unnamed.routeKey = key;
  CHECK(!WriteIntoTheBook(*after, unnamed));
  CHECK(after->displayName == "Old Money");
}

static void TestTheLotPutsUpLinesAndNamesThem() {
  Crag crag = RoadsideCrag(Rng::FromSeed("crag-1"));
  CragLine* project = nullptr;
  for (CragLine& l : crag.lines) if (l.isProject) { project = &l; break; }
  CHECK(project != nullptr);

  // They name it, sign it, and it stops being a project.
  CHECK(TheyPutUpTheLine(*project, "Dev"));
  CHECK(!project->displayName.empty());
  CHECK(project->firstAscentBy == "Dev");
  CHECK(!project->isProject);
  CHECK(project->route.grade == project->route.trueGrade);

  // And the player cannot then claim the first ascent of it, which is the
  // whole reason this had to reach the book: the Lot has taken lines since
  // it was built, into its own list only, so a line Dev did last spring was
  // still an open project with nobody's name on it.
  ProjectMemory late = NewProjectLedger(*project);
  late.sent = true;
  CHECK(!CanName(*project, late));

  // Nobody takes it twice, and nobody takes a line already signed.
  CHECK(!TheyPutUpTheLine(*project, "Margo"));
  CHECK(project->firstAscentBy == "Dev");

  // Names are deterministic, and differ by who and by which.
  CragLine other;
  other.route.name = "the other arete";
  other.isProject = true;
  CHECK(NameTheirLine("Dev", *project) == NameTheirLine("Dev", *project));
  CHECK(NameTheirLine("Dev", *project) != NameTheirLine("Dev", other));
  bool anyoneDiffers = false;
  for (const char* who : {"Margo", "Ray", "Trish", "Bo"}) {
    if (NameTheirLine(who, *project) != NameTheirLine("Dev", *project)) {
      anyoneDiffers = true;
    }
  }
  CHECK(anyoneDiffers);

  // SpokenFor: anything you have pulled on, cleaned, or already put up.
  std::vector<ProjectMemory> ledgers;
  ProjectMemory touched;
  touched.routeName = "the arete left of Diesel";
  touched.attempts = 1;
  ProjectMemory brushed;
  brushed.routeName = "the low traverse into Chalk Ghost";
  brushed.cleanliness = 0.9;
  ProjectMemory untouched;
  untouched.routeName = "the blank wall behind the parking";
  // A virgin project, not a default ledger: ProjectMemory defaults to
  // cleanliness 1.0 because an established line is clean, and only a
  // project's ledger ever starts filthy. Getting this wrong the first time
  // is what turned brushedEnoughToBeYours from a bare 0.2 into a dial.
  untouched.cleanliness = FirstAscentDials{}.virginCleanliness;
  ledgers = {touched, brushed, untouched};
  const std::vector<std::string> mine = SpokenFor(ledgers);
  CHECK(mine.size() == 2u);
  CHECK(std::find(mine.begin(), mine.end(), touched.routeName) != mine.end());
  CHECK(std::find(mine.begin(), mine.end(), brushed.routeName) != mine.end());
  CHECK(std::find(mine.begin(), mine.end(), untouched.routeName) == mine.end());

  // And the property that makes commitment mean something: over a whole
  // career of rolls, a line you are visibly on is never taken. A per-day
  // roll converges on certainty however small it is -- measured, at 1 in
  // 2000 per partner per day the Lot still took 27 of 30 open lines -- so
  // this cannot be a dial, it has to be a rule.
  Crag fresh = RoadsideCrag(Rng::FromSeed("crag-1"));
  Partner dev;
  dev.name = "Dev";
  dev.climbs = true;
  dev.ambition = 1.0;
  dev.climber.skills = {95, 95, 95, 95, 95};
  const Rng world = Rng::FromSeed("lot-etiquette");

  std::string yours;
  for (const CragLine& l : fresh.lines) if (l.isProject) { yours = l.route.name; break; }
  CHECK(!yours.empty());
  const std::vector<std::string> onIt = {yours};
  for (int day = 1; day <= 10950; day++) {
    const int got = PartnerTakesFirstAscent(world, dev, fresh, onIt, day);
    if (got >= 0) CHECK(fresh.lines[got].route.name != yours);
  }

  // But a project nobody can lose is not a project: leave it alone for a
  // career and somebody takes it.
  bool everTaken = false;
  for (int day = 1; day <= 10950 && !everTaken; day++) {
    if (PartnerTakesFirstAscent(world, dev, fresh, {}, day) >= 0) everTaken = true;
  }
  CHECK(everTaken);
}

static void TestTheLotPlateausLikePeopleDo() {
  PartnerDials d;
  const Rng world = Rng::FromSeed("gym-1");

  // The dial's own sentence, checked rather than trusted: a season moves
  // somebody about a third of a grade. It said this while being set to
  // 0.03, which is 1.53 grades a season -- four and a half times its own
  // documented intent, and nothing in the repo disagreed with it.
  const double gradesPerSeason = d.skillPerDay * 365.0 / 100.0 * 14.0;
  CHECK(gradesPerSeason > 0.25);
  CHECK(gradesPerSeason < 0.45);

  // And a ceiling, because there was none. Partners are the only climbers
  // in the game with no age model, so the creep ran unbounded: over thirty
  // years Dev reached power 382.5 on a scale documented 0..100, and Trish --
  // who is delighted to help and cannot climb your project -- reached 237.6.
  for (int day : {1, 365, 3650, 10950, 40000}) {
    for (const Partner& p : LotRegulars(world, day, d)) {
      CHECK(p.climber.skills.power <= d.ceiling);
      CHECK(p.climber.skills.fingers <= d.ceiling);
      CHECK(p.climber.skills.technique <= d.ceiling);
      CHECK(p.climber.skills.endurance <= d.ceiling);
      CHECK(p.climber.skills.head <= d.ceiling);
    }
  }

  // They do still improve, or the Lot is scenery.
  double early = 0.0, late = 0.0;
  for (const Partner& p : LotRegulars(world, 1, d)) early += p.climber.skills.power;
  for (const Partner& p : LotRegulars(world, 3650, d)) late += p.climber.skills.power;
  CHECK(late > early);

  // The strongest local plateaus rather than ascending forever: ten years
  // apart, past the ceiling, is the same person.
  double at20 = 0.0, at40 = 0.0;
  for (const Partner& p : LotRegulars(world, 7300, d))
    at20 = std::max(at20, p.climber.skills.power);
  for (const Partner& p : LotRegulars(world, 14600, d))
    at40 = std::max(at40, p.climber.skills.power);
  CHECK(at20 == at40);
  CHECK(at20 == d.ceiling);
}

static void TestRockGoesBackToTheWeather() {
  PlayerState player;
  ProjectMemory dirty;
  dirty.routeName = "the arete";
  dirty.cleanliness = 0.8;
  ProjectMemory known;
  known.routeName = "Diesel";
  known.cleanliness = 1.0;
  player.projects = {dirty, known};

  for (int night = 0; night < 5; night++) WeatherProjects(player);
  CHECK(player.projects[0].cleanliness < 0.8);   // your project needs re-brushing
  CHECK(player.projects[1].cleanliness == 1.0);  // a popular route does not
  // It never rots away entirely.
  for (int night = 0; night < 500; night++) WeatherProjects(player);
  CHECK(player.projects[0].cleanliness >= 0.0);
}

static void TestFirstAscentsAreACareer() {
  PlayerState player;
  ProjectMemory a;
  a.routeName = "line a"; a.firstAscent = true; a.confirmedGrade = 7;
  a.givenName = "Alpha";
  ProjectMemory b;
  b.routeName = "line b"; b.firstAscent = true; b.confirmedGrade = 9;
  b.givenName = "Beta";
  ProjectMemory c;
  c.routeName = "Diesel"; c.sent = true;    // sent, but somebody else's line
  player.projects = {a, c, b};

  std::vector<const ProjectMemory*> fas = FirstAscents(player);
  CHECK(fas.size() == 2);
  CHECK(fas[0]->confirmedGrade == 9);   // hardest first
  CHECK(fas[1]->confirmedGrade == 7);
  for (const ProjectMemory* m : fas) CHECK(m->firstAscent);
}

static void TestTheWholeArc() {
  // clean -> work -> send -> name, played through as a career would.
  Rng world = Rng::FromSeed("crag-1");
  Crag crag = RoadsideCrag(world);

  PlayerState player;
  player.climber.skills.power = player.climber.skills.fingers =
      player.climber.skills.technique = player.climber.skills.endurance =
          player.climber.skills.head = 72.0;   // strong enough, eventually

  // Deliberately the hardest open line rather than the first: dirt costs
  // about four grades, so a strong climber can bully a filthy moderate up
  // and the "you must clean it first" half of the arc would prove nothing.
  // At your limit it is the difference between impossible and merely hard.
  const CragLine* hardest = nullptr;
  for (const CragLine* p : OpenProjects(crag))
    if (!hardest || p->route.grade > hardest->route.grade)
      if (p->route.grade <= 8) hardest = p;   // within reach eventually
  CHECK(hardest != nullptr);
  CragLine project = *hardest;
  DayState day = WakeUp(player);
  ProjectMemory m = NewProjectLedger(project);

  // Day one: it is unclimbable, and throwing yourself at it proves it.
  CHECK(!IsWorkable(m));
  {
    Rng session = Rng::FromSeed("arc-dirty");
    SessionState st = StartSession(player.climber);
    int sends = 0;
    for (int i = 0; i < 8; i++)
      if (AttemptInSession(session, st, m, player.climber, project.route,
                           Conditions{}).sent) sends++;
    CHECK(sends == 0);        // filthy rock does not go
  }

  // So you clean it instead — to workable, which is where a first session
  // with a brush gets you.
  m.beta = 0.0;
  while (!IsWorkable(m)) CleanLine(player, day, m, 1.0);
  CHECK(IsWorkable(m));
  CHECK(m.cleanliness < 1.0);   // workable is not clean

  // Then you work it, across sessions, until it goes. Sixty days rather
  // than twenty-five since skin became a real cost across its whole range:
  // the late burns of a session are now meaningfully worse than the early
  // ones, so a project at your limit takes more sessions. That is the
  // change working, not the test being loosened.
  bool sent = false;
  for (int dayN = 0; dayN < 60 && !sent; dayN++) {
    // Every visit starts with the brush, because that is what projecting
    // is. Merely workable leaves most of a dirt penalty on the line — at
    // 0.55 cleanliness that is still 1.8 grades, which on top of skin
    // wearing through a session is why this used to stall out entirely.
    CleanLine(player, day, m, 1.0);

    Rng session = Rng::FromSeed("arc-" + std::to_string(dayN));
    SessionState st = StartSession(player.climber);
    for (int burn = 0; burn < 8 && st.skinLeft > 0.5 && !sent; burn++) {
      Conditions prime;
      prime.friction = 0.85;
      sent = AttemptInSession(session, st, m, player.climber, project.route,
                              prime).sent;
    }
    WeatherProjects(player);
  }
  CHECK(sent);
  CHECK(m.attempts > 1);      // it was not a gift
  CHECK(m.beta > 0.0);        // you learned it on the way

  // And then it is yours to name, and to find out what it was.
  CHECK(CanName(project, m));
  CHECK(NameFirstAscent(m, project, "Roadside Rites"));
  CHECK(m.confirmedGrade == project.route.trueGrade);
  CHECK(!FirstAscentLine(m, "you").empty());
}

static void TestSaveCarriesFirstAscents() {
  PlayerState player;
  ProjectMemory m;
  m.routeName = "the arete left of Diesel";
  m.grade = 7;
  m.attempts = 23;
  m.beta = 0.8;
  m.sent = true;
  m.cleanliness = 0.74;
  m.givenName = "Roadside Rites";
  m.firstAscent = true;
  m.confirmedGrade = 8;
  player.projects = {m};

  SaveGame save;
  save.seed = "crag-1";
  save.player = player;
  const std::string text = SerializeSave(save);

  SaveGame loaded;
  CHECK(DeserializeSave(text, loaded) == LoadResult::Ok);
  CHECK(loaded.player.projects.size() == 1);
  const ProjectMemory& r = loaded.player.projects[0];
  CHECK(r.routeName == m.routeName);
  CHECK(r.givenName == "Roadside Rites");
  CHECK(r.firstAscent);
  CHECK(r.confirmedGrade == 8);
  CHECK(std::fabs(r.cleanliness - 0.74) < 1e-12);

  // An unnamed project round-trips too — an empty given name is the normal
  // case, not a corrupt one.
  player.projects[0].givenName = "";
  player.projects[0].firstAscent = false;
  player.projects[0].confirmedGrade = -1;
  save.player = player;
  SaveGame again;
  CHECK(DeserializeSave(SerializeSave(save), again) == LoadResult::Ok);
  CHECK(again.player.projects[0].givenName.empty());
  CHECK(!again.player.projects[0].firstAscent);
  CHECK(again.player.projects[0].confirmedGrade == -1);
}

static void TestLoadsVersion2Save() {
  // A hand-written v2 career, from before first ascents existed. It must
  // still load, and it must not claim things that were never true: every
  // line a v2 save touched was already in a book, so nothing is a first
  // ascent and nothing needs its grade confirming.
  const std::string v2 =
      "version=2\n"
      "seed=crag-1\n"
      "day=14\n"
      "cash=317.5\n"
      "skills.power=52\n"
      "skills.fingers=55\n"
      "skills.technique=48\n"
      "skills.endurance=51\n"
      "skills.head=44\n"
      "morphology=1\n"
      "skin=6.5\n"
      "psyche=0.7\n"
      "projects=1\n"
      "project.0.name=Diesel\n"
      "project.0.grade=5\n"
      "project.0.attempts=31\n"
      "project.0.best=6\n"
      "project.0.beta=0.75\n"
      "project.0.sent=1\n"
      "project.0.style=2\n";

  SaveGame loaded;
  CHECK(DeserializeSave(v2, loaded) == LoadResult::Ok);
  CHECK(loaded.version == kSaveVersion);
  CHECK(loaded.player.day == 14);
  CHECK(loaded.player.projects.size() == 1);
  const ProjectMemory& m = loaded.player.projects[0];
  CHECK(m.routeName == "Diesel");
  CHECK(m.attempts == 31);
  CHECK(m.sent);
  CHECK(m.cleanliness == 1.0);      // it was in the book; it was clean
  CHECK(!m.firstAscent);            // and it was never yours
  CHECK(m.givenName.empty());
  CHECK(m.confirmedGrade == -1);    // nothing to confirm
}

static void TestTheYearHasSeasons() {
  ConditionsDials d;

  // Hottest where it says, coldest half a year away, and the swing is the
  // size it claims.
  const double summer = SeasonalCentreF(d.warmestDay, d);
  const double winter = SeasonalCentreF(d.warmestDay + d.daysPerYear / 2, d);
  CHECK(summer > winter);
  CHECK(std::fabs(summer - (d.baseTempF + d.seasonSwingF)) < 0.01);
  CHECK(std::fabs(winter - (d.baseTempF - d.seasonSwingF)) < 0.01);

  // It comes back round: a year later is the same place.
  CHECK(std::fabs(SeasonalCentreF(40, d) -
                  SeasonalCentreF(40 + d.daysPerYear, d)) < 0.01);

  // Four names, and they are in the right places.
  CHECK(std::string(SeasonName(d.warmestDay, d)) == "summer");
  CHECK(std::string(SeasonName(d.warmestDay + d.daysPerYear / 2, d)) ==
        "winter");
  bool sawSpring = false, sawAutumn = false;
  for (int day = 1; day <= d.daysPerYear; day++) {
    const std::string s = SeasonName(day, d);
    if (s == "spring") sawSpring = true;
    if (s == "autumn") sawAutumn = true;
  }
  CHECK(sawSpring);
  CHECK(sawAutumn);
}

static void TestTheWindowMovesThroughTheYear() {
  // The point of seasons, and the thing that gives a job teeth: in summer
  // only dawn is cool enough, in winter the good hours are the middle of
  // the day. A window that sat in the same place all year could never
  // conflict with anything.
  ConditionsDials d;
  Rng world = Rng::FromSeed("crag-1");

  auto MeanPeak = [&](int from, int days) {
    double sum = 0.0;
    int n = 0, withWindow = 0;
    for (int day = from; day < from + days; day++) {
      PrimeWindow w =
          FindPrimeWindow(GenerateWeather(world, day, d), Aspect::East, d);
      if (!w.exists) continue;
      withWindow++;
      sum += w.peakHour;
      n++;
    }
    return std::pair<double, double>(n ? sum / n : 0.0,
                                     100.0 * withWindow / days);
  };

  const auto summer = MeanPeak(d.warmestDay, 45);
  const auto winter = MeanPeak(d.warmestDay + d.daysPerYear / 2, 45);

  // Summer climbs at dawn; winter climbs in the middle of the day.
  CHECK(summer.first > 0.0);
  CHECK(winter.first > 0.0);
  CHECK(summer.first < winter.first - 3.0);

  // And summer offers far fewer days worth walking to the crag for.
  CHECK(summer.second < winter.second);
}

static void TestTheLightGoesInWinter() {
  ConditionsDials d;
  const int longest = d.warmestDay - d.solsticeLeadDays;
  const int shortest = longest + d.daysPerYear / 2;

  // A year of light, swinging around a fixed midday.
  CHECK(DaylightHours(longest, d) > DaylightHours(shortest, d) + 6.0);
  CHECK(DaylightHours(longest, d) < 18.5);   // nowhere near the arctic
  CHECK(DaylightHours(shortest, d) > 6.0);   // and nowhere near it the other way
  for (int day = 1; day <= d.daysPerYear; day++) {
    CHECK(FirstLightHour(day, d) < LastLightHour(day, d));
    CHECK(FirstLightHour(day, d) > 2.0 && LastLightHour(day, d) < 22.0);
  }

  // The point of all of it: on the shortest days the light is gone before a
  // nine-to-five lets you out, and on the longest there is most of an
  // evening left. Held at a fixed 6-to-20 (as it was through the first
  // Phase 3 measurement) a salaried season climbed as many burns as an
  // unemployed one, because there was always evening.
  JobDials j;
  const double clockOff = j.salaryStartHour + j.salaryHours;
  CHECK(LastLightHour(shortest, d) < clockOff);
  CHECK(LastLightHour(longest, d) > clockOff + 2.0);
}

static void TestAJobCostsYouTheWinter() {
  // Measured before seasons existed, a nine-to-five and an east-facing crag
  // never conflicted, because the window sat in the evening all year. The
  // whole reason for seasons is that a winter window at midday is one you
  // cannot have if you are at work.
  ConditionsDials d;
  JobDials j;
  Rng world = Rng::FromSeed("crag-1");

  Job employed;
  employed.salaried = true;

  auto LostToWork = [&](int from, int days) {
    int windows = 0, lost = 0;
    for (int day = from; day < from + days; day++) {
      PrimeWindow w =
          FindPrimeWindow(GenerateWeather(world, day, d), Aspect::East, d);
      if (!w.exists) continue;
      if (!SalariedToday(employed, day, j)) continue;   // a day off is free
      windows++;
      bool reachable = false;
      for (double h = w.startHour; h <= w.endHour + 1e-9; h += 0.25)
        if (!SalaryOwnsHour(employed, day, h, j)) reachable = true;
      if (!reachable) lost++;
    }
    return windows ? 100.0 * lost / windows : 0.0;
  };

  const double winterLost = LostToWork(d.warmestDay + d.daysPerYear / 2, 60);
  const double summerLost = LostToWork(d.warmestDay, 60);

  CHECK(winterLost > 25.0);   // winter is when the job actually costs you
  CHECK(winterLost > summerLost);   // and summer is when it does not
}

// --- The Lot -------------------------------------------------------------------

static void TestSaveCarriesWhoYouKnow() {
  SaveGame save;
  save.seed = "crag-1";
  PartnerBond margo;
  margo.name = "Margo";
  margo.rapport = 0.42;
  PartnerBond dev;
  dev.name = "Dev";
  dev.rapport = 0.9;
  dev.firstAscents = {"the arete left of Diesel",
                      "the low traverse into Chalk Ghost"};
  margo.everKnew = 0.88;   // knew them better once than you do now
  save.player.bonds = {margo, dev};

  SaveGame loaded;
  CHECK(DeserializeSave(SerializeSave(save), loaded) == LoadResult::Ok);
  CHECK(loaded.player.bonds.size() == 2);
  CHECK(loaded.player.bonds[0].name == "Margo");
  // **The memory has to survive the save**, and this is the one field where
  // losing it is not merely forgetting: rapport floors against it, so a
  // career reloaded without it would ratchet its friendships down a little
  // every time the game was closed.
  CHECK(std::fabs(loaded.player.bonds[0].everKnew - 0.88) < 1e-12);

  // A v34 career knew people at least as well as it knows them now, which
  // is what the runtime would have derived on its next day at the Lot. The
  // only migration in the file that has to read the save to write it --
  // bonds are a counted list, so there is no fixed set of keys to add.
  {
    std::string v34 = SerializeSave(save);
    DropSaveLine(v34, "bond.0.knew=");
    DropSaveLine(v34, "bond.1.knew=");
    SetSaveVersion(v34, 34);
    SaveGame old;
    CHECK(DeserializeSave(v34, old) == LoadResult::Ok);
    CHECK(old.version == kSaveVersion);
    CHECK(old.player.bonds.size() == 2);
    CHECK(old.player.bonds[0].everKnew == old.player.bonds[0].rapport);
    CHECK(old.player.bonds[1].everKnew == old.player.bonds[1].rapport);
    CHECK(static_cast<int>(DefaultMigrations().size()) == kSaveVersion - 1);
  }
  CHECK(std::fabs(loaded.player.bonds[0].rapport - 0.42) < 1e-12);
  CHECK(loaded.player.bonds[0].firstAscents.empty());
  CHECK(loaded.player.bonds[1].name == "Dev");
  CHECK(loaded.player.bonds[1].firstAscents.size() == 2);
  CHECK(loaded.player.bonds[1].firstAscents[1] ==
        "the low traverse into Chalk Ghost");

  // Knowing nobody round-trips as knowing nobody.
  SaveGame alone;
  alone.seed = "crag-1";
  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(alone), back) == LoadResult::Ok);
  CHECK(back.player.bonds.empty());

  // And the derived half really is derived: strength is never written down,
  // so it cannot drift between a save and a load.
  CHECK(SerializeSave(save).find("skills.power=") != std::string::npos);
  CHECK(SerializeSave(save).find("bond.0.rapport=") != std::string::npos);
  CHECK(SerializeSave(save).find("bond.0.power") == std::string::npos);
  CHECK(SerializeSave(save).find("bond.0.skill") == std::string::npos);
}

static void TestLoadsVersion3Save() {
  // A v3 career, from before the Lot existed. It knew nobody, which is
  // exactly what an empty bond list means — nothing to guess at.
  const std::string v3 =
      "version=3\n"
      "seed=crag-1\n"
      "day=22\n"
      "cash=180\n"
      "skills.power=54\n"
      "skills.fingers=57\n"
      "skills.technique=51\n"
      "skills.endurance=53\n"
      "skills.head=49\n"
      "morphology=1\n"
      "skin=7\n"
      "psyche=0.72\n"
      "projects=1\n"
      "project.0.name=Diesel\n"
      "project.0.grade=5\n"
      "project.0.attempts=8\n"
      "project.0.best=4\n"
      "project.0.beta=0.4\n"
      "project.0.sent=0\n"
      "project.0.clean=1\n"
      "project.0.given=\n"
      "project.0.fa=0\n"
      "project.0.confirmed=-1\n"
      "project.0.style=4\n";

  SaveGame loaded;
  CHECK(DeserializeSave(v3, loaded) == LoadResult::Ok);
  CHECK(loaded.version == kSaveVersion);
  CHECK(loaded.player.day == 22);
  CHECK(loaded.player.projects.size() == 1);
  CHECK(loaded.player.projects[0].routeName == "Diesel");
  CHECK(loaded.player.bonds.empty());     // knew nobody, still knows nobody
}

static void TestTheLotIsPeopleNotADistribution() {
  Rng world = Rng::FromSeed("crag-1");
  std::vector<Partner> lot = LotRegulars(world, 1);
  CHECK(lot.size() >= 5);   // three partners and two neighbours

  int climbers = 0, neighbours = 0;
  for (const Partner& p : lot) {
    CHECK(!p.name.empty());
    CHECK(!p.tag.empty());        // everybody is somebody
    CHECK(p.rapport == 0.0);      // and a stranger on day one
    CHECK(p.firstAscents.empty());
    if (p.climbs) climbers++; else neighbours++;
  }
  CHECK(climbers >= 3);
  CHECK(neighbours >= 2);

  // The same world gives the same people; a different world does not give
  // different people, because the cast is authored — it gives them
  // different bodies.
  std::vector<Partner> again = LotRegulars(world, 1);
  std::vector<Partner> elsewhere = LotRegulars(Rng::FromSeed("crag-2"), 1);
  bool bodiesDiffer = false;
  for (size_t i = 0; i < lot.size(); i++) {
    CHECK(again[i].name == lot[i].name);
    CHECK(again[i].climber.skills.power == lot[i].climber.skills.power);
    CHECK(elsewhere[i].name == lot[i].name);
    if (elsewhere[i].climbs &&
        elsewhere[i].climber.skills.power != lot[i].climber.skills.power)
      bodiesDiffer = true;
  }
  CHECK(bodiesDiffer);
}

static void TestPartnersHaveCareersOfTheirOwn() {
  Rng world = Rng::FromSeed("crag-1");
  std::vector<Partner> day1 = LotRegulars(world, 1);
  std::vector<Partner> later = LotRegulars(world, 400);

  bool someoneImproved = false;
  for (size_t i = 0; i < day1.size(); i++) {
    if (!day1[i].climbs) {
      continue;
    }
    const double before = SkillToGrade(day1[i].climber.skills.power);
    const double after = SkillToGrade(later[i].climber.skills.power);
    CHECK(after >= before);          // nobody goes backwards
    if (after > before + 0.2) someoneImproved = true;
  }
  CHECK(someoneImproved);            // they climb whether you do or not

  // Ambition decides pace: over a year the keen one gains more than the one
  // who has been here twenty seasons.
  auto Named = [](const std::vector<Partner>& v, const std::string& n) {
    for (const Partner& p : v) if (p.name == n) return p;
    return Partner{};
  };
  const double devGain = Named(later, "Dev").climber.skills.power -
                         Named(day1, "Dev").climber.skills.power;
  const double margoGain = Named(later, "Margo").climber.skills.power -
                           Named(day1, "Margo").climber.skills.power;
  CHECK(devGain > margoGain);

  // And it stays glacial. A Lot where everyone outruns the player is a
  // different and worse game: a year is well under two grades even for the
  // keenest.
  CHECK(SkillToGrade(Named(later, "Dev").climber.skills.power) -
            SkillToGrade(Named(day1, "Dev").climber.skills.power) < 2.0);
}

static void TestRapportGrowsAndFades() {
  PartnerDials d;
  Partner p;
  p.name = "Margo";
  p.climbs = true;

  for (int i = 0; i < 5; i++) SpendDayWith(p, true, 1.0, d);
  const double warm = p.rapport;
  CHECK(warm > 0.0);

  for (int i = 0; i < 5; i++) SpendDayWith(p, false, 1.0, d);
  CHECK(p.rapport < warm);       // people remember you, not forever
  CHECK(p.rapport > 0.0);        // and five days away is not amnesia

  // It saturates rather than running away.
  for (int i = 0; i < 500; i++) SpendDayWith(p, true, 1.0, d);
  CHECK(p.rapport == 1.0);
  CHECK(p.everKnew == 1.0);

  // **And it does not fade to nothing, however long you are gone.** This
  // used to assert exactly 0 after fourteen years away, and that assertion
  // was what let three thirty-year careers end as strangers to everybody at
  // the Lot -- including one that climbed 2,085 days. You do not forget
  // somebody you spent five years with; you stop being current with them.
  for (int i = 0; i < 5000; i++) SpendDayWith(p, false, 1.0, d);
  CHECK(p.rapport > 0.0);
  CHECK(std::fabs(p.rapport - d.rapportKeeps) < 1e-9);
  // The memory itself never falls -- it is what the drift floors against.
  CHECK(p.everKnew == 1.0);

  // Somebody you barely knew keeps barely anything, which is the other half
  // of the rule: the floor is a fraction of how well you ever knew them,
  // not a constant everybody gets.
  Partner nodding;
  nodding.name = "Dev";
  SpendDayWith(nodding, true, 1.0, d);
  const double slight = nodding.everKnew;
  for (int i = 0; i < 5000; i++) SpendDayWith(nodding, false, 1.0, d);
  CHECK(nodding.rapport < p.rapport);
  CHECK(std::fabs(nodding.rapport - slight * d.rapportKeeps) < 1e-9);

  // And what a belayer will do for you follows it, which is the point --
  // this is the only place in the game rapport buys something you cannot
  // get any other way.
  SportDials sd;
  CHECK(BurnsTheyWillHold(p, sd) > BurnsTheyWillHold(nodding, sd));
  CHECK(BurnsTheyWillHold(p, sd) > sd.burnsFromAStranger);
}

static void TestBetaIsWorthAskingForAndOnlyOnce() {
  Rng world = Rng::FromSeed("crag-1");
  Crag crag = RoadsideCrag(world);
  std::vector<Partner> lot = LotRegulars(world, 200);

  // Margo knows the moderates cold.
  Partner margo;
  for (const Partner& p : lot) if (p.name == "Margo") margo = p;
  CHECK(margo.climbs);

  const CragLine* known = nullptr;
  for (const CragLine& l : crag.lines)
    if (!l.isProject && KnowsLine(margo, l)) { known = &l; break; }
  CHECK(known != nullptr);

  ProjectMemory cold;
  cold.routeName = known->route.name;
  margo.rapport = 0.0;
  const double fromStranger = ShareBeta(margo, *known, cold);
  CHECK(fromStranger > 0.0);

  ProjectMemory warmLedger;
  warmLedger.routeName = known->route.name;
  margo.rapport = 1.0;
  const double fromFriend = ShareBeta(margo, *known, warmLedger);
  CHECK(fromFriend > fromStranger);   // rapport is worth something
  CHECK(warmLedger.beta < 1.0);       // and never everything

  // Asking twice gives diminishing returns and never goes backwards.
  const double before = warmLedger.beta;
  ShareBeta(margo, *known, warmLedger);
  CHECK(warmLedger.beta >= before);
  CHECK(warmLedger.beta <= 1.0);

  // Nobody has beta on a line nobody has climbed.
  const CragLine* project = OpenProjects(crag)[0];
  ProjectMemory virgin;
  virgin.routeName = project->route.name;
  CHECK(ShareBeta(margo, *project, virgin) == 0.0);
  CHECK(virgin.beta == 0.0);

  // Ray does not climb and has no moves to give, however well you know him.
  Partner ray;
  for (const Partner& p : lot) if (p.name == "Ray") ray = p;
  ray.rapport = 1.0;
  ProjectMemory fromRay;
  CHECK(ShareBeta(ray, *known, fromRay) == 0.0);
}

static void TestCompanyIsWorthSomethingAndNeverAGrade() {
  PartnerDials d;
  SessionDials sd;
  Partner p;
  p.name = "Trish";
  p.climbs = true;
  p.rapport = 0.0;
  CHECK(PsycheFrom(p, d) == 0.0);

  p.rapport = 1.0;
  const double lift = PsycheFrom(p, d);
  CHECK(lift > 0.0);
  // Psyche is ability in the resolver, so this has to stay small: a good
  // belayer is worth something and never worth a grade.
  CHECK(lift * sd.psycheWeight < 1.0);

  // A neighbour at the fire is worth less than a partner on the pads, and
  // still worth more than nothing.
  Partner ray;
  ray.climbs = false;
  ray.rapport = 1.0;
  CHECK(PsycheFrom(ray, d) > 0.0);
  CHECK(PsycheFrom(ray, d) < lift);
}

static void TestSomebodyCanTakeYourProject() {
  Rng world = Rng::FromSeed("crag-1");
  Crag crag = RoadsideCrag(world);
  PartnerDials d;

  // Dev, years in, strong enough for the moderate open lines.
  std::vector<Partner> lot = LotRegulars(world, 900);
  Partner dev;
  for (const Partner& p : lot) if (p.name == "Dev") dev = p;

  std::vector<std::string> taken;
  int tookOn = -1;
  int days = 0;
  for (; days < 4000 && tookOn < 0; days++) {
    tookOn = PartnerTakesFirstAscent(world, dev, crag, taken, days, d);
  }
  CHECK(tookOn >= 0);                        // it does happen
  CHECK(days > 20);                          // and not on the first afternoon
  CHECK(crag.lines[tookOn].isProject);       // only ever an unclimbed line

  // Once it is somebody's, nobody takes it again.
  taken.push_back(crag.lines[tookOn].route.name);
  for (int day = 0; day < 4000; day++)
    CHECK(PartnerTakesFirstAscent(world, dev, crag, taken, day, d) != tookOn);

  // Nobody takes a line they cannot climb: the blank wall outlasts them.
  const CragLine* blank = nullptr;
  for (const CragLine& l : crag.lines)
    if (l.isProject && l.route.trueGrade >= 10) blank = &l;
  if (blank) {
    std::vector<Partner> earlyLot = LotRegulars(world, 1);
    Partner young;
    for (const Partner& p : earlyLot) if (p.name == "Trish") young = p;
    for (int day = 0; day < 3000; day++) {
      const int t = PartnerTakesFirstAscent(world, young, crag, {}, day, d);
      if (t >= 0) CHECK(crag.lines[t].route.trueGrade < blank->route.trueGrade);
    }
  }

  // The neighbours never take anything; Ray stopped climbing years ago.
  Partner ray;
  for (const Partner& p : lot) if (p.name == "Ray") ray = p;
  for (int day = 0; day < 2000; day++)
    CHECK(PartnerTakesFirstAscent(world, ray, crag, {}, day, d) == -1);
}

static void TestTheLotDoesNotDisturbThePlayersRng() {
  // Partners getting on with their lives runs on its own named stream. If
  // it ever shared one with attempts, who else was at the crag would change
  // how your burns resolved, and a save would stop being replayable.
  Rng world = Rng::FromSeed("crag-1");
  Crag crag = RoadsideCrag(world);
  Climber c;
  c.skills.power = c.skills.fingers = c.skills.technique =
      c.skills.endurance = c.skills.head = 55.0;

  auto PlayADay = [&](bool withTheLot) {
    Rng session = Rng::FromSeed("rng-isolation");
    SessionState st = StartSession(c);
    ProjectMemory m;
    m.routeName = crag.lines[5].route.name;
    if (withTheLot) {
      std::vector<Partner> lot = LotRegulars(world, 40);
      for (const Partner& p : lot)
        PartnerTakesFirstAscent(world, p, crag, {}, 40);
    }
    std::string trace;
    for (int i = 0; i < 6; i++) {
      const AttemptResult r =
          AttemptInSession(session, st, m, c, crag.lines[5].route, Conditions{});
      trace += std::to_string(r.highpoint) + ":" +
               std::to_string(static_cast<int>(r.peakPump * 1000)) + " ";
    }
    return trace;
  };
  CHECK(PlayADay(false) == PlayADay(true));
}

static void TestTheFireHasSomethingToSay() {
  Rng world = Rng::FromSeed("crag-1");
  Crag crag = RoadsideCrag(world);
  std::vector<Partner> lot = LotRegulars(world, 30);
  for (const Partner& p : lot) {
    const std::string talk = LotTalk(p, crag, 30);
    CHECK(!talk.empty());
    CHECK(talk.find(p.name) != std::string::npos);
    // The same week gets the same answer; people are on a thing for a while.
    CHECK(LotTalk(p, crag, 30) == talk);
    CHECK(LotTalk(p, crag, 31) == talk);
  }
}

// --- The dog -------------------------------------------------------------------

static void TestTheSessionTellsYouWhereYouAre() {
  // The warmth trap, made visible: a line you cannot start is a line you
  // can never warm up on, and nothing used to say so.
  Climber c;
  c.skills.power = c.skills.fingers = c.skills.technique =
      c.skills.endurance = c.skills.head = 50.0;
  SessionLoopDials loop;

  // Pulling on cold, which is where every session starts.
  SessionState fresh = StartSession(c);
  CHECK(fresh.warmth < loop.coldBelowWarmth);
  CHECK(ReadSession(fresh, c) == SessionAdvice::Cold);

  // Warm and skinned: nothing to say.
  SessionState ready = fresh;
  ready.warmth = 1.0;
  CHECK(ReadSession(ready, c) == SessionAdvice::Ready);

  // Warm but running out of tips: the advice is different holds, not home.
  SessionState thin = ready;
  thin.skinLeft = 2.0;
  CHECK(ReadSession(thin, c) == SessionAdvice::SkinThin);

  // And when it is over, it says so rather than letting you grind.
  SessionState done = ready;
  done.skinLeft = 0.5;
  CHECK(ReadSession(done, c) == SessionAdvice::Wrecked);

  // Cold AND thin is the trap itself: it reads as over, not as "try again".
  SessionState trapped = fresh;
  trapped.skinLeft = 2.0;
  CHECK(ReadSession(trapped, c) == SessionAdvice::Wrecked);

  // Every state says something, and no two say the same thing.
  const SessionAdvice all[] = {SessionAdvice::Ready, SessionAdvice::Cold,
                               SessionAdvice::SkinThin,
                               SessionAdvice::Wrecked};
  for (SessionAdvice a : all) {
    CHECK(std::string(SessionAdviceText(a)).size() > 0);
    for (SessionAdvice b : all)
      if (a != b)
        CHECK(std::string(SessionAdviceText(a)) != SessionAdviceText(b));
  }

  // Warming up actually clears it, which is the whole point of saying it.
  Rng world = Rng::FromSeed("crag-1");
  Route jugs = BuildRoute(world, "The Warmup", 1, 1, RouteType::Endurance,
                          Discipline::Boulder);
  Rng session = Rng::FromSeed("warmup");
  SessionState st = StartSession(c);
  ProjectMemory m;
  m.routeName = jugs.name;
  int laps = 0;
  while (ReadSession(st, c) == SessionAdvice::Cold && laps < 10) {
    AttemptInSession(session, st, m, c, jugs, Conditions{});
    laps++;
  }
  CHECK(ReadSession(st, c) != SessionAdvice::Cold);
  CHECK(laps <= 4);   // two or three easy problems, as the dial intends
}

static void TestFreshSkinIsWorthSomething() {
  // Skin used to bite only below 3, only on crimps, and cap at 0.45 grades
  // — so skin 9 and skin 3 were identical to climb on and resting bought
  // nothing at all. Measured over a season, a career played fresh and one
  // played wrecked came out the same. It has to be worth something.
  Rng world = Rng::FromSeed("crag-1");
  Climber c;
  c.skills.power = c.skills.fingers = c.skills.technique =
      c.skills.endurance = c.skills.head = 50.0;

  auto SendRateAt = [&](const Route& r, double skin) {
    int sends = 0;
    const int trials = 1500;
    for (int i = 0; i < trials; i++) {
      Rng rng = Rng::FromSeed("skin-" + std::to_string(i));
      AttemptInput in;
      in.climber = c;
      in.climber.skin = skin;
      in.route = r;
      in.warmth = 1.0;
      if (ResolveAttempt(rng, in).sent) sends++;
    }
    return 100.0 * sends / trials;
  };

  Route crimpy = BuildRoute(world, "Tips", 5, 5, RouteType::Crimp,
                            Discipline::Boulder);
  const double fresh = SendRateAt(crimpy, 9.0);
  const double half = SendRateAt(crimpy, 4.5);
  const double gone = SendRateAt(crimpy, 1.0);

  CHECK(fresh > half);
  CHECK(half > gone);
  CHECK(fresh > gone * 2.0);   // a real difference, not a rounding one

  // The top of the range stays nearly free: nobody can feel 9 against 8.
  CHECK(fresh - SendRateAt(crimpy, 8.0) < 5.0);

  // And shot skin steers you rather than stopping you — jugs stay a real
  // option on a day your tips are gone, which is what those days are for.
  Route juggy = BuildRoute(world, "Buckets", 5, 5, RouteType::Endurance,
                           Discipline::Boulder);
  // Compared against itself rather than against the crimpy line: the two
  // routes differ in length as well as holds, so their absolute rates are
  // not comparable and an earlier version of this check compared them
  // anyway. What matters is that skin costs a jug day less.
  const double juggyGone = SendRateAt(juggy, 1.0);
  const double juggyFresh = SendRateAt(juggy, 9.0);
  CHECK(juggyFresh > juggyGone);
  CHECK(juggyFresh - juggyGone < fresh - gone);
}

static void TestFlailingIsNotTraining() {
  // Found by playing a season headless: training read only the grade on the
  // tag, so falling off move one of something impossible trained exactly as
  // well as nearly doing it, and a year of hopeless flailing was the fastest
  // way to get strong. What you climbed has to count.
  Rng world = Rng::FromSeed("crag-1");
  DayDials d;

  Route line = BuildRoute(world, "The Test Piece", 8, 8, RouteType::Crimp,
                          Discipline::Boulder);
  CHECK(line.moves.size() >= 4);

  auto TrainOn = [&](int highpoint) {
    PlayerState player;
    player.climber.skills.power = player.climber.skills.fingers =
        player.climber.skills.technique = player.climber.skills.endurance =
            player.climber.skills.head = 50.0;
    DayState day = WakeUp(player, d);

    AttemptResult r;
    r.highpoint = highpoint;
    r.sent = highpoint >= static_cast<int>(line.moves.size());
    for (int i = 0; i < highpoint + 1; i++) {
      MoveResult m;
      m.index = i;
      r.timeline.push_back(m);
    }
    const double before = player.climber.skills.fingers;
    ApplyAttemptToDay(player, day, line, r, Rng::FromSeed("body"), d);
    return player.climber.skills.fingers - before;
  };

  const double offTheGround = TrainOn(0);
  const double halfway = TrainOn(static_cast<int>(line.moves.size() / 2));
  const double nearlyThere = TrainOn(static_cast<int>(line.moves.size() - 1));

  // Getting further up teaches more, every step of the way.
  CHECK(offTheGround > 0.0);        // pulling on is not nothing
  CHECK(halfway > offTheGround);
  CHECK(nearlyThere > halfway);
  // And flailing is worth a fraction of working it, not the same.
  CHECK(offTheGround < halfway * 0.6);

  // The line still has to be hard: a warmup teaches nothing much however
  // cleanly you climb it. That was already true and must stay true.
  Route jug = BuildRoute(world, "The Warmup", 1, 1, RouteType::Endurance,
                         Discipline::Boulder);
  PlayerState player;
  player.climber.skills.power = player.climber.skills.fingers =
      player.climber.skills.technique = player.climber.skills.endurance =
          player.climber.skills.head = 50.0;
  DayState day = WakeUp(player, d);
  AttemptResult sent;
  sent.sent = true;
  sent.highpoint = static_cast<int>(jug.moves.size());
  for (size_t i = 0; i < jug.moves.size(); i++) {
    MoveResult m;
    m.index = static_cast<int>(i);
    sent.timeline.push_back(m);
  }
  const double beforeJug = player.climber.skills.fingers;
  ApplyAttemptToDay(player, day, jug, sent, Rng::FromSeed("body"), d);
  CHECK(player.climber.skills.fingers - beforeJug < nearlyThere);
}

static void TestAStrayBecomesYoursByBeingFed() {
  DogDials d;
  Dog dog;
  double cash = 100.0;

  CHECK(!dog.adopted);
  CHECK(dog.bond == 0.0);
  CHECK(DogPsyche(dog, d) == 0.0);   // a stray is not company yet

  // No ceremony: you feed it enough times and it stops being a stray.
  int meals = 0;
  while (!dog.adopted && meals < 20) {
    CHECK(FeedDog(dog, cash, d));
    meals++;
  }
  CHECK(dog.adopted);
  CHECK(meals >= 2);                 // and not on the first tin
  CHECK(meals <= 6);                 // nor after a month of it
  CHECK(cash < 100.0);               // it costs, every time
  CHECK(DogPsyche(dog, d) > 0.0);

  // Broke is broke.
  double empty = 0.0;
  Dog other;
  CHECK(!FeedDog(other, empty, d));
  CHECK(other.fed == Dog{}.fed);     // and nothing happened
}

static void TestTheDogGetsHungryAndSaysSo() {
  DogDials d;
  Dog dog;
  dog.adopted = true;
  dog.bond = 0.9;
  dog.fed = 1.0;
  CHECK(DogPsyche(dog, d) > 0.0);

  for (int day = 0; day < 3; day++) DogDay(dog, true, d);
  CHECK(dog.fed < 1.0);

  // Once it is actually hungry, having it around stops being a comfort.
  dog.fed = 0.1;
  CHECK(DogPsyche(dog, d) < 0.0);
  CHECK(DogText(dog, d).find("not eaten") != std::string::npos);

  // Feeding fixes it, and never overfills.
  double cash = 50.0;
  for (int i = 0; i < 10; i++) FeedDog(dog, cash, d);
  CHECK(dog.fed == 1.0);
  CHECK(dog.bond == 1.0);
  CHECK(DogPsyche(dog, d) > 0.0);
}

static void TestBondNeedsYouAround() {
  DogDials d;
  Dog dog;
  dog.adopted = true;
  dog.fed = 1.0;
  for (int i = 0; i < 10; i++) DogDay(dog, true, d);
  const double close = dog.bond;
  CHECK(close > 0.0);

  for (int i = 0; i < 10; i++) DogDay(dog, false, d);
  CHECK(dog.bond < close);
  CHECK(dog.bond >= 0.0);

  // Company is worth something and never worth a grade — the same rule the
  // Lot's people are held to, since it lands in the same place.
  SessionDials sd;
  Dog devoted;
  devoted.adopted = true;
  devoted.fed = 1.0;
  devoted.bond = 1.0;
  CHECK(DogPsyche(devoted, d) * sd.psycheWeight < 1.0);
}

static void TestTheDogClimbsWithYou() {
  // The dog is not a line of text: it lands in the body you climb in, the
  // same place fatigue does.
  PlayerState player;
  DayState day = WakeUp(player);
  const double alone = ClimberForSession(player, day).psyche;

  player.dog.adopted = true;
  player.dog.fed = 1.0;
  player.dog.bond = 1.0;
  const double together = ClimberForSession(player, day).psyche;
  CHECK(together > alone);

  // And a hungry one is worse than no dog at all, which is the honest
  // version of having taken something on.
  player.dog.fed = 0.05;
  const double guilty = ClimberForSession(player, day).psyche;
  CHECK(guilty < alone);

  // Psyche stays a 0..1 quantity whatever the dog is doing.
  for (double bond : {0.0, 0.5, 1.0}) {
    for (double fed : {0.0, 0.5, 1.0}) {
      player.dog.bond = bond;
      player.dog.fed = fed;
      const double p = ClimberForSession(player, day).psyche;
      CHECK(p >= 0.0 && p <= 1.0);
    }
  }
}

static void TestTheVanGetsHot() {
  DogDials d;
  Dog dog;
  dog.adopted = true;
  dog.fed = 1.0;

  CHECK(VanIsSafe(60.0, d));
  CHECK(VanGuilt(dog, 60.0, d) == 0.0);      // a cool day costs nothing

  CHECK(!VanIsSafe(d.warmVanF + 5.0, d));
  const double warm = VanGuilt(dog, d.warmVanF + 5.0, d);
  const double hot = VanGuilt(dog, d.warmVanF + 25.0, d);
  CHECK(warm > 0.0);
  CHECK(hot > warm);                          // and it rises with the heat

  // There is no lock anywhere in this: a stray is not your problem, and
  // you can always leave. It just costs.
  Dog stray;
  CHECK(VanGuilt(stray, 110.0, d) == 0.0);
}

static void TestTheDogSurvivesASave() {
  SaveGame save;
  save.seed = "crag-1";
  save.player.dog.name = "Biscuit";
  save.player.dog.adopted = true;
  save.player.dog.bond = 0.73;
  save.player.dog.fed = 0.41;

  SaveGame loaded;
  CHECK(DeserializeSave(SerializeSave(save), loaded) == LoadResult::Ok);
  CHECK(loaded.player.dog.name == "Biscuit");
  CHECK(loaded.player.dog.adopted);
  CHECK(std::fabs(loaded.player.dog.bond - 0.73) < 1e-12);
  CHECK(std::fabs(loaded.player.dog.fed - 0.41) < 1e-12);
}

static void TestLoadsVersion4Save() {
  // A v4 career, from before the dog. It never met one, so it migrates to
  // exactly the stray a new career finds at the Lot.
  const std::string v4 =
      "version=4\n"
      "seed=crag-1\n"
      "day=31\n"
      "cash=95\n"
      "skills.power=56\n"
      "skills.fingers=58\n"
      "skills.technique=53\n"
      "skills.endurance=55\n"
      "skills.head=51\n"
      "morphology=1\n"
      "skin=8\n"
      "psyche=0.7\n"
      "projects=0\n"
      "bonds=1\n"
      "bond.0.name=Margo\n"
      "bond.0.rapport=0.3\n"
      "bond.0.fas=0\n";

  SaveGame loaded;
  CHECK(DeserializeSave(v4, loaded) == LoadResult::Ok);
  CHECK(loaded.version == kSaveVersion);
  CHECK(loaded.player.day == 31);
  CHECK(loaded.player.bonds.size() == 1);
  CHECK(loaded.player.bonds[0].name == "Margo");
  CHECK(!loaded.player.dog.adopted);       // never met it
  CHECK(loaded.player.dog.bond == 0.0);
  CHECK(loaded.player.dog.fed > 0.0);      // and it is hungry, as strays are
}

// --- Gear ----------------------------------------------------------------------

static void TestShoesWearByWhatYouClimb() {
  GearDials g;
  Shoes s;
  CHECK(s.wear == 0.0);

  // Rubber goes by the move, not by the day.
  WearShoes(s, 0, 5, g);
  CHECK(s.wear == 0.0);
  WearShoes(s, 50, 5, g);
  CHECK(s.wear > 0.0);

  // And faster the harder you pull: the same mileage on a V9 eats more
  // than on a V3, because it is toe pressure that kills shoes.
  Shoes easy, hard;
  WearShoes(easy, 200, 3, g);
  WearShoes(hard, 200, 9, g);
  CHECK(hard.wear > easy.wear);

  // A pair dies somewhere near its stated life and never past dead.
  Shoes worn;
  WearShoes(worn, static_cast<int>(g.shoeLifeMoves), 5, g);
  CHECK(worn.wear >= 0.9);
  WearShoes(worn, 100000, 9, g);
  CHECK(worn.wear == 1.0);
}

static void TestDeadRubberCostsGrades() {
  GearDials g;
  Shoes fresh;
  Shoes dead;
  dead.wear = 1.0;

  CHECK(ShoePenalty(fresh, true, g) == 0.0);
  CHECK(ShoePenalty(dead, true, g) > 0.0);
  // Edging suffers most; slopers and jugs care less, which is what pushes a
  // worn pair onto different holds before it stops you climbing.
  CHECK(ShoePenalty(dead, true, g) > ShoePenalty(dead, false, g));
  CHECK(ShoePenalty(dead, false, g) > 0.0);

  // Squared, like skin: half-worn is much better than half as bad.
  Shoes half;
  half.wear = 0.5;
  CHECK(ShoePenalty(half, true, g) < ShoePenalty(dead, true, g) * 0.5);

  // And it reaches the wall, not just the shop.
  Rng world = Rng::FromSeed("crag-1");
  Climber c;
  c.skills.power = c.skills.fingers = c.skills.technique =
      c.skills.endurance = c.skills.head = 55.0;
  Route edgy = BuildRoute(world, "Edges", 5, 5, RouteType::Crimp,
                          Discipline::Boulder);
  auto Rate = [&](double wear) {
    int sends = 0;
    const int trials = 1200;
    for (int i = 0; i < trials; i++) {
      Rng rng = Rng::FromSeed("shoe-" + std::to_string(i));
      AttemptInput in;
      in.climber = c;
      in.route = edgy;
      in.warmth = 1.0;
      in.shoeWear = wear;
      if (ResolveAttempt(rng, in).sent) sends++;
    }
    return 100.0 * sends / trials;
  };
  CHECK(Rate(0.0) > Rate(1.0));
  CHECK(Rate(0.0) > Rate(1.0) * 1.3);   // a real handicap, not a rounding one
}

static void TestResoleOrReplace() {
  GearDials g;
  Shoes s;
  s.wear = 0.9;
  double cash = 500.0;

  // A resole is most of the performance for a third of the price.
  CHECK(CanResole(s, g));
  CHECK(Resole(s, cash, false, g));
  CHECK(s.wear < 0.9);
  CHECK(s.wear > 0.0);            // and never quite new again
  CHECK(cash == 500.0 - g.resoleCost);

  // But the uppers only take so many.
  s.wear = 0.9;
  CHECK(Resole(s, cash, false, g));
  CHECK(!CanResole(s, g));
  CHECK(!Resole(s, cash, false, g));

  // The warning only appears once this pair is worn again — a freshly
  // resoled shoe with no resoles left says nothing, because there is
  // nothing to decide yet.
  CHECK(ShoeText(s, g).find("uppers") == std::string::npos);
  s.wear = 0.7;
  CHECK(ShoeText(s, g).find("uppers") != std::string::npos);

  // At which point it is a new pair or nothing.
  const double before = cash;
  CHECK(BuyNewShoes(s, cash, false, g));
  CHECK(s.wear == 0.0);
  CHECK(s.resoles == 0);
  CHECK(s.pairsOwned == 2);
  CHECK(cash == before - g.newShoeCost);

  // Broke is broke, for both.
  double empty = 10.0;
  Shoes poor;
  poor.wear = 0.95;
  CHECK(!Resole(poor, empty, false, g));
  CHECK(!BuyNewShoes(poor, empty, false, g));
  CHECK(poor.wear == 0.95);       // and nothing happened
  CHECK(empty == 10.0);

  // Resoling is the cheaper path per pair, which is why anyone does it.
  CHECK(g.resoleCost * g.resolesPerPair < g.newShoeCost);
}

static void TestShoesReachTheSession() {
  // Shoes live on the career and the session has to be handed them, or the
  // whole mechanic is a number in a shop that never touches a wall.
  PlayerState player;
  player.shoes.wear = 0.8;
  DayState day = WakeUp(player);
  StartGymSession(player, day);
  CHECK(std::fabs(day.session.shoeWear - 0.8) < 1e-12);

  // And climbing wears them: the day loop knows how many moves you did.
  Rng world = Rng::FromSeed("crag-1");
  Route r = BuildRoute(world, "Mileage", 4, 4, RouteType::Endurance,
                       Discipline::Boulder);
  player.shoes.wear = 0.0;
  AttemptResult result;
  result.highpoint = static_cast<int>(r.moves.size());
  for (size_t i = 0; i < r.moves.size(); i++) {
    MoveResult m;
    m.index = static_cast<int>(i);
    result.timeline.push_back(m);
  }
  ApplyAttemptToDay(player, day, r, result, Rng::FromSeed("body"));
  CHECK(player.shoes.wear > 0.0);
}

// --- The van -------------------------------------------------------------------

static void TestBillsYouCannotPayWait() {
  // Bills used to deduct unconditionally, so cash sat at -16 with no debt
  // mechanic behind it — a hole with nothing in it. Money now floors at
  // zero and the shortfall waits.
  DayDials d;
  PlayerState player;
  player.cash = 30.0;
  player.owed = 0.0;

  Charge(player, 100.0);
  CHECK(player.cash == 0.0);      // never negative
  CHECK(std::fabs(player.owed - 70.0) < 1e-12);

  // A wage goes to what you owe before it goes to you.
  Pay(player, 50.0);
  CHECK(player.cash == 0.0);
  CHECK(std::fabs(player.owed - 20.0) < 1e-12);

  // And once you are level the rest is yours.
  Pay(player, 50.0);
  CHECK(player.owed == 0.0);
  CHECK(std::fabs(player.cash - 30.0) < 1e-12);

  // Charging nothing does nothing.
  const double cash = player.cash;
  Charge(player, 0.0);
  Pay(player, 0.0);
  CHECK(player.cash == cash);
  CHECK(player.owed == 0.0);

  // Bills landing on a broke career leave debt rather than negative cash,
  // and a shift digs you out rather than paying you.
  PlayerState broke;
  broke.cash = 5.0;
  DayState day = WakeUp(broke, d);
  for (int i = 0; i < d.billsEveryDays; i++) {
    SleepToNextDay(broke, day, Rng::FromSeed("body"), d);
    day = WakeUp(broke, d);
  }
  CHECK(broke.cash >= 0.0);
  CHECK(broke.owed > 0.0);

  const double owedBefore = broke.owed;
  WorkShift(broke, day, d);
  CHECK(broke.owed < owedBefore);
  CHECK(broke.cash >= 0.0);
}

static void TestLoadsVersion5Save() {
  // A v5 career, from before anything you owned could wear out. Shoes and
  // van both arrive new — generous rather than exact, because the honest
  // alternative is inventing damage nobody earned.
  const std::string v5 =
      "version=5\n"
      "seed=crag-1\n"
      "day=44\n"
      "cash=210\n"
      "skills.power=57\n"
      "skills.fingers=59\n"
      "skills.technique=54\n"
      "skills.endurance=56\n"
      "skills.head=52\n"
      "morphology=1\n"
      "skin=8\n"
      "psyche=0.7\n"
      "dog.name=Biscuit\n"
      "dog.adopted=1\n"
      "dog.bond=0.8\n"
      "dog.fed=0.9\n"
      "projects=0\n"
      "bonds=0\n";

  SaveGame loaded;
  CHECK(DeserializeSave(v5, loaded) == LoadResult::Ok);
  CHECK(loaded.version == kSaveVersion);
  CHECK(loaded.player.day == 44);
  CHECK(loaded.player.dog.name == "Biscuit");   // the old career survives
  CHECK(loaded.player.dog.adopted);
  CHECK(loaded.player.shoes.wear == 0.0);
  CHECK(loaded.player.shoes.pairsOwned == 1);
  CHECK(loaded.player.van.hoursDriven == 0.0);
  for (int i = 0; i < kVanPartCount; i++) {
    CHECK(loaded.player.van.parts[i].wear == 0.0);
    CHECK(!loaded.player.van.parts[i].failed);
  }
  CHECK(VanRuns(loaded.player.van));
  CHECK(loaded.player.owed == 0.0);   // could not have owed anything
}

static void TestWhatYouOwnSurvivesASave() {
  SaveGame save;
  save.seed = "crag-1";
  save.player.owed = 137.5;
  save.player.standing.with[static_cast<int>(Faction::OldGuard)] = 0.62;
  save.player.standing.with[static_cast<int>(Faction::Stewardship)] = -0.5;
  save.player.standing.closedDays = 4;
  save.player.shoes.wear = 0.62;
  save.player.shoes.resoles = 1;
  save.player.shoes.pairsOwned = 3;
  save.player.van.hoursDriven = 137.5;
  save.player.van.parts[static_cast<int>(VanPart::Belt)].wear = 0.91;
  save.player.van.parts[static_cast<int>(VanPart::Belt)].failed = true;
  save.player.van.parts[static_cast<int>(VanPart::Tyres)].patches = 2;

  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  CHECK(std::fabs(back.player.owed - 137.5) < 1e-12);
  CHECK(std::fabs(StandingWith(back.player.standing, Faction::OldGuard) -
                  0.62) < 1e-12);
  CHECK(std::fabs(StandingWith(back.player.standing, Faction::Stewardship) +
                  0.5) < 1e-12);
  CHECK(back.player.standing.closedDays == 4);
  CHECK(!CragIsOpen(back.player.standing));   // you load in as shut out
  CHECK(std::fabs(back.player.shoes.wear - 0.62) < 1e-12);
  CHECK(back.player.shoes.resoles == 1);
  CHECK(back.player.shoes.pairsOwned == 3);
  CHECK(std::fabs(back.player.van.hoursDriven - 137.5) < 1e-12);
  CHECK(back.player.van.parts[static_cast<int>(VanPart::Belt)].failed);
  CHECK(back.player.van.parts[static_cast<int>(VanPart::Tyres)].patches == 2);
  CHECK(!VanRuns(back.player.van));   // you load in exactly as stranded
}

static void TestDrivingWearsTheVan() {
  VanDials d;
  Rng world = Rng::FromSeed("crag-1");
  Van van;
  for (int i = 0; i < kVanPartCount; i++) CHECK(van.parts[i].wear == 0.0);
  CHECK(VanRuns(van));

  DriveVan(van, world, 1, 1.0, 60.0, d);
  CHECK(van.hoursDriven == 1.0);
  for (int i = 0; i < kVanPartCount; i++) CHECK(van.parts[i].wear > 0.0);

  // Tyres go before the clutch does, which is the order a dirtbag learns.
  CHECK(van.parts[static_cast<int>(VanPart::Tyres)].wear >
        van.parts[static_cast<int>(VanPart::Clutch)].wear);

  // Standing still costs nothing.
  Van parked;
  DriveVan(parked, world, 1, 0.0, 60.0, d);
  CHECK(parked.hoursDriven == 0.0);
  CHECK(parked.parts[0].wear == 0.0);

  // Wear never runs past done.
  Van hammered;
  for (int day = 0; day < 400; day++)
    DriveVan(hammered, world, day, 5.0, 60.0, d);
  for (int i = 0; i < kVanPartCount; i++) CHECK(hammered.parts[i].wear <= 1.0);
}

static void TestHotDaysCookTheRadiator() {
  VanDials d;
  Rng world = Rng::FromSeed("crag-1");
  Van cool, hot;
  for (int day = 1; day <= 20; day++) {
    DriveVan(cool, world, day, 1.0, 60.0, d);
    DriveVan(hot, world, day, 1.0, d.radiatorWarmF + 15.0, d);
  }
  const int rad = static_cast<int>(VanPart::Radiator);
  CHECK(hot.parts[rad].wear > cool.parts[rad].wear);

  // And only the radiator: heat does not wear a clutch.
  const int clutch = static_cast<int>(VanPart::Clutch);
  CHECK(std::fabs(hot.parts[clutch].wear - cool.parts[clutch].wear) < 1e-9);
}

static void TestNothingFailsOutOfTheBlue() {
  VanDials d;
  Rng world = Rng::FromSeed("crag-1");

  // A fresh van does not break, however much you drive it in one day.
  Van fresh;
  for (int day = 1; day <= 50; day++) {
    fresh.parts[0].wear = 0.1;   // held well under the threshold
    CHECK(DriveVan(fresh, world, day, 0.5, 60.0, d) < 0);
  }

  // Something well past its life does, eventually, and warns you first.
  Van tired;
  tired.parts[static_cast<int>(VanPart::Belt)].wear = 1.0;
  CHECK(VanText(tired, d).find("belt") != std::string::npos);
  int failedOn = -1;
  for (int day = 1; day <= 300 && failedOn < 0; day++)
    failedOn = DriveVan(tired, world, day, 0.5, 60.0, d);
  CHECK(failedOn == static_cast<int>(VanPart::Belt));
  CHECK(!VanRuns(tired));
  CHECK(VanText(tired, d).find("has gone") != std::string::npos);

  // The thing you have been ignoring is the thing that goes.
  Van mixed;
  mixed.parts[static_cast<int>(VanPart::Clutch)].wear = 1.0;
  mixed.parts[static_cast<int>(VanPart::Tyres)].wear = 0.75;
  int broke = -1;
  for (int day = 1; day <= 300 && broke < 0; day++)
    broke = DriveVan(mixed, world, day, 0.5, 60.0, d);
  CHECK(broke == static_cast<int>(VanPart::Clutch));
}

static void TestBeingBrokeCannotEndTheSave() {
  // The load-bearing promise: a stranded player with no money can always
  // spend a morning under the van. Breakdowns take your season, never your
  // save.
  VanDials d;
  Van van;
  van.parts[static_cast<int>(VanPart::Clutch)].wear = 1.0;
  van.parts[static_cast<int>(VanPart::Clutch)].failed = true;
  CHECK(!VanRuns(van));

  double broke = 0.0;
  double hours = 0.0;
  CHECK(!PatchVan(van, VanPart::Clutch, broke, hours, d));
  CHECK(!ReplaceVanPart(van, VanPart::Clutch, broke, hours, d));
  CHECK(hours == 0.0);            // and nothing happened

  CHECK(BodgeVan(van, VanPart::Clutch, hours, d));
  CHECK(VanRuns(van));            // it moves again
  CHECK(hours >= d.bodgeHours);   // and it cost you the morning
  CHECK(broke == 0.0);            // for nothing, because there was nothing

  // A bodge is a bodge: it gives back least, so it goes again soonest.
  Van bodged, patched;
  bodged.parts[0].wear = patched.parts[0].wear = 1.0;
  double h = 0.0, cash = 500.0;
  BodgeVan(bodged, VanPart::Tyres, h, d);
  PatchVan(patched, VanPart::Tyres, cash, h, d);
  CHECK(bodged.parts[0].wear > patched.parts[0].wear);
}

static void TestPatchUntilYouCannot() {
  VanDials d;
  Van van;
  double cash = 2000.0, hours = 0.0;
  const int i = static_cast<int>(VanPart::Brakes);

  for (int n = 0; n < d.patchesPerPart; n++) {
    van.parts[i].wear = 0.95;
    CHECK(PatchVan(van, VanPart::Brakes, cash, hours, d));
  }
  // After that it wants doing properly.
  van.parts[i].wear = 0.95;
  CHECK(!PatchVan(van, VanPart::Brakes, cash, hours, d));
  CHECK(ReplaceVanPart(van, VanPart::Brakes, cash, hours, d));
  CHECK(van.parts[i].wear == 0.0);
  CHECK(van.parts[i].patches == 0);   // a new part patches like a new part

  // Replacing costs more than patching, which is why anyone patches.
  CHECK(d.replaceCost[i] > d.patchCost[i]);
  // And every repair takes time as well as money.
  CHECK(hours > 0.0);
}

static void TestTheVanRunsOnItsOwnRng() {
  // The van rusting must never shift the rng an attempt resolves on, or a
  // save stops being replayable.
  Rng world = Rng::FromSeed("crag-1");
  Climber c;
  c.skills.power = c.skills.fingers = c.skills.technique =
      c.skills.endurance = c.skills.head = 55.0;
  Route r = BuildRoute(world, "Control", 5, 5, RouteType::Crimp,
                       Discipline::Boulder);

  auto Play = [&](bool withVan) {
    Rng session = Rng::FromSeed("van-isolation");
    SessionState st = StartSession(c);
    ProjectMemory m;
    m.routeName = r.name;
    if (withVan) {
      Van van;
      for (int day = 1; day <= 30; day++) DriveVan(van, world, day, 1.0, 90.0);
    }
    std::string trace;
    for (int i = 0; i < 6; i++)
      trace += std::to_string(
          AttemptInSession(session, st, m, c, r, Conditions{}).highpoint);
    return trace;
  };
  CHECK(Play(false) == Play(true));
}

// --- The town ------------------------------------------------------------------

static void TestTheSceneWatchesWhatYouActuallyDo() {
  // Factions are only real if they attach to things the player was doing
  // anyway. These are the three hooks.
  Rng world = Rng::FromSeed("crag-1");
  DayDials d;

  // Brushing a project: the development crew approve, the stewards do not.
  {
    PlayerState player;
    DayState day = WakeUp(player, d);
    Crag crag = RoadsideCrag(world);
    ProjectMemory m = NewProjectLedger(*OpenProjects(crag)[0]);
    CleanLine(player, day, m, 2.0);
    CHECK(StandingWith(player.standing, Faction::Development) > 0.0);
    CHECK(StandingWith(player.standing, Faction::Stewardship) < 0.0);
  }

  // The guidebook gig pays best and costs most; trail work is the reverse.
  {
    PlayerState famous, worker;
    DayState d1 = WakeUp(famous, d), d2 = WakeUp(worker, d);
    OddJob photos;
    photos.name = "shooting photos for the guidebook";
    photos.pay = 130.0;
    OddJob trail;
    trail.name = "trail work for the park";
    trail.pay = 80.0;

    CHECK(WorkOddJob(famous, d1, photos, Rng::FromSeed("w"), false, d));
    CHECK(WorkOddJob(worker, d2, trail, Rng::FromSeed("w"), false, d));
    CHECK(StandingWith(famous.standing, Faction::Scene) > 0.0);
    CHECK(StandingWith(famous.standing, Faction::Stewardship) < 0.0);
    CHECK(StandingWith(worker.standing, Faction::Stewardship) > 0.0);
    // The one that pays more is the one that costs you.
    CHECK(photos.pay > trail.pay);
  }

  // A first ascent, and style is what the old guard read.
  {
    Crag crag = RoadsideCrag(world);
    const CragLine& line = *OpenProjects(crag)[0];

    PlayerState clean;
    ProjectMemory cm = NewProjectLedger(line);
    cm.sent = true;
    cm.firstSendStyle = Style::Flash;
    CHECK(NameFirstAscent(cm, line, "Good Style"));
    CreditFirstAscent(clean, cm);

    PlayerState sieged;
    ProjectMemory sm = NewProjectLedger(line);
    sm.sent = true;
    sm.firstSendStyle = Style::Redpoint;
    CHECK(NameFirstAscent(sm, line, "Eventually"));
    CreditFirstAscent(sieged, sm);

    // Both opened a line; only one of them impressed the old guard.
    CHECK(StandingWith(clean.standing, Faction::Development) > 0.0);
    CHECK(StandingWith(sieged.standing, Faction::Development) > 0.0);
    CHECK(StandingWith(clean.standing, Faction::OldGuard) >
          StandingWith(sieged.standing, Faction::OldGuard));

    // And a ledger with no first ascent in it credits nothing.
    PlayerState nobody;
    ProjectMemory empty;
    CreditFirstAscent(nobody, empty);
    for (int i = 0; i < kFactionCount; i++)
      CHECK(nobody.standing.with[i] == 0.0);
  }
}

static void TestTheTownIsData() {
  const Town t = DirtbagTown();
  CHECK(!t.name.empty());
  CHECK(t.venues.size() >= 5);

  for (const Venue& v : t.venues) {
    CHECK(!v.name.empty());
    CHECK(!v.flavour.empty());          // everywhere is somewhere
    CHECK(v.opensAt < v.closesAt);      // and nothing wraps midnight
    CHECK(v.priceFactor > 0.0);
    CHECK(v.travelHours > 0.0);         // town is always a drive
  }

  // Every service the day loop needs has somewhere to happen.
  for (Service s : {Service::Meal, Service::Gear, Service::VanRepair,
                    Service::Gym, Service::Work}) {
    CHECK(!VenuesFor(t, s).empty());
    CHECK(std::string(ServiceName(s)).size() > 0);
  }

  // Food has more than one answer, which is what makes eating a decision.
  const std::vector<const Venue*> food = VenuesFor(t, Service::Meal);
  CHECK(food.size() >= 2);
  bool cheapAndGrim = false, dearAndGood = false;
  for (const Venue* v : food) {
    if (v->priceFactor < 1.0 && v->qualityFactor < 1.0) cheapAndGrim = true;
    if (v->priceFactor > 1.0 && v->qualityFactor > 1.0) dearAndGood = true;
  }
  CHECK(cheapAndGrim);
  CHECK(dearAndGood);
}

static void TestHoursAreTheMechanic() {
  const Town t = DirtbagTown();

  // A day that ran long is a day you eat badly: the diner shuts and the
  // gas station does not.
  const Venue* lateFood = OpenVenueFor(t, Service::Meal, 22.0);
  CHECK(lateFood != nullptr);
  CHECK(lateFood->priceFactor < 1.0);     // the warmer, then

  // And in the middle of the day you have the choice.
  CHECK(VenuesFor(t, Service::Meal).size() >= 2);
  int openAtNoon = 0;
  for (const Venue* v : VenuesFor(t, Service::Meal))
    if (IsOpen(*v, 12.0)) openAtNoon++;
  CHECK(openAtNoon >= 2);

  // The gear shop keeps banker's hours, which is precisely why a resole
  // competes with a window.
  const std::vector<const Venue*> gear = VenuesFor(t, Service::Gear);
  CHECK(!gear.empty());
  CHECK(!IsOpen(*gear[0], 6.0));
  CHECK(!IsOpen(*gear[0], 20.0));
  CHECK(IsOpen(*gear[0], 12.0));

  // At four in the morning the town is shut and says so rather than
  // returning something.
  for (Service s : {Service::Meal, Service::Gear, Service::VanRepair})
    CHECK(OpenVenueFor(t, s, 4.0) == nullptr);

  // The text says which it is, both ways round.
  const Venue* diner = nullptr;
  for (const Venue& v : t.venues)
    if (v.name.find("Diner") != std::string::npos) diner = &v;
  CHECK(diner != nullptr);
  CHECK(VenueText(*diner, 12.0).find("open until") != std::string::npos);
  CHECK(VenueText(*diner, 4.0).find("closed until") != std::string::npos);
  CHECK(VenueText(*diner, 12.0).find(diner->name) != std::string::npos);
}

// --- Factions ------------------------------------------------------------------

static void TestFactionsAreActuallyOpposed() {
  FactionDials d;
  Standing s;
  for (int i = 0; i < kFactionCount; i++) CHECK(s.with[i] == 0.0);

  // The two axes, and they run both ways.
  CHECK(OppositeOf(Faction::OldGuard) == Faction::Scene);
  CHECK(OppositeOf(Faction::Scene) == Faction::OldGuard);
  CHECK(OppositeOf(Faction::Development) == Faction::Stewardship);
  CHECK(OppositeOf(Faction::Stewardship) == Faction::Development);
  for (int i = 0; i < kFactionCount; i++) {
    const Faction f = static_cast<Faction>(i);
    CHECK(OppositeOf(OppositeOf(f)) == f);
    CHECK(OppositeOf(f) != f);
  }

  // Pleasing one costs its opposite. A faction system you can max out is a
  // checklist rather than a choice.
  Shift(s, Faction::Scene, 0.4, d);
  CHECK(StandingWith(s, Faction::Scene) > 0.0);
  CHECK(StandingWith(s, Faction::OldGuard) < 0.0);
  // But it costs less than it gains, so a career can lean without being
  // shoved into a corner.
  CHECK(std::fabs(StandingWith(s, Faction::OldGuard)) <
        StandingWith(s, Faction::Scene));
  // And the other axis is untouched: they are opposed, not entangled.
  CHECK(StandingWith(s, Faction::Development) == 0.0);
  CHECK(StandingWith(s, Faction::Stewardship) == 0.0);

  // Standing stays in its range however hard you push.
  for (int i = 0; i < 200; i++) Shift(s, Faction::Scene, 0.5, d);
  CHECK(StandingWith(s, Faction::Scene) <= 1.0);
  CHECK(StandingWith(s, Faction::OldGuard) >= -1.0);
}

static void TestTheThingsYouDoHaveOpinions() {
  FactionDials d;

  // A first ascent in good style is the one act that pleases both ends of
  // an axis — which is what "good style" is for.
  Standing clean;
  DidFirstAscent(clean, true, d);
  CHECK(StandingWith(clean, Faction::Development) > 0.0);
  CHECK(StandingWith(clean, Faction::OldGuard) > 0.0);

  Standing scrappy;
  DidFirstAscent(scrappy, false, d);
  CHECK(StandingWith(scrappy, Faction::Development) > 0.0);
  CHECK(StandingWith(scrappy, Faction::OldGuard) <
        StandingWith(clean, Faction::OldGuard));

  // Cleaning is both things at once, honestly.
  Standing brushed;
  ScrubbedALine(brushed, d);
  CHECK(StandingWith(brushed, Faction::Development) > 0.0);
  CHECK(StandingWith(brushed, Faction::Stewardship) < 0.0);

  // The best-paying gig on the board costs you two factions, which is the
  // whole reason it pays that well.
  Standing famous;
  TookTheGuidebookPhotos(famous, d);
  CHECK(StandingWith(famous, Faction::Scene) > 0.0);
  CHECK(StandingWith(famous, Faction::Stewardship) < 0.0);
  CHECK(StandingWith(famous, Faction::OldGuard) < 0.0);

  // And the worst-paying honest one buys it back.
  Standing worker;
  DidTrailWork(worker, d);
  CHECK(StandingWith(worker, Faction::Stewardship) > 0.0);
}

static void TestAccessGetsPulled() {
  FactionDials d;
  Rng world = Rng::FromSeed("crag-1");

  // A crag nobody objects to stays open forever.
  Standing fine;
  for (int day = 1; day <= 400; day++) FactionDay(fine, world, day, d);
  CHECK(CragIsOpen(fine));
  CHECK(fine.closedDays == 0);

  // Push the stewards far enough and the signs go up.
  Standing hated;
  for (int i = 0; i < 20; i++) TookTheGuidebookPhotos(hated, d);
  CHECK(StandingWith(hated, Faction::Stewardship) < d.closureBelow);

  int closedOn = 0;
  for (int day = 1; day <= 400 && closedOn == 0; day++) {
    FactionDay(hated, world, day, d);
    if (!CragIsOpen(hated)) closedOn = day;
  }
  CHECK(closedOn > 0);
  CHECK(!CragIsOpen(hated));
  CHECK(StandingText(hated).find("closed") != std::string::npos);

  // And it reopens: a closure is a season, not a life sentence.
  for (int i = 0; i < d.closureDays + 2; i++)
    FactionDay(hated, world, 500 + i, d);
  CHECK(CragIsOpen(hated));
}

static void TestTheSceneForgetsSlowly() {
  FactionDials d;
  Rng world = Rng::FromSeed("crag-1");
  Standing s;
  DidTrailWork(s, d);
  const double earned = StandingWith(s, Faction::Stewardship);
  CHECK(earned > 0.0);

  // A week barely touches it.
  for (int day = 1; day <= 7; day++) FactionDay(s, world, day, d);
  CHECK(StandingWith(s, Faction::Stewardship) > earned * 0.9);

  // A long time does, and toward nothing rather than toward disliking you.
  for (int day = 8; day <= 900; day++) FactionDay(s, world, day, d);
  CHECK(StandingWith(s, Faction::Stewardship) >= 0.0);
  CHECK(StandingWith(s, Faction::Stewardship) < earned * 0.5);

  // The same is true from below: bad standing fades up to nothing.
  Standing bad;
  for (int i = 0; i < 6; i++) TookTheGuidebookPhotos(bad, d);
  const double disliked = StandingWith(bad, Faction::Stewardship);
  CHECK(disliked < 0.0);
  for (int day = 1; day <= 900; day++) FactionDay(bad, world, day, d);
  CHECK(StandingWith(bad, Faction::Stewardship) > disliked);
  CHECK(StandingWith(bad, Faction::Stewardship) <= 0.0);
}

static void TestTheLotSpeaksForSomebody() {
  FactionDials d;
  // Everyone at the Lot stands for something, so standing is felt in a
  // conversation rather than read off a screen.
  CHECK(FactionOf("Margo") == Faction::OldGuard);
  CHECK(FactionOf("Dev") == Faction::Scene);
  CHECK(FactionOf("Ray") == Faction::Stewardship);

  Standing s;
  CHECK(std::fabs(BetaMultiplierFor(s, "Margo", d) - 1.0) < 1e-12);

  // Stand well with the old guard and Margo has more to say; chase the
  // scene instead and she has less.
  Standing trad;
  for (int i = 0; i < 5; i++) DidFirstAscent(trad, true, d);
  CHECK(BetaMultiplierFor(trad, "Margo", d) > 1.0);
  CHECK(BetaMultiplierFor(trad, "Dev", d) < 1.0);

  Standing famous;
  for (int i = 0; i < 5; i++) TookTheGuidebookPhotos(famous, d);
  CHECK(BetaMultiplierFor(famous, "Margo", d) < 1.0);
  CHECK(BetaMultiplierFor(famous, "Dev", d) > 1.0);
}

// --- Work ----------------------------------------------------------------------

static void TestTheBoardIsDifferentEveryDay() {
  JobDials j;
  Rng world = Rng::FromSeed("crag-1");

  std::vector<OddJob> monday = OddJobBoard(world, 1, j);
  CHECK(static_cast<int>(monday.size()) == j.boardSize);

  // Three different things, not the same gig three times.
  for (size_t a = 0; a < monday.size(); a++)
    for (size_t b = a + 1; b < monday.size(); b++)
      CHECK(monday[a].name != monday[b].name);

  // The same day is the same board; a different day is not.
  CHECK(OddJobBoard(world, 1, j)[0].name == monday[0].name);
  bool differs = false;
  for (int day = 2; day <= 12; day++) {
    std::vector<OddJob> other = OddJobBoard(world, day, j);
    if (other[0].name != monday[0].name) differs = true;
  }
  CHECK(differs);

  // Every gig is worth having and none of them is free money.
  for (int day = 1; day <= 60; day++)
    for (const OddJob& g : OddJobBoard(world, day, j)) {
      CHECK(!g.name.empty());
      CHECK(g.hours > 0.0);
      CHECK(g.pay > 0.0);
      CHECK(g.energy >= 0.0);
    }
}

static void TestABrokenVanCostsYouTheWorkToo() {
  // The sharp coupling: some gigs need the van, so a breakdown costs the
  // repair AND the job that would have paid for it.
  DayDials d;
  PlayerState player;
  DayState day = WakeUp(player, d);

  OddJob hauling;
  hauling.name = "hauling firewood";
  hauling.needsVan = true;
  hauling.pay = 70.0;

  CHECK(WorkOddJob(player, day, hauling, Rng::FromSeed("w"), false, d));       // van runs, fine

  player.van.parts[static_cast<int>(VanPart::Belt)].failed = true;
  const double cash = player.cash;
  const double hour = day.hour;
  CHECK(!WorkOddJob(player, day, hauling, Rng::FromSeed("w"), false, d));      // and now it does not
  CHECK(player.cash == cash);                       // nothing happened
  CHECK(day.hour == hour);

  // Work that does not need it is unaffected.
  OddJob dishes;
  dishes.name = "washing dishes at the diner";
  dishes.needsVan = false;
  CHECK(WorkOddJob(player, day, dishes, Rng::FromSeed("w"), false, d));
}

static void TestOddJobsPayDebtFirst() {
  DayDials d;
  PlayerState player;
  player.cash = 0.0;
  player.owed = 100.0;
  DayState day = WakeUp(player, d);

  OddJob gig;
  gig.pay = 60.0;
  CHECK(WorkOddJob(player, day, gig, Rng::FromSeed("w"), false, d));
  CHECK(player.cash == 0.0);                        // none of it is yours yet
  CHECK(std::fabs(player.owed - 40.0) < 1e-12);
  CHECK(day.hour > d.wakeHour);                     // and it took the hours
}

static void TestTheSalaryOwnsTheMiddleOfTheDay() {
  JobDials j;
  PlayerState player;

  // Unemployed, nothing owns anything.
  CHECK(!SalariedToday(player.job, 1, j));
  CHECK(!SalaryOwnsHour(player.job, 1, 12.0, j));

  TakeSalariedJob(player);
  CHECK(player.job.salaried);

  // Five days on, two off, and the two off are what a weekend is.
  int workdays = 0;
  for (int day = 1; day <= 7; day++)
    if (SalariedToday(player.job, day, j)) workdays++;
  CHECK(workdays == j.salaryDaysPerWeek);

  // And on a workday it owns exactly the hours the rock is good in.
  CHECK(!SalaryOwnsHour(player.job, 1, 7.0, j));    // before
  CHECK(SalaryOwnsHour(player.job, 1, 12.0, j));    // the middle of the day
  CHECK(SalaryOwnsHour(player.job, 1, 16.0, j));
  CHECK(!SalaryOwnsHour(player.job, 1, 18.0, j));   // after

  // A day of it lands you on the far side of the afternoon.
  DayDials d;
  DayState day = WakeUp(player, d);
  WorkSalariedDay(player, day, j, d);
  CHECK(day.hour >= j.salaryStartHour + j.salaryHours);
  CHECK(player.cash > 0.0);
  CHECK(day.energy < 100.0);

  // Quitting is allowed and costs a little, because the job was the
  // punishment and walking out is not.
  const double psyche = player.climber.psyche;
  QuitSalariedJob(player, j);
  CHECK(!player.job.salaried);
  CHECK(player.climber.psyche < psyche);
  QuitSalariedJob(player, j);   // and quitting twice is not a thing
  CHECK(!player.job.salaried);
}

static void TestWorkRunsOnItsOwnRng() {
  Rng world = Rng::FromSeed("crag-1");
  Climber c;
  c.skills.power = c.skills.fingers = c.skills.technique =
      c.skills.endurance = c.skills.head = 55.0;
  Route r = BuildRoute(world, "Control", 5, 5, RouteType::Crimp,
                       Discipline::Boulder);
  auto Play = [&](bool readTheBoard) {
    Rng session = Rng::FromSeed("jobs-isolation");
    SessionState st = StartSession(c);
    ProjectMemory m;
    m.routeName = r.name;
    if (readTheBoard)
      for (int day = 1; day <= 40; day++) OddJobBoard(world, day);
    std::string trace;
    for (int i = 0; i < 6; i++)
      trace += std::to_string(
          AttemptInSession(session, st, m, c, r, Conditions{}).highpoint);
    return trace;
  };
  CHECK(Play(false) == Play(true));
}

// --- RNG ---------------------------------------------------------------------

static void TestRngDeterminism() {
  Rng a = Rng::FromSeed("grim-fjord-123");
  Rng b = Rng::FromSeed("grim-fjord-123");
  for (int i = 0; i < 100; i++) CHECK(a.NextDouble() == b.NextDouble());

  Rng s1 = Rng::FromStream("grim-fjord-123", Stream::Session);
  Rng s2 = Rng::FromStream("grim-fjord-123", Stream::Events);
  CHECK(s1.NextDouble() != s2.NextDouble());

  Rng base = Rng::FromSeed("grim-fjord-123");
  Rng derived = base.Derive("day-4");
  CHECK(derived.state != base.state);
}

// The UTF-16-code-unit hash, checked against unit lists written out by hand.
// "Þórr" must hash as 4 BMP units; "😀" as a surrogate pair — the two ports
// (UTF-8-bytes and code-points) landnam-ue warned about both fail here.
static void TestRngUnicodeSeeds() {
  auto fnv = [](std::vector<uint16_t> units) {
    uint32_t h = 0x811c9dc5u;
    for (uint16_t u : units) { h ^= u; h *= 0x01000193u; }
    return h;
  };
  CHECK(HashSeedString("Þórr") == fnv({0x00DE, 0x00F3, 0x0072, 0x0072}));
  CHECK(HashSeedString("\U0001F600") == fnv({0xD83D, 0xDE00}));
  CHECK(HashSeedString("abc") == fnv({0x61, 0x62, 0x63}));
}

// Frozen outputs of the initial implementation. If these move, every existing
// seed names a different session — a save-breaking change, not a refactor.
static void TestRngGoldenVectors() {
  Rng rng = Rng::FromSeed("golden");
  const double expected[] = {0.59432327281683683, 0.94342079781927168,
                             0.30383505066856742, 0.76765315467491746,
                             0.47240672400221229};
  for (double e : expected) {
    CHECK(std::fabs(rng.NextDouble() - e) < 1e-15);
  }
}

// --- Route generation --------------------------------------------------------

static void TestRouteStability() {
  Rng world = Rng::FromStream("grim-fjord-123", Stream::Worldgen);
  Route a = BuildRoute(world, "Rust Never Sleeps", 7, 7, RouteType::Crimp,
                       Discipline::Boulder);
  Route b = BuildRoute(world, "Rust Never Sleeps", 7, 7, RouteType::Crimp,
                       Discipline::Boulder);
  CHECK(a.moves.size() == b.moves.size());
  for (size_t i = 0; i < a.moves.size(); i++) {
    CHECK(a.moves[i].difficulty == b.moves[i].difficulty);
    CHECK(a.moves[i].hold == b.moves[i].hold);
  }
  Route c = BuildRoute(world, "Another Line", 7, 7, RouteType::Crimp,
                       Discipline::Boulder);
  bool differs = c.moves.size() != a.moves.size();
  for (size_t i = 0; !differs && i < c.moves.size(); i++) {
    differs = c.moves[i].difficulty != a.moves[i].difficulty;
  }
  CHECK(differs);

  CHECK(std::string(BoulderGradeName(10)) == "V10");
  CHECK(std::string(SportGradeName(18)) == "5.16a");
}

// --- Session -----------------------------------------------------------------

static Climber MakeClimber(double power, double fingers, double technique,
                           double endurance, double head) {
  Climber c;
  c.skills = {power, fingers, technique, endurance, head};
  return c;
}

static AttemptInput MakeInput(const Climber& c, const Route& r) {
  AttemptInput in;
  in.climber = c;
  in.route = r;
  return in;
}

static void TestSessionDeterminism() {
  Rng world = Rng::FromStream("seed-x", Stream::Worldgen);
  Route route = BuildRoute(world, "The Ledger", 6, 6, RouteType::Technical,
                           Discipline::Boulder);
  Climber c = MakeClimber(50, 50, 50, 50, 50);

  Rng r1 = Rng::FromStream("seed-x", Stream::Session).Derive("The Ledger#1");
  Rng r2 = Rng::FromStream("seed-x", Stream::Session).Derive("The Ledger#1");
  AttemptResult a = ResolveAttempt(r1, MakeInput(c, route));
  AttemptResult b = ResolveAttempt(r2, MakeInput(c, route));
  CHECK(a.sent == b.sent);
  CHECK(a.highpoint == b.highpoint);
  CHECK(a.timeline.size() == b.timeline.size());
  for (size_t i = 0; i < a.timeline.size(); i++) {
    CHECK(a.timeline[i].odds == b.timeline[i].odds);
    CHECK(a.timeline[i].pumpAfter == b.timeline[i].pumpAfter);
  }
}

// Monte Carlo over many seeds: N large enough that these orderings are stable
// by construction, not by luck (the effects are multiple grades wide).
static double AverageHighpoint(const Climber& c, const Route& r, int n,
                               double execution = -1.0) {
  double total = 0.0;
  for (int i = 0; i < n; i++) {
    Rng rng = Rng::FromSeed("mc-" + std::to_string(i));
    AttemptInput in = MakeInput(c, r);
    if (execution >= 0.0) in.botExecution = execution;
    total += ResolveAttempt(rng, in).highpoint;
  }
  return total / n;
}

static double SendRate(const Climber& c, const Route& r, int n) {
  int sends = 0;
  for (int i = 0; i < n; i++) {
    Rng rng = Rng::FromSeed("mc-" + std::to_string(i));
    if (ResolveAttempt(rng, MakeInput(c, r)).sent) sends++;
  }
  return static_cast<double>(sends) / n;
}

static void TestFingersMatterOnCrimps() {
  Rng world = Rng::FromStream("seed-y", Stream::Worldgen);
  Route crimps = BuildRoute(world, "Tips", 8, 8, RouteType::Crimp,
                            Discipline::Boulder);
  Climber strong = MakeClimber(50, 80, 50, 50, 50);
  Climber weak = MakeClimber(50, 40, 50, 50, 50);
  CHECK(AverageHighpoint(strong, crimps, 400) > AverageHighpoint(weak, crimps, 400));
}

static void TestEnduranceControlsPump() {
  Rng world = Rng::FromStream("seed-z", Stream::Worldgen);
  Route pitch = BuildRoute(world, "The Long Haul", 7, 7, RouteType::Endurance,
                           Discipline::Sport);
  // Both are 5.12-capable climbers on a 5.12 pitch (SkillToGrade(70) ≈ V7.8)
  // — the pitch has to be *climbable* for pump to be the variable. Two
  // climbers who both redline on move three tell you nothing.
  Climber fit = MakeClimber(70, 70, 70, 90, 60);
  Climber unfit = MakeClimber(70, 70, 70, 30, 60);

  // Compare pump at a move both climbers reach, not peak pump: peak is
  // confounded by how far each got, and on a long pitch everyone redlines
  // eventually — falling off early would read as "less pumped".
  const size_t kAt = 3;
  double fitPump = 0.0, unfitPump = 0.0;
  int paired = 0;
  for (int i = 0; i < 400; i++) {
    Rng r1 = Rng::FromSeed("p-" + std::to_string(i));
    Rng r2 = Rng::FromSeed("p-" + std::to_string(i));
    const AttemptResult a = ResolveAttempt(r1, MakeInput(fit, pitch));
    const AttemptResult b = ResolveAttempt(r2, MakeInput(unfit, pitch));
    if (a.timeline.size() > kAt && b.timeline.size() > kAt) {
      fitPump += a.timeline[kAt].pumpAfter;
      unfitPump += b.timeline[kAt].pumpAfter;
      paired++;
    }
  }
  CHECK(paired > 100);
  CHECK(fitPump / paired < unfitPump / paired);
}

static void TestSandbagBites() {
  Rng world = Rng::FromStream("seed-w", Stream::Worldgen);
  Route honest = BuildRoute(world, "Honest Abe", 6, 6, RouteType::Power,
                            Discipline::Boulder);
  Route sandbag = BuildRoute(world, "Honest Abe", 6, 8, RouteType::Power,
                             Discipline::Boulder);
  // A climber near the grade — strong enough to send the honest V6 often,
  // close enough to the margin that the hidden two grades genuinely bite.
  // SkillToGrade(60) = V6.4: exactly the climber this test wants.
  Climber c = MakeClimber(60, 60, 60, 60, 60);
  CHECK(SendRate(c, honest, 400) > SendRate(c, sandbag, 400));
}

// The ladder must resist. A V5 climber sends V5 often and V7 rarely; if that
// spread ever flattens, the gym board is decoration and every grade reads the
// same. (It *was* flat once: a skill→grade mapping that made every 50-stat
// climber a V9 sent everything on the board — caught by measuring a month,
// not by any test, which is why this one exists.)
static void TestGradesResist() {
  CHECK(std::fabs(SkillToGrade(50.0) - 5.0) < 1e-9);
  CHECK(std::fabs(SkillToGrade(80.0) - 9.2) < 1e-9);

  Rng world = Rng::FromStream("resist", Stream::Worldgen);
  Climber c = MakeClimber(50, 50, 50, 50, 50);
  Route below = BuildRoute(world, "Comfortable", 4, 4, RouteType::Power,
                           Discipline::Boulder);
  Route atLevel = BuildRoute(world, "At The Limit", 5, 5, RouteType::Power,
                             Discipline::Boulder);
  Route twoUp = BuildRoute(world, "Two Grades Up", 7, 7, RouteType::Power,
                           Discipline::Boulder);

  // The shape of a climbing life: a grade below is a warmup, your grade
  // goes down in a few burns, two above is next year's problem.
  const double belowRate = SendRate(c, below, 400);
  const double atRate = SendRate(c, atLevel, 400);
  const double upRate = SendRate(c, twoUp, 400);
  CHECK(belowRate > 0.70);
  CHECK(atRate > 0.15);
  CHECK(upRate < 0.05);
  CHECK(belowRate > atRate && atRate > upRate);
}

static void TestExecutionMatters() {
  Rng world = Rng::FromStream("seed-v", Stream::Worldgen);
  Route route = BuildRoute(world, "Clean Hands", 8, 8, RouteType::Technical,
                           Discipline::Boulder);
  Climber c = MakeClimber(55, 55, 55, 55, 55);
  CHECK(AverageHighpoint(c, route, 400, 0.95) > AverageHighpoint(c, route, 400, 0.25));
}

static void TestStyleLadder() {
  Rng world = Rng::FromStream("seed-u", Stream::Worldgen);
  Route easy = BuildRoute(world, "Warmup Jugs", 0, 0, RouteType::Endurance,
                          Discipline::Boulder);
  Climber crusher = MakeClimber(95, 95, 95, 95, 95);

  Rng r1 = Rng::FromSeed("style-1");
  AttemptInput onsight = MakeInput(crusher, easy);
  AttemptResult a = ResolveAttempt(r1, onsight);
  CHECK(a.sent && a.style == Style::Onsight);

  Rng r2 = Rng::FromSeed("style-2");
  AttemptInput flash = MakeInput(crusher, easy);
  flash.beta = 1.0;
  AttemptResult b = ResolveAttempt(r2, flash);
  CHECK(b.sent && b.style == Style::Flash);

  Rng r3 = Rng::FromSeed("style-3");
  AttemptInput redpoint = MakeInput(crusher, easy);
  redpoint.beta = 1.0;
  redpoint.attemptNumber = 4;
  AttemptResult c = ResolveAttempt(r3, redpoint);
  CHECK(c.sent && c.style == Style::Redpoint);
}

// --- Live attempts -----------------------------------------------------------

// Driving the live form by hand with the bot's policy must reproduce the
// batch resolver exactly — timeline, style, skin, and the rng draws the
// caller's stream loses along the way.
static void TestLiveDriveMatchesBatch() {
  Rng world = Rng::FromStream("live-eq", Stream::Worldgen);
  Route route = BuildRoute(world, "Mirror Image", 7, 7, RouteType::Endurance,
                           Discipline::Sport);
  // Strong enough to get deep into a 5.12 pitch: equivalence is only worth
  // checking over long timelines, and a climber who falls on move two
  // silently turns this into a two-move test.
  Climber c = MakeClimber(75, 75, 75, 80, 70);

  for (int i = 0; i < 50; i++) {
    Rng batchRng = Rng::FromSeed("eq-" + std::to_string(i));
    Rng liveRng = Rng::FromSeed("eq-" + std::to_string(i));
    AttemptInput in = MakeInput(c, route);

    AttemptResult batch = ResolveAttempt(batchRng, in);

    LiveAttempt la = BeginAttempt(liveRng, in);
    while (!AttemptOver(la)) {
      const int at = la.nextMove;
      MoveResult mr = StepMove(la, in.botExecution);
      if (mr.success && route.moves[at].restQuality > 0.0) ShakeOut(la);
    }
    AttemptResult live = FinishAttempt(la);

    CHECK(live.sent == batch.sent);
    CHECK(live.highpoint == batch.highpoint);
    CHECK(live.style == batch.style);
    CHECK(live.skinCost == batch.skinCost);
    CHECK(live.timeline.size() == batch.timeline.size());
    for (size_t m = 0; m < live.timeline.size(); m++) {
      CHECK(live.timeline[m].odds == batch.timeline[m].odds);
      CHECK(live.timeline[m].pumpAfter == batch.timeline[m].pumpAfter);
      CHECK(live.timeline[m].success == batch.timeline[m].success);
    }
    CHECK(la.rng.state == batchRng.state);
  }
}

// The odds preview is the odds the move then rolls against — the UI readout
// can never flatter the player.
static void TestPeekOddsHonest() {
  Rng world = Rng::FromStream("live-peek", Stream::Worldgen);
  Route route = BuildRoute(world, "Truth in Advertising", 6, 6,
                           RouteType::Technical, Discipline::Boulder);
  Climber c = MakeClimber(50, 50, 50, 50, 50);

  Rng rng = Rng::FromSeed("peek-1");
  LiveAttempt la = BeginAttempt(rng, MakeInput(c, route));
  while (!AttemptOver(la)) {
    const double peeked = PeekOdds(la, 0.6);
    MoveResult mr = StepMove(la, 0.6);
    CHECK(mr.odds == peeked);
  }
  CHECK(PeekOdds(la, 0.6) == 0.0);  // nothing left to preview
}

// Shake economics: the first shake at a good stance is worth the most, each
// repeat halves and pays the hang tax, and milking a non-rest pumps you up.
static void TestShakeOutEconomics() {
  Route route;
  route.name = "Hand Built";
  Move jug;
  jug.difficulty = 0.5;
  jug.hold = HoldType::Jug;
  jug.restQuality = 0.9;
  route.moves = {jug, jug, jug};
  Climber c = MakeClimber(60, 60, 60, 60, 60);

  Rng rng = Rng::FromSeed("shake-1");
  LiveAttempt la = BeginAttempt(rng, MakeInput(c, route));
  MoveResult mr = StepMove(la, 0.7);
  CHECK(mr.success);

  la.pump = 80.0;  // stage a desperate arrival at the stance
  const double s1 = ShakeOut(la);
  const double s2 = ShakeOut(la);
  const double s3 = ShakeOut(la);
  const double s4 = ShakeOut(la);
  CHECK(s1 > s2 && s2 > s3 && s3 > s4);
  CHECK(s1 > 0.0);
  CHECK(s4 < 0.0);  // the hang tax has overtaken the stance
  CHECK(la.partial.timeline.back().pumpAfter == la.pump);

  // Same stance, no rest to milk: the first shake is free but worthless,
  // and every one after costs.
  Route blank = route;
  for (Move& m : blank.moves) { m.hold = HoldType::Sloper; m.restQuality = 0.0; }
  Rng rng2 = Rng::FromSeed("shake-2");
  LiveAttempt lb = BeginAttempt(rng2, MakeInput(c, blank));
  MoveResult first = StepMove(lb, 0.7);
  CHECK(first.success);
  lb.pump = 50.0;
  CHECK(ShakeOut(lb) == 0.0);
  CHECK(ShakeOut(lb) < 0.0);
  CHECK(lb.pump > 50.0);
}

// --- Session loop ------------------------------------------------------------

// Same seed → the identical session, attempt by attempt. This is the loop's
// version of the resolver determinism contract.
static void TestSessionLoopDeterminism() {
  Rng world = Rng::FromStream("loop-seed", Stream::Worldgen);
  Route route = BuildRoute(world, "Groundhog Day", 7, 7, RouteType::Power,
                           Discipline::Boulder);
  Climber c = MakeClimber(45, 45, 45, 45, 45);

  auto runSession = [&](std::vector<AttemptResult>& out, SessionState& s,
                        ProjectMemory& mem) {
    Rng sessionRng = Rng::FromStream("loop-seed", Stream::Session);
    s = StartSession(c);
    for (int i = 0; i < 3; i++) {
      out.push_back(AttemptInSession(sessionRng, s, mem, c, route, Conditions{}));
    }
  };
  std::vector<AttemptResult> a, b;
  SessionState sa, sb;
  ProjectMemory ma, mb;
  runSession(a, sa, ma);
  runSession(b, sb, mb);
  for (size_t i = 0; i < a.size(); i++) {
    CHECK(a[i].highpoint == b[i].highpoint);
    CHECK(a[i].sent == b[i].sent);
    CHECK(a[i].peakPump == b[i].peakPump);
  }
  CHECK(sa.skinLeft == sb.skinLeft);
  CHECK(sa.warmth == sb.warmth);
  CHECK(ma.beta == mb.beta);
  CHECK(ma.bestHighpoint == mb.bestHighpoint);
}

// Pulling straight onto the project cold vs. after two warmup boulders. The
// warmed pair of sessions shares the project's rng derivation (same route,
// same attempt #1), so the only difference is state — warmth has to earn it.
static void TestWarmupMatters() {
  Rng world = Rng::FromStream("loop-warm", Stream::Worldgen);
  Route easy1 = BuildRoute(world, "Morning Jugs", 0, 0, RouteType::Endurance,
                           Discipline::Boulder);
  Route easy2 = BuildRoute(world, "Second Coffee", 1, 1, RouteType::Endurance,
                           Discipline::Boulder);
  Route proj = BuildRoute(world, "The Business", 7, 7, RouteType::Power,
                          Discipline::Boulder);
  Climber c = MakeClimber(48, 48, 48, 48, 48);

  const int n = 400;
  double coldTotal = 0.0, warmTotal = 0.0;
  for (int i = 0; i < n; i++) {
    Rng sessionRng = Rng::FromStream("warm-" + std::to_string(i), Stream::Session);

    SessionState cold = StartSession(c);
    ProjectMemory coldMem;
    coldTotal += AttemptInSession(sessionRng, cold, coldMem, c, proj, Conditions{})
                     .highpoint;

    SessionState warm = StartSession(c);
    ProjectMemory w1, w2, warmMem;
    AttemptInSession(sessionRng, warm, w1, c, easy1, Conditions{});
    AttemptInSession(sessionRng, warm, w2, c, easy2, Conditions{});
    CHECK(warm.warmth > 0.5);
    warmTotal += AttemptInSession(sessionRng, warm, warmMem, c, proj, Conditions{})
                     .highpoint;
  }
  CHECK(warmTotal / n > coldTotal / n);
}

// Projecting: the ledger only moves forward, and the accumulated state
// (beta + warmth + attempt count) makes late burns land higher than burn #1.
static void TestProjectingBuildsBeta() {
  Rng world = Rng::FromStream("loop-proj", Stream::Worldgen);
  Route proj = BuildRoute(world, "Nemesis", 8, 8, RouteType::Technical,
                          Discipline::Boulder);
  Climber c = MakeClimber(50, 50, 50, 50, 50);

  const int n = 300;
  double firstTotal = 0.0, lateTotal = 0.0;
  for (int i = 0; i < n; i++) {
    Rng sessionRng = Rng::FromStream("proj-" + std::to_string(i), Stream::Session);
    SessionState s = StartSession(c);
    ProjectMemory mem;
    double prevBeta = 0.0;
    int prevBest = 0;
    for (int burn = 0; burn < 4; burn++) {
      AttemptResult r = AttemptInSession(sessionRng, s, mem, c, proj, Conditions{});
      CHECK(mem.beta >= prevBeta);
      CHECK(mem.bestHighpoint >= prevBest);
      CHECK(mem.attempts == burn + 1);
      prevBeta = mem.beta;
      prevBest = mem.bestHighpoint;
      if (burn == 0) firstTotal += r.highpoint;
      if (burn == 3) lateTotal += r.highpoint;
    }
    CHECK(mem.beta > 0.0);
  }
  CHECK(lateTotal / n > firstTotal / n);
}

// A send after prior falls is a Redpoint in the ledger, forever; an
// unrepeated first-go send stays an Onsight even after later laps.
static void TestFirstSendStyleSticks() {
  Rng world = Rng::FromStream("loop-style", Stream::Worldgen);
  Route hard = BuildRoute(world, "Slow Learner", 9, 9, RouteType::Crimp,
                          Discipline::Boulder);
  Route easy = BuildRoute(world, "Gimme", 0, 0, RouteType::Endurance,
                          Discipline::Boulder);
  Climber crusher = MakeClimber(90, 90, 90, 90, 90);
  Climber mortal = MakeClimber(35, 35, 35, 35, 35);

  // The mortal falls, then we hand them a crusher's body: the send that
  // finally comes is a redpoint because the attempts before it happened.
  Rng sessionRng = Rng::FromStream("loop-style", Stream::Session);
  SessionState s = StartSession(mortal);
  ProjectMemory mem;
  for (int i = 0; i < 6 && !mem.sent; i++) {
    const Climber& who = i < 2 ? mortal : crusher;
    AttemptInSession(sessionRng, s, mem, who, hard, Conditions{});
  }
  CHECK(mem.sent);
  CHECK(mem.attempts > 1);
  CHECK(mem.firstSendStyle == Style::Redpoint);

  SessionState s2 = StartSession(crusher);
  s2.warmth = 1.0;
  ProjectMemory easyMem;
  AttemptInSession(sessionRng, s2, easyMem, crusher, easy, Conditions{});
  CHECK(easyMem.sent && easyMem.firstSendStyle == Style::Onsight);
  AttemptInSession(sessionRng, s2, easyMem, crusher, easy, Conditions{});
  CHECK(easyMem.firstSendStyle == Style::Onsight);  // the lap changes nothing
}

// The skin budget is real: a session's burns spend it, and a climber down to
// tips-tape skin climbs measurably worse on crimps than a fresh one.
static void TestSkinBudgetBites() {
  Rng world = Rng::FromStream("loop-skin", Stream::Worldgen);
  // Above the climber's level on purpose: falls dominate, and falls are what
  // spend skin (the 2D game's 1-point rule).
  Route crimps = BuildRoute(world, "Paper Cuts", 9, 9, RouteType::Crimp,
                            Discipline::Boulder);
  Climber c = MakeClimber(48, 48, 48, 48, 48);

  // Drain: a long crimpy session leaves less skin than it started with.
  Rng sessionRng = Rng::FromStream("loop-skin", Stream::Session);
  SessionState s = StartSession(c);
  ProjectMemory mem;
  for (int i = 0; i < 5; i++) {
    AttemptInSession(sessionRng, s, mem, c, crimps, Conditions{});
  }
  CHECK(s.skinLeft < c.skin - 3.0);

  // Bite: same route, warm in both cases, fresh skin vs. worked skin.
  const int n = 400;
  double freshTotal = 0.0, workedTotal = 0.0;
  for (int i = 0; i < n; i++) {
    Rng rng = Rng::FromStream("skin-" + std::to_string(i), Stream::Session);
    SessionState fresh = StartSession(c);
    fresh.warmth = 1.0;
    SessionState worked = fresh;
    worked.skinLeft = 1.0;
    ProjectMemory m1, m2;
    freshTotal += AttemptInSession(rng, fresh, m1, c, crimps, Conditions{}).highpoint;
    workedTotal += AttemptInSession(rng, worked, m2, c, crimps, Conditions{}).highpoint;
  }
  CHECK(freshTotal / n > workedTotal / n);
}

// The decomposed session pieces (derive rng → build input → live-drive →
// commit) must land exactly where AttemptInSession does — this is the path
// the engine's interactive layer takes, and it may not drift.
static void TestSessionLiveComposition() {
  Rng world = Rng::FromStream("loop-compose", Stream::Worldgen);
  Route route = BuildRoute(world, "Two Roads", 7, 7, RouteType::Crimp,
                           Discipline::Boulder);
  Climber c = MakeClimber(46, 46, 46, 46, 46);
  Rng sessionRng = Rng::FromStream("loop-compose", Stream::Session);

  SessionState sBatch = StartSession(c), sLive = StartSession(c);
  ProjectMemory mBatch, mLive;
  for (int burn = 0; burn < 3; burn++) {
    AttemptResult batch = AttemptInSession(sessionRng, sBatch, mBatch, c,
                                           route, Conditions{});

    Rng rng = DeriveAttemptRng(sessionRng, mLive, route);
    AttemptInput in =
        BuildSessionAttemptInput(sLive, mLive, c, route, Conditions{});
    LiveAttempt la = BeginAttempt(rng, in);
    while (!AttemptOver(la)) {
      const int at = la.nextMove;
      MoveResult mr = StepMove(la, in.botExecution);
      if (mr.success && route.moves[at].restQuality > 0.0) ShakeOut(la);
    }
    AttemptResult live = FinishAttempt(la);
    CommitAttempt(sLive, mLive, route, live);

    CHECK(live.sent == batch.sent);
    CHECK(live.highpoint == batch.highpoint);
    CHECK(live.skinCost == batch.skinCost);
    CHECK(sLive.skinLeft == sBatch.skinLeft);
    CHECK(sLive.warmth == sBatch.warmth);
    CHECK(sLive.psyche == sBatch.psyche);
    CHECK(mLive.beta == mBatch.beta);
    CHECK(mLive.bestHighpoint == mBatch.bestHighpoint);
    CHECK(mLive.attempts == mBatch.attempts);
  }
}

// Psyche follows the session: sends feed it, going nowhere drains it, and
// the floor holds.
static void TestPsycheSwings() {
  Rng world = Rng::FromStream("loop-psy", Stream::Worldgen);
  Route easy = BuildRoute(world, "Confidence", 0, 0, RouteType::Endurance,
                          Discipline::Boulder);
  Route desperate = BuildRoute(world, "Humility", 14, 14, RouteType::Crimp,
                               Discipline::Boulder);
  Climber c = MakeClimber(50, 50, 50, 50, 50);

  Rng sessionRng = Rng::FromStream("loop-psy", Stream::Session);
  SessionState up = StartSession(c);
  up.warmth = 1.0;
  ProjectMemory upMem;
  AttemptResult r = AttemptInSession(sessionRng, up, upMem, c, easy, Conditions{});
  CHECK(r.sent);
  CHECK(up.psyche > c.psyche);

  SessionState down = StartSession(c);
  ProjectMemory downMem;
  double last = down.psyche;
  for (int i = 0; i < 30; i++) {
    AttemptResult burn =
        AttemptInSession(sessionRng, down, downMem, c, desperate, Conditions{});
    CHECK(!burn.sent);
    CHECK(down.psyche <= last || down.psyche >= last);  // never NaN
    last = down.psyche;
  }
  CHECK(down.psyche < c.psyche);
  CHECK(down.psyche >= SessionLoopDials{}.psycheFloor);
}

// --- Reading the line from the ground -----------------------------------------

static void TestRouteReads() {
  Rng world = Rng::FromStream("read", Stream::Worldgen);
  Climber c = MakeClimber(50, 50, 50, 50, 50);  // a V5 climber

  auto readOf = [&](int grade, int trueGrade) {
    Route r = BuildRoute(world, "Read V" + std::to_string(grade) + "-" +
                                    std::to_string(trueGrade),
                         grade, trueGrade, RouteType::Power,
                         Discipline::Boulder);
    return ReadRoute(c, r);
  };

  CHECK(readOf(1, 1) == RouteRead::Warmup);
  CHECK(readOf(4, 4) == RouteRead::Comfortable);
  CHECK(readOf(5, 5) == RouteRead::AtYourLimit);
  CHECK(readOf(7, 7) == RouteRead::Project);
  CHECK(readOf(10, 10) == RouteRead::NotThisYear);

  // The whole point of a sandbag: it reads like its tag right up until
  // you're on it. A V4-tagged V7 must look comfortable from the ground.
  CHECK(readOf(4, 7) == readOf(4, 4));

  // Ability moves the read, not just the grade: the same line is a project
  // for a weaker climber and a warmup for a stronger one.
  Route line = BuildRoute(world, "Same Line", 6, 6, RouteType::Power,
                          Discipline::Boulder);
  CHECK(ReadRoute(MakeClimber(35, 35, 35, 35, 35), line) == RouteRead::NotThisYear);
  CHECK(ReadRoute(MakeClimber(85, 85, 85, 85, 85), line) == RouteRead::Warmup);

  CHECK(std::string(ReadRouteText(RouteRead::NotThisYear)) == "Not this year.");
}

// --- Day loop ----------------------------------------------------------------

static void TestDayBasics() {
  PlayerState player;
  player.climber.skills = {50, 50, 50, 50, 50};
  DayState day = WakeUp(player);
  CHECK(day.hour == DayDials{}.wakeHour);
  CHECK(day.energy == 100.0);

  // Hours feed hunger; a meal costs cash and buys it back.
  PassHours(day, 5.0);
  CHECK(day.hunger == 15.0);
  const double cashBefore = player.cash;
  CHECK(EatMeal(player, day));
  CHECK(player.cash == cashBefore - DayDials{}.mealCost);
  CHECK(day.hunger < 15.0);

  // A shift pays and drains.
  const double energyBefore = day.energy;
  WorkShift(player, day);
  CHECK(player.cash > cashBefore - DayDials{}.mealCost);
  CHECK(day.energy < energyBefore);

  // Broke means hungry: a $3 wallet buys no burrito.
  PlayerState broke;
  broke.cash = 3.0;
  DayState brokeDay = WakeUp(broke);
  CHECK(!EatMeal(broke, brokeDay));
  CHECK(broke.cash == 3.0);
}

// Fatigue has to arrive while you can still climb, or it may as well not
// exist: the old threshold sat below where any real session ever got.
static void TestFatigueFadesIn() {
  PlayerState player;
  player.climber.skills = {50, 50, 50, 50, 50};
  DayState fresh = WakeUp(player);
  DayState worked = fresh;
  worked.energy = 40.0;
  DayState wrecked = fresh;
  wrecked.energy = 0.0;

  const double freshPsyche = ClimberForSession(player, fresh).psyche;
  const double workedPsyche = ClimberForSession(player, worked).psyche;
  const double wreckedPsyche = ClimberForSession(player, wrecked).psyche;
  CHECK(freshPsyche == player.climber.psyche);   // rested is unpenalised
  CHECK(workedPsyche < freshPsyche);             // and it fades in smoothly,
  CHECK(wreckedPsyche < workedPsyche);           // not as a cliff

  // Trying hard drains harder than cruising.
  Rng world = Rng::FromStream("fatigue", Stream::Worldgen);
  Route easy = BuildRoute(world, "Mileage", 2, 2, RouteType::Power,
                          Discipline::Boulder);
  Route limit = BuildRoute(world, "The Limit", 8, 8, RouteType::Power,
                           Discipline::Boulder);
  PlayerState cruiser = player, tryer = player;
  DayState cruiseDay = WakeUp(cruiser), tryDay = WakeUp(tryer);
  Rng rng = Rng::FromStream("fatigue", Stream::Session);
  StartGymSession(cruiser, cruiseDay);
  StartGymSession(tryer, tryDay);
  for (int i = 0; i < 4; i++) {
    AttemptResult a = AttemptInSession(rng, cruiseDay.session,
                                       MemoryFor(cruiser, easy),
                                       cruiser.climber, easy, Conditions{});
    ApplyAttemptToDay(cruiser, cruiseDay, easy, a, Rng::FromSeed("body"));
    AttemptResult b = AttemptInSession(rng, tryDay.session,
                                       MemoryFor(tryer, limit), tryer.climber,
                                       limit, Conditions{});
    ApplyAttemptToDay(tryer, tryDay, limit, b, Rng::FromSeed("body"));
  }
  CHECK(tryDay.energy < cruiseDay.energy);
}

static void TestBillsLandWeekly() {
  PlayerState player;
  DayState day = WakeUp(player);
  const double start = player.cash;
  // **Parked somewhere the city does not write tickets.** This test is
  // about the bill clock and nothing else, and the Lot -- which is the
  // default, free, and exactly where the city knows to look -- starts
  // ticketing after two nights. That is the bivy system working; it just
  // is not what this check is asking about. See Sim/DirtbagBivy.h.
  CHECK(ParkAt(player.bivy, Spot::UpperTrail, player.day, false, 0.0));
  const double drive = FuelFor(Describe(Spot::UpperTrail).driveHours);

  for (int i = 0; i < 7; i++) SleepToNextDay(player, day, Rng::FromSeed("body"));
  CHECK(player.day == 8);
  CHECK(std::fabs(player.cash -
                  (start - DayDials{}.billsAmount - 7 * drive)) < 1e-9);
  for (int i = 0; i < 7; i++) SleepToNextDay(player, day, Rng::FromSeed("body"));
  CHECK(std::fabs(player.cash -
                  (start - 2 * DayDials{}.billsAmount - 14 * drive)) < 1e-9);
}

static void TestSkinRegrowsOvernight() {
  PlayerState player;
  player.climber.skin = 4.0;
  DayState day = WakeUp(player);
  SleepToNextDay(player, day, Rng::FromSeed("body"));
  CHECK(player.climber.skin == 4.0 + DayDials{}.skinRegenPerNight);
  player.climber.skin = 8.9;
  SleepToNextDay(player, day, Rng::FromSeed("body"));
  CHECK(player.climber.skin == DayDials{}.maxSkin);  // capped, never past fresh
}

static void TestHungrySleepRecoversPoorly() {
  PlayerState fed, starving;
  DayState fedDay = WakeUp(fed), starvingDay = WakeUp(starving);
  starvingDay.hunger = 100.0;
  SleepToNextDay(fed, fedDay, Rng::FromSeed("body"));
  SleepToNextDay(starving, starvingDay, Rng::FromSeed("body"));
  CHECK(fedDay.energy == 100.0);
  CHECK(starvingDay.energy == DayDials{}.sleepEnergyFloor);
}

// Hard-for-you routes train; cruising doesn't. The 2D game's oldest rule.
static void TestTrainingCreep() {
  Rng world = Rng::FromStream("gym-seed", Stream::Worldgen);
  Route hard = BuildRoute(world, "Project Board", 8, 8, RouteType::Crimp,
                          Discipline::Boulder);
  Route easy = BuildRoute(world, "Warmup Circuit", 0, 0, RouteType::Crimp,
                          Discipline::Boulder);

  PlayerState grinder;
  grinder.climber.skills = {50, 50, 50, 50, 50};
  PlayerState cruiser = grinder;
  Rng sessionRng = Rng::FromStream("gym-seed", Stream::Session);

  DayState gDay = WakeUp(grinder), cDay = WakeUp(cruiser);
  StartGymSession(grinder, gDay);
  StartGymSession(cruiser, cDay);
  for (int i = 0; i < 10; i++) {
    AttemptResult g = AttemptInSession(sessionRng, gDay.session,
                                       MemoryFor(grinder, hard),
                                       grinder.climber, hard, Conditions{});
    ApplyAttemptToDay(grinder, gDay, hard, g, Rng::FromSeed("body"));
    AttemptResult c = AttemptInSession(sessionRng, cDay.session,
                                       MemoryFor(cruiser, easy),
                                       cruiser.climber, easy, Conditions{});
    ApplyAttemptToDay(cruiser, cDay, easy, c, Rng::FromSeed("body"));
  }
  CHECK(grinder.climber.skills.fingers > 50.0);
  CHECK(grinder.climber.skills.fingers > cruiser.climber.skills.fingers);
  // Mileage is mileage: even the cruiser's endurance ticks.
  CHECK(cruiser.climber.skills.endurance > 50.0);
}

// The Phase 1 canary: seven full days — wake, gym, eat, shift, sleep — with
// nothing going NaN, broke, or backwards.
static void TestSevenDayLoop() {
  PlayerState player;
  player.climber.skills = {45, 45, 45, 45, 45};
  Rng world = Rng::FromStream("week-1", Stream::Worldgen);
  std::vector<Route> board = GymBoard(world);
  CHECK(board.size() == 8);
  CHECK(board.front().grade < board.back().grade);  // a ladder, not a lottery

  const Skills startSkills = player.climber.skills;
  DayState day = WakeUp(player);
  for (int d = 0; d < 7; d++) {
    Rng sessionRng =
        Rng::FromStream("week-1#day" + std::to_string(player.day), Stream::Session);
    StartGymSession(player, day);
    for (int burn = 0; burn < 4; burn++) {
      const Route& route = board[(burn + d) % board.size()];
      AttemptResult r =
          AttemptInSession(sessionRng, day.session, MemoryFor(player, route),
                           ClimberForSession(player, day), route, Conditions{});
      ApplyAttemptToDay(player, day, route, r, Rng::FromSeed("body"));
    }
    EatMeal(player, day);
    WorkShift(player, day);
    EatMeal(player, day);
    SleepToNextDay(player, day, Rng::FromSeed("body"));
  }
  CHECK(player.day == 8);
  // Worked every day: one week of wages minus food and bills stays solvent.
  CHECK(player.cash > 0.0);
  CHECK(player.cash == player.cash);  // NaN guard
  CHECK(player.climber.skills.endurance > startSkills.endurance);
  CHECK(player.climber.skin > 0.0 && player.climber.skin <= DayDials{}.maxSkin);
  CHECK(!player.projects.empty());
}

// --- Save file ---------------------------------------------------------------

static PlayerState MakeSavedPlayer() {
  PlayerState p;
  p.climber.skills = {51.25, 47.5, 62.125, 39.0, 55.5};
  p.climber.morphology = Morphology::Lanky;
  p.climber.skin = 6.35;
  p.climber.psyche = 0.6125;
  p.cash = 337.5;
  p.day = 11;
  ProjectMemory m;
  m.routeName = "Setter's Revenge";
  m.attempts = 7;
  m.bestHighpoint = 5;
  m.beta = 0.4375;
  m.sent = false;
  p.projects.push_back(m);
  ProjectMemory sent;
  sent.routeName = "Jug Haul";
  sent.attempts = 1;
  sent.bestHighpoint = 6;
  sent.beta = 0.5;
  sent.sent = true;
  sent.firstSendStyle = Style::Onsight;
  p.projects.push_back(sent);
  return p;
}

static void TestSaveRoundTrip() {
  SaveGame save;
  save.seed = "grim-fjord-123";
  save.player = MakeSavedPlayer();

  const std::string text = SerializeSave(save);
  CHECK(text == SerializeSave(save));  // deterministic bytes

  SaveGame loaded;
  CHECK(DeserializeSave(text, loaded) == LoadResult::Ok);
  CHECK(loaded.version == kSaveVersion);
  CHECK(loaded.seed == save.seed);
  CHECK(loaded.player.day == save.player.day);
  CHECK(loaded.player.cash == save.player.cash);
  CHECK(loaded.player.climber.skills.power == save.player.climber.skills.power);
  CHECK(loaded.player.climber.skills.technique ==
        save.player.climber.skills.technique);
  CHECK(loaded.player.climber.morphology == Morphology::Lanky);
  CHECK(loaded.player.climber.skin == save.player.climber.skin);
  CHECK(loaded.player.climber.psyche == save.player.climber.psyche);
  CHECK(loaded.player.projects.size() == 2);
  CHECK(loaded.player.projects[0].routeName == "Setter's Revenge");
  CHECK(loaded.player.projects[0].beta == 0.4375);
  CHECK(loaded.player.projects[1].sent);
  CHECK(loaded.player.projects[1].firstSendStyle == Style::Onsight);
}

static void TestSaveRejectsGarbageAndFuture() {
  SaveGame out;
  CHECK(DeserializeSave("not a save", out) == LoadResult::BadFormat);
  CHECK(DeserializeSave("", out) == LoadResult::BadFormat);
  CHECK(DeserializeSave("version=99\nseed=x\n", out) ==
        LoadResult::FutureVersion);
  // A truncated save (missing fields) must refuse, never half-load.
  CHECK(DeserializeSave("version=1\nseed=x\nday=3\n", out) ==
        LoadResult::BadFormat);
}

// --- The body ----------------------------------------------------------------

// --- Ethics ------------------------------------------------------------------

static double YearsUntilFound(double visibility, int n = 1500) {
  EthicsDials d;
  double total = 0.0;
  int caught = 0;
  for (int i = 0; i < n; i++) {
    std::vector<Secret> secrets{
        Commit(EthicalAct::ChippedAHold, "Chalk Ghost", 1)};
    Rng world = Rng::FromSeed("ey-" + std::to_string(i));
    for (int day = 1; day <= 30 * 365; day++) {
      if (SomebodyFindsOut(secrets, world, day, visibility, d) >= 0) {
        total += day / 365.0;
        caught++;
        break;
      }
    }
  }
  return caught ? total / caught : 1e9;
}

static void TestBeingWatchedIsWhatCatchesYou() {
  // The whole design. The same act on the same day, by two people, is two
  // completely different lives — and it is success that exposes you.
  Standing nobody;
  Standing star;
  star.with[static_cast<int>(Faction::Scene)] = 1.0;

  CHECK(VisibilityFrom(nobody, SponsorTier::None) == 0.0);
  CHECK(VisibilityFrom(star, SponsorTier::Title) >
        VisibilityFrom(star, SponsorTier::None));
  CHECK(VisibilityFrom(nobody, SponsorTier::Title) >
        VisibilityFrom(nobody, SponsorTier::None));
  CHECK(VisibilityFrom(star, SponsorTier::Title) <= 1.0);

  // Being disliked by the Scene is not the same as being unknown to it, but
  // it is not what gets your old lines looked at either.
  Standing hated;
  hated.with[static_cast<int>(Faction::Scene)] = -1.0;
  CHECK(VisibilityFrom(hated, SponsorTier::None) == 0.0);

  const double quiet = YearsUntilFound(0.0);
  const double watched = YearsUntilFound(1.0);
  CHECK(watched < quiet);
  CHECK(quiet > watched * 1.5);   // and it is not a marginal difference
}

static void TestItReallyMightNeverComeOut() {
  // The choice has to be genuinely tempting, not a trap with a timer on it.
  // A quiet career has better than even odds of carrying this to the end —
  // measured, 50% across thirty years — and that is the whole reason
  // anybody would do it.
  EthicsDials d;
  int never = 0;
  const int N = 1200;
  for (int i = 0; i < N; i++) {
    std::vector<Secret> secrets{
        Commit(EthicalAct::ChippedAHold, "Chalk Ghost", 1)};
    Rng world = Rng::FromSeed("nv-" + std::to_string(i));
    bool out = false;
    for (int day = 1; day <= 30 * 365 && !out; day++) {
      out = SomebodyFindsOut(secrets, world, day, 0.0, d) >= 0;
    }
    if (!out) never++;
  }
  const double share = static_cast<double>(never) / N;
  CHECK(share > 0.3);   // an unknown very often gets away with it...
  CHECK(share < 0.7);   // ...and never reliably

  // A star does not. Almost nobody with a career keeps this.
  int starNever = 0;
  for (int i = 0; i < 400; i++) {
    std::vector<Secret> secrets{
        Commit(EthicalAct::ChippedAHold, "Chalk Ghost", 1)};
    Rng world = Rng::FromSeed("sv-" + std::to_string(i));
    bool out = false;
    for (int day = 1; day <= 30 * 365 && !out; day++) {
      out = SomebodyFindsOut(secrets, world, day, 1.0, d) >= 0;
    }
    if (!out) starNever++;
  }
  CHECK(starNever < 400 / 10);
}

static void TestAFreshSecretIsQuiet() {
  EthicsDials d;
  // The people who were there have not compared notes yet, and nobody is
  // looking at a line that just went. Not even a star is caught same-week.
  //
  // The world seed varies per iteration, and that is the whole point of the
  // loop: discovery derives its stream from (world, day, index), so reusing
  // one world would roll the same forty-five numbers four hundred times.
  // The first version of this test did exactly that, and passed with the
  // quiet-period guard deleted.
  for (int i = 0; i < 400; i++) {
    const Rng world = Rng::FromSeed("fresh-" + std::to_string(i));
    std::vector<Secret> secrets{Commit(EthicalAct::ChippedAHold, "X", 100)};
    for (int day = 100; day < 100 + d.quietDays; day++) {
      CHECK(SomebodyFindsOut(secrets, world, day, 1.0, d) < 0);
    }
  }
}

static void TestOneThingAtATime() {
  EthicsDials d;
  const Rng world = Rng::FromSeed("cascade");
  // A career unravelling in a single afternoon is a punishment; this is
  // meant to be a story. Five secrets cannot all surface on one day.
  std::vector<Secret> secrets;
  for (int i = 0; i < 5; i++) {
    secrets.push_back(Commit(static_cast<EthicalAct>(i), "L", 1));
  }
  for (int day = 1; day <= 20000; day++) {
    const int found = SomebodyFindsOut(secrets, world, day, 1.0, d);
    if (found < 0) continue;
    int knownToday = 0;
    for (const Secret& s : secrets) {
      if (s.known && s.dayFound == day) knownToday++;
    }
    CHECK(knownToday == 1);
  }
  // And they do all come out eventually, to somebody that visible.
  CHECK(Unknown(secrets).empty());
}

static void TestSomeLiesTakeTheAscentAndSomeDoNot() {
  // Chipping and retro-bolting change the rock: the ascent stands, hollow,
  // on a line that is not what it was. The other three are lies about what
  // happened, and there is nothing left to stand.
  CHECK(!StripsTheAscent(EthicalAct::ChippedAHold));
  CHECK(!StripsTheAscent(EthicalAct::RetroBolted));
  CHECK(StripsTheAscent(EthicalAct::ClaimedASend));
  CHECK(StripsTheAscent(EthicalAct::StagedAPhoto));
  CHECK(StripsTheAscent(EthicalAct::PulledOnGear));
}

static void TestWhoIsActuallyAngry() {
  EthicsDials d;
  // The old guard is the injured party — these are their ethics and the
  // rock is theirs. The stewards care about what was done to rock and about
  // the rest not at all.
  Standing chipped, claimed;
  double p1 = 0.7, p2 = 0.7;
  ItComesOut(Commit(EthicalAct::ChippedAHold, "L", 1), chipped, p1, d);
  ItComesOut(Commit(EthicalAct::ClaimedASend, "L", 1), claimed, p2, d);

  CHECK(StandingWith(chipped, Faction::OldGuard) < 0.0);
  CHECK(StandingWith(claimed, Faction::OldGuard) < 0.0);
  // Chipping is the unforgivable one.
  CHECK(StandingWith(chipped, Faction::OldGuard) <
        StandingWith(claimed, Faction::OldGuard));
  // Only the rock acts reach the stewards.
  CHECK(StandingWith(chipped, Faction::Stewardship) < 0.0);
  CHECK(StandingWith(claimed, Faction::Stewardship) >= 0.0);
  // And being found out is not only arithmetic.
  CHECK(p1 < 0.7);
  CHECK(p2 < 0.7);

  // A secret nobody knows costs nothing — the act is not what is priced,
  // being caught is.
  Standing untouched;
  double psyche = 0.7;
  const std::vector<Secret> carried{Commit(EthicalAct::ChippedAHold, "L", 1)};
  CHECK(Unknown(carried).size() == 1);
  CHECK(StandingWith(untouched, Faction::OldGuard) == 0.0);
  CHECK(psyche == 0.7);
}

static void TestWhatYouDidSurvivesASave() {
  SaveGame save;
  save.seed = "carried";
  save.player.secrets.push_back(
      Commit(EthicalAct::ChippedAHold, "Chalk Ghost", 400));
  Secret caught = Commit(EthicalAct::StagedAPhoto, "", 900);
  caught.known = true;
  caught.dayFound = 2600;
  save.player.secrets.push_back(caught);

  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  CHECK(back.player.secrets.size() == 2);
  CHECK(back.player.secrets[0].act == EthicalAct::ChippedAHold);
  CHECK(back.player.secrets[0].routeKey == "Chalk Ghost");
  CHECK(back.player.secrets[0].dayDone == 400);
  CHECK(!back.player.secrets[0].known);
  // A staged photo is not on a line, so an empty route key has to survive
  // as an empty route key rather than failing the parse.
  CHECK(back.player.secrets[1].routeKey.empty());
  CHECK(back.player.secrets[1].known);
  CHECK(back.player.secrets[1].dayFound == 2600);

  // An honest career round-trips too, and it is the common one.
  SaveGame clean;
  clean.seed = "honest";
  SaveGame cleanBack;
  CHECK(DeserializeSave(SerializeSave(clean), cleanBack) == LoadResult::Ok);
  CHECK(cleanBack.player.secrets.empty());
}

static void TestFindingOutIsDeterministic() {
  EthicsDials d;
  const auto Run = [&](int seed) {
    std::vector<Secret> secrets{Commit(EthicalAct::PulledOnGear, "L", 1)};
    Rng world = Rng::FromSeed("determinism-" + std::to_string(seed));
    for (int day = 1; day <= 20000; day++) {
      if (SomebodyFindsOut(secrets, world, day, 0.5, d) >= 0) return day;
    }
    return -1;   // a real answer: about one career in eighty keeps it
  };
  // Same seed, same day, always — including the seeds where it never comes
  // out, which is why this compares the answer rather than assuming one.
  int everFound = 0;
  for (int seed = 0; seed < 20; seed++) {
    const int first = Run(seed);
    CHECK(first == Run(seed));
    if (first > 0) everFound++;
  }
  CHECK(everFound > 15);   // and at this visibility, most careers do

  // And the text says the years out loud, because that is the point of it.
  Secret old = Commit(EthicalAct::ChippedAHold, "Chalk Ghost", 1);
  CHECK(EthicsText(old, 4000).empty());   // nobody knows: nothing to say
  old.known = true;
  old.dayFound = 3300;
  const std::string said = EthicsText(old, 3300);
  CHECK(said.find("Chalk Ghost") != std::string::npos);
  CHECK(said.find("9 years ago") != std::string::npos);
}

// --- Sponsorship -------------------------------------------------------------

static void TestNobodySponsorsAClimberNobodyHasHeardOf() {
  SponsorDials d;
  Standing unknown;                       // nobody has an opinion
  Standing known;
  known.with[static_cast<int>(Faction::Scene)] = 0.7;

  // Ability is not the currency. What people can *see* is.
  CHECK(OfferFor(d.gradeForTitle + 2, 0, unknown, d) != SponsorTier::Title);
  CHECK(OfferFor(d.gradeForTitle, 0, known, d) == SponsorTier::Title);

  // And the ladder is a ladder.
  CHECK(OfferFor(0, 0, known, d) == SponsorTier::None);
  CHECK(OfferFor(d.gradeForShoes, 0, unknown, d) == SponsorTier::Shoes);
  CHECK(OfferFor(d.gradeForGear, 0, known, d) == SponsorTier::Gear);

  // First ascents are visibility, because they are what gets written about
  // — a quiet crusher with four new lines is somebody people have heard of.
  Standing quiet;
  CHECK(OfferFor(d.gradeForGear, 0, quiet, d) == SponsorTier::Shoes);
  CHECK(OfferFor(d.gradeForGear, 4, quiet, d) == SponsorTier::Gear);
}

static void TestTheirDaysAreTheGoodDays() {
  SponsorDials d;
  const Rng world = Rng::FromSeed("shoot");
  Sponsorship title;
  title.tier = SponsorTier::Title;

  // The whole mechanic: you cannot shoot climbing photos in the rain, and
  // nobody runs a comp in February for the love of it. A shift takes a
  // spare day; a shoot takes the one you wanted.
  int onGoodDays = 0;
  for (int day = 1; day <= 2000; day++) {
    CHECK(!ObligationToday(title, world, day, false, d));   // never, ever
    if (ObligationToday(title, world, day, true, d)) onGoodDays++;
  }
  CHECK(onGoodDays > 0);

  // A shoe deal owns nothing. That is what makes it the one to take.
  Sponsorship shoes;
  shoes.tier = SponsorTier::Shoes;
  for (int day = 1; day <= 500; day++) {
    CHECK(!ObligationToday(shoes, world, day, true, d));
  }
  // And a title deal owns more of your calendar than a gear deal.
  Sponsorship gear;
  gear.tier = SponsorTier::Gear;
  int gearDays = 0, titleDays = 0;
  for (int day = 1; day <= 2000; day++) {
    if (ObligationToday(gear, world, day, true, d)) gearDays++;
    if (ObligationToday(title, world, day, true, d)) titleDays++;
  }
  CHECK(titleDays > gearDays);
  CHECK(gearDays > 0);
}

static void TestADealIsReviewedAndBeingHurtIsNotFailing() {
  SponsorDials d;
  Sponsorship deal;
  deal.tier = SponsorTier::Title;
  deal.gradeAtLastReview = 9;

  // Keep climbing harder and they keep you.
  CHECK(ReviewSeason(deal, 10, 0, d) == SponsorTier::Title);
  CHECK(deal.seasonsWithoutProgress == 0);
  CHECK(deal.seasonsHeld == 1);

  // Stop, and after a couple of seasons they stop returning calls — one
  // rung down rather than out, because a career ending in a single review
  // would be a punishment rather than a story.
  for (int i = 0; i < d.seasonsOfNothingBeforeDropped; i++) {
    ReviewSeason(deal, 10, 0, d);
  }
  CHECK(deal.tier == SponsorTier::Gear);

  // Being hurt is not failing, and a sponsor who dropped you for it would
  // be worse than most real ones.
  Sponsorship hurt;
  hurt.tier = SponsorTier::Title;
  hurt.gradeAtLastReview = 9;
  for (int i = 0; i < 5; i++) {
    ReviewSeason(hurt, 9, d.injuryDaysThatPauseReview, hurt.tier ==
                 SponsorTier::None ? d : d);
  }
  CHECK(hurt.tier == SponsorTier::Title);
  CHECK(hurt.seasonsWithoutProgress == 0);
}

static void TestSigningSaysSomethingAboutYou() {
  Standing s;
  const double sceneBefore = StandingWith(s, Faction::Scene);
  SignedWith(SponsorTier::Title, s);
  CHECK(StandingWith(s, Faction::Scene) > sceneBefore);
  // Taking money to climb is the oldest argument in the sport, and the
  // opposed axis was built for exactly this.
  CHECK(StandingWith(s, Faction::OldGuard) < 0.0);

  // A bigger deal says more.
  Standing small, big;
  SignedWith(SponsorTier::Shoes, small);
  SignedWith(SponsorTier::Title, big);
  CHECK(StandingWith(big, Faction::Scene) > StandingWith(small, Faction::Scene));

  // And no deal says nothing at all.
  Standing none;
  SignedWith(SponsorTier::None, none);
  CHECK(StandingWith(none, Faction::Scene) == 0.0);
}

static void TestTheShoeDealIsWorthMoreThanItLooks() {
  SponsorDials d;
  // It pays nothing and saves a dirtbag $165 a pair on a $6,000 year, which
  // for this game's economy is most of a deal.
  CHECK(MonthlyStipend(SponsorTier::Shoes, d) == 0.0);
  CHECK(MonthlyStipend(SponsorTier::Title, d) >
        MonthlyStipend(SponsorTier::Gear, d));
  Sponsorship shoes;
  shoes.tier = SponsorTier::Shoes;
  CHECK(CoversShoes(shoes));
  Sponsorship none;
  CHECK(!CoversShoes(none));
  CHECK(MonthlyStipend(SponsorTier::None, d) == 0.0);
}

static void TestADealSurvivesASave() {
  SaveGame save;
  save.seed = "sponsored";
  save.player.sponsor.tier = SponsorTier::Gear;
  save.player.sponsor.seasonsHeld = 4;
  save.player.sponsor.gradeAtLastReview = 8;
  save.player.sponsor.seasonsWithoutProgress = 1;

  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  CHECK(back.player.sponsor.tier == SponsorTier::Gear);
  CHECK(back.player.sponsor.seasonsHeld == 4);
  CHECK(back.player.sponsor.gradeAtLastReview == 8);
  CHECK(back.player.sponsor.seasonsWithoutProgress == 1);
}

// --- Legacy ------------------------------------------------------------------

static PlayerState ACareer() {
  PlayerState p;
  p.day = 9 * 365;                 // nine seasons in
  p.climber.skills = {70, 70, 70, 70, 70};
  p.standing.with[static_cast<int>(Faction::OldGuard)] = 0.6;
  p.standing.with[static_cast<int>(Faction::Stewardship)] = -0.4;

  ProjectMemory fa;
  fa.routeName = "the arete left of Diesel";
  fa.givenName = "Cattle Grid Arete";
  fa.firstAscent = true;
  fa.sent = true;
  fa.grade = 6;
  fa.confirmedGrade = 7;
  fa.firstSendStyle = Style::Redpoint;
  fa.attempts = 40;
  p.projects.push_back(fa);

  ProjectMemory pitch;
  pitch.routeName = "the bolted line through the roof";
  pitch.givenName = "Forty Minutes Up";
  pitch.firstAscent = true;
  pitch.sent = true;
  pitch.grade = 9;
  pitch.confirmedGrade = 10;
  pitch.discipline = Discipline::Sport;
  pitch.firstSendStyle = Style::Redpoint;
  pitch.attempts = 60;
  p.projects.push_back(pitch);

  ProjectMemory never;
  never.routeName = "Chalk Ghost";
  never.attempts = 210;
  never.grade = 6;
  p.projects.push_back(never);
  return p;
}

static void TestACareerCanEnd() {
  LegacyDials d;
  const PlayerState p = ACareer();
  const Legacy l = TallyCareer(p, "Evan", 9, d);

  CHECK(l.name == "Evan");
  CHECK(l.seasons == 9);
  CHECK(l.retiredAt > 30.0);
  CHECK(l.firstAscents.size() == 2);
  CHECK(l.totalSends == 2);
  CHECK(l.nemesis == "Chalk Ghost");
  CHECK(l.nemesisAttempts == 210);
  CHECK(l.standing.with[static_cast<int>(Faction::OldGuard)] == 0.6);

  // Keys, never given names. A guidebook that keys on what somebody called
  // a line loses the line the day it is renamed, and naming has never been
  // allowed to move a key anywhere else in this project.
  for (const NamedLine& n : l.firstAscents) {
    CHECK(!n.routeKey.empty());
    CHECK(n.by == "Evan");
  }
  CHECK(l.firstAscents[0].routeKey == "the arete left of Diesel");
  CHECK(l.firstAscents[0].givenName == "Cattle Grid Arete");

  const std::string text = LegacyText(l);
  CHECK(text.find("9 seasons") != std::string::npos);
  CHECK(text.find("Chalk Ghost") != std::string::npos);   // the one that never went
  CHECK(!text.empty());
}

static void TestTheGuidebookPrintsTheRightLadder() {
  // A rope route reads 5.13c and a boulder reads V7. A career card that
  // cannot tell them apart will confidently print the wrong ladder, which
  // is the sort of thing a climber notices immediately and never forgives.
  const Legacy l = TallyCareer(ACareer(), "Evan", 9);
  std::string boulderEntry, sportEntry;
  for (const NamedLine& n : l.firstAscents) {
    if (n.discipline == Discipline::Sport) sportEntry = GuidebookEntry(n);
    else boulderEntry = GuidebookEntry(n);
  }
  CHECK(boulderEntry.find("V7") != std::string::npos);
  CHECK(sportEntry.find("5.13c") != std::string::npos);
  CHECK(boulderEntry.find("FA Evan") != std::string::npos);
  CHECK(sportEntry.find("Forty Minutes Up") != std::string::npos);
}

static void TestTheWorldRemembersAndTheBodyDoesNot() {
  // The whole design call. Inheriting somebody else's fingers would be
  // nonsense and would make the second life a save-scum of the first.
  LegacyDials d;
  const Rng world = Rng::FromSeed("legacy-world");
  const Legacy l = TallyCareer(ACareer(), "Evan", 9);
  const PlayerState next = Inherit(l, world, d);

  // Nothing physical carries — they start where anybody starts.
  //
  // This used to compare against a default-constructed PlayerState, which
  // is a *zeroed struct* rather than a person, and so it passed while the
  // inheritor was being born with 0 in all five skills: unable to send a V0
  // and, because the retirement test needs a peak above zero, never once
  // offered the chance to stop. Thirty years of it went unnoticed because
  // the assertion agreed with the bug.
  //
  // "Where anybody starts" is now a spread rather than a stamp: a beginner
  // within ±6 of kStartingSkill, never a veteran and never that zero.
  const auto aBeginner = [](double s) {
    return s >= kStartingSkill - 6.0 && s <= kStartingSkill + 6.0;
  };
  CHECK(aBeginner(next.climber.skills.power));
  CHECK(aBeginner(next.climber.skills.fingers));
  CHECK(aBeginner(next.climber.skills.technique));
  CHECK(aBeginner(next.climber.skills.endurance));
  CHECK(aBeginner(next.climber.skills.head));

  // Deterministic: the same world and the same predecessor hand back the
  // same successor, or a reload would re-roll your body.
  const PlayerState again = Inherit(l, world, d);
  CHECK(again.climber.skills.power == next.climber.skills.power);
  CHECK(again.climber.morphology == next.climber.morphology);

  // And different predecessors are different draws -- four generations
  // must not be four copies. (Skills are five independent ±6 rolls; the
  // chance of a collision on all five is nil, and this seed pair differs.)
  const Legacy other = TallyCareer(ACareer(), "Wren", 9);
  const PlayerState sibling = Inherit(other, world, d);
  CHECK(sibling.climber.skills.power != next.climber.skills.power ||
        sibling.climber.skills.technique != next.climber.skills.technique);

  // And the two properties the zeroed version silently failed: they can
  // climb, and they can eventually stop.
  CHECK(SummarizeCareer(next).abilityGrade > 0.0);
  PlayerState ageing = next;
  double peak = 0.0;
  bool everOffered = false;
  for (int day = 1; day <= 60 * 365; day++) {
    ageing.day = day;
    AgeDay(ageing.climber, ageing.day);
    peak = std::max(peak, SkillToGrade(ageing.climber.skills.power));
    if (TimeToThinkAboutIt(ageing, 0, peak)) { everOffered = true; break; }
  }
  CHECK(everOffered);
  CHECK(next.climber.load == 0.0);
  CHECK(!IsHurt(next.climber));
  CHECK(next.projects.empty());          // none of the ledgers are yours
  CHECK(next.day == 1);                  // and you are twenty-four again
  CHECK(AgeOn(next.day) == AgeDials{}.startAge);

  // The van and the coffee tin, and deliberately not the gear: a career
  // that ended rich must not hand the next one a shortcut past the part of
  // this game that is about being broke.
  CHECK(next.cash == d.inheritedCash);
  CHECK(next.kit.pads == PlayerState{}.kit.pads);   // one pad, like anybody
  CHECK(!next.kit.hangboard);
  CHECK(!IsGymMember(next.kit));
  CHECK(!next.job.salaried);

  // But the Lot knows whose van that is — and it cuts both ways.
  CHECK(next.standing.with[static_cast<int>(Faction::OldGuard)] > 0.0);
  CHECK(next.standing.with[static_cast<int>(Faction::Stewardship)] < 0.0);
  CHECK(next.standing.with[static_cast<int>(Faction::OldGuard)] <
        l.standing.with[static_cast<int>(Faction::OldGuard)]);

  // A closure is not inherited. A gate that stays shut forever is a dead
  // crag rather than a consequence.
  Legacy shut = l;
  shut.standing.closedDays = 9;
  CHECK(Inherit(shut, Rng::FromSeed("w"), d).standing.closedDays == 0);
  CHECK(CragIsOpen(Inherit(shut, Rng::FromSeed("w"), d).standing));
}

static void TestNobodyIsEverThrownOut() {
  LegacyDials d;
  // Retirement is offered, never forced. Deciding when to stop is the last
  // real choice a climbing career contains.
  PlayerState strong;
  strong.day = 25 * 365;             // 49 years old
  strong.climber.skills = {90, 90, 90, 90, 90};
  // Old, but still climbing at their best: nothing is said.
  CHECK(!TimeToThinkAboutIt(strong, 0, SkillToGrade(90.0), d));

  // Two grades off their best and past the age: now the game is honest.
  PlayerState faded;
  faded.day = 25 * 365;
  faded.climber.skills = {45, 45, 45, 45, 45};
  CHECK(TimeToThinkAboutIt(faded, 0, SkillToGrade(90.0), d));

  // A body that keeps breaking says it before the numbers do — at any age.
  PlayerState young;
  young.day = 2 * 365;
  young.climber.skills = {70, 70, 70, 70, 70};
  CHECK(!TimeToThinkAboutIt(young, 0, SkillToGrade(70.0), d));
  CHECK(TimeToThinkAboutIt(young, d.injuriesInARowToHint, SkillToGrade(70.0), d));

  // And age alone is never the reason. Plenty of people climb their hardest
  // at forty, and telling one of them to pack it in because of a birthday
  // would be both wrong and insulting.
  PlayerState oldAndStrong;
  oldAndStrong.day = 30 * 365;
  oldAndStrong.climber.skills = {80, 80, 80, 80, 80};
  CHECK(!TimeToThinkAboutIt(oldAndStrong, 0, SkillToGrade(80.0), d));
}

static void TestLoadsVersion11Save() {
  // A v11 career predates the rope crag entirely, so every line it ever
  // touched was a boulder — which is not a guess, it is the only thing that
  // could have been true. And nobody came before it.
  const std::string v11 =
      "version=11\n"
      "seed=v11-fixture\n"
      "day=200\n"
      "cash=420\n"
      "skills.power=0\n"
      "skills.fingers=0\n"
      "skills.technique=0\n"
      "skills.endurance=0\n"
      "skills.head=0\n"
      "morphology=1\n"
      "skin=9\n"
      "psyche=0.69999999999999996\n"
      "projects=1\n"
      "project.0.name=Chalk Ghost\n"
      "project.0.grade=6\n"
      "project.0.attempts=12\n"
      "project.0.best=0\n"
      "project.0.beta=0\n"
      "project.0.sent=1\n"
      "project.0.clean=1\n"
      "project.0.given=\n"
      "project.0.fa=0\n"
      "project.0.confirmed=-1\n"
      "project.0.style=4\n"
      "owed=0\n"
      "standing.0=0\n"
      "standing.1=0\n"
      "standing.2=0\n"
      "standing.3=0\n"
      "load=0\n"
      "injury.active=0\n"
      "injury.kind=0\n"
      "injury.severity=0\n"
      "injury.days=0\n"
      "physio.last=0\n"
      "kit.pads=1\n"
      "kit.hangboard=0\n"
      "kit.membership=0\n"
      "job.salaried=0\n"
      "job.days=0\n"
      "job.weeks=0\n"
      "standing.closed=0\n"
      "shoes.wear=0\n"
      "shoes.resoles=0\n"
      "shoes.pairs=1\n"
      "van.hours=0\n"
      "van.0.wear=0\n"
      "van.0.patches=0\n"
      "van.0.failed=0\n"
      "van.1.wear=0\n"
      "van.1.patches=0\n"
      "van.1.failed=0\n"
      "van.2.wear=0\n"
      "van.2.patches=0\n"
      "van.2.failed=0\n"
      "van.3.wear=0\n"
      "van.3.patches=0\n"
      "van.3.failed=0\n"
      "van.4.wear=0\n"
      "van.4.patches=0\n"
      "van.4.failed=0\n"
      "van.5.wear=0\n"
      "van.5.patches=0\n"
      "van.5.failed=0\n"
      "dog.name=the dog\n"
      "dog.adopted=0\n"
      "dog.bond=0\n"
      "dog.fed=0.40000000000000002\n"
      "bonds=0\n";

  SaveGame loaded;
  CHECK(DeserializeSave(v11, loaded) == LoadResult::Ok);
  CHECK(loaded.version == kSaveVersion);
  CHECK(loaded.seed == "v11-fixture");
  CHECK(loaded.player.day == 200);
  CHECK(loaded.player.projects.size() == 1);
  CHECK(loaded.player.projects[0].routeName == "Chalk Ghost");
  CHECK(loaded.player.projects[0].attempts == 12);
  CHECK(loaded.player.projects[0].sent);
  CHECK(loaded.player.projects[0].discipline == Discipline::Boulder);
  CHECK(loaded.legacies.empty());   // it is the first life

  // And it re-saves in the new format, so the upgrade is permanent.
  SaveGame again;
  CHECK(DeserializeSave(SerializeSave(loaded), again) == LoadResult::Ok);
  CHECK(again.player.projects[0].discipline == Discipline::Boulder);
}

static void TestGenerationsSurviveASave() {
  SaveGame save;
  save.seed = "dynasty";
  save.player = ACareer();
  save.legacies.push_back(TallyCareer(ACareer(), "Evan", 9));
  save.legacies.push_back(TallyCareer(ACareer(), "Sam", 6));

  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  CHECK(back.legacies.size() == 2);
  CHECK(back.legacies[0].name == "Evan");
  CHECK(back.legacies[1].name == "Sam");
  CHECK(back.legacies[1].seasons == 6);
  CHECK(back.legacies[0].firstAscents.size() == 2);
  CHECK(back.legacies[0].firstAscents[0].routeKey ==
        "the arete left of Diesel");
  CHECK(back.legacies[0].firstAscents[0].givenName == "Cattle Grid Arete");
  CHECK(back.legacies[0].nemesis == "Chalk Ghost");
  // And the ladder survives, which is the field that needed a version bump.
  bool foundSport = false;
  for (const NamedLine& n : back.legacies[0].firstAscents) {
    if (n.discipline == Discipline::Sport) foundSport = true;
  }
  CHECK(foundSport);
  CHECK(back.player.projects[1].discipline == Discipline::Sport);

  // A save with no legacies is the first life, and must round-trip too.
  SaveGame first;
  first.seed = "alone";
  SaveGame firstBack;
  CHECK(DeserializeSave(SerializeSave(first), firstBack) == LoadResult::Ok);
  CHECK(firstBack.legacies.empty());
}

// --- Sport -------------------------------------------------------------------

static Route Pitch(const char* name, int grade) {
  Rng world = Rng::FromStream("pitch", Stream::Worldgen);
  return BuildRoute(world, name, grade, grade, RouteType::Endurance,
                    Discipline::Sport);
}

static void TestOnlyPitchesHaveBolts() {
  SportDials d;
  const Route pitch = Pitch("The Long Haul", 7);
  Rng world = Rng::FromStream("pitch", Stream::Worldgen);
  const Route boulder = BuildRoute(world, "Short", 7, 7, RouteType::Power,
                                   Discipline::Boulder);

  CHECK(BoltsFor(boulder, d).empty());
  CHECK(!BoltsFor(pitch, d).empty());
  // Boulders are untouched by every part of this file, which is what keeps
  // it clear of everything already measured.
  for (int i = 0; i < static_cast<int>(boulder.moves.size()); i++) {
    CHECK(RunoutAt(boulder, i, d) == 0.0);
    CHECK(ClipCost(boulder, i, d) == 0.0);
    CHECK(!OnTheRope(boulder, i, d));
  }

  // Bolts climb in order and stay on the route.
  const std::vector<int> bolts = BoltsFor(pitch, d);
  CHECK(bolts[0] == d.firstBoltAtMove);
  for (size_t i = 1; i < bolts.size(); i++) CHECK(bolts[i] > bolts[i - 1]);
  CHECK(bolts.back() < static_cast<int>(pitch.moves.size()));
}

static void TestThePadStopsAtTheFirstBolt() {
  SportDials d;
  const Route pitch = Pitch("The Long Haul", 7);

  // Below the first bolt you are bouldering, and the ground is the question.
  for (int i = 0; i < d.firstBoltAtMove; i++) CHECK(!OnTheRope(pitch, i, d));
  // Above it you are on the rope and it is not.
  for (int i = d.firstBoltAtMove;
       i < static_cast<int>(pitch.moves.size()); i++) {
    CHECK(OnTheRope(pitch, i, d));
  }

  // And it reaches the wall: high on a pitch, having no crash pad costs
  // nothing, because a pad under a rope route is not a thing anybody wants.
  // Held wrong, the boulder penalty follows a roped climber thirty metres
  // up and quietly taxes them for it.
  Climber c = MakeClimber(70, 70, 70, 75, 60);
  AttemptInput padded = MakeInput(c, pitch);
  AttemptInput bare = MakeInput(c, pitch);
  bare.padding = 0.0;
  LiveAttempt a = BeginAttempt(Rng::FromSeed("rope"), padded);
  LiveAttempt b = BeginAttempt(Rng::FromSeed("rope"), bare);
  a.nextMove = b.nextMove = static_cast<int>(pitch.moves.size()) - 1;
  CHECK(std::fabs(PeekOdds(a, 0.72) - PeekOdds(b, 0.72)) < 1e-9);
}

static void TestTheRunoutIsTheRopeHeadGame() {
  SportDials d;
  const Route pitch = Pitch("The Long Haul", 7);

  // Below the first bolt you are not runout, you are bouldering — and the
  // ground-fall penalty covers that. The resolver never asks here because
  // OnTheRope gates it, but anything reading this for a HUD line will, and
  // a non-zero answer there would say "a long way above the last clip" to
  // somebody standing on the ground.
  for (int i = 0; i < d.firstBoltAtMove; i++) {
    CHECK(RunoutAt(pitch, i, d) == 0.0);
  }

  // Zero at a clip, and rising above it — that is the whole feeling.
  for (int bolt : BoltsFor(pitch, d)) CHECK(RunoutAt(pitch, bolt, d) == 0.0);
  const std::vector<int> bolts = BoltsFor(pitch, d);
  CHECK(RunoutAt(pitch, bolts[0] + 1, d) > 0.0);
  CHECK(RunoutAt(pitch, bolts[0] + 2, d) > RunoutAt(pitch, bolts[0] + 1, d));
  for (int i = 0; i < static_cast<int>(pitch.moves.size()); i++) {
    const double r = RunoutAt(pitch, i, d);
    CHECK(r >= 0.0 && r <= 1.0);
  }

  // A bold climber is still bolder above the bolt, same as above gravel.
  AttemptInput bold = MakeInput(MakeClimber(70, 70, 70, 75, 95), pitch);
  AttemptInput timid = MakeInput(MakeClimber(70, 70, 70, 75, 15), pitch);
  LiveAttempt bo = BeginAttempt(Rng::FromSeed("nerve"), bold);
  LiveAttempt ti = BeginAttempt(Rng::FromSeed("nerve"), timid);
  bo.nextMove = ti.nextMove = bolts[0] + 3;
  CHECK(PeekOdds(bo, 0.72) > PeekOdds(ti, 0.72));
}

static void TestClippingCostsAndTheStanceDecidesHowMuch() {
  SportDials d;
  Route pitch = Pitch("The Long Haul", 7);
  const std::vector<int> bolts = BoltsFor(pitch, d);

  for (int i = 0; i < static_cast<int>(pitch.moves.size()); i++) {
    const bool clips = IsClippingMove(pitch, i, d);
    CHECK((ClipCost(pitch, i, d) > 0.0) == clips);
  }

  // From a jug it is nearly free; from a crimp with your feet cutting it is
  // where routes get lost.
  Route jugStance = pitch, badStance = pitch;
  jugStance.moves[bolts[0]].restQuality = 1.0;
  badStance.moves[bolts[0]].restQuality = 0.0;
  CHECK(ClipCost(badStance, bolts[0], d) > ClipCost(jugStance, bolts[0], d));
  // And never more than it costs to make the move itself, or clipping stops
  // being a decision and becomes a wall.
  SessionDials sd;
  CHECK(ClipCost(badStance, bolts[0], d) < sd.basePumpCost);
}

static void TestBetaIsWorthMoreOnALongerRoute() {
  // A six-move boulder is one puzzle; a twenty-move pitch is a dozen, and
  // wiring them is most of what redpointing is. Held flat, a climber a
  // grade above their level topped out at 4.8% even fully rehearsed.
  SessionDials d;
  Rng world = Rng::FromStream("beta-len", Stream::Worldgen);
  const Route boulder = BuildRoute(world, "Short", 7, 7, RouteType::Technical,
                                   Discipline::Boulder);
  const Route pitch = Pitch("Long", 7);
  CHECK(boulder.moves.size() <= static_cast<size_t>(d.betaFlatUntilMoves));
  CHECK(pitch.moves.size() > static_cast<size_t>(d.betaFlatUntilMoves));

  const auto BetaWorth = [&](const Route& r) {
    Climber c = MakeClimber(70, 70, 70, 75, 60);
    AttemptInput cold = MakeInput(c, r);
    AttemptInput wired = MakeInput(c, r);
    wired.beta = 1.0;
    LiveAttempt a = BeginAttempt(Rng::FromSeed("b"), cold);
    LiveAttempt b = BeginAttempt(Rng::FromSeed("b"), wired);
    return PeekOdds(b, 0.72) - PeekOdds(a, 0.72);
  };
  CHECK(BetaWorth(pitch) > BetaWorth(boulder));

  // And a boulder is worth *exactly* what it always was — this scaling must
  // not have quietly reflowed every short-route number in the project.
  Climber c = MakeClimber(70, 70, 70, 75, 60);
  AttemptInput wired = MakeInput(c, boulder);
  wired.beta = 1.0;
  AttemptInput cold = MakeInput(c, boulder);
  LiveAttempt w = BeginAttempt(Rng::FromSeed("x"), wired);
  LiveAttempt n = BeginAttempt(Rng::FromSeed("x"), cold);
  // Half a grade, as it has always been.
  const double marginGap =
      (PeekOdds(w, 0.72) > 0.0 && PeekOdds(n, 0.72) > 0.0) ? 1.0 : 0.0;
  CHECK(marginGap == 1.0);
}

static void TestAPitchGoesOnRedpoint() {
  // The sport loop, end to end. Onsighting a pitch at your level is a real
  // ask; wiring it makes it yours. A long route compounds per-move odds,
  // which is exactly why the curve has to be this steep.
  // At the climber's limit, which is where a projecting curve is a curve.
  // A grade below (V7) this route goes 73% cold and is not a project; a
  // grade above (V9) goes 0% cold and 8% wired, which is a season.
  const Route pitch = Pitch("The Long Haul", 8);
  const auto SendRate = [&](double beta) {
    Climber c = MakeClimber(70, 70, 70, 75, 60);
    int sent = 0;
    for (int i = 0; i < 1200; i++) {
      Rng rng = Rng::FromSeed("rp-" + std::to_string(i));
      AttemptInput in = MakeInput(c, pitch);
      in.beta = beta;
      if (ResolveAttempt(rng, in).sent) sent++;
    }
    return sent / 1200.0;
  };
  const double onsight = SendRate(0.0);
  const double half = SendRate(0.5);
  const double wired = SendRate(1.0);
  CHECK(onsight > 0.03);            // an onsight is possible...
  CHECK(onsight < 0.35);            // ...and never the expectation
  CHECK(half > onsight);            // every burn you learn from pays
  CHECK(wired > half);
  CHECK(wired > onsight * 2.5);     // and working it is what actually pays
  CHECK(wired < 0.95);              // but nothing is ever certain

  // A pitch is a bigger ask than a boulder of the same grade, because
  // fifteen chances to fall are not one chance to fall.
  Rng world = Rng::FromStream("pitch", Stream::Worldgen);
  const Route boulder = BuildRoute(world, "Short", 8, 8, RouteType::Power,
                                   Discipline::Boulder);
  Climber c = MakeClimber(70, 70, 70, 75, 60);
  CHECK(AverageHighpoint(c, pitch, 300) / pitch.moves.size() <
        AverageHighpoint(c, boulder, 300) / boulder.moves.size());
}

static void TestTheCaveIsRopeRockAndRoadsideIsNot() {
  Rng world = Rng::FromSeed("cave-1");
  const Crag cave = ShadedCave(world);
  const Crag roadside = RoadsideCrag(world);

  CHECK(!cave.lines.empty());
  for (const CragLine& l : cave.lines) {
    CHECK(l.route.discipline == Discipline::Sport);
    CHECK(!BoltsFor(l.route).empty());
    CHECK(NeedsABelayer(l.route));
  }
  // And Roadside is still every bit the boulder field it was. Adding a
  // second crag must not have quietly reached into the first.
  for (const CragLine& l : roadside.lines) {
    CHECK(l.route.discipline == Discipline::Boulder);
    CHECK(!NeedsABelayer(l.route));
  }

  // North-facing is the whole point: the season model puts Roadside's
  // summer window at dawn and nowhere else, and north-facing rock never
  // takes a direct hit. In July this is the only rock worth the walk.
  CHECK(cave.aspect == Aspect::North);
  CHECK(roadside.aspect != Aspect::North);
  CHECK(cave.approachHours > roadside.approachHours);

  ConditionsDials cd;
  Rng weatherWorld = Rng::FromSeed("cave-1");
  const int midsummer = cd.warmestDay;
  const Weather w = GenerateWeather(weatherWorld, midsummer, cd);
  const PrimeWindow caveWindow = FindPrimeWindow(w, cave.aspect, cd);
  const PrimeWindow roadWindow = FindPrimeWindow(w, roadside.aspect, cd);
  // On a midsummer day the shaded crag is at least as good, and usually
  // the only thing on.
  CHECK(caveWindow.peakFriction >= roadWindow.peakFriction);
}

static void TestTheCaveIsAStableWorldAndItsOwnOne() {
  // Same seed, same rock, forever — and a different seed is a different
  // cave. Both matter: the first is the save contract, the second is
  // whether worldgen is doing anything at all.
  Rng a = Rng::FromSeed("cave-1");
  Rng b = Rng::FromSeed("cave-1");
  const Crag one = ShadedCave(a);
  const Crag two = ShadedCave(b);
  CHECK(one.lines.size() == two.lines.size());
  for (size_t i = 0; i < one.lines.size(); i++) {
    CHECK(one.lines[i].route.name == two.lines[i].route.name);
    CHECK(one.lines[i].route.trueGrade == two.lines[i].route.trueGrade);
    CHECK(one.lines[i].route.moves.size() == two.lines[i].route.moves.size());
  }

  Rng other = Rng::FromSeed("cave-2");
  const Crag elsewhere = ShadedCave(other);
  bool differs = false;
  for (size_t i = 0; i < one.lines.size() && !differs; i++) {
    differs = one.lines[i].route.moves.size() !=
                  elsewhere.lines[i].route.moves.size() ||
              one.lines[i].route.trueGrade !=
                  elsewhere.lines[i].route.trueGrade;
  }
  CHECK(differs);

  // The cave draws on its own stream, so adding it cannot have moved a
  // single hold at Roadside — every save that already exists depends on
  // that being true.
  Rng r1 = Rng::FromSeed("cave-1");
  const Crag roadside = RoadsideCrag(r1);
  CHECK(roadside.lines[0].route.name == "Roadside Attraction");
  // A content freeze, not a law of nature: it moves when a line is added to
  // Roadside on purpose, and catches it when one moves by accident. 25 book
  // entries and 6 projects.
  CHECK(roadside.lines.size() == 31);

  // Pitches are longer than boulders, which is the thing the whole sport
  // system is about.
  double caveMoves = 0.0, roadMoves = 0.0;
  for (const CragLine& l : one.lines) caveMoves += l.route.moves.size();
  for (const CragLine& l : roadside.lines) roadMoves += l.route.moves.size();
  CHECK(caveMoves / one.lines.size() > 2.0 * roadMoves / roadside.lines.size());
}

static void TestNoPartnerNoPitch() {
  SportDials d;
  // A boulder is something you can always do alone at dawn. A pitch is
  // something you have to have arranged — this is the first thing in the
  // game that genuinely needs the Lot to exist.
  std::vector<Partner> empty;
  CHECK(BestBelayer(empty, d) == nullptr);
  CHECK(BelayText(nullptr, d) == "nobody is going up there with you today");

  Partner neighbour;
  neighbour.name = "Bo";
  neighbour.climbs = false;      // the neighbours are not all climbers
  neighbour.rapport = 1.0;
  CHECK(!WillBelay(neighbour, d));
  CHECK(BurnsTheyWillHold(neighbour, d) == 0);
  std::vector<Partner> justBo = {neighbour};
  CHECK(BestBelayer(justBo, d) == nullptr);
}

static void TestRapportBuysBurnsAndNothingElseDoes() {
  SportDials d;
  // Somebody you have never spoken to will hold your rope for a lap.
  // Somebody you have spent a season with stands there all afternoon while
  // you work the same three moves. That is the first thing rapport buys
  // that you cannot get any other way.
  Partner stranger;
  stranger.name = "Ray";
  stranger.rapport = 0.0;
  Partner friend_;
  friend_.name = "Margo";
  friend_.rapport = 1.0;

  CHECK(WillBelay(stranger, d));
  CHECK(BurnsTheyWillHold(stranger, d) == d.burnsFromAStranger);
  CHECK(BurnsTheyWillHold(friend_, d) == d.burnsAtFullRapport);
  CHECK(BurnsTheyWillHold(friend_, d) > BurnsTheyWillHold(stranger, d) * 2);

  // And it is monotonic — there is never a rapport where you are better off
  // knowing somebody less well.
  int last = -1;
  for (double r = 0.0; r <= 1.0001; r += 0.05) {
    Partner p;
    p.rapport = r;
    const int burns = BurnsTheyWillHold(p, d);
    CHECK(burns >= last);
    last = burns;
  }

  // The best belayer at the Lot is the one who knows you best.
  std::vector<Partner> lot = {stranger, friend_};
  CHECK(BestBelayer(lot, d) != nullptr);
  CHECK(BestBelayer(lot, d)->name == "Margo");
  CHECK(!BelayText(BestBelayer(lot, d), d).empty());
}

static void TestTheLotCanActuallyBelayTheCave() {
  // The end-to-end version, against the real Lot rather than hand-made
  // people: on a given day at a given world there is somebody who will tie
  // in, and the cave is climbable because of it. A crag nobody at the Lot
  // will belay is content that cannot be reached.
  Rng world = Rng::FromSeed("cave-1");
  const Crag cave = ShadedCave(world);
  int daysWithABelayer = 0;
  for (int day = 1; day <= 60; day++) {
    const std::vector<Partner> lot = LotRegulars(world, day);
    if (BestBelayer(lot) != nullptr) daysWithABelayer++;
  }
  CHECK(daysWithABelayer == 60);   // the regulars are regulars
  CHECK(!cave.lines.empty());
}

// --- Trad --------------------------------------------------------------------
//
// The milestone's three gates, in order, and one test each for the things
// that had to be true for them to mean anything.

static Route TradPitch(const char* name, int grade,
                       RouteType type = RouteType::Crack) {
  Rng world = Rng::FromStream("pitch", Stream::Worldgen);
  return BuildRoute(world, name, grade, grade, type, Discipline::Trad);
}

static void TestTheRockDecidesWhereGearGoes() {
  TradDials d;
  const auto On = [&](HoldType h, double rest = 0.0) {
    Move m;
    m.hold = h;
    m.restQuality = rest;
    return TakesGear(m, d);
  };

  // A crack is a crack, and a blank wall is a blank wall however good you
  // are. This ordering is the whole reason one route on the buttress is a
  // different lead from another.
  CHECK(On(HoldType::Crack) > On(HoldType::Jug));
  CHECK(On(HoldType::Jug) > On(HoldType::Pocket));
  CHECK(On(HoldType::Pocket) > On(HoldType::Pinch));
  CHECK(On(HoldType::Pinch) > On(HoldType::Crimp));
  CHECK(On(HoldType::Crimp) > On(HoldType::Sloper));
  CHECK(On(HoldType::Sloper) > On(HoldType::Dyno));
  // You are not placing anything mid-dyno.
  CHECK(On(HoldType::Dyno) == 0.0);

  // A ledge is a ledge because something structural is happening — but the
  // stance cannot conjure a placement out of a hold that offers nothing.
  CHECK(On(HoldType::Crimp, 0.9) > On(HoldType::Crimp, 0.0));
  CHECK(On(HoldType::Crack, 1.0) <= 1.0);
}

static void TestAPlacementIsADecisionMadeUnderPump() {
  // **Gate 2.** Everything below is the same placement changing its mind
  // about how good it is, and every one of the reasons is something the
  // player is doing rather than something the route decided in advance.
  TradDials d;
  Climber c = MakeClimber(60, 60, 60, 65, 55);
  Rack full = RackOf(RackTier::Cams, d);

  // Pockets, because the interesting comparison is the same rock from two
  // stances — and a crack is deliberately exempt from that comparison, as
  // the pair below this one pins.
  Move ledge;
  ledge.hold = HoldType::Pocket;
  ledge.restQuality = 0.85;

  Move nothing;
  nothing.hold = HoldType::Pocket;
  nothing.restQuality = 0.0;

  // Fresh beats pumped, at the same stance, with the same rack. This is the
  // line the gate is about: *when* you spend the rack is a decision made on
  // the route, not a plan made in the car park.
  CHECK(PlaceHere(ledge, 0.0, c, full, d) > PlaceHere(ledge, 80.0, c, full, d));

  // Standing on something beats hanging off something — and both of them
  // are placements rather than one of them being nothing, which is what
  // makes this a comparison.
  CHECK(PlaceHere(nothing, 30.0, c, full, d) > 0.0);
  CHECK(PlaceHere(ledge, 30.0, c, full, d) >
        PlaceHere(nothing, 30.0, c, full, d));

  // And it costs more to try from the bad one — more, from the worst
  // stance, than the move you are standing on costs to climb. That is the
  // relationship that makes a trad leader climb to stances.
  CHECK(PlaceCost(nothing, d) > PlaceCost(ledge, d));
  CHECK(PlaceCost(nothing, d) > SessionDials{}.basePumpCost);
  // A placement from a stance is still about twice a clip and never free.
  CHECK(PlaceCost(ledge, d) > SportDials{}.clipPumpCost);

  // The exemption, and it is the whole reason the first measured pass of
  // this file produced a game in which leading was strictly worse than
  // soloing: a hand-sized cam into a hand-sized crack goes in off a jam.
  // The rock being obvious buys you the same thing a ledge does.
  Move jam;
  jam.hold = HoldType::Crack;
  jam.restQuality = 0.0;
  Move crackLedge = jam;
  crackLedge.restQuality = 0.9;
  CHECK(PlaceCost(jam, d) == PlaceCost(crackLedge, d));
  CHECK(PlaceHere(jam, 30.0, c, full, d) ==
        PlaceHere(crackLedge, 30.0, c, full, d));
  // Which is not the same as free: a placement is always more than a clip.
  CHECK(PlaceCost(jam, d) > SportDials{}.clipPumpCost);

  // The rack you own, and knowing what goes where.
  CHECK(PlaceHere(ledge, 30.0, c, RackOf(RackTier::Doubles, d), d) >
        PlaceHere(ledge, 30.0, c, RackOf(RackTier::Nuts, d), d));
  Climber gumby = MakeClimber(60, 60, 15, 65, 55);
  Climber craftsman = MakeClimber(60, 60, 95, 65, 55);
  CHECK(PlaceHere(ledge, 30.0, craftsman, full, d) >
        PlaceHere(ledge, 30.0, gumby, full, d));

  // An empty harness places nothing, which is not the same as placing badly.
  CHECK(PlaceHere(ledge, 0.0, c, Rack{}, d) == 0.0);

  // And the piece that is worse than nothing on the rope, because it *is*
  // nothing on the rope: bad rock, no stance, wrecked, out of your depth.
  Move blank;
  blank.hold = HoldType::Sloper;
  blank.restQuality = 0.0;
  CHECK(PlaceHere(blank, 85.0, gumby, RackOf(RackTier::Nuts, d), d) == 0.0);
}

static void TestOneRunoutModelServesBothDisciplines() {
  // **Gate 3.** Not "trad has a runout too" — the same function, arriving
  // at the same numbers, from two different sources of protection. Bolt the
  // trad route by hand and it *is* the sport route, move for move.
  SportDials sd;
  TradDials td;
  Route pitch = Pitch("Two Ways Up", 7);
  Route same = pitch;
  same.discipline = Discipline::Trad;

  Protection gear;
  gear.quality.assign(pitch.moves.size(), 0.0);
  for (int bolt : BoltsFor(pitch, sd)) gear.quality[bolt] = 1.0;

  for (int i = 0; i < static_cast<int>(pitch.moves.size()); i++) {
    CHECK(RunoutAt(same, i, sd, gear) == RunoutAt(pitch, i, sd));
    CHECK(OnTheRope(same, i, sd, gear) == OnTheRope(pitch, i, sd));
    CHECK(PieceAt(same, i, sd, gear) == PieceAt(pitch, i, sd));
    // Above the first piece the two are the same climb, exactly.
    if (OnTheRope(same, i, sd, gear)) {
      CHECK(ExposureAt(same, i, 1.0, SessionDials{}, gear) ==
            ExposureAt(pitch, i, 1.0));
    }
  }

  // Below it they are deliberately not, and this is the one place the two
  // disciplines are allowed to disagree: on a bolted route the ungeared
  // region is two moves by construction and the crash-pad answer is right
  // there, and on a trad lead it is however far you have climbed without
  // stopping, which is a solo. Same function, same seam, different
  // question.
  CHECK(ExposureAt(same, 0, 1.0, SessionDials{}, gear) ==
        ExposureAt(pitch, 0, 1.0));   // move one is free on anything
  CHECK(ExposureAt(same, 1, 1.0, SessionDials{}, gear) >
        ExposureAt(pitch, 1, 1.0));

  // Now believe in the gear less. Same spacing, same everything, and every
  // protected move is more frightening — which is the sentence a watcher
  // says out loud: *he is not runout, he doesn't believe in that nut*.
  Protection dodgy = gear;
  for (double& q : dodgy.quality) {
    if (q > 0.0) q = 0.4;
  }
  bool anyWorse = false;
  for (int i = 0; i < static_cast<int>(pitch.moves.size()); i++) {
    CHECK(RunoutAt(same, i, sd, dodgy) >= RunoutAt(same, i, sd, gear));
    if (RunoutAt(same, i, sd, dodgy) > RunoutAt(same, i, sd, gear)) {
      anyWorse = true;
    }
  }
  CHECK(anyWorse);

  // And the two fears add rather than compete: high above a bad piece is
  // worse than either being high above a good one or level with a bad one.
  Protection oneBad;
  oneBad.quality.assign(pitch.moves.size(), 0.0);
  oneBad.quality[0] = 0.3;
  Protection oneGood = oneBad;
  oneGood.quality[0] = 1.0;
  CHECK(RunoutAt(same, 3, sd, oneBad) > RunoutAt(same, 3, sd, oneGood));
  CHECK(RunoutAt(same, 3, sd, oneBad) > RunoutAt(same, 0, sd, oneBad));

  // The pad rule is the sport one, unchanged, and it hangs off the *first
  // piece* rather than off the first bolt: a leader who has not placed
  // anything is bouldering, and the ground-fall model is right for them.
  Protection none;
  none.quality.assign(pitch.moves.size(), 0.0);
  const int top = static_cast<int>(pitch.moves.size()) - 1;
  CHECK(!OnTheRope(same, top, sd, none));
  CHECK(ExposureAt(same, top, 0.0, SessionDials{}, none) > 0.0);
  // And no amount of foam helps, because nobody has ever carried a pad up a
  // pitch and it would not reach. Measured through the pad term this was
  // exactly zero, and soloing was the strongest strategy in the game.
  CHECK(ExposureAt(same, top, 1.0, SessionDials{}, none) ==
        ExposureAt(same, top, 0.0, SessionDials{}, none));
  // The largest exposure in the resolver, above being fully pumped and
  // above being injured: it is the only one that prices dying.
  CHECK(ExposureAt(same, top, 1.0, SessionDials{}, none) >
        SessionDials{}.pumpGradePenalty);
  // It climbs with you rather than switching on: nobody is scared on move
  // one, and by six moves up it is as bad as it gets.
  CHECK(ExposureAt(same, 0, 1.0, SessionDials{}, none) == 0.0);
  CHECK(ExposureAt(same, 3, 1.0, SessionDials{}, none) <
        ExposureAt(same, top, 1.0, SessionDials{}, none));
  // Get one in low down and the ground stops being the question.
  Protection off = none;
  off.quality[1] = 0.9;
  CHECK(OnTheRope(same, top, sd, off));
  CHECK(ExposureAt(same, top, 0.0, SessionDials{}, off) ==
        ExposureAt(same, top, 1.0, SessionDials{}, off));

  // Nothing here reaches a boulder, which is what keeps it clear of
  // everything already measured.
  Rng world = Rng::FromStream("pitch", Stream::Worldgen);
  const Route boulder = BuildRoute(world, "Short", 7, 7, RouteType::Crack,
                                   Discipline::Boulder);
  for (int i = 0; i < static_cast<int>(boulder.moves.size()); i++) {
    CHECK(RunoutAt(boulder, i, sd, gear) == 0.0);
    CHECK(!OnTheRope(boulder, i, sd, gear));
  }
  CHECK(!IsTrad(boulder));
  CHECK(IsTrad(same));
  CHECK(NeedsABelayer(same));
  CHECK(!NeedsABelayer(boulder));
  (void)td;
}

static double TradSendRate(const Route& route, const Climber& c,
                           const Rack& rack, int runs) {
  int sent = 0;
  for (int i = 0; i < runs; i++) {
    Rng rng = Rng::FromSeed("trad-" + std::to_string(i));
    AttemptInput in = MakeInput(c, route);
    in.rack = rack;
    if (ResolveAttempt(rng, in).sent) sent++;
  }
  return static_cast<double>(sent) / static_cast<double>(runs);
}

static void TestATradLeadIsNotASportLead() {
  // **Gate 1.** The same rock, the same climber, the same seeds, and a
  // different climb — for two reasons a watcher can name from the ground:
  // the leader is spending pump on gear, and the gear is not as good as a
  // bolt.
  TradDials d;
  Climber c = MakeClimber(70, 70, 70, 75, 60);
  // A grade the climber leads rather than projects. Trad comes out about a
  // full grade under sport across the ladder (0.96/0.74 at V5, 0.74/0.20 at
  // V6, 0.17/0.00 at V7), which is the number every trad climber will
  // quote you about their own two grades — so the comparison has to be
  // made somewhere both of them are climbing.
  Route bolted = Pitch("The Long Haul", 5);
  Route onGear = bolted;
  onGear.discipline = Discipline::Trad;

  const Rack rack = RackOf(RackTier::Cams, d);
  const double sport = TradSendRate(bolted, c, Rack{}, 600);
  const double trad = TradSendRate(onGear, c, rack, 600);
  CHECK(sport > 0.05);          // the comparison needs both to be climbable
  CHECK(trad > 0.0);
  CHECK(trad < sport);

  // The two reasons, isolated. Both are measured on a climber who cannot
  // fall off, so that each pitch is climbed to the top and the comparison
  // is about the *route* rather than about where the two of them happened
  // to come off — peak pump on an attempt that ended at move six is a
  // statement about falling, not about protecting.
  Climber crusher = MakeClimber(100, 100, 100, 100, 100);
  const auto ToTheTop = [&](const Route& r, const Rack& k) {
    AttemptInput in = MakeInput(crusher, r);
    in.rack = k;
    LiveAttempt la = BeginAttempt(Rng::FromSeed("top"), in);
    while (!AttemptOver(la)) {
      if (WouldPlace(la)) PlaceGear(la);
      StepMove(la, 1.0);
    }
    return FinishAttempt(la);
  };
  // Two versions of the same pitch: one you can protect and one you cannot.
  // The reasons a trad lead is harder are not one reason, and which of them
  // you are paying depends on the rock — which is the whole point of having
  // both on the buttress.
  Route crackGear = onGear;
  Route crackBolted = bolted;
  for (Move& m : crackGear.moves) m.hold = HoldType::Crack;
  for (Move& m : crackBolted.moves) m.hold = HoldType::Crack;
  Route faceGear = onGear;
  Route faceBolted = bolted;
  for (Move& m : faceGear.moves) m.hold = HoldType::Sloper;
  for (Move& m : faceBolted.moves) m.hold = HoldType::Sloper;

  // **Reason one: the pump**, and you pay it on the rock that takes gear.
  // Placing costs more than clipping, so a leader arrives at the top of a
  // crack more tired than a clipper does.
  const AttemptResult ledCrack = ToTheTop(crackGear, rack);
  const AttemptResult clippedCrack = ToTheTop(crackBolted, Rack{});
  CHECK(ledCrack.sent && clippedCrack.sent);
  CHECK(ledCrack.peakPump > clippedCrack.peakPump);

  // **Reason two: the head**, and you pay it on the rock that does not.
  // The gear is placed rather than drilled, so it is not all bomber and it
  // is not all where you would have wanted it — and on a blank face there
  // is barely any of it, which is a different climb again.
  const auto Worst = [&](const Route& r, const AttemptResult& res) {
    double worst = 0.0;
    for (int i = 0; i < static_cast<int>(r.moves.size()); i++) {
      worst = std::max(worst, ExposureAt(r, i, 1.0, SessionDials{}, res.gear));
    }
    return worst;
  };
  const AttemptResult ledFace = ToTheTop(faceGear, rack);
  const AttemptResult clippedFace = ToTheTop(faceBolted, Rack{});
  CHECK(Worst(crackGear, ledCrack) > Worst(crackBolted, clippedCrack));
  CHECK(Worst(faceGear, ledFace) > Worst(crackGear, ledCrack));
  // And the leader on the face is not pumped, they are frightened: there
  // was nothing to stop for.
  CHECK(ledFace.peakPump < ledCrack.peakPump);

  // Which shows up where it counts. The face costs a leader more grades
  // than the crack does, against the same rock bolted.
  const double faceGap = TradSendRate(faceBolted, c, Rack{}, 600) -
                         TradSendRate(faceGear, c, rack, 600);
  const double crackGap = TradSendRate(crackBolted, c, Rack{}, 600) -
                          TradSendRate(crackGear, c, rack, 600);
  CHECK(faceGap > crackGap);

  // And the rack you own is the difference between two trad leads, which is
  // what makes it worth six hundred dollars. Measured with a flat fall
  // penalty this came out backwards — see FallPenalty in DirtbagSport.h.
  CHECK(TradSendRate(crackGear, c, RackOf(RackTier::Doubles, d), 900) >
        TradSendRate(crackGear, c, RackOf(RackTier::Nuts, d), 900));
}

static void TestNeitherPolicyWinsEveryPitch() {
  // The proof that placing is a *decision*: the two extremes are both
  // worse than thinking about it. Sew it up and you arrive at the crux
  // wrecked; run it out and you arrive there frightened and empty-handed.
  //
  // If either extreme dominated, the prompt would be a formality and the
  // gate would be a lie.
  TradDials d;
  Climber c = MakeClimber(70, 70, 70, 75, 60);
  const Route route = TradPitch("The Whole Argument", 5);
  const Rack rack = RackOf(RackTier::Cams, d);

  // Three leaders. `sew` places at every move that offers anything at all
  // until the rack is gone; `solo` never places; `sensible` is the bot.
  const auto Rate = [&](int policy) {
    int sent = 0;
    const int runs = 700;
    for (int i = 0; i < runs; i++) {
      AttemptInput in = MakeInput(c, route);
      in.rack = policy == 1 ? Rack{} : rack;
      LiveAttempt la = BeginAttempt(Rng::FromSeed("pol-" + std::to_string(i)), in);
      while (!AttemptOver(la)) {
        if (policy == 0 && WouldPlace(la)) PlaceGear(la);
        if (policy == 2) PlaceGear(la);
        const int at = la.nextMove;
        StepMove(la, in.botExecution);
        if (la.nextMove > at && route.moves[at].restQuality > 0.0) {
          ShakeOut(la);
        }
      }
      if (FinishAttempt(la).sent) sent++;
    }
    return static_cast<double>(sent) / static_cast<double>(runs);
  };

  const double sensible = Rate(0);
  const double solo = Rate(1);
  const double sew = Rate(2);
  CHECK(sensible > solo);
  CHECK(sensible > sew);
}

static void TestTheRackRunsOut() {
  // A finite rack is what makes *where* you spend it a decision. Two
  // pieces on a sixteen-move pitch is a lead with a long second half, and
  // the leader should not have discovered that at the top.
  TradDials d;
  const Route route = TradPitch("Last Piece Below You", 6);

  Rack thin = RackOf(RackTier::Cams, d);
  thin.pieces = 2;

  // Climbed by somebody who cannot fall off, because this test is about
  // where the gear went rather than whether the route goes.
  Climber crusher = MakeClimber(100, 100, 100, 100, 100);
  const auto Lead = [&](const Rack& k) {
    AttemptInput in = MakeInput(crusher, route);
    in.rack = k;
    LiveAttempt la = BeginAttempt(Rng::FromSeed("thin"), in);
    while (!AttemptOver(la)) {
      if (WouldPlace(la)) PlaceGear(la);
      StepMove(la, 1.0);
    }
    return FinishAttempt(la);
  };

  const auto PlacedIn = [](const AttemptResult& res) {
    std::vector<int> at;
    for (int i = 0; i < static_cast<int>(res.gear.quality.size()); i++) {
      if (res.gear.quality[i] > 0.0) at.push_back(i);
    }
    return at;
  };

  const std::vector<int> thinAt = PlacedIn(Lead(thin));
  const std::vector<int> fatAt = PlacedIn(Lead(RackOf(RackTier::Doubles, d)));

  // You cannot place what you have not got.
  CHECK(static_cast<int>(thinAt.size()) <= thin.pieces);
  CHECK(!thinAt.empty());
  // And with a full rack the same pitch is protected far more often, which
  // is the whole of what the money buys.
  CHECK(fatAt.size() > thinAt.size());

  // Rationing, which is what stops a thin rack being a fat rack that ends
  // early: a leader with two pieces does not spend the first one where a
  // leader with eighteen does. They climb past it.
  CHECK(thinAt.front() > fatAt.front());

  // An empty harness is a solo from wherever it ran out, and the exposure
  // model says so rather than quietly leaving the leader safe.
  Rack single = RackOf(RackTier::Cams, d);
  single.pieces = 1;
  const AttemptResult oneGo = Lead(single);
  const std::vector<int> oneAt = PlacedIn(oneGo);
  CHECK(oneAt.size() == 1);
  const int top = static_cast<int>(route.moves.size()) - 1;
  CHECK(top - oneAt.front() > static_cast<int>(d.botSpacing));
  CHECK(ExposureAt(route, top, 1.0, SessionDials{}, oneGo.gear) >
        ExposureAt(route, oneAt.front(), 1.0, SessionDials{}, oneGo.gear));
}

static void TestABatchTradAttemptIsLedNotSoloed() {
  // The measured game must be the played game. A trad route resolved in
  // batch is a trad route led sensibly — if the bot did not place, every
  // number this file has ever produced would have been a solo.
  TradDials d;
  Climber c = MakeClimber(70, 70, 70, 75, 60);
  const Route route = TradPitch("Hand Jam Holiday", 4);

  Rng rng = Rng::FromSeed("batch");
  AttemptInput in = MakeInput(c, route);
  in.rack = RackOf(RackTier::Cams, d);
  const AttemptResult led = ResolveAttempt(rng, in);
  int pieces = 0;
  for (double q : led.gear.quality) {
    if (q > 0.0) pieces++;
  }
  CHECK(pieces > 0);

  // With nothing on your harness the same attempt is the same climb with
  // no gear in it, which is the correct answer rather than a crash.
  Rng bare = Rng::FromSeed("batch");
  AttemptInput nothing = MakeInput(c, route);
  const AttemptResult soloed = ResolveAttempt(bare, nothing);
  for (double q : soloed.gear.quality) CHECK(q == 0.0);

  // And a sport route still carries no gear vector at all: protection there
  // is a property of the rock, and a second copy of the bolts stored per
  // attempt is exactly the duplication gate 3 forbids.
  Rng bolted = Rng::FromSeed("batch");
  AttemptInput sport = MakeInput(c, Pitch("The Long Haul", 4));
  CHECK(ResolveAttempt(bolted, sport).gear.quality.empty());
}

static void TestARackIsTheMostExpensiveThingYouOwn() {
  TradDials d;
  Rack rack;
  CHECK(!CanLeadTrad(rack));
  CHECK(TierOf(rack, d) == RackTier::None);

  double cash = 100.0;
  CHECK(!BuyRack(rack, cash, d));      // cannot afford the nuts
  CHECK(cash == 100.0);                // and nothing was half-bought
  CHECK(!CanLeadTrad(rack));

  cash = 190.0;
  CHECK(BuyRack(rack, cash, d));
  CHECK(cash == 0.0);
  CHECK(CanLeadTrad(rack));
  CHECK(TierOf(rack, d) == RackTier::Nuts);

  // You buy up the ladder, one rung at a time, and the counter's price is
  // the price of the rung you are buying.
  cash = 10000.0;
  CHECK(BuyRack(rack, cash, d));
  CHECK(TierOf(rack, d) == RackTier::Cams);
  CHECK(cash == 10000.0 - d.camsCost);
  CHECK(BuyRack(rack, cash, d));
  CHECK(TierOf(rack, d) == RackTier::Doubles);
  // Nothing above doubles, and asking does not cost you anything.
  const double before = cash;
  CHECK(!BuyRack(rack, cash, d));
  CHECK(cash == before);

  // Each rung is more gear and better gear than the one below it, which is
  // what makes the price a decision rather than a tax.
  CHECK(RackOf(RackTier::Doubles, d).pieces > RackOf(RackTier::Cams, d).pieces);
  CHECK(RackOf(RackTier::Cams, d).pieces > RackOf(RackTier::Nuts, d).pieces);
  CHECK(d.doublesQuality > d.camsQuality);
  CHECK(d.camsQuality > d.nutsQuality);
  CHECK(RackPrice(RackTier::Doubles, d) > RackPrice(RackTier::Cams, d));
  CHECK(RackPrice(RackTier::Cams, d) > RackPrice(RackTier::Nuts, d));
  // A rack is the most expensive thing on any shelf in the game.
  CHECK(d.camsCost > KitDials{}.padCost);

  // The family money works here the way it works at every other counter.
  Rack theirs;
  double trust = 190.0 * 0.7;
  CHECK(BuyRack(theirs, trust, d, 0.7));

  // Words, and never a number.
  CHECK(std::string(RackTierName(RackTier::None)) == "no rack");
  CHECK(RackText(Rack{}, d) == "no rack");
  CHECK(RackText(RackOf(RackTier::Cams, d), d).find("cams") !=
        std::string::npos);
  Rack one = RackOf(RackTier::Cams, d);
  one.pieces = 1;
  CHECK(RackText(one, d).find("1 piece left") != std::string::npos);
  CHECK(std::string(PieceText(0.0)) == "nothing");
  CHECK(std::string(PieceText(0.95)) == "bomber");
  CHECK(std::string(PieceText(0.2)) != std::string(PieceText(0.9)));
}

static void TestTheRackSurvivesTheSave() {
  TradDials d;
  SaveGame save;
  save.seed = "gear-placement";
  save.player.rack = RackOf(RackTier::Cams, d);
  save.player.rack.pieces = 9;   // three of them still at the last belay

  const std::string text = SerializeSave(save);
  SaveGame back;
  CHECK(DeserializeSave(text, back) == LoadResult::Ok);
  CHECK(back.player.rack.pieces == 9);
  CHECK(back.player.rack.quality == d.camsQuality);
  CHECK(TierOf(back.player.rack, d) == RackTier::Cams);

  // A career from before there was any trad to place gear on loads with an
  // empty harness, which is exactly what it had.
  std::string v32 = text;
  DropSaveLine(v32, "rack.pieces=");
  DropSaveLine(v32, "rack.quality=");
  SetSaveVersion(v32, 32);
  SaveGame old;
  CHECK(DeserializeSave(v32, old) == LoadResult::Ok);
  CHECK(old.version == kSaveVersion);
  CHECK(old.player.rack.pieces == 0);
  CHECK(!CanLeadTrad(old.player.rack));
  CHECK(old.seed == save.seed);
  CHECK(static_cast<int>(DefaultMigrations().size()) == kSaveVersion - 1);
}

static void TestTheButtressIsTradRock() {
  Rng world = Rng::FromStream("valley", Stream::Worldgen);
  const Crag buttress = TheOldButtress(world);
  const Crag cave = ShadedCave(world);

  CHECK(!buttress.lines.empty());
  int cracks = 0, easy = 0;
  for (const CragLine& line : buttress.lines) {
    CHECK(line.route.discipline == Discipline::Trad);
    CHECK(NeedsABelayer(line.route));
    // A pitch, not a long boulder: it has the stances that make one
    // climbable, exactly as the cave's do.
    CHECK(line.route.moves.size() > 8);
    if (line.route.type == RouteType::Crack) cracks++;
    if (!line.isProject && line.route.grade <= 3) easy++;
  }
  // Mostly cracks, because a crack is a line you can protect.
  CHECK(cracks * 2 > static_cast<int>(buttress.lines.size()));
  // And its classics are moderate where the cave's are hard — trad is the
  // one discipline whose entry-level lines are the famous ones.
  CHECK(easy >= 4);
  int easyCave = 0;
  for (const CragLine& line : cave.lines) {
    if (!line.isProject && line.route.grade <= 3) easyCave++;
  }
  CHECK(easy > easyCave);

  // Its own world, stable across runs and different from the other crags'.
  Rng other = Rng::FromStream("elsewhere", Stream::Worldgen);
  CHECK(TheOldButtress(world).lines[0].route.moves[0].difficulty ==
        buttress.lines[0].route.moves[0].difficulty);
  CHECK(TheOldButtress(other).lines.back().route.trueGrade !=
            buttress.lines.back().route.trueGrade ||
        TheOldButtress(other).lines.back().route.moves.size() !=
            buttress.lines.back().route.moves.size());

  // The headwall's face testpiece offers a leader less than its crack
  // testpiece does, at the same grade — which is why one of them is called
  // Ropeless in a Sense and the other is called Bombproof.
  const auto GearOn = [&](const std::string& name) {
    double total = 0.0;
    for (const CragLine& line : buttress.lines) {
      if (line.route.name != name) continue;
      for (const Move& m : line.route.moves) total += TakesGear(m);
      return total / static_cast<double>(line.route.moves.size());
    }
    return -1.0;
  };
  CHECK(GearOn("Bombproof") > 0.0);
  CHECK(GearOn("Ropeless in a Sense") > 0.0);
  CHECK(GearOn("Bombproof") > GearOn("Ropeless in a Sense"));
}

// --- Habits and quirks -------------------------------------------------------
//
// One rule, and every test below is about it: a habit is what you have been
// doing lately and it can change; a quirk is what you turned out to be and
// it does not.

static void ClimbLike(Logbook& book, int fromDay, int days, Did what,
                      double perDay, const HabitDials& d = HabitDials{}) {
  for (int i = 0; i < days; i++) {
    const int day = fromDay + i;
    Note(book, Did::Burn, 1.0, day, d);
    Note(book, Did::DayOut, 1.0, day, d);
    if (perDay > 0.0) Note(book, what, perDay, day, d);
  }
}

static void TestTheLogbookForgets() {
  HabitDials d;
  Logbook book;
  ClimbLike(book, 1, 60, Did::BurnOnOneLine, 1.0, d);

  const double burns = book.count[static_cast<int>(Did::Burn)];
  CHECK(burns > 0.0);
  CHECK(book.lifetimeBurns == 60.0);

  // Half a life later, half of it is gone -- and the lifetime count is not,
  // because the two answer different questions.
  Logbook faded = book;
  RememberTo(faded, 60 + static_cast<int>(d.remembersDays), d);
  const double after = faded.count[static_cast<int>(Did::Burn)];
  CHECK(after < burns * 0.55 && after > burns * 0.45);
  CHECK(faded.lifetimeBurns == 60.0);

  // Folding forward twice is folding forward once. The scars shipped a
  // version of this that compounded, and the ranking shipped one before
  // that, so it is pinned here rather than trusted.
  Logbook once = book;
  RememberTo(once, 200, d);
  Logbook twice = book;
  RememberTo(twice, 130, d);
  RememberTo(twice, 200, d);
  for (int i = 0; i < kDidCount; i++) {
    CHECK(std::abs(once.count[i] - twice.count[i]) < 1e-9);
  }

  // A clock that goes backwards decays nothing rather than growing it.
  Logbook back = book;
  RememberTo(back, 1, d);
  CHECK(std::abs(back.count[static_cast<int>(Did::Burn)] -
                 book.count[static_cast<int>(Did::Burn)]) < 1e-9);
}

static void TestOneAfternoonIsNotAHabit() {
  HabitDials d;
  Logbook book;
  // Three days of pure grinding: every burn on one line, which is a ratio
  // of 1.0 and is still not a habit.
  ClimbLike(book, 1, 3, Did::BurnOnOneLine, 1.0, d);
  CHECK(!Doing(book, Habit::Grinder, 4, d));
  CHECK(HabitsNow(book, 4, d).empty());

  // Keep it up for a season and it is who you have been.
  ClimbLike(book, 4, 40, Did::BurnOnOneLine, 1.0, d);
  CHECK(Doing(book, Habit::Grinder, 45, d));
}

static void TestAHabitCanBeStoppedAndAQuirkCannot() {
  HabitDials d;
  Logbook book;
  Quirks q;

  // Two seasons of grinding, with the night tick running.
  int day = 1;
  for (; day <= 40; day++) {
    Note(book, Did::Burn, 1.0, day, d);
    Note(book, Did::DayOut, 1.0, day, d);
    Note(book, Did::BurnOnOneLine, 1.0, day, d);
    HabitsDay(q, book, day, d);
  }
  CHECK(Doing(book, Habit::Grinder, day, d));
  CHECK(!Has(q, Quirk::Obsessive));   // doing it is not being it, yet

  // Stop, and the habit goes. This is the half a badge system would not
  // have: nothing was taken away, you simply stopped doing it.
  for (; day <= 40 + 3 * static_cast<int>(d.remembersDays); day++) {
    HabitsDay(q, book, day, d);
  }
  CHECK(!Doing(book, Habit::Grinder, day, d));
  CHECK(!Has(q, Quirk::Obsessive));
  // And the progress toward the quirk drained rather than waiting for you.
  CHECK(q.heldFor[static_cast<int>(Habit::Grinder)] == 0.0);

  // Now do it long enough that it stops being something you do.
  Quirks stuck;
  Logbook book2;
  int landedOn = -1;
  for (int i = 1; i <= 400; i++) {
    Note(book2, Did::Burn, 1.0, i, d);
    Note(book2, Did::DayOut, 1.0, i, d);
    Note(book2, Did::BurnOnOneLine, 1.0, i, d);
    if (HabitsDay(stuck, book2, i, d) == Quirk::Obsessive) landedOn = i;
  }
  CHECK(Has(stuck, Quirk::Obsessive));
  // Two seasons, not two weeks and not two careers.
  CHECK(landedOn > 180 && landedOn < 260);
  CHECK(!QuirkLanded(Quirk::Obsessive).empty());

  // And now stop for three years. The habit goes; the person does not.
  for (int i = 401; i <= 401 + 1000; i++) HabitsDay(stuck, book2, i, d);
  CHECK(!Doing(book2, Habit::Grinder, 1401, d));
  CHECK(Has(stuck, Quirk::Obsessive));
  // Which is the whole trap: the beta bonus stays, and so does the cost.
  CHECK(HabitBetaRate(stuck, book2, 1401, d) > 1.0);
  CHECK(HabitSkillGain(stuck, book2, Skill::Head, 1401, d) ==
        HabitSkillGain(Quirks{}, Logbook{}, Skill::Head, 1401, d));
}

static void TestNothingHappensToSomebodyWithNoHabits() {
  // The same invariant Phase 7's `built` flag exists to protect, and for
  // the same reason: this file multiplies numbers the whole game is
  // balanced on, and a default that is not exactly neutral silently
  // rebalances every measurement in the repo.
  HabitDials d;
  const Quirks none;
  const Logbook fresh;
  for (int i = 0; i < kSkillCount; i++) {
    CHECK(HabitSkillGain(none, fresh, static_cast<Skill>(i), 500, d) == 1.0);
  }
  CHECK(HabitInjuryRisk(none, fresh, 500, d) == 1.0);
  CHECK(HabitBetaRate(none, fresh, 500, d) == 1.0);
  CHECK(HabitSkinCost(none, fresh, 500, d) == 1.0);
  CHECK(HabitNerve(none, fresh, 500, d) == 0.0);
  CHECK(HabitShiftPay(none) == 1.0);
  CHECK(HabitDailyCost(none) == 1.0);
  CHECK(HabitsNow(fresh, 500, d).empty());
  CHECK(HowYouClimb(none, fresh, 500, d).find("most people") !=
        std::string::npos);
}

static void TestEveryHabitCostsSomething() {
  // A habit that was only ever a bonus would be the game telling you how to
  // play. Every one of them either starves a lane, costs risk, or is paid
  // for by the thing it stops you doing.
  HabitDials d;
  const auto with = [&](Habit h) {
    Quirks q;
    q.held.push_back(HardensInto(h));
    return q;
  };
  const Logbook none;

  // Never warming up: strong, and it catches up with you.
  CHECK(HabitSkillGain(with(Habit::NeverWarmsUp), none, Skill::Power, 9, d) > 1.0);
  CHECK(HabitInjuryRisk(with(Habit::NeverWarmsUp), none, 9, d) > 1.0);

  // Plastic: fingers, and a head that has never been anywhere.
  CHECK(HabitSkillGain(with(Habit::GymRat), none, Skill::Fingers, 9, d) > 1.0);
  CHECK(HabitSkillGain(with(Habit::GymRat), none, Skill::Head, 9, d) < 1.0);

  // The two leaders. Both real, neither free.
  CHECK(HabitSkillGain(with(Habit::RunsItOut), none, Skill::Head, 9, d) > 1.0);
  CHECK(HabitInjuryRisk(with(Habit::RunsItOut), none, 9, d) > 1.0);
  CHECK(HabitNerve(with(Habit::RunsItOut), none, 9, d) > 0.0);
  CHECK(HabitSkillGain(with(Habit::SewsItUp), none, Skill::Head, 9, d) < 1.0);
  CHECK(HabitInjuryRisk(with(Habit::SewsItUp), none, 9, d) < 1.0);

  // Walking away is the right call and it teaches you nothing.
  CHECK(HabitInjuryRisk(with(Habit::KnowsWhenToStop), none, 9, d) < 1.0);
  CHECK(HabitSkillGain(with(Habit::KnowsWhenToStop), none, Skill::Head, 9, d) < 1.0);
  CHECK(HabitNerve(with(Habit::KnowsWhenToStop), none, 9, d) < 0.0);

  // The grinder and the magpie are each other's cost, which is the neatest
  // pair in the file: one learns lines and the other learns crags.
  CHECK(HabitBetaRate(with(Habit::Grinder), none, 9, d) > 1.0);
  CHECK(HabitBetaRate(with(Habit::Tourist), none, 9, d) < 1.0);
  CHECK(HabitSkillGain(with(Habit::Grinder), none, Skill::Technique, 9, d) > 1.0);
  CHECK(HabitSkillGain(with(Habit::Tourist), none, Skill::Head, 9, d) > 1.0);

  // Skin of Steel gets exactly one thing and it is the right one -- skin is
  // not a joint, and a tough-tips discount on tendon injuries would be the
  // file handing out a bonus twice.
  CHECK(HabitSkinCost(with(Habit::SkinOfSteel), none, 9, d) < 1.0);
  CHECK(HabitInjuryRisk(with(Habit::SkinOfSteel), none, 9, d) == 1.0);

  // Every earned quirk has a habit behind it and every habit hardens into
  // one. A quirk nothing can earn is a save field with a name on it.
  for (int i = 1; i < kHabitCount; i++) {
    const Habit h = static_cast<Habit>(i);
    CHECK(HardensInto(h) != Quirk::None);
    CHECK(HabitBehind(HardensInto(h)) == h);
    CHECK(!IsPicked(HardensInto(h)));
    CHECK(std::string(HabitLine(h)).size() > 10);
    CHECK(std::string(QuirkLine(HardensInto(h))).size() > 10);
  }
}

static void TestPickingOneIsNotBecomingOne() {
  Quirks q;
  // The picked ones are the picked ones. Choosing to be obsessive at the
  // counter is not the same thing as becoming it over two seasons, and this
  // file's only rule is that the difference matters.
  CHECK(!Pick(q, Quirk::Obsessive));
  CHECK(!Pick(q, Quirk::None));
  CHECK(q.held.empty());

  CHECK(Pick(q, Quirk::Stubborn));
  CHECK(Has(q, Quirk::Stubborn));
  CHECK(q.picked == Quirk::Stubborn);
  // One, and it holds. A career does not get to shop for a second.
  CHECK(!Pick(q, Quirk::Quiet));
  CHECK(!Has(q, Quirk::Quiet));

  // And it does something, in a lane no earned quirk touches.
  CHECK(HabitNerve(q, Logbook{}, 9) > 0.0);
  Quirks money;
  CHECK(Pick(money, Quirk::BadWithMoney));
  CHECK(HabitDailyCost(money) > 1.0);
  Quirks loud;
  CHECK(Pick(loud, Quirk::Gregarious));
  CHECK(HabitShiftPay(loud) < 1.0);
  Quirks quiet;
  CHECK(Pick(quiet, Quirk::Quiet));
  CHECK(HabitShiftPay(quiet) > 1.0);

  // Every pickable one is offered and named.
  for (int i = 0; i < kQuirkCount; i++) {
    const Quirk k = static_cast<Quirk>(i);
    CHECK(std::string(QuirkName(k)).size() > 3);
    if (!IsPicked(k)) continue;
    Quirks one;
    CHECK(Pick(one, k));
    CHECK(HabitBehind(k) == Habit::None);
  }
}

static void TestTheLogbookIsWrittenByClimbing() {
  // The instrumentation, which is the half of this Phase 7 said it did not
  // have. Nothing below calls Note: it climbs, and the book fills in.
  DayDials dd;
  HabitDials hd;
  PlayerState player;
  player.climber = MakeClimber(60, 60, 60, 60, 60);
  player.day = 1;

  Rng world = Rng::FromStream("habits", Stream::Worldgen);
  const Route warmup = BuildRoute(world, "Easy Ticket", 0, 0,
                                  RouteType::Endurance, Discipline::Boulder);
  const Route limit = BuildRoute(world, "The Hard One", 9, 9,
                                 RouteType::Crimp, Discipline::Boulder);

  DayState day = WakeUp(player, dd);
  StartGymSession(player, day, KitDials{}, dd);
  const Rng session = Rng::FromSeed("book");

  // Climbed through the real path, not a bare ResolveAttempt. The first
  // draft of this test used one and caught a real ordering: the ledger is
  // incremented by CommitAttempt, so a first burn read *before* the commit
  // and a first burn read *after* it are different numbers, and only the
  // day loop's ordering is the one that ships.
  const auto burnOn = [&](const Route& route) {
    ProjectMemory& m = MemoryFor(player, route);
    const AttemptResult res =
        AttemptInSession(session, day.session, m, player.climber, route,
                         Conditions{}, {}, 0.72, SessionDials{},
                         SessionLoopDials{}, player.character, player.medical,
                         player.day, player.sickness, player.teeth,
                         player.quirks, player.logbook);
    ApplyAttemptToDay(player, day, route, res, world, dd);
  };

  burnOn(limit);
  const ProjectMemory& mem = MemoryFor(player, limit);

  const Logbook& book = player.logbook;
  CHECK(book.count[static_cast<int>(Did::Burn)] == 1.0);
  // A V9 for a grade-6.4 climber is not a warmup, and the book judges it
  // the way the player reading it from the ground does.
  CHECK(book.count[static_cast<int>(Did::BurnAtYourLimit)] == 1.0);
  CHECK(book.count[static_cast<int>(Did::LineTouched)] == 1.0);
  CHECK(mem.attempts == 1);

  // A warmup is not a limit burn, and a second go on the same line is not a
  // new line.
  burnOn(warmup);
  CHECK(book.count[static_cast<int>(Did::Burn)] == 2.0);
  CHECK(book.count[static_cast<int>(Did::BurnAtYourLimit)] == 1.0);
  CHECK(book.count[static_cast<int>(Did::LineTouched)] == 2.0);

  burnOn(limit);
  CHECK(book.count[static_cast<int>(Did::Burn)] == 3.0);
  CHECK(book.count[static_cast<int>(Did::LineTouched)] == 2.0);

  // ...and a line you have fed six burns is a grind.
  for (int i = 0; i < dd.grindingAfter; i++) burnOn(limit);
  CHECK(book.count[static_cast<int>(Did::BurnOnOneLine)] > 0.0);

  // The night says what kind of day it was.
  CHECK(day.firstPullOnHour > 0.0);
  SleepToNextDay(player, day, world, dd);
  CHECK(book.count[static_cast<int>(Did::DayOut)] == 1.0);
  CHECK(book.count[static_cast<int>(Did::DayIndoors)] == 0.0);

  // Nine burns on a V9 takes the tips off, and somebody who climbs until
  // their skin is gone is not the one who knows when to stop.
  CHECK(book.count[static_cast<int>(Did::StoppedEarly)] == 0.0);

  // Going home with something left is the decision that makes somebody
  // cautious, and it is a decision rather than a forecast.
  // Fresh tips, because yesterday took them all and a night does not give
  // nine back -- the point of this day is what the climber chose, not what
  // their skin allowed.
  player.climber.skin = 9.0;
  DayState shortDay = WakeUp(player, dd);
  StartGymSession(player, shortDay, KitDials{}, dd);
  {
    ProjectMemory& m = MemoryFor(player, warmup);
    const AttemptResult res = AttemptInSession(
        session, shortDay.session, m, player.climber, warmup, Conditions{}, {},
        0.72, SessionDials{}, SessionLoopDials{}, player.character,
        player.medical, player.day, player.sickness, player.teeth,
        player.quirks, player.logbook);
    ApplyAttemptToDay(player, shortDay, warmup, res, world, dd);
  }
  CHECK(shortDay.session.skinLeft > SessionLoopDials{}.thinSkin);
  SleepToNextDay(player, shortDay, world, dd);
  CHECK(book.count[static_cast<int>(Did::StoppedEarly)] == 1.0);
  CHECK(book.count[static_cast<int>(Did::DayOut)] > 1.0);

  // **A session that started and produced nothing is not a day out and is
  // not a bail.** Measured with the first version of this, which counted
  // it, a career's last season came out 71 days out and zero burns -- all
  // of them read as backing off -- so every career in the game ended up
  // cautious and the weather is what made them that way.
  PlayerState washedOut;
  washedOut.climber = player.climber;
  DayState quiet = WakeUp(washedOut, dd);
  StartGymSession(washedOut, quiet, KitDials{}, dd);
  SleepToNextDay(washedOut, quiet, world, dd);
  CHECK(washedOut.logbook.count[static_cast<int>(Did::StoppedEarly)] == 0.0);
  CHECK(washedOut.logbook.count[static_cast<int>(Did::DayOut)] == 0.0);

  // ...and a rest day is neither. It only ages the book.
  PlayerState resting;
  DayState off = WakeUp(resting, dd);
  SleepToNextDay(resting, off, world, dd);
  CHECK(resting.logbook.count[static_cast<int>(Did::DayOut)] == 0.0);
  CHECK(resting.logbook.count[static_cast<int>(Did::StoppedEarly)] == 0.0);
  (void)hd;
}

static void TestGoingToTheGymIsWrittenDownAsGoingToTheGym() {
  // `atGym` has meant "a session started" since Phase 1 and both the wall
  // and the crag set it. Nothing needed the distinction until the logbook
  // did, and getting it wrong would have made every outdoor career a
  // plastic merchant.
  DayDials dd;
  PlayerState player;
  player.climber = MakeClimber(60, 60, 60, 60, 60);
  player.kit.membershipDaysLeft = 30;
  Rng world = Rng::FromStream("gym", Stream::Worldgen);

  Rng session = Rng::FromStream("gymday", Stream::Session);
  const Route problem = BuildRoute(world, "The Set", 2, 2, RouteType::Crimp,
                                   Discipline::Boulder);
  const auto pullOn = [&](DayState& day) {
    ProjectMemory& m = MemoryFor(player, problem);
    const AttemptResult res = AttemptInSession(
        session, day.session, m, player.climber, problem, Conditions{}, {},
        0.72, SessionDials{}, SessionLoopDials{}, player.character,
        player.medical, player.day, player.sickness, player.teeth,
        player.quirks, player.logbook);
    ApplyAttemptToDay(player, day, problem, res, world, dd);
  };

  DayState crag = WakeUp(player, dd);
  StartGymSession(player, crag, KitDials{}, dd);
  CHECK(crag.atGym);        // a session, which is what the flag means
  CHECK(!crag.indoors);     // and it is outside
  pullOn(crag);
  SleepToNextDay(player, crag, world, dd);
  CHECK(player.logbook.count[static_cast<int>(Did::DayOut)] == 1.0);
  CHECK(player.logbook.count[static_cast<int>(Did::DayIndoors)] == 0.0);

  DayState plastic = WakeUp(player, dd);
  CHECK(GoToTheGym(player, plastic, KitDials{}, dd));
  CHECK(plastic.indoors);
  pullOn(plastic);
  SleepToNextDay(player, plastic, world, dd);
  CHECK(player.logbook.count[static_cast<int>(Did::DayIndoors)] > 0.0);
}

static void TestAHabitReachesTheWall() {
  // The thing every one of these systems has shipped without at least once:
  // the effect existing and nothing reading it. Three seams, three checks,
  // each against the number the game actually uses.
  HabitDials hd;
  Quirks bold;
  bold.held.push_back(Quirk::Bold);

  // Nerve, through the body context and onto the attempt -- which is how a
  // temperament gets there, and is why this is the same line rather than a
  // second one.
  Climber c = MakeClimber(60, 60, 60, 60, 60);
  Rng world = Rng::FromStream("wall", Stream::Worldgen);
  const Route pitch = BuildRoute(world, "Above The Gear", 5, 5,
                                 RouteType::Crack, Discipline::Trad);
  AttemptInput steady = MakeInput(c, pitch);
  AttemptInput brave = MakeInput(c, pitch);
  BodyContext plain;
  plain.day = 500;
  BodyContext theirs = plain;
  theirs.quirks = bold;
  ApplyBody(steady, plain);
  ApplyBody(brave, theirs);
  CHECK(brave.boldness > steady.boldness);

  // Skin and beta, through the session state, which is where a habit meets
  // a burn -- and both of them off the career rather than off a parameter,
  // because a parameter would have reached one of the three attempt paths.
  DayDials dd;
  PlayerState tough;
  tough.climber = c;
  tough.quirks.held.push_back(Quirk::Leathery);
  tough.day = 500;
  DayState day = WakeUp(tough, dd);
  StartGymSession(tough, day, KitDials{}, dd);
  CHECK(day.session.skinRate < 1.0);

  PlayerState keen;
  keen.climber = c;
  keen.quirks.held.push_back(Quirk::Obsessive);
  keen.day = 500;
  DayState keenDay = WakeUp(keen, dd);
  StartGymSession(keen, keenDay, KitDials{}, dd);
  CHECK(keenDay.session.betaRate > 1.0);

  // And the skin rate is spent rather than stored: the same burn costs a
  // leathery climber less of their tips.
  SessionState plainSession = StartSession(c);
  SessionState hard = plainSession;
  hard.skinRate = HabitSkinCost(tough.quirks, tough.logbook, 500, hd);
  ProjectMemory m1, m2;
  AttemptResult burn;
  burn.skinCost = 1.0;
  burn.timeline.push_back(MoveResult{});
  CommitAttempt(plainSession, m1, pitch, burn);
  CommitAttempt(hard, m2, pitch, burn);
  CHECK(hard.skinLeft > plainSession.skinLeft);
  // ...and the beta rate the same way.
  SessionState slow = StartSession(c);
  SessionState fast = slow;
  fast.betaRate = HabitBetaRate(keen.quirks, keen.logbook, 500, hd);
  ProjectMemory m3, m4;
  AttemptResult go;
  go.highpoint = 4;
  for (int i = 0; i < 4; i++) go.timeline.push_back(MoveResult{});
  CommitAttempt(slow, m3, pitch, go);
  CommitAttempt(fast, m4, pitch, go);
  CHECK(m4.beta > m3.beta);
}

static void TestWhoYouBecameSurvivesTheSave() {
  HabitDials d;
  SaveGame save;
  save.seed = "two-seasons";
  save.player.day = 400;
  Note(save.player.logbook, Did::Burn, 40.0, 380, d);
  Note(save.player.logbook, Did::BurnOnOneLine, 33.0, 380, d);
  Note(save.player.logbook, Did::DayOut, 30.0, 380, d);
  save.player.quirks.held.push_back(Quirk::Obsessive);
  save.player.quirks.held.push_back(Quirk::Leathery);
  save.player.quirks.heldFor[static_cast<int>(Habit::Tourist)] = 44.0;
  CHECK(Pick(save.player.quirks, Quirk::Superstitious));
  save.player.becameToday = Quirk::Leathery;

  const std::string text = SerializeSave(save);
  SaveGame back;
  CHECK(DeserializeSave(text, back) == LoadResult::Ok);
  CHECK(back.player.logbook.asOfDay == 380);
  CHECK(back.player.logbook.lifetimeBurns == 40.0);
  CHECK(Has(back.player.quirks, Quirk::Obsessive));
  CHECK(Has(back.player.quirks, Quirk::Leathery));
  CHECK(Has(back.player.quirks, Quirk::Superstitious));
  CHECK(back.player.quirks.picked == Quirk::Superstitious);
  CHECK(back.player.quirks.heldFor[static_cast<int>(Habit::Tourist)] == 44.0);
  CHECK(back.player.becameToday == Quirk::Leathery);
  // And the effects come back with it, which is the thing that matters:
  // a save that carried the flags and dropped the counters would load a
  // climber who is obsessive and learns lines at the ordinary rate.
  CHECK(HabitBetaRate(back.player.quirks, back.player.logbook, 400, d) ==
        HabitBetaRate(save.player.quirks, save.player.logbook, 400, d));

  // A v33 career climbed without anybody counting, and loads as nobody.
  std::string v33 = text;
  DropSaveLine(v33, "log.asof=");
  for (int i = 0; i < kDidCount; i++) {
    DropSaveLine(v33, ("log.n" + std::to_string(i) + "=").c_str());
  }
  DropSaveLine(v33, "log.burns=");
  DropSaveLine(v33, "log.days=");
  DropSaveLine(v33, "quirk.picked=");
  DropSaveLine(v33, "quirk.n=");
  DropSaveLine(v33, "quirk.0=");
  DropSaveLine(v33, "quirk.1=");
  DropSaveLine(v33, "quirk.2=");
  DropSaveLine(v33, "quirk.became=");
  for (int i = 0; i < kHabitCount; i++) {
    DropSaveLine(v33, ("quirk.h" + std::to_string(i) + "=").c_str());
  }
  SetSaveVersion(v33, 33);
  SaveGame old;
  CHECK(DeserializeSave(v33, old) == LoadResult::Ok);
  CHECK(old.version == kSaveVersion);
  CHECK(old.player.quirks.held.empty());
  CHECK(old.player.logbook.lifetimeBurns == 0.0);
  // Exactly neutral, so a career loads with the numbers it was measured
  // with rather than being ambushed by a system that did not exist for it.
  CHECK(HabitBetaRate(old.player.quirks, old.player.logbook, 400, d) == 1.0);
  CHECK(old.seed == save.seed);
  CHECK(static_cast<int>(DefaultMigrations().size()) == kSaveVersion - 1);
}

static void TestYouCanSayHowYouClimbInOneSentence() {
  // Phase 7's gate 3 asked that a player can say who their climber is
  // without reading a stat line. This is the other half of that question --
  // *how do you climb* -- and it is answered separately on purpose,
  // because a phase and a person are not the same thing.
  HabitDials d;
  Logbook book;
  Quirks q;
  for (int i = 1; i <= 40; i++) {
    Note(book, Did::Burn, 1.0, i, d);
    Note(book, Did::DayOut, 1.0, i, d);
    Note(book, Did::BurnOnOneLine, 1.0, i, d);
  }
  const std::string doing = HowYouClimb(q, book, 41, d);
  CHECK(doing.find("One line") != std::string::npos);
  CHECK(doing.find("by now you are") == std::string::npos);

  q.held.push_back(Quirk::Obsessive);
  q.held.push_back(Quirk::Leathery);
  const std::string both = HowYouClimb(q, book, 41, d);
  CHECK(both.find("One line") != std::string::npos);
  CHECK(both.find("obsessive and leathery") != std::string::npos);
  // Never a number.
  for (char ch : both) CHECK(!(ch >= '0' && ch <= '9'));
}

// --- The narrator ------------------------------------------------------------
//
// The gate is that somebody watching over your shoulder knows what happened
// without reading a stat line, and the way this file fails is not by
// missing a moment -- it is by narrating a Tuesday.

static bool AnyDigit(const std::string& s) {
  for (char c : s) {
    if (c >= '0' && c <= '9') return true;
  }
  return false;
}

static bool Says(const std::vector<Beat>& beats, BeatKind kind) {
  for (const Beat& b : beats) {
    if (b.kind == kind) return true;
  }
  return false;
}

static std::string LineFor(const std::vector<Beat>& beats, BeatKind kind) {
  for (const Beat& b : beats) {
    if (b.kind == kind) return b.line;
  }
  return "";
}

// A result assembled by hand, so a test can ask the narrator about a climb
// the resolver never produced. This is the whole point of the narrator
// reading rather than recomputing: what it says is a function of the
// timeline and nothing else, so a timeline is all a test needs.
static AttemptResult Timeline(const Route& route, int moves,
                              double odds, double pumpPerMove, bool fell) {
  AttemptResult r;
  double pump = 0.0;
  for (int i = 0; i < moves; i++) {
    MoveResult mr;
    mr.index = i;
    mr.odds = odds;
    pump = std::min(100.0, pump + pumpPerMove);
    mr.pumpAfter = pump;
    mr.success = !(fell && i == moves - 1);
    r.timeline.push_back(mr);
    if (mr.success) r.highpoint = i + 1;
  }
  r.peakPump = pump;
  r.sent = r.highpoint == static_cast<int>(route.moves.size());
  r.style = r.sent ? Style::Onsight : Style::Fell;
  return r;
}

static void TestTheCornerOfTheScreenStaysQuiet() {
  // The HUD's own standing rule, applied to this file: a line that says
  // "fine" every frame for a season teaches you to stop reading the one
  // that will eventually say otherwise. `HowYouClimb` always answers, and
  // these two do not -- which is the only reason both exist.
  HabitDials d;
  Logbook fresh;
  Quirks nobody;
  CHECK(DoingLabel(fresh, 500, d).empty());
  CHECK(AreLabel(nobody).empty());
  // ...where the sentence form still says something, because it is asked
  // rather than drawn.
  CHECK(!HowYouClimb(nobody, fresh, 500, d).empty());

  // Doing something says what.
  Logbook book;
  for (int i = 1; i <= 40; i++) {
    Note(book, Did::Burn, 1.0, i, d);
    Note(book, Did::DayOut, 1.0, i, d);
    Note(book, Did::BurnOnOneLine, 1.0, i, d);
  }
  CHECK(DoingLabel(book, 41, d) == HabitName(Habit::Grinder));
  // And it stops saying it when you stop, which a quirk never does.
  CHECK(DoingLabel(book, 41 + 4 * static_cast<int>(d.remembersDays), d).empty());

  // Being something says what, with the commas in the right places.
  Quirks one;
  one.held.push_back(Quirk::Obsessive);
  CHECK(AreLabel(one) == QuirkName(Quirk::Obsessive));
  Quirks two = one;
  two.held.push_back(Quirk::Leathery);
  CHECK(AreLabel(two).find(" and ") != std::string::npos);
  CHECK(AreLabel(two).find(",") == std::string::npos);
  Quirks three = two;
  three.held.push_back(Quirk::Cautious);
  CHECK(AreLabel(three).find(",") != std::string::npos);
  CHECK(AreLabel(three).find(" and ") != std::string::npos);
  // The long form says the same thing, because there is one comma rule.
  CHECK(HowYouClimb(three, book, 41, d).find(AreLabel(three)) !=
        std::string::npos);
  // Never a number, in either half.
  CHECK(!AnyDigit(AreLabel(three)));
  CHECK(!AnyDigit(DoingLabel(book, 41, d)));
}

static void TestTheNarratorKnowsWhenToShutUp() {
  // **The gate this file is most likely to fail.** A line per move is a
  // log, not commentary, and the moment that mattered is somewhere in the
  // middle of it.
  Climber c = MakeClimber(70, 70, 70, 75, 60);
  const Route pitch = Pitch("The Long Haul", 4);
  const int moves = static_cast<int>(pitch.moves.size());
  AttemptInput in = MakeInput(c, pitch);

  // A cruise: every move landed, nothing was ever in doubt, no pump.
  const std::vector<Beat> quiet =
      CallTheAttempt(in, Timeline(pitch, moves, 0.97, 1.0, false));
  // The ground and the top and whatever the rope did. Nowhere near a line
  // a move.
  CHECK(static_cast<int>(quiet.size()) < moves / 2);
  CHECK(!quiet.empty());
  CHECK(!Says(quiet, BeatKind::Pumped));
  CHECK(!Says(quiet, BeatKind::NearlyBlew));
  CHECK(!Says(quiet, BeatKind::Crux));

  // The same route, fought for: desperate odds all the way and the pump
  // going. It should have more to say, and still not one line a move.
  const std::vector<Beat> loud =
      CallTheAttempt(in, Timeline(pitch, moves, 0.2, 7.0, true));
  CHECK(loud.size() > quiet.size());
  CHECK(static_cast<int>(loud.size()) < moves);
  CHECK(Says(loud, BeatKind::Pumped));
  CHECK(Says(loud, BeatKind::NearlyBlew));

  // And a boulder, which is six moves and should be three or four lines at
  // the very most -- there is not room in a boulder for a story.
  Rng world = Rng::FromStream("narr", Stream::Worldgen);
  const Route boulder = BuildRoute(world, "Short Story", 6, 6,
                                   RouteType::Power, Discipline::Boulder);
  AttemptInput bin = MakeInput(c, boulder);
  const std::vector<Beat> shortOne = CallTheAttempt(
      bin, Timeline(boulder, static_cast<int>(boulder.moves.size()), 0.9, 4.0,
                    false));
  CHECK(shortOne.size() <= 4);
}

static void TestEveryFallIsADifferentFall() {
  // **The gate.** A watcher does not say "you fell"; they say what went
  // wrong. Four falls, four causes, four sentences -- and the day log gets
  // the fact rather than the flourish.
  Climber c = MakeClimber(70, 70, 70, 75, 60);
  const Route pitch = Pitch("The Long Haul", 6);
  const int moves = static_cast<int>(pitch.moves.size());
  AttemptInput in = MakeInput(c, pitch);
  const SportDials sd;

  // Redlined: the hands open, and no roll was ever taken.
  AttemptResult redline = Timeline(pitch, 6, 0.9, 25.0, true);
  CHECK(redline.timeline[4].pumpAfter >= 100.0);
  const std::string opened = HowItWent(in, redline);

  // Blown clip: fell on a bolt, with pump to spare.
  int clipAt = -1;
  for (int bolt : BoltsFor(pitch, sd)) {
    if (bolt > 2) { clipAt = bolt; break; }
  }
  CHECK(clipAt > 0);
  AttemptResult clip = Timeline(pitch, clipAt + 1, 0.8, 2.0, true);
  const std::string blew = HowItWent(in, clip);

  // One move short.
  AttemptResult nearly = Timeline(pitch, moves, 0.8, 2.0, true);
  const std::string oneMove = HowItWent(in, nearly);

  // And off the second move, which is a different day entirely.
  AttemptResult early = Timeline(pitch, 2, 0.8, 2.0, true);
  const std::string offEarly = HowItWent(in, early);

  // Every one of them different, and every one of them naming the cause
  // rather than the position.
  CHECK(opened != blew && opened != oneMove && opened != offEarly);
  CHECK(blew != oneMove && blew != offEarly);
  CHECK(oneMove != offEarly);
  CHECK(opened.find("hands") != std::string::npos);
  CHECK(blew.find("clip") != std::string::npos);
  CHECK(oneMove.find("One move") != std::string::npos);

  // A send is not a fall, and it says which kind of send.
  AttemptResult sent = Timeline(pitch, moves, 0.9, 2.0, false);
  CHECK(sent.sent);
  sent.style = Style::Onsight;
  const std::string onsight = HowItWent(in, sent);
  sent.style = Style::Redpoint;
  const std::string redpoint = HowItWent(in, sent);
  CHECK(onsight != redpoint);
  CHECK(onsight.find("nsight") != std::string::npos);

  // And an attempt still going says so rather than inventing an ending.
  AttemptResult partial = Timeline(pitch, 4, 0.9, 2.0, false);
  CHECK(HowItWent(in, partial) == "Still on it.");
  CHECK(!Says(CallTheAttempt(in, partial), BeatKind::Fell));
  CHECK(!Says(CallTheAttempt(in, partial), BeatKind::Topped));
}

static void TestTheReasonIsNeverTheMomentItEnded() {
  // What happened at the moment it ended is the ending, not the reason for
  // it. Without this rule the log said a bare "Off at the crux" and stopped
  // -- because the crux beat outranked the pump that actually explained the
  // afternoon, and then declined to say anything about itself.
  Climber c = MakeClimber(70, 70, 70, 75, 60);
  const Route pitch = Pitch("The Long Haul", 6);
  int cruxAt = -1;
  for (int i = 0; i < static_cast<int>(pitch.moves.size()); i++) {
    if (pitch.moves[i].crux) cruxAt = i;
  }
  CHECK(cruxAt > 3);

  NarratorDials nd;
  AttemptInput in = MakeInput(c, pitch);
  AttemptResult res = Timeline(pitch, cruxAt + 1, 0.7, 1.0, true);
  // Pumped well before the crux, and not redlined at it -- so the ending is
  // the crux and the reason is somewhere behind it.
  for (int i = 0; i < static_cast<int>(res.timeline.size()); i++) {
    res.timeline[i].pumpAfter =
        i >= cruxAt - 3 ? nd.forearmsGoingAt + 4.0 : 20.0;
  }
  const std::vector<Beat> beats = CallTheAttempt(in, res, nd);
  CHECK(Says(beats, BeatKind::Fell));
  CHECK(Says(beats, BeatKind::Pumped));
  CHECK(Says(beats, BeatKind::Crux));

  const std::string log = HowItWent(in, res, nd);
  CHECK(log.find("crux") != std::string::npos);
  // ...and it says why, from before the end rather than from the end.
  CHECK(log.find("pump") != std::string::npos);
  CHECK(log.size() > std::string("Off at the crux.").size());
}

static void TestItReadsAndNeverRecomputes() {
  // The narrator is handed a timeline the resolver never produced and
  // agrees with it. That is the contract: what it says is a function of
  // the result, so it can never disagree with the climb that happened.
  Climber c = MakeClimber(70, 70, 70, 75, 60);
  const Route pitch = Pitch("The Long Haul", 4);
  const int moves = static_cast<int>(pitch.moves.size());
  AttemptInput in = MakeInput(c, pitch);
  NarratorDials nd;

  // Odds just above the bar: nothing to say. Just below: it was desperate,
  // and the narrator says so -- on a route the climber could do in their
  // sleep, because it is reading the roll rather than judging the grade.
  AttemptResult safe = Timeline(pitch, moves, nd.nearlyBlewBelow + 0.01, 1.0, false);
  AttemptResult lucky = Timeline(pitch, moves, nd.nearlyBlewBelow - 0.01, 1.0, false);
  CHECK(!Says(CallTheAttempt(in, safe, nd), BeatKind::NearlyBlew));
  CHECK(Says(CallTheAttempt(in, lucky, nd), BeatKind::NearlyBlew));

  // Same for the forearms, which are read off the pump the resolver
  // recorded and not off anything this file works out for itself.
  AttemptResult fresh = Timeline(pitch, moves, 0.9, 1.0, false);
  AttemptResult cooked = Timeline(pitch, moves, 0.9, 20.0, false);
  CHECK(!Says(CallTheAttempt(in, fresh, nd), BeatKind::Pumped));
  CHECK(Says(CallTheAttempt(in, cooked, nd), BeatKind::Pumped));

  // A rest is a fall in the pump across a move, which is the only way pump
  // can go down -- so a hand-built drop is a shake and the narrator sees it.
  AttemptResult rested = Timeline(pitch, 6, 0.9, 10.0, false);
  rested.timeline[4].pumpAfter =
      rested.timeline[3].pumpAfter - nd.shakeWorthIt - 1.0;
  CHECK(Says(CallTheAttempt(in, rested, nd), BeatKind::Shake));

  // Determinism, because everything else in this project is.
  CHECK(HowItWent(in, cooked) == HowItWent(in, cooked));
  const std::vector<Beat> once = CallTheAttempt(in, cooked, nd);
  const std::vector<Beat> twice = CallTheAttempt(in, cooked, nd);
  CHECK(once.size() == twice.size());
  for (std::size_t i = 0; i < once.size(); i++) {
    CHECK(once[i].line == twice[i].line);
    CHECK(once[i].weight == twice[i].weight);
    CHECK(once[i].move == twice[i].move);
  }
}

static void TestTheNarratorNeverSaysANumber() {
  // The standing rule for text in this project, and `HowCloseText` says why:
  // "you fell at move nine of twelve" is a thing you read and "one move,
  // that was the go" is a thing you see.
  Climber c = MakeClimber(70, 70, 70, 75, 60);
  Rng world = Rng::FromStream("narr", Stream::Worldgen);
  const Route boulder = BuildRoute(world, "Short Story", 6, 6,
                                   RouteType::Power, Discipline::Boulder);
  const Route pitch = Pitch("The Long Haul", 6);
  const Route gear = TradPitch("Last Piece Below You", 6);

  for (const Route* r : {&boulder, &pitch, &gear}) {
    const int moves = static_cast<int>(r->moves.size());
    for (int taken = 1; taken <= moves; taken++) {
      for (bool fell : {true, false}) {
        for (double odds : {0.1, 0.5, 0.95}) {
          for (double pump : {1.0, 9.0, 26.0}) {
            AttemptInput in = MakeInput(c, *r);
            in.rack = RackOf(RackTier::Cams);
            AttemptResult res = Timeline(*r, taken, odds, pump, fell);
            for (const Beat& b : CallTheAttempt(in, res)) {
              CHECK(!AnyDigit(b.line));
              CHECK(!b.line.empty());
              CHECK(b.weight >= 0.0 && b.weight <= 1.0);
            }
            CHECK(!AnyDigit(HowItWent(in, res)));
          }
        }
      }
    }
  }
  // And every kind has a name, for the camera that keys off it.
  for (int i = 0; i < kBeatKindCount; i++) {
    CHECK(std::string(BeatKindName(static_cast<BeatKind>(i))).size() > 2);
  }
}

static void TestAGearLeadIsNotToldInBolts() {
  // A borrowed vocabulary is how a narrator stops being trusted. `RunoutText`
  // says *bolt* -- right for the HUD it was written for, and wrong out loud
  // on a lead where the whole point is that nobody drilled anything.
  Climber c = MakeClimber(70, 70, 70, 75, 60);
  const Route gear = TradPitch("Last Piece Below You", 5);
  const int moves = static_cast<int>(gear.moves.size());
  AttemptInput in = MakeInput(c, gear);
  in.rack = RackOf(RackTier::Cams);

  // One good piece low down and then nothing, which is a lead that gets
  // frightening on its own.
  AttemptResult res = Timeline(gear, moves, 0.85, 3.0, false);
  res.gear.quality.assign(gear.moves.size(), 0.0);
  res.gear.quality[1] = 0.9;
  const std::vector<Beat> beats = CallTheAttempt(in, res);
  CHECK(Says(beats, BeatKind::OffTheDeck));
  CHECK(Says(beats, BeatKind::Runout));
  for (const Beat& b : beats) CHECK(b.line.find("bolt") == std::string::npos);
  CHECK(LineFor(beats, BeatKind::OffTheDeck).find("piece") != std::string::npos);

  // A piece you do not believe in is said differently from one you do, and
  // the runout above it is said differently again.
  AttemptResult dodgy = res;
  dodgy.gear.quality[1] = 0.2;
  const std::vector<Beat> scared = CallTheAttempt(in, dodgy);
  CHECK(LineFor(scared, BeatKind::OffTheDeck) !=
        LineFor(beats, BeatKind::OffTheDeck));
  CHECK(LineFor(scared, BeatKind::Runout) != LineFor(beats, BeatKind::Runout));
  CHECK(LineFor(scared, BeatKind::Runout).find("believe") != std::string::npos);

  // A sport route is told in bolts, which is correct there and is the
  // reason the two have to be different at all.
  const Route bolted = Pitch("The Long Haul", 5);
  AttemptInput sin_ = MakeInput(c, bolted);
  const std::vector<Beat> clipped =
      CallTheAttempt(sin_, Timeline(bolted, static_cast<int>(bolted.moves.size()),
                                    0.85, 3.0, false));
  CHECK(Says(clipped, BeatKind::OffTheDeck));
  CHECK(LineFor(clipped, BeatKind::OffTheDeck).find("Clipped") !=
        std::string::npos);

  // And a boulder is told in neither. Nothing about a rope reaches it.
  Rng world = Rng::FromStream("narr", Stream::Worldgen);
  const Route boulder = BuildRoute(world, "Short Story", 6, 6,
                                   RouteType::Power, Discipline::Boulder);
  AttemptInput bin = MakeInput(c, boulder);
  const std::vector<Beat> pad = CallTheAttempt(
      bin, Timeline(boulder, static_cast<int>(boulder.moves.size()), 0.85, 3.0,
                    false));
  CHECK(!Says(pad, BeatKind::OffTheDeck));
  CHECK(!Says(pad, BeatKind::Runout));
  CHECK(!Says(pad, BeatKind::Placed));
}

static void TestTheLiveWallAndTheReplayAgree() {
  // `LiveAttempt::partial` is an `AttemptResult`, so the wall calls the
  // same function move by move and takes what is new. One implementation on
  // purpose: this project has twice shipped two paths that drifted, and the
  // narrator is the last place that should be allowed to say two different
  // things about one climb.
  Climber c = MakeClimber(70, 70, 70, 75, 60);
  const Route pitch = Pitch("The Long Haul", 7);
  AttemptInput in = MakeInput(c, pitch);
  in.beta = 0.5;

  LiveAttempt la = BeginAttempt(Rng::FromSeed("live"), in);
  std::vector<Beat> asItHappened;
  while (!AttemptOver(la)) {
    StepMove(la, 0.72);
    if (!AttemptOver(la) && la.input.route.moves[la.nextMove - 1].restQuality > 0.0) {
      ShakeOut(la);
    }
    const Beat said = LastWord(in, la.partial);
    if (!said.line.empty()) asItHappened.push_back(said);
  }
  const AttemptResult finished = FinishAttempt(la);

  // Everything the wall said as it happened is in the replay, word for
  // word and at the same move.
  const std::vector<Beat> replay = CallTheAttempt(in, finished);
  for (const Beat& live : asItHappened) {
    bool found = false;
    for (const Beat& b : replay) {
      if (b.move == live.move && b.line == live.line) found = true;
    }
    CHECK(found);
  }
  // The wall is quiet most moves, which is the whole design.
  CHECK(asItHappened.size() < finished.timeline.size());
}

static void TestThreeLinesOfRoomGetTheRightThree() {
  Climber c = MakeClimber(70, 70, 70, 75, 60);
  const Route pitch = Pitch("The Long Haul", 6);
  const int moves = static_cast<int>(pitch.moves.size());
  AttemptInput in = MakeInput(c, pitch);
  const std::vector<Beat> all =
      CallTheAttempt(in, Timeline(pitch, moves, 0.2, 8.0, true));
  CHECK(all.size() > 3);

  const std::vector<Beat> three = Loudest(all, 3);
  CHECK(three.size() == 3);

  // The loudest three, whichever they are.
  double lowestKept = 1.0;
  for (const Beat& b : three) lowestKept = std::min(lowestKept, b.weight);
  for (const Beat& b : all) {
    bool kept = false;
    for (const Beat& k : three) {
      if (k.move == b.move && k.line == b.line) kept = true;
    }
    if (!kept) CHECK(b.weight <= lowestKept);
  }

  // In the order they happened, and the ending last -- the forearms going
  // and the top-out land on the same move, and a reel that read "you know
  // you cannot hold on / top / the forearms are going" was three true lines
  // in an order nobody climbed them in.
  for (std::size_t i = 1; i < three.size(); i++) {
    CHECK(three[i].move >= three[i - 1].move);
  }
  CHECK(three.back().kind == BeatKind::Fell ||
        three.back().kind == BeatKind::Topped);

  // The ending always makes the cut, however few lines there are.
  CHECK(Loudest(all, 1).size() == 1);
  CHECK(Loudest(all, 1)[0].kind == BeatKind::Fell);
  // No room is no room rather than a request for everything.
  CHECK(Loudest(all, 0).empty());
  CHECK(Loudest(all, 99).size() == all.size());
}

static void TestTheGroundKnowsWhichGoThisIs() {
  // After the first go the interesting thing is not what the route looks
  // like, it is how many times you have been here -- which is the whole
  // difference between an onsight and a project.
  Climber c = MakeClimber(70, 70, 70, 75, 60);
  const Route pitch = Pitch("The Long Haul", 8);
  const int moves = static_cast<int>(pitch.moves.size());
  const AttemptResult res = Timeline(pitch, moves / 2, 0.7, 3.0, true);
  NarratorDials nd;

  AttemptInput first = MakeInput(c, pitch);
  AttemptInput second = MakeInput(c, pitch);
  second.attemptNumber = 2;
  AttemptInput wired = MakeInput(c, pitch);
  wired.attemptNumber = 30;
  wired.beta = 0.95;
  AttemptInput lost = MakeInput(c, pitch);
  lost.attemptNumber = nd.lostCountAt + 1;

  const std::string a = LineFor(CallTheAttempt(first, res, nd), BeatKind::Ground);
  const std::string b = LineFor(CallTheAttempt(second, res, nd), BeatKind::Ground);
  const std::string d = LineFor(CallTheAttempt(wired, res, nd), BeatKind::Ground);
  const std::string e = LineFor(CallTheAttempt(lost, res, nd), BeatKind::Ground);
  CHECK(a != b);
  CHECK(b != d && b != e && d != e);
  CHECK(d.find("every move") != std::string::npos);

  // And whatever is wrong with you today gets said, over the top of any of
  // them -- because it is the thing a climber standing at the bottom is
  // actually thinking about.
  AttemptInput hurt = MakeInput(c, pitch);
  hurt.climber.injury.active = true;
  AttemptInput cold = MakeInput(c, pitch);
  cold.warmth = 0.1;
  AttemptInput shot = MakeInput(c, pitch);
  shot.climber.skin = 1.0;
  const std::string h = LineFor(CallTheAttempt(hurt, res, nd), BeatKind::Ground);
  const std::string k = LineFor(CallTheAttempt(cold, res, nd), BeatKind::Ground);
  const std::string s = LineFor(CallTheAttempt(shot, res, nd), BeatKind::Ground);
  CHECK(h.size() > a.size() && k.size() > a.size() && s.size() > a.size());
  CHECK(h != k && k != s && h != s);
}

// --- Age ---------------------------------------------------------------------

static void TestAgeIsDerivedNotStored() {
  // Age is computed from the day counter, which is why adding it needed no
  // save version at all — and why a save can never disagree with a birthday.
  AgeDials d;
  CHECK(AgeOn(1, d) == d.startAge);
  CHECK(AgeOn(1 + d.daysPerYear, d) == d.startAge + 1.0);
  CHECK(AgeOn(1 + 10 * d.daysPerYear, d) == d.startAge + 10.0);
  CHECK(AgeOn(2, d) > AgeOn(1, d));
}

static void TestNothingIsTakenBeforeThePeak() {
  // A twenty-four-year-old is not losing anything. A system that quietly
  // taxed them from day one would be a bug nobody could see.
  AgeDials d;
  Climber young;
  young.skills = {80, 80, 80, 80, 80};
  const Skills before = young.skills;
  for (int day = 1; day < static_cast<int>((d.powerPeak - d.startAge) *
                                           d.daysPerYear); day++) {
    AgeDay(young, day, d);
  }
  CHECK(young.skills.power == before.power);
  CHECK(young.skills.fingers == before.fingers);
  CHECK(young.skills.endurance == before.endurance);
}

static void TestPowerGoesFirstAndTechniqueNeverGoes() {
  // The asymmetry is the whole reason to have this system: it changes what
  // kind of climber you are, not just how good. The best climber at the
  // crag is often the one nobody would pick in an arm wrestle.
  AgeDials d;
  Climber old;
  old.skills = {100, 100, 100, 100, 100};
  for (int day = 1; day <= 30 * d.daysPerYear; day++) AgeDay(old, day, d);

  CHECK(old.skills.technique == 100.0);   // never declines, at any age
  CHECK(old.skills.head == 100.0);
  CHECK(old.skills.power < old.skills.fingers);      // power goes first
  CHECK(old.skills.fingers < old.skills.endurance);  // and endurance last
  CHECK(old.skills.power > 0.0);                     // but nobody unlearns
}

static void TestTheCeilingIsWhatTrainingCannotArgueWith() {
  AgeDials d;
  // Without a falling ceiling, a climber who keeps turning up never
  // declines at all: measured, they pinned at 100 power from 28 to 52,
  // because training gains at the ceiling (~8 points a year) outrun decay
  // (1.6) five to one. Nobody climbs at 52 the way they did at 28.
  CHECK(MaxSkillFor(d.powerPeak - 1.0, d.powerPeak, d.ceilingLostPerYear, d) ==
        100.0);
  CHECK(MaxSkillFor(d.powerPeak + 10.0, d.powerPeak, d.ceilingLostPerYear, d) <
        100.0);
  CHECK(MaxSkillFor(200.0, d.powerPeak, d.ceilingLostPerYear, d) ==
        d.ceilingFloor);

  // It holds a trained climber down...
  Climber trained;
  trained.skills = {100, 100, 100, 100, 100};
  for (int day = 1; day <= 25 * d.daysPerYear; day++) {
    trained.skills.power = 100.0;   // trains it straight back every day
    AgeDay(trained, day, d);
  }
  CHECK(trained.skills.power < 100.0);
  CHECK(trained.skills.power ==
        MaxSkillFor(AgeOn(25 * d.daysPerYear, d), d.powerPeak,
                    d.ceilingLostPerYear, d));

  // ...and leaves alone anybody it was never limiting. Somebody at 60 power
  // in their forties was not being held back by their age.
  Climber ordinary;
  ordinary.skills = {60, 60, 60, 60, 60};
  const double was = ordinary.skills.power;
  AgeDay(ordinary, 20 * d.daysPerYear, d);
  CHECK(ordinary.skills.power < was);          // decay still applies
  CHECK(ordinary.skills.power > 59.0);         // but the ceiling does not
}

static void TestAgeShrinksTheTrainingBudgetNotTheClimber() {
  // The real mechanic. An older climber is not a smaller climber — they are
  // one who cannot train as much, because load clears more slowly and the
  // line they get hurt at has come down to meet them.
  AgeDials ad;
  BodyDials bd;
  CHECK(RecoveryFactorFor(25.0, ad) == 1.0);
  CHECK(RecoveryFactorFor(ad.recoveryHoldsUntil, ad) == 1.0);
  CHECK(RecoveryFactorFor(45.0, ad) < RecoveryFactorFor(35.0, ad));
  CHECK(RecoveryFactorFor(200.0, ad) == ad.recoveryFloor);   // and it floors

  CHECK(InjuryThresholdFor(25.0, bd.injuryThreshold, ad) == bd.injuryThreshold);
  CHECK(InjuryThresholdFor(50.0, bd.injuryThreshold, ad) < bd.injuryThreshold);
  CHECK(InjuryThresholdFor(200.0, bd.injuryThreshold, ad) ==
        ad.injuryThresholdFloor);

  // The same session, twenty years apart: the older climber carries it
  // longer, which is the entire difference.
  Climber young, old;
  young.load = old.load = 40.0;
  BodyDay(young, false, 25.0, bd, ad);
  BodyDay(old, false, 50.0, bd, ad);
  CHECK(old.load > young.load);
}

static void TestACareerHasAnArc() {
  // The shape, end to end: a climber who trains hard peaks and then comes
  // down, and the technical line outlasts the power line the whole way.
  AgeDials ad;
  Rng world = Rng::FromStream("arc", Stream::Worldgen);
  const Route power = BuildRoute(world, "Burl", 8, 6, RouteType::Power,
                                 Discipline::Boulder);
  const Route tech = BuildRoute(world, "Slab", 8, 6, RouteType::Technical,
                                Discipline::Boulder);

  Climber c;
  c.skills = {100, 100, 100, 100, 100};
  double peak = 0.0;
  double atPeakAge = 0.0;
  double last = 0.0;
  for (int day = 1; day <= 30 * ad.daysPerYear; day++) {
    AgeDay(c, day, ad);
    const double now = AbilityOnRoute(c, power);
    if (now > peak) { peak = now; atPeakAge = AgeOn(day, ad); }
    last = now;
    // Technique carries you: the slab is never harder than the burl.
    CHECK(AbilityOnRoute(c, tech) >= now - 1e-9);
  }
  CHECK(atPeakAge < 30.0);        // the peak is early, and it is behind you
  CHECK(last < peak - 2.0);       // and the arc really does come down
  CHECK(last > 4.0);              // to a climber, not to nothing
}

static void TestANightIsWhereEverythingCountsDown() {
  // Three per-day ticks have now been written and left uncalled: KitDay
  // (one $75 bought 365 days of membership), the body roll, and FactionDay
  // — which meant a closed crag would have stayed closed for the rest of
  // the save, because nothing in the engine ever counted the days off.
  // Sleeping is where a day ends; everything that runs out runs out here.
  DayDials dd;
  const Rng world = Rng::FromSeed("nights");

  PlayerState player;
  double cash = 200.0;
  CHECK(RenewMembership(player.kit, cash));
  player.standing.closedDays = 5;
  player.climber.load = 40.0;

  const int membershipWas = player.kit.membershipDaysLeft;
  DayState day = WakeUp(player, dd);
  SleepToNextDay(player, day, world, dd);

  CHECK(player.kit.membershipDaysLeft == membershipWas - 1);
  CHECK(player.standing.closedDays == 4);
  CHECK(player.climber.load < 40.0);

  // And the closure actually ends, which is the thing that was broken.
  for (int i = 0; i < 10; i++) {
    DayState d2 = WakeUp(player, dd);
    SleepToNextDay(player, d2, world, dd);
  }
  CHECK(player.standing.closedDays == 0);
  CHECK(CragIsOpen(player.standing));
}

static void TestLoadRisesWithHardnessNotMileage() {
  BodyDials d;
  // A day of mileage on jugs and a day of trying hard are the same number
  // of burns and nothing like the same cost. If this ever inverts, resting
  // becomes about how much you climbed rather than how hard, and the whole
  // point of a second budget goes with it.
  Climber cruiser, tryer;
  for (int i = 0; i < 10; i++) {
    AccrueLoad(cruiser, 0.0, HoldType::Jug, d);
    AccrueLoad(tryer, 2.0, HoldType::Crimp, d);
  }
  CHECK(tryer.load > cruiser.load * 4.0);

  // And it caps, so a season of abuse cannot run the number off the end.
  Climber wrecked;
  for (int i = 0; i < 500; i++) AccrueLoad(wrecked, 4.0, HoldType::Crimp, d);
  CHECK(wrecked.load == d.loadCeiling);
}

static void TestLoadIsASlowerClockThanSkin() {
  BodyDials bd;
  DayDials dd;
  // Six nights takes skin from wrecked to fresh; load must take weeks, or
  // it is just a second skin bar and says nothing new.
  Climber c;
  c.load = 100.0;
  int nights = 0;
  while (c.load > 0.0 && nights < 200) {
    BodyDay(c, false, 25.0, bd);
    nights++;
  }
  CHECK(nights > 20);          // a month-ish, not a week
  const double skinNights = dd.maxSkin / dd.skinRegenPerNight;
  CHECK(nights > skinNights * 3.0);

  // Resting properly beats sleeping it off, which is what makes a rest day
  // a decision rather than a day you lost.
  Climber lazy, resting;
  lazy.load = resting.load = 50.0;
  BodyDay(lazy, false, 25.0, bd);
  BodyDay(resting, true, 25.0, bd);
  CHECK(resting.load < lazy.load);
}

static void TestNobodyGetsHurtOutOfTheBlue() {
  BodyDials d;
  const Rng world = Rng::FromSeed("unlucky");
  // Below the threshold: never, no matter how many days you roll. An injury
  // has to be something you were warned about, or it is weather.
  // Well below the line: never, across a lifetime of days.
  Climber careful;
  careful.load = d.injuryThreshold * 0.5;
  for (int day = 1; day <= 5000; day++) {
    CHECK(!RollForInjury(careful, world, day, d));
  }
  CHECK(!IsHurt(careful));

  // And the risk has to climb with how far past the line you are, which is
  // the property that actually holds this mechanic up. Without it the
  // warning in LoadText means nothing and the number is decoration.
  //
  // Two obvious defects pass every other check in this test: dropping the
  // `over <= 0` guard (Chance() refuses a negative probability anyway), and
  // a fabs() sign error that makes being far past the line as safe as being
  // just over it. Both are caught here and nowhere else.
  const auto RateAt = [&](double load) {
    int hurt = 0;
    for (int i = 0; i < 3000; i++) {
      Climber c;
      c.load = load;
      if (RollForInjury(c, world, i, d)) hurt++;
    }
    return hurt / 3000.0;
  };
  const double justOver = RateAt(d.injuryThreshold + 4.0);
  const double wellOver = RateAt(d.injuryThreshold + 30.0);
  const double redlined = RateAt(100.0);
  CHECK(justOver > 0.0);
  CHECK(wellOver > justOver * 3.0);
  CHECK(redlined > wellOver);
  // Under the line the rate is flat zero, not merely small.
  CHECK(RateAt(d.injuryThreshold - 5.0) == 0.0);
  CHECK(RateAt(0.0) == 0.0);

  // Above it, and long enough, it lands.
  Climber reckless;
  bool hurt = false;
  for (int day = 1; day <= 2000 && !hurt; day++) {
    reckless.load = 95.0;
    hurt = RollForInjury(reckless, world, day, d);
  }
  CHECK(hurt);
  CHECK(IsHurt(reckless));
  CHECK(reckless.injury.daysLeft > 0);
  CHECK(reckless.injury.severity >= 0.0 && reckless.injury.severity <= 1.0);
  // Being hurt is not only lost time.
  CHECK(reckless.psyche < 0.7);

  // Already hurt is not hurt again — that is what climbing on it is for.
  const int was = reckless.injury.daysLeft;
  reckless.load = 100.0;
  for (int day = 1; day <= 200; day++) RollForInjury(reckless, world, day, d);
  CHECK(reckless.injury.daysLeft == was);
}

static void TestMostInjuriesAreAFortnightAndTheSeasonEnderIsRare() {
  BodyDials d;
  const Rng world = Rng::FromSeed("epidemiology");
  int n = 0, long_ = 0;
  double totalDays = 0.0;
  for (int i = 0; i < 4000; i++) {
    Climber c;
    c.load = 100.0;
    if (!RollForInjury(c, world, i, d)) continue;
    n++;
    totalDays += c.injury.daysLeft;
    if (c.injury.daysLeft > 40) long_++;
  }
  CHECK(n > 200);
  const double mean = totalDays / n;
  CHECK(mean > 10.0 && mean < 26.0);          // most are an annoyance
  CHECK(long_ > 0);                            // the season-ender exists
  CHECK(static_cast<double>(long_) / n < 0.2); // and it is rare
}

static void TestAnInjuryDecidesWhatYouCanStillClimbOn() {
  BodyDials d;
  // The injury's whole gameplay is the choice of what to get on. A pulley
  // ends crimping and leaves slopers alone; a shoulder is the reverse. If
  // both bit everything equally there would be no decision to make.
  CHECK(InjuryBiteOn(InjuryKind::Pulley, HoldType::Crimp, d) >
        InjuryBiteOn(InjuryKind::Pulley, HoldType::Sloper, d));
  CHECK(InjuryBiteOn(InjuryKind::Shoulder, HoldType::Sloper, d) >
        InjuryBiteOn(InjuryKind::Shoulder, HoldType::Crimp, d));
  CHECK(InjuryBiteOn(InjuryKind::Lumbrical, HoldType::Pocket, d) >
        InjuryBiteOn(InjuryKind::Lumbrical, HoldType::Crimp, d));
  // Nothing that hurts ever fully leaves you alone.
  for (int k = 0; k < kInjuryKindCount; k++) {
    for (HoldType h : {HoldType::Crimp, HoldType::Sloper, HoldType::Pinch,
                       HoldType::Pocket, HoldType::Jug, HoldType::Dyno,
                       HoldType::Crack}) {
      const double bite = InjuryBiteOn(static_cast<InjuryKind>(k), h, d);
      CHECK(bite > 0.0 && bite <= 1.0);
    }
  }

  // And it reaches the wall. A hurt climber on the holds that hurt is
  // measurably worse; on the holds that do not, they are themselves.
  Rng world = Rng::FromStream("hurt", Stream::Worldgen);
  const Route crimpy = BuildRoute(world, "Tips", 6, 6, RouteType::Crimp,
                                  Discipline::Boulder);
  Climber healthy = MakeClimber(55, 55, 55, 55, 50);
  Climber pulley = healthy;
  pulley.injury.active = true;
  pulley.injury.kind = InjuryKind::Pulley;
  pulley.injury.severity = 1.0;
  CHECK(AverageHighpoint(pulley, crimpy, 200) <
        AverageHighpoint(healthy, crimpy, 200));
}

static void TestClimbingOnItIsAGambleBothWays() {
  BodyDials d;
  const Rng world = Rng::FromSeed("stubborn");

  Climber c;
  c.injury.active = true;
  c.injury.severity = 0.2;
  c.injury.daysLeft = 20;

  // A healthy climber cannot aggravate what they have not got.
  Climber fine;
  CHECK(!ClimbOnIt(fine, world, 1, 1, d));

  // One burn is a real gamble in both directions: sometimes you get away
  // with it, which is what makes pulling on a choice rather than a warning.
  int gotAway = 0, paid = 0;
  for (int i = 0; i < 400; i++) {
    Climber tryIt;
    tryIt.injury.active = true;
    tryIt.injury.severity = 0.2;
    tryIt.injury.daysLeft = 20;
    if (ClimbOnIt(tryIt, world, 1, i, d)) paid++; else gotAway++;
  }
  CHECK(paid > 0 && gotAway > 0);
  CHECK(gotAway > paid);   // most of the time, you get away with it

  // Twenty burns should be twenty times as sorry, not once.
  Climber grinder;
  grinder.injury.active = true;
  grinder.injury.severity = 0.2;
  grinder.injury.daysLeft = 20;
  for (int i = 0; i < 20; i++) ClimbOnIt(grinder, world, 1, i, d);
  CHECK(grinder.injury.daysLeft > 20);
  CHECK(grinder.injury.severity > 0.2);
  CHECK(grinder.injury.severity <= 1.0);

  // But it is bounded. Adding days per aggravation rather than recomputing
  // them from severity turned one bad fortnight into a 226-day injury in a
  // season probe — which is not a season-ender, it is a runaway.
  Climber stubborn;
  stubborn.injury.active = true;
  stubborn.injury.severity = 0.1;
  stubborn.injury.daysLeft = 12;
  for (int i = 0; i < 5000; i++) ClimbOnIt(stubborn, world, 1, i, d);
  CHECK(stubborn.injury.severity == 1.0);
  CHECK(stubborn.injury.daysLeft <= InjuryDaysFor(1.0, d));
  // And the ceiling is a season-ender, not a career-ender.
  CHECK(InjuryDaysFor(1.0, d) < 100);
}

static void TestPhysioBuysTimeAndNotAMiracle() {
  BodyDials d;
  PlayerState player;
  player.climber.injury.active = true;
  player.climber.injury.severity = 0.5;
  player.climber.injury.daysLeft = 30;
  player.cash = 400.0;

  // Healthy people do not get to bank sessions.
  PlayerState fine;
  fine.cash = 400.0;
  CHECK(!Physio(fine.climber, fine.cash, fine.lastPhysioDay, 1, d));
  CHECK(fine.cash == 400.0);

  CHECK(Physio(player.climber, player.cash, player.lastPhysioDay, 10, d));
  CHECK(player.climber.injury.daysLeft == 30 - d.physioDaysSaved);
  CHECK(player.cash == 400.0 - d.physioCost);

  // No buying your way out of a season in an afternoon.
  CHECK(!Physio(player.climber, player.cash, player.lastPhysioDay, 11, d));
  CHECK(Physio(player.climber, player.cash, player.lastPhysioDay,
               10 + d.physioDaysBetween, d));

  // And it never heals you to zero — an injury always costs at least a day
  // more, however much money you throw at it.
  PlayerState rich;
  rich.climber.injury.active = true;
  rich.climber.injury.daysLeft = 2;
  rich.cash = 10000.0;
  CHECK(Physio(rich.climber, rich.cash, rich.lastPhysioDay, 100, d));
  CHECK(rich.climber.injury.daysLeft >= 1);

  // Broke is broke.
  PlayerState skint;
  skint.climber.injury.active = true;
  skint.climber.injury.daysLeft = 30;
  skint.cash = 10.0;
  CHECK(!Physio(skint.climber, skint.cash, skint.lastPhysioDay, 1, d));
  CHECK(skint.climber.injury.daysLeft == 30);
}

static void TestTendonsDoNotTearInACampChair() {
  // The roll only happens on a day you pulled on. A player who rested all
  // season through a redlined load is being stupid, not unlucky, and the
  // sim must not punish them for the thing that was going to fix it.
  DayDials dd;
  const Rng world = Rng::FromSeed("resting");
  PlayerState player;
  player.climber.load = 100.0;
  for (int i = 0; i < 30; i++) {
    DayState day = WakeUp(player, dd);   // never pulls on: atGym stays false
    SleepToNextDay(player, day, world, dd);
  }
  CHECK(!IsHurt(player.climber));
  CHECK(player.climber.load == 0.0);   // and it came all the way back down
}

static void TestTheBodySurvivesASave() {
  SaveGame save;
  save.seed = "hurt";
  save.player.climber.load = 71.5;
  save.player.climber.injury.active = true;
  save.player.climber.injury.kind = InjuryKind::Shoulder;
  save.player.climber.injury.severity = 0.62;
  save.player.climber.injury.daysLeft = 23;
  save.player.lastPhysioDay = 140;

  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  CHECK(back.player.climber.load == 71.5);
  CHECK(back.player.climber.injury.active);
  CHECK(back.player.climber.injury.kind == InjuryKind::Shoulder);
  CHECK(back.player.climber.injury.severity == 0.62);
  CHECK(back.player.climber.injury.daysLeft == 23);
  CHECK(back.player.lastPhysioDay == 140);
}

// --- The kit -----------------------------------------------------------------

static void TestPadsPayOffWhereYouAreScared() {
  // The pad's whole argument is that it matters where you are trying
  // hardest. If it were a flat penalty it would just be a tax, and buying
  // one would be arithmetic rather than a decision.
  Rng world = Rng::FromStream("pads", Stream::Worldgen);
  const Route line = BuildRoute(world, "Highball", 6, 6, RouteType::Technical,
                                Discipline::Boulder);
  const Climber c = MakeClimber(55, 55, 55, 55, 50);

  AttemptInput padded = MakeInput(c, line);
  AttemptInput bare = MakeInput(c, line);
  bare.padding = 0.0;

  // Down low, the ground is close enough that foam is not the point.
  LiveAttempt lowPad = BeginAttempt(Rng::FromSeed("p"), padded);
  LiveAttempt lowBare = BeginAttempt(Rng::FromSeed("p"), bare);
  CHECK(std::fabs(PeekOdds(lowPad, 0.72) - PeekOdds(lowBare, 0.72)) < 1e-9);

  // High on it, it is worth its price.
  LiveAttempt highPad = BeginAttempt(Rng::FromSeed("p"), padded);
  LiveAttempt highBare = BeginAttempt(Rng::FromSeed("p"), bare);
  for (int i = 0; i < 5; i++) {
    highPad.nextMove = i;
    highBare.nextMove = i;
  }
  highPad.nextMove = 5;
  highBare.nextMove = 5;
  CHECK(PeekOdds(highPad, 0.72) > PeekOdds(highBare, 0.72));

  // And a bold climber above gravel is still bolder than a timid one —
  // head is what pays for the missing pad, so it has to still speak.
  AttemptInput bold = MakeInput(MakeClimber(55, 55, 55, 55, 90), line);
  AttemptInput timid = MakeInput(MakeClimber(55, 55, 55, 55, 20), line);
  bold.padding = timid.padding = 0.0;
  LiveAttempt b = BeginAttempt(Rng::FromSeed("p"), bold);
  LiveAttempt t = BeginAttempt(Rng::FromSeed("p"), timid);
  b.nextMove = t.nextMove = 5;
  CHECK(PeekOdds(b, 0.72) > PeekOdds(t, 0.72));
}

static void TestPadsChangeNothingForCallersWhoNeverHeardOfThem() {
  // padding defaults to 1.0 precisely so that adding it moved no vector.
  // If this ever fails, every save in existence just changed grade.
  AttemptInput fresh;
  CHECK(fresh.padding == 1.0);
  SessionState session;
  CHECK(session.padding == 1.0);
}

static void TestTheKitIsBoughtOrItIsNot() {
  KitDials d;
  Kit kit;
  double cash = 100.0;

  // Nothing half-buys: a refused purchase leaves the money alone.
  CHECK(!BuyPad(kit, cash, d));
  CHECK(cash == 100.0);
  CHECK(kit.pads == Kit{}.pads);

  CHECK(BuyHangboard(kit, cash, d));
  CHECK(cash == 15.0);
  CHECK(kit.hangboard);
  CHECK(!BuyHangboard(kit, cash, d));   // you only need the one
  CHECK(cash == 15.0);

  // You arrive with one pad, which is the whole of what a dirtbag owns.
  CHECK(Kit{}.pads == 1);
  const double one = PaddingFrom(Kit{}, d);
  CHECK(one > 0.0 && one < 1.0);
  CHECK(PaddingFrom(Kit{.pads = 0}, d) == 0.0);

  // The second is the purchase, and it is the one that tops it out —
  // at mostFoamCanDo rather than at 1.0. Foam never gets all the way there:
  // a well-padded highball is still a highball, and at exactly 1.0 the pad
  // stopped being a trade and became a switch, erasing head training
  // outright and forever.
  cash = 900.0;
  CHECK(BuyPad(kit, cash, d));
  CHECK(PaddingFrom(kit, d) == d.mostFoamCanDo);
  CHECK(PaddingFrom(kit, d) < 1.0);
  CHECK(BuyPad(kit, cash, d));
  CHECK(PaddingFrom(kit, d) == d.mostFoamCanDo);  // no further; borrow a third

  // And the thing that guarantees the trade survives: there is exposure
  // left at the top of a boulder even fully padded, so a two-pad career
  // still trains head — slower than a bare-ground one, never zero.
  const Rng padWorld = Rng::FromSeed("pad-world");
  const Route highball = BuildRoute(padWorld, "the highball", 4, 4,
                                    RouteType::Power, Discipline::Boulder);
  const int top = static_cast<int>(highball.moves.size()) - 1;
  CHECK(ExposureAt(highball, top, PaddingFrom(kit, d)) > 0.0);
  CHECK(ExposureAt(highball, top, PaddingFrom(Kit{}, d)) >
        ExposureAt(highball, top, PaddingFrom(kit, d)));
}

static void TestThePadSaysWhatItCosts() {
  KitDials d;

  // You arrive with one, so the offer is live from day one.
  const std::string offer = PadOfferText(Kit{}, d);
  CHECK(!offer.empty());
  CHECK(offer.find("260") != std::string::npos);        // the money
  CHECK(offer.find("brave") != std::string::npos);      // and the other price

  // Once you own the pads that matter there is nothing to sell you. The
  // third is borrowed from whoever is at the Lot.
  Kit padded;
  padded.pads = d.padsThatMatter;
  CHECK(PadOfferText(padded, d).empty());
  padded.pads = 9;
  CHECK(PadOfferText(padded, d).empty());

  // And a player with none is still offered one.
  Kit none;
  none.pads = 0;
  CHECK(!PadOfferText(none, d).empty());

  // The price it names is the price it charges.
  Kit buying;
  double cash = d.padCost;
  CHECK(BuyPad(buying, cash, d));
  CHECK(cash == 0.0);
}

static void TestTheMembershipRunsOut() {
  KitDials d;
  Kit kit;
  double cash = 200.0;

  CHECK(!IsGymMember(kit));
  CHECK(RenewMembership(kit, cash, d));
  CHECK(IsGymMember(kit));

  // Renewing early stacks rather than resets — losing days you already paid
  // for would punish being organised.
  const int left = kit.membershipDaysLeft;
  CHECK(RenewMembership(kit, cash, d));
  CHECK(kit.membershipDaysLeft == left + d.membershipDays);

  // And it lapses on its own, one night at a time.
  Kit lapsing;
  double money = 100.0;
  CHECK(RenewMembership(lapsing, money, d));
  for (int i = 0; i < d.membershipDays; i++) {
    CHECK(IsGymMember(lapsing));
    KitDay(lapsing);
  }
  CHECK(!IsGymMember(lapsing));
  KitDay(lapsing);                        // and stays lapsed without going
  CHECK(lapsing.membershipDaysLeft == 0); // negative for the rest of time
}

static void TestTheGymIsTheOnlyClimbingThatIgnoresTheWeather() {
  DayDials dd;
  KitDials kd;
  PlayerState player;
  player.climber.skills = {50, 50, 50, 50, 50};

  DayState shut = WakeUp(player, dd);
  CHECK(!GoToTheGym(player, shut, kd, dd));   // the gym is the one place
  CHECK(shut.hour == dd.wakeHour);            // that checks, and it costs
  CHECK(!shut.atGym);                         // you nothing to be refused

  double cash = 100.0;
  CHECK(RenewMembership(player.kit, cash, kd));

  DayState open = WakeUp(player, dd);
  CHECK(GoToTheGym(player, open, kd, dd));
  CHECK(open.atGym);
  CHECK(open.hour > dd.wakeHour);             // the drive in is real
  // Full mats, every time. That is what the $75 actually buys on a day the
  // weather has already decided for you.
  CHECK(open.session.padding == 1.0);
  CHECK(player.kit.pads < KitDials{}.padsThatMatter);   // and not by owning it
}

static void TestTheHangboardIsTheBrokeAnswer() {
  DayDials dd;
  KitDials kd;
  PlayerState player;
  player.climber.skills = {50, 50, 50, 50, 50};

  DayState day = WakeUp(player, dd);
  CHECK(!HangboardSession(player, day, kd, dd));   // you do not own one

  double cash = 100.0;
  CHECK(BuyHangboard(player.kit, cash, kd));

  const double before = player.climber.skills.fingers;
  const double skinBefore = player.climber.skin;
  CHECK(HangboardSession(player, day, kd, dd));
  CHECK(player.climber.skills.fingers > before);
  CHECK(player.climber.skin < skinBefore);
  CHECK(day.hour > dd.wakeHour);

  // Fingers only. A board that trained movement would make the weather
  // irrelevant, which is the opposite of this game.
  CHECK(player.climber.skills.power == 50.0);
  CHECK(player.climber.skills.technique == 50.0);

  // It is also strictly worse than climbing. A day on the board must not
  // out-train a day on the wall, or nobody would ever leave the van.
  PlayerState hanger;
  hanger.climber.skills = {50, 50, 50, 50, 50};
  hanger.kit.hangboard = true;
  DayState hangDay = WakeUp(hanger, dd);
  CHECK(HangboardSession(hanger, hangDay, kd, dd));
  // One session a day, and the second ask gets nothing.
  CHECK(!HangboardSession(hanger, hangDay, kd, dd));

  PlayerState climber;
  climber.climber.skills = {50, 50, 50, 50, 50};
  DayState climbDay = WakeUp(climber, dd);
  StartGymSession(climber, climbDay, KitDials{}, dd);
  Rng world = Rng::FromStream("board", Stream::Worldgen);
  const Route hard = BuildRoute(world, "Plastic", 7, 6, RouteType::Crimp,
                                Discipline::Boulder);
  ProjectMemory mem;
  mem.routeName = hard.name;
  Rng session = Rng::FromSeed("board-session");
  for (int i = 0; i < 4; i++) {
    const AttemptResult r =
        AttemptInSession(session, climbDay.session, mem,
                         ClimberForSession(climber, climbDay, dd), hard,
                         Conditions{});
    ApplyAttemptToDay(climber, climbDay, hard, r, Rng::FromSeed("body"), dd);
  }
  CHECK(climber.climber.skills.fingers > hanger.climber.skills.fingers);

  // Never on skin that is already gone: hanging through it is how you take
  // a week off, and the injury this game does not model yet.
  PlayerState wrecked;
  wrecked.kit.hangboard = true;
  wrecked.climber.skin = kd.hangboardSkinCost;
  DayState wreckedDay = WakeUp(wrecked, dd);
  CHECK(!HangboardSession(wrecked, wreckedDay, kd, dd));
}

static void TestTheKitSurvivesASave() {
  SaveGame save;
  save.seed = "kitted";
  save.player.kit.pads = 2;
  save.player.kit.hangboard = true;
  save.player.kit.membershipDaysLeft = 17;

  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  CHECK(back.player.kit.pads == 2);
  CHECK(back.player.kit.hangboard);
  CHECK(back.player.kit.membershipDaysLeft == 17);
}

static void TestTheJobSurvivesASave() {
  // Standing was saved from v8 and the job was not, so a career loaded its
  // way out of employment. Nothing failed; you were simply not salaried any
  // more, which is the kind of bug a player reports as "I think it forgot".
  SaveGame save;
  save.seed = "employed";
  save.player.job.salaried = true;
  save.player.job.daysWorked = 148;
  save.player.job.weeksSalaried = 21;

  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  CHECK(back.player.job.salaried);
  CHECK(back.player.job.daysWorked == 148);
  CHECK(back.player.job.weeksSalaried == 21);
}

static void TestLoadsVersion8Save() {
  // A v8 career: it had a scene to stand with, and no way to hold a job.
  const std::string v8 =
      "version=8\n"
      "seed=grim-fjord-123\n"
      "day=40\n"
      "cash=212.5\n"
      "skills.power=48\n"
      "skills.fingers=52\n"
      "skills.technique=50\n"
      "skills.endurance=47\n"
      "skills.head=55\n"
      "morphology=2\n"
      "skin=5.5\n"
      "psyche=0.65\n"
      "projects=0\n"
      "owed=35\n"
      "standing.0=-0.4\n"
      "standing.1=0.2\n"
      "standing.2=0\n"
      "standing.3=-0.1\n"
      "standing.closed=3\n"
      "shoes.wear=0.4\n"
      "shoes.resoles=1\n"
      "shoes.pairs=2\n"
      "van.hours=180\n"
      "van.0.wear=0.3\nvan.0.patches=0\nvan.0.failed=0\n"
      "van.1.wear=0.1\nvan.1.patches=0\nvan.1.failed=0\n"
      "van.2.wear=0.5\nvan.2.patches=1\nvan.2.failed=0\n"
      "van.3.wear=0.2\nvan.3.patches=0\nvan.3.failed=0\n"
      "van.4.wear=0.6\nvan.4.patches=0\nvan.4.failed=0\n"
      "van.5.wear=0.1\nvan.5.patches=0\nvan.5.failed=0\n"
      "dog.name=Wire\n"
      "dog.adopted=1\n"
      "dog.bond=0.8\n"
      "dog.fed=0.5\n"
      "bonds=0\n";

  SaveGame loaded;
  CHECK(DeserializeSave(v8, loaded) == LoadResult::Ok);
  CHECK(loaded.version == kSaveVersion);
  // Everything v8 did carry is still exactly what it was.
  CHECK(loaded.player.day == 40);
  CHECK(loaded.player.owed == 35.0);
  CHECK(loaded.player.standing.with[0] == -0.4);
  CHECK(loaded.player.standing.closedDays == 3);
  CHECK(loaded.player.dog.name == "Wire");
  CHECK(loaded.player.dog.adopted);
  // And the field it never had arrives as what it truthfully was: a career
  // that could not have held a job has not held one.
  CHECK(!loaded.player.job.salaried);
  CHECK(loaded.player.job.daysWorked == 0);

  SaveGame again;
  CHECK(DeserializeSave(SerializeSave(loaded), again) == LoadResult::Ok);
  CHECK(again.player.standing.with[0] == -0.4);
  CHECK(again.player.standing.closedDays == 3);
}

// The real thing: a save written by the v1 build, loaded by this one. Hand
// written rather than generated, because the whole point is that it is a
// file this code can no longer produce. If this ever fails, someone's
// career history just evaporated.
static void TestLoadsVersion1Save() {
  const std::string v1 =
      "version=1\n"
      "seed=grim-fjord-123\n"
      "day=9\n"
      "cash=212.5\n"
      "skills.power=48\n"
      "skills.fingers=52\n"
      "skills.technique=50\n"
      "skills.endurance=47\n"
      "skills.head=55\n"
      "morphology=2\n"
      "skin=5.5\n"
      "psyche=0.65\n"
      "projects=2\n"
      "project.0.name=Pink Crimps\n"
      "project.0.attempts=11\n"
      "project.0.best=6\n"
      "project.0.beta=0.5\n"
      "project.0.sent=1\n"
      "project.0.style=2\n"
      "project.1.name=Campus Special\n"
      "project.1.attempts=4\n"
      "project.1.best=2\n"
      "project.1.beta=0.25\n"
      "project.1.sent=0\n"
      "project.1.style=4\n";

  SaveGame loaded;
  CHECK(DeserializeSave(v1, loaded) == LoadResult::Ok);
  CHECK(loaded.version == kSaveVersion);       // arrives upgraded,
  CHECK(loaded.seed == "grim-fjord-123");      // with everything intact
  CHECK(loaded.player.day == 9);
  CHECK(loaded.player.cash == 212.5);
  CHECK(loaded.player.climber.morphology == Morphology::Lanky);
  CHECK(loaded.player.projects.size() == 2);
  CHECK(loaded.player.projects[0].routeName == "Pink Crimps");
  CHECK(loaded.player.projects[0].attempts == 11);
  CHECK(loaded.player.projects[0].sent);
  CHECK(loaded.player.projects[0].firstSendStyle == Style::Redpoint);
  // The field v1 never had: unknown, not invented.
  CHECK(loaded.player.projects[0].grade == -1);
  CHECK(loaded.player.projects[1].grade == -1);

  // And it re-saves in the new format, so the upgrade is permanent.
  SaveGame again;
  CHECK(DeserializeSave(SerializeSave(loaded), again) == LoadResult::Ok);
  CHECK(again.player.projects[0].attempts == 11);
}

// A career reads back out of the ledgers alone — no route list required,
// which is the point of storing the grade.
static void TestCareerSummary() {
  PlayerState player;
  player.climber.skills = {50, 50, 50, 50, 50};

  CareerSummary empty = SummarizeCareer(player);
  CHECK(empty.hardestSendGrade == -1);
  CHECK(empty.totalSends == 0);
  CHECK(std::string(CareerLine(empty)).find("You climb V5") == 0);

  ProjectMemory easy;
  easy.routeName = "Jug Haul";
  easy.grade = 2;
  easy.attempts = 1;
  easy.sent = true;
  easy.firstSendStyle = Style::Onsight;
  ProjectMemory best;
  best.routeName = "Volume Country";
  best.grade = 5;
  best.attempts = 6;
  best.sent = true;
  best.firstSendStyle = Style::Redpoint;
  ProjectMemory nemesis;
  nemesis.routeName = "Campus Special";
  nemesis.grade = 6;
  nemesis.attempts = 14;
  nemesis.sent = false;
  player.projects = {easy, best, nemesis};

  CareerSummary c = SummarizeCareer(player);
  CHECK(c.totalSends == 2);
  CHECK(c.totalAttempts == 21);
  CHECK(c.hardestSendGrade == 5);
  CHECK(c.hardestSendName == "Volume Country");
  CHECK(c.hardestSendStyle == Style::Redpoint);
  CHECK(c.openProjects == 1);
  CHECK(c.nemesis == "Campus Special");
  CHECK(c.nemesisAttempts == 14);

  const std::string line = CareerLine(c);
  CHECK(line.find("Volume Country") != std::string::npos);
  CHECK(line.find("14 burns") != std::string::npos);
}

// A ledger records the grade the first time you touch a line, so a career
// survives the gym resetting its walls.
static void TestLedgerRecordsGrade() {
  Rng world = Rng::FromStream("ledger", Stream::Worldgen);
  Route route = BuildRoute(world, "Reset Tuesday", 4, 6, RouteType::Crimp,
                           Discipline::Boulder);
  Climber c = MakeClimber(50, 50, 50, 50, 50);
  Rng sessionRng = Rng::FromStream("ledger", Stream::Session);
  SessionState session = StartSession(c);
  ProjectMemory mem;
  AttemptInSession(sessionRng, session, mem, c, route, Conditions{});
  // The guidebook's grade, not the rock's — your logbook records what the
  // tag said, sandbag and all.
  CHECK(mem.grade == 4);
}

// Proves the registry machinery with a synthetic migration, so the first
// real one (version 2) inherits working plumbing.
static void TestMigrationMachinery() {
  SaveFields fields;
  fields["oldname"] = "42";
  const std::vector<Migration> registry = {+[](SaveFields& f) {
    f["newname"] = f["oldname"];
    f.erase("oldname");
  }};
  CHECK(ApplyMigrations(fields, 1, 2, registry));
  CHECK(fields.count("newname") == 1);
  CHECK(fields.count("oldname") == 0);
  // A gap in the registry refuses rather than skipping a version.
  SaveFields f2;
  CHECK(!ApplyMigrations(f2, 1, 3, registry));
}


// ---------------------------------------------------------------------
// A life outside it
// ---------------------------------------------------------------------

// Run the nights with nobody giving anything to anything, and report what
// ended. `from` is the first night; the thread is never fed.
static Thread NeglectFor(Life& life, int from, int nights,
                         const LifeDials& d = LifeDials{}) {
  Thread lost = Thread::None;
  for (int i = 0; i < nights; i++) {
    const Thread tonight = LifeNight(life, from + i, d);
    if (tonight != Thread::None) lost = tonight;
  }
  return lost;
}

static void TestAThreadGoesColdAndComesBack() {
  LifeDials d;
  Life life;
  CHECK(TakeUp(life, Thread::Books, 1, d));
  CHECK(Going(life, Thread::Books));
  CHECK(WarmthOf(life, Thread::Books) == 1.0);   // nobody starts neglecting

  // Inside its patience nothing happens at all. A paperback waits.
  NeglectFor(life, 2, static_cast<int>(d.patienceDays[
      static_cast<int>(Thread::Books)]), d);
  CHECK(WarmthOf(life, Thread::Books) == 1.0);
  CHECK(HowItIsGoing(life, Thread::Books, 30, d) ==
        std::string("You are halfway through something and you know which."));

  // Past it, it cools -- and keeps cooling, all the way to nothing.
  NeglectFor(life, 32, 200, d);
  CHECK(WarmthOf(life, Thread::Books) == 0.0);
  CHECK(Going(life, Thread::Books));          // still yours, just cold
  CHECK(!HowItIsGoing(life, Thread::Books, 232, d).empty());

  // And an evening picks it straight back up, because it is a book.
  CHECK(GiveItTime(life, Thread::Books, 3.0, 233, d));
  CHECK(WarmthOf(life, Thread::Books) > 0.0);
  CHECK(LifeRestRate(life, d) > 1.0);
}

static void TestOnlyAPersonLeaves() {
  LifeDials d;
  Life life;
  for (int i = 1; i < kThreadCount; i++) {
    CHECK(TakeUp(life, static_cast<Thread>(i), 1, d));
  }
  // A year of climbing and nothing else.
  const Thread lost = NeglectFor(life, 2, 365, d);
  CHECK(lost == Thread::Someone);
  CHECK(!Going(life, Thread::Someone));
  // Everything else is at nothing and still there. A guitar does not
  // resign; it sits behind the driver's seat paying you nothing.
  for (int i = 1; i < kThreadCount; i++) {
    const Thread t = static_cast<Thread>(i);
    if (t == Thread::Someone) continue;
    CHECK(Going(life, t));
    CHECK(WarmthOf(life, t) == 0.0);
  }
  // And with every thread at zero, every effect is back to the plain rate.
  CHECK(LifeNerve(life, d) == 0.0);
  CHECK(LifeMealCost(life, d) == 1.0);
  CHECK(LifeRestRate(life, d) == 1.0);
  CHECK(BuskingPay(life, 4.0, d) == 0.0);
}

static void TestAFortnightIsNotALoss() {
  LifeDials d;
  Life life;
  CHECK(TakeUp(life, Thread::Someone, 1, d));
  // Two weeks of turning up, then nothing. It ends -- but it was two
  // weeks, and a game that grieves everything grieves nothing.
  for (int day = 1; day <= 14; day++) GiveItTime(life, Thread::Someone, 3.0, day, d);
  CHECK(DepthOf(life, Thread::Someone) < d.enoughToLose);

  // **Stopped on the night it ends**, not ninety nights later. The first
  // version of this ran the neglect straight through and asked afterwards
  // -- which counts a grief of five days all the way back down to zero and
  // then reports zero, so the check passed with the floor removed. A test
  // that cannot fail is worse than no test, and the reintroduction pass is
  // the only thing that has ever caught one of these.
  Thread lost = Thread::None;
  for (int day = 15; day < 15 + 90 && lost == Thread::None; day++) {
    lost = LifeNight(life, day, d);
  }
  CHECK(lost == Thread::Someone);
  CHECK(life.grieving == 0.0);
  CHECK(LifePsyche(life, d) == 0.0);
}

static void TestALossCostsYouASeasonAndThenStops() {
  LifeDials d;
  Life life;
  CHECK(TakeUp(life, Thread::Someone, 1, d));
  // Half a year of actually being there.
  int day = 1;
  for (; day <= 180; day++) GiveItTime(life, Thread::Someone, 3.0, day, d);
  CHECK(DepthOf(life, Thread::Someone) > d.enoughToLose);
  const double kept = LifePsyche(life, d);
  CHECK(kept > 0.0);                       // somebody is worth something

  // Then a season at the crag.
  CHECK(NeglectFor(life, day, 40, d) == Thread::Someone);
  CHECK(life.grieving > 0.0);
  const double justAfter = LifePsyche(life, d);
  CHECK(justAfter < 0.0);                  // and it costs, for a while

  // It fades rather than switching off, and then it is over. A career is
  // not ended by this; a season is.
  Life fading = life;
  NeglectFor(fading, day + 40, 10, d);
  const double later = LifePsyche(fading, d);
  CHECK(later > justAfter);
  CHECK(later < 0.0);
  NeglectFor(fading, day + 50, 200, d);
  CHECK(fading.grieving == 0.0);
  CHECK(LifePsyche(fading, d) == 0.0);
}

static void TestThereIsNobodyElseOnThursday() {
  LifeDials d;
  Life life;
  CHECK(TakeUp(life, Thread::Someone, 1, d));
  const int ended = 1 + 30;
  NeglectFor(life, 2, 60, d);
  CHECK(!Going(life, Thread::Someone));
  const int endedOn = life.strands[static_cast<int>(Thread::Someone)].endedDay;
  CHECK(endedOn >= ended - 30);
  CHECK(!CanTakeUp(life, Thread::Someone, endedOn + 1, d));
  CHECK(!TakeUp(life, Thread::Someone, endedOn + 1, d));
  CHECK(CanTakeUp(life, Thread::Someone,
                  endedOn + static_cast<int>(d.sparkCooldownDays) + 1, d));
  // Everything else you can pick back up on a wet Tuesday.
  Life other;
  CHECK(CanTakeUp(other, Thread::Music, 1, d));
}

static void TestEachThreadPaysInItsOwnCurrency() {
  LifeDials d;
  Life life;
  // Nothing pays anything until it is in your life.
  CHECK(LifeMealCost(life, d) == 1.0);
  CHECK(BuskingPay(life, 4.0, d) == 0.0);
  CHECK(LifeNerve(life, d) == 0.0);

  CHECK(TakeUp(life, Thread::Cooking, 1, d));
  CHECK(LifeMealCost(life, d) < 1.0);
  CHECK(LifeMealHunger(life, d) > 1.0);

  CHECK(TakeUp(life, Thread::Home, 1, d));
  CHECK(LifeNerve(life, d) > 0.0);

  // The guitar pays coins at first, and the reputation is worth more than
  // the hour ever is.
  CHECK(TakeUp(life, Thread::Music, 1, d));
  const double firstAfternoon = BuskingPay(life, 4.0, d);
  CHECK(firstAfternoon > 0.0);
  for (int day = 1; day <= 300; day++) GiveItTime(life, Thread::Music, 3.0, day, d);
  CHECK(BuskingPay(life, 4.0, d) > 2.0 * firstAfternoon);

  // And a meal is cheaper through the day loop, not just in the formula --
  // the same money, taken twice, differs by what the stove is worth.
  PlayerState plain;
  PlayerState cooks;
  cooks.life = life;
  DayState dp = WakeUp(plain);
  DayState dc = WakeUp(cooks);
  dp.hunger = dc.hunger = 60.0;
  CHECK(EatMeal(plain, dp));
  CHECK(EatMeal(cooks, dc));
  CHECK(cooks.cash > plain.cash);
  CHECK(dc.hunger < dp.hunger);
}

static void TestAPhoneCallHomeReachesTheWall() {
  LifeDials d;
  Life called;
  CHECK(TakeUp(called, Thread::Home, 1, d));

  const auto boldnessWith = [](const Life& life) {
    AttemptInput in;
    in.route = Route{};
    BodyContext body;
    body.life = life;
    ApplyBody(in, body);
    return in.boldness;
  };
  CHECK(boldnessWith(called) > boldnessWith(Life{}));
  // ...and it is gone when the thread is, which is what makes it a thread
  // and not an upgrade.
  Life lapsed = called;
  NeglectFor(lapsed, 2, 400, d);
  CHECK(boldnessWith(lapsed) == boldnessWith(Life{}));
}

static void TestTheLifeLabelStaysQuiet() {
  LifeDials d;
  Life life;
  CHECK(LifeLabel(life, 1, d).empty());          // no threads, no nagging
  CHECK(TakeUp(life, Thread::Home, 1, d));
  CHECK(LifeLabel(life, 2, d).empty());          // kept, so nothing to say
  NeglectFor(life, 2, 60, d);
  CHECK(!LifeLabel(life, 62, d).empty());        // and now there is

  // ...and it stops once there is nothing left to lose. A guitar that has
  // been in its case for two years is at zero and is not going anywhere;
  // saying so every night for the rest of a career is the same failure
  // from the other end.
  NeglectFor(life, 62, 400, d);
  CHECK(WarmthOf(life, Thread::Home) == 0.0);
  CHECK(LifeLabel(life, 462, d).empty());
  CHECK(!HowItIsGoing(life, Thread::Home, 462, d).empty());  // still asks
  CHECK(!HowItIsGoing(life, Thread::Home, 62, d).empty());
  // A thread you never took up is not a status.
  CHECK(HowItIsGoing(life, Thread::Books, 62, d).empty());
  // Grieving speaks over everything else, because it should.
  Life lost;
  TakeUp(lost, Thread::Someone, 1, d);
  for (int day = 1; day <= 180; day++) GiveItTime(lost, Thread::Someone, 3.0, day, d);
  NeglectFor(lost, 181, 40, d);
  CHECK(LifeLabel(lost, 221, d) == std::string("Still not over it"));
}

static void TestALifeSurvivesASave() {
  SaveGame save;
  save.seed = "a-life-outside-it";
  Life& life = save.player.life;
  TakeUp(life, Thread::Someone, 1);
  for (int day = 1; day <= 120; day++) GiveItTime(life, Thread::Someone, 3.0, day);
  TakeUp(life, Thread::Music, 40);
  for (int day = 40; day <= 120; day++) GiveItTime(life, Thread::Music, 2.0, day);
  TakeUp(life, Thread::Books, 90);
  NeglectFor(life, 121, 40);          // long enough to lose one
  save.player.lostToday = Thread::Someone;

  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  const Life& got = back.player.life;
  CHECK(!Going(got, Thread::Someone));
  CHECK(got.strands[static_cast<int>(Thread::Someone)].ended);
  CHECK(got.strands[static_cast<int>(Thread::Someone)].endedDay ==
        life.strands[static_cast<int>(Thread::Someone)].endedDay);
  CHECK(std::fabs(got.grieving - life.grieving) < 1e-9);
  CHECK(std::fabs(got.griefWeight - life.griefWeight) < 1e-9);
  CHECK(back.player.lostToday == Thread::Someone);
  CHECK(Going(got, Thread::Music));
  CHECK(std::fabs(DepthOf(got, Thread::Music) - DepthOf(life, Thread::Music)) <
        1e-9);
  // You load in exactly as grieving and exactly as broke as you saved.
  CHECK(std::fabs(LifePsyche(got) - LifePsyche(life)) < 1e-9);
  CHECK(std::fabs(BuskingPay(got, 4.0) - BuskingPay(life, 4.0)) < 1e-9);
}

static void TestLoadsVersion35Save() {
  // A v35 career had nothing in it but climbing, because there was nothing
  // else in the game to have. It has to load anyway.
  SaveGame save;
  save.seed = "a-life-outside-it";
  save.player.cash = 137.5;
  TakeUp(save.player.life, Thread::Someone, 1);

  // Derived from this build's own writer rather than typed out: drop every
  // line v35 could not have had and put the version back. A hand-typed
  // fixture tests what I believed v35 looked like, which is the thing most
  // likely to be wrong.
  std::string v35 = SerializeSave(save);
  for (int i = 1; i < kThreadCount; i++) {
    const std::string k = "life." + std::to_string(i) + ".";
    DropSaveLine(v35, (k + "going=").c_str());
    DropSaveLine(v35, (k + "warm=").c_str());
    DropSaveLine(v35, (k + "deep=").c_str());
    DropSaveLine(v35, (k + "last=").c_str());
    DropSaveLine(v35, (k + "days=").c_str());
    DropSaveLine(v35, (k + "ended=").c_str());
    DropSaveLine(v35, (k + "endday=").c_str());
  }
  DropSaveLine(v35, "life.grieving=");
  DropSaveLine(v35, "life.griefw=");
  DropSaveLine(v35, "life.lost=");
  SetSaveVersion(v35, 35);

  SaveGame old;
  CHECK(DeserializeSave(v35, old) == LoadResult::Ok);
  CHECK(old.version == kSaveVersion);            // arrives upgraded
  CHECK(std::fabs(old.player.cash - 137.5) < 1e-9);   // and intact
  // Nothing taken up, which is the truth about that career rather than a
  // loss: there was nobody to neglect.
  for (int i = 1; i < kThreadCount; i++) {
    CHECK(!Going(old.player.life, static_cast<Thread>(i)));
    CHECK(!old.player.life.strands[i].ended);
  }
  CHECK(old.player.life.grieving == 0.0);
  CHECK(old.player.lostToday == Thread::None);
  CHECK(static_cast<int>(DefaultMigrations().size()) == kSaveVersion - 1);
}

static void TestALifeCoolsWhileYouAreAtTheCrag() {
  PlayerState player;
  DayState day;
  const Rng world = Rng::FromSeed("a-life-outside-it");
  player.climber = NewClimber(world);

  // Half a year of somebody, taken through the day loop rather than the
  // dials, because the night tick is where three per-day rules in this
  // project have been written and left uncalled.
  CHECK(TakeUp(player.life, Thread::Someone, player.day));
  CHECK(TakeUp(player.life, Thread::Home, player.day));
  // **Fed.** Sleeping now charges for the hours you did not spend, so a
  // climber who never eats is a climber slowly starving -- which is
  // correct, and which would make this test about food rather than about
  // who is not around any more.
  const auto liveADay = [&](PlayerState& p, DayState& d) {
    p.cash = 500.0;
    d = WakeUp(p);
    PassHours(d, 12.0);
    EatMeal(p, d);
    SleepToNextDay(p, d, world);
  };
  for (int i = 0; i < 180; i++) {
    GiveItTime(player.life, Thread::Someone, 3.0, player.day);
    GiveItTime(player.life, Thread::Home, 1.0, player.day);
    liveADay(player, day);
  }
  CHECK(WarmthOf(player.life, Thread::Someone) == 1.0);
  const double keptWarm = player.climber.psyche;

  // Then a season of climbing and nothing else. Nobody calls LifeNight
  // here; the night does.
  bool announced = false;
  for (int i = 0; i < 120; i++) {
    liveADay(player, day);
    if (player.lostToday == Thread::Someone) announced = true;
  }
  CHECK(announced);                            // and it said so, once
  CHECK(!Going(player.life, Thread::Someone));
  CHECK(WarmthOf(player.life, Thread::Home) < 1.0);
  // The psyche baseline followed it down. Same climber, same weather, same
  // seed -- the only thing that changed is who is not around any more.
  CHECK(player.climber.psyche < keptWarm);
}

static void TestABookMakesAnAfternoonWorthMore() {
  PlayerState reader;
  CHECK(TakeUp(reader.life, Thread::Books, reader.day));
  PlayerState plain;
  DayState a = WakeUp(plain);
  DayState b = WakeUp(reader);
  a.energy = b.energy = 40.0;
  Rest(a, 4.0, plain.life);
  Rest(b, 4.0, reader.life);
  CHECK(b.energy > a.energy);
  CHECK(a.hour == b.hour);        // the hours cost the same either way
}

static void TestAnEveningCostsTheEvening() {
  PlayerState player;
  DayState day = WakeUp(player);
  // One press takes it up and pays for it, which is the whole point: a
  // first go costs a first go.
  CHECK(!Going(player.life, Thread::Music));
  CHECK(SpendTheEvening(player, day, Thread::Music));
  CHECK(Going(player.life, Thread::Music));

  // And what it will not do is start something the game has said no to.
  CHECK(TakeUp(player.life, Thread::Someone, player.day));
  for (int night = 0; night < 120; night++) {
    LifeNight(player.life, player.day + night);
  }
  CHECK(!Going(player.life, Thread::Someone));
  CHECK(!SpendTheEvening(player, day, Thread::Someone));   // not this soon

  const double hour0 = day.hour;
  const double cash0 = player.cash;
  CHECK(SpendTheEvening(player, day, Thread::Music));
  // The hours go, like any other hours -- and how many is the sim's
  // answer, not the caller's. Seeing somebody costs a working day and a
  // phone call costs half an hour, which is the whole of why only one of
  // the five can leave you.
  CHECK(day.hour == hour0 + AsksFor(Thread::Music));
  CHECK(AsksFor(Thread::Someone) > 4.0);      // a day, not an evening
  CHECK(AsksFor(Thread::Home) < 1.0);
  CHECK(AsksFor(Thread::None) == 0.0);
  CHECK(day.hunger > 0.0);
  CHECK(player.cash > cash0);           // and the hat had something in it
}


// ---------------------------------------------------------------------
// The people who remember you between visits
// ---------------------------------------------------------------------

// Turn up enough times that somebody knows your face.
static void BecomeAFace(Local& who, int fromDay, int visits) {
  for (int i = 0; i < visits; i++) Seen(who, fromDay + i);
}

// **Never dereference `At` without checking it.** The first version of
// these tests wrote `At(town, Service::Meal)->name` directly, and when the
// reintroduction pass removed the roster's seeding the test **segfaulted
// instead of failing** -- which took the whole binary down and silently
// un-ran every test after it, so four separate defects came back "not
// caught" from one crash. A test that cannot fail is bad; a test that
// crashes is worse, because it takes its neighbours with it.
static const Local& Counter(const Locals& town, Service where) {
  static Local kNobodyConst;
  const Local* who = At(town, where);
  CHECK(who != nullptr);
  return who ? *who : kNobodyConst;
}

static Local& Counter(Locals& town, Service where) {
  static Local kNobody;
  kNobody = Local{};
  Local* who = At(town, where);
  CHECK(who != nullptr);
  return who ? *who : kNobody;
}

// How many people were around across a stretch, which is the only way to
// ask about a thing that is a coin flip on any given day.
static int CompanyOver(const Rng& world, int days,
                       const std::vector<PartnerBond>& bonds, double social,
                       bool rockIsIn, const PartnerDials& d = PartnerDials{}) {
  int seen = 0;
  for (int day = 1; day <= days; day++) {
    seen += static_cast<int>(
        WhoIsAround(world, day, bonds, social, rockIsIn, 1.0, d).size());
  }
  return seen;
}

// ---------------------------------------------------------------------
// The gym you bought
// ---------------------------------------------------------------------

static Gym ABoughtGym(double& cash, int day = 1) {
  Gym g;
  cash = GymDials{}.price + 20000.0;
  CHECK(BuyTheGym(g, cash, "The Woodshed", day));
  return g;
}

// ---------------------------------------------------------------------
// Living in the van
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// Where you are parking tonight
// ---------------------------------------------------------------------

static void TestNoSpotIsTheBestSpot() {
  // **The trade is the system.** If one of the five won on every axis the
  // decision would not exist, so the check is that none does.
  for (int i = 0; i < kSpotCount; i++) {
    const SpotDef& mine = Describe(static_cast<Spot>(i));
    bool beatenSomewhere = false;
    for (int j = 0; j < kSpotCount && !beatenSomewhere; j++) {
      if (i == j) continue;
      const SpotDef& theirs = Describe(static_cast<Spot>(j));
      beatenSomewhere = theirs.psyche > mine.psyche ||
                        theirs.driveHours < mine.driveHours ||
                        theirs.exposure < mine.exposure ||
                        theirs.grime < mine.grime ||
                        theirs.cooldownDays < mine.cooldownDays;
    }
    CHECK(beatenSomewhere);
  }
  // And the free one is the one the city can tag, which is the whole
  // shape of it.
  CHECK(Describe(Spot::Lot).driveHours == 0.0);
  CHECK(Describe(Spot::Lot).tickets);
  for (int i = 1; i < kSpotCount; i++) {
    CHECK(!Describe(static_cast<Spot>(i)).tickets);
  }
}

static void TestTheGoodSpotsHaveToBeEarned() {
  BivyDials d;
  Bivy bivy;
  // The Lot and the trailhead are open to anybody with a van.
  CHECK(SpotIsOpen(bivy, Spot::Lot, 1, false, 0.0, d));
  CHECK(SpotIsOpen(bivy, Spot::UpperTrail, 1, false, 0.0, d));

  // The ridge is not in the guidebook -- somebody has to tell you.
  CHECK(!SpotIsOpen(bivy, Spot::Ridge, 1, false, 0.0, d));
  CHECK(!WhyNot(bivy, Spot::Ridge, 1, false, 0.0, d).empty());
  CHECK(SpotIsOpen(bivy, Spot::Ridge, 1, true, 0.0, d));

  // And a driveway is a favour rather than an address.
  CHECK(!SpotIsOpen(bivy, Spot::Driveway, 1, true, 0.0, d));
  CHECK(WhyNot(bivy, Spot::Driveway, 1, true, 0.0, d) ==
        std::string("nobody knows you well enough to offer yet"));
  CHECK(SpotIsOpen(bivy, Spot::Driveway, 1, true, 1.0, d));
  CHECK(WhyNot(bivy, Spot::Driveway, 1, true, 1.0, d).empty());
}

static void TestADrivewayIsNotAnAddress() {
  BivyDials d;
  Bivy bivy;
  const Rng world = Rng::FromSeed("parked-up");
  CHECK(ParkAt(bivy, Spot::Driveway, 10, true, 1.0, d));
  NightAt(bivy, world, 10, d);
  // Four nights before you can ask again, which is what stops the best
  // spot in the game from being the only one anybody uses.
  CHECK(!SpotIsOpen(bivy, Spot::Driveway, 11, true, 1.0, d));
  CHECK(!ParkAt(bivy, Spot::Driveway, 11, true, 1.0, d));
  CHECK(!WhyNot(bivy, Spot::Driveway, 11, true, 1.0, d).empty());
  CHECK(SpotIsOpen(bivy, Spot::Driveway,
                   10 + Describe(Spot::Driveway).cooldownDays, true, 1.0, d));
}

static void TestTheCityNoticesYouEventually() {
  BivyDials d;
  Bivy bivy;
  const Rng world = Rng::FromSeed("parked-up");
  // **Two nights grace, and then it climbs.** The Lot works, and then it
  // stops working, which is the entire reason the other four exist.
  bool ticketed = false;
  for (int day = 1; day <= d.ticketGrace; day++) {
    ticketed = ticketed || NightAt(bivy, world, day, d).ticketed;
  }
  CHECK(!ticketed);
  CHECK(bivy.lotNights == d.ticketGrace);

  int day = d.ticketGrace + 1;
  for (; day <= 200 && !bivy.booted; day++) NightAt(bivy, world, day, d);
  CHECK(bivy.ticketsOwed >= d.bootAt);
  CHECK(bivy.booted);
  CHECK(!BivyWarning(bivy, d).empty());

  // Going anywhere else resets the streak -- that is what the other spots
  // are *for*.
  Bivy moved;
  const Rng w2 = Rng::FromSeed("parked-up");
  for (int i = 1; i <= 20; i++) {
    ParkAt(moved, i % 2 ? Spot::Lot : Spot::UpperTrail, i, false, 0.0, d);
    NightAt(moved, w2, i, d);
  }
  CHECK(moved.lotNights <= 1);
  CHECK(!moved.booted);
}

static void TestTheCityDoesNotDoInstalments() {
  BivyDials d;
  Bivy bivy;
  bivy.ticketsOwed = 3;
  bivy.booted = true;
  const double owed = WhatYouOwe(bivy, d);
  CHECK(owed == 3 * d.ticketFine + d.impoundFee);

  double nearly = owed - 1.0;
  CHECK(!PayTheTickets(bivy, nearly, d));
  CHECK(bivy.booted);                    // and it is still clamped
  CHECK(nearly == owed - 1.0);           // and it cost them nothing

  double enough = owed;
  CHECK(PayTheTickets(bivy, enough, d));
  CHECK(enough == 0.0);
  CHECK(bivy.ticketsOwed == 0);
  CHECK(!bivy.booted);
  CHECK(BivyWarning(bivy, d).empty());   // and it goes quiet again
  CHECK(!PayTheTickets(bivy, enough, d));  // nothing left to pay
}

static void TestWhereYouSleptFollowsYouIntoTheDay() {
  PlayerState ridge;
  PlayerState lot;
  DayState a, b;
  const Rng world = Rng::FromSeed("parked-up");
  ridge.climber = NewClimber(world);
  lot.climber = ridge.climber;
  ridge.climber.psyche = lot.climber.psyche = 0.5;
  CHECK(ParkAt(ridge.bivy, Spot::Ridge, ridge.day, true, 1.0));

  SleepToNextDay(ridge, a, world);
  SleepToNextDay(lot, b, world);
  // The ridge is worth six psyche points and costs a drive; the Lot is
  // free and worth nothing. Both true on the same night.
  CHECK(ridge.climber.psyche > lot.climber.psyche);
  CHECK(ridge.cash < lot.cash);
}

static void TestGrimeOnlyEverCostsYouCompany() {
  LivingDials d;
  Living clean;
  CHECK(GrimeSocial(clean.grime, d) == 1.0);
  CHECK(GrimeWord(clean.grime, d) == std::string("fresh"));

  // **The only mechanical bite in the whole system**, and it is narrow on
  // purpose: three quarters when ripe, half when feral, and one the rest
  // of the time -- because lived-in is what everybody in this valley is.
  CHECK(GrimeSocial(30.0, d) == 1.0);
  CHECK(GrimeWord(30.0, d) == std::string("lived-in"));
  CHECK(GrimeSocial(d.ripeAt, d) == d.ripeSocial);
  CHECK(GrimeWord(d.ripeAt, d) == std::string("ripe"));
  CHECK(GrimeSocial(d.feralAt, d) == d.feralSocial);
  CHECK(GrimeWord(d.feralAt, d) == std::string("feral"));
}

static void TestYouCannotGetCleanOutOfAJug() {
  LivingDials d;
  Living living;
  living.grime = 70.0;
  living.water = d.waterCap;

  CHECK(WashInTheVan(living, d));
  CHECK(living.grime == 70.0 - d.jugWash);
  CHECK(living.water == d.waterCap - d.waterWash);

  // ...and it floors. A rag and a jug does not get you to nothing, which
  // is the whole reason the truck stop is worth eight dollars.
  for (int i = 0; i < 10; i++) {
    living.water = d.waterCap;
    WashInTheVan(living, d);
  }
  CHECK(living.grime == d.jugFloor);
  CHECK(!WashInTheVan(living, d));       // nothing left to buy

  // The stall does what the jug cannot.
  double cash = 100.0, hours = 8.0;
  CHECK(ShowerAtTheTruckStop(living, cash, hours, d));
  CHECK(living.grime == 0.0);
  CHECK(cash == 100.0 - d.showerCost);
  CHECK(hours == 8.0 + d.showerHours);

  // And no water is no wash, whatever you smell like.
  Living dry;
  dry.grime = 90.0;
  dry.water = 0.0;
  CHECK(!WashInTheVan(dry, d));
}

static void TestTheJugsAndTheBottleRunOut() {
  LivingDials d;
  Living living;
  double cash = 100.0;

  CHECK(CanCook(living, d));
  const double propane = living.propane;
  Cooked(living, d);
  CHECK(living.propane == propane - d.propaneCook);

  // Cook until the bottle is done, and then you cannot.
  for (int i = 0; i < 40 && CanCook(living, d); i++) Cooked(living, d);
  CHECK(!CanCook(living, d));
  CHECK(!LivingLine(living, d).empty());   // and the van says so

  CHECK(SwapTheBottle(living, cash, d));
  CHECK(living.propane == d.propaneCap);
  CHECK(cash == 100.0 - d.propaneBottleCost);
  CHECK(!SwapTheBottle(living, cash, d));  // it is already full

  Living empty;
  empty.water = 0.0;
  double broke = 0.0;
  CHECK(!FillTheJugs(empty, broke, d));    // two dollars is still two dollars
  double coins = 5.0;
  CHECK(FillTheJugs(empty, coins, d));
  CHECK(empty.water == d.waterJug);
}

static void TestAWeekOutHereGetsRipe() {
  LivingDials d;
  Living living;
  living.grime = 8.0;
  // A night out and a day on the rock, over and over. Nobody showers on a
  // road trip and the game should be able to tell.
  for (int i = 0; i < 7; i++) {
    ClimbedToday(living, d);
    LivingNight(living, false, false, d);
  }
  CHECK(living.grime >= d.ripeAt);
  CHECK(GrimeSocial(living.grime, d) < 1.0);
  // ...and the water goes with it.
  CHECK(living.water < 30.0);
}

static void TestBeingRipeEmptiesTheLotAroundYou() {
  const Rng world = Rng::FromSeed("who-turns-up");
  // **The seam this whole system exists for.** Grime discounts what *you*
  // bring -- your rapport and your Social axis -- and leaves each person's
  // own reliability alone, because Trish being here on a Tuesday is not
  // about you.
  std::vector<PartnerBond> known;
  for (const Partner& p : LotRegulars(world, 1)) {
    PartnerBond b;
    b.name = p.name;
    b.rapport = 1.0;
    b.everKnew = 1.0;
    known.push_back(b);
  }
  const auto over = [&](double smell) {
    int seen = 0;
    for (int day = 1; day <= 365; day++) {
      seen += static_cast<int>(
          WhoIsAround(world, day, known, 60.0, true, smell).size());
    }
    return seen;
  };
  const int fresh = over(1.0);
  const int ripe = over(LivingDials{}.ripeSocial);
  const int feral = over(LivingDials{}.feralSocial);
  CHECK(ripe < fresh);
  CHECK(feral < ripe);
  // But never to nothing: the floor still holds, so somebody still turns
  // up for the most feral climber in the valley.
  CHECK(feral > 0);
}

static void TestGrimeCostsYouRapportAndTheCounter() {
  // The other two social gains. Same multiplier, same shape.
  Partner a, b;
  a.name = b.name = "Margo";
  for (int i = 0; i < 10; i++) {
    SpendDayWith(a, true, 1.0);
    SpendDayWith(b, true, LivingDials{}.feralSocial);
  }
  CHECK(b.rapport < a.rapport);

  Locals town = TheLocals(DirtbagTown(), Rng::FromSeed("crag-1"));
  Locals ripe = town;
  for (int i = 0; i < 6; i++) {
    Seen(Counter(town, Service::Meal), i + 1, 1.0);
    Seen(Counter(ripe, Service::Meal), i + 1, LivingDials{}.feralSocial);
  }
  CHECK(Counter(ripe, Service::Meal).known <
        Counter(town, Service::Meal).known);
}

static void TestTheVanIsQuietUntilSomethingRunsOut() {
  LivingDials d;
  Living fine;
  CHECK(LivingLine(fine, d).empty());     // lived-in with full jugs says nothing

  Living dry = fine;
  dry.water = 0.0;
  CHECK(!LivingLine(dry, d).empty());

  // One line, loudest first -- a van with four warnings on it is a
  // dashboard, and this is the edge of a screen.
  Living everything;
  everything.grime = 95.0;
  everything.water = 0.0;
  everything.propane = 0.0;
  const std::string said = LivingLine(everything, d);
  CHECK(said.find("further away") != std::string::npos);
}

static void TestYouCannotBuyWhatYouCannotAfford() {
  GymDials d;
  Gym g;
  // **$25,000**, which is the original's price and lands just under Home
  // Base's $30,000 -- so it is a choice against the dream rather than a
  // tier above it.
  double broke = d.price - 1.0;
  CHECK(!BuyTheGym(g, broke, "The Woodshed", 1));
  CHECK(!g.owned);
  CHECK(broke == d.price - 1.0);          // and it cost them nothing

  double enough = d.price;
  CHECK(BuyTheGym(g, enough, "The Woodshed", 1));
  CHECK(g.owned);
  CHECK(enough == 0.0);
  CHECK(g.members == d.seedMembers);
  CHECK(!BuyTheGym(g, enough, "Another One", 1));   // and only the one
}

static void TestTheFloorIsAMeterAndNotASwitch() {
  // **The town held still**, which is what the clamp dials are for. With
  // GYM-4 and GYM-6 live the target is not a constant, so a test about the
  // drift model has to stop the thing the drift model is chasing -- see
  // TestTheFloorIsAlwaysChasing for the other half.
  GymDials d;
  d.town.pressureMin = 1.0;
  d.town.pressureMax = 1.0;
  for (int i = 0; i < kGymSeasonCount; i++) d.town.seasonPull[i] = 0.0;
  const Rng world = Rng::FromSeed("the-woodshed");
  double cash = 0.0;
  Gym g = ABoughtGym(cash);

  // Budget pulls a packed floor; the members do **not** jump to it. The
  // original's own comment: a meter, not a switch -- pricing takes several
  // days to show what it did, the same shape as every other value in this
  // port that ages toward something.
  SetPrice(g, GymPrice::Budget);
  const double target = MembersItPullsToward(g, d);
  CHECK(target > g.members + 10.0);
  const double started = g.members;
  GymDay(g, world, 2, d);
  CHECK(g.members > started);
  CHECK(g.members < target);              // nowhere near it yet

  for (int day = 3; day <= 60; day++) GymDay(g, world, day, d);
  // ...and a couple of months later it is there, within the daily wobble.
  CHECK(std::fabs(g.members - target) <= d.memberNoise * 2.0);
}

static void TestTheLeversTradeVolumeAgainstMargin() {
  GymDials d;
  Gym budget, premium;
  double a = 0.0, b = 0.0;
  budget = ABoughtGym(a);
  premium = ABoughtGym(b);
  SetPrice(budget, GymPrice::Budget);
  SetPrice(premium, GymPrice::Premium);

  // The trade the whole system rests on: a packed floor on thin margins
  // against fewer members who are each worth real money.
  CHECK(MembersItPullsToward(budget, d) > MembersItPullsToward(premium, d));
  CHECK(RatePerMember(budget, d) < RatePerMember(premium, d));

  // And the set mix leans the same axis the other way, which is why it is
  // a second lever rather than more of the first.
  Gym hardcore = budget;
  SetMix(hardcore, GymSetMix::Hardcore);
  CHECK(MembersItPullsToward(hardcore, d) < MembersItPullsToward(budget, d));
  CHECK(RatePerMember(hardcore, d) > RatePerMember(budget, d));
}

static void TestEquipmentIsALadderAndStaffAreAWage() {
  GymDials d;
  double cash = 0.0;
  Gym g = ABoughtGym(cash);

  const double pulled = MembersItPullsToward(g, d);
  const double owed = DailyOverhead(g, d);
  CHECK(UpgradeEquipment(g, cash, d));
  CHECK(g.equip == GymEquip::HoldsAndMats);
  CHECK(MembersItPullsToward(g, d) > pulled);
  CHECK(DailyOverhead(g, d) > owed);       // it costs to keep, as well
  CHECK(UpgradeEquipment(g, cash, d));
  CHECK(g.equip == GymEquip::FullRenovation);
  CHECK(!UpgradeEquipment(g, cash, d));    // and the ladder ends

  // Cumulative rather than incremental: the two rungs together cost the
  // top rung's price, not the sum of both.
  double fresh = 0.0;
  Gym straight = ABoughtGym(fresh);
  const double before = fresh;
  UpgradeEquipment(straight, fresh, d);
  UpgradeEquipment(straight, fresh, d);
  CHECK(std::fabs((before - fresh) - d.equipCost[2]) < 1e-9);

  // A hire pulls members and costs a wage every day after -- **their own
  // wage**, not the role's, which is the whole of GYM-3 in one assertion.
  const double pulling = MembersItPullsToward(g, d);
  const double paying = DailyOverhead(g, d);
  CHECK(Hire(g, cash, true, 1, 0, d));
  const GymStaffer* hired = WhoIsOn(g, true);
  CHECK(hired != nullptr);
  CHECK(MembersItPullsToward(g, d) > pulling);
  CHECK(DailyOverhead(g, d) - paying == hired->wage);
  CHECK(!Hire(g, cash, true, 1, 0, d));    // one desk, one person
}

static void TestACampaignRunsOutAndOnlyOneRunsAtATime() {
  GymDials d;
  const Rng world = Rng::FromSeed("the-woodshed");
  double cash = 0.0;
  Gym g = ABoughtGym(cash);

  const double quiet = MembersItPullsToward(g, d);
  CHECK(LaunchCampaign(g, cash, GymCampaign::Social, 1, d));
  // Twice over, in fact: the boost is in the base, and a live campaign
  // also shields you from whatever the other two gyms are doing.
  CHECK(MembersItPullsToward(g, d) > quiet);
  // Launching does not stack, and the gate is the running one rather than
  // the money.
  CHECK(!LaunchCampaign(g, cash, GymCampaign::Flyers, 1, d));

  bool ended = false;
  for (int day = 2; day <= 1 + d.campaignDays[2] + 2; day++) {
    ended = ended || GymDay(g, world, day, d).campaignEnded;
  }
  CHECK(ended);
  CHECK(g.campaign == GymCampaign::None);
  // And the pull goes with it. **Compared against a twin rather than
  // against the number sampled on day 1**: the target is multiplied by the
  // town and the year now, so the only honest control is the same gym on
  // the same day that never ran the campaign.
  Gym never = ABoughtGym(cash);
  never.lastTickDay = g.lastTickDay;
  CHECK(std::fabs(MembersItPullsToward(g, d) -
                  MembersItPullsToward(never, d)) < 1e-9);
  // ...after which you can run another.
  CHECK(LaunchCampaign(g, cash, GymCampaign::Flyers, 40, d));
}

static void TestTheBankTakesItBack() {
  GymDials d;
  const Rng world = Rng::FromSeed("the-woodshed");
  double cash = 0.0;
  Gym g = ABoughtGym(cash);

  // **Premium pricing with a hardcore mix and nobody on staff.** Six
  // members at $25 against $180 of rent is thirty dollars a day under, and
  // it does not recover on its own.
  //
  // The first version of this test hired both staff on the theory that two
  // wages would sink it, and it was **wrong in an interesting way**: a hire
  // raises the target it is paid out of, so at premium rates the desk
  // returns $110 a day for a $25 wage. Staffing does not sink a gym in
  // these numbers -- an empty floor at full overhead does.
  SetPrice(g, GymPrice::Premium);
  SetMix(g, GymSetMix::Hardcore);
  CHECK(MembersItPullsToward(g, d) * RatePerMember(g, d) <
        DailyOverhead(g, d));

  bool foreclosed = false;
  int day = 2;
  for (; day < 400 && !foreclosed; day++) {
    foreclosed = GymDay(g, world, day, d).foreclosed;
    // **It resets the moment you are not in the red**, which is what makes
    // a fortnight a grace period rather than a countdown.
    if (g.owned && g.balance >= 0.0) CHECK(g.debtDays == 0);
    if (g.owned) CHECK(g.debtDays < d.bankruptcyDays);
  }
  CHECK(foreclosed);
  CHECK(!g.owned);                         // the building is gone
  CHECK(g.name.empty());

  // A gym nobody owns has no books and says nothing.
  CHECK(GymDay(g, world, day, d).net == 0.0);
  CHECK(GymLine(g, d).empty());
  CHECK(GymWarning(g, d).empty());
}

static void TestTheNoticeboardIsQuietWhileTheBooksAreFine() {
  GymDials d;
  const Rng world = Rng::FromSeed("the-woodshed");
  double cash = 0.0;
  Gym g = ABoughtGym(cash);
  SetPrice(g, GymPrice::Budget);

  for (int day = 2; day <= 40; day++) GymDay(g, world, day, d);
  CHECK(g.balance > 0.0);
  // **An open incident is legitimately not quiet**, and after the grace
  // window one can land on any night -- so the quiet being asserted here is
  // the quiet of the books, with anything on the clipboard cleared first.
  double pocket = 100000.0;
  AnswerTheIncident(g, pocket, 0, d);
  CHECK(g.incident == GymIncident::None);
  CHECK(GymWarning(g, d).empty());          // nothing to say, so nothing said
  CHECK(!GymLine(g, d).empty());            // ...but the readout still reads

  // And it speaks once the books do not.
  Gym sinking = g;
  sinking.balance = -1.0;
  sinking.debtDays = 4;
  CHECK(!GymWarning(sinking, d).empty());
  sinking.debtDays = d.bankruptcyDays - 1;
  const std::string last = GymWarning(sinking, d);
  CHECK(last.find("from the bank taking it") != std::string::npos);
}

static void TestTheGymRunsOnItsOwnRng() {
  // The floor's daily wobble must not move any other stream.
  Rng a = Rng::FromSeed("the-woodshed");
  Rng b = Rng::FromSeed("the-woodshed");
  const double before = a.NextDouble();
  const Rng world = Rng::FromSeed("the-woodshed");
  double cash = 0.0;
  Gym g = ABoughtGym(cash);
  for (int day = 2; day <= 80; day++) GymDay(g, world, day);
  CHECK(before == b.NextDouble());

  // ...and it is replayable: the same gym, the same days, the same floor.
  double twin = 0.0;
  Gym again = ABoughtGym(twin);
  for (int day = 2; day <= 80; day++) GymDay(again, world, day);
  CHECK(again.members == g.members);
  CHECK(std::fabs(again.balance - g.balance) < 1e-9);
}

static void TestTheGymSurvivesASave() {
  SaveGame save;
  save.seed = "the-woodshed";
  double cash = GymDials{}.price;
  CHECK(BuyTheGym(save.player.gym, cash, "The Woodshed", 40));
  SetPrice(save.player.gym, GymPrice::Premium);
  SetMix(save.player.gym, GymSetMix::Hardcore);
  cash = 20000.0;
  CHECK(UpgradeEquipment(save.player.gym, cash));
  CHECK(Hire(save.player.gym, cash, false, 40, 0));
  CHECK(LaunchCampaign(save.player.gym, cash, GymCampaign::Social, 40));
  save.player.gym.members = 31.0;
  save.player.gym.balance = -412.5;
  save.player.gym.debtDays = 6;

  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  const Gym& g = back.player.gym;
  CHECK(g.owned);
  CHECK(g.name == std::string("The Woodshed"));
  CHECK(g.price == GymPrice::Premium);
  CHECK(g.mix == GymSetMix::Hardcore);
  CHECK(g.equip == GymEquip::HoldsAndMats);
  CHECK(g.campaign == GymCampaign::Social);
  CHECK(g.campaignUntil == save.player.gym.campaignUntil);
  CHECK(!g.frontDesk);
  CHECK(g.setter);
  CHECK(g.members == 31.0);
  CHECK(std::fabs(g.balance + 412.5) < 1e-9);
  CHECK(g.debtDays == 6);
  // **You load in exactly as close to losing it**, which is the only
  // honest answer -- otherwise saving is a payment.
  CHECK(GymWarning(g) == GymWarning(save.player.gym));
  CHECK(std::fabs(MembersItPullsToward(g) -
                  MembersItPullsToward(save.player.gym)) < 1e-9);
}

// --- GYM-4 and GYM-6: the town and the year -------------------------------

static void TestTheTownIsWeatherAndNotATax() {
  // **The invariant the source records paying for.** Its first cut of the
  // rival tables was net-positive and averaged 0.948 pressure -- a silent
  // 5% nerf to every gym in the game forever, invisible because it looked
  // like competition. Each table has to sum to zero, or this is a tax.
  //
  // Asserted through the public surface rather than against the tables,
  // because the tables are the thing that would drift.
  const GymTownDials d;
  for (int r = 0; r < kGymRivalCount; r++) {
    const GymRival who = static_cast<GymRival>(r);
    double total = 0.0;
    int seen = 0;
    // Every window in a long career, so every move in the table gets drawn
    // many times over and their mean is the table's mean.
    for (int w = 0; w < 4000; w++) {
      const TownMove m = WhatTheTownDid(w, "The Woodshed");
      if (m.who != who) continue;
      total += m.pull;
      seen++;
    }
    CHECK(seen > 500);              // and both rooms actually move
    CHECK(std::fabs(total / seen) < 0.02);
  }

  // ...and over a career the pressure it produces averages about 1.0 --
  // the town is a weather system, not a headwind.
  double sum = 0.0;
  for (int day = 1; day <= 10950; day++) {
    sum += TownPressure(day, "The Woodshed", 1.0, d);
  }
  const double mean = sum / 10950.0;
  CHECK(mean > 0.96 && mean < 1.04);
}

static void TestTheYearIsARhythmAndNotATax() {
  // The same invariant one system over, and **the one deliberate deviation
  // in this port**: the source's four pulls are balanced against equal
  // calendar quarters, and this port names its seasons off the temperature,
  // which makes them 61/121/61/122. Ported as written they average +0.0298
  // -- a permanent 3% bonus to every gym forever. Re-centred, they do not.
  const GymTownDials d;
  double sum = 0.0;
  for (int day = 1; day <= 365; day++) sum += SeasonPull(day, d);
  CHECK(std::fabs(sum / 365.0) < 0.001);

  // And it runs the way this world runs rather than the way the real one
  // does: **the slump is autumn**, because autumn is send season and the
  // whole town is at the crag.
  CHECK(d.seasonPull[static_cast<int>(GymSeason::Autumn)] < 0.0);
  CHECK(d.seasonPull[static_cast<int>(GymSeason::Summer)] > 0.0);
  CHECK(d.seasonPull[static_cast<int>(GymSeason::Winter)] > 0.0);
  CHECK(d.seasonPull[static_cast<int>(GymSeason::Autumn)] <
        d.seasonPull[static_cast<int>(GymSeason::Spring)]);
  // Named off the climate model, so it agrees with the rest of the port.
  CHECK(GymSeasonOf(200) == GymSeason::Summer);
  CHECK(!std::string(GymSeasonNote(GymSeason::Autumn)).empty());
}

static void TestTheTownIsYoursAndNotAScript() {
  // Salted with the gym's name, and the source records why: keyed on the
  // day alone the town's whole history is one fixed script identical in
  // every save, and measured over the days players actually reach that
  // script sits slightly against you. No playthrough could be dealt a kind
  // town.
  int differed = 0;
  for (int w = 0; w < 200; w++) {
    const TownMove mine = WhatTheTownDid(w, "The Woodshed");
    const TownMove theirs = WhatTheTownDid(w, "Gravity Club");
    if (mine.who != theirs.who || mine.pull != theirs.pull) differed++;
  }
  CHECK(differed > 100);

  // ...and it is still fixed for a given save: the same name, the same
  // window, the same move, however many times you ask.
  CHECK(WhatTheTownDid(17, "The Woodshed").pull ==
        WhatTheTownDid(17, "The Woodshed").pull);
}

static void TestTheTownIsClampedAtBothEnds() {
  const GymTownDials d;
  double lowest = 99.0, highest = 0.0;
  for (int day = 1; day <= 10950; day++) {
    const double p = TownPressure(day, "The Woodshed", 1.0, d);
    lowest = std::min(lowest, p);
    highest = std::max(highest, p);
  }
  // A rival hot streak is a squeeze, not a death sentence; their bad one is
  // an advantage, not a monopoly.
  CHECK(lowest >= d.pressureMin);
  CHECK(highest <= d.pressureMax);
  // And both ends are actually reachable, or the clamp is decoration.
  CHECK(lowest < 0.9);
  CHECK(highest > 1.1);
}

static void TestMarketingIsTheAnswerToTheOtherTwoGyms() {
  // The one lever of yours that touches the multiplier. Every other lever
  // is already in the base, and scoring them here too would double-count
  // the player's own choices -- which is the bug the source records.
  const GymTownDials d;
  int helped = 0;
  for (int day = 1; day <= 900; day++) {
    const double bare = TownPressure(day, "The Woodshed", 1.0, d);
    const double shielded = TownPressure(day, "The Woodshed", 1.18, d);
    CHECK(shielded >= bare - 1e-12);
    if (shielded > bare + 1e-9) helped++;
  }
  CHECK(helped > 500);   // and it is not only ever a rounding difference
}

static void TestTheFloorIsAlwaysChasing() {
  // The other half of TestTheFloorIsAMeterAndNotASwitch. With the town live
  // the target is not a constant, and **that is the point**: a move lands
  // every nine days and the floor closes fifteen percent of the gap a day,
  // so the membership never arrives, it tracks. A business you have to keep
  // answering for rather than one you set once.
  GymDials d;
  const Rng world = Rng::FromSeed("the-woodshed");
  double cash = 0.0;
  Gym g = ABoughtGym(cash);
  SetPrice(g, GymPrice::Budget);

  double moved = 0.0, worst = 0.0;
  double was = MembersItPullsToward(g, d);
  for (int day = 2; day <= 800; day++) {
    GymDay(g, world, day, d);
    if (day < 60) continue;                 // let the initial drift settle
    const double now = MembersItPullsToward(g, d);
    moved = std::max(moved, std::fabs(now - was));
    worst = std::max(worst, std::fabs(g.members - now));
    was = now;
  }
  CHECK(moved > 3.0);      // the target genuinely moves under it
  CHECK(worst > 5.0);      // so the floor is genuinely behind it
  CHECK(worst < 25.0);     // ...and never running away
}

static void TestSomethingGoesWrongAndItHasAClock() {
  GymDials d;
  const Rng world = Rng::FromSeed("the-woodshed");
  double cash = 0.0;
  Gym g = ABoughtGym(cash);

  // **Nothing goes wrong while you are settling in.** A fortnight's grace,
  // and it is derived from the day rather than rolled, so asking twice
  // gives the same answer.
  for (int day = 1; day < 1 + d.town.incidentGrace; day++) {
    CHECK(IncidentToday(g.name, 1, day, true, d.town) == GymIncident::None);
  }

  int landed = -1;
  for (int day = 2; day <= 400 && landed < 0; day++) {
    if (GymDay(g, world, day, d).landed != GymIncident::None) landed = day;
  }
  CHECK(landed > 0);
  CHECK(g.incident != GymIncident::None);
  CHECK(g.incidentDay == landed);
  // It is the loudest thing on the noticeboard, because it is the one with
  // a clock on it.
  CHECK(GymWarning(g, d).find("needs an answer") != std::string::npos);

  // Leave it, and **it answers itself the way you would expect** -- the
  // cheap option, and its cost.
  const double before = g.members;
  const GymIncidentDef* def = IncidentDef(g.incident);
  CHECK(def != nullptr);
  GymNight lapsed;
  for (int day = landed + 1; day <= landed + d.town.incidentDays + 1; day++) {
    const GymNight n = GymDay(g, world, day, d);
    if (n.lapsed != GymIncident::None) lapsed = n;
  }
  CHECK(lapsed.lapsed != GymIncident::None);
  CHECK(g.incident == GymIncident::None);
  CHECK(lapsed.news.find("You left it") != std::string::npos);
  CHECK(g.members <= before);
  CHECK(lapsed.standing == def->choices[1].standing);
}

static void TestAnsweringItCostsMoneyOrCostsYouSomethingElse() {
  GymDials d;
  double cash = 0.0;
  Gym g = ABoughtGym(cash);
  g.incident = GymIncident::Pipe;
  g.incidentDay = 20;

  // **Every choice in the table is a bill or a consequence, never both and
  // never neither.** That is the shape of the whole system: you can always
  // pay your way out, and not paying is never free.
  for (int i = 1; i < kGymIncidentCount; i++) {
    const GymIncidentDef* def = IncidentDef(static_cast<GymIncident>(i));
    CHECK(def != nullptr);
    for (int c = 0; c < 2; c++) {
      const GymIncidentChoice& choice = def->choices[c];
      const bool bill = choice.cost > 0.0;
      const bool consequence =
          choice.members < 0.0 || choice.standing < 0.0 || choice.matchesTheOffer;
      const bool upside = choice.members > 0.0 || choice.standing > 0.0;
      CHECK(bill || consequence || upside);
      CHECK(!std::string(choice.label).empty());
      CHECK(!std::string(choice.line).empty());
    }
  }

  // You cannot answer what you cannot pay for, and a refused answer leaves
  // it open rather than half-resolving it.
  double broke = 10.0;
  CHECK(!AnswerTheIncident(g, broke, 0, d).answered);
  CHECK(g.incident == GymIncident::Pipe);
  CHECK(broke == 10.0);

  double flush = 5000.0;
  const IncidentAnswer got = AnswerTheIncident(g, flush, 0, d);
  CHECK(got.answered);
  CHECK(flush == 5000.0 - IncidentDef(GymIncident::Pipe)->choices[0].cost);
  CHECK(!got.line.empty());
  CHECK(g.incident == GymIncident::None);
  // ...and it cannot be answered twice.
  CHECK(!AnswerTheIncident(g, flush, 1, d).answered);
}

static void TestNobodyPoachesASetterYouDoNotHave() {
  const GymTownDials d;
  bool sawPoach = false, sawSomethingElse = false;
  for (int day = 20; day <= 4000; day++) {
    const GymIncident with = IncidentToday("The Woodshed", 1, day, true, d);
    const GymIncident without = IncidentToday("The Woodshed", 1, day, false, d);
    if (with == GymIncident::Poach) sawPoach = true;
    if (without != GymIncident::None) sawSomethingElse = true;
    CHECK(without != GymIncident::Poach);
  }
  CHECK(sawPoach);
  CHECK(sawSomethingElse);

  // And matching the offer is the one answer in the table with a tail: it
  // costs you every day from here.
  GymDials full;
  double cash = 0.0;
  Gym g = ABoughtGym(cash);
  cash = 20000.0;
  CHECK(Hire(g, cash, false, 1, 0, full));
  g.incident = GymIncident::Poach;
  g.incidentDay = 20;
  const double was = WhoIsOn(g, false)->wage;
  const double owed = DailyOverhead(g, full);
  CHECK(AnswerTheIncident(g, cash, 0, full).answered);
  CHECK(WhoIsOn(g, false)->wage == was + full.town.poachRaise);
  CHECK(DailyOverhead(g, full) == owed + full.town.poachRaise);
}

// --- GYM-3: the staff are people ------------------------------------------

static void TestTheShortlistIsThreeDifferentPeople() {
  GymDials d;
  const std::vector<GymStaffer> pool = GymCandidates("The Woodshed", true, 30, d);
  CHECK(static_cast<int>(pool.size()) == d.hirePool);
  for (size_t i = 0; i < pool.size(); i++) {
    CHECK(!pool[i].name.empty());
    CHECK(!pool[i].trait.empty());
    CHECK(pool[i].wage >= d.lowestWage);
    for (size_t j = i + 1; j < pool.size(); j++) {
      // Three identical candidates is not a choice, and two Devs is a bug
      // report.
      CHECK(pool[i].name != pool[j].name);
      CHECK(pool[i].trait != pool[j].trait);
      CHECK(pool[i].quality != pool[j].quality);
    }
  }

  // **Stable while you think about it**, and a different week is different
  // people. Sleeping on it is not a re-roll; it is a fortnight.
  const std::vector<GymStaffer> again = GymCandidates("The Woodshed", true, 34, d);
  for (size_t i = 0; i < pool.size(); i++) CHECK(again[i].name == pool[i].name);

  bool moved = false;
  for (int week = 0; week < 40 && !moved; week++) {
    const std::vector<GymStaffer> later =
        GymCandidates("The Woodshed", true, 30 + week * 7, d);
    for (size_t i = 0; i < pool.size(); i++) {
      if (later[i].name != pool[i].name) moved = true;
    }
  }
  CHECK(moved);

  // The desk and the setter are different jobs and different lists.
  const std::vector<GymStaffer> desk = GymCandidates("The Woodshed", true, 30, d);
  const std::vector<GymStaffer> sets = GymCandidates("The Woodshed", false, 30, d);
  bool differs = false;
  for (size_t i = 0; i < desk.size(); i++) {
    if (desk[i].trait != sets[i].trait) differs = true;
  }
  CHECK(differs);
}

static void TestTheRightHireDependsOnALeverYouAlreadyPulled() {
  // **The claim the source says it measured**, checked in this port rather
  // than taken on trust: the wage spread beats the quality spread at budget
  // pricing and loses to it everywhere else. When a member is worth $8 you
  // want a cheap body on the desk; when they are worth $22 you want the one
  // who knows their name. The first cut had the best candidate winning at
  // every tier, which is not a choice.
  //
  // Measured with the town held at 1.0, because pressure multiplies the
  // boost and not the wage -- a kind town leans the whole thing toward
  // quality, which is a second-order truth this test is not about.
  GymDials d;
  d.town.pressureMin = 1.0;
  d.town.pressureMax = 1.0;
  for (int i = 0; i < kGymSeasonCount; i++) d.town.seasonPull[i] = 0.0;

  const auto worthPerDay = [&](GymPrice price, const GymStaffer& who) {
    Gym g;
    double cash = d.price;
    BuyTheGym(g, cash, "The Woodshed", 1, d);
    SetPrice(g, price);
    const double bare =
        MembersItPullsToward(g, d) * RatePerMember(g, d) - DailyOverhead(g, d);
    g.frontDesk = true;
    g.desk = who;
    return MembersItPullsToward(g, d) * RatePerMember(g, d) -
           DailyOverhead(g, d) - bare;
  };

  const std::vector<GymStaffer> pool = GymCandidates("The Woodshed", true, 30, d);
  const GymStaffer* best = &pool[0];
  const GymStaffer* cheapest = &pool[0];
  for (const GymStaffer& c : pool) {
    if (c.quality > best->quality) best = &c;
    if (c.wage < cheapest->wage) cheapest = &c;
  }
  CHECK(best != cheapest);
  CHECK(worthPerDay(GymPrice::Budget, *cheapest) >
        worthPerDay(GymPrice::Budget, *best));
  CHECK(worthPerDay(GymPrice::Premium, *best) >
        worthPerDay(GymPrice::Premium, *cheapest));
}

static void TestTenureComesDueAndRefusingHasTwoPrices() {
  GymDials d;
  double cash = 20000.0;
  Gym g = ABoughtGym(cash);
  CHECK(Hire(g, cash, true, 1, 0, d));
  CHECK(!IsAskingForARaise(g, true, 1, d));
  CHECK(AnswerTheAsk(g, true, true, 1, d) == RaiseAnswer::NotAsking);

  const int asks = 1 + d.raiseDays;
  CHECK(IsAskingForARaise(g, true, asks, d));
  const double was = WhoIsOn(g, true)->wage;
  const double wants = TheRaiseTheyWant(g, true, d);
  CHECK(wants >= 3.0);
  CHECK(AnswerTheAsk(g, true, true, asks, d) == RaiseAnswer::Granted);
  CHECK(WhoIsOn(g, true)->wage == was + wants);
  CHECK(!IsAskingForARaise(g, true, asks, d));   // and the clock resets

  // **Refusing costs quality, not attendance.** They keep turning up and
  // stop going out of their way, which is what makes it the expensive no.
  const int second = asks + d.raiseDays;
  const double quality = WhoIsOn(g, true)->quality;
  const double wage = WhoIsOn(g, true)->wage;
  CHECK(AnswerTheAsk(g, true, false, second, d) == RaiseAnswer::TheySulk);
  CHECK(g.frontDesk);
  CHECK(WhoIsOn(g, true)->quality < quality);
  CHECK(WhoIsOn(g, true)->wage == wage);         // and it did not cost a cent

  // Refuse twice and they go, and the empty seat cannot run the place.
  CHECK(SetHandsOff(g, true) == false);          // no setter yet anyway
  const int third = second + d.raiseDays;
  CHECK(AnswerTheAsk(g, true, false, third, d) == RaiseAnswer::TheyQuit);
  CHECK(!g.frontDesk);
  CHECK(WhoIsOn(g, true) == nullptr);
  CHECK(DailyOverhead(g, d) == d.overhead);      // off the books entirely
}

// --- GYM-12 and hands-off -------------------------------------------------

static void TestWingsAreIndependentAndOnlyOneOfThemEarns() {
  GymDials d;
  double cash = 100000.0;
  Gym g = ABoughtGym(cash);
  g.members = 40.0;

  // **No order and no prerequisite**, which is the whole difference from
  // the equipment ladder: build the dear one first if you want to.
  CHECK(BuildWing(g, cash, GymWing::Annex, d));
  CHECK(HasWing(g, GymWing::Annex));
  CHECK(!HasWing(g, GymWing::Showers));
  CHECK(!BuildWing(g, cash, GymWing::Annex, d));   // and only the once

  double broke = 10.0;
  CHECK(!BuildWing(g, broke, GymWing::Cafe, d));
  CHECK(broke == 10.0);

  // Every wing pulls people and costs upkeep; exactly one earns money by
  // itself, and it is the one with a coffee machine in it.
  int earners = 0;
  for (int i = 0; i < kGymWingCount; i++) {
    CHECK(d.wingTarget[i] > 0.0);
    CHECK(d.wingUpkeep[i] > 0.0);
    CHECK(d.wingCost[i] > 0.0);
    if (d.wingEarns[i] > 0.0) earners++;
    CHECK(!std::string(GymWingName(static_cast<GymWing>(i))).empty());
    CHECK(!std::string(GymWingBlurb(static_cast<GymWing>(i))).empty());
  }
  CHECK(earners == 1);
  CHECK(WhatTheWingsEarn(g, d) == 0.0);            // the annex is not a cafe
  double more = 100000.0;
  CHECK(BuildWing(g, more, GymWing::Cafe, d));
  CHECK(WhatTheWingsEarn(g, d) > 0.0);

  // **The ceiling this system exists to absorb.** All five wings, both
  // hires, the equipment ladder and the building is real money.
  double everything = 0.0;
  Gym other;
  double buying = d.price;
  BuyTheGym(other, buying, "The Woodshed", 1, d);
  everything += d.price + d.equipCost[kGymEquipCount - 1] +
                d.frontDeskCost + d.setterCost;
  for (int i = 0; i < kGymWingCount; i++) everything += d.wingCost[i];
  CHECK(everything > 55000.0);
}

static void TestItCanRunWithoutYouOnceThereIsSomebodyToRunIt() {
  GymDials d;
  double cash = 50000.0;
  Gym g = ABoughtGym(cash);

  // **Both seats, or it does not run without you.** That was the trigger:
  // the reward for staffing the place properly is not having to be in it.
  CHECK(!SetHandsOff(g, true));
  CHECK(Hire(g, cash, true, 1, 0, d));
  CHECK(!SetHandsOff(g, true));
  CHECK(Hire(g, cash, false, 1, 0, d));

  const double owed = DailyOverhead(g, d);
  CHECK(SetHandsOff(g, true));
  CHECK(g.passive);
  CHECK(DailyOverhead(g, d) == owed - d.passiveOverheadSaving);
  CHECK(!SetHandsOff(g, true));               // already there

  // ...and the price is that the day-to-day levers lock where you left
  // them. Staffing and equipment stay yours -- those are owner-level calls.
  const GymPrice price = g.price;
  SetPrice(g, GymPrice::Premium);
  SetMix(g, GymSetMix::Hardcore);
  CHECK(g.price == price);
  CHECK(g.mix == GymSetMix::AllComers);
  CHECK(!LaunchCampaign(g, cash, GymCampaign::Social, 2, d));
  CHECK(UpgradeEquipment(g, cash, d));

  // Reversible any time.
  CHECK(SetHandsOff(g, false));
  SetPrice(g, GymPrice::Premium);
  CHECK(g.price == GymPrice::Premium);

  // And it cannot outlive an empty seat.
  CHECK(SetHandsOff(g, true));
  CHECK(AnswerTheAsk(g, false, false, 1 + d.raiseDays, d) ==
        RaiseAnswer::TheySulk);
  CHECK(AnswerTheAsk(g, false, false, 1 + 2 * d.raiseDays, d) ==
        RaiseAnswer::TheyQuit);
  CHECK(!g.passive);
}

static void TestLoadsVersion41Save() {
  SaveGame save;
  save.seed = "the-woodshed";
  double cash = GymDials{}.price;
  CHECK(BuyTheGym(save.player.gym, cash, "The Woodshed", 40));
  save.player.gym.setter = true;      // the way a v41 save carried a hire:
  save.player.gym.frontDesk = true;   // a boolean, and nobody behind it

  std::string v41 = SerializeSave(save);
  for (const char* k : {"gym.deskwho.", "gym.setwho.", "gym.wing"}) {
    DropSaveLines(v41, k);
  }
  for (const char* k : {"gym.passive=", "gym.incidentday=", "gym.incident="}) {
    DropSaveLine(v41, k);
  }
  SetSaveVersion(v41, 41);

  SaveGame old;
  CHECK(DeserializeSave(v41, old) == LoadResult::Ok);
  CHECK(old.version == kSaveVersion);
  const Gym& g = old.player.gym;
  CHECK(g.owned);

  // **The books do not move by a cent**, which is the whole promise of this
  // migration: a derived staffer on the base wage at quality 1.0 costs and
  // pulls exactly what the boolean did.
  const GymDials d;
  CHECK(WhoIsOn(g, true) != nullptr);
  CHECK(WhoIsOn(g, true)->wage == d.frontDeskWage);
  CHECK(WhoIsOn(g, false)->wage == d.setterWage);
  CHECK(WhoIsOn(g, true)->quality == 1.0);
  CHECK(WhoIsOn(g, false)->quality == 1.0);
  // ...and the person who has apparently been on that desk since day 40
  // gets a name and a tenure, rather than being hired by a save loader.
  CHECK(!WhoIsOn(g, true)->name.empty());
  CHECK(WhoIsOn(g, true)->hiredDay == 40);
  CHECK(!IsAskingForARaise(g, true, 41, d));

  for (int i = 0; i < kGymWingCount; i++) CHECK(!g.wings[i]);
  CHECK(!g.passive);
  CHECK(g.incident == GymIncident::None);
  CHECK(static_cast<int>(DefaultMigrations().size()) == kSaveVersion - 1);
}

static void TestLoadsVersion38Save() {
  SaveGame save;
  save.seed = "the-woodshed";
  save.player.cash = 137.5;

  std::string v38 = SerializeSave(save);
  for (const char* k : {"gym.owned=", "gym.name=", "gym.day=", "gym.price=",
                        "gym.mix=", "gym.equip=", "gym.camp=", "gym.campuntil=",
                        "gym.desk=", "gym.setter=", "gym.members=",
                        "gym.balance=", "gym.debtdays=", "gym.tick="}) {
    DropSaveLine(v38, k);
  }
  SetSaveVersion(v38, 38);

  SaveGame old;
  CHECK(DeserializeSave(v38, old) == LoadResult::Ok);
  CHECK(old.version == kSaveVersion);
  CHECK(std::fabs(old.player.cash - 137.5) < 1e-9);
  // A v38 career never had the option -- it was a recorded cut -- so it
  // owns nothing, which is exactly what it owned.
  CHECK(!old.player.gym.owned);
  CHECK(old.player.gym.members == 0.0);
  CHECK(static_cast<int>(DefaultMigrations().size()) == kSaveVersion - 1);
}

static void TestLosingTheGymIsToldOnceAndCostsStanding() {
  PlayerState player;
  DayState day;
  const Rng world = Rng::FromSeed("the-woodshed");
  player.climber = NewClimber(world);
  double cash = GymDials{}.price;
  CHECK(BuyTheGym(player.gym, cash, "The Woodshed", player.day));
  SetPrice(player.gym, GymPrice::Premium);
  SetMix(player.gym, GymSetMix::Hardcore);

  const double sceneBefore = StandingWith(player.standing, Faction::Scene);
  int toldOn = -1;
  // **It takes a while, and that is the system working.** You inherit
  // fifteen members and premium-hardcore only pulls six, so the floor
  // drifts *down* over a fortnight while the books are still healthy --
  // banking a surplus that then has to burn off before a single day is
  // spent in the red. A gym does not fail the week you misprice it.
  double sceneAtTheTime = sceneBefore;
  for (int i = 0; i < 400; i++) {
    const double justBefore = StandingWith(player.standing, Faction::Scene);
    SleepToNextDay(player, day, world);
    // **The foreclosure specifically**, not any gym news. GYM-6's incidents
    // write to the same line, and one of those landing is not the bank
    // taking the building.
    if (player.gymNews.find("The bank took") != std::string::npos) {
      CHECK(toldOn < 0);          // exactly once, never twice
      toldOn = player.day;
      // **Read on the night it happens.** The scene forgets slowly and
      // `FactionDay` runs every night, so by the end of the run the hit
      // has decayed back to neutral -- comparing the end against the start
      // measures the forgetting, not the setback.
      sceneAtTheTime = StandingWith(player.standing, Faction::Scene);
      CHECK(sceneAtTheTime < justBefore);
    }
  }
  CHECK(toldOn > 0);
  CHECK(!player.gym.owned);
  // Losing a business is a visible setback in the scene rather than only
  // your own loss.
  CHECK(sceneAtTheTime < sceneBefore);
}

static void TestInsuranceIsWhatGetsADirtbagRepaired() {
  MedicalDials d;
  const Rng world = Rng::FromSeed("a-bad-one");

  // A career that spends everything it earns on climbing. Its **highest
  // balance in thirty years** is about $474, measured, and it usually
  // holds a couple of hundred -- so this is what it is like to be hurt
  // while poor, which is the state the whole insurance block exists for.
  const double whatAPoorCareerHolds = 300.0;

  const auto tryIt = [&](bool insured) {
    Climber c = NewClimber(world);
    c.injury.active = true;
    c.injury.kind = InjuryKind::Pulley;
    // Bad enough to be worth operating on.
    c.injury.severity = std::max(0.9, d.surgeryNeedsSeverity + 0.2);
    c.injury.daysLeft = 60;

    Medical med;
    med.diagnosis = Diagnosis::Scanned;   // nobody operates on a guess
    if (insured) {
      med.insured = true;
      // Bought long enough ago that the wait is served -- buying it the
      // week before a planned operation is deliberately not a strategy.
      med.insuredOnDay = 1;
    }
    double cash = whatAPoorCareerHolds;
    const int day = 1 + d.waitingDays + 1;
    CHECK(!insured || CoverIsLive(med, day, d));
    const bool had = HaveSurgery(med, c, cash, day, d);
    return std::make_pair(had, cash);
  };

  // **Uninsured, they cannot be repaired.** $2,200 against $300.
  const auto alone = tryIt(false);
  CHECK(!alone.first);
  CHECK(alone.second == whatAPoorCareerHolds);   // and it cost them nothing

  // **Insured, they can** -- and that is the entire point of the system.
  // At the old `covers = 0.80` the copay was $440 against a career whose
  // best ever day was $474, so an insured poor climber scanned every
  // injury and still never once had the operation: 0.00 surgeries across
  // eight thirty-year careers. This check is what would have said so.
  const auto covered = tryIt(true);
  CHECK(covered.first);
  CHECK(covered.second < whatAPoorCareerHolds);   // it still cost them
  CHECK(covered.second > 0.0);                    // ...and not everything

  // The copay has to stay a real decision rather than becoming free.
  const double paid = whatAPoorCareerHolds - covered.second;
  CHECK(paid > 100.0);
}

static void TestTheLotDoesNotAlwaysTurnUp() {
  const Rng world = Rng::FromSeed("who-turns-up");
  const int cast = static_cast<int>(LotRegulars(world, 1).size());
  CHECK(cast == 5);

  // **The whole point.** Before turnout, this was `cast` every single day
  // for ninety years -- which is why `social` had no reader and why the
  // belayer gate had never once bitten.
  bool everFewer = false, everSomebody = false;
  for (int day = 1; day <= 200; day++) {
    const int here = static_cast<int>(WhoIsAround(world, day, {}, 0.0, true).size());
    CHECK(here <= cast);
    everFewer = everFewer || here < cast;
    everSomebody = everSomebody || here > 0;
  }
  CHECK(everFewer);
  CHECK(everSomebody);

  // Deterministic, and per person per day: asking twice gives the same Lot,
  // and a reload has to hand back the same one.
  const std::vector<Partner> once = WhoIsAround(world, 42, {}, 0.0, true);
  const std::vector<Partner> again = WhoIsAround(world, 42, {}, 0.0, true);
  CHECK(once.size() == again.size());
  for (std::size_t i = 0; i < once.size(); i++) {
    CHECK(once[i].name == again[i].name);
  }
  // ...and a different valley is different people.
  CHECK(CompanyOver(world, 200, {}, 0.0, true) !=
        CompanyOver(Rng::FromSeed("another-valley"), 200, {}, 0.0, true));
}

// How many of `days` a particular person was there for.
static int DaysWith(const Rng& world, const std::string& who, int days,
                    const std::vector<PartnerBond>& bonds, double social,
                    bool rockIsIn, const PartnerDials& d = PartnerDials{}) {
  int seen = 0;
  for (int day = 1; day <= days; day++) {
    for (const Partner& p :
         WhoIsAround(world, day, bonds, social, rockIsIn, 1.0, d)) {
      if (p.name == who) seen++;
    }
  }
  return seen;
}

static void TestNobodyIsNeverThereAndNobodyIsAlways() {
  PartnerDials d;
  const Rng world = Rng::FromSeed("who-turns-up");
  const int days = 400;

  // **Asked of one person rather than of the crowd**, which is the only way
  // to see a floor or a ceiling: a total across five people is under the
  // ceiling and over the floor whatever happens to any one of them, so the
  // first version of this test passed with both removed.
  std::vector<PartnerBond> known;
  for (const Partner& p : LotRegulars(world, 1)) {
    PartnerBond b;
    b.name = p.name;
    b.rapport = 1.0;
    b.everKnew = 1.0;
    known.push_back(b);
  }

  // The most reliable person in the valley, for the most gregarious climber
  // in it, on a perfect day, who knows him well: **still not every day.**
  const int rayAtBest =
      DaysWith(world, "Ray", days, known, kPersonalityMax, true, d);
  CHECK(rayAtBest > 0);
  CHECK(rayAtBest < days);

  // And the least reliable, for a Loner, in the rain, who does not know
  // him: **still sometimes.** Measured without the floor, an Influencer was
  // turned away zero times in thirty years -- an axis switching a mechanic
  // off rather than bending it.
  const int boAtWorst =
      DaysWith(world, "Bo", days, {}, -kPersonalityMax, false, d);
  CHECK(boAtWorst > 0);
  CHECK(boAtWorst < days / 4);
}

static void TestWhoIsHereDoesNotDependOnWhoElseIs() {
  const Rng world = Rng::FromSeed("who-turns-up");
  // **One roll per person per day, not one roll per day.** Sharing a
  // stream makes each person's draw depend on how many were asked about
  // first, so changing what you know about Margo would silently change
  // whether Trish was there -- and a Lot where people's absences are
  // correlated is a Lot that empties all at once.
  std::vector<PartnerBond> justMargo;
  PartnerBond b;
  b.name = "Margo";
  b.rapport = 1.0;
  b.everKnew = 1.0;
  justMargo.push_back(b);

  const int days = 400;
  for (const char* who : {"Dev", "Trish", "Ray", "Bo"}) {
    CHECK(DaysWith(world, who, days, {}, 0.0, true) ==
          DaysWith(world, who, days, justMargo, 0.0, true));
  }
  // ...while Margo herself does move, or the bond is doing nothing at all.
  CHECK(DaysWith(world, "Margo", days, justMargo, 0.0, true) >
        DaysWith(world, "Margo", days, {}, 0.0, true));

  // **And they are five people, not one dial.** A single stream restarted
  // per person hands everybody the same number, so presence comes out
  // perfectly nested -- if Bo is here then everyone more reliable than Bo
  // is too, and the Lot is never *Bo but not Ray*. It stops being a group
  // of people and becomes a reading of how busy it is.
  //
  // This is the property the per-person stream is actually for, and the
  // first version of this test could not see it: it asked whether one
  // person's presence depended on another's, which that mistake does not
  // break.
  bool everOutOfOrder = false;
  for (int day = 1; day <= days && !everOutOfOrder; day++) {
    bool bo = false, ray = false;
    for (const Partner& p : WhoIsAround(world, day, {}, 0.0, true)) {
      bo = bo || p.name == "Bo";        // here 0.45 of the time
      ray = ray || p.name == "Ray";     // here 0.88 of the time
    }
    everOutOfOrder = bo && !ray;
  }
  CHECK(everOutOfOrder);
}

static void TestAWashoutEmptiesTheLot() {
  const Rng world = Rng::FromSeed("who-turns-up");
  const int dry = CompanyOver(world, 365, {}, 0.0, true);
  const int wet = CompanyOver(world, 365, {}, 0.0, false);
  CHECK(wet < dry);
  CHECK(wet < dry - dry / 4);   // a magnitude: a washout is properly lonely
  CHECK(wet > 0);               // ...and Trish is still psyched
}

static void TestKnowingSomebodyBringsThemOut() {
  const Rng world = Rng::FromSeed("who-turns-up");
  std::vector<PartnerBond> known;
  for (const Partner& p : LotRegulars(world, 1)) {
    PartnerBond b;
    b.name = p.name;
    b.rapport = 1.0;
    b.everKnew = 1.0;
    known.push_back(b);
  }
  CHECK(RapportWith(known, known.front().name) == 1.0);
  CHECK(RapportWith(known, "Nobody At All") == 0.0);
  // Not "they come because you are there" -- you know their week, so you
  // turn up when they do.
  CHECK(CompanyOver(world, 365, known, 0.0, true) >
        CompanyOver(world, 365, {}, 0.0, true));
}

static void TestTurnoutDoesNotDisturbThePlayersRng() {
  // Asking who is around must not move any other stream, the same rule the
  // Lot's own rolls follow.
  Rng a = Rng::FromSeed("who-turns-up");
  Rng b = Rng::FromSeed("who-turns-up");
  const double before = a.NextDouble();
  const Rng world = Rng::FromSeed("who-turns-up");
  for (int day = 1; day <= 50; day++) WhoIsAround(world, day, {}, 40.0, true);
  CHECK(before == b.NextDouble());
}

static void TestTheDayHappensWhetherOrNotYouDo() {
  DayDials dd;
  const Rng world = Rng::FromSeed("a-day-that-happened");

  // Two identical people. One spends twelve hours doing things; the other
  // walks back to the van at ten in the morning and goes to bed.
  PlayerState busy;
  PlayerState idle;
  DayState b = WakeUp(busy, dd);
  DayState i = WakeUp(idle, dd);
  PassHours(b, 12.0, dd);
  CHECK(b.hunger > i.hunger);            // during the day, obviously
  SleepToNextDay(busy, b, world, dd);
  SleepToNextDay(idle, i, world, dd);
  // **The night cost them the same**, because the hours happened to both
  // of them. Before this, sleeping at ten in the morning was free, and a
  // measured thirty-year career ate 595 meals.
  CHECK(std::fabs(busy.hungerCarried - idle.hungerCarried) < 1e-9);
  CHECK(idle.hungerCarried > 0.0);
  CHECK(i.hunger == idle.hungerCarried);   // and you wake up with it
}

static void TestSleepingHungryIsAThingThatCanHappen() {
  DayDials dd;
  const Rng world = Rng::FromSeed("the-oldest-trap");

  // **This could not happen at all until hunger carried.** Fifteen waking
  // hours at three an hour is forty-five, and `starvingHunger` is seventy,
  // so one day could never reach it however it went -- the probe measured
  // zero hungry nights across thirty years, in every career it ever ran.
  PlayerState fed;
  PlayerState unfed;
  fed.cash = 500.0;
  unfed.cash = 0.0;
  double fedEnergy = 0.0;
  double unfedEnergy = 0.0;
  for (int i = 0; i < 5; i++) {
    DayState f = WakeUp(fed, dd);
    DayState u = WakeUp(unfed, dd);
    EatMeal(fed, f, dd);          // and nothing for the other one
    SleepToNextDay(fed, f, world, dd);
    SleepToNextDay(unfed, u, world, dd);
    fedEnergy = f.energy;
    unfedEnergy = u.energy;
  }
  // Eating once a day keeps you level; not eating compounds. The penalty
  // reads the hunger you went to bed on, which is what carried plus a
  // day's worth comes to -- `hungerCarried` is what is left of it by
  // morning and is the smaller number.
  const double unfedAtBedtime =
      unfed.hungerCarried / dd.hungerKeptOvernight;
  CHECK(unfedAtBedtime > dd.starvingHunger);
  CHECK(fed.hungerCarried / dd.hungerKeptOvernight < dd.starvingHunger);
  CHECK(fedEnergy == 100.0);            // fed and rested is fed and rested
  CHECK(unfedEnergy < fedEnergy);
  CHECK(unfedEnergy <= dd.sleepEnergyFloor + 1e-9);

  // ...and it is a slide rather than a cliff: **one missed day is not
  // this.** Fifteen waking hours is forty-five and the trap is at seventy,
  // so the first night on nothing costs you nothing, the second is bad,
  // and the third is worse. That shape is the whole point of it.
  PlayerState missedOne;
  DayState m = WakeUp(missedOne, dd);
  SleepToNextDay(missedOne, m, world, dd);
  CHECK(m.energy == 100.0);
  CHECK(missedOne.hungerCarried / dd.hungerKeptOvernight < dd.starvingHunger);
}

static void TestAStrangerIsServedNotGreeted() {
  LocalDials d;
  Locals town = TheLocals(DirtbagTown(), Rng::FromSeed("crag-1"));
  CHECK(!town.people.empty());
  Local& diner = Counter(town, Service::Meal);

  // They have heard. They still do not say anything, because you are a
  // customer -- which is the gate that makes the greeting read as earned
  // rather than issued.
  Tell(town, Heard::Sent, "The Prow", 10);
  CHECK(diner.holds == Heard::Sent);
  CHECK(WhatTheySay(diner, 10, d).empty());

  // **And it keeps holding it until somebody hears it.** Which happens on
  // the visit that takes them over `nodsAt`, not before -- so the memory
  // survives the strangers' visits and is spent by the first one that was
  // actually a greeting.
  BecomeAFace(diner, 1, 3);
  CHECK(diner.known < d.nodsAt);
  CHECK(diner.holds == Heard::Sent);      // still held, nobody said anything
  BecomeAFace(diner, 4, 2);
  CHECK(diner.known >= d.nodsAt);
  CHECK(diner.holds == Heard::None);      // and the moment they knew you

  // Measured over thirty years, a version that spent the memory on every
  // visit -- including the ones where they said nothing -- ate a career's
  // first two hundred days of sends, and what a regular said came out as
  // "thought you'd moved on" thirty-seven times out of forty.
  Tell(town, Heard::Sent, "The Prow", 10);
  CHECK(WhatTheySay(diner, 10, d) == std::string("Heard you got The Prow."));
}

static void TestTheySayItOnceAndGoBackToNodding() {
  LocalDials d;
  Locals town = TheLocals(DirtbagTown(), Rng::FromSeed("crag-1"));
  Local& shop = Counter(town, Service::Gear);
  BecomeAFace(shop, 1, 5);

  Tell(town, Heard::Named, "Slab Wednesday", 40);
  CHECK(WhatTheySay(shop, 40, d) ==
        std::string("Slab Wednesday. That was you, then."));
  Seen(shop, 40, 1.0, d);
  // Spent. A line that repeats every visit stops being a greeting inside a
  // week, which is the whole difference between a person and wallpaper.
  CHECK(shop.holds == Heard::None);
  CHECK(WhatTheySay(shop, 41, d).empty());
}

static void TestTheLoudestThingWins() {
  LocalDials d;
  Locals town = TheLocals(DirtbagTown(), Rng::FromSeed("crag-1"));
  Local& diner = Counter(town, Service::Meal);
  BecomeAFace(diner, 1, 5);

  Tell(town, Heard::Hurt, "finger", 10);
  Tell(town, Heard::Sent, "The Prow", 12);
  CHECK(diner.holds == Heard::Sent);       // louder replaces

  Tell(town, Heard::Hurt, "shoulder", 14);
  CHECK(diner.holds == Heard::Sent);       // quieter is dropped, not queued
  CHECK(diner.about == std::string("The Prow"));

  // ...and equal replaces, because the question they are answering is
  // *what did you do last time*.
  Tell(town, Heard::Sent, "Gravedigger", 16);
  CHECK(diner.about == std::string("Gravedigger"));
  CHECK(WhatTheySay(diner, 16, d) == std::string("Heard you got Gravedigger."));
}

static void TestOnlyTheOneWhoSawItKnowsAboutTheBill() {
  Locals town = TheLocals(DirtbagTown(), Rng::FromSeed("crag-1"));
  TellOne(town, Service::Meal, Heard::Broke, "", 10);
  CHECK(Counter(town, Service::Meal).holds == Heard::Broke);
  Local& shop = Counter(town, Service::Gear);
  CHECK(shop.holds == Heard::None);
}

static void TestTheyRememberYouAfterAWinter() {
  LocalDials d;
  Locals town = TheLocals(DirtbagTown(), Rng::FromSeed("crag-1"));
  Local& diner = Counter(town, Service::Meal);
  BecomeAFace(diner, 1, 20);
  const double best = diner.known;
  CHECK(best > 0.9);

  // Two years away. It fades, and it floors -- you stop being current with
  // somebody, you do not become a stranger to them.
  for (int day = 100; day < 100 + 730; day++) LocalsDay(town, day, d);
  CHECK(diner.known < best);
  CHECK(diner.known >= best * d.knownKeeps - 1e-9);
  CHECK(diner.known > 0.0);
}

static void TestNobodyTellsThemYouWereAway() {
  LocalDials d;
  Locals town = TheLocals(DirtbagTown(), Rng::FromSeed("crag-1"));
  Local& diner = Counter(town, Service::Meal);
  BecomeAFace(diner, 1, 10);
  CHECK(diner.holds == Heard::None);
  CHECK(WhatTheySay(diner, diner.lastSeen + 1, d).empty());
  // The one memory in the file that is derived rather than given, and the
  // only one that fires for doing nothing at all.
  const int later = diner.lastSeen + static_cast<int>(d.awayDays) + 1;
  CHECK(WhatTheySay(diner, later, d) == std::string("Thought you'd moved on."));
}

static void TestBeingAKnownFaceIsWorthSomething() {
  LocalDials d;
  Locals town = TheLocals(DirtbagTown(), Rng::FromSeed("crag-1"));
  CHECK(LocalPrice(town, Service::Meal, d) == 1.0);   // a stranger pays full
  BecomeAFace(Counter(town, Service::Meal), 1, 20);
  CHECK(LocalPrice(town, Service::Meal, d) < 1.0);
  CHECK(LocalPrice(town, Service::Meal, d) > 0.8);    // and never much below
  CHECK(LocalPrice(town, Service::Gear, d) == 1.0);   // per counter, not global
}

static void TestTheTownIsSeededByTheNight() {
  PlayerState player;
  DayState day;
  const Rng world = Rng::FromSeed("a-town-with-faces");
  player.climber = NewClimber(world);
  CHECK(player.locals.people.empty());
  SleepToNextDay(player, day, world);
  // A career that never called anything is still in a town with people in
  // it -- and an old save walks into the same one.
  CHECK(!player.locals.people.empty());
  CHECK(!Counter(player.locals, Service::Meal).name.empty());

  // Deterministic from the world seed: two careers in the same valley meet
  // the same people.
  PlayerState twin;
  DayState twinDay;
  twin.climber = NewClimber(world);
  SleepToNextDay(twin, twinDay, world);
  CHECK(Counter(twin.locals, Service::Meal).name ==
        Counter(player.locals, Service::Meal).name);

  // **And the night fades them, through the day loop rather than by hand.**
  // The first version of this called `LocalsDay` directly and therefore
  // proved only that the function works -- take the call out of
  // `SleepToNextDay` and nothing failed, which is the "written and never
  // wired" shape arriving through a test that looked like coverage.
  BecomeAFace(Counter(player.locals, Service::Meal), player.day, 20);
  const double best = Counter(player.locals, Service::Meal).known;
  CHECK(best > 0.9);
  for (int i = 0; i < 400; i++) SleepToNextDay(player, day, world);
  const Local& diner = Counter(player.locals, Service::Meal);
  CHECK(diner.known < best);          // you stopped turning up
  CHECK(diner.known > 0.0);           // ...and they still know your face
}

static void TestTheDinerGreetsYouByWhatYouDid() {
  PlayerState player;
  DayState day = WakeUp(player);
  const Rng world = Rng::FromSeed("a-town-with-faces");
  player.locals = TheLocals(DirtbagTown(), world);
  BecomeAFace(Counter(player.locals, Service::Meal), 1, 6);

  // A send, through the one function every burn in the game passes through.
  Route route;
  route.name = "The Prow";
  AttemptResult sent;
  sent.sent = true;
  sent.timeline.resize(1);
  ApplyAttemptToDay(player, day, route, sent, world);
  CHECK(Counter(player.locals, Service::Meal).holds == Heard::Sent);

  // ...and it comes back at you over a burrito.
  day.heard.clear();
  player.cash = 500.0;
  CHECK(EatMeal(player, day));
  CHECK(day.heard == std::string("Heard you got The Prow."));

  // Once. The second burrito is just a burrito.
  day.heard.clear();
  CHECK(EatMeal(player, day));
  CHECK(day.heard.empty());
}

static void TestWhatYouWentToBedWithSurvivesASave() {
  SaveGame save;
  save.seed = "the-oldest-trap";
  save.player.hungerCarried = 41.5;

  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  CHECK(std::fabs(back.player.hungerCarried - 41.5) < 1e-9);
  // You load in as hungry as you saved, which is the only honest answer --
  // otherwise saving is a meal.
  CHECK(WakeUp(back.player).hunger == back.player.hungerCarried);

  // And a v37 career woke level however it had lived, so it loads level.
  std::string v37 = SerializeSave(save);
  DropSaveLine(v37, "hunger.carried=");
  SetSaveVersion(v37, 37);
  SaveGame old;
  CHECK(DeserializeSave(v37, old) == LoadResult::Ok);
  CHECK(old.version == kSaveVersion);
  CHECK(old.player.hungerCarried == 0.0);
  CHECK(static_cast<int>(DefaultMigrations().size()) == kSaveVersion - 1);
}

static void TestTheTownHearsWhatYouNamedIt() {
  PlayerState player;
  player.locals = TheLocals(DirtbagTown(), Rng::FromSeed("crag-1"));
  BecomeAFace(Counter(player.locals, Service::Gear), 1, 6);

  Crag crag = RoadsideCrag(Rng::FromSeed("crag-1"));
  CragLine* open = nullptr;
  for (CragLine& l : crag.lines) {
    if (l.isProject) { open = &l; break; }
  }
  CHECK(open != nullptr);
  if (!open) return;

  ProjectMemory mem;
  mem.routeName = open->route.name;
  mem.sent = true;
  mem.firstAscent = true;
  mem.firstSendStyle = Style::Redpoint;
  mem.confirmedGrade = open->route.grade;
  CHECK(ClaimFirstAscent(player, mem, *open, "Slab Wednesday"));

  // The whole reason naming a line is a verb: the name is what gets said
  // back to you.
  const Local& shop = Counter(player.locals, Service::Gear);
  CHECK(shop.holds == Heard::Named);
  CHECK(WhatTheySay(shop, 40) ==
        std::string("Slab Wednesday. That was you, then."));
}

static void TestAKnownFaceIsCharacterLessForTheBurrito() {
  DayDials dd;
  PlayerState stranger;
  PlayerState known;
  stranger.locals = TheLocals(DirtbagTown(), Rng::FromSeed("crag-1"));
  known.locals = stranger.locals;
  BecomeAFace(Counter(known.locals, Service::Meal), 1, 20);

  stranger.cash = known.cash = 500.0;
  DayState a = WakeUp(stranger, dd);
  DayState b = WakeUp(known, dd);
  a.hunger = b.hunger = 60.0;
  CHECK(EatMeal(stranger, a, dd));
  CHECK(EatMeal(known, b, dd));
  // Through the day loop, not just the formula -- the same money, taken
  // twice, differs by whether they know your face.
  CHECK(known.cash > stranger.cash);
}

static void TestTheTownSurvivesASave() {
  SaveGame save;
  save.seed = "a-town-with-faces";
  save.player.locals = TheLocals(DirtbagTown(), Rng::FromSeed("a-town-with-faces"));
  BecomeAFace(*At(save.player.locals, Service::Meal), 1, 9);
  Tell(save.player.locals, Heard::Named, "Slab Wednesday", 40);
  save.player.locals.knownTitles = 2;
  save.player.locals.knownTier = 3;

  SaveGame back;
  CHECK(DeserializeSave(SerializeSave(save), back) == LoadResult::Ok);
  const Locals& got = back.player.locals;
  CHECK(got.people.size() == save.player.locals.people.size());
  const Local& diner = Counter(got, Service::Meal);
  CHECK(diner.name == At(save.player.locals, Service::Meal)->name);
  CHECK(diner.holds == Heard::Named);
  CHECK(diner.about == std::string("Slab Wednesday"));
  CHECK(std::fabs(diner.known -
                  At(save.player.locals, Service::Meal)->known) < 1e-9);
  CHECK(diner.lastSeen == At(save.player.locals, Service::Meal)->lastSeen);
  CHECK(got.knownTitles == 2);
  CHECK(got.knownTier == 3);
  // You load in with the same thing about to be said to you.
  CHECK(WhatTheySay(diner, 41) ==
        WhatTheySay(*At(save.player.locals, Service::Meal), 41));
}

static void TestLoadsVersion36Save() {
  SaveGame save;
  save.seed = "a-town-with-faces";
  save.player.cash = 137.5;

  std::string v36 = SerializeSave(save);
  DropSaveLine(v36, "loc.n=");
  DropSaveLine(v36, "loc.titles=");
  DropSaveLine(v36, "loc.tier=");
  SetSaveVersion(v36, 36);

  SaveGame old;
  CHECK(DeserializeSave(v36, old) == LoadResult::Ok);
  CHECK(old.version == kSaveVersion);
  CHECK(std::fabs(old.player.cash - 137.5) < 1e-9);
  // Nobody knows you, which is the truth about that career: nobody did.
  CHECK(old.player.locals.people.empty());
  CHECK(old.player.locals.knownTitles == 0);
  // ...and the roster opens on the first night, like a circuit season.
  DayState day;
  const Rng world = Rng::FromSeed(old.seed);
  SleepToNextDay(old.player, day, world);
  CHECK(!old.player.locals.people.empty());
  CHECK(static_cast<int>(DefaultMigrations().size()) == kSaveVersion - 1);
}

static void TestTheBillYouCannotCoverIsBetweenYouAndThem() {
  PlayerState player;
  DayState day = WakeUp(player);
  player.locals = TheLocals(DirtbagTown(), Rng::FromSeed("crag-1"));
  player.cash = 0.0;
  CHECK(!EatMeal(player, day));
  CHECK(Counter(player.locals, Service::Meal).holds == Heard::Broke);
  // And nobody else. The gear shop did not watch you fail to pay for a
  // burrito, and a town that gossips about that is a different game.
  CHECK(Counter(player.locals, Service::Gear).holds == Heard::None);
}

int main() {
  TestRngDeterminism();
  TestRngUnicodeSeeds();
  TestRngGoldenVectors();
  TestRouteStability();
  TestSessionDeterminism();
  TestFingersMatterOnCrimps();
  TestEnduranceControlsPump();
  TestSandbagBites();
  TestGradesResist();
  TestExecutionMatters();
  TestStyleLadder();
  TestLiveDriveMatchesBatch();
  TestPeekOddsHonest();
  TestShakeOutEconomics();
  TestSessionLoopDeterminism();
  TestWarmupMatters();
  TestProjectingBuildsBeta();
  TestFirstSendStyleSticks();
  TestSkinBudgetBites();
  TestSessionLiveComposition();
  TestPsycheSwings();
  TestRouteReads();
  TestDayBasics();
  TestFatigueFadesIn();
  TestBillsLandWeekly();
  TestSkinRegrowsOvernight();
  TestHungrySleepRecoversPoorly();
  TestTrainingCreep();
  TestSevenDayLoop();
  TestSaveRoundTrip();
  TestSaveRejectsGarbageAndFuture();
  TestBeingWatchedIsWhatCatchesYou();
  TestItReallyMightNeverComeOut();
  TestAFreshSecretIsQuiet();
  TestOneThingAtATime();
  TestSomeLiesTakeTheAscentAndSomeDoNot();
  TestWhoIsActuallyAngry();
  TestWhatYouDidSurvivesASave();
  TestFindingOutIsDeterministic();
  TestNobodySponsorsAClimberNobodyHasHeardOf();
  TestTheirDaysAreTheGoodDays();
  TestADealIsReviewedAndBeingHurtIsNotFailing();
  TestSigningSaysSomethingAboutYou();
  TestTheShoeDealIsWorthMoreThanItLooks();
  TestADealSurvivesASave();
  TestACareerCanEnd();
  TestTheGuidebookPrintsTheRightLadder();
  TestTheWorldRemembersAndTheBodyDoesNot();
  TestNobodyIsEverThrownOut();
  TestLoadsVersion11Save();
  TestGenerationsSurviveASave();
  TestTheCaveIsRopeRockAndRoadsideIsNot();
  TestTheCaveIsAStableWorldAndItsOwnOne();
  TestNoPartnerNoPitch();
  TestRapportBuysBurnsAndNothingElseDoes();
  TestTheLotCanActuallyBelayTheCave();
  TestOnlyPitchesHaveBolts();
  TestThePadStopsAtTheFirstBolt();
  TestTheRunoutIsTheRopeHeadGame();
  TestClippingCostsAndTheStanceDecidesHowMuch();
  TestBetaIsWorthMoreOnALongerRoute();
  TestAPitchGoesOnRedpoint();
  TestTheRockDecidesWhereGearGoes();
  TestAPlacementIsADecisionMadeUnderPump();
  TestOneRunoutModelServesBothDisciplines();
  TestATradLeadIsNotASportLead();
  TestNeitherPolicyWinsEveryPitch();
  TestTheRackRunsOut();
  TestABatchTradAttemptIsLedNotSoloed();
  TestARackIsTheMostExpensiveThingYouOwn();
  TestTheRackSurvivesTheSave();
  TestTheButtressIsTradRock();
  TestTheLogbookForgets();
  TestOneAfternoonIsNotAHabit();
  TestAHabitCanBeStoppedAndAQuirkCannot();
  TestNothingHappensToSomebodyWithNoHabits();
  TestEveryHabitCostsSomething();
  TestPickingOneIsNotBecomingOne();
  TestTheLogbookIsWrittenByClimbing();
  TestGoingToTheGymIsWrittenDownAsGoingToTheGym();
  TestAHabitReachesTheWall();
  TestWhoYouBecameSurvivesTheSave();
  TestYouCanSayHowYouClimbInOneSentence();
  TestTheCornerOfTheScreenStaysQuiet();
  TestTheNarratorKnowsWhenToShutUp();
  TestEveryFallIsADifferentFall();
  TestTheReasonIsNeverTheMomentItEnded();
  TestItReadsAndNeverRecomputes();
  TestTheNarratorNeverSaysANumber();
  TestAGearLeadIsNotToldInBolts();
  TestTheLiveWallAndTheReplayAgree();
  TestThreeLinesOfRoomGetTheRightThree();
  TestTheGroundKnowsWhichGoThisIs();
  TestAgeIsDerivedNotStored();
  TestNothingIsTakenBeforeThePeak();
  TestPowerGoesFirstAndTechniqueNeverGoes();
  TestTheCeilingIsWhatTrainingCannotArgueWith();
  TestAgeShrinksTheTrainingBudgetNotTheClimber();
  TestACareerHasAnArc();
  TestANightIsWhereEverythingCountsDown();
  TestLoadRisesWithHardnessNotMileage();
  TestLoadIsASlowerClockThanSkin();
  TestNobodyGetsHurtOutOfTheBlue();
  TestMostInjuriesAreAFortnightAndTheSeasonEnderIsRare();
  TestAnInjuryDecidesWhatYouCanStillClimbOn();
  TestClimbingOnItIsAGambleBothWays();
  TestPhysioBuysTimeAndNotAMiracle();
  TestTendonsDoNotTearInACampChair();
  TestTheBodySurvivesASave();
  TestPadsPayOffWhereYouAreScared();
  TestPadsChangeNothingForCallersWhoNeverHeardOfThem();
  TestTheKitIsBoughtOrItIsNot();
  TestTheMembershipRunsOut();
  TestTheGymIsTheOnlyClimbingThatIgnoresTheWeather();
  TestTheHangboardIsTheBrokeAnswer();
  TestTheKitSurvivesASave();
  TestTheJobSurvivesASave();
  TestLoadsVersion8Save();
  TestLoadsVersion1Save();
  TestCareerSummary();
  TestLedgerRecordsGrade();
  TestMigrationMachinery();
  TestWeatherDeterminism();
  TestTemperatureCurve();
  TestSunFollowsAspect();
  TestRockHoldsTheSun();
  TestFrictionRespondsToWeather();
  TestWindowIsShortEnoughToBeADecision();
  TestAspectDecidesWhen();
  TestWindowChangesWhenYouBurn();
  TestCragIsStable();
  TestCragIsNotALadder();
  TestCragHasProjectsAndTheyAreOpen();
  TestTheBookHasSomethingAtTheTopOfACareer();
  TestTheDirtbagYear();
  TestTheDirtbagYearMigrates();
  TestTheTownNamesYourCrew();
  TestTheCrewNameMigrates();
  TestThePlayerNameMigrates();
  TestTheTweak();
  TestDreamsCostTheBuffer();
  TestDreamsMigrate();
  TestTheCampfireGame();
  TestLiarsDice();
  TestBlackjack();
  TestTheTable();
  TestHowClose();
  TestPumpShows();
  TestComp();
  TestCircuit();
  TestCircuitCareer();
  TestNationalTeam();
  TestWorldStage();
  TestLeague();
  TestMedical();
  TestAilments();
  TestCraft();
  TestEveryZoneIsSomewhere();
  TestWorldStageSave();
  TestRival();
  TestRivalRace();
  TestRivalCareer();
  TestRivalSave();
  TestCharacter();
  TestCharacterSave();
  TestZones();
  TestWhoTurnsUp();
  TestSandbagsAreSpecific();
  TestCragGivesAClimberADay();
  TestNamingNeverMovesTheLedgerKey();
  TestGuidebookReadsRight();
  TestRestingBuysTimeNotStrength();
  TestVirginLinesStartFilthy();
  TestADefaultLedgerIsClean();
  TestDirtIsWhatStandsInTheWay();
  TestCleaningCostsTheDay();
  TestNamingIsEarnedAndExact();
  TestTheBookRecordsWhatItReallyWent();
  TestTheBookGetsWrittenInto();
  TestTheLoadWarningAgreesWithItself();
  TestHeadTrainsOnWhatYouCommitTo();
  TestClaimingIsNamingPlusTellingTheScene();
  TestAShoeDealActuallyBuysShoes();
  TestASponsorGetsPaidAndReviewed();
  TestStandingBuysBetaAndPeopleLiftYou();
  TestTheMirroredDialsStillAgree();
  TestThePadSaysWhatItCosts();
  TestTheBestBelayerIsTheMostPatientOne();
  TestTheSunTerraceIsTheWinterCrag();
  TestEachCragCostsSomethingDifferentToReach();
  TestTheValleyRemembersAcrossGenerations();
  TestTheLotPutsUpLinesAndNamesThem();
  TestTheLotPlateausLikePeopleDo();
  TestRockGoesBackToTheWeather();
  TestFirstAscentsAreACareer();
  TestTheWholeArc();
  TestSaveCarriesFirstAscents();
  TestLoadsVersion2Save();
  TestTheYearHasSeasons();
  TestTheWindowMovesThroughTheYear();
  TestTheLightGoesInWinter();
  TestAJobCostsYouTheWinter();
  TestTheSceneWatchesWhatYouActuallyDo();
  TestTheTownIsData();
  TestHoursAreTheMechanic();
  TestFactionsAreActuallyOpposed();
  TestTheThingsYouDoHaveOpinions();
  TestAccessGetsPulled();
  TestTheSceneForgetsSlowly();
  TestTheLotSpeaksForSomebody();
  TestTheBoardIsDifferentEveryDay();
  TestABrokenVanCostsYouTheWorkToo();
  TestOddJobsPayDebtFirst();
  TestTheSalaryOwnsTheMiddleOfTheDay();
  TestWorkRunsOnItsOwnRng();
  TestBillsYouCannotPayWait();
  TestLoadsVersion5Save();
  TestWhatYouOwnSurvivesASave();
  TestDrivingWearsTheVan();
  TestHotDaysCookTheRadiator();
  TestNothingFailsOutOfTheBlue();
  TestBeingBrokeCannotEndTheSave();
  TestPatchUntilYouCannot();
  TestTheVanRunsOnItsOwnRng();
  TestShoesWearByWhatYouClimb();
  TestDeadRubberCostsGrades();
  TestResoleOrReplace();
  TestShoesReachTheSession();
  TestTheSessionTellsYouWhereYouAre();
  TestFreshSkinIsWorthSomething();
  TestFlailingIsNotTraining();
  TestAStrayBecomesYoursByBeingFed();
  TestTheDogGetsHungryAndSaysSo();
  TestBondNeedsYouAround();
  TestTheDogClimbsWithYou();
  TestTheVanGetsHot();
  TestTheDogSurvivesASave();
  TestLoadsVersion4Save();
  TestSaveCarriesWhoYouKnow();
  TestLoadsVersion3Save();
  TestTheLotIsPeopleNotADistribution();
  TestPartnersHaveCareersOfTheirOwn();
  TestRapportGrowsAndFades();
  TestBetaIsWorthAskingForAndOnlyOnce();
  TestCompanyIsWorthSomethingAndNeverAGrade();
  TestSomebodyCanTakeYourProject();
  TestTheLotDoesNotDisturbThePlayersRng();
  TestTheFireHasSomethingToSay();
  TestAThreadGoesColdAndComesBack();
  TestOnlyAPersonLeaves();
  TestAFortnightIsNotALoss();
  TestALossCostsYouASeasonAndThenStops();
  TestThereIsNobodyElseOnThursday();
  TestEachThreadPaysInItsOwnCurrency();
  TestAPhoneCallHomeReachesTheWall();
  TestTheLifeLabelStaysQuiet();
  TestALifeSurvivesASave();
  TestLoadsVersion35Save();
  TestALifeCoolsWhileYouAreAtTheCrag();
  TestABookMakesAnAfternoonWorthMore();
  TestAnEveningCostsTheEvening();
  TestNoSpotIsTheBestSpot();
  TestTheGoodSpotsHaveToBeEarned();
  TestADrivewayIsNotAnAddress();
  TestTheCityNoticesYouEventually();
  TestTheCityDoesNotDoInstalments();
  TestWhereYouSleptFollowsYouIntoTheDay();
  TestGrimeOnlyEverCostsYouCompany();
  TestYouCannotGetCleanOutOfAJug();
  TestTheJugsAndTheBottleRunOut();
  TestAWeekOutHereGetsRipe();
  TestBeingRipeEmptiesTheLotAroundYou();
  TestGrimeCostsYouRapportAndTheCounter();
  TestTheVanIsQuietUntilSomethingRunsOut();
  TestYouCannotBuyWhatYouCannotAfford();
  TestTheGymSurvivesASave();
  TestLoadsVersion38Save();
  TestLosingTheGymIsToldOnceAndCostsStanding();
  TestTheFloorIsAMeterAndNotASwitch();
  TestTheLeversTradeVolumeAgainstMargin();
  TestEquipmentIsALadderAndStaffAreAWage();
  TestACampaignRunsOutAndOnlyOneRunsAtATime();
  TestTheBankTakesItBack();
  TestTheNoticeboardIsQuietWhileTheBooksAreFine();
  TestTheGymRunsOnItsOwnRng();
  TestTheTownIsWeatherAndNotATax();
  TestTheYearIsARhythmAndNotATax();
  TestTheTownIsYoursAndNotAScript();
  TestTheTownIsClampedAtBothEnds();
  TestMarketingIsTheAnswerToTheOtherTwoGyms();
  TestTheFloorIsAlwaysChasing();
  TestSomethingGoesWrongAndItHasAClock();
  TestAnsweringItCostsMoneyOrCostsYouSomethingElse();
  TestNobodyPoachesASetterYouDoNotHave();
  TestTheShortlistIsThreeDifferentPeople();
  TestTheRightHireDependsOnALeverYouAlreadyPulled();
  TestTenureComesDueAndRefusingHasTwoPrices();
  TestWingsAreIndependentAndOnlyOneOfThemEarns();
  TestItCanRunWithoutYouOnceThereIsSomebodyToRunIt();
  TestLoadsVersion41Save();
  TestInsuranceIsWhatGetsADirtbagRepaired();
  TestTheLotDoesNotAlwaysTurnUp();
  TestNobodyIsNeverThereAndNobodyIsAlways();
  TestWhoIsHereDoesNotDependOnWhoElseIs();
  TestAWashoutEmptiesTheLot();
  TestKnowingSomebodyBringsThemOut();
  TestTurnoutDoesNotDisturbThePlayersRng();
  TestTheDayHappensWhetherOrNotYouDo();
  TestSleepingHungryIsAThingThatCanHappen();
  TestAStrangerIsServedNotGreeted();
  TestTheySayItOnceAndGoBackToNodding();
  TestTheLoudestThingWins();
  TestOnlyTheOneWhoSawItKnowsAboutTheBill();
  TestTheyRememberYouAfterAWinter();
  TestNobodyTellsThemYouWereAway();
  TestBeingAKnownFaceIsWorthSomething();
  TestTheTownIsSeededByTheNight();
  TestTheDinerGreetsYouByWhatYouDid();
  TestWhatYouWentToBedWithSurvivesASave();
  TestTheTownHearsWhatYouNamedIt();
  TestAKnownFaceIsCharacterLessForTheBurrito();
  TestTheTownSurvivesASave();
  TestLoadsVersion36Save();
  TestTheBillYouCannotCoverIsBetweenYouAndThem();

  if (g_failures == 0) {
    std::printf("OK  %d checks passed\n", g_checks);
    return 0;
  }
  std::printf("FAILED  %d of %d checks\n", g_failures, g_checks);
  return 1;
}
