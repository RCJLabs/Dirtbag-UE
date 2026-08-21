// The Lot: the people who are already there when you pull in.
//
// Phase 2 asks for two named neighbours and three partners with their own
// careers. The careers are the point — a partner who is only a beta vending
// machine is furniture. These people climb whether you do or not, get better
// at their own pace, and work their own projects, which means the open lines
// at the crag are not reserved for you. Somebody can take the first ascent
// you were saving skin for. That is the sharpest thing the Lot does, and it
// is why the window and the skin budget have teeth beyond arithmetic.
//
// Almost nothing here is saved. A partner's strength on any given day is a
// pure function of world seed, who they are, and the date, so it cannot
// drift and costs nothing to store. Only what passed *between* you and them
// — rapport, and which lines they took — is career state.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagCrag.h"
#include "DirtbagRng.h"
#include "DirtbagSessionLoop.h"

namespace dirtbag {

// What a career remembers about somebody. Their strength is derived from
// seed and date, so none of that is here — only what the player changed.
struct PartnerBond {
  std::string name;
  double rapport = 0.0;
  std::vector<std::string> firstAscents;   // route keys they got there first
};

struct PartnerDials {
  // How fast a partner's grade creeps, in skill points per day. At 0.03 a
  // season moves somebody about a third of a grade — the same glacial pace
  // the player lives at, because a Lot where everyone outpaces you is a
  // different and worse game.
  double skillPerDay = 0.03;

  // Rapport per day spent climbing together, and what it decays to when you
  // stop turning up. People remember you, but not forever.
  double rapportPerDay = 0.08;
  double rapportDecayPerDay = 0.01;

  // What company is worth. Psyche is ability in the resolver (psycheWeight
  // 1.5 grades across its range), so this is deliberately small: a good
  // belayer is worth something and never worth a grade.
  double psychePerRapport = 0.12;

  // Beta a partner hands over, as a fraction of what they know that you do
  // not. Full rapport gets you most of it; a stranger tells you where the
  // crux is and no more.
  double betaShareAtFullRapport = 0.7;
  double betaShareAtNoRapport = 0.2;

  // How likely a partner is to take an open project on a day they are
  // strong enough for it. Low: the crag's lines have sat there for years,
  // and the player should usually get the chance if they commit. But not
  // never — a project nobody can lose is not a project.
  double firstAscentChancePerDay = 0.02;

  // How far above a line's true grade a partner has to be before they will
  // commit to it at all. They are not projecting at their limit for months;
  // that is the player's job.
  double partnerMarginGrades = 1.0;
};

// One of the Lot's people.
struct Partner {
  std::string name;
  std::string tag;          // one line of who they are, in their own register
  Climber climber;          // strength on the day you asked
  double ambition = 0.5;    // 0..1; scales how fast they improve
  bool climbs = true;       // the neighbours are not all climbers

  // Career state — the only part that is saved, because it is the only part
  // that depends on what the player did.
  double rapport = 0.0;     // 0..1
  std::vector<std::string> firstAscents;  // route keys they got to first
};

// The Lot's regulars for this world, as they stand on `day`. Deterministic:
// the same seed and day always yields the same people at the same strength.
std::vector<Partner> LotRegulars(const Rng& worldRng, int day,
                                 const PartnerDials& dials = PartnerDials{});

// A partner's climbing strength on a given day, derived rather than stored.
Climber PartnerOn(const Rng& worldRng, const std::string& name,
                  double baseSkill, double ambition, int day,
                  const PartnerDials& dials = PartnerDials{});

// Does this partner know this line well enough to be worth asking? They know
// what they have climbed, and the crag's moderates are common knowledge.
bool KnowsLine(const Partner& partner, const CragLine& line);

// Beta, handed over. Returns how much the ledger's knowledge went up by;
// never past what the partner actually knows, and never backwards.
double ShareBeta(const Partner& partner, const CragLine& line,
                 ProjectMemory& memory,
                 const PartnerDials& dials = PartnerDials{});

// The same, adjusted for who you are to them. `generosity` scales how much
// of the sequence they bother to spell out — somebody who likes you talks
// you through the crux, somebody who does not says "it goes left".
//
// It scales the share, never the ceiling: beta still stops at fully wired
// and still only covers the part you were missing, so a well-liked climber
// learns a line faster and never learns more of it than there is.
double ShareBeta(const Partner& partner, const CragLine& line,
                 ProjectMemory& memory, double generosity,
                 const PartnerDials& dials = PartnerDials{});

// What climbing with this person does to your head, added to psyche.
double PsycheFrom(const Partner& partner,
                  const PartnerDials& dials = PartnerDials{});

// A day at the Lot with them: rapport grows if you turned up, decays if you
// did not.
void SpendDayWith(Partner& partner, bool together,
                  const PartnerDials& dials = PartnerDials{});

// Whether this partner takes an open line today, and which. Returns the
// index into crag.lines, or -1. Rolls on its own named stream, so partners
// getting on with their lives cannot shift the player's attempts.
int PartnerTakesFirstAscent(const Rng& worldRng, const Partner& partner,
                            const Crag& crag,
                            const std::vector<std::string>& alreadyTaken,
                            int day,
                            const PartnerDials& dials = PartnerDials{});

// Fold a career's remembered bonds into today's people, and back out again.
// The split is the point: everything derived stays derived.
void ApplyBonds(std::vector<Partner>& lot,
                const std::vector<PartnerBond>& bonds);
std::vector<PartnerBond> BondsFrom(const std::vector<Partner>& lot);

// "Margo has been on the arete all week." — what you can see from the fire,
// so a project you are losing is something you watched happen.
std::string LotTalk(const Partner& partner, const Crag& crag, int day);

}  // namespace dirtbag
