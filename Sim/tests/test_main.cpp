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
#include "../DirtbagFirstAscent.h"
#include "../DirtbagPartner.h"
#include "../DirtbagRng.h"
#include "../DirtbagSave.h"
#include "../DirtbagSession.h"
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
      for (double h = d.firstLight; h <= d.lastLight; h += 0.5) {
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
      CHECK(win.startHour >= d.firstLight - 1e-9);
      CHECK(win.endHour <= d.lastLight + 1e-9);
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
        const double reconHour = std::max(d.firstLight, win.startHour - 2.0);
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
  auto WindowBurnsLeft = [&](int recon) {
    const Route& p = projects[0];
    Weather w = GenerateWeather(world, 3, d);
    PrimeWindow win = FindPrimeWindow(w, Aspect::East, d);
    CHECK(win.exists);
    const Conditions poor =
        ConditionsAt(w, Aspect::East,
                     std::max(d.firstLight, win.startHour - 2.0), d);
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

  // So you clean it instead.
  m.beta = 0.0;
  while (!IsWorkable(m)) CleanLine(player, day, m, 1.0);
  CHECK(IsWorkable(m));

  // Then you work it, across sessions, until it goes.
  bool sent = false;
  for (int dayN = 0; dayN < 25 && !sent; dayN++) {
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
    ApplyAttemptToDay(cruiser, cruiseDay, easy, a);
    AttemptResult b = AttemptInSession(rng, tryDay.session,
                                       MemoryFor(tryer, limit), tryer.climber,
                                       limit, Conditions{});
    ApplyAttemptToDay(tryer, tryDay, limit, b);
  }
  CHECK(tryDay.energy < cruiseDay.energy);
}

static void TestBillsLandWeekly() {
  PlayerState player;
  DayState day = WakeUp(player);
  const double start = player.cash;
  for (int i = 0; i < 7; i++) SleepToNextDay(player, day);
  CHECK(player.day == 8);
  CHECK(player.cash == start - DayDials{}.billsAmount);
  for (int i = 0; i < 7; i++) SleepToNextDay(player, day);
  CHECK(player.cash == start - 2 * DayDials{}.billsAmount);
}

static void TestSkinRegrowsOvernight() {
  PlayerState player;
  player.climber.skin = 4.0;
  DayState day = WakeUp(player);
  SleepToNextDay(player, day);
  CHECK(player.climber.skin == 4.0 + DayDials{}.skinRegenPerNight);
  player.climber.skin = 8.9;
  SleepToNextDay(player, day);
  CHECK(player.climber.skin == DayDials{}.maxSkin);  // capped, never past fresh
}

static void TestHungrySleepRecoversPoorly() {
  PlayerState fed, starving;
  DayState fedDay = WakeUp(fed), starvingDay = WakeUp(starving);
  starvingDay.hunger = 100.0;
  SleepToNextDay(fed, fedDay);
  SleepToNextDay(starving, starvingDay);
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
    ApplyAttemptToDay(grinder, gDay, hard, g);
    AttemptResult c = AttemptInSession(sessionRng, cDay.session,
                                       MemoryFor(cruiser, easy),
                                       cruiser.climber, easy, Conditions{});
    ApplyAttemptToDay(cruiser, cDay, easy, c);
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
      ApplyAttemptToDay(player, day, route, r);
    }
    EatMeal(player, day);
    WorkShift(player, day);
    EatMeal(player, day);
    SleepToNextDay(player, day);
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
  TestRockGoesBackToTheWeather();
  TestFirstAscentsAreACareer();
  TestTheWholeArc();
  TestSaveCarriesFirstAscents();
  TestLoadsVersion2Save();
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
