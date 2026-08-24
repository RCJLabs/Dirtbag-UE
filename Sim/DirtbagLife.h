// A life outside it.
//
// The 2D game is a **life**-sim; the port, until now, is a climbing sim
// with bills. Phase 11 is the bucket of everything that fills the days a
// climbing career mostly consists of — and the thing that makes it a
// system rather than a menu of pastimes is that none of it waits for you.
//
// One rule, and everything here is downstream of it:
//
//   **A thread stays in your life because you keep giving it days.
//    Stop, and it goes cold. One of them leaves.**
//
// That is deliberately the opposite shape to `PartnerBond::rapport`, which
// keeps a floor from `everWas` because *somebody you climbed with for a
// season is somebody you climbed with for a season* and acquaintance is
// remembered. A thread has no floor, because the whole point of it is that
// it can be lost. What is remembered here is `depth` — how far in you got —
// and its only job is to decide how much the ending costs.
//
// **Nothing here touches send odds either**, for Phase 7's reason: a life
// outside climbing that made the moves easier would be the game telling you
// how to live. What these move is what a night's sleep is worth, what a
// meal costs, how steady you are above the last piece, and how much money
// arrives on a day it rained — seams that already exist and are already
// measured.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCore.h"

namespace dirtbag {

// The threads. `None` sits at zero so that "nothing ended tonight" has a
// name, the same way `Habit::None` and `Quirk::None` do — the array below
// carries a dead slot for it and that is a cheap price for a night tick
// that can answer honestly.
enum class Thread {
  None = 0,
  Someone,   // the one that can leave, and the reason this phase exists
  Home,      // a phone call you keep meaning to make
  Music,     // a guitar behind the driver's seat, and eventually a hat
  Books,
  Cooking,
  kThreadCount
};
constexpr int kThreadCount = static_cast<int>(Thread::kThreadCount);

struct LifeDials {
  // --- How long each one waits ------------------------------------------
  //
  // Days of nothing before it starts to cool. Slot 0 is `None` and is dead
  // weight; every row below reads in enum order after it.
  //
  //   Someone   under a week. Somebody you are seeing notices a fortnight
  //             before your mother does, and that asymmetry is the whole
  //             character of the two threads.
  //   Home      a month. Family is patient, which is what makes neglecting
  //             it so easy and so cheap-feeling right up until it is not.
  //   Music     three weeks, because hands forget.
  //   Books     a month; a paperback waits.
  //   Cooking   a fortnight — a stove you stop using becomes a box you
  //             stop unpacking.
  double patienceDays[kThreadCount] = {0.0, 6.0, 30.0, 21.0, 30.0, 14.0};

  // ...and how fast it goes once it has started, per day. Someone is set so
  // that a full-warmth relationship survives about three and a half weeks
  // of you being at the crag and nothing else: six days of grace, then
  // eighteen of cooling. Long enough to be a slide you could have stopped,
  // short enough that a season away ends it.
  double coolsPerDay[kThreadCount] = {0.0, 0.055, 0.012, 0.020, 0.016, 0.030};

  // **What one go at it asks for, in hours.** Measured, and the reason
  // this dial exists at all: with everything priced at a two-hour evening,
  // a thirty-year career spent 1,138 evenings and **lost nothing, ever**.
  // The hours after the crag are dead time in this game -- the window has
  // gone, the shift is done -- so a thread that costs an evening costs
  // nothing, and a mechanic whose whole claim is "this can be lost through
  // neglect alone" was unlosable.
  //
  //   Someone   **a day.** You drive over. That is the day, and it is the
  //             only entry here that competes with a session, which is
  //             exactly why it is the only one that can leave you.
  //   Home      half an hour on the phone, standing by the van.
  //   Music     an afternoon with the hat out; near enough a shift.
  //   Books     an evening.
  //   Cooking   an hour, and you have to eat anyway.
  double asksForHours[kThreadCount] = {0.0, 6.0, 0.5, 3.0, 2.0, 1.0};

  // What an hour of it buys back. Set against the row above so that one
  // full go is worth roughly: Someone all of it, Home most of it, the rest
  // about half -- which is what makes the cheap threads need keeping up
  // and the expensive one need deciding about.
  double warmsPerHour[kThreadCount] = {0.0, 0.167, 1.20, 0.167, 0.20, 0.50};

  // And how far in you get, per day given, scaled by how well it is going —
  // you do not deepen a thing you are doing badly. At 0.010 a romance is
  // all the way in after about a hundred days of actually turning up, which
  // is the right order of magnitude for a thing whose ending is supposed to
  // cost a season.
  double deepensPerDay[kThreadCount] = {0.0, 0.010, 0.004, 0.012, 0.008, 0.010};

  // --- What they are worth ----------------------------------------------
  //
  // Each in the units the system it touches already uses, and all of them
  // scaled by warmth — a thread you are neglecting pays you nothing while
  // it cools, which is the point.

  // Added to the psyche baseline `SleepToNextDay` drifts you toward. Worth
  // roughly what an hour with the shrink buys, and unlike the shrink it
  // does not stay bought.
  double someonePsyche = 0.10;

  // Boldness, on the same 0..1 axis as the temperament's and the habits'.
  // **Home rather than Someone on purpose**: what a phone call does is
  // remind you there is a version of you that is not this, and that is the
  // thing that makes a person steady above the last piece. Being in love is
  // not famously steadying.
  double homeNerve = 0.08;

  // Busking. What an hour in front of the co-op pays at full warmth with
  // nobody knowing who you are, and what a street reputation adds on top.
  // The second number is the larger one because that is the shape of the
  // thing: the first month is coins.
  //
  // **Sized against the belay desk, after measurement.** At $9 and $26 a
  // mastered busker made $35 an hour against a shift's $15, cost no
  // energy, and turned in **$63,772 over ten years** in a career that
  // ended with $1,709 in the bank -- the best-paid work in the game, for a
  // hobby. At $4 and $11 the first month really is coins and a decade of
  // practice brings you level with the desk, which is the right ceiling
  // for a thing you do because you like it.
  double buskPerHour = 4.0;
  double buskDepthPay = 11.0;

  // What an hour of rest is worth when you are actually resting rather than
  // sitting in a camp chair going over the beta. A multiplier on
  // `DayDials::restEnergyPerHour`.
  double booksRest = 1.45;

  // A stove and the will to use it: what a meal costs, and how far it goes.
  double cookingCost = 0.62;
  double cookingFeeds = 1.30;

  // --- And what it costs when one of them goes --------------------------

  // Below this depth it was never a thread and its ending is not an event.
  // Without a floor, a fortnight of seeing somebody and then not would
  // register as a loss, and a game that grieves everything grieves nothing.
  double enoughToLose = 0.25;

  // How long the ending lasts, at full depth, and what it takes off the
  // psyche baseline while it does. Six weeks and a fifth of a psyche point
  // is a bad season rather than a broken career — this is a life-sim, not a
  // punishment, and the cost that matters is the one you can see coming.
  double grievesForDays = 45.0;
  double griefPsyche = 0.22;

  // And how long before there is anybody else. Not a mechanic so much as a
  // refusal to let the answer to a breakup be "start another one on
  // Thursday".
  double sparkCooldownDays = 30.0;
};

// One thread's state. What it is is the index; a `what` field would be a
// fact the array already knows and a field a save has to carry.
struct Strand {
  bool going = false;

  // How alive it is, 0..1. No floor, by design — see the file header.
  double warmth = 0.0;

  // How far in you got, 0..1. Never falls. This is the memory, and its
  // only job is to price the ending.
  double depth = 0.0;

  // The last day you gave it anything, and how many days you have. Days
  // rather than hours because the question every readout asks is *when did
  // you last*, and hours are how you answer it, not what it means.
  int lastGivenDay = 0;
  int daysGiven = 0;

  // It ended, as opposed to never having started. The two are the same
  // `going == false` and they are not remotely the same thing.
  bool ended = false;
  int endedDay = 0;
};

struct Life {
  Strand strands[kThreadCount];

  // What an ending is still costing, and how heavily. Counted down at
  // night, with everything else in this game that counts down at night.
  double grieving = 0.0;
  double griefWeight = 0.0;
};

// --- Asking -------------------------------------------------------------

bool Going(const Life& life, Thread t);
double WarmthOf(const Life& life, Thread t);
double DepthOf(const Life& life, Thread t);

// Days since you last gave it anything. Large and meaningless for a thread
// you never took up, which is why every caller checks `Going` first.
//
// unwired-ok: the engine reads the sentence, not the number. `HowItIsGoing`
// and `LifeLabel` are both built on this and both are exposed; a second
// door handing a widget the raw count would be a way to build a readout
// that disagrees with the one the game already says out loud.
int DaysSince(const Life& life, Thread t, int today);

// What one go at it takes, in hours. **The sim owns this number**: a spot
// or a widget that could set its own would be a second copy of the rule,
// and one go at a thing being a different length in two places is how the
// evening stopped costing anything the first time.
double AsksFor(Thread t, const LifeDials& dials = LifeDials{});

// Can this be started today? Everything but Someone can always be picked
// back up; Someone has a cooldown after an ending, and it is the only
// place in this file where the game says no.
bool CanTakeUp(const Life& life, Thread t, int today,
               const LifeDials& dials = LifeDials{});

// --- Doing --------------------------------------------------------------

// Take it up. Starts warm rather than cold: nobody begins a thing they are
// already neglecting.
bool TakeUp(Life& life, Thread t, int today, const LifeDials& dials = LifeDials{});

// An evening, an afternoon, twenty minutes on the phone. Hours are the
// currency because hours are what a day in this game is made of, and every
// hour spent here is an hour not spent on the wall — which is the trade the
// whole phase is about. False if the thread is not in your life.
bool GiveItTime(Life& life, Thread t, double hours, int today,
                const LifeDials& dials = LifeDials{});

// The night tick. Cools what you ignored, deepens what you kept, counts the
// grief down, and returns the thread that ended tonight — or `None`, which
// is almost every night. Same contract as `HabitsDay`, and for the same
// reason: a line the player reads once must not depend on a caller
// remembering to ask twice.
Thread LifeNight(Life& life, int today, const LifeDials& dials = LifeDials{});

// --- What it does -------------------------------------------------------

// Added to the psyche baseline. Can be negative, and the day it is, you
// know exactly why.
double LifePsyche(const Life& life, const LifeDials& dials = LifeDials{});

// Boldness, added to the same axis the temperament and the habits use.
double LifeNerve(const Life& life, const LifeDials& dials = LifeDials{});

// Multipliers on what a meal costs and how far it goes.
double LifeMealCost(const Life& life, const LifeDials& dials = LifeDials{});
double LifeMealHunger(const Life& life, const LifeDials& dials = LifeDials{});

// Multiplier on what an hour of sitting still buys back.
double LifeRestRate(const Life& life, const LifeDials& dials = LifeDials{});

// What an hour with the hat out is worth. Zero without the thread, which is
// what stops this being a job anybody can take.
double BuskingPay(const Life& life, double hours,
                  const LifeDials& dials = LifeDials{});

// --- Saying it ----------------------------------------------------------

const char* ThreadName(Thread t);

// What it is, in one line. Used at the point you take it up.
const char* ThreadLine(Thread t);

// How that one is going, always answered while it is in your life or after
// it ended — the readout you open on purpose. Empty only for a thread you
// never took up, because "you have no hobbies" is not a status.
std::string HowItIsGoing(const Life& life, Thread t, int today,
                         const LifeDials& dials = LifeDials{});

// The line on the night it ends.
std::string ItEnded(Thread t);

// **Terse, for a corner of a screen, and quiet unless something is wrong.**
// The same rule the injury line and the habit label follow: a readout that
// prints "everything is fine" every night for a season teaches you to stop
// reading the one that will eventually say otherwise. This speaks when a
// thread is cooling, and it speaks while you are grieving, and the rest of
// the time it says nothing at all.
std::string LifeLabel(const Life& life, int today,
                      const LifeDials& dials = LifeDials{});

}  // namespace dirtbag
