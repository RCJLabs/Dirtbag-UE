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

#include <climits>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include <algorithm>

#include "DirtbagCharacter.h"
#include "DirtbagConditions.h"
#include "DirtbagRival.h"
#include "DirtbagCore.h"
#include "DirtbagBody.h"
#include "DirtbagCrag.h"
#include "DirtbagSport.h"
#include "DirtbagTrad.h"
#include "DirtbagHabits.h"
#include "DirtbagDay.h"
#include "DirtbagDog.h"
#include "DirtbagFirstAscent.h"
#include "DirtbagGear.h"
#include "DirtbagVan.h"
#include "DirtbagFactions.h"
#include "DirtbagJobs.h"
#include "DirtbagTown.h"
#include "DirtbagPartner.h"
#include "DirtbagSave.h"
#include "DirtbagLegacy.h"
#include "DirtbagSponsor.h"
#include "DirtbagSession.h"
#include "DirtbagSessionLoop.h"

using namespace dirtbag;

namespace {

struct LineTally { std::string name; int burns = 0, sends = 0, best = 0; int moves = 0; };

struct Tally {
  // --- Sport, and the walk in ----------------------------------------------
  // Both measured for the first time. The approach has never been paid by
  // any career in any note; the belayer has never been asked for.
  double approachHours = 0.0;
  int noBelayerDays = 0;    // you walked up there and nobody would tie in
  int belayerCappedDays = 0;// their patience ended the session, not your skin
  int burnsTheyHeld = 0;    // total burns a belayer stood under
  double bestRapport = 0.0; // with anybody, at the end

  // --- Habits ---------------------------------------------------------------
  // What a career turned into, which is a question nothing in this probe
  // could ask before: every measurement here has been about what a climber
  // *did*, and this is the first about what they became.
  int quirksEarned = 0;
  int firstQuirkDay = -1;
  std::string became;            // in the order they landed
  int daysWithAHabit = 0;        // nights the player was doing something
  double habitDaysSum = 0.0;     // habits held per night, for the average

  // --- A life outside it ---------------------------------------------------
  // The first measurement of a career that is not entirely about climbing.
  // Every number here is a "how often" question the harness cannot ask:
  // does a career that is not being steered ever actually lose somebody,
  // and if so, on what day.
  int eveningsSpent = 0;
  int buskSessions = 0;
  double busked = 0.0;
  // How careers actually end, which nothing has ever counted: the rule has
  // two triggers and they mean opposite things.
  // The gym, if the policy bought one.
  int boughtGymOnDay = -1;
  double gymBalanceEnd = 0.0;
  double gymMembersEnd = 0.0;
  int gymDaysOwned = 0;
  // **Nights the gym was in the news**, which since pass two is incidents
  // as well as the bank -- so the foreclosures are counted separately or
  // the column quietly stops meaning what it is named.
  int gymLost = 0;
  int gymForeclosed = 0;
  double gymWorstWage = 0.0;
  // Pass two. What the building actually absorbed, and what it did with it.
  double gymSpent = 0.0;
  int gymWings = 0;
  int gymHired = 0;
  int gymRaises = 0;
  int gymQuit = 0;
  int gymFixed = 0;
  int gymLeft = 0;
  // GYM-2/GYM-5: the floor.
  int gymWalks = 0;
  int gymMoments = 0;
  int gymComps = 0;
  double gymCompTakings = 0.0;
  int gymCohortDay = 0;
  int gymArcsLived = 0;
  // GYM-8: the squad.
  int youthDay = 0;
  int youthSessions = 0;
  int youthGrads = 0;
  int youthOldest = 0;
  double youthCraftEnd = 0.0;
  int youthSteppedUp = 0;
  // GYM-9: the federation.
  int gymBids = 0;
  int gymSeasonsHeld = 0;
  int gymRoundsRun = 0;
  double gymBidSpend = 0.0;
  double gymRoundPay = 0.0;
  // GYM-10: the league you run.
  int glNights = 0;
  int glRuns = 0;
  double glTakings = 0.0;
  int glSuitedChamps = 0;
  int glDistinctChamps = 0;
  int gymHandsOffDay = 0;

  int declinedTheOffer = 0;
  int retiredByBody = 0;
  int retiredByGrade = 0;
  double retirementAgeSum = 0.0;
  double youngestRetirement = 999.0;

  int lostSomebodyOnDay = -1;   // -1: never had anybody, or never lost them
  int hadSomebodyOnDay = -1;
  int grievingDays = 0;
  int nagDays = 0;              // nights the life label had something to say
  double warmthEnd[kThreadCount] = {0.0};
  double deepest = 0.0;         // the furthest into anything you ever got

  // --- The people behind the counters --------------------------------------
  // Phase 11's third gate, measured: does anybody in a thirty-year career
  // ever actually get greeted by what they did last time, and how often.
  int greeted = 0;
  int firstGreetingDay = -1;
  double bestKnown = 0.0;       // at the end, not the high-water mark
  std::string lastSaid;
  int greetedByKind[kHeardCount] = {0};

  // --- Trad ---------------------------------------------------------------
  // What a career of leading looks like, which is a different question from
  // what an attempt looks like.
  double spentRack = 0.0;
  int gotTheRackOnDay = -1;     // -1: never could afford one
  int leadsOnGear = 0;          // burns on a trad route
  int piecesPlaced = 0;
  int ranItOut = 0;             // burns that reached the top third with nothing in
  double leadFearSum = 0.0;     // worst exposure per lead, in grade units

  int daysClimbed = 0, daysWorked = 0, daysRested = 0, daysWashedOut = 0;
  int burns = 0, sends = 0, firstAscents = 0;
  int mealsEaten = 0, dogMeals = 0, brokeDays = 0, starvedNights = 0;
  double cashLow = 1e9, cashHigh = -1e9;
  int linesLostToTheLot = 0;
  int linesLostToTheRival = 0;
  int rivalGenerations = 0;
  int racesStarted = 0, racesWon = 0, racesLost = 0;   // won: you got there
  // The ladder: comps entered, and how far up it a career actually gets.
  // `CHAR-7`: the highest any lane's monotony ever got, and how many days
  // a lane was genuinely plateaued. **Sampled rather than derived**: the
  // end-of-career level says nothing, because a rest day sheds it -- which
  // is how the first cut of this reported "nothing stuck" from a career
  // that had spent years at half gains.
  double monotonyPeak = 0.0;
  int plateauDays = 0;
  int compsEntered = 0, compWins = 0, compPodiums = 0;
  double rankingPeak = 0.0;
  double rankingEnd = 0.0;   // where it settles, which is the real number
  // The medical file, over a career.
  int diagnoses = 0, shots = 0, surgeries = 0, rushed = 0, untreated = 0;
  int sickDays = 0, timesIll = 0, medsTaken = 0;
  int prehabDays = 0, toothFixes = 0, worstTooth = 0, toothDays = 0;
  double medicalSpend = 0.0;
  double premiums = 0.0, claims = 0.0;
  int roundsClimbed = 0, finalsReached = 0;
  int leagueNights = 0, leaguePBs = 0;
  // The trades.
  int momentsTaken = 0, momentsBotched = 0, momentsDucked = 0, sackings = 0;
  double bestCraft = 0.0;
  std::string trade;
  int teamSeasons = 0;
  int wcStarts = 0, wcMissed = 0, wcPodiums = 0, wcWins = 0, wcTitles = 0;
  int gamesEntered = 0, medals = 0;
  double peakAllround = 0.0;   // the best this body ever was
  std::vector<std::string> lotNames;   // what they called them
  std::vector<LineTally> perLine;
  double earned = 0.0, spentFood = 0.0, spentDog = 0.0, spentBills = 0.0;
  double spentShoes = 0.0;
  int resoles = 0, newPairs = 0, deadRubberDays = 0;
  double spentVan = 0.0, spentFuel = 0.0, spentKit = 0.0;
  int gymDays = 0, boardDays = 0, memberDays = 0;
  int injuries = 0, hurtDays = 0, climbedHurtDays = 0, aggravations = 0;
  int dreamsBought = 0;
  int physioSessions = 0;
  double spentPhysio = 0.0, loadSum = 0.0, peakLoad = 0.0;
  int dealsSigned = 0, sponsoredDays = 0, obligationDays = 0;
  double sponsorPay = 0.0;
  int breakdowns = 0, strandedDays = 0, bodges = 0;
  int closedDays = 0, closures = 0;
  int photoGigs = 0, trailGigs = 0;
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
                         const PlayerState& player, bool racesBack = false) {
  // **A player who races.** Every other policy in this probe is a fixed
  // heuristic that ignores the rival entirely, which is exactly why the
  // rival measured as changing nothing about what a career climbs: they
  // took lines out of the world and nobody reacted. This one drops what it
  // is on and goes to the contested line while the clock is running -- and
  // the difference between it and the same policy without this branch is
  // the only honest answer to "does the rival change what you climb".
  if (racesBack && RaceIsOn(player.rival)) {
    for (const CragLine& l : crag.lines) {
      if (l.route.name != player.rival.race.routeName) continue;
      if (ReadRoute(c, l.route) == RouteRead::NotThisYear) break;
      bool done = false;
      for (const ProjectMemory& m : player.projects)
        if (m.routeName == l.route.name && m.sent) done = true;
      if (!done) return &l;
      break;
    }
  }
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
  // "careful" turns down work that would cost you the stewards. The
  // question the faction system asks is whether you can afford to.
  const bool mindReputation = argc > 5 && std::string(argv[5]) == "careful";
  // "kept" is the control, and the only one that can answer the Phase 3
  // gate. Every other policy works whenever the money runs low, so by
  // construction none of them ever goes broke and none of them can tell you
  // what the rent costs. A player with no bills, free food and free fuel
  // never works a day; the difference between their season and a real one
  // is, exactly, the price of being alive.
  const bool kept = argc > 5 && std::string(argv[5]) == "kept";
  // "kitted" spends money on climbing rather than hoarding it: pads, a
  // board, and the gym membership that turns a washed-out day into a day.
  // This is the policy the whole kit exists to make possible, and the one
  // that says whether money now buys anything.
  const bool buysKit = argc > 5 && std::string(argv[5]) == "kitted";
  // "sponsored" takes every deal offered. The last untried answer to the
  // Phase 3 gate: every other source of money competes with climbing, and
  // this one arrives because of it — at the price of the good days.
  const bool takesDeals = argc > 5 && std::string(argv[5]) == "sponsored";
  // "saver" is `kitted` that will actually work for the thing it wants.
  //
  // Every other policy works only when nearly broke -- cash below $120 --
  // which is a thermostat, and a thermostat never saves up. That is why the
  // second crash pad measured as unreachable: not because the economy
  // forbids it, but because no simulated player has ever tried to buy it.
  // Since seventy days of work a year cost no sends, working *toward*
  // something should be close to free, and whether it is is exactly what
  // Phase 3's restated criterion 2 asks.
  const bool savesUp = argc > 5 && std::string(argv[5]) == "saver";
  // "projector" cleans a line properly before pulling on it, instead of
  // stopping the moment it is merely workable.
  //
  // Measured, on Roadside's open projects at skill 65: workable (0.55) with
  // no beta sends the V8 arete **0.0%** of the time; clean and wired it
  // sends **66.8%**. The whole difference between "impossible" and "two
  // thirds" is the projecting loop the game is built around, and no probe
  // policy had ever run it -- so ninety years of careers concluded the
  // valley was exhausted when it was only unbrushed.
  const bool projects = argc > 5 && (std::string(argv[5]) == "projector" ||
                                     std::string(argv[5]) == "racer");
  // "racer" is `projector` that answers the rival: same money, same brushing,
  // same everything -- and it drops what it is on when a race starts. The
  // pair is the measurement Phase 8's first gate asks for.
  const bool racesBack = argc > 5 && std::string(argv[5]) == "racer";
  // `stakeout` is `projector` plus one habit every real dirtbag has and no
  // probe policy has ever modelled: brushing a line you cannot climb yet.
  //
  // SpokenFor already counts a brushed line as yours, so the game has always
  // allowed this. The probe never did it because PickLine skips anything
  // that reads NotThisYear, so a project ten years above the player's grade
  // was never touched -- and the Lot, which rolls for a first ascent every
  // day from day one, had a decade's head start on every line in the book.
  // This policy tests whether that head start is the whole story.
  // `hoarder` is `stakeout` plus the one thing no policy has ever done:
  // actually bank money. `saver` is misleadingly named -- it buys pads and
  // its own comment says the pad is "the only thing in the game worth saving
  // for", which is exactly the hole dreams are supposed to fill. Nobody has
  // ever measured what a career can accumulate, or what accumulating costs
  // in climbing, and a dream cannot be priced without both.
  // **`comper` climbs the ladder.** Every other policy in this probe has
  // ignored the entire comp system, which means the circuit, the ranking
  // tiers, the national team, the World Cup and the Games were measured
  // only by the harness -- and a harness can prove a rule fires without
  // ever answering whether a career gets near it.
  //
  // It signs in at every comp it can pay for, plays the board easiest
  // first, gets on the plane when the federation is paying and the money is
  // there, and starts at the Games when it qualifies. Deliberately greedy
  // and deliberately dumb: the question is whether the ladder is reachable,
  // not whether it can be optimised.
  const bool comps = argc > 5 && std::string(argv[5]) == "comper";

  // **A leader.** Saves for a rack and then climbs the buttress instead of
  // Roadside, buying up the shelf as the money arrives.
  //
  // It exists because trad shipped measured only by the harness, and this
  // project has twice now built a system whose every assertion held and
  // whose *career* was nonsense -- the World Cup's fifty Games in ten years
  // and the tooth's ten thousand days of abscess. Both were about how often
  // rather than whether, and a harness assertion cannot ask how often.
  const bool leads = argc > 5 && std::string(argv[5]) == "trad";

  // **A clipper.** Goes to the Shaded Cave, which needs somebody to hold
  // the rope, and climbs on bolts.
  //
  // Sport has existed since Phase 8 and **no career has ever climbed one.**
  // The probe has only ever been to Roadside, so the belayer gate, what
  // rapport buys, the clipping economy and the cave's whole north-facing
  // reason to exist have been measured by the harness and never played --
  // the same gap that hid the World Cup's calendar and the tooth's dead
  // end, and both of those were "how often" questions no assertion can ask.
  const bool clips = argc > 5 && std::string(argv[5]) == "sporty";

  // **Somebody with a life.** Makes the time whether or not the day left
  // any -- an evening on the thread that most needs it, even when that
  // means going in from the crag early.
  //
  // Every career gets a life either way; this one just refuses to let the
  // weather decide. The contrast is the measurement: Phase 11's whole
  // claim is that a thread can be lost through neglect *alone*, and the
  // only way to know whether that ever actually happens to a career nobody
  // is steering is to run one that tries and one that does not.
  const bool makesTime = argc > 5 && std::string(argv[5]) == "lifer";

  // Arg 6 overrides skin regen per night (shipped: 1.5, so nine points of
  // skin is six nights). This is not a balance proposal — it is the knob
  // that answers the one question five measurements have left standing:
  // *if skin stops being the binding constraint, what binds next?* If the
  // answer is "the weather", then no money mechanic can ever close Phase
  // 3's gate and the gate is what has to move. If the answer is "nothing",
  // then skin was the whole wall and lifting it is a real option.
  // --- Options, by name --------------------------------------------------
  //
  // **Everything past the policy is `key=value`.** It used to be
  // positional, and two knobs quietly grew onto each slot:
  //
  //   arg 6   `skinRegen` *and* `buildSpec`
  //   arg 7   `startingPads` *and* `withRival`
  //
  // Both readers were live, both were documented as "Arg 6" and "Arg 7" a
  // hundred lines apart, and neither knew about the other. So **every
  // skin-regen sweep also set the archetype** -- `atoi("1.5")` is 1,
  // `atoi("9.0")` is 9 -- and overwrote the starting cash with that
  // character's, which is the sweep `notes/phase3-what-binds.md` is built
  // on. **And every `rival` run set the pads to zero**, `atoi("rival")`
  // being 0, so Phase 8's rival career was measured on the padless season
  // -- the exact knob the comment below calls "the whole mechanic".
  //
  // Named options cannot collide silently: a repeated key is a repeated
  // key, and an unknown one is refused rather than ignored. An unrecognised
  // token is a hard error for the same reason -- a probe that shrugs at
  // `pads2` and measures the default is a probe that lies quietly.
  std::map<std::string, std::string> opts;
  for (int i = 6; i < argc; i++) {
    const std::string tok = argv[i];
    const std::size_t eq = tok.find('=');
    if (eq == std::string::npos || eq == 0) {
      std::fprintf(stderr,
                   "season: options are key=value; got \"%s\".\n"
                   "  known: skin= pads= build= rival= norubber= foam=\n"
                   "         careers= lot= savings= med=\n",
                   tok.c_str());
      return 2;
    }
    const std::string key = tok.substr(0, eq);
    if (!opts.emplace(key, tok.substr(eq + 1)).second) {
      std::fprintf(stderr, "season: %s given twice.\n", key.c_str());
      return 2;
    }
  }
  {
    static const char* kKnown[] = {"skin", "pads", "build", "rival",
                                   "norubber", "foam", "careers", "lot",
                                   "savings", "med", "retire", "covers",
                                   "gym"};
    for (const auto& kv : opts) {
      bool found = false;
      for (const char* k : kKnown) found = found || kv.first == k;
      if (!found) {
        std::fprintf(stderr, "season: unknown option \"%s\".\n",
                     kv.first.c_str());
        return 2;
      }
    }
  }
  const auto opt = [&opts](const char* key, const char* fallback) {
    const auto found = opts.find(key);
    return found == opts.end() ? std::string(fallback) : found->second;
  };
  const auto optOn = [&opts](const char* key) {
    const auto found = opts.find(key);
    return found != opts.end() && found->second != "0" && found->second != "";
  };

  const double skinRegen = std::atof(opt("skin", "-1").c_str());
  // Arg 7 overrides the pads you start with (shipped: 1 of the 2 that
  // matter, so padding 0.5). The knob exists because head trains on
  // exposure, and pads are the thing that buys exposure away — 0 pads and 2
  // pads are the bold and the safe season, and the gap between them is the
  // whole mechanic.
  const int startingPads = std::atoi(opt("pads", "-1").c_str());
  // Arg 8: never buy or resole shoes. Dead rubber has never once been felt
  // in the actual game -- ToSim dropped shoeWear, so every attempt resolved
  // on new shoes -- so before calling that fixed it is worth knowing what
  // the thing nobody has felt is actually worth.
  const bool neverBuysRubber = optOn("norubber");
  // Arg 9 overrides the most foam can ever do, for finding a cap at which
  // the pad is a trade rather than a switch.
  const double foamCap = std::atof(opt("foam", "-1").c_str());
  // Arg 10 = "careers": retire and inherit when the game offers it, instead
  // of running one immortal climber forever. Everything the legacy system
  // does -- TimeToThinkAboutIt, TallyCareer, Inherit, the guidebook -- was
  // reachable only from the engine, so nothing had ever simulated more than
  // one lifetime. Phase 4's gate is a career playing end to end, and this
  // is the only way to look at one without living it.
  const bool multiLife = optOn("careers");
  // Arg 11 overrides how often somebody at the Lot takes an open line. The
  // dial's own comment says "the player should usually get the chance if
  // they commit" -- which was intuition, never measured, and measurable
  // only once the Lot's claims actually reached the book.
  const double lotChance = std::atof(opt("lot", "-1").c_str());
  // Arg 12: what a hoarder is working towards. The whole point of the sweep
  // -- at zero this is stakeout, and every dollar above it is bought with
  // days that could have been climbing.
  const double savingsTarget = std::atof(opt("savings", "20000").c_str());

  // **Asking for a savings target is asking to hoard.** It used to be the
  // `hoarder` policy alone, which made `savings=` a silent no-op on every
  // other one -- and that quietly put the whole of GYM-9 out of reach of
  // measurement, because no policy that competes could ever hold the
  // $25,000 a building costs, so the one decision the system is about
  // (run the round, or climb it) had nobody who could face it.
  const bool hoards = (argc > 5 && std::string(argv[5]) == "hoarder") ||
                      opts.count("savings") > 0 ||
                      (argc > 5 && std::string(argv[5]) == "dreamer");
  const bool stakesClaims =
      (argc > 5 && std::string(argv[5]) == "stakeout") || hoards;


  // **Arg 13: how this climber handles being hurt.** Orthogonal to the
  // climbing policy on purpose -- Phase 10's second gate asks whether a
  // career can be *shortened by choices made while injured*, and the only
  // way to answer it is two careers that climb identically and differ only
  // here.
  //
  //   "sensible"  -- sees somebody, rests every stage out, never the shot.
  //   "impatient" -- never pays to look, takes the shot the moment it is
  //                  offered, and comes back the day after.
  //   "" (default) -- neither: the injury runs its own course, which is
  //                  what every measurement before this phase assumed.
  //
  // Add "-ins" to any of them to carry a policy. **Orthogonal on purpose
  // too**: the third gate asks whether insurance is a real bet, and a bet
  // can only be measured against the same career without it.
  // **How this climber answers the one question the game ever asks them.**
  //
  //   "always" (default) -- takes the offer the moment it comes, which is
  //                what every careers measurement before this was taken
  //                with, so they all still reproduce.
  //   "late"   -- takes the grade offer, which already needs forty-six, and
  //                refuses the *body* offer until then. **Nobody retires at
  //                twenty-four because of one bad year**, and a probe that
  //                does measures a dynasty of people who never climbed.
  //
  // Measured, the difference is the whole shape of a ninety-year run: on a
  // seed that climbs hard, "always" gives 88 lives with 87 of them ended by
  // the body at a mean age of 24.7.
  // What insurance pays of a bill, for finding the fraction at which the
  // thing it exists for actually happens.
  const double coversOverride = std::atof(opt("covers", "-1").c_str());
  // **Buy the gym the day you can afford it, and run it.** The whole
  // reason the cut was reversed is that nothing in this game absorbs what
  // a saving career holds -- so the measurement that matters is whether a
  // career that buys one is better or worse off for it.
  const bool buysAGym = optOn("gym");
  // **`gym=run` is pass two.** `gym=1` buys the building and leaves every
  // lever where it starts, which is what pass one measured; `gym=run` also
  // staffs it, builds onto it, answers what lands on the clipboard and
  // eventually hands over the keys. The two together are how you find out
  // whether pass two absorbs the ceiling pass one only half-absorbed.
  const bool runsAGym = opt("gym", "") == "run" || opt("gym", "") == "life" ||
                        opt("gym", "") == "hired";
  // **`gym=life` is `run` plus the people in it.** Walk the floor every day
  // it will let you and throw a comp night whenever one is allowed. It is
  // separate from `run` because it costs **an hour a day**, and an hour a
  // day for thirty years is the sort of thing that has to be measured
  // rather than assumed harmless.
  const bool livesInIt = opt("gym", "") == "life";
  // **`gym=hired` is `life` with the squad handed over.** Its own word
  // because the two coaching policies cannot be measured at once, and
  // because a battery without it reports `gainHired` dead -- which is how
  // the source's own hired coach came to do nothing at all.
  const bool paysACoach = opt("gym", "") == "hired";
  const std::string retirePolicy = opt("retire", "always");
  const std::string medPolicy = opt("med", "");
  const auto has = [&medPolicy](const char* what) {
    return medPolicy.find(what) != std::string::npos;
  };
  // "careful" is `sensible` with money behind it: a scan every time, and
  // the operation when the scan says it is that bad. **The only policy
  // that uses the expensive end of the medical system**, and therefore the
  // only one against which insurance can be a bet at all.
  // "-up" adds the upkeep half: twenty minutes of prehab most mornings,
  // meds when ill, and the tooth dealt with while it is still a filling.
  // **Its own flag because it is its own question** -- gate 2 asks about
  // choices made *while injured*, and these are choices made before.
  // "-hard" answers every shift's decision the hard way, whether or not
  // the craft is there. **Its own flag because gate three needs it**: a
  // policy that only reaches when it can reach never botches, never loses
  // standing and is never sacked -- so the sacking is testable in the
  // harness and unreachable in a played career, which is the same as not
  // existing.
  const bool workHard = has("-hard");
  const bool medUpkeep = has("-up");
  const bool medCareful = has("careful");
  const bool medSensible = has("sensible") || medCareful;
  const bool medImpatient = has("impatient");
  const bool medInsured = has("-ins");
  const bool medUninsured = !medInsured;
  // `dreamer` is `hoarder` that actually spends what it saved, the moment
  // it can. The whole question the design rests on: does buying the thing
  // hurt, or is a dream just a number going up?
  const bool dreams = argc > 5 && std::string(argv[5]) == "dreamer";

  const Rng world = Rng::FromSeed(seed);
  // Not const: across generations the book has to be written into, or the
  // next climber arrives to find their predecessor's lines unclimbed. That
  // is exactly what ninety years of this probe found on its first run --
  // four careers each doing the first ascent of the same two boulders.
  Crag crag = RoadsideCrag(world);
  ConditionsDials cd;
  DayDials dd;
  if (kept) {
    dd.billsAmount = 0.0;
    dd.mealCost = 0.0;
    // And a float, which the control was missing for five measurements.
    //
    // `kept` was meant to be "a player for whom money is not a question".
    // It was not. Zeroing the bills also removed every reason to work, so
    // the kept player earned nothing, ended every year on about $2, and
    // could never buy shoes -- ending on 0.89 wear against a working
    // player's 0.30. Since dead rubber is worth roughly 3.7 sends against
    // 1.0, that one omission accounts for the whole of the anomaly the
    // first Phase 3 note recorded and could not explain: "fewer sends than
    // the player paying rent".
    //
    // A control that removes the costs *and* the income is not a control
    // for "does money pressure climbing". It is a control for being broke.
    // The float itself is applied where the player exists, below.
  }
  if (skinRegen > 0.0) dd.skinRegenPerNight = skinRegen;
  FirstAscentDials fd;
  DogDials dog;

  // Arg 7 is "rival" to give the career somebody to beat. Absent means
  // nobody, which is what every measurement before Phase 8 was taken with,
  // so those all still reproduce.
  const bool withRival = optOn("rival");

  // Arg 6 is the build, as "archetype/origin/flaw/temperament" by index --
  // e.g. "0/2/4/1" is a Boulderer from the desert with tweaky fingers who
  // is send-or-bust. Absent means unbuilt, which is neutral in every lane,
  // so every measurement taken before Phase 7 reproduces exactly.
  const std::string buildSpec = opt("build", "");

  PlayerState player;
  player.name = "Climber 1";
  // Drawn from the world like every life after it -- the flat 50s this
  // replaces were the same climber every seed, which understated how much
  // careers differ before a single day is played.
  player.climber = NewClimber(world);

  if (!buildSpec.empty()) {
    int part[4] = {0, 0, 0, 0};
    int at = 0;
    std::string cur;
    for (std::size_t i = 0; i <= buildSpec.size(); i++) {
      if (i == buildSpec.size() || buildSpec[i] == '/') {
        if (at < 4) part[at++] = std::atoi(cur.c_str());
        cur.clear();
      } else {
        cur += buildSpec[i];
      }
    }
    Build b;
    b.archetype = static_cast<Archetype>(part[0] % kArchetypeCount);
    b.origin = static_cast<Origin>(part[1] % kOriginCount);
    b.flaw = static_cast<Flaw>(part[2] % kFlawCount);
    b.temperament = static_cast<Temperament>(part[3] % kTemperamentCount);
    player.character = MakeCharacter(b, world);
    player.climber = MakeClimber(b, world);
    player.cash = player.character.startingCash;
  }

  if (startingPads >= 0) player.kit.pads = startingPads;
  // The kept control's float. See the note where its dials are zeroed:
  // without this it is a control for being broke rather than for being
  // free of money, and it could never afford shoes.
  if (kept) player.cash = 10000.0;

  if (withRival) {
    const double startGrade =
        SkillToGrade((player.climber.skills.power +
                      player.climber.skills.fingers +
                      player.climber.skills.technique +
                      player.climber.skills.endurance +
                      player.climber.skills.head) / 5.0);
    player.rival = RollRival(world, player.climber.skills, 1, 0, startGrade);
  }

  if (takeTheSalary) TakeSalariedJob(player);

  // **The two you leave home with.** A number to dial and a stove in the
  // van are not purchases and not decisions; everybody has them on day one
  // and the question is only whether they ever use them. Books and the
  // guitar arrive later, and Someone arrives through the scene -- see the
  // day loop.
  TakeUp(player.life, Thread::Home, player.day);
  TakeUp(player.life, Thread::Cooking, player.day);

  Tally t;
  int consecutiveInjuries = 0;
  int sinceHurt = 0;
  double peakGradeEver = 0.0;
  int lives = 1;
  std::vector<Legacy> legacies;
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
    // Snapshot the injury at dawn, before any climbing. ClimbOnIt fires
    // per burn during the session, so a snapshot taken later in the day --
    // which is where this used to live, just above SleepToNextDay -- is
    // taken after the event it is trying to detect, and reports zero
    // aggravations forever. It did: 274 days climbed on hurt, 0
    // aggravations, and the mechanic was working the whole time.
    const bool hurtAtDawn = IsHurt(player.climber);
    const double sevAtDawn = player.climber.injury.severity;
    const int leftAtDawn = player.climber.injury.daysLeft;
    const Weather w = GenerateWeather(world, player.day, cd);
    const PrimeWindow win = FindPrimeWindow(w, crag.aspect, cd);

    // **Who is actually at the Lot today.** Rolled once, here, and used by
    // everything that means *today* -- the belayer, and whose rapport
    // moves. `LotRegulars` stays the cast: they live their own lives and
    // take their own lines whether or not you saw them.
    //
    // Until this existed the two were the same list, which is why
    // `Personality::social` had no reader and why `nobelayer` measured
    // zero across three thirty-year careers.
    std::vector<Partner> hereToday =
        WhoIsAround(world, player.day, player.bonds,
                    player.character.personality.social, win.exists);
    ApplyBonds(hereToday, player.bonds);

    std::string note;
    // **Did you climb *today*.** The rapport call below read
    // `t.daysClimbed > 0` -- the career total, not the day -- so from the
    // first climbing day onward every single day counted as a day spent
    // together, including the nine in ten a career spends washed out,
    // working or resting. Rapport therefore hit 1.00 with everybody at the
    // Lot inside a fortnight and stayed there for thirty years, which is
    // why the belayer's patience ended a session **twice in four hundred
    // days**: `BurnsTheyWillHold` was returning the full-rapport number
    // from almost the first week of a career.
    //
    // The engine has always had this right (`SpendDayWith(P, bClimbedToday)`).
    // The probe has not, and everything it has ever reported about the Lot
    // was measured on a player who never missed a day.
    const int climbedBefore = t.daysClimbed;

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

    // **The ladder, before the day is spent on anything else.** A comp
    // is a whole day and so is a plane, so this comes first -- and it runs
    // the same functions the engine's doors run, which is the whole point
    // of the probe existing.
    if (comps && !IsHurt(player.climber)) {
      const WorldStageDials wsd;
      const CompDials cpd;
      const double yourGrade =
          SkillToGrade((player.climber.skills.power +
                        player.climber.skills.fingers +
                        player.climber.skills.technique +
                        player.climber.skills.endurance +
                        player.climber.skills.head) / 5.0);

      // **What the climber walked in carrying.** Rebuilt each time it is
      // used, because the flu and the tooth and the joints all move; a
      // comp resolved without it is a comp a wrecked climber walks
      // through -- see Sim/DirtbagBodyContext.h.
      const auto bodyNow = [&]() {
        BodyContext b;
        b.who = player.character;
        b.medical = player.medical;
        b.sickness = player.sickness;
        b.teeth = player.teeth;
        b.shoeWear = player.shoes.wear;
        b.day = player.day;
        return b;
      };

      // Seven goes at five problems, easiest first.
      const auto playTheBoard = [&](CompState& board, const CompDials& cdl) {
        for (int a = 0; a < cdl.attempts; a++) {
          int pick = -1;
          for (std::size_t i = 0; i < board.problems.size(); i++) {
            if (!board.progress[i].topped) {
              pick = static_cast<int>(i);
              break;
            }
          }
          if (pick < 0) break;
          AttemptProblem(board, pick, player.climber,
                         world.Derive("probe-comp#" + std::to_string(day) +
                                      "#" + std::to_string(a)),
                         cdl, bodyNow());
        }
      };

      // **The Wednesday night, played rather than skipped.** Cheap, so
      // the policy always turns up -- which is the honest test of whether
      // a league is worth turning up to.
      if (LeagueTonight(player.league, player.day) &&
          player.cash >= LeagueDials{}.nightFee) {
        const LeagueDials ld;
        player.cash -= ld.nightFee;
        CompState night =
            SetTheLeagueBoard(world, yourGrade, player.day, ld);
        playTheBoard(night, LeagueCompDials(ld));
        const LeagueResult lr =
            SettleLeague(player.league, night, yourGrade, player.day,
                         world.Derive("probe-league#" +
                                      std::to_string(day)),
                         ld);
        player.cash += lr.cash;
        if (lr.rep > 0.0) Shift(player.standing, Faction::Scene, lr.rep);
        t.leagueNights++;
        if (lr.personalBest) t.leaguePBs++;
        // Through PassHours, not by assignment: a clock you set rather
        // than spend is a day that costs no hunger, which is the same
        // bug the night tick just had -- see DayDials::bedtimeHour.
        if (today.hour < 21.0) PassHours(today, 21.0 - today.hour, dd);
        note = "league: " + std::to_string(lr.place);
      }

      const GamesCheck games =
          CanEnterTheGames(player.olympics, player.rankingPoints, player.day,
                           wsd);
      const FlightCheck flight =
          CanFly(player.worldCup, player.team, player.cash, player.day, wsd);

      if (games.can) {
        CompState board =
            SetTheOlympicBoard(world, player.day, wsd);
        playTheBoard(board, OlympicCompDials(wsd));
        const CompResult r = SettleTheGames(
            board, world.Derive("probe-games#" + std::to_string(day)), wsd);
        BankTheGames(player.olympics, MedalFor(r.place),
                     player.olympics.nextDay);
        t.gamesEntered++;
        if (r.place <= 3) t.medals++;
        // Through PassHours, not by assignment: a clock you set rather
        // than spend is a day that costs no hunger, which is the same
        // bug the night tick just had -- see DayDials::bedtimeHour.
        if (today.hour < 23.0) PassHours(today, 23.0 - today.hour, dd);
        note = "THE GAMES: " + std::to_string(r.place);
      } else if (flight.can) {
        player.cash -= flight.cost;
        CompState board = SetTheWorldBoard(world, player.day, wsd);
        playTheBoard(board, WorldCupCompDials(wsd));
        const CompResult r = SettleWorldRound(
            board, world.Derive("probe-wc#" + std::to_string(day)), wsd);
        BankRound(player.worldCup, flight.round, &r,
                  world.Derive("probe-bank#" + std::to_string(day)), wsd);
        // Through PassHours, not by assignment: a clock you set rather
        // than spend is a day that costs no hunger, which is the same
        // bug the night tick just had -- see DayDials::bedtimeHour.
        if (today.hour < 23.0) PassHours(today, 23.0 - today.hour, dd);
        note = "World Cup " +
               std::string(TheVenues()[
                   player.worldCup.schedule[flight.round].venue].city) +
               ": " + std::to_string(r.place);
      } else if (RoundIsOpen(player.circuit, player.day) &&
                 player.cash >= cpd.entryFee) {
        player.cash -= cpd.entryFee;
        const CompTier tier = TierFor(player.rankingPoints);
        CompState board = SetTheBoard(world, tier, yourGrade, player.day);
        const bool finals = FinalsToday(player.circuit, player.day);

        // **Quals, semi, final -- climbed, not skipped.** At Regional and
        // above the scorecard decides who goes through rather than who
        // won, and the probe has to run the rounds or the format is
        // measured by nothing. The loop ends when a round puts you out or
        // there is no round left.
        CompResult r;
        for (int round = 0; round < kCompRoundCount; round++) {
          playTheBoard(board, board.round == CompRound::Final
                                  ? [&] {
                                      CompDials f = cpd;
                                      f.attempts = cpd.finalAttempts;
                                      return f;
                                    }()
                                  : cpd);
          r = Settle(board, yourGrade,
                     player.rival.retired ? std::string()
                                          : player.rival.name,
                     player.rival.grade,
                     world.Derive("probe-settle#" + std::to_string(day) +
                                  "#" + std::to_string(round)));
          if (!RunsRounds(tier, cpd) ||
              board.round == CompRound::Final) {
            break;
          }
          const RoundOutcome ro =
              NextRound(board, r, world, yourGrade, player.day, cpd);
          if (!ro.through) break;
          t.roundsClimbed++;
          if (board.round == CompRound::Final) t.finalsReached++;
        }
        player.cash += r.cash;
        // `TAX-1`: banked where the money lands, exactly as the engine
        // does it -- the probe paying prize money the tax man never hears
        // about is how a balance number comes to describe a game nobody
        // plays.
        BankTaxable(player.tax, r.cash);
        BankResult(player.circuit, r, finals);
        Record(player.rankingRecord, player.day,
               RankingPointsFor(r.place, r.fieldSize,
                                TierFor(player.rankingPoints), finals, false,
                                false, false));
        t.compsEntered++;
        if (r.place == 1) t.compWins++;
        if (r.place <= 3) t.compPodiums++;
        if (SeasonOver(player.circuit)) {
          const SeasonEnd end = CloseSeason(player.circuit);
          player.cash += end.cash;
          BankTaxable(player.tax, end.cash);
          Record(player.rankingRecord, player.day, end.rankingPoints);
          if (end.title) player.circuit.titles++;
          const TeamReview review =
              ReviewTheTeam(player.team, player.rankingPoints,
                            player.circuit, player.day,
                            player.circuit.season);
          player.cash += review.stipend;
          BankTaxable(player.tax, review.stipend);
          if (review.changed) {
            Shift(player.standing, Faction::Scene, review.rep);
          }
          // Read off the team rather than counted here: the committee
          // sits once a year now, and a counter that ticked at every
          // season's close would report five years for every one.
          t.teamSeasons = player.team.seasons;
        }
        // Through PassHours, not by assignment: a clock you set rather
        // than spend is a day that costs no hunger, which is the same
        // bug the night tick just had -- see DayDials::bedtimeHour.
        if (today.hour < 23.0) PassHours(today, 23.0 - today.hour, dd);
        note = "comp: " + std::to_string(r.place);
      }
      t.rankingPeak = std::max(t.rankingPeak, player.rankingPoints);
      t.rankingEnd = player.rankingPoints;
    }

    // **What you do about being hurt, before anything else about the
    // day.** Phase 10's whole point is that this is a decision and not a
    // wait, so it is taken here where the other decisions are.
    if (IsHurt(player.climber) && (medSensible || medImpatient)) {
      MedicalDials mdl;
      if (coversOverride >= 0.0) mdl.covers = coversOverride;
      const double before = player.cash;
      if (medSensible) {
        // See somebody, then rest every stage out to the day.
        const Diagnosis want =
            medCareful ? Diagnosis::Scanned : Diagnosis::Guessed;
        if (static_cast<int>(player.medical.diagnosis) <
            static_cast<int>(want)) {
          Diagnose(player.medical, player.climber, player.cash, want, world,
                   player.day, mdl);
        }
        // And the operation, when the scan says it is that bad. Nobody
        // operates on a guess, so this arm only exists for `careful`.
        if (medCareful) {
          HaveSurgery(player.medical, player.climber, player.cash,
                      player.day, mdl);
        }
        if (StageIsDone(player.medical, player.day)) {
          NextStage(player.medical, player.climber, world, player.day, mdl);
        }
      } else {
        // Never pay to look. Take the shot the moment it is offered, and
        // come back the day after whatever anybody says.
        if (player.medical.stage == Comeback::Resting &&
            player.cash >= mdl.cortisoneCost) {
          TakeTheShot(player.medical, player.climber, player.cash,
                      player.day, mdl);
        }
        NextStage(player.medical, player.climber, world, player.day, mdl);
      }
      t.medicalSpend += before - player.cash;
    }
    // **The things that are wrong with you that are not the injury.**
    // Cheap, boring, and all three of them are decisions this game has
    // never asked anybody to make.
    if (medUpkeep) {
      const AilmentDials ald;
      double h = today.hour;
      if (DoPrehab(player.upkeep, h, player.day, ald)) {
        PassHours(today, h - today.hour, dd);
        t.prehabDays++;
      }
      if (player.sickness.active && !player.sickness.medicated) {
        if (TakeSomethingForIt(player.sickness, player.cash, ald)) {
          t.medsTaken++;
        }
      }
      // **While it is still a filling.** The whole test of the tooth is
      // whether a career will spend on something that is not climbing.
      if (player.teeth.stage != ToothStage::Fine) {
        if (FixTheTooth(player.teeth, player.cash, player.day, ald)) {
          t.toothFixes++;
        }
      }
    }
    if (medInsured && !player.medical.insured) {
      BuyInsurance(player.medical, player.climber, player.day);
    }
    if (medUninsured && player.medical.insured) {
      CancelInsurance(player.medical);
    }

    // The body, before anything is decided. Being hurt is the first thing
    // you know about a day, not a thing you discover at the crag.
    const bool hurt = IsHurt(player.climber);
    if (hurt) t.hurtDays++;
    t.loadSum += player.climber.load;
    t.peakLoad = std::max(t.peakLoad, player.climber.load);
    if (hurt && !kept) {
      const double before = player.cash;
      if (Physio(player.climber, player.cash, player.lastPhysioDay,
                 player.day)) {
        t.physioSessions++;
        t.spentPhysio += before - player.cash;
        note = "physio";
      }
    }

    // Who is calling. Offers are checked monthly, because a sponsor is not
    // watching you every Tuesday.
    SponsorDials sp;
    if (takesDeals && player.day % 30 == 1) {
      const CareerSummary c = SummarizeCareer(player);
      int fas = 0;
      for (const ProjectMemory& m : player.projects) if (m.firstAscent) fas++;
      const SponsorTier offered =
          OfferFor(c.hardestSendGrade, fas, player.standing, sp);
      if (static_cast<int>(offered) > static_cast<int>(player.sponsor.tier)) {
        player.sponsor.tier = offered;
        player.sponsor.gradeAtLastReview = c.hardestSendGrade;
        SignedWith(offered, player.standing);
        t.dealsSigned++;
        note = std::string("signed: ") + SponsorTierName(offered);
      }
      const double paid = MonthlyStipend(player.sponsor.tier, sp);
      if (paid > 0.0) { Pay(player, paid); t.sponsorPay += paid; }
    }

    // And once a year they decide whether to keep you. ReviewSeason was
    // called by nothing anywhere until today — engine or probe — so no
    // career, played or simulated, had ever been reviewed. Matches the
    // engine, which does this from Sleep on the same cadence.
    if (player.sponsor.tier != SponsorTier::None && player.day > 1 &&
        player.day % 365 == 1) {
      ReviewSeason(player.sponsor, SummarizeCareer(player).hardestSendGrade,
                   player.sponsor.daysHurtThisSeason, sp);
      player.sponsor.daysHurtThisSeason = 0;
    }
    if (player.sponsor.tier != SponsorTier::None) t.sponsoredDays++;

    // Shopping, before the day gets spent. A dirtbag buys in this order:
    // the cheap thing that works, then the landing, then the roof over
    // winter — and never so deep that the bills go unpaid, because being
    // behind is worse than being unequipped.
    KitDials kd;
    if (foamCap > 0.0) kd.mostFoamCanDo = foamCap;
    if (buysKit || savesUp) {
      const double float_ = 150.0;   // never spend the last of it
      if (!player.kit.hangboard && player.cash > kd.hangboardCost + float_) {
        if (BuyHangboard(player.kit, player.cash, kd)) t.spentKit += kd.hangboardCost;
      }
      // A winter membership, which is the only kind anyone actually buys.
      // Renewing it whenever it lapsed bought 126 days and used 27, because
      // most of the year the rock is right there — that was the probe
      // shopping badly, not the gym being a bad deal, and reporting the
      // second while the first was true is how you tune away a mechanic
      // that was working.
      const bool shortDays = DaylightHours(player.day, cd) < 10.0;
      if (shortDays && !IsGymMember(player.kit) &&
          player.cash > kd.membershipCost + float_) {
        if (RenewMembership(player.kit, player.cash, kd))
          t.spentKit += kd.membershipCost;
      }
      if (player.kit.pads < kd.padsThatMatter &&
          player.cash > kd.padCost + float_) {
        if (BuyPad(player.kit, player.cash, kd)) t.spentKit += kd.padCost;
      }
    }

    // The rack, which is the one purchase that opens a crag rather than
    // improving a day -- and by some distance the most expensive thing on
    // any shelf in the game, which is the point of measuring whether a
    // career ever gets to the top of it.
    if (leads) {
      const TradDials td;
      const RackTier have = TierOf(player.rack, td);
      if (have != RackTier::Doubles) {
        const RackTier want = static_cast<RackTier>(static_cast<int>(have) + 1);
        // A bigger float than the kit's: nobody spends their last thousand
        // dollars on cams, and the first rack is worth waiting for in a way
        // a second pad is not.
        const double keep = 150.0;
        if (player.cash > RackPrice(want, td) + keep) {
          const double before = player.cash;
          if (BuyRack(player.rack, player.cash, td)) {
            t.spentRack += before - player.cash;
            if (have == RackTier::None) t.gotTheRackOnDay = player.day;
          }
        }
      }
      // And once you own one, the buttress is where you go. Before that it
      // is an hour's walk to rock you cannot lead, which is exactly the
      // gate the purchase opens.
      crag = CanLeadTrad(player.rack) ? TheOldButtress(world) : RoadsideCrag(world);
    }
    // The rope crag. No purchase gates it -- what gates it is somebody being
    // willing to stand under you, which is the whole of what the Lot is for
    // and has never once been asked for over a career.
    if (clips) crag = ShadedCave(world);

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
      needMoney = !kept && (player.cash < 120.0 || player.owed > 0.0);
      // A saver also works on any day they are short of the pad, which is
      // the only thing in the game worth saving for. Once it is bought they
      // go back to the thermostat -- nobody keeps working for its own sake.
      if (savesUp && player.kit.pads < kd.padsThatMatter &&
          player.cash < kd.padCost + 150.0) {
        needMoney = true;
      }
      // A hoarder works towards a number instead of a thermostat. This is
      // the only policy that ever turns a climbing day into a working day
      // for something other than rent, which is what a dream would do.
      if (hoards && player.cash < savingsTarget) needMoney = true;
      // **A leader works for the rack**, which is the only reason anybody
      // in this game has ever turned a climbing day into a working day for
      // a piece of equipment. Measured without it, three thirty-year
      // careers bought a set of nuts on day one for $190 and never once
      // held enough cash to consider cams: the career's balance oscillates
      // between about $125 and $290 all its life, so the top two rungs of
      // the shelf were decoration. What this now measures is the honest
      // question -- not whether a rack is affordable, but what it costs in
      // climbing days.
      if (leads) {
        const TradDials td;
        const RackTier have = TierOf(player.rack, td);
        if (have != RackTier::Doubles) {
          const RackTier want =
              static_cast<RackTier>(static_cast<int>(have) + 1);
          if (player.cash < RackPrice(want, td) + 150.0) needMoney = true;
        }
      }
      // A dreamer never has to work while the War Chest is running. That is
      // the whole of what it bought.
      if (NoNeedToWork(player.dreams)) needMoney = false;
      if (needMoney) {
        const std::vector<OddJob> board = OddJobBoard(world, player.day, jd);
        const OddJob* best = nullptr;
        for (const OddJob& g : board) {
          if (g.needsVan && !VanRuns(player.van)) continue;
          // A careful player turns down the gig that pays best, because
          // they have seen what it does to the gate.
          if (mindReputation &&
              g.name.find("guidebook") != std::string::npos &&
              StandingWith(player.standing, Faction::Stewardship) < -0.15) {
            continue;
          }
          if (!best || g.pay > best->pay) best = &g;
        }
        if (best) {
          // Earned is what the gig paid, not what reached the pocket —
          // debt takes its cut first and that is not lost income.
          // **The hard way when the trade can carry it**, which is the
          // whole decision: reaching past your craft is how you botch it,
          // and the easy answer is always there and never gets you
          // anywhere. A probe that always ducked would measure a game
          // nobody plays.
          const ShiftMoment moment =
              MomentOnShift(CraftForGig(best->name), world, player.day);
          const bool goForIt =
              moment.happened &&
              (workHard ||
               player.hand.skill[static_cast<int>(moment.craft)] >=
                   moment.needs);
          if (WorkOddJob(player, today, *best, world, goForIt, dd)) {
            t.earned += best->pay;
            t.daysWorked++;
            note = "gig";
            if (best->name.find("guidebook") != std::string::npos)
              t.photoGigs++;
            if (best->name.find("trail work") != std::string::npos)
              t.trailGigs++;
          }
        }
      }
    }

    // Drive to the crag and back, if the van goes, and if there is any
    // point. (The first version of this drove out on every day with a
    // window, including days the gate was locked and days the player had
    // already decided to rest — a hundred pointless hours a year, which
    // stopped being free the moment fuel had a price.)
    VanDials vd;
    if (kept) vd.fuelPerHour = 0.0;
    const bool worthTheDrive =
        win.exists && CragIsOpen(player.standing) &&
        player.climber.skin >= restUntilSkin &&
        LastLightHour(player.day, cd) > today.hour + 0.5;
    if (VanRuns(player.van) && worthTheDrive) {
      Charge(player, FuelFor(1.0, vd));
      t.spentFuel += FuelFor(1.0, vd);
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
      Rest(today, 4.0, player.life, dd);
      t.daysRested++;
      note = note.empty() ? "resting skin" : note + " + resting skin";
    }

    const bool shut = !CragIsOpen(player.standing);
    if (shut) {
      t.closedDays++;
      if (note.empty()) note = "crag closed";
    }

    const bool stranded = !VanRuns(player.van);
    if (IsGymMember(player.kit)) t.memberDays++;

    // Climbing on a bad one is how a fortnight becomes a season. A mild
    // tweak you work around by getting on the holds that do not hurt; past
    // half severity nobody sensible pulls on at all.
    const bool tooHurt = hurt && player.climber.injury.severity > 0.5;

    // The day they own. Only ever one with a window — you cannot shoot
    // climbing photos in the rain — which is the whole mechanic: a shift
    // takes a spare day and a shoot takes the one you wanted.
    const bool theirDay =
        ObligationToday(player.sponsor, world, player.day, win.exists, sp);
    if (theirDay) {
      PassHours(today, sp.obligationHours, dd);
      today.energy = std::max(0.0, today.energy - sp.obligationEnergy);
      player.sponsor.obligationsMetThisSeason++;
      t.obligationDays++;
      note = note.empty() ? "their day, not yours"
                          : note + " + their day, not yours";
    }

    if (!win.exists || tooThin || stranded || shut || tooHurt || theirDay) {
      // The day the rock said no. This is the pile the kit exists to reach:
      // 157 washed out, 75 more resting skin, and until now every one of
      // them was dead time no amount of money could touch.
      bool salvaged = false;
      if (!tooThin && GoToTheGym(player, today, kd, dd)) {
        // Plastic ignores the weather, and the gym board is the one wall
        // that is always in.
        const std::vector<Route> board = GymBoard(world);
        Rng session =
            Rng::FromSeed(seed + "#gym" + std::to_string(player.day));
        const Climber body = ClimberForSession(player, today, dd);
        const Route* pick = nullptr;
        for (const Route& r : board) {
          const RouteRead read = ReadRoute(body, r);
          if (read == RouteRead::NotThisYear) continue;
          if (!pick || r.trueGrade > pick->trueGrade) pick = &r;
        }
        if (pick) {
          ProjectMemory& mem = MemoryFor(player, *pick);
          while (today.session.skinLeft > 0.5 && today.hour < 21.0) {
            const AttemptResult r =
                AttemptInSession(session, today.session, mem, body, *pick,
                                 Conditions{}, {}, 0.72, SessionDials{},
                                 SessionLoopDials{}, player.character,
                                 player.medical, player.day,
                                 player.sickness, player.teeth,
                                 player.quirks, player.logbook);
            ApplyAttemptToDay(player, today, *pick, r, world, dd);
            t.burns++;
            t.movesClimbed += static_cast<int>(r.timeline.size());
          }
          t.gymDays++;
          salvaged = true;
          note = note.empty() ? "plastic" : note + " + plastic";
        }
      }
      // Only on skin the weather was going to waste anyway. This is the
      // whole of what separates the board from a mistake, and the margin is
      // not subtle: hanging whenever the day was dead gave 0 sends across
      // five seasons, and hanging only on genuinely surplus skin gave 24
      // against a non-owner's 12. Same item, same price, opposite sign.
      //
      // Which makes sense once measured — skin is conserved, so an hour on
      // the board spends the crag's budget unless the crag was never going
      // to get it. Resting to 3.0 and then boarding at 3.1 is paying for
      // training with the session you were resting for.
      // And not when the warning light is on. The board is what redlines
      // you — it is the one thing in the game that loads tendons without
      // the weather getting a say — so a player who listens to their body
      // stops hanging before the threshold rather than after.
      if (!salvaged && player.climber.skin > 7.5 &&
          player.climber.load < BodyDials{}.injuryThreshold * 0.8 &&
          HangboardSession(player, today, kd, dd)) {
        t.boardDays++;
        salvaged = true;
        note = note.empty() ? "an hour on the board"
                            : note + " + an hour on the board";
      }
      if (!win.exists) {
        if (!needMoney && !tooThin && !salvaged) {
          Rest(today, 4.0, player.life, dd);
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
        Rest(today, win.startHour - today.hour, player.life, dd);
      }
      const bool missedIt = today.hour > win.endHour;
      if (missedIt) t.missedWindows++;

      // **The walk in.** `Crag::approachHours` is set by all four crags and
      // the engine charges it -- `ApproachHoursFor` feeds the travel spot --
      // and **this probe has never paid a minute of it.** Every career
      // number in every note in this repo was measured by a climber who
      // teleported to the rock, which on the buttress is 2.2 hours of a
      // fourteen-hour day, unpaid, every day for thirty years.
      //
      // Paid here rather than folded into the window, because it is the
      // reason a further crag is a *decision*: it comes out of the same
      // daylight the session does.
      PassHours(today, crag.approachHours, dd);
      t.approachHours += crag.approachHours;

      // The light is the hard stop. Nothing else in the day was one: before
      // this, a salaried player who got out at five still climbed a full
      // window's worth of burns, in the dark, in December.
      const double dusk = LastLightHour(player.day, cd);

      StartGymSession(player, today, kd, dd);
      const Climber body = ClimberForSession(player, today, dd);
      // Before climbing: put the brush on the lines you want but cannot do.
      // Half an hour each, which is real time out of a real window -- the
      // claim is not free, and that is the point of measuring it.
      if (stakesClaims) {
        for (const CragLine* p : OpenProjects(crag)) {
          if (today.hour >= dusk) break;
          ProjectMemory& m = LedgerFor(player, *p);
          if (m.cleanliness > 0.4 || m.sent) continue;
          CleanLine(player, today, m, 0.5, fd, dd);
        }
      }

      // **Somebody has to hold the rope.** The one thing in this game that
      // genuinely requires the Lot to exist, and the reason a pitch is a
      // different *decision* from a boulder rather than a longer one: a
      // boulder is something you can always do alone at dawn, and a pitch
      // is something you have to have arranged.
      //
      // How many burns you get is how well they know you -- a stranger
      // holds your rope for a couple of laps because that is what people do
      // at a crag, and standing under somebody all afternoon while they
      // work the same three moves is a favour.
      int burnsAllowed = INT_MAX;
      if (NeedsABelayer(crag.lines.empty() ? Route{} : crag.lines[0].route)) {
        const Partner* belayer = BestBelayer(hereToday);
        if (!belayer) {
          // Walked up there and there is nobody. Not a rest day and not a
          // washout -- a different way for a day to go wrong, and one only
          // a rope crag has.
          //
          // **Zero burns rather than `continue`.** The first version of this
          // skipped to the next day, which also skipped the Lot's own
          // climbing, the rival, the crew and the night's bookkeeping -- a
          // day that went wrong for the player is still a day the valley
          // had. It has never fired (see below), which is exactly why it
          // was worth getting right: an untaken branch that is also broken
          // is a bug with a fuse on it.
          t.noBelayerDays++;
          note = "nobody to tie in with";
          burnsAllowed = 0;
        } else {
          burnsAllowed = BurnsTheyWillHold(*belayer);
        }
      }

      const CragLine* line = burnsAllowed == 0
                                 ? nullptr
                                 : PickLine(crag, body, player, racesBack);
      if (line) {
        ProjectMemory& mem = LedgerFor(player, *line);

        // Clean it if it needs it — the cost that comes before any chance.
        //
        // A projector keeps brushing past workable. Everyone else stops at
        // the minimum, which on a project is the difference between a 0%
        // line and a 67% one, and is why the valley looked used up.
        const double brushUntil = (projects || stakesClaims) ? 0.98 : 0.0;
        while ((!IsWorkable(mem, fd) || mem.cleanliness < brushUntil) &&
               today.hour < win.endHour && today.hour < dusk)
          CleanLine(player, today, mem, 0.5, fd, dd);

        // Conditions where the clock actually is, not where the window was.
        const double climbAt =
            missedIt ? today.hour : win.peakHour;
        const Conditions cond = ConditionsAt(w, crag.aspect, climbAt, cd);
        Rng session = Rng::FromSeed(seed + "#day" + std::to_string(player.day));
        int burnsToday = 0;
        while (today.session.skinLeft > 0.5 && today.hour < dusk &&
               burnsToday < burnsAllowed &&
               burnsToday < static_cast<int>(win.hours() / 0.25) + 4) {
          // The two halves the engine's DayAttempt node wraps: resolve the
          // burn against the session, then let the day pay for it.
          const AttemptResult r =
              AttemptInSession(session, today.session, mem, body, line->route,
                               cond, {}, 0.72, SessionDials{},
                               SessionLoopDials{}, player.character,
                               player.medical, player.day,
                               player.sickness, player.teeth,
                               player.quirks, player.logbook);
          ApplyAttemptToDay(player, today, line->route, r, world, dd);
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
          // What the lead was actually like. Recorded here rather than
          // inferred from the send, because the interesting thing about a
          // trad career is not how often it tops out -- it is how much of
          // it was spent above bad gear.
          if (line->route.discipline == Discipline::Trad) {
            t.leadsOnGear++;
            int pieces = 0;
            for (double q : r.gear.quality) {
              if (q > 0.0) pieces++;
            }
            t.piecesPlaced += pieces;
            double worst = 0.0;
            const int reached =
                std::min(r.highpoint,
                         static_cast<int>(line->route.moves.size()) - 1);
            for (int i = 0; i <= reached; i++) {
              worst = std::max(worst, ExposureAt(line->route, i,
                                                 today.session.padding,
                                                 SessionDials{}, r.gear));
            }
            t.leadFearSum += worst;
            // Emptied the harness. The first version of this asked
            // whether the top third was unprotected, which measures
            // soloing the *bottom* two thirds and came out zero across
            // 2,449 leads -- a metric that cannot fire is worse than no
            // metric, because it reads as a system behaving itself.
            if (pieces >= today.session.rack.pieces) t.ranItOut++;
          }

          if (burnsAllowed != INT_MAX) t.burnsTheyHeld++;

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
            // **Beating them to it.** The engine does this on the naming
            // path; a probe that did not would report the racer as losing
            // every race it actually won, which is a measurement of a game
            // nobody plays.
            if (RaceIsOn(player.rival) &&
                player.rival.race.routeName == line->route.name) {
              YouWonTheRace(player.rival);
              t.racesWon++;
            }
            if (CanName(*line, mem)) {
              // ClaimFirstAscent, not NameFirstAscent: the probe had the
              // same half-a-verb bug the engine did, so every simulated
              // first ascent was worth no opinion to any faction — and
              // standing is what earns a sponsor, so the probe has been
              // measuring a career the Lot never noticed.
              ClaimFirstAscent(player, mem, *line,
                               "Line " + std::to_string(player.day));
              t.firstAscents++;
              note += (note.empty() ? "" : " + ");
              note += "FIRST ASCENT of " + line->description;
            }
            break;
          }
        }
        // **Whose patience ended the day.** The interesting case is the
        // one where you had skin left and light left and the belayer had
        // simply had enough -- that is the number the whole "a stranger
        // gives you a couple, somebody who knows you gives you the day"
        // design turns on, and nothing has ever counted it.
        if (burnsAllowed != INT_MAX && burnsToday >= burnsAllowed &&
            today.session.skinLeft > 0.5 && today.hour < dusk) {
          t.belayerCappedDays++;
        }
        if (burnsToday > 0) { t.daysClimbed++; if (note.empty()) note = "climbed"; }
        {
          double worst = 0.0;
          for (int i = 0; i < kSkillCount; i++) {
            worst = std::max(worst, player.monotony.level[i]);
          }
          t.monotonyPeak = std::max(t.monotonyPeak, worst);
          if (worst >= MonotonyDials{}.breakthroughAt) t.plateauDays++;
        }
      }
    }

    // Rubber: resole while the uppers hold, replace when they do not.
    GearDials gd;
    if (!neverBuysRubber && player.shoes.wear > gd.noticeablyWorn) {
      const double before = player.cash;
      const bool shod = CoversShoes(player.sponsor);
      if (CanResole(player.shoes, gd)) {
        if (Resole(player.shoes, player.cash, shod, gd)) {
          t.resoles++;
          t.spentShoes += before - player.cash;
        }
      } else if (BuyNewShoes(player.shoes, player.cash, shod, gd)) {
        t.newPairs++;
        t.spentShoes += before - player.cash;
      }
    }
    if (player.shoes.wear > 0.8) t.deadRubberDays++;

    // The Lot lives its life.
    std::vector<Partner> lot = LotRegulars(world, player.day);
    ApplyBonds(lot, player.bonds);
    for (Partner& p : lot) {
      // **Rapport moves with people you actually saw.** A day you climbed
      // is not a day you climbed *with them* if they were not there, and
      // before turnout existed there was no difference between the two.
      bool sawThem = false;
      for (const Partner& h : hereToday) sawThem = sawThem || h.name == p.name;
      // **And how you turned up smells.** The engine reads grime into the
      // three social gains through `GrimeSocial`; the probe has to read it
      // the same way or it is measuring a cleaner climber than the one the
      // player is.
      SpendDayWith(p, sawThem && t.daysClimbed > climbedBefore,
                   GrimeSocial(player.living.grime));
      std::vector<std::string> taken = lotTaken;
      for (const ProjectMemory& m : player.projects)
        if (m.firstAscent) taken.push_back(m.routeName);
      PartnerDials lotDials;
      if (lotChance >= 0.0) lotDials.firstAscentChancePerDay = lotChance;
      // Everything claimed, plus everything the player is visibly on.
      std::vector<std::string> offLimits = taken;
      for (const std::string& s : SpokenFor(player.projects)) {
        offLimits.push_back(s);
      }
      const int got = PartnerTakesFirstAscent(world, p, crag, offLimits,
                                              player.day, lotDials);
      if (got >= 0) {
        lotTaken.push_back(crag.lines[got].route.name);
        p.firstAscents.push_back(crag.lines[got].route.name);
        // Into the book, or the line stays an open project with nobody's
        // name on it and the player can still claim the first ascent of
        // something Dev did last spring. Ninety years of this probe had the
        // Lot take five lines and the guidebook show none of them.
        TheyPutUpTheLine(crag.lines[got], p.name);
        t.linesLostToTheLot++;
        t.lotNames.push_back(DisplayName(crag.lines[got]) + " (" + p.name + ")");
        note += (note.empty() ? "" : " + ");
        note += p.name + " got " + crag.lines[got].description + ", calling it " +
                DisplayName(crag.lines[got]);
      }
    }
    // **And the rival, through the same machinery the engine uses.** The
    // Lot's loop above is replicated here because the probe has to run what
    // the game runs; the rival is no different, and leaving them out would
    // make every Phase 8 measurement a measurement of a game nobody plays.
    if (!player.rival.name.empty() && !player.rival.retired &&
        !player.rival.allied) {
      std::vector<std::string> offLimits = lotTaken;
      for (const std::string& s : SpokenFor(player.projects)) {
        offLimits.push_back(s);
      }
      Partner them = AsAClimber(player.rival, world, player.day);
      const int got = PartnerTakesFirstAscent(world, them, crag, offLimits,
                                              player.day);
      if (got >= 0) {
        lotTaken.push_back(crag.lines[got].route.name);
        TheyPutUpTheLine(crag.lines[got], player.rival.name);
        TheyGotThereFirst(player.rival, crag.lines[got].route.name);
        player.rival.met = true;
        t.linesLostToTheRival++;
      }
      // **The race, which is the thing that is supposed to change what you
      // climb.** The probe's policies are fixed heuristics and none of them
      // reads it -- that is exactly the finding, and the counters here are
      // what say so with a number rather than an opinion.
      if (RaceRanOut(player.rival, player.day)) {
        for (CragLine& L : crag.lines) {
          if (L.route.name == player.rival.race.routeName &&
              player.rival.race.forFirstAscent) {
            TheyPutUpTheLine(L, player.rival.name);
            lotTaken.push_back(L.route.name);
            break;
          }
        }
        TheyWonTheRace(player.rival);
        t.racesLost++;
      } else if (!RaceIsOn(player.rival)) {
        std::vector<std::string> raceOff = lotTaken;
        for (const std::string& s : SpokenFor(player.projects)) {
          raceOff.push_back(s);
        }
        const double yours =
            SkillToGrade((player.climber.skills.power +
                          player.climber.skills.fingers +
                          player.climber.skills.technique +
                          player.climber.skills.endurance +
                          player.climber.skills.head) / 5.0);
        if (StartARace(player.rival, crag, yours, raceOff, world,
                       player.day)) {
          t.racesStarted++;
        }
      }

      if (WouldPartnerUp(player.rival)) {
        player.rival.offered = true;
        // The probe takes it, and that is a *policy* rather than the rule:
        // the game offers it on two keys at the van. A probe that always
        // said no would never measure what an ally does to a career, and
        // one that could not answer at all would be the old bug back.
        player.rival.allied = true;
      }
    }
    t.rivalGenerations = player.rival.generation;

    player.bonds = BondsFrom(lot);

    // Evening: eat if the day has made you hungry and you can afford it.
    if (today.hunger > 28.0) {
      const double before = player.cash;
      // **What the counter is holding, before the meal spends it.** Read
      // rather than derived: `WhatTheySay` is what the game shows, so
      // asking anything else here would be the probe measuring a different
      // greeting from the one the player gets.
      Heard held = Heard::None;
      if (const Local* who = At(player.locals, Service::Meal)) {
        held = who->holds;
      }
      if (EatMeal(player, today, dd)) {
        t.mealsEaten++;
        t.spentFood += before - player.cash;
        if (!today.heard.empty()) {
          t.greeted++;
          if (t.firstGreetingDay < 0) t.firstGreetingDay = player.day;
          t.lastSaid = today.heard;
          t.greetedByKind[static_cast<int>(held)]++;
        }
      } else {
        t.brokeDays++;
      }
    }

    const bool wasOpen = CragIsOpen(player.standing);
    FactionDay(player.standing, world, player.day);
    if (wasOpen && !CragIsOpen(player.standing)) {
      t.closures++;
      note += (note.empty() ? "" : " + ");
      note += "ACCESS PULLED";
    }

    // One dream per career now, chosen up front -- the choice rotates with
    // the generation so the sweep sees all three lived. Buy it the day it
    // can be covered.
    if (dreams) {
      if (player.dreams.chosen == Dream::None) {
        const Dream order[3] = {Dream::Rig, Dream::WarChest, Dream::HomeBase};
        ChooseDream(player.dreams, order[lives % 3]);
      }
      if (BuyDream(player.dreams, player.van, player.cash,
                   player.dreams.chosen)) {
        t.dreamsBought++;
      }
    }

    DogDay(player.dog, !needMoney, dog);
    WeatherProjects(player, fd);
    if (today.hunger > dd.starvingHunger) t.starvedNights++;

    if (player.sickness.active) t.sickDays++;
    if (player.teeth.stage != ToothStage::Fine) t.toothDays++;
    t.timesIll = player.sickness.caught;
    t.worstTooth = player.teeth.worstEver;
    t.diagnoses = player.medical.diagnoses;
    t.shots = player.medical.shotsTaken;
    t.surgeries = player.medical.surgeries;
    t.untreated = player.medical.untreatedInjuries;
    t.premiums = player.medical.premiumsPaid;
    t.claims = player.medical.claimsPaid;

    t.momentsTaken = player.hand.momentsTaken;
    t.momentsBotched = player.hand.momentsBotched;
    t.momentsDucked = player.hand.momentsDucked;
    t.sackings = player.hand.sackings;

    t.cashLow = std::min(t.cashLow, player.cash);
    t.cashHigh = std::max(t.cashHigh, player.cash);
    // Read off the season rather than accumulated, because these are the
    // career counters and they already are the total.
    t.wcStarts = player.worldCup.starts;
    t.wcMissed = player.worldCup.missed;
    t.wcPodiums = player.worldCup.podiums;
    t.wcWins = player.worldCup.wins;
    t.wcTitles = player.worldCup.titles;

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
    // The night's roll happens inside SleepToNextDay, which is the point of
    // it living there — so an injury is noticed the way the player notices
    // one, by waking up with it.
    const bool wasHurt = hurtAtDawn;
    const double sev = sevAtDawn;
    const int left = leftAtDawn;
    if (wasHurt && today.atGym) t.climbedHurtDays++;
    // Whether the day made it worse is asked before sleep, because sleep
    // is what heals: a night's recovery would mask an aggravation that
    // cost more than the night gave back.
    const bool worsened = wasHurt && IsHurt(player.climber) &&
                          (player.climber.injury.severity > sev + 1e-9 ||
                           player.climber.injury.daysLeft > left);
    // **The evening**, and the day it sometimes takes instead.
    //
    // Books turn up after the first month and the guitar after the second
    // -- neither is a purchase yet, and inventing a shop to buy them in
    // would be a mechanic built to fill a column. **Somebody arrives
    // through the scene**, which is the one that is not arbitrary: you
    // meet people at the fire, so a career that never gets to know
    // anybody at the Lot never has anybody to neglect.
    {
      const LifeDials ld;
      double known = 0.0;
      for (const PartnerBond& b : player.bonds) {
        known = std::max(known, b.rapport);
      }

      // Keeping something going beats starting something new, always --
      // otherwise a career collects five threads in a fortnight and then
      // watches them all go cold at once.
      Thread needy = Thread::None;
      double worst = 0.0;
      for (int i = 1; i < kThreadCount; i++) {
        const Thread th = static_cast<Thread>(i);
        double want = 0.0;
        if (Going(player.life, th)) {
          want = static_cast<double>(DaysSince(player.life, th, player.day)) -
                 ld.patienceDays[i];
        } else {
          const bool offered =
              (th == Thread::Books && player.day > 30) ||
              (th == Thread::Music && player.day > 60) ||
              (th == Thread::Someone && known >= 0.5);
          // Barely above nothing: starting is what you do on a day with
          // nothing to keep up.
          want = offered && CanTakeUp(player.life, th, player.day, ld) ? 0.01
                                                                      : 0.0;
        }
        if (want > worst) {
          worst = want;
          needy = th;
        }
      }
      if (needy == Thread::None) goto noEvening;

      // **The gate is the whole measurement.** How long one go takes is the
      // sim's answer and it differs by an order of magnitude across the
      // five: half an hour on the phone fits after any day, and going to
      // see somebody takes six.
      //
      // The first version priced everything at a two-hour evening and gated
      // on 21:00, so every thread fitted every night: 1,138 evenings across
      // thirty years and **nothing ever lost**.
      const double asks = AsksFor(needy);
      const bool roomForIt = today.hour + asks <= 22.0;

      // And what a policy is actually for. Everybody keeps up what fits in
      // the leftovers. **Giving somebody a whole day competes with the
      // rock**, so it happens when the rock is not in condition -- which is
      // the dirtbag's real priority order, and it means a good season costs
      // you. The `lifer` refuses to let the weather decide.
      const bool wouldBother =
          makesTime || asks <= 3.0 || !win.exists;
      if (wouldBother && roomForIt) {
        const bool wasGoing = Going(player.life, needy);
        if (SpendTheEvening(player, today, needy, dd)) {
          t.eveningsSpent++;
          if (!wasGoing && needy == Thread::Someone &&
              t.hadSomebodyOnDay < 0) {
            t.hadSomebodyOnDay = player.day;
          }
          if (needy == Thread::Music) {
            t.buskSessions++;
            // Off the sim's own answer rather than off the cash, which pays
            // debt first and would under-report every broke year -- and
            // which, read as a delta, was reporting eight times the money
            // that was ever earned.
            t.busked += BuskingPay(player.life, AsksFor(needy));
          }
        }
      }
    }
  noEvening:;

    // **The one purchase above the dreams' range.** Bought the day it is
    // affordable, with the levers left where they start -- a policy that
    // tuned them would be measuring my tuning rather than the system. The
    // books themselves tick inside SleepToNextDay, like everything else
    // that counts down at night.
    if (buysAGym && !player.gym.owned && player.cash >= GymDials{}.price) {
      BuyTheGym(player.gym, player.cash, "The Woodshed", player.day);
      t.boughtGymOnDay = player.day;
      t.gymSpent += GymDials{}.price;
    }

    // **`gym=run`: the levers pass two put on the counter.** Every choice
    // here is the simple one on purpose -- the top of the shortlist, the
    // wings in table order, always say yes to a raise, always pay the bill
    // if the money is there. A policy that picked cleverly would be
    // measuring my picking rather than the system.
    if (runsAGym && player.gym.owned) {
      const GymDials gd;

      // The clipboard first, because it is the thing with a clock on it.
      if (player.gym.incident != GymIncident::None) {
        const GymIncidentDef* def = IncidentDef(player.gym.incident);
        const int which =
            def != nullptr && player.cash >= def->choices[0].cost ? 0 : 1;
        const IncidentAnswer said =
            AnswerTheIncident(player.gym, player.cash, which, gd);
        if (said.answered) {
          Shift(player.standing, Faction::Scene, said.standing / 100.0);
          if (which == 0) t.gymFixed++; else t.gymLeft++;
          if (def != nullptr) t.gymSpent += def->choices[which].cost;
        }
      }

      // Tenure. Always granted -- refusing is the interesting choice and
      // therefore the one a measurement policy must not make for you.
      for (int seat = 0; seat < 2; seat++) {
        const bool desk = seat == 0;
        if (!IsAskingForARaise(player.gym, desk, player.day, gd)) continue;
        const RaiseAnswer said =
            AnswerTheAsk(player.gym, desk, true, player.day, gd);
        if (said == RaiseAnswer::Granted) t.gymRaises++;
        if (said == RaiseAnswer::TheyQuit) t.gymQuit++;
      }

      // The desk before the setter, whoever is at the top of the list.
      for (int seat = 0; seat < 2; seat++) {
        const bool desk = seat == 0;
        if (desk ? player.gym.frontDesk : player.gym.setter) continue;
        const double was = player.cash;
        if (Hire(player.gym, player.cash, desk, player.day, 0, gd)) {
          t.gymHired++;
          t.gymSpent += was - player.cash;
        }
        break;   // one hire a day, the way one purchase a day is one day
      }

      // **Climb the equipment ladder**, which pass two's policy skipped --
      // and that omission is why `gym=run` never once bid to host: the
      // federation will not put a national number on as-bought walls, so
      // every bid was refused at a gate the policy had no way to pass.
      if (player.gym.equip != GymEquip::FullRenovation) {
        UpgradeEquipment(player.gym, player.cash, gd);
      }

      // Then build onto it, in table order, one a day.
      for (int w = 0; w < kGymWingCount; w++) {
        const GymWing wing = static_cast<GymWing>(w);
        if (HasWing(player.gym, wing)) continue;
        const double was = player.cash;
        if (BuildWing(player.gym, player.cash, wing, gd)) {
          t.gymWings++;
          t.gymSpent += was - player.cash;
        }
        break;
      }

      // And once both seats are filled, hand it over.
      if (!player.gym.passive && SetHandsOff(player.gym, true)) {
        t.gymHandsOffDay = player.day;
      }

      // GYM-2/GYM-5. The floor first, then the comp -- one is an hour and
      // the other is an evening, and doing the cheap one first is what
      // anybody would do.
      if (livesInIt || paysACoach) {
        const bool hadCohort = player.floor.waveTwoArrived;
        if (WalkTheGymFloor(player, today, dd)) {
          t.gymWalks++;
          if (!hadCohort && player.floor.waveTwoArrived) {
            t.gymCohortDay = player.day;
          }
        }
        if (HostCompNight(player, today, world, dd)) {
          t.gymComps++;
        }

        // **GYM-8.** Founded the moment the question has been asked and the
        // money is there, and then a session every night it will take one.
        // Coached by you under `gym=life` and by somebody you pay under
        // `gym=hired` -- the two cannot be measured at once.
        // **GYM-10.** Start one the day the room is big enough, on
        // whatever night that is, and then run it every week. The format
        // is `lot=`-style content rather than a policy knob: the probe
        // takes the handicap league, which is the one every room has
        // somebody for, so the measurement is about the institution and
        // not about a format that happened to suit nobody.
        if (!player.gymLeague.running && WhyNotStartTheLeague(player).empty()) {
          StartTheLeague(player, LeagueFormat::Handicap, NightOf(player.day));
        }
        if (WhyNotTheLeagueTonight(player, today).empty()) {
          const int runsWere = player.gymLeague.runs;
          const double was = player.cash;
          if (RunTheLeagueNight(player, today, dd)) {
            t.glNights++;
            t.glTakings += player.cash - was;
            if (player.gymLeague.runs > runsWere) t.glRuns++;
          }
        }

        // **GYM-9.** Bid every season the room is big enough, and on the
        // day, run it. Running rather than climbing is the whole decision
        // and a policy that sometimes did one and sometimes the other
        // would measure neither -- so this one is the host, always, and
        // the cost of that shows up in the circuit columns.
        if (WhyNotBidToHost(player).empty()) {
          const double was = player.cash;
          t.gymBids++;
          if (BidToHostTheSeason(player)) t.gymSeasonsHeld++;
          t.gymBidSpend += was - player.cash;
        }
        if (WhyNotRunTheRound(player).empty()) {
          const double was = player.cash;
          if (RunTheRound(player, today, dd)) {
            t.gymRoundsRun++;
            t.gymRoundPay += player.cash - was;
          }
        }

        if (!player.youth.going && FoundYouthTeam(player, world, dd)) {
          t.youthDay = player.day;
          if (paysACoach) SetTheYouthCoach(player, true, world);
        }
        RunYouthSession(player, today, world, dd);
      }
    }

    const int generationWas = player.rival.generation;
    SleepToNextDay(player, today, world, dd);

    // **The payoff, counted where it lands.** A generation turning over
    // with a name off the graduate list is a kid you coached stepping up --
    // and counting it here rather than off the queue's length is the honest
    // way, because the queue also forgets anybody who aged out of it.
    if (player.rival.generation != generationWas) {

      for (const Graduate& gone : player.youth.graduated) {
        if (gone.name == player.rival.name) { t.youthSteppedUp++; break; }
      }
    }

    if (player.gym.owned) t.gymDaysOwned++;
    if (!player.gymNews.empty()) t.gymLost++;
    if (player.gymNews.find("The bank took") != std::string::npos) {
      t.gymForeclosed++;
    }
    for (int seat = 0; seat < 2; seat++) {
      const GymStaffer* who = WhoIsOn(player.gym, seat == 0);
      if (who != nullptr) t.gymWorstWage = std::max(t.gymWorstWage, who->wage);
    }

    // What the night did to it. Counted rather than asserted, because the
    // question is *how often* and no harness check can ask that.
    if (player.lostToday == Thread::Someone) t.lostSomebodyOnDay = player.day;
    if (player.life.grieving > 0.0) t.grievingDays++;
    if (!LifeLabel(player.life, player.day).empty()) t.nagDays++;
    for (int i = 1; i < kThreadCount; i++) {
      t.deepest = std::max(t.deepest, player.life.strands[i].depth);
    }

    // What the night made of you. Read after the tick, because that is when
    // it is true -- and counted rather than asserted, because the question
    // this probe exists to answer is *how often*, which no harness check
    // can ask.
    {
      const std::vector<Habit> doing = HabitsNow(player.logbook, player.day);
      if (!doing.empty()) t.daysWithAHabit++;
      t.habitDaysSum += static_cast<double>(doing.size());
      if (player.becameToday != Quirk::None) {
        t.quirksEarned++;
        if (t.firstQuirkDay < 0) t.firstQuirkDay = player.day;
        if (!t.became.empty()) t.became += "+";
        t.became += QuirkName(player.becameToday);
      }
    }
    if (!wasHurt && IsHurt(player.climber)) {
      t.injuries++;
      consecutiveInjuries++;
    } else if (worsened) {
      t.aggravations++;
    }
    // **A clean stretch resets the "three in a row" that offers
    // retirement** -- and this has to be the *engine's* clean stretch, not
    // a second opinion about one.
    //
    // It was neither. The probe waited **120** days and only counted a day
    // toward them if you were healthy yesterday too; the engine waits
    // **90** and counts any day you are not hurt. So the measured career
    // was offered the door on a stricter rule than the played one, and
    // `check-parity.py` cannot see it -- both consumers call
    // `TimeToThinkAboutIt`, and the divergence is in what they hand it.
    // *The game runs the rule and disagrees about the input* is the one
    // shape that checker names as its own blind spot.
    if (!IsHurt(player.climber)) {
      sinceHurt++;
      if (sinceHurt >= 90) consecutiveInjuries = 0;
    } else {
      sinceHurt = 0;
    }
    peakGradeEver = std::max(peakGradeEver,
                             SkillToGrade(player.climber.skills.power));
    {
      const Skills& s = player.climber.skills;
      t.peakAllround = std::max(
          t.peakAllround,
          SkillToGrade((s.power + s.fingers + s.technique + s.endurance +
                        s.head) / 5.0));
    }

    // The one thing that ends a career. Never a command: the game offers,
    // and this policy always takes it, because a probe that declines would
    // measure nothing.
    if (multiLife &&
        TimeToThinkAboutIt(player, consecutiveInjuries, peakGradeEver)) {
      // **Which of the two triggers fired**, counted rather than assumed.
      // `TimeToThinkAboutIt` returns one bool for two very different
      // reasons -- a body that keeps breaking, or a grade two off your best
      // after forty-six -- and a career ending at twenty-four means
      // something completely different from one ending at fifty.
      const bool bodyGaveOut =
          consecutiveInjuries >= LegacyDials{}.injuriesInARowToHint;
      // The offer is never a command -- see LegacyDials. A `late` climber
      // hears the body's opinion at twenty-four and keeps climbing.
      if (retirePolicy == "late" && bodyGaveOut &&
          AgeOn(player.day) < LegacyDials{}.retirementAgeHint) {
        t.declinedTheOffer++;
        continue;
      }
      if (bodyGaveOut) t.retiredByBody++;
      else t.retiredByGrade++;
      t.retirementAgeSum += AgeOn(player.day);
      if (AgeOn(player.day) < t.youngestRetirement) {
        t.youngestRetirement = AgeOn(player.day);
      }
      const Legacy done = TallyCareer(player, "Climber " + std::to_string(lives),
                                      1 + player.day / 365);
      legacies.push_back(done);
      // Write them into the book before the next one arrives, so the
      // valley remembers what the ledger cannot: Inherit wipes the personal
      // ledger, correctly, and the page is not personal.
      for (const NamedLine& n : done.firstAscents) {
        for (CragLine& line : crag.lines) WriteIntoTheBook(line, n);
      }
      player = Inherit(done, world);
      // Do NOT carry player.day across. Inherit sets it to 1 on purpose:
      // age is *derived* from the day counter, so resetting the counter is
      // how the next climber is twenty-four. Carrying it over -- which this
      // probe did on its first run -- births the inheritor at the age their
      // predecessor retired, and the whole thirty years reads as a
      // catastrophic skill collapse rather than two careers.
      //
      // The cost of that decision, which is real and belongs in the notes:
      // the world's calendar restarts with them. Generation two climbs
      // generation one's weather.
      lives++;
      player.name = "Climber " + std::to_string(lives);
      consecutiveInjuries = 0;
      sinceHurt = 0;
      peakGradeEver = 0.0;
      today = WakeUp(player, dd);
    }
  }

  // The guidebook, which is Phase 4's actual gate: "a career plays end to
  // end and the guidebook at the end reads like somebody lived there".
  // Nothing has ever printed it, so nothing has ever checked.
  if (multiLife) {
    // The last life never retired, so it never got tallied. Count it, or
    // the guidebook is missing whoever is standing in it -- but say so,
    // because LegacyText only knows how to write an ending and will claim
    // a twenty-five-year-old two seasons in has retired.
    const std::size_t stillGoing = legacies.size();
    legacies.push_back(TallyCareer(player, "Climber " + std::to_string(lives),
                                   1 + player.day / 365));
    printf("\n=== %d lives over %d years ===\n", lives, DAYS / 365);
    // How they ended. A run where most of them ended at twenty-four with a
    // body that kept breaking is not a dynasty, it is a bug or a dial.
    const int ended = t.retiredByBody + t.retiredByGrade;
    printf("ENDINGS\t%d\tbody\t%d\tgrade\t%d\tdeclined\t%.1f\tmean age"
           "\t%.0f\tyoungest\n\n",
           t.retiredByBody, t.retiredByGrade, t.declinedTheOffer,
           ended ? t.retirementAgeSum / ended : 0.0,
           ended ? t.youngestRetirement : 0.0);
    int lineCount = 0;
    for (std::size_t i = 0; i < legacies.size(); i++) {
      if (i == stillGoing) printf("[still climbing]\n");
      printf("%s\n", LegacyText(legacies[i]).c_str());
      for (const NamedLine& n : legacies[i].firstAscents) {
        printf("    %-42s  [key: %s]\n", GuidebookEntry(n).c_str(),
               n.routeKey.c_str());
        lineCount++;
      }
      printf("\n");
    }
    // And the lines that are not yours. A guidebook with only your own
    // name in it is a diary.
    if (!t.lotNames.empty()) {
      printf("and, by the people who were also there:\n");
      for (const std::string& s : t.lotNames) printf("    %s\n", s.c_str());
      printf("\n");
    }
    printf("PEAK\t%.2f\tbest allround the player ever was\n", t.peakAllround);
    printf("GUIDEBOOK\t%d\tlives\t%d\tyours\t%d\ttheirs\n", lives,
           lineCount, static_cast<int>(t.lotNames.size()));
  }

  // **Where rapport ended up**, which is the question -- the first version
  // of this took a maximum across the whole career and therefore reported
  // 1.00 for anybody who ever had a good fortnight, thirty years after it
  // stopped being true. A high-water mark is not a state.
  for (const PartnerBond& b : player.bonds) {
    t.bestRapport = std::max(t.bestRapport, b.rapport);
  }

  t.gymBalanceEnd = player.gym.balance;
  t.gymMembersEnd = player.gym.members;
  for (int i = 1; i < kGymRegularCount; i++) {
    if (player.floor.stage[i] >= kArcStages) t.gymArcsLived++;
  }
  // **Read off the squad, not off the verb.** A hired coach's sessions run
  // in the night tick and never touch `RunYouthSession`, so counting the
  // verb's return reported a handed-over squad as doing nothing at all --
  // which is exactly the bug in the original this policy exists to catch.
  t.youthSessions = player.youth.sessions;
  t.youthGrads = static_cast<int>(player.youth.graduated.size());
  for (const Graduate& gone : player.youth.graduated) {
    t.youthOldest = std::max(t.youthOldest, gone.age);
  }
  // **Who kept winning it.** Two numbers, because they answer different
  // questions: how often the format's own people took it, and how many
  // different names ever went on the wall. A league where three people
  // hold every trophy is not a league.
  {
    const Leaning favours = FormatFavours(player.gymLeague.format);
    std::vector<std::string> seen;
    for (const LeagueChampion& won : player.gymLeague.champions) {
      bool known = false;
      for (const std::string& already : seen) known = known || already == won.name;
      if (!known) seen.push_back(won.name);
      for (int i = 1; i < kGymRegularCount; i++) {
        const GymRegularDef* def = GymRegularOf(static_cast<GymRegular>(i));
        if (def == nullptr || def->name != won.name) continue;
        if (def->lean == favours) t.glSuitedChamps++;
        break;
      }
    }
    t.glDistinctChamps = static_cast<int>(seen.size());
  }
  t.youthCraftEnd = player.youth.craft;


  // Who ended up knowing you. Same rule as rapport: the state, not the
  // high-water mark.
  for (const Local& p : player.locals.people) {
    t.bestKnown = std::max(t.bestKnown, p.known);
  }

  // And where the life outside it ended up. Same rule as rapport: the
  // state, not the high-water mark.
  for (int i = 1; i < kThreadCount; i++) {
    t.warmthEnd[i] = player.life.strands[i].warmth;
  }

  // One machine-readable line, always. Comparing two policies across
  // several seeds means parsing this output, and parsing the prose form
  // cost an afternoon to a sends count that wrapped onto the next line.
  // Header first, so a column can never be read off by eye against the
  // wrong name. Twice now a field has been appended to the values and not
  // to the format, and the table came out with silently empty columns.
  printf("HEAD\tseed\tpolicy\trest\tcash\tlow\tsends\tFAs\tdays\tburns"
         "\tgrade\tstew\tclosures\tshut\twork%%\tbroke\tstarved"
         "\tmissed\tgym\tboard\tinjuries\thurt\tpeakload\tphysio\tsponsor$"
         "\ttheirdays\tskinregen\tpower\tfingers\ttechnique\tendurance"
         "\thead\tallround\tshoewear\trivallost\trivalgens"
         "\traces\traceslost\traceswon"
         "\tcomps\tcompwins\tcomppods\trank\trankend\tteamyears"
         "\trounds\tfinals\tleaguenights\tleaguepbs\tleaguebest"
         "\tleaguewins"
         "\twcstarts\twcmissed\twcpods\twcwins\twctitles"
         "\tgames\tmedals"
         "\tdiagnoses\tshots\tsurgeries\tuntreated\tmedspend"
         "\tpremiums\tclaims\tjointrisk\tscars"
         "\tsickdays\ttimesill\tmeds\tprehab\ttoothdays\ttoothfixes"
         "\tworsttooth"
         "\tmoments\tbotched\tducked\tsackings\tbestcraft\ttrade"
         "\track\trackday\track$\tleads\tpieces\tranout\tleadfear"
         "\tquirks\tfirstquirk\thabitdays\thabits\tbecame"
         "\tapproach\tnobelayer\tbelaycap\theldburns\trapport"
         "\tevenings\tbusks\tbusked\tmet\tlost\tgrieving\tnag\tdeepest"
         "\twsomeone\twhome\twmusic\twbooks\twstove"
         "\tgreeted\tfirstgreet\tknown\tgsent\tgnamed\tgaway\tghurt"
         "\tgbroke\tgwon\tgspons\tsaid"
         "\tgymday\tgymdays\tgymbal\tgymmem\tgymlost"
         "\tgymspent\tgymwings\tgymhired\tgymraises\tgymquit"
         "\tgymfixed\tgymleft\tgymhands\tgymgone\tgymtopwage"
         "\tgymwalks\tgymcomps\tgymcohort\tgymarcs"
         "\tyouthday\tyouthsess\tyouthgrads\tyouthage\tyouthcraft"
         "\tyouthstep\tgymbids\tgymheld\tgymrounds\tgymbid$\tgymround$"
         "\tglnights\tglruns\tgl$\tglsuited\tglnames\n");
  printf("ROW\t%s\t%s\t%.1f\t%.0f\t%.0f\t%d\t%d\t%d\t%d\t%.1f\t%+.2f"
         "\t%d\t%d\t%.0f\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%.0f\t%d\t%.0f\t%d"
         "\t%.2f\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%.2f\t%.2f\t%d\t%d"
         "\t%d\t%d\t%d"
         "\t%d\t%d\t%d\t%.0f\t%.0f\t%d\t%d\t%d"
         "\t%d\t%d\t%.0f\t%d"
         "\t%d\t%d\t%d\t%d\t%d"
         "\t%d\t%d"
         "\t%d\t%d\t%d\t%d\t%.0f\t%.0f\t%.0f\t%.3f\t%d"
         "\t%d\t%d\t%d\t%d\t%d\t%d\t%d"
         "\t%d\t%d\t%d\t%d\t%.0f\t%s"
         "\t%s\t%d\t%.0f\t%d\t%d\t%d\t%.2f"
         "\t%d\t%d\t%d\t%.2f\t%s"
         "\t%.0f\t%d\t%d\t%d\t%.2f"
         "\t%d\t%d\t%.0f\t%d\t%d\t%d\t%d\t%.2f"
         "\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f"
         "\t%d\t%d\t%.2f\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s"
         "\t%d\t%d\t%.0f\t%.0f\t%d"
         "\t%.0f\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%.0f"
         "\t%d\t%d\t%d\t%d"
         "\t%d\t%d\t%d\t%d\t%.1f\t%d"
         "\t%d\t%d\t%d\t%.0f\t%.0f"
         "\t%d\t%d\t%.0f\t%d\t%d\n",
         seed.c_str(),
         takeTheSalary    ? "salary"
         : mindReputation ? "careful"
         : kept           ? "kept"
         : buysKit        ? "kitted"
         : savesUp        ? "saver"
         : dreams         ? "dreamer"
         : hoards         ? "hoarder"
         : stakesClaims   ? "stakeout"
         : projects       ? "projector"
         : comps          ? "comper"
         : leads          ? "trad"
         : clips          ? "sporty"
         : takesDeals     ? "sponsored"
                          : "greedy",
         restUntilSkin, player.cash, t.cashLow, t.sends, t.firstAscents,
         t.daysClimbed, t.burns, SkillToGrade(player.climber.skills.power),
         player.standing.with[static_cast<int>(Faction::Stewardship)],
         t.closures, t.closedDays, 100.0 * t.daysWorked / DAYS, t.brokeDays,
         t.starvedNights, t.missedWindows, t.gymDays, t.boardDays,
         t.injuries, t.hurtDays, t.peakLoad, t.physioSessions,
         t.sponsorPay, t.obligationDays, dd.skinRegenPerNight,
         player.climber.skills.power, player.climber.skills.fingers,
         player.climber.skills.technique, player.climber.skills.endurance,
         player.climber.skills.head,
         // `grade` above is SkillToGrade(power) and has been since this
         // probe was written, which under-reports a season: power is the
         // *slowest* growing skill, gaining +2.4 in a year where fingers
         // gain +7.6. Read off power a year of climbing looks worth a third
         // of a grade; across all five it is worth about half. `allround`
         // is the five-skill mean and is the honest headline. `grade` is
         // left alone so the older notes stay comparable to their own
         // numbers.
         SkillToGrade((player.climber.skills.power +
                       player.climber.skills.fingers +
                       player.climber.skills.technique +
                       player.climber.skills.endurance +
                       player.climber.skills.head) / 5.0),
         player.shoes.wear, t.linesLostToTheRival, t.rivalGenerations,
         t.racesStarted, t.racesLost, t.racesWon,
         t.compsEntered, t.compWins, t.compPodiums, t.rankingPeak,
         t.rankingEnd, t.teamSeasons, t.roundsClimbed, t.finalsReached,
         t.leagueNights, t.leaguePBs, player.league.best,
         player.league.blockWins, t.wcStarts, t.wcMissed, t.wcPodiums, t.wcWins,
         t.wcTitles, t.gamesEntered, t.medals,
         t.diagnoses, t.shots, t.surgeries, t.untreated, t.medicalSpend,
         t.premiums, t.claims, BodyRisk(player.medical, player.day),
         static_cast<int>(player.medical.scars.size()),
         t.sickDays, t.timesIll, t.medsTaken, t.prehabDays, t.toothDays,
         t.toothFixes, t.worstTooth,
         t.momentsTaken, t.momentsBotched, t.momentsDucked, t.sackings,
         [&] {
           double best = 0.0;
           for (int i = 1; i < kCraftCount; i++) {
             best = std::max(best, player.hand.skill[i]);
           }
           return best;
         }(),
         CraftName(YourTrade(player.hand)),
         // What a career of leading came to. `rackday` is the one that
         // matters most: a rack you can never afford is a discipline that
         // does not exist, however well it resolves in the harness.
         RackTierName(TierOf(player.rack)), t.gotTheRackOnDay, t.spentRack,
         t.leadsOnGear, t.piecesPlaced, t.ranItOut,
         t.leadsOnGear ? t.leadFearSum / t.leadsOnGear : 0.0,
         // And who the career turned into. `became` is the whole point:
         // a list of quirks in the order they landed is a biography, and
         // no other column in this table is one.
         t.quirksEarned, t.firstQuirkDay, t.daysWithAHabit,
         DAYS > 0 ? t.habitDaysSum / DAYS : 0.0,
         t.became.empty() ? "nobody in particular" : t.became.c_str(),
         // The walk in, and what the rope cost. `belaycap` is the one that
         // matters: days that ended because somebody had had enough rather
         // than because your skin had.
         t.approachHours, t.noBelayerDays, t.belayerCappedDays,
         t.burnsTheyHeld, t.bestRapport,
         // And the life outside it. `lost` is the column this pass exists
         // for: the day a career lost somebody through nothing but not
         // being there, or -1 if it never did.
         t.eveningsSpent, t.buskSessions, t.busked, t.hadSomebodyOnDay, t.lostSomebodyOnDay,
         t.grievingDays, t.nagDays, t.deepest,
         t.warmthEnd[static_cast<int>(Thread::Someone)],
         t.warmthEnd[static_cast<int>(Thread::Home)],
         t.warmthEnd[static_cast<int>(Thread::Music)],
         t.warmthEnd[static_cast<int>(Thread::Books)],
         t.warmthEnd[static_cast<int>(Thread::Cooking)],
         // And whether anybody ever greeted you by what you did. `gaway`
         // is the one nobody had to be told.
         t.greeted, t.firstGreetingDay, t.bestKnown,
         t.greetedByKind[static_cast<int>(Heard::Sent)],
         t.greetedByKind[static_cast<int>(Heard::Named)],
         t.greetedByKind[static_cast<int>(Heard::None)],
         t.greetedByKind[static_cast<int>(Heard::Hurt)],
         t.greetedByKind[static_cast<int>(Heard::Broke)],
         t.greetedByKind[static_cast<int>(Heard::Won)],
         t.greetedByKind[static_cast<int>(Heard::Sponsored)],
         t.lastSaid.empty() ? "nothing" : t.lastSaid.c_str(),
         t.boughtGymOnDay, t.gymDaysOwned, t.gymBalanceEnd, t.gymMembersEnd,
         t.gymLost, t.gymSpent, t.gymWings, t.gymHired, t.gymRaises,
         t.gymQuit, t.gymFixed, t.gymLeft, t.gymHandsOffDay, t.gymForeclosed,
         t.gymWorstWage, t.gymWalks, t.gymComps, t.gymCohortDay,
         t.gymArcsLived, t.youthDay, t.youthSessions, t.youthGrads,
         t.youthOldest, t.youthCraftEnd, t.youthSteppedUp, t.gymBids,
         t.gymSeasonsHeld, t.gymRoundsRun, t.gymBidSpend, t.gymRoundPay,
         t.glNights, t.glRuns, t.glTakings, t.glSuitedChamps,
         t.glDistinctChamps);

  if (quiet) {
    printf("%6.1f %8d %8d %8d %8d %9.1f %7.0f\n", restUntilSkin,
           t.daysClimbed, t.burns, t.sends, t.firstAscents,
           SkillToGrade(player.climber.skills.power), player.cash);
    return 0;
  }

  printf("\n=== after %d days ===\n", DAYS);
  printf("  tax: $%.0f handed over across the career, $%.0f owing this year\n",
         player.tax.paidLifetime, player.tax.taxable);
  {
    // `DEPTH-6`: who a career of climbing turned you into, and whether it
    // turned you into anybody at all.
    printf("  style:");
    for (int i = 0; i < kRouteTypeCount; i++) {
      printf(" %s %.0f%%%s", RouteTypeName(static_cast<RouteType>(i)),
             100.0 * player.style.xp[i] / std::max(1.0, StyleVolume(player.style)),
             StyleTierName(player.style, static_cast<RouteType>(i))[0]
                 ? "" : "");
    }
    printf("\n         %s\n", StyleLine(player.style).empty()
                                   ? "no style at all"
                                   : StyleLine(player.style).c_str());
    printf("         %s | stale %.1f at %s (freshness %.2f)\n",
           player.signature.name.empty()
               ? "no move with a name"
               : player.signature.name.c_str(),
           player.stale.days,
           player.stale.venue.empty() ? "nowhere" : player.stale.venue.c_str(),
           Freshness(player.stale));
    printf("         monotony peaked at %.2f; %d days plateaued of %d "
           "climbed\n", t.monotonyPeak, t.plateauDays, t.daysClimbed);
  }
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
  // Signed, and floored. Unsigned arithmetic on a size_t underflowed to
  // 18446744073709551614 the first time a long run claimed more lines than
  // the crag had open -- which is not impossible over thirty years, because
  // across generations the projects reset with the world and can be claimed
  // again.
  const int openLeft =
      std::max(0, static_cast<int>(OpenProjects(crag).size()) -
                      t.linesLostToTheLot - t.firstAscents);
  printf("  the Lot took %d lines; %d open lines remain\n",
         t.linesLostToTheLot, openLeft);

  const double bills = static_cast<double>(DAYS / dd.billsEveryDays) *
                       dd.billsAmount;
  printf("\n  the money, over %d days:\n", DAYS);
  printf("    earned  $%7.0f from %d shifts\n", t.earned, t.daysWorked);
  printf("    bills   $%7.0f\n", bills);
  printf("    food    $%7.0f (%d meals)\n", t.spentFood, t.mealsEaten);
  printf("    dog     $%7.0f (%d tins)\n", t.spentDog, t.dogMeals);
  printf("    shoes   $%7.0f (%d resoles, %d new pairs; %d days on dead "
         "rubber)\n", t.spentShoes, t.resoles, t.newPairs, t.deadRubberDays);
  printf("    fuel    $%7.0f\n", t.spentFuel);
  printf("\n  the body, over %d days:\n", DAYS);
  printf("    %d injuries, %d days hurt (%d of them climbed on), %d "
         "aggravations\n", t.injuries, t.hurtDays, t.climbedHurtDays,
         t.aggravations);
  printf("    load: %.0f average, %.0f peak (%s)\n", t.loadSum / DAYS,
         t.peakLoad, LoadText(player.climber).c_str());
  printf("    physio  $%7.0f (%d sessions)\n", t.spentPhysio,
         t.physioSessions);
  printf("\n  who pays you: %s\n", SponsorText(player.sponsor).c_str());
  printf("    %d deals signed, %d days under one -> $%.0f, and %d days that "
         "were theirs\n", t.dealsSigned, t.sponsoredDays, t.sponsorPay,
         t.obligationDays);
  printf("    kit     $%7.0f (%d pads, %s, %d days a member -> %d gym days, "
         "%d on the board)\n", t.spentKit, player.kit.pads,
         player.kit.hangboard ? "a board" : "no board", t.memberDays,
         t.gymDays, t.boardDays);
  printf("    van     $%7.0f (%d breakdowns, %d days stranded, %d bodged, "
         "%.0f hours under it)\n", t.spentVan, t.breakdowns, t.strandedDays,
         t.bodges, t.vanHoursLost);
  printf("\n  where you stand: %s\n",
         StandingText(player.standing).empty()
             ? "nobody has an opinion about you"
             : StandingText(player.standing).c_str());
  for (int i = 0; i < kFactionCount; i++) {
    printf("    %-22s %+.2f\n", FactionName(static_cast<Faction>(i)),
           player.standing.with[i]);
  }
  printf("    %d guidebook gigs, %d days of trail work -> %d closures, %d "
         "days shut out\n", t.photoGigs, t.trailGigs, t.closures,
         t.closedDays);

  printf("    -> %d windows arrived at after they had gone\n",
         t.missedWindows);
  printf("    -> %.0f%% of days worked to stay level; %d moves climbed\n",
         100.0 * t.daysWorked / DAYS, t.movesClimbed);

  printf("\n  average at the moment of pulling on: warmth %.2f, skin left "
         "%.1f, first-move odds %.0f%%\n",
         t.burns ? t.warmthSum / t.burns : 0.0,
         t.burns ? t.skinSum / t.burns : 0.0,
         t.oddsN ? 100.0 * t.oddsSum / t.oddsN : 0.0);
  // The logbook as it stands, which is the only way to see *why* a career
  // became what it became rather than just that it did.
  {
    const HabitDials hd;
    Logbook book = player.logbook;
    RememberTo(book, player.day, hd);
    const auto n = [&](Did w) { return book.count[static_cast<int>(w)]; };
    const double burns = std::max(1.0, n(Did::Burn));
    const double days = std::max(1.0, n(Did::DayOut));
    printf("  the logbook, at the end: %.0f burns and %.0f days out in the "
           "last season\n", n(Did::Burn), n(Did::DayOut));
    printf("    one line %.2f  new line %.2f  at limit %.2f  thin skin %.2f  "
           "| dawn %.2f  indoors %.2f  stopped early %.2f\n",
           n(Did::BurnOnOneLine) / burns, n(Did::LineTouched) / burns,
           n(Did::BurnAtYourLimit) / burns, n(Did::BurnOnThinSkin) / burns,
           n(Did::DawnStart) / days, n(Did::DayIndoors) / days,
           n(Did::StoppedEarly) / days);
    // Both forms, because the HUD draws the short one and the handover
    // draws the long one, and a probe that only ever measured the long one
    // would not notice the short one going empty.
    printf("    corner of the screen: [%s] [%s]\n",
           DoingLabel(book, player.day, hd).c_str(),
           AreLabel(player.quirks).c_str());
    printf("    %s\n", HowYouClimb(player.quirks, player.logbook, player.day).c_str());
  }
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
