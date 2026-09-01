#include "DirtbagCrew.h"

#include <algorithm>

namespace dirtbag {

namespace {

// What a valley calls people, sorted by who is doing the calling.
//
// These are nicknames rather than titles: short, slightly sideways, and
// never something the crew would have chosen. A town does not name you
// after your best send, it names you after the thing it keeps seeing --
// the van in the corner, the bags carried out, the brush.

const char* kOldGuardNames[] = {
    "the regulars", "the quiet ones", "the early crowd", "the old lot",
    "the ones who put the gate back",
};

const char* kSceneNames[] = {
    "the Lot lot",      "the campfire crowd", "the van kids",
    "the loud corner",  "the ones with the speaker",
};

const char* kDevelopmentNames[] = {
    "the brush crew",   "the new-route lot", "the line hunters",
    "the ones with the drill", "the bolt fund",
};

const char* kStewardshipNames[] = {
    "the trail crew",   "the path menders", "the good neighbours",
    "the ones who carry the bags out", "the litter run",
};

// And the name you get when nobody rates you. Still a name -- being called
// something is the point, and a valley that has not decided about you yet
// has still noticed the van.
const char* kUnratedNames[] = {
    "those kids",       "the ones from the car park", "the visitors",
    "the van in the corner", "whoever they are",
};

struct NameList {
  const char* const* names;
  int count;
};

NameList ListFor(Faction f) {
  switch (f) {
    case Faction::OldGuard:
      return {kOldGuardNames, 5};
    case Faction::Scene:
      return {kSceneNames, 5};
    case Faction::Development:
      return {kDevelopmentNames, 5};
    case Faction::Stewardship:
      return {kStewardshipNames, 5};
  }
  return {kUnratedNames, 5};
}

}  // namespace

bool ReadsAsACrew(const std::vector<PartnerBond>& bonds,
                  const CrewDials& dials) {
  int close = 0;
  for (const PartnerBond& b : bonds) {
    if (b.rapport >= dials.rapportThatCounts) close++;
  }
  return close >= dials.bondsThatMakeACrew;
}

bool CrewDay(Crew& crew, const std::string& self,
             const std::vector<PartnerBond>& bonds, const Standing& standing,
             const Rng& worldRng, int day, const CrewDials& dials) {
  // Named is named. The town does not revisit it because you had a thin
  // winter, and it does not take it back if the crew drifts apart -- that
  // is what a nickname is, and a career that outlives its own crew still
  // gets called the thing it got called.
  if (!crew.name.empty()) return false;

  if (!ReadsAsACrew(bonds, dials)) {
    // Not a reset to zero: a fortnight where somebody was hurt or working
    // should not cost you the month. It slides back at the same rate it
    // built, so a crew that mostly holds together still gets there.
    crew.daysReadingAsACrew = std::max(0, crew.daysReadingAsACrew - 1);
    return false;
  }

  crew.daysReadingAsACrew++;
  if (crew.daysReadingAsACrew < dials.daysBeforeTheyNameYou) return false;

  // Who is doing the talking: the part of town that thinks most of you.
  int loudest = 0;
  for (int i = 1; i < kFactionCount; i++) {
    if (standing.with[i] > standing.with[loudest]) loudest = i;
  }
  const bool rated =
      standing.with[loudest] >= dials.standingThatEarnsAKindName;
  const NameList list =
      rated ? ListFor(static_cast<Faction>(loudest)) : NameList{kUnratedNames, 5};

  // Stable across replays and across saves: the same people in the same
  // valley are always called the same thing. Members are sorted first so
  // that the order bonds happen to sit in cannot change the answer -- they
  // are rebuilt from the world seed daily and only the bond is career state.
  std::vector<std::string> who;
  for (const PartnerBond& b : bonds) {
    if (b.rapport >= dials.rapportThatCounts) who.push_back(b.name);
  }
  std::sort(who.begin(), who.end());

  std::string key = worldRng.seed + "|" + self;
  for (const std::string& w : who) key += "|" + w;

  const std::size_t h = HashSeedString(key);
  crew.name = list.names[h % static_cast<std::size_t>(list.count)];
  crew.namedOnDay = day;
  crew.membersWhenNamed = static_cast<int>(who.size()) + 1;  // and you
  return true;
}

std::string CrewText(const Crew& crew) {
  if (crew.name.empty()) return std::string();
  return "They call you " + crew.name + ".";
}

}  // namespace dirtbag
