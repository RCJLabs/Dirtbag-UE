// The people who remember you between visits.
//
// Phase 11's third gate, and the only part of it that is not content:
// *somebody in the world greets you by what you did last time.* Everything
// else in the bucket — fishing, the garden, the caravan — is another row on
// `Thread`. This is a different shape: a person who holds exactly one fact
// about you and hands it back the next time you are in front of them.
//
// Two rules, and the file is downstream of both:
//
//   **They hold one thing, and it is the loudest thing they have heard
//    since they last saw you.**
//   **They say it once. Then they go back to nodding.**
//
// The first is what keeps this from being a feed. A shopkeeper who lists
// your season at you is a changelog with a face; one who says *"heard you
// got the Prow"* and nothing else is a person. The second is what keeps it
// from being wallpaper: a line that repeats every visit stops being a
// greeting inside a week, which is the same rule the injury line, the habit
// label and the life label are all built on.
//
// **Familiarity keeps a floor**, unlike `Thread::warmth` and like
// `PartnerBond::rapport`. Somebody who has sold you rubber for five years
// still knows you after a winter away — you stop being current with them,
// you do not become a stranger. That is the third time this project has
// needed *a value that ages needs a memory, not just a slope*, and the
// first time it has been built that way rather than found that way.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagRng.h"
#include "DirtbagTown.h"

namespace dirtbag {

// What somebody heard about you.
//
// **The order is the loudness**, quietest first, and that is load-bearing:
// a new memory replaces the held one only if it compares greater, so the
// enum ordering *is* the rule about what gets talked about instead of a
// second table that could disagree with it. Putting a fact in the right
// place in this list is the whole of adding one.
enum class Heard {
  None = 0,
  Away,       // nobody tells them this; it is what is left when it has been ages
  Broke,      // they watched you not afford it, and only they saw it
  Hurt,
  Sent,
  Named,      // you put a line up and put a name on it
  Won,
  Sponsored,
  kHeardCount
};
constexpr int kHeardCount = static_cast<int>(Heard::kHeardCount);

struct LocalDials {
  // How fast somebody starts knowing you, per visit. About a dozen visits
  // to be a regular — a season of eating badly in the same place.
  double knownPerVisit = 0.08;

  // ...and how fast that fades when you stop turning up. Slow: eight
  // months from knowing you well to not, if it had no floor.
  double knownDecayPerDay = 0.004;

  // Which it does. Same shape as `PartnerDials::rapportKeeps`, and for the
  // same reason — see the file header. You stop being current, you do not
  // become a stranger.
  double knownKeeps = 0.45;

  // Below this you are a customer, and a customer does not get told what
  // they have heard about you. This is the gate that makes the greeting
  // feel earned rather than issued: the diner does not comment on your
  // season the first week you are in town.
  double nodsAt = 0.25;

  // What being a regular is worth off the bill, at full familiarity.
  // Deliberately small — this is a relationship, not a loyalty card, and
  // the money is not the point of it.
  double discountAt = 0.12;

  // How long since they last saw you before that is itself the thing they
  // say. A season.
  double awayDays = 90.0;
};

// One person, at one counter.
struct Local {
  std::string name;

  // Which counter. One per service the town offers, so "the person at the
  // gear shop" is a fact about the town rather than a roster to maintain.
  Service where = Service::Meal;

  // How well they know you now, and the best it ever was. `everKnew` never
  // falls and is what the floor is a fraction of.
  double known = 0.0;
  double everKnew = 0.0;

  // The one thing they are holding, and its noun — a route name, a joint,
  // a comp. Empty `about` is legal and simply leaves the line general.
  Heard holds = Heard::None;
  std::string about;

  int lastSeen = 0;
};

struct Locals {
  std::vector<Local> people;

  // **What the town has already been told.** On the town rather than on
  // the career, because "has this got around yet" is a fact about the
  // town: a career that wins a season has won a season whether or not
  // anybody has mentioned it at the diner.
  //
  // Two counters rather than a general event feed, because these are the
  // only two facts in the game with nowhere else to be noticed from --
  // a send passes through `ApplyAttemptToDay`, a first ascent through
  // `ClaimFirstAscent`, an injury through the night's roll, and a bill you
  // could not cover through the person standing there. A season title and
  // a signature are assembled by hand at two call sites each, so the night
  // tick is the only place both consumers pass through.
  int knownTitles = 0;
  int knownTier = 0;
};

// The town's counters, one person each, deterministic from the world seed.
// Rolled rather than authored so a new venue gets a face for free, and off
// the world stream so two careers in the same valley meet the same people.
Locals TheLocals(const Town& town, const Rng& worldRng);

// Who is behind that counter, or null if the town has nobody for it.
Local* At(Locals& locals, Service where);
const Local* At(const Locals& locals, Service where);

// **The town hears something.** Reaches everybody, because it is a valley
// and that is what valleys are like — but only the ones who know you will
// ever bring it up, which is `nodsAt` doing the work.
//
// Quieter than what somebody is already holding is not news; it is dropped
// rather than queued. A person is not a mailbox.
void Tell(Locals& locals, Heard what, const std::string& about, int day);

// ...and the things only one person saw. The bill you could not cover is
// between you and whoever was standing there.
void TellOne(Locals& locals, Service where, Heard what,
             const std::string& about, int day);

// What they say when you walk in, or empty — which is most visits, and is
// the point. `Away` is derived here rather than told, because it is the one
// memory nobody has to give them.
std::string WhatTheySay(const Local& who, int today,
                        const LocalDials& dials = LocalDials{});

// You were in front of them. Bumps familiarity and **spends whatever they
// actually said** — call it once per visit, or they will say the same thing
// every time you buy a burrito and stop being a person.
//
// **It spends only what was said.** The first version cleared the memory on
// every visit, including the ones where they said nothing because you were
// still a stranger to them — so a career's first two hundred days of sends
// were silently eaten by the burritos that ate them, and measured over
// thirty years the thing a regular said was *"thought you'd moved on"*
// thirty-seven times out of forty. What they are holding survives until
// somebody hears it.
// `smell` -- the third and last of the social gains grime discounts. They
// still see you and the memory is still spent; you just get less of them
// for it, which is exactly what being ripe at a counter is like.
void Seen(Local& who, int today, double smell = 1.0,
          const LocalDials& dials = LocalDials{});

// The nightly fade, with everything else in this game that counts down at
// night.
void LocalsDay(Locals& locals, int today,
                 const LocalDials& dials = LocalDials{});

// A multiplier on what that counter charges. 1.0 for a stranger, and never
// much below it.
double LocalPrice(const Locals& locals, Service where,
                    const LocalDials& dials = LocalDials{});

// unwired-ok: a formatter for the harness and the probe's report; the game
// prints the sentence, and a screen that named the category next to it
// would be the game showing its working.
const char* HeardName(Heard what);

}  // namespace dirtbag
