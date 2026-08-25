#include "DirtbagYouth.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {
namespace {

struct Candidate {
  const char* name;
  const char* tag;
  int age;
};

// **Fourteen, and a squad holds four**, so a career meets most of them and
// never all at once. Pool order is recruitment order, so a squad fills
// predictably and every name eventually gets a turn.
// ...and **the fourteen are not enough for this port's careers.** Measured:
// a squad of four graduates in clumps every four or five years, so a
// thirty-year career runs the written pool dry at about the halfway point
// and the squad then dwindles to nothing -- a system that quietly ends.
//
// The source cannot hit this, because its year is eighteen days long. So
// past the fourteen, the squad keeps filling: a name off a longer list and
// one of the written tags, both derived from the recruit's number so a save
// always meets the same kid. You stop knowing them by heart; they do not
// stop turning up.
const char* kMoreNames[] = {
  "Sorrel", "Idris", "Clementine", "Ottoline", "Bram", "Nyla", "Caspar",
  "Elowen", "Rafi", "Marnie", "Tobin", "Delphine", "Cassius", "Orla",
  "Emrys", "Saskia", "Linus", "Wilhelmina", "Ptolemy Jr", "Anouk",
  "Fenwick", "Rosalind", "Ivo", "Sunniva", "Barnaby", "Xanthe",
};
constexpr int kMoreNameCount =
    static_cast<int>(sizeof(kMoreNames) / sizeof(kMoreNames[0]));

const Candidate kPool[] = {
  {"Piper", "the one who started all this", 12},
  {"Wren", "no fear, no technique, yet", 11},
  {"Amos", "thinks about every move too long", 13},
  {"Soledad", "here for her sister, staying for herself", 11},
  {"Kit", "strong, bored, needs a project", 14},
  {"Bo", "cries when he falls, comes back anyway", 10},
  {"Ines", "quietly the best footwork in the room", 12},
  {"Ptolemy", "talks the entire session", 11},
  {"Mabel", "turned up alone on a bus", 13},
  {"Freddie", "his dad watches from the car", 12},
  {"Ozzy", "only wants to do the overhang", 10},
  {"Junie", "reads the route before she touches it", 13},
  {"Hal", "joined to get out of football", 11},
  {"Perpetua", "small, furious, unstoppable", 9},
};
constexpr int kPoolCount = static_cast<int>(sizeof(kPool) / sizeof(kPool[0]));

const char* kCoaches[] = {"Marta Oyelaran", "Denny Whitlock", "Sunny Kaur",
                          "Cormac Byrne"};
constexpr int kCoachCount = static_cast<int>(sizeof(kCoaches) / sizeof(kCoaches[0]));

const char* kSessionLines[] = {
  "Drills, silliness, and one genuine breakthrough on the slab.",
  "Two of them worked the same problem for an hour and would not be moved "
  "off it.",
  "Somebody cried, somebody sent, and everybody stayed twenty minutes late.",
  "You spent the whole session on footwork and they spent the whole session "
  "complaining about footwork.",
  "A parent stayed to watch the last half hour and did not say a word the "
  "entire time.",
};
constexpr int kSessionLineCount =
    static_cast<int>(sizeof(kSessionLines) / sizeof(kSessionLines[0]));

const char* kTripLines[] = {
  "A local round two towns over. One podium, three personal bests, and a van "
  "full of noise on the way home.",
  "A regional junior comp. They finished mid-pack and talked about it like a "
  "war they had won.",
  "A youth open at a gym four times your size. Nobody was intimidated, which "
  "was the whole point of going.",
};
constexpr int kTripLineCount =
    static_cast<int>(sizeof(kTripLines) / sizeof(kTripLines[0]));

double YouthDraw(const std::string& label) {
  return Rng::FromSeed(label).NextDouble();
}

// Everybody who has ever been on this squad, so nobody is recruited twice.
bool BeenHere(const Youth& youth, const char* name) {
  for (const YouthKid& k : youth.kids) {
    if (k.name == name) return true;
  }
  for (const Graduate& g : youth.graduated) {
    if (g.name == name) return true;
  }
  return false;
}

// The next unused name in pool order, and past that a made-up one. Pool
// order is recruitment order, so a squad fills predictably and the written
// fourteen all get their turn before anybody is invented.
Candidate NextUp(const Youth& youth, std::string& held) {
  for (int i = 0; i < kPoolCount; i++) {
    if (!BeenHere(youth, kPool[i].name)) return kPool[i];
  }
  // Everybody written has been through. Keep going: derived from how many
  // have, so the same save always meets the same kid, and it walks forward
  // on a collision rather than re-drawing.
  const int past = static_cast<int>(youth.graduated.size() + youth.kids.size());
  int n = past % kMoreNameCount;
  int cycle = past / kMoreNameCount;
  // **And when the second list comes round too, an initial** -- which is
  // what a squad with two Sorrels in it does anyway. Bounded only by how
  // long anybody plays: twenty-six names by twenty-six initials is more
  // kids than a career of any length puts through a gym.
  for (int guard = 0; guard <= kMoreNameCount * 26; guard++) {
    held = kMoreNames[n];
    if (cycle > 0) {
      held += ' ';
      held += static_cast<char>('A' + (cycle - 1) % 26);
      held += '.';
    }
    if (!BeenHere(youth, held.c_str())) break;
    n = (n + 1) % kMoreNameCount;
    if (n == past % kMoreNameCount) cycle++;
  }
  Candidate made;
  made.name = held.c_str();
  made.tag = kPool[past % kPoolCount].tag;
  // Nine to fourteen, the same span the written pool covers.
  made.age = 9 + past % 6;
  return made;
}

}  // namespace

const char* YouthBand(double level, const YouthDials& dials) {
  if (level >= dials.levelMax) return "ready";
  if (level >= dials.levelMax * 0.75) return "winning things";
  if (level >= dials.levelMax * 0.5) return "competing";
  if (level >= dials.levelMax * 0.25) return "getting somewhere";
  return "learning to fall";
}

int YouthAge(const YouthKid& kid, int day, const AgeDials& age) {
  const int period = std::max(1, age.daysPerYear);
  return kid.ageAtJoin + std::max(0, day - kid.joinedDay) / period;
}

bool ReadyToGraduate(const YouthKid& kid, int day, const YouthDials& dials,
                     const AgeDials& age) {
  return kid.level >= dials.levelMax && YouthAge(kid, day, age) >= dials.gradAge;
}

bool FoundTheYouthTeam(Youth& youth, double& cash, int day,
                       const Rng& worldRng, const YouthDials& dials) {
  (void)worldRng;
  if (youth.going) return false;
  if (cash < dials.foundCost) return false;
  cash -= dials.foundCost;

  youth = Youth{};
  youth.going = true;
  youth.foundedDay = day;
  youth.lastSessionDay = -99;
  for (int i = 0; i < dials.squadSize; i++) {
    std::string held;
    const Candidate who = NextUp(youth, held);
    YouthKid kid;
    kid.name = who.name;
    kid.tag = who.tag;
    kid.ageAtJoin = who.age;
    kid.joinedDay = day;
    youth.kids.push_back(kid);
  }
  return true;
}

bool SetYouthCoach(Youth& youth, bool hired, const Rng& worldRng, int day) {
  (void)worldRng;
  if (!youth.going) return false;
  if (hired == !youth.youCoach) return false;
  if (!hired) {
    youth.youCoach = true;
    youth.coachName.clear();
    return true;
  }
  const int which = static_cast<int>(
      YouthDraw("youthcoach|" + std::to_string(day) + "|" +
                std::to_string(youth.foundedDay)) * kCoachCount) % kCoachCount;
  youth.youCoach = false;
  youth.coachName = kCoaches[which];
  return true;
}

bool SquadIsDue(const Youth& youth, int day, const YouthDials& dials) {
  return youth.going && day - youth.lastSessionDay >= dials.sessionGapDays;
}

std::string WhyNotASession(const Youth& youth, int day, double energy,
                           const YouthDials& dials) {
  if (!youth.going) return "";
  if (!youth.youCoach) {
    return youth.coachName +
           " runs the sessions now. Take them back if you want the evenings.";
  }
  const int since = day - youth.lastSessionDay;
  if (since < dials.sessionGapDays) {
    return "They train every " + std::to_string(dials.sessionGapDays) +
           " days -- next one in " +
           std::to_string(dials.sessionGapDays - since) + ".";
  }
  if (energy < dials.sessionEnergy) {
    return "Too wrecked to be any use to them tonight.";
  }
  return "";
}

YouthSession RunASession(Youth& youth, const Rng& worldRng, int day,
                         double wingHelp, const YouthDials& dials,
                         const AgeDials& age) {
  (void)worldRng;
  YouthSession out;
  if (!SquadIsDue(youth, day, dials)) return out;

  youth.sessions++;
  youth.lastSessionDay = day;
  out.ran = true;
  out.wasATrip = youth.sessions % dials.compEvery == 0;

  // **Your hours, or somebody's wage.** Steady. Not you.
  const double gain =
      (youth.youCoach ? dials.gainBase + youth.craft * dials.gainPerCraft
                      : dials.gainHired) +
      wingHelp + (out.wasATrip ? dials.compBoost : 0.0);
  for (YouthKid& kid : youth.kids) {
    kid.level = std::min(dials.levelMax, kid.level + gain);
  }

  if (out.wasATrip) {
    // The trip teaches you something and reads as yours in the scene --
    // both only if you were the one in the van.
    if (youth.youCoach) {
      youth.craft = std::min(dials.craftMax, youth.craft + dials.craftPerTrip);
      out.standing = dials.tripStanding;
    }
    const int which = static_cast<int>(
        YouthDraw("youthtrip|" + std::to_string(youth.sessions) + "|" +
                  std::to_string(youth.foundedDay)) * kTripLineCount) %
        kTripLineCount;
    out.said = kTripLines[which];
  } else {
    const int which = static_cast<int>(
        YouthDraw("youthsess|" + std::to_string(youth.sessions) + "|" +
                  std::to_string(youth.foundedDay)) * kSessionLineCount) %
        kSessionLineCount;
    out.said = kSessionLines[which];
  }

  // **One graduation a session**, so it always gets its own moment rather
  // than being one of three things that happened on a Tuesday.
  for (std::size_t i = 0; i < youth.kids.size(); i++) {
    if (!ReadyToGraduate(youth.kids[i], day, dials, age)) continue;
    Graduate gone;
    gone.name = youth.kids[i].name;
    gone.day = day;
    gone.age = YouthAge(youth.kids[i], day, age);
    youth.graduated.push_back(gone);
    // Held until the rival generation turns over. See the header: this is
    // the payoff, and it lands one system over.
    youth.steppingUp.push_back(gone.name);
    youth.kids.erase(youth.kids.begin() + static_cast<long>(i));
    out.graduated = gone.name;
    out.psyche = dials.psycheGraduation;
    break;
  }

  if (!out.graduated.empty()) {
    // A place opens, so somebody fills it. **Always** -- the squad is four
    // and never the same four, and it does not dwindle because the written
    // names ran out.
    std::string held;
    const Candidate who = NextUp(youth, held);
    YouthKid kid;
    kid.name = who.name;
    kid.tag = who.tag;
    kid.ageAtJoin = who.age;
    kid.joinedDay = day;
    youth.kids.push_back(kid);
    out.joined = kid.name;
  } else {
    out.psyche = dials.psycheSession;
  }
  return out;
}

double YouthWageToday(const Youth& youth, const YouthDials& dials) {
  return youth.going && !youth.youCoach ? dials.coachWage : 0.0;
}

std::string SomebodyStepsUp(Youth& youth, int day, double wantedAge,
                            const YouthDials& dials, const AgeDials& age) {
  const int period = std::max(1, age.daysPerYear);
  std::size_t best = youth.steppingUp.size();
  double closest = 0.0;

  // Walk backwards so that removing as we go is safe, and so a tie goes to
  // the more recent graduate.
  for (std::size_t i = youth.steppingUp.size(); i-- > 0;) {
    // Find what they left as and when, so their age today is known.
    double now = -1.0;
    for (const Graduate& gone : youth.graduated) {
      if (gone.name != youth.steppingUp[i]) continue;
      now = gone.age + static_cast<double>(std::max(0, day - gone.day)) / period;
      break;
    }
    if (now < 0.0) continue;
    if (now > wantedAge + dials.stepsUpSlack) {
      // Too old to be a rookie. They had their chance and went and had a
      // life instead.
      youth.steppingUp.erase(youth.steppingUp.begin() +
                             static_cast<long>(i));
      continue;
    }
    if (now < wantedAge - dials.stepsUpSlack) continue;   // not yet
    const double off = std::fabs(now - wantedAge);
    if (best == youth.steppingUp.size() || off < closest) {
      best = i;
      closest = off;
    }
  }
  if (best >= youth.steppingUp.size()) return "";
  const std::string who = youth.steppingUp[best];
  youth.steppingUp.erase(youth.steppingUp.begin() + static_cast<long>(best));
  return who;
}

std::string YouthLine(const Youth& youth, int day, const YouthDials& dials,
                      const AgeDials& age) {
  if (!youth.going) return "";
  std::string out = std::to_string(youth.kids.size()) + " kids";
  if (!youth.youCoach) out += ", " + youth.coachName + " coaching";

  // Whoever is closest, and **which of the two gates is the one holding
  // them** -- because those are different sentences and the player can only
  // do something about one of them.
  const YouthKid* nearest = nullptr;
  for (const YouthKid& kid : youth.kids) {
    if (nearest == nullptr || kid.level > nearest->level) nearest = &kid;
  }
  if (nearest != nullptr) {
    out += "  --  " + nearest->name + ", " + nearest->tag + ", " +
           YouthBand(nearest->level, dials);
    if (nearest->level >= dials.levelMax) {
      const int years = dials.gradAge - YouthAge(*nearest, day, age);
      out += years > 0
                 ? ", and " + std::to_string(years) +
                       (years == 1 ? " year too young" : " years too young")
                 : ", and gone the next time you take them out";
    }
  }
  if (!youth.graduated.empty()) {
    out += "  --  " + std::to_string(youth.graduated.size()) +
           (youth.graduated.size() == 1 ? " has left for the field"
                                        : " have left for the field");
  }
  return out;
}

}  // namespace dirtbag
