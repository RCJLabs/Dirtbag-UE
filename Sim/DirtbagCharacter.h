#pragma once

// Who your climber is before they have climbed anything.
//
// **Every climber in this game was identical at birth until this file.**
// `NewClimber` rolled five skills within +/-6 of fifty and a body type, and
// that was the whole of a person. The 2D game asks four questions before
// you touch rock -- where you came from, what kind of climber you are, what
// is wrong with you, and what you are like -- and then rolls two things
// behind your back that you find out by playing. That is the gap this
// closes (`notes/the-2d-audit.md`, Phase 7).
//
// Five pieces, and they do different jobs on purpose:
//
//   Archetype    what you are good at *now*. Chosen, visible, zero-sum.
//   Origin       where you came from. Chosen, visible, and it never stops
//                mattering -- each keeps one permanent perk in its own lane.
//   Flaw         what is wrong with you. Chosen, visible, and a real cost.
//   Temperament  what you are like. Chosen, and it bends four mechanics.
//   Talents      what you were born with. **Rolled, hidden, discovered.**
//
// The first four are a build. The fifth is the reason two climbers built
// the same way are still not the same climber.
//
// ## Zero-sum where it should be, and not where it should not
//
// **Archetypes redistribute and never add.** The five offsets of every
// archetype sum to zero, so choosing one is choosing a shape rather than a
// score, and no archetype is the strong one. This is the same rule the 2D
// game arrived at for its competition field after measuring the version
// that did not have it -- a set of offsets that is not zero-sum is a
// difficulty setting wearing a costume.
//
// **Origins do not, and that is the point.** Where you came from gives you
// money, or takes it; hands you a skill or a reputation you did not earn.
// A Trust-Fund Kid starts with $400 and the valley can smell it. They are
// not balanced against each other on one axis because they are not on one
// axis -- each holds a permanent multiplier in a **different lane** (shift
// pay, indoor gains, training gains, daily costs, shop prices, physio), so
// no two origins ever compete on the same number and none of them touches
// send odds. Borrowed wholesale from the 2D game's CHAR-15, which is where
// that rule was worked out.
//
// Engine-free like everything in Sim/.

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagRng.h"

namespace dirtbag {

// Which of the five. `Skills` is a struct of named doubles, which reads
// well everywhere except here: a talent is *about* a skill, so it needs the
// skill as a value. Kept in this file rather than added to DirtbagCore
// because this is the only place that wants it as a value.
enum class Skill { Power, Fingers, Technique, Endurance, Head };
constexpr int kSkillCount = 5;
const char* SkillName(Skill which);

// ---------------------------------------------------------------------
// Archetype -- what kind of climber you are on day one.
// ---------------------------------------------------------------------

enum class Archetype { Boulderer, RopeGun, Technician, AllRounder };
constexpr int kArchetypeCount = 4;

struct ArchetypeDef {
  const char* name;
  const char* blurb;
  // Offsets from kStartingSkill, in skill points. **These sum to zero for
  // every archetype** and a test enforces it. Six points is a bit under a
  // grade, so a Boulderer's power lead over their endurance is about a
  // grade and a half of shape.
  double power, fingers, technique, endurance, head;
};
const ArchetypeDef& Describe(Archetype a);
const char* ArchetypeName(Archetype a);

// ---------------------------------------------------------------------
// Origin -- where you came from, and it never stops mattering.
// ---------------------------------------------------------------------

enum class Origin {
  SoldItAll,     // quit the job, bought the van
  GymRat,        // raised on plastic
  DesertLocal,   // sandstone at dawn, no money
  ExGymnast,     // a motor, and no head for a fall
  LateBloomer,   // started at thirty-two with savings and no illusions
  TrustFund,     // money is no object and the scene can tell
};
constexpr int kOriginCount = 6;

struct OriginDef {
  const char* name;
  const char* blurb;
  const char* perk;   // said in the player's words, because it is a promise

  // Skill offsets. Unlike archetypes these do **not** sum to zero -- an
  // origin is a life, not a build, and a Desert Local really is better at
  // rock and worse at pulling hard than somebody who quit an office job.
  double power = 0.0, fingers = 0.0, technique = 0.0, endurance = 0.0,
         head = 0.0;

  double cash = 0.0;      // what you arrive with
  double standing = 0.0;  // and what the valley already thinks
  int agePlus = 0;        // Late Bloomer starts ten years down the road

  // The permanent perk, and **exactly one of these is ever not 1.0**. Each
  // origin owns a different lane so no two can be compared on one number.
  double shiftPay = 1.0;      // every shift pays this much more
  double indoorGain = 1.0;    // skill from a session at the gym
  double trainingGain = 1.0;  // skill from deliberate training
  double dailyCost = 1.0;     // what living costs you
  double shopPrice = 1.0;     // what the counter charges
  double physioPrice = 1.0;   // what getting fixed costs

  // Where your temperament starts before you pick one. An origin leans you
  // without deciding for you.
  double discipline = 0.0, boldness = 0.0, social = 0.0, purism = 0.0;
};
const OriginDef& Describe(Origin o);
const char* OriginName(Origin o);

// ---------------------------------------------------------------------
// Flaw -- the one you chose, knowing.
// ---------------------------------------------------------------------

enum class Flaw {
  Gumby,          // footwork trains at half rate
  HardGainer,     // muscle is slow
  FairWeather,    // you only really train when you feel good
  HappyFeet,      // shaky feet, and technical lines know it
  TweakyFingers,  // pulleys made of paper
};
constexpr int kFlawCount = 5;

struct FlawDef {
  const char* name;
  const char* blurb;
};
const FlawDef& Describe(Flaw f);
const char* FlawName(Flaw f);

// ---------------------------------------------------------------------
// Temperament -- what you are like, as four numbers.
// ---------------------------------------------------------------------

// Each axis runs -100..+100 with 0 as neither. They bend four different
// mechanics and never the same one twice:
//
//   discipline  how much a session teaches you
//   boldness    what you will commit to above the last piece
//   social      whether people turn up, and what solo is worth
//   purism      outdoor reputation against what a shift pays
struct Personality {
  double discipline = 0.0;
  double boldness = 0.0;
  double social = 0.0;
  double purism = 0.0;
};
constexpr double kPersonalityMax = 100.0;

enum class Temperament { Purist, SendOrBust, Lifer, Influencer };
constexpr int kTemperamentCount = 4;

struct TemperamentDef {
  const char* name;
  const char* blurb;
  Personality p;
};
const TemperamentDef& Describe(Temperament t);
const char* TemperamentName(Temperament t);

// ---------------------------------------------------------------------
// Talents -- the two you did not choose.
// ---------------------------------------------------------------------

enum class Talent {
  None = 0,
  // gifts
  NaturalCrimper,   // fingers come fast
  Explosive,        // power piles on
  SlowTwitchEngine, // a bottomless tank
  QuietFeet,        // technique clicks
  IceInTheVeins,    // an unflappable head
  BomberTendons,    // cabled fingers that shrug off load
  // anti-talents
  GlassTendons,     // paper pulleys: slow, and they tweak
  NoPop,            // strength is a grind
  NoEngine,         // you pump out
  Stiff,            // movement fights you
  Skittish,         // fear runs hot
};
constexpr int kTalentCount = 12;  // including None

struct TalentDef {
  const char* name;
  const char* desc;
  Skill skill;
  bool gift;
  double gain;    // multiplier on skill gain in that lane
  double injury;  // multiplier on injury risk; 1.0 for most
};
const TalentDef& Describe(Talent t);
const char* TalentName(Talent t);

// ---------------------------------------------------------------------
// The person
// ---------------------------------------------------------------------

// What you picked. Everything here is a choice; nothing is rolled.
struct Build {
  Archetype archetype = Archetype::AllRounder;
  Origin origin = Origin::SoldItAll;
  Flaw flaw = Flaw::Gumby;
  Temperament temperament = Temperament::Lifer;
};

// What you are, which is what you picked plus what you were born with.
struct Character {
  // **Has anybody actually been built yet.** Every effect function in this
  // file returns exactly neutral while this is false, and that is not a
  // convenience -- it is the invariant that keeps this system from silently
  // rebalancing the whole game.
  //
  // `Build` has to default to *something*, and whatever it defaults to
  // carries that origin's perk and that flaw's cost. The first version of
  // this file defaulted to an All-Rounder who Sold It All with Gumby, and
  // the measured result was **technique training at half rate and every
  // shift paying 12% more, in every existing caller, the probe and the
  // golden vectors included** -- with **all 75,027 checks still passing**,
  // because the training tests are orderings ("the grinder ends stronger
  // than the cruiser") and halving both sides preserves an ordering. Every
  // balance note in this repo would have been measured on a game that no
  // longer existed.
  //
  // So: an unbuilt character is nobody, and nobody gets no modifiers.
  // `MakeCharacter` is the only thing that sets this, and a test pins every
  // effect function at exactly neutral without it.
  bool built = false;

  Build build;
  Personality personality;

  // One gift and one anti-talent, on **different skills**, rolled at birth
  // and not shown. They are live from the first session -- what is hidden
  // is the knowing, not the effect, which is the honest model: nobody is
  // told they have good tendons, they find out over ten years.
  Talent gift = Talent::None;
  Talent antiTalent = Talent::None;
  bool giftKnown = false;
  bool antiKnown = false;

  // Sessions that worked each skill. A talent surfaces once you have put
  // enough into its lane to feel it, which is why a gift in a skill you
  // never train stays a secret forever.
  double reps[kSkillCount] = {0.0, 0.0, 0.0, 0.0, 0.0};

  // What you arrived with, kept because the origin's money is a fact about
  // the career rather than a one-off deposit -- the handover reads it.
  double startingCash = 0.0;
  int agePlus = 0;
};

struct CharacterDials {
  // Skill points per point of archetype offset. One, because the offsets
  // are already written in skill points -- the dial exists so the whole
  // spread can be widened or flattened without editing twelve numbers.
  double archetypeScale = 1.0;

  // (The +/-6 spread of luck on top of the build lives in `NewClimber`,
  // where it always did. A copy of it here would be a second number nobody
  // updates -- the dial checker caught exactly that on the first draft of
  // this struct.)

  // Sessions in a skill's lane before a talent in it surfaces.
  //
  // **Forty, and it is a season and a bit.** Small enough that a career
  // built around a lane finds out what it was born with while the finding
  // out still changes decisions, large enough that it is never day three.
  // A talent in a lane you neglect stays hidden for good, which is the
  // point: you learn what you are by what you do.
  double repsToSurface = 40.0;

  // How hard a flaw bites, as the fraction of normal gain left.
  double gumbyTechnique = 0.5;   // footwork at half rate
  double hardGainerPull = 0.65;  // power and fingers
  double fairWeatherTired = 0.4; // what a tired session is worth
  double fairWeatherBelow = 0.5; // ...and what counts as tired, in psyche
  double happyFeetOdds = 0.10;   // straight off technical send odds
  double tweakyInjury = 1.8;     // pulleys, multiplied

  // What a full axis of personality is worth in its own lane. Each is the
  // value at +/-100; the effect is linear between.
  double disciplineTraining = 0.20;  // +/-20% skill from a session
  double purismShiftPay = 0.15;      // a purist works for less
};

// Roll the two talents. Deterministic from the rng, always one gift and one
// anti-talent, and **never both on the same skill** -- a climber whose
// fingers both come fast and come slow is a wash, which is not a character.
void RollTalents(Character& c, const Rng& rng);

// A climber built rather than stamped out. Applies the archetype's shape,
// the origin's life, the temperament's disposition and the roll on top, and
// rolls the hidden talents.
//
// This is what `NewClimber` should have been. `NewClimber` still exists and
// still does what it did -- it is what the harness and the probe use when
// the question is not about identity -- and this wraps it.
Climber MakeClimber(const Build& build, const Rng& rng,
                    const CharacterDials& dials = CharacterDials{});

// The same, for the parts that do not live on `Climber`.
Character MakeCharacter(const Build& build, const Rng& rng,
                        const CharacterDials& dials = CharacterDials{});

// ---------------------------------------------------------------------
// What it all does. Every one of these is a multiplier on a number some
// other module already computes, which is why this file can be additive.
// ---------------------------------------------------------------------

// Skill gain in one lane, from a session or a training block.
// `tiredPsyche` is the climber's psyche at the time, which only the
// Fair-Weather Trainer reads.
double SkillGainMultiplier(const Character& c, Skill lane, bool indoor,
                           bool deliberateTraining, double psyche,
                           const CharacterDials& dials = CharacterDials{});

// Straight off send odds, and **only for a flaw** -- nothing else in this
// file is allowed to touch odds, which is what keeps origins from being a
// difficulty setting.
double OddsPenalty(const Character& c, RouteType type,
                   const CharacterDials& dials = CharacterDials{});

// Injury risk, multiplied. Tendon talents and one flaw.
double InjuryRiskMultiplier(const Character& c,
                            const CharacterDials& dials = CharacterDials{});

// The origin lanes, each read by exactly one system.
double ShiftPayMultiplier(const Character& c,
                          const CharacterDials& dials = CharacterDials{});
double DailyCostMultiplier(const Character& c);
double ShopPriceMultiplier(const Character& c);
double PhysioPriceMultiplier(const Character& c);

// **Two of the four personality axes land in this phase and two do not**,
// and that is recorded here rather than shipped as three multipliers
// nothing multiplies. `discipline` bends what a session teaches you and
// `purism` bends what a shift pays, and both are wired. `social` (whether
// people turn up) and `boldness` (what you commit to above the last piece)
// want seams in the partner and sport models that this phase does not
// touch, so their readers arrive with those systems rather than sitting
// here unread. The axes are still stored, still saved and still drift-ready
// -- what is missing is two consumers, not two numbers.

// Record a session's work in a lane, and surface a talent if this is the
// session that makes it obvious. Returns the talent that just surfaced, or
// None -- so the caller can say it out loud, once, in the player's words.
Talent WorkedOn(Character& c, Skill lane, double amount,
                const CharacterDials& dials = CharacterDials{});

// The line the game says on the day a talent stops being a secret. Empty
// for `None`, so a caller can say it without asking twice.
std::string TalentSurfaced(Talent t);

// One sentence saying who this climber is. The Phase 7 gate asks that a
// player can answer that without reading a stat line; this is the game's
// own answer, and if it reads badly the build is not legible enough.
std::string WhoYouAre(const Character& c);

}  // namespace dirtbag
