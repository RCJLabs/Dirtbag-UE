#include "DirtbagLocals.h"

#include <algorithm>

namespace dirtbag {
namespace {

// Sixteen, and none of them shared with the rival pool or the Lot's. A
// career meets five of these and keeps them for thirty years, so the pool
// only has to be wide enough that two valleys do not feel like one town.
const char* kCounterNames[] = {
    "Marge Tiller",  "Dev Okonkwo",  "Sal Brightwater", "Hollis Kerr",
    "Nan Petrov",    "Amos Redding", "Junie Fenwick",   "Tobias Grewe",
    "Winnie Ash",    "Cal Marchetti","Ruth Bandara",    "Ozzie Lin",
    "Pearl Hadley",  "Emeka Stroud", "Fran Delacroix",  "Bo Whitfield",
};
constexpr int kCounterNameCount =
    static_cast<int>(sizeof(kCounterNames) / sizeof(kCounterNames[0]));

}  // namespace

Locals TheLocals(const Town& town, const Rng& worldRng) {
  // Off a labelled sub-stream so adding a person here can never shift the
  // weather, the routes or the Lot. Same discipline as every other roll in
  // the game.
  Rng rng = worldRng.Derive("locals");
  Locals out;
  // One per service the town actually offers, found by walking the venues
  // rather than by a second list of services -- a town that gains a bakery
  // gains a face for free, and one that loses the garage does not leave a
  // person standing in a field.
  for (int s = 0; s < 5; s++) {
    const Service service = static_cast<Service>(s);
    if (VenuesFor(town, service).empty()) continue;
    Local who;
    who.where = service;
    who.name = kCounterNames[rng.IntRange(0, kCounterNameCount - 1)];
    out.people.push_back(who);
  }
  return out;
}

Local* At(Locals& locals, Service where) {
  for (Local& r : locals.people) {
    if (r.where == where) return &r;
  }
  return nullptr;
}

const Local* At(const Locals& locals, Service where) {
  for (const Local& r : locals.people) {
    if (r.where == where) return &r;
  }
  return nullptr;
}

namespace {

// The one rule about what gets talked about, in one place. Quieter than
// what they are already holding is not news -- it is dropped rather than
// queued, because a person is not a mailbox.
void Hold(Local& who, Heard what, const std::string& about) {
  // **Equal replaces.** Two sends in a fortnight and what they bring up is
  // the second one -- the gate is *what did you do last time*, so a newer
  // fact of the same loudness has to win. Only a quieter one is dropped.
  if (what < who.holds) return;
  who.holds = what;
  who.about = about;
}

}  // namespace

void Tell(Locals& locals, Heard what, const std::string& about, int day) {
  (void)day;   // the day it happened is not a thing anybody repeats back
  for (Local& r : locals.people) Hold(r, what, about);
}

void TellOne(Locals& locals, Service where, Heard what,
             const std::string& about, int day) {
  (void)day;
  if (Local* who = At(locals, where)) Hold(*who, what, about);
}

std::string WhatTheySay(const Local& who, int today,
                        const LocalDials& dials) {
  // A customer gets served. Being known is what buys the sentence, and it
  // is the whole reason the greeting reads as earned rather than issued.
  if (who.known < dials.nodsAt) return "";

  const std::string it = who.about;
  switch (who.holds) {
    case Heard::Sponsored:
      return "Saw your name on something. Suits you.";
    case Heard::Won:
      return it.empty() ? "Somebody said you won something."
                        : "Somebody said you won " + it + ".";
    case Heard::Named:
      return it.empty() ? "Heard you put one up."
                        : it + ". That was you, then.";
    case Heard::Sent:
      return it.empty() ? "Heard you had a good week."
                        : "Heard you got " + it + ".";
    case Heard::Hurt:
      return it.empty() ? "You were limping. Better?" : "How is the " + it + "?";
    case Heard::Broke:
      return "You're good for it. I know.";
    default:
      break;
  }

  // **Nobody tells them this one.** It is what is left when it has been
  // ages and there is nothing else -- the only memory in the file that is
  // derived rather than given, and the only one that fires for doing
  // nothing at all.
  if (static_cast<double>(today - who.lastSeen) >= dials.awayDays) {
    return "Thought you'd moved on.";
  }
  return "";
}

void Seen(Local& who, int today, const LocalDials& dials) {
  // Asked before the visit is counted, so it is the same answer the player
  // just read -- they greet you as they knew you walking in, not as they
  // know you walking out. Which also makes this spend exactly what
  // `WhatTheySay` would have said, by construction rather than by two
  // conditions agreeing.
  const bool saidIt = !WhatTheySay(who, today, dials).empty();

  who.known = std::min(1.0, who.known + dials.knownPerVisit);
  who.everKnew = std::max(who.everKnew, who.known);
  who.lastSeen = today;

  // **Spent, if it was spent.** They said it; now they go back to nodding.
  // A line that repeats every visit stops being a greeting inside a week --
  // and a memory nobody heard is not a line that was said.
  if (saidIt) {
    who.holds = Heard::None;
    who.about.clear();
  }
}

void LocalsDay(Locals& locals, int today, const LocalDials& dials) {
  for (Local& r : locals.people) {
    if (r.lastSeen >= today) continue;
    const double floor_ = r.everKnew * Clamp01(dials.knownKeeps);
    r.known = std::max(floor_, r.known - dials.knownDecayPerDay);
  }
}

double LocalPrice(const Locals& locals, Service where,
                    const LocalDials& dials) {
  const Local* who = At(locals, where);
  if (!who) return 1.0;
  return 1.0 - dials.discountAt * Clamp01(who->known);
}

const char* HeardName(Heard what) {
  switch (what) {
    case Heard::Away:      return "away";
    case Heard::Broke:     return "broke";
    case Heard::Hurt:      return "hurt";
    case Heard::Sent:      return "sent";
    case Heard::Named:     return "named";
    case Heard::Won:       return "won";
    case Heard::Sponsored: return "sponsored";
    default:               return "nothing";
  }
}

}  // namespace dirtbag
