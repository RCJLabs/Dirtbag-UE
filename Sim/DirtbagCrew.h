#pragma once

// The crew.
//
// Pillar 6 of concepts/DIRTBAG.md: *"a crew the town names without asking
// you."* That last clause is the whole system. You do not pick this, you do
// not earn it by deciding to, and you cannot change it once it sticks -- it
// is what a valley calls the people who keep turning up together, and a
// valley is not asking your opinion.
//
// Every input already existed: bonds say who you climb with, standing says
// what the town has watched you do. Nothing here is new state except the
// name itself and how long you have read as a group.

#include <string>
#include <vector>

#include "DirtbagFactions.h"
#include "DirtbagPartner.h"
#include "DirtbagRng.h"

namespace dirtbag {

struct CrewDials {
  // Two people you actually climb with. You plus two is a crew; you plus one
  // is a partnership, which the game already models as a bond and does not
  // need a name for.
  int bondsThatMakeACrew = 2;

  // What counts as climbing with somebody rather than knowing their name.
  // Half rapport is a season of turning up -- rapportPerDay is 0.08 against
  // a decay of 0.01, so this is roughly a fortnight of actually going out
  // and not much more of not.
  double rapportThatCounts = 0.5;

  // And it has to hold. A month, because the point of a nickname is that
  // somebody said it twice -- a crew that exists for one good week in
  // September is three people who had a good week in September.
  int daysBeforeTheyNameYou = 30;

  // Below this the town does not rate you, whoever you drink with, and the
  // name says so. Not a punishment: an unflattering nickname is still a
  // nickname, and being *called* something is the point.
  double standingThatEarnsAKindName = 0.15;
};

struct Crew {
  // Empty until the town says it. Once said, it never changes -- see the
  // header comment. Migrating this to something the player picks would be a
  // different design and a worse one.
  std::string name;
  int namedOnDay = 0;
  int daysReadingAsACrew = 0;
  int membersWhenNamed = 0;
};

// Do you read as a group today? Bonds only -- what the town sees is who is
// in the van and at the fire, not what you climbed.
bool ReadsAsACrew(const std::vector<PartnerBond>& bonds,
                  const CrewDials& dials = CrewDials{});

// A day of being seen together. Returns true on the day the name lands,
// which is the caller's cue to say so once.
//
// The name is drawn from the faction that thinks most of you, because that
// is the part of town doing the talking -- and from a hash of the world and
// the members, so the same people in the same valley are always called the
// same thing and a different crew is called something else.
bool CrewDay(Crew& crew, const std::vector<PartnerBond>& bonds,
             const Standing& standing, const Rng& worldRng, int day,
             const CrewDials& dials = CrewDials{});

// "They call you the trail crew." Empty until there is a name.
std::string CrewText(const Crew& crew);

}  // namespace dirtbag
