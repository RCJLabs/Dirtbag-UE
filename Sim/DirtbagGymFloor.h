// The people in the building.
//
// `GYM-1` and pass two made the gym a business: pricing, staffing, a town
// pushing back, a year with a shape, and a bank that takes it away. What
// none of that says is **who the business is for**. Members were a number
// on a dashboard, and the source's own comment on `GYM-2` is the whole
// argument in one line: *"The business sim is untouched -- this layer is
// who the business is FOR."*
//
// `GYM-2` and `GYM-5`, ported from the 2D source.
//
// ## Four people, then three more, and which three is your fault
//
// Wave one is four regulars with three-stage arcs that advance when you
// walk the floor: the man whose daughter bought him a birthday membership,
// the eleven-year-old who is already better than your strongest member, the
// nurse who climbs alone at six, and the one who campuses everything.
//
// `GYM-5` exists because those four arcs end and then the floor goes quiet
// -- *"every walk after that fires the same summary line forever, and the
// four people you spent a career on become a row of stars."* Two fixes, and
// the first is the interesting one:
//
// **Wave two is keyed to the set mix in force when it arrives, and then it
// is sticky.** Which people a gym collects is the most honest consequence a
// set mix has -- a wall of beginner slabs and a wall of hard boards do not
// fill with the same room. And they do not evaporate because you re-taped
// the place six months later. **This is the only place in the port where a
// free, reversible identity lever writes something permanent**, which is
// exactly what makes it worth having.
//
// The second fix is that a finished arc is not a finished person: everybody
// whose story has been told keeps talking, in ambient lines picked off the
// day, so a week of walks reads differently with no state kept.
//
// ## What it is not
//
// Not a second economy. The floor walk costs an hour and returns two
// members and some psyche; comp night costs $150 and an evening and returns
// entry fees, signups and standing. Both are small on purpose. What they
// return that matters is **the reason to go and stand in the building you
// bought**, which the P&L engine could never give you -- it runs whether
// you are there or not, and once you go hands-off it runs better.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagGym.h"
#include "DirtbagRng.h"

namespace dirtbag {

// Everybody who could ever be a regular here. Wave one is the four
// everybody gets; the other nine are three cohorts of three, and a gym
// grows exactly one of them.
enum class GymRegular {
  None = 0,
  Dale, Piper, June, Bruno,
  Marisol, Ade, Horace,
  Nell, Tobias, Esperanza,
  Kestrel, Dom, Rafferty,
  kGymRegularCount
};
constexpr int kGymRegularCount = static_cast<int>(GymRegular::kGymRegularCount);

// **Three stages, and then they are still here.** The arc ends; the person
// does not.
constexpr int kArcStages = 3;

struct GymRegularDef {
  GymRegular who = GymRegular::None;
  const char* name = "";
  // One true thing about them, for a list that has to fit on a line.
  const char* tag = "";
  int wave = 1;
  // Which cohort they belong to. Meaningless for wave one, and read only
  // when `wave` is 2.
  GymSetMix cohort = GymSetMix::AllComers;
  const char* stages[kArcStages] = {"", "", ""};
  // Ambient, once the arc is lived. Two each, picked off the day.
  const char* after[2] = {"", ""};
  // And what they do on comp night. A table rather than the source's
  // original if/else chain, which **ended on Bruno** and so printed his
  // line for any wave-two star.
  const char* compStar = "";
};

const GymRegularDef* GymRegularOf(GymRegular who);

struct FloorDials {
  // --- walking the floor (GYM-2) ----------------------------------------
  // An hour among your members, once a day.
  double walkHours = 1.0;

  // Living out a stage brings word of mouth. Small, because the drift model
  // is what actually decides the membership -- this is a nudge on top of
  // it, not a second lever.
  double memberBump = 2.0;

  // What a walk does to your head, in this port's 0..1 psyche. Three
  // different numbers because they are three different days: a stage lived
  // out, the week a whole cohort walks in, and a floor where every story
  // has already been told.
  double walkPsyche = 0.02;
  double waveTwoPsyche = 0.04;
  double ambientPsyche = 0.03;

  // --- comp night (GYM-2) -----------------------------------------------
  // Pizza, prizes, extra staff hours.
  double compCost = 150.0;

  // Below this the room is too empty to bother, which makes comp night
  // something the business has to have earned rather than a button.
  double compMinMembers = 20.0;

  // Days between them. **Scarcity is the draw** -- an occasion, not a
  // crank.
  int compCooldown = 10;
  double compHours = 3.0;

  // Walk-ins who sign up after a good night.
  double compSignupsMin = 4.0;
  double compSignupsMax = 9.0;

  // Roughly this fraction of the room enters, at this much a head. The
  // takings are therefore a function of the membership you built, which is
  // the point: comp night pays out of the business, not out of nowhere.
  double compTurnout = 0.6;
  double compEntry = 8.0;

  // Hosting reads as community-building in the scene, in scene points
  // before the /100 the caller does.
  double compStanding = 4.0;
  double compPsyche = 0.05;
};

// The floor's memory. **Lives on the player and not on the `Gym`**, the
// same way the source keeps it on the state rather than on `ownGym` -- and
// for a reason worth writing down: these are people in a town, not fixtures
// in a building. If the bank takes the lease, Dale does not stop existing.
struct GymFloor {
  // 0..3 per regular. Three is a life lived out.
  int stage[kGymRegularCount] = {0};

  int lastWalkDay = -1;
  int lastCompDay = -99;

  // **Sticky.** Set once, the day wave one is fully lived out, from the mix
  // on the walls that day.
  bool waveTwoArrived = false;
  GymSetMix waveTwo = GymSetMix::AllComers;
};

// Wave one, plus whichever wave two this gym grew. Empty of wave two until
// it arrives.
std::vector<GymRegular> TheCast(const GymFloor& floor);

// The next member due a moment: **lowest stage first, cast order breaks
// ties**, so it is fully deterministic and a player who walks every day
// sees the room in a fixed, sensible order. `None` when every arc is lived.
GymRegular NextMomentDue(const GymFloor& floor);

// Is every wave-one arc at three? The trigger for the cohort.
bool WaveOneIsLivedOut(const GymFloor& floor);

// What they are up to today, once their story is told. Off the day, so a
// week of walks reads differently with nothing kept.
std::string AmbientLine(GymRegular who, int day);

// **The setter's line of the week.** A pure function of the week number --
// no state, no roll -- so every save sees the same name the same week and
// it changes on Mondays. Empty when nobody is setting.
std::string SetterLineOfTheWeek(const Gym& gym, int day);

// --- walking the floor ----------------------------------------------------

struct FloorWalk {
  bool walked = false;
  std::string said;
  double members = 0.0;
  double psyche = 0.0;
  // Whose moment it was, when it was somebody's.
  GymRegular who = GymRegular::None;
  bool cohortArrived = false;
};

// An hour among your members. Advances the next arc due, or greets the
// cohort, or -- once everybody's story is told -- tells you what one of
// them is doing today. Once a day; `walked` is false if you already have,
// or there is no gym.
FloorWalk WalkTheFloor(GymFloor& floor, Gym& gym, int day,
                       const FloorDials& dials = FloorDials{});

// --- comp night -----------------------------------------------------------

struct CompNight {
  bool held = false;
  std::string said;
  // **The takings are handed back rather than added**, because they are
  // income and this game pays its debts first -- the caller runs them
  // through `Pay`. The cost is taken up front against `cash`, the way every
  // other purchase in the gym module is, so a comp night cannot be thrown
  // on credit.
  double takings = 0.0;
  double cost = 0.0;
  double members = 0.0;
  double standing = 0.0;
  double psyche = 0.0;
};

// Throw one. Takes the cost up front and hands back the takings, so a night
// that loses money loses it visibly.
CompNight HostACompNight(GymFloor& floor, Gym& gym, double& cash,
                         const Rng& worldRng, int day,
                         const FloorDials& dials = FloorDials{});

// Why not, in one line, or empty when you can. **The refusal carries the
// reason** -- a greyed-out button that will not say why is the thing this
// port keeps deciding it does not want.
std::string WhyNotACompNight(const GymFloor& floor, const Gym& gym,
                             double cash, int day,
                             const FloorDials& dials = FloorDials{});

// One line for the counter: who is due, and what is on the walls this week.
std::string FloorLine(const GymFloor& floor, const Gym& gym, int day);

}  // namespace dirtbag
