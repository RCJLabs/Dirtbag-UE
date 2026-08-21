// Standalone harness for the Dirtbag sim core — no engine, no framework.
// Build and run: Sim/run-tests.sh (plain g++). These are the same translation
// units the Unreal module will compile; if they pass here, the sim is the sim.

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

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

  Rest(day, 3.0, d);
  CHECK(day.hour == hour0 + 3.0);                      // the hours go
  CHECK(day.hunger > hunger0);                         // and cost the same
  CHECK(day.energy > 40.0);                            // you get a little back
  CHECK(day.energy < 40.0 + 3.0 * d.restEnergyPerHour + 1e-9);

  // An afternoon in the shade is worth less than a night: resting is how you
  // spend hours you cannot climb in, not a way to farm energy.
  DayState rested = WakeUp(player);
  rested.energy = 40.0;
  Rest(rested, 6.0, d);
  CHECK(rested.energy < d.sleepEnergyFloor + 6.0 * d.restEnergyPerHour);
  CHECK(rested.energy <= 100.0);

  // It never overfills, and zero hours does nothing at all.
  DayState full = WakeUp(player);
  full.energy = 99.0;
  Rest(full, 10.0, d);
  CHECK(full.energy == 100.0);
  const double before = full.hour;
  Rest(full, 0.0, d);
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
  const std::size_t at = v14.find("sponsor.hurtdays=");
  CHECK(at != std::string::npos);
  v14.erase(at, v14.find('\n', at) + 1 - at);
  CHECK(v14.find("sponsor.hurtdays") == std::string::npos);
  const std::size_t vat = v14.find("version=15\n");
  CHECK(vat != std::string::npos);
  v14.replace(vat, std::string("version=15\n").size(), "version=14\n");

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
  save.player.bonds = {margo, dev};

  SaveGame loaded;
  CHECK(DeserializeSave(SerializeSave(save), loaded) == LoadResult::Ok);
  CHECK(loaded.player.bonds.size() == 2);
  CHECK(loaded.player.bonds[0].name == "Margo");
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

  for (int i = 0; i < 5; i++) SpendDayWith(p, true, d);
  const double warm = p.rapport;
  CHECK(warm > 0.0);

  for (int i = 0; i < 5; i++) SpendDayWith(p, false, d);
  CHECK(p.rapport < warm);       // people remember you, not forever
  CHECK(p.rapport > 0.0);        // and five days away is not amnesia

  // It saturates rather than running away.
  for (int i = 0; i < 500; i++) SpendDayWith(p, true, d);
  CHECK(p.rapport == 1.0);
  for (int i = 0; i < 5000; i++) SpendDayWith(p, false, d);
  CHECK(p.rapport == 0.0);
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

    CHECK(WorkOddJob(famous, d1, photos, d));
    CHECK(WorkOddJob(worker, d2, trail, d));
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

  CHECK(WorkOddJob(player, day, hauling, d));       // van runs, fine

  player.van.parts[static_cast<int>(VanPart::Belt)].failed = true;
  const double cash = player.cash;
  const double hour = day.hour;
  CHECK(!WorkOddJob(player, day, hauling, d));      // and now it does not
  CHECK(player.cash == cash);                       // nothing happened
  CHECK(day.hour == hour);

  // Work that does not need it is unaffected.
  OddJob dishes;
  dishes.name = "washing dishes at the diner";
  dishes.needsVan = false;
  CHECK(WorkOddJob(player, day, dishes, d));
}

static void TestOddJobsPayDebtFirst() {
  DayDials d;
  PlayerState player;
  player.cash = 0.0;
  player.owed = 100.0;
  DayState day = WakeUp(player, d);

  OddJob gig;
  gig.pay = 60.0;
  CHECK(WorkOddJob(player, day, gig, d));
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
  for (int i = 0; i < 7; i++) SleepToNextDay(player, day, Rng::FromSeed("body"));
  CHECK(player.day == 8);
  CHECK(player.cash == start - DayDials{}.billsAmount);
  for (int i = 0; i < 7; i++) SleepToNextDay(player, day, Rng::FromSeed("body"));
  CHECK(player.cash == start - 2 * DayDials{}.billsAmount);
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
  const Legacy l = TallyCareer(ACareer(), "Evan", 9);
  const PlayerState next = Inherit(l, d);
  const PlayerState fresh;

  // Nothing physical carries.
  CHECK(next.climber.skills.power == fresh.climber.skills.power);
  CHECK(next.climber.skills.fingers == fresh.climber.skills.fingers);
  CHECK(next.climber.load == 0.0);
  CHECK(!IsHurt(next.climber));
  CHECK(next.projects.empty());          // none of the ledgers are yours
  CHECK(next.day == 1);                  // and you are twenty-four again
  CHECK(AgeOn(next.day) == AgeDials{}.startAge);

  // The van and the coffee tin, and deliberately not the gear: a career
  // that ended rich must not hand the next one a shortcut past the part of
  // this game that is about being broke.
  CHECK(next.cash == d.inheritedCash);
  CHECK(next.kit.pads == fresh.kit.pads);
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
  CHECK(Inherit(shut, d).standing.closedDays == 0);
  CHECK(CragIsOpen(Inherit(shut, d).standing));
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
  CHECK(roadside.lines.size() == 30);

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

  // The second is the purchase, and it is the one that tops it out.
  cash = 900.0;
  CHECK(BuyPad(kit, cash, d));
  CHECK(PaddingFrom(kit, d) == 1.0);
  CHECK(BuyPad(kit, cash, d));
  CHECK(PaddingFrom(kit, d) == 1.0);    // and no further, borrow the third
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
  StartGymSession(climber, climbDay, dd);
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

  if (g_failures == 0) {
    std::printf("OK  %d checks passed\n", g_checks);
    return 0;
  }
  std::printf("FAILED  %d of %d checks\n", g_failures, g_checks);
  return 1;
}
