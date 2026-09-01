#include "DirtbagLife.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {
namespace {

int Slot(Thread t) { return static_cast<int>(t); }

bool Real(Thread t) {
  return t != Thread::None && t != Thread::kThreadCount;
}

// How long ago, in the units a person would actually use. Prefixed because
// the unity build makes every anonymous namespace in Sim/ one namespace,
// and "HowLong" is a name three other files could plausibly want.
std::string LifeHowLong(int days) {
  if (days <= 0) return "today";
  if (days == 1) return "a day";
  if (days < 14) return std::to_string(days) + " days";
  if (days < 60) return std::to_string(days / 7) + " weeks";
  return std::to_string(days / 30) + " months";
}

// What the game says when you have been keeping it up, and when you have
// not. Per thread rather than generic, because "it is going well" said
// about a person and about a paperback is a game that has stopped paying
// attention. Index by slot; slot 0 is `None` and is never read.
const char* kKept[kThreadCount] = {
    "",
    "You have been around, and it shows.",
    "They know roughly where you are and roughly what you are doing.",
    "Your calluses are doing two jobs.",
    "You are halfway through something and you know which.",
    "You have been eating like a person.",
};

// ...and the cooling line, which takes how long it has been.
const char* kCooling[kThreadCount] = {
    "",
    "You have not seen them in ",
    "You have not called in ",
    "It has been in the case ",
    "Same page for ",
    "The stove has not come out in ",
};

}  // namespace

bool Going(const Life& life, Thread t) {
  return Real(t) && life.strands[Slot(t)].going;
}

double WarmthOf(const Life& life, Thread t) {
  return Real(t) ? life.strands[Slot(t)].warmth : 0.0;
}

double DepthOf(const Life& life, Thread t) {
  return Real(t) ? life.strands[Slot(t)].depth : 0.0;
}

int DaysSince(const Life& life, Thread t, int today) {
  if (!Real(t)) return 0;
  return std::max(0, today - life.strands[Slot(t)].lastGivenDay);
}

double AsksFor(Thread t, const LifeDials& dials) {
  return Real(t) ? dials.asksForHours[Slot(t)] : 0.0;
}

bool CanTakeUp(const Life& life, Thread t, int today, const LifeDials& dials) {
  if (!Real(t)) return false;
  const Strand& s = life.strands[Slot(t)];
  if (s.going) return false;
  // The only refusal in the file. Everything else you can pick back up on a
  // wet Tuesday; this one you cannot, and that is the difference between a
  // hobby and a person.
  if (t == Thread::Someone && s.ended &&
      static_cast<double>(today - s.endedDay) < dials.sparkCooldownDays) {
    return false;
  }
  return true;
}

bool TakeUp(Life& life, Thread t, int today, const LifeDials& dials) {
  if (!CanTakeUp(life, t, today, dials)) return false;
  Strand& s = life.strands[Slot(t)];
  s.going = true;
  // Starts warm. Nobody takes up a thing they are already neglecting, and
  // starting cold would mean every new thread began one bad fortnight from
  // ending. **Depth starts at nothing**, which is the field that matters:
  // a week-old romance is as warm as a five-year one and costs nothing to
  // lose, which is exactly right.
  s.warmth = 1.0;
  s.lastGivenDay = today;
  // The day you took it up is a day you gave it.
  s.daysGiven = std::max(1, s.daysGiven);
  // `ended` is deliberately left alone: taking somebody else up does not
  // un-happen the last one, and `endedDay` is what the cooldown reads.
  return true;
}

bool GiveItTime(Life& life, Thread t, double hours, int today,
                const LifeDials& dials) {
  if (!Going(life, t) || hours <= 0.0) return false;
  const int i = Slot(t);
  Strand& s = life.strands[i];
  s.warmth = std::min(1.0, s.warmth + dials.warmsPerHour[i] * hours);
  // Depth moves once a day and only as well as the day went — you do not
  // get further in by turning up badly, and an afternoon split into six
  // calls is still an afternoon.
  if (s.lastGivenDay != today) {
    s.daysGiven++;
    s.depth = std::min(1.0, s.depth + dials.deepensPerDay[i] * s.warmth);
  }
  s.lastGivenDay = today;
  return true;
}

Thread LifeNight(Life& life, int today, const LifeDials& dials) {
  // The grief first, and it fades as it counts down rather than switching
  // off on the last night. The weight is simply what is left of it, which
  // needs no third field and cannot drift out of step with the clock.
  if (life.grieving > 0.0) {
    life.griefWeight *=
        std::max(0.0, (life.grieving - 1.0) / std::max(1.0, life.grieving));
    life.grieving = std::max(0.0, life.grieving - 1.0);
    if (life.grieving <= 0.0) life.griefWeight = 0.0;
  }

  Thread lost = Thread::None;
  for (int i = 1; i < kThreadCount; i++) {
    Strand& s = life.strands[i];
    if (!s.going) continue;
    const int since = DaysSince(life, static_cast<Thread>(i), today);
    if (static_cast<double>(since) > dials.patienceDays[i]) {
      s.warmth = std::max(0.0, s.warmth - dials.coolsPerDay[i]);
    }
    // **Only Someone leaves.** A guitar that has been in its case for a
    // year is still behind the driver's seat, and family does not resign;
    // both of those go to zero warmth and sit there paying you nothing,
    // waiting for an afternoon. A person does not wait, and the phase's
    // "can end badly through neglect alone" is this branch and nothing
    // else.
    if (s.warmth <= 0.0 && static_cast<Thread>(i) == Thread::Someone) {
      s.going = false;
      s.ended = true;
      s.endedDay = today;
      if (s.depth >= dials.enoughToLose) {
        life.grieving = dials.grievesForDays * s.depth;
        life.griefWeight = s.depth;
      }
      lost = static_cast<Thread>(i);
    }
  }
  return lost;
}

double LifePsyche(const Life& life, const LifeDials& dials) {
  return dials.someonePsyche * WarmthOf(life, Thread::Someone) -
         dials.griefPsyche * Clamp01(life.griefWeight);
}

double LifeNerve(const Life& life, const LifeDials& dials) {
  return dials.homeNerve * WarmthOf(life, Thread::Home);
}

double LifeMealCost(const Life& life, const LifeDials& dials) {
  const double w = WarmthOf(life, Thread::Cooking);
  return 1.0 - (1.0 - dials.cookingCost) * w;
}

double LifeMealHunger(const Life& life, const LifeDials& dials) {
  return 1.0 + (dials.cookingFeeds - 1.0) * WarmthOf(life, Thread::Cooking);
}

double LifeRestRate(const Life& life, const LifeDials& dials) {
  return 1.0 + (dials.booksRest - 1.0) * WarmthOf(life, Thread::Books);
}

double BuskingPay(const Life& life, double hours, const LifeDials& dials) {
  if (hours <= 0.0 || !Going(life, Thread::Music)) return 0.0;
  const double w = WarmthOf(life, Thread::Music);
  const double d = DepthOf(life, Thread::Music);
  return hours * w * (dials.buskPerHour + dials.buskDepthPay * d);
}

const char* ThreadName(Thread t) {
  switch (t) {
    case Thread::Someone: return "Someone";
    case Thread::Home: return "Home";
    case Thread::Music: return "The guitar";
    case Thread::Books: return "Books";
    case Thread::Cooking: return "The stove";
    default: return "";
  }
}

const char* ThreadLine(Thread t) {
  switch (t) {
    case Thread::Someone:
      return "Somebody who does not care what you climbed today.";
    case Thread::Home:
      return "A number you keep meaning to dial.";
    case Thread::Music:
      return "It lives behind the driver's seat and it is slightly out of tune.";
    case Thread::Books:
      return "A milk crate of paperbacks, most of them somebody else's.";
    case Thread::Cooking:
      return "Two burners and the will to use them.";
    default:
      return "";
  }
}

std::string ItEnded(Thread t) {
  if (t == Thread::Someone) {
    return "It ended the way these end: not in a row, in a month of you "
           "being somewhere else.";
  }
  if (!Real(t)) return "";
  return std::string(ThreadName(t)) + " went quiet, and you could not say "
                                      "which week.";
}

std::string HowItIsGoing(const Life& life, Thread t, int today,
                         const LifeDials& dials) {
  if (!Real(t)) return "";
  const int i = Slot(t);
  const Strand& s = life.strands[i];
  // Never taken up is not a status. "You have no hobbies" printed at
  // somebody who has not been offered one yet is the game blaming them for
  // a menu they have not opened.
  if (!s.going && !s.ended) return "";
  if (!s.going) {
    return "Over, and it has been " + LifeHowLong(today - s.endedDay) + ".";
  }
  const int since = DaysSince(life, t, today);
  if (static_cast<double>(since) <= dials.patienceDays[i]) return kKept[i];

  std::string line = std::string(kCooling[i]) + LifeHowLong(since) + ".";
  if (t == Thread::Someone) {
    // The one thread that can be lost gets told twice: once when it starts
    // slipping and once when it is nearly gone. A thing that ends without
    // warning is not neglect, it is a die roll.
    line += s.warmth <= 0.25 ? " This is the part where it ends."
                             : " They have noticed.";
  }
  return line;
}

std::string LifeLabel(const Life& life, int today, const LifeDials& dials) {
  if (life.grieving > 0.0) return "Still not over it";

  // The worst offender, and only if it is actually offending. Coolest
  // first, so a thread on the edge beats one that has merely lapsed.
  int worst = 0;
  double coolest = 2.0;
  for (int i = 1; i < kThreadCount; i++) {
    const Strand& s = life.strands[i];
    if (!s.going) continue;
    // **Nothing left to warn about.** A guitar that has been in its case
    // for two years is at zero and cannot go lower, and it is not going
    // anywhere either -- so a line saying "The guitar: 2 years" every
    // night for the rest of a career is the exact failure this readout
    // exists to avoid, arrived at from the other end. Warn while there is
    // something to lose; after that it is not news, it is an inventory.
    if (s.warmth <= 0.0) continue;
    const int since = DaysSince(life, static_cast<Thread>(i), today);
    if (static_cast<double>(since) <= dials.patienceDays[i]) continue;
    if (s.warmth < coolest) {
      coolest = s.warmth;
      worst = i;
    }
  }
  if (worst == 0) return "";
  return std::string(ThreadName(static_cast<Thread>(worst))) + ": " +
         LifeHowLong(DaysSince(life, static_cast<Thread>(worst), today));
}

}  // namespace dirtbag
