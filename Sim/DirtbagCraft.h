#pragma once

// Work as a second career you are also having.
//
// `DirtbagJobs` ports what §4 said to port — an odd-jobs board and a
// salaried trap — and both are built, wired and measured. What it did not
// port is that in the 2D game **every job has a craft skill**, and that
// changes what work *is*. A lever you pull for money is a lever. A trade
// you are getting better at is a life.
//
// Three things follow from that, and they are this phase's three gates:
//
// **The job you keep changes who your climber is.** Every craft pays back
// into the body or the head — a setter spends all day reading movement, a
// coach spends all day talking somebody through being frightened, trail
// work is six hours of carrying rock uphill. None of it is as good as
// climbing and all of it is better than nothing, which is exactly what a
// job you keep for ten years does to a person.
//
// **A shift has a decision in it.** Something comes up — a hold spins, a
// kid freezes at the top, a delivery is going to be late — and there are
// two ways to handle it. The right one is harder and needs the craft you
// have actually built. **Reaching past your craft is how you botch it**,
// and botching it is how the third gate happens.
//
// **Getting fired outlives the job.** Standing is per trade, and when it
// falls far enough that employer stops offering you work — the gig comes
// off your board and stays off. Losing the gym is losing the best-paid
// regular work in the valley, and no amount of climbing gets it back.
//
// ## What is deliberately simpler than the 2D game
//
// The 2D game tracks employer standing **per building**. This tracks it
// **per trade**, because a trade here has one employer behind it: the gym
// sets, the park does trail, the guidebook buys photos. Where two gigs
// share a trade (furniture, firewood and night shelves are all Labour)
// they share the reputation too, which is a simplification and is written
// down rather than hidden.
//
// Engine-free like everything in Sim/.

#include <string>
#include <vector>

#include "DirtbagCharacter.h"
#include "DirtbagCore.h"
#include "DirtbagRng.h"

namespace dirtbag {

// The trades. Ten, and every one of them has a gig on the board behind it
// — a craft you cannot practise is a stat.
enum class Craft {
  None = 0,     // flyering. Some work is just work.
  Setting,      // the gym
  Coaching,     // kids' birthdays, and later people who mean it
  Counter,      // the gear shop, the diner
  Labour,       // furniture, firewood, night shelves
  Trail,        // the park
  Camera,       // the guidebook
  Courier,      // a van and a deadline
  Rescue,       // the callout nobody wants
  Bar,          // the one that pays in tips and finishes at two
  Office,       // the nine-to-five
};
constexpr int kCraftCount = 11;
const char* CraftName(Craft c);

// What trade a gig belongs to. Matched on the gig's own name, because the
// board is data and the name is the only stable thing about an entry.
Craft CraftForGig(const std::string& gigName);

struct CraftDials {
  // --- getting better at it -------------------------------------------
  // Per hour worked, with the same diminishing returns climbing has: the
  // first fifty come fast and the last ten crawl. A season of Saturdays
  // makes you competent; ten years of them makes you as good at it as
  // anybody.
  //
  // **A craft is masterable and climbing is not**, which is a real
  // difference and not an oversight: you can be a genuinely great route
  // setter, and nobody is ever finished being a climber.
  double gainPerHour = 0.55;
  // **Named apart from `PartnerDials::ceiling` on purpose**, same as every
  // other shadowed dial this project has found: the checker reads two dials
  // of one name as one number that has drifted, and the deliberate
  // difference is indistinguishable from a bug until the name says so.
  double craftCeiling = 100.0;

  // What it pays. **The economic reason to specialise**: a setter who has
  // been doing it for years is worth more than one who has not, and the
  // spread is wide enough to notice and narrow enough that it never beats
  // just taking the better gig.
  double payAtNothing = 0.80;
  double payAtMastery = 1.55;

  // --- what it does to you --------------------------------------------
  // What an hour of the trade teaches your climbing, per hour, in skill
  // points. **Tiny on purpose.** Ten years of setting is worth a couple of
  // grades of technique; ten years of climbing is worth far more. Work
  // that trained you as well as climbing did would make the trap not a
  // trap.
  double trainsPerHour = 0.020;

  // --- the moment ------------------------------------------------------
  // How often something comes up on a shift that has to be decided.
  double momentChance = 0.35;
  // The craft you need to pull off the harder answer cleanly, and how
  // steeply it falls away below that. At the line it is a coin flip you
  // usually win; well under it you are mostly making things worse.
  double botchBelow = 0.9;
  // What the hard way is worth when it comes off, as a multiplier on the
  // shift's pay, and what botching it takes off your standing.
  double momentBonus = 0.35;
  double momentStanding = 0.06;
  double botchStanding = 0.14;
  // Doing it the easy way is never wrong and never gets you anywhere. It
  // has to be a real option or the "decision" is a skill check.
  double easyStanding = 0.01;

  // --- and the sack ----------------------------------------------------
  // Below this they stop offering you work, and **it does not come back**
  // on its own. Standing drifts up slowly with every shift you do not
  // botch, so the way back is to be good at it for a long time -- but the
  // gig is off your board while you are, which is the point.
  double sackAt = -0.55;
  // Where they will take you back, once you are somehow above it again.
  // Higher than `sackAt`, so being sacked is a state and not a flicker.
  double rehireAt = -0.20;

  // --- the trade you are ------------------------------------------------
  // How much of a trade it takes before the game will call you one. Below
  // this you have done some shifts; above it, it is what you are.
  double tradeAt = 45.0;
};

// Everything the trades know about you.
struct Craftsman {
  double skill[kCraftCount] = {0};
  // Per trade, -1..1. What the people who hire you for this think.
  double standing[kCraftCount] = {0};
  int shifts[kCraftCount] = {0};
  // Sacked, and it does not come back on its own.
  bool sacked[kCraftCount] = {false};

  int momentsTaken = 0;      // times you went the hard way
  int momentsBotched = 0;
  int momentsDucked = 0;     // times you did not
  int sackings = 0;
};

// An hour of the trade. Grows the craft, drifts standing up a little, and
// hands back what it taught your climbing.
struct WorkedShift {
  double skillGain = 0.0;       // in the climbing skill below
  Skill teaches = Skill::Technique;
};
WorkedShift WorkTheTrade(Craftsman& hand, Craft craft, double hours,
                         const CraftDials& dials = CraftDials{});

// What the trade teaches, and it is never nothing except for `None`.
Skill CraftTeaches(Craft craft);

// What this shift pays you, as a multiplier. One for somebody with no
// trade at all is deliberately *not* the case -- a beginner is worth less.
double CraftPay(const Craftsman& hand, Craft craft,
                const CraftDials& dials = CraftDials{});

// Will they have you? False once you have been sacked from that trade.
bool WillTheyHireYou(const Craftsman& hand, Craft craft,
                     const CraftDials& dials = CraftDials{});

// --- the moment ---------------------------------------------------------

// Something that came up on shift. **Two ways to handle it**, and the
// right one is harder and needs the craft you have actually built.
struct ShiftMoment {
  bool happened = false;
  Craft craft = Craft::None;
  const char* what = "";      // what came up
  const char* theHardWay = "";
  const char* theEasyWay = "";
  double needs = 0.0;         // the craft that pulls it off cleanly
};

// Does anything come up on this shift? Deterministic on the day and the
// trade, so a reload does not reroll it.
ShiftMoment MomentOnShift(Craft craft, const Rng& worldRng, int day,
                          const CraftDials& dials = CraftDials{});

struct MomentOutcome {
  bool botched = false;
  double payMultiplier = 1.0;
  double standingShift = 0.0;
  bool sacked = false;
  std::string news;
};

// Decide it. `theHardWay` false is always available, always safe, and
// never gets you anywhere.
MomentOutcome DecideTheMoment(Craftsman& hand, const ShiftMoment& moment,
                              bool theHardWay, const Rng& worldRng, int day,
                              const CraftDials& dials = CraftDials{});

// --- who that makes you -------------------------------------------------

// The trade you have most of, once you have enough of it to be one.
// `None` until then, which is most careers.
Craft YourTrade(const Craftsman& hand, const CraftDials& dials = CraftDials{});

// "a setter who climbs" / "a climber who sets" -- **the work identity**,
// and which way round it goes depends on how much of each you have.
std::string TradeText(const Craftsman& hand, double climbingGrade,
                      const CraftDials& dials = CraftDials{});

// What the trades have to say about you. Empty before you have any.
std::string CraftText(const Craftsman& hand,
                      const CraftDials& dials = CraftDials{});
}  // namespace dirtbag
