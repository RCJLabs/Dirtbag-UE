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

#include "DirtbagCharacter.h"
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
  // **How well you ever knew them.** See `rapportKeeps` below.
  double everKnew = 0.0;
  std::vector<std::string> firstAscents;   // route keys they got there first
};

struct PartnerDials {
  // How fast a partner's grade creeps, in skill points per day, so that
  // a season moves somebody about a third of a grade — the same glacial
  // pace the player lives at, because a Lot where everyone outpaces you is
  // a different and worse game.
  //
  // The arithmetic, written out, because it was wrong for a long time at a
  // value whose comment claimed this exact sentence: a season is 365 days,
  // a grade is 100/14 = 7.14 skill points, so a third of a grade a season
  // is 2.38 points, which is 0.0065 a day. **It was 0.03** — 1.53 grades a
  // season, four and a half times its own documented intent.
  double skillPerDay = 0.0065;

  // And a ceiling, because there was none. Skills are documented 0..100
  // and partners are the only climbers in the game with no age model, so
  // the creep above ran unbounded: measured over thirty years, **Dev
  // reached power 382.5** and Trish — the one who is delighted to help and
  // cannot — reached 237.6. They did not merely out-climb the player, they
  // left the scale. A strong local plateaus; they do not ascend forever.
  double ceiling = 100.0;

  // --- Whether anybody is here ------------------------------------------
  //
  // **The Lot never varied.** `LotRegulars` handed back all five, every
  // day, for ninety years -- which is why `Personality::social` has had no
  // reader since Phase 7 (*"whether people turn up, and nobody ever fails
  // to turn up"*), why `nobelayer` measured **0 across three thirty-year
  // careers** so the belayer gate has never once bitten, and why the
  // roadmap's own standing question is *"the Lot never varies"*. Three
  // recorded gaps, one missing mechanic.
  //
  // Turnout is that mechanic. Each of them has their own week; these are
  // what bends it.

  // What knowing somebody is worth to whether you see them. Not *they come
  // because you are there* -- it is that you know their week, and you turn
  // up when they do. At full rapport it is worth a quarter, which turns
  // Dev from somebody you catch half the time into somebody you can plan
  // around.
  double rapportBringsThemOut = 0.25;

  // ...and what being Social is worth, at the axis's full +100. Slightly
  // more than rapport, because this is the difference between a person who
  // texts the Lot on the way and one who drives up hoping. It is signed:
  // a Loner at -100 loses the same amount, and that is the point of an
  // axis rather than a perk.
  double socialBringsThemOut = 0.30;

  // A day the rock is not in condition empties the Lot. Not to nothing --
  // Ray is a neighbour and Trish is psyched beyond all reason -- but a
  // washout is the loneliest day in this game, and now it reads that way.
  double emptyWhenWet = 0.45;

  // **Nobody is never here and nobody is always here.**
  //
  // Measured without these: an Influencer walked up to the Cave and found
  // nobody **zero times in thirty years**, against a Purist's ninety days.
  // The axis was not bending the mechanic, it was switching it off -- and
  // it is only that violent because it applies to three people
  // independently and their absences multiply. A floor and a ceiling keep
  // both ends of the axis inside the game: the most gregarious climber in
  // the valley still gets the odd empty Tuesday, and the most solitary one
  // still has Trish turn up.
  double neverLessThan = 0.05;
  double neverMoreThan = 0.95;

  // Rapport per day spent climbing together, and what it decays to when you
  // stop turning up. People remember you, but not forever.
  double rapportPerDay = 0.08;
  double rapportDecayPerDay = 0.01;

  // **What you keep of somebody after you stop turning up**, as a fraction
  // of how well you ever knew them.
  //
  // Without this, rapport is a hundred-day memory with hard clamps at both
  // ends, and the clamps destroy the arithmetic: a burst of climbing pins
  // you at 1.0, a long gap floors you at 0, and **where a career ends up is
  // decided entirely by its last few months.** Measured over three
  // thirty-year careers, every one of them ended at **0.00 with everybody
  // at the Lot** -- including one that climbed 2,085 days, which is nearly
  // six years of turning up. Nobody in ninety years of this game has ever
  // got past being a stranger, so `BurnsTheyWillHold` has only ever
  // returned the stranger's number and *"somebody who knows you gives you
  // the day"* has never once happened.
  //
  // You do not forget somebody you spent five years with. You stop being
  // current with them, which is a different thing and is what this is: the
  // decay still runs, and it runs down to a floor rather than to nothing.
  //
  // Same shape as the two the project has already found -- the ranking's
  // window and the scars' fade -- and the same lesson from a third angle:
  // **a value that ages needs a memory, not just a slope.**
  double rapportKeeps = 0.45;

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

  // Cleanliness above which a project counts as visibly somebody's. A
  // virgin line starts at 0.05 (FirstAscentDials::virginCleanliness), so
  // anything past this means a person has stood there with a wire brush,
  // which is the most public way there is of saying you are on it.
  //
  // Only meaningful for projects, and only ever consulted for projects:
  // an established line's ledger starts at 1.0 because everybody climbs it,
  // so this rule would call every route in the book "spoken for" if it were
  // ever asked about one. A test asked, which is how the number got a name
  // instead of being a bare 0.2 in a condition.
  double brushedEnoughToBeYours = 0.2;

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
  // How well you ever knew them. Never falls; it is what the drift floors
  // against. See `PartnerDials::rapportKeeps`.
  double everKnew = 0.0;
  std::vector<std::string> firstAscents;  // route keys they got to first
};

// The Lot's regulars for this world, as they stand on `day`. Deterministic:
// the same seed and day always yields the same people at the same strength.
// **Who is actually here today.** `LotRegulars` is the cast; this is the
// call sheet, and until it existed the two were the same list.
//
// Rolled per person per day on its own derived stream, so who is here does
// not depend on the order anybody was asked about, and a reload hands back
// the same Lot -- the same discipline as every other roll in the game.
//
// `social` is `Personality::social`, -100..+100. `rockIsIn` is whether
// today has a window at all; the caller knows and the Lot does not.
std::vector<Partner> WhoIsAround(const Rng& worldRng, int day,
                                 const std::vector<PartnerBond>& bonds,
                                 double social, bool rockIsIn,
                                 const PartnerDials& dials = PartnerDials{});

// How well you know somebody by name, or zero. The bonds list is short and
// keyed by name because partners are rebuilt from the world seed daily.
double RapportWith(const std::vector<PartnerBond>& bonds,
                   const std::string& name);

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

// Lines nobody at the Lot will touch: already claimed, or **visibly being
// worked by the player**.
//
// The second half is etiquette and it is also load-bearing. A per-day roll
// over a thirty-year career converges on certainty however small it is --
// measured, at a chance of 1 in 2000 per partner per day the Lot still took
// 27 of 30 open lines across six careers, and at the shipped 0.02 it took
// all of them, every time. The dial's stated intent, "the player should
// usually get the chance if they commit", is not reachable by making the
// number smaller. It is reachable by making commitment mean something.
//
// So: you keep what you are working on and you lose what you ignore, which
// is both how it actually goes and the only version where a project is
// worth committing to.
// Takes the ledgers rather than the PlayerState: DirtbagDay.h includes this
// header, so depending on it back would be a cycle, and the ledgers are the
// only part this needs.
std::vector<std::string> SpokenFor(const std::vector<ProjectMemory>& projects,
                                   const PartnerDials& dials = PartnerDials{});

// Whether this partner takes an open line today, and which. Returns the
// index into crag.lines, or -1. Rolls on its own named stream, so partners
// getting on with their lives cannot shift the player's attempts.
int PartnerTakesFirstAscent(const Rng& worldRng, const Partner& partner,
                            const Crag& crag,
                            const std::vector<std::string>& spokenFor,
                            int day,
                            const PartnerDials& dials = PartnerDials{});

// What they called the line they put up.
//
// Deterministic on (who, which line), so the guidebook says the same thing
// every time it is read and across a save. Not random and not sequential:
// two people are never handed the same name for the same rock, and the same
// person always calls the same line the same thing.
std::string NameTheirLine(const std::string& who, const CragLine& line);

// Somebody at the Lot put it up. Names it in their voice, puts their name on
// it, and takes it out of the projects.
//
// This is the half that was missing. `PartnerTakesFirstAscent` has recorded
// claims since the Lot was built, but only into the partner's own list — the
// book never learned, so the line stayed an open project with nobody's name
// on it and **the player could still walk up and claim the first ascent of
// something Dev did last spring**. Measured over ninety years: the Lot took
// five lines and the guidebook showed none of them.
//
// Returns false if the line was not theirs to take.
//
// Keyed on the name rather than the whole Partner because that is the only
// part that survives: partners are rebuilt from the world seed every day
// and only `PartnerBond` is saved. Replaying a bond's route keys through
// this is what puts the Lot's ascents back on the page after a reload.
bool TheyPutUpTheLine(CragLine& line, const std::string& who);

// Fold a career's remembered bonds into today's people, and back out again.
// The split is the point: everything derived stays derived.
void ApplyBonds(std::vector<Partner>& lot,
                const std::vector<PartnerBond>& bonds);
std::vector<PartnerBond> BondsFrom(const std::vector<Partner>& lot);

// "Margo has been on the arete all week." — what you can see from the fire,
// so a project you are losing is something you watched happen.
std::string LotTalk(const Partner& partner, const Crag& crag, int day);

}  // namespace dirtbag
