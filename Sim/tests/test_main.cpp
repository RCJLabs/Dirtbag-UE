// Standalone harness for the Dirtbag sim core — no engine, no framework.
// Build and run: Sim/run-tests.sh (plain g++). These are the same translation
// units the Unreal module will compile; if they pass here, the sim is the sim.

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "../DirtbagCore.h"
#include "../DirtbagDay.h"
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
  Climber fit = MakeClimber(55, 55, 55, 85, 50);
  Climber unfit = MakeClimber(55, 55, 55, 25, 50);
  double fitPump = 0.0, unfitPump = 0.0;
  const int n = 400;
  for (int i = 0; i < n; i++) {
    Rng r1 = Rng::FromSeed("p-" + std::to_string(i));
    Rng r2 = Rng::FromSeed("p-" + std::to_string(i));
    fitPump += ResolveAttempt(r1, MakeInput(fit, pitch)).peakPump;
    unfitPump += ResolveAttempt(r2, MakeInput(unfit, pitch)).peakPump;
  }
  CHECK(fitPump / n < unfitPump / n);
}

static void TestSandbagBites() {
  Rng world = Rng::FromStream("seed-w", Stream::Worldgen);
  Route honest = BuildRoute(world, "Honest Abe", 6, 6, RouteType::Power,
                            Discipline::Boulder);
  Route sandbag = BuildRoute(world, "Honest Abe", 6, 8, RouteType::Power,
                             Discipline::Boulder);
  // A climber near the grade — strong enough to send the honest V6 often,
  // close enough to the margin that the hidden two grades genuinely bite.
  Climber c = MakeClimber(38, 38, 38, 38, 38);
  CHECK(SendRate(c, honest, 400) > SendRate(c, sandbag, 400));
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
  Climber c = MakeClimber(52, 52, 52, 60, 50);

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
  TestDayBasics();
  TestBillsLandWeekly();
  TestSkinRegrowsOvernight();
  TestHungrySleepRecoversPoorly();
  TestTrainingCreep();
  TestSevenDayLoop();
  TestSaveRoundTrip();
  TestSaveRejectsGarbageAndFuture();
  TestMigrationMachinery();

  if (g_failures == 0) {
    std::printf("OK  %d checks passed\n", g_checks);
    return 0;
  }
  std::printf("FAILED  %d of %d checks\n", g_failures, g_checks);
  return 1;
}
