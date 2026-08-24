// Living in the van.
//
// **Ported from the 2D source** (`LIFE-21` and `LIFE-22`), which is where
// the numbers and the one mechanical rule come from. Four meters that are
// not about climbing at all and are the whole texture of the thing the game
// is named after: **how long it has been since you washed**, and what is
// left in the jugs, the bottle and the battery.
//
// ## The one rule that gives grime teeth
//
// The original is explicit and narrow about it, in a comment on the
// function itself: *"the only mechanical bite: what a room full of people
// is willing to give you today."* Grime does not make you climb worse. It
// does not cost money. It multiplies **social gains** and nothing else --
// three quarters of them when you are ripe, half when you are feral.
//
// That lands on a seam this port only grew today. `WhoIsAround` decides who
// turns up, `PartnerBond::rapport` decides what they are worth to you, and
// `Local::known` decides whether the counter says anything -- three social
// gains, one multiplier, and none of them existed a week ago. **A system
// arriving to find the thing it needs already built is the argument for
// porting sim-first.**
//
// ## The three that are not grime
//
// Water, propane and power are consumables with a cap, a nightly draw and a
// price. They are deliberately dull: the interesting thing about running
// out of propane is that it happened forty minutes up a dirt road, and that
// is the day loop's business rather than this file's.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>

#include "DirtbagCore.h"

namespace dirtbag {

struct LivingDials {
  // --- grime ------------------------------------------------------------
  // 0 is showered this morning. 100 is the crux being finding a belayer.
  double grimeNight = 6.0;      // a night out here
  double grimeClimb = 5.0;      // ...and a day of chalk, sweat and rock dust
  double grimeRainRinse = 3.0;  // the sky washes you whether you asked or not

  // Where people start giving you room, and where they start giving you a
  // lot of it.
  double ripeAt = 60.0;
  double feralAt = 80.0;

  // **The only mechanical bite.** What your social gains are worth once you
  // are ripe, and once you are feral. Nothing else in the game reads grime.
  double ripeSocial = 0.75;
  double feralSocial = 0.5;

  // What washing actually buys. **You cannot get properly clean out of a
  // water jug**, which is why the floor exists and why the truck stop is
  // worth eight dollars.
  double jugWash = 32.0;
  double jugFloor = 15.0;
  double swimRinse = 50.0;
  double showerCost = 8.0;
  double showerHours = 1.0;
  double swimHours = 2.0;

  // --- what is in the van -----------------------------------------------
  double waterCap = 40.0;
  double waterNight = 3.0;      // drinking, dishes, the kettle
  double waterCook = 3.0;
  double waterWash = 8.0;       // a rag and a jug
  double waterJugCost = 2.0;    // a refill at the tap
  double waterJug = 20.0;       // ...and what a refill puts in

  double propaneCap = 100.0;
  double propaneCook = 8.0;
  double propaneHeat = 6.0;     // a cold night with the heater on
  double propaneBottleCost = 18.0;

};

// How you are living. Percentages except the water, which is litres,
// because that is how you actually think about a jug.
struct Living {
  double grime = 8.0;
  double water = 30.0;
  double propane = 70.0;
};

// **The house battery is deliberately not here.** `LIFE-22` has one --
// lights, the fan, the phone -- and its only real consumer in the original
// is `POWER_PHOTO`, the camera, which belongs to the media economy and is
// a recorded cut past 1.0 (`concepts/DIRTBAG.md` §4).
//
// So a battery ported now would drain six points a night, reach zero in a
// fortnight, have nothing to recharge it and nothing to spend it on. That
// is a meter that only goes down and does nothing, which is worse than no
// meter -- it looks like a system. It arrives with the camera or not at
// all. `check-dials.py` is what stopped it: `powerCap` was read by nothing
// and said so.

// What you smell like, in one word: fresh, lived-in, ripe, feral.
const char* GrimeWord(double grime, const LivingDials& dials = LivingDials{});

// **The one multiplier.** Applied to anything social -- who turns up, what
// rapport a day together is worth, whether a counter says anything. One
// when you are merely lived-in, because lived-in is the normal state of
// everybody in this valley.
double GrimeSocial(double grime, const LivingDials& dials = LivingDials{});

// A night in the van: dirtier, and down whatever the night drew. `cold` is
// whether the heater ran.
void LivingNight(Living& living, bool cold, bool rained,
                 const LivingDials& dials = LivingDials{});

// And a day of chalk, sweat, dirt and rock dust on top of it.
void ClimbedToday(Living& living, const LivingDials& dials = LivingDials{});

// A rag and a jug. Costs water, and **cannot get you properly clean** --
// false when there is not enough water or you are already at the floor.
bool WashInTheVan(Living& living, const LivingDials& dials = LivingDials{});

// The truck stop's stall: hot water, questionable tile, worth every dollar.
// Takes the money and the hour; this is the only thing that gets you to
// nothing.
bool ShowerAtTheTruckStop(Living& living, double& cash, double& hours,
                          const LivingDials& dials = LivingDials{});

// The lake. Free, takes an afternoon, and is a rinse rather than a scrub.
void SwimInTheLake(Living& living, double& hours,
                   const LivingDials& dials = LivingDials{});

// Fill the jugs, swap the bottle. Both refuse when you cannot pay.
bool FillTheJugs(Living& living, double& cash,
                 const LivingDials& dials = LivingDials{});
bool SwapTheBottle(Living& living, double& cash,
                   const LivingDials& dials = LivingDials{});

// Enough to cook a meal on? The burner needs both.
bool CanCook(const Living& living, const LivingDials& dials = LivingDials{});
void Cooked(Living& living, const LivingDials& dials = LivingDials{});

// **Quiet until it is worth saying**, like every other readout in this
// game: empty while you are merely lived-in with full jugs, and a sentence
// the day one of them runs out.
std::string LivingLine(const Living& living,
                       const LivingDials& dials = LivingDials{});

}  // namespace dirtbag
