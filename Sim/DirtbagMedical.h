#pragma once

// Getting hurt, and what you do about it.
//
// `DirtbagBody` decides *that* you are hurt — load, tweaks, aggravation,
// a severity and a count of days. That was Phase 3's whole medical system
// and it is a wait with a price tag on it: you are out for eleven days,
// physio buys six of them back, and there is nothing else to decide.
//
// This is the rest of it, and the shape of it is one sentence: **an injury
// hides its grade until you pay to look at it.** Everything else follows
// from that.
//
// ## Why the fog is the mechanic
//
// A climber with a sore finger does not know whether it is three weeks or
// three months. They know it hurts. Every decision they make — rest it,
// see somebody, take the shot, book the surgery, pull on anyway — is made
// without the number, and **the number is exactly what would make the
// decision easy.** So the sim keeps the severity and hands the player a
// description, and buying the number is itself the first decision.
//
// That is what turns Phase 3's wait into a decision with a wrong answer,
// which is this phase's first gate. The wrong answers are real and they
// are all available: skip the diagnosis and guess, take the cortisone
// because it works *now*, come back at the first stage that does not hurt.
//
// ## The three clocks
//
// - **The comeback** is staged — rest, mobility, a graded return — and it
//   advances because you say so, not because a timer ran out. Advancing
//   before the stage is done is the gamble.
// - **The joint** carries what you did to it. Cortisone works immediately
//   and degrades the joint permanently; scars from healed injuries flare
//   under load for the rest of a career. Neither shows up this season.
// - **The policy** you either bought before you needed it or did not.
//
// Engine-free like everything in Sim/.

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagRng.h"

namespace dirtbag {

// What you know about what is wrong with you.
enum class Diagnosis {
  None = 0,   // something hurts
  Guessed,    // somebody who knows had a look and a feel
  Scanned,    // a machine looked at it, and now there is a number
};
constexpr int kDiagnosisCount = 3;
// unwired-ok: the engine mirrors Diagnosis as a UENUM whose DisplayName
// carries the same words, so it never needs the C string. `ComebackName`
// is wired -- the comeback is the one of the three the player is told
// about in a sentence rather than shown in a dropdown.
const char* DiagnosisName(Diagnosis d);

// What you are doing about it.
enum class Treatment {
  Rest = 0,
  Physio,
  Cortisone,
  Surgery,
};
constexpr int kTreatmentCount = 4;
// unwired-ok: mirrored as a UENUM, same as Diagnosis above.
const char* TreatmentName(Treatment t);

// **The comeback is staged, not timed.** Each stage has a length derived
// from severity, and moving to the next one is something you decide.
enum class Comeback {
  Clear = 0,     // nothing wrong
  Resting,       // it is acute; you are not doing anything
  Mobility,      // moving it, not loading it
  GradedReturn,  // climbing, carefully, below your level
};
constexpr int kComebackCount = 4;
const char* ComebackName(Comeback c);

// A healed injury that did not heal clean. **Scars flare under load for
// the rest of a career** -- which is the difference between a career that
// got hurt and a career that got hurt badly.
struct Scar {
  InjuryKind kind = InjuryKind::Pulley;
  // How much it raised the odds of the same thing happening again, **on
  // the day it was made**. What it is worth now is `ScarWeight`, derived
  // from this and the date.
  //
  // Derived rather than decayed in place, and the difference is not
  // academic: the first version faded the weight nightly by reading the
  // weight it had just written, so a scar decayed geometrically -- a
  // thousand multiplications over three years -- and the floor, being a
  // fraction of the current value, collapsed with it. Same lesson as the
  // ranking two phases ago: **a value that ages is a function of the
  // date, not a field you keep editing.**
  double weight = 0.0;
  int fromDay = 0;
};

struct MedicalDials {
  // --- knowing ---------------------------------------------------------
  // **What the number costs.** A physio's opinion is a week of food; the
  // scan is most of a month. Both are real money against a career that
  // lives on $420 and a shift, which is the point -- the diagnosis has to
  // be something you can decide not to afford.
  double guessCost = 60.0;
  double scanCost = 340.0;

  // How wrong a guess can be, in severity units. A physio's hands are good
  // and not perfect; the scan is exact. **This is the whole fog**: treat a
  // 0.8 you were told was a 0.55 and you will come back too early.
  double guessError = 0.18;

  // --- treating --------------------------------------------------------
  // The shot. **Works now and costs you forever.** It ends the acute
  // stage outright -- you are climbing this week -- and puts a permanent
  // mark on the joint that raises every future roll on it.
  double cortisoneCost = 180.0;
  double cortisoneJointCost = 0.22;

  // Surgery: the long way round, and the only thing that takes damage
  // *off* a joint. Needs a scan -- nobody operates on a guess -- and only
  // above the line, because you cannot have an operation for a strain.
  double surgeryCost = 2200.0;
  double surgeryNeedsSeverity = 0.62;
  double surgeryDays = 90.0;
  // What it gives back: the joint comes off the books, and the scar it
  // leaves is a fraction of the one the injury would have.
  double surgeryJointRepair = 0.85;
  double surgeryScar = 0.10;

  // --- the comeback ----------------------------------------------------
  // How the injury's days split across the three stages. They sum to one;
  // a graded return is nearly half of it, because that is the half
  // everybody skips.
  double restingShare = 0.34;
  double mobilityShare = 0.22;
  double returnShare = 0.44;

  // **Coming back early.** The chance of setting yourself back, per day
  // early, and what it costs when it lands. Rolled once when you advance,
  // so it is a decision and not a slow leak.
  double earlyChancePerDay = 0.055;
  double earlySeverityCost = 0.20;

  // What each stage costs you while you are in it, as a fraction of the
  // full injury penalty. A graded return is climbing -- badly.
  double restingPenalty = 1.0;
  double mobilityPenalty = 0.7;
  double returnPenalty = 0.35;

  // --- what it leaves -------------------------------------------------
  // The scar a healed injury leaves, scaled by how bad it was and by
  // whether you did it properly. **Rushing it is what scars you** -- a
  // clean comeback off a bad injury leaves less than a rushed one off a
  // mild.
  double scarFromSeverity = 0.30;
  double scarFromRushing = 0.45;
  // **What a joint's whole history is worth on a future roll**, per unit
  // of wear -- and since wear is clamped at one, this is also the ceiling:
  // the worst joint in the game is 2.6 times as likely to go as a clean
  // one, and no worse.
  //
  // Cortisone and scars weighed separately and uncapped first, and it ran
  // away: measured at thirty years, an impatient career reached a risk
  // multiplier of **17.9 with 111 injuries** -- more risk, more injuries,
  // more scars, more risk. A death spiral is not a consequence, it is an
  // absence of one, because past a certain point nothing the player does
  // matters. One formula and one cap.
  double riskPerWear = 1.6;
  // Scars fade, slowly and never to nothing. Per year.
  double scarFadePerYear = 0.06;
  double scarFloor = 0.25;

  // --- insurance -------------------------------------------------------
  // **A bet you place before you know.** The premium is a bill like any
  // other and it lands whether or not you are hurt; what it buys is most
  // of the bill on the day you are.
  // **Sized against what a career can actually lose.** A dirtbag earns
  // about $7,200 a year and ends thirty years of it with $159 in the tin,
  // so cash-on-hand is the binding constraint and not lifetime income.
  // Thirty years of premiums at this number is about $6,250 -- measured
  // against claims that run from $816 on a lucky body to $8,976 on an
  // unlucky one, which is the bet: **right on the career that got hurt a
  // lot, and a slow expensive mistake on the one that did not.**
  //
  // It was $26 first, and that is $20,332 over thirty years against a
  // maximum measured claim of $8,976 -- never right, in any seed, which is
  // not a bet, it is a tax with a story.
  double premium = 8.0;
  int premiumEveryDays = 14;

  // **What it pays of the bill, and the number that decides whether this
  // system has a customer at all.**
  //
  // At 0.80 it had none, measured. A surgery lists at $2,200, so the copay
  // was $440 -- and a climbing-first career's *highest balance in thirty
  // years* is $474, usually holding a couple of hundred. So the insured
  // poor climber scanned every injury and **still never once had the
  // operation**: 0.00 surgeries across eight seeds, with $6,256 of
  // premiums paid and $612 of claims back.
  //
  // Meanwhile a career that saves affords the whole $2,200 unaided and has
  // no use for cover. **The people who needed it could not use it and the
  // people who could use it did not need it**, which is a system with
  // nobody in the middle.
  //
  // Swept: the cliff is between a $330 copay (0.00 surgeries) and a $220
  // one (1.50). 0.90 is the least change that makes the mechanic exist,
  // and $220 is still a real decision for somebody who ends the year on
  // two hundred. After it, insurance is **the only way a climbing-first
  // career is ever repaired** -- uninsured 0.00 surgeries, insured 1.50 --
  // which is what the note at the top of this block always said it was
  // for. It stays a losing bet on average, at $0.61 back per dollar, and
  // that is what insurance is.
  double covers = 0.90;
  // You cannot buy it while you are already hurt. Everybody tries.
  bool refuseWhenHurt = true;
  // And there is a wait before it pays, so buying it the week before a
  // planned surgery is not a strategy.
  int waitingDays = 30;

  // --- being poor ------------------------------------------------------
  // **The undertreated path, and it is reached by being broke rather than
  // by choosing it.** An injury that runs its course with no diagnosis and
  // no treatment heals -- badly. It takes longer and it scars worse, and
  // nothing about it is a decision, which is exactly what being poor in a
  // body is like.
  double untreatedDaysMult = 1.35;
  double untreatedScar = 0.30;
};

// Everything Phase 10 knows. Lives on the player rather than the climber,
// because `Climber` is what a body *is* and this is what happened to it.
struct Medical {
  Diagnosis diagnosis = Diagnosis::None;
  Treatment treatment = Treatment::Rest;
  Comeback stage = Comeback::Clear;

  // When the current stage started, and what the sim thinks it is worth.
  int stageStarted = 0;
  int stageDays = 0;

  // **What you were told**, which is not always what is true. Zero when
  // nobody has looked.
  double toldSeverity = 0.0;

  // Per-joint permanent damage, parallel to InjuryKind. Cortisone puts it
  // on; surgery takes most of it off; nothing else touches it.
  double joints[kInjuryKindCount] = {0.0, 0.0, 0.0, 0.0};
  int shots[kInjuryKindCount] = {0, 0, 0, 0};

  std::vector<Scar> scars;

  // The policy.
  bool insured = false;
  int insuredOnDay = 0;
  double premiumsPaid = 0.0;
  double claimsPaid = 0.0;

  // Career counters, for the write-up and for the probe.
  int diagnoses = 0;
  int shotsTaken = 0;
  int surgeries = 0;
  int rushedComebacks = 0;
  int untreatedInjuries = 0;

  // Whether this injury ever got any care at all. Reset when a new one
  // starts; read when it heals.
  bool treatedThisTime = false;
};

// --- knowing -----------------------------------------------------------

// Pay to look. Returns false if there is nothing wrong, you already know
// this much, or you cannot pay. `Guessed` gives you a number that is close;
// `Scanned` gives you the number.
bool Diagnose(Medical& med, const Climber& climber, double& cash,
              Diagnosis tier, const Rng& worldRng, int day,
              const MedicalDials& dials = MedicalDials{});

// What the diagnosis cost, after insurance.
double BillFor(const Medical& med, double list, int day,
               const MedicalDials& dials = MedicalDials{});

// --- treating ----------------------------------------------------------

// The shot. Ends the acute stage now and marks the joint forever.
bool TakeTheShot(Medical& med, Climber& climber, double& cash, int day,
                 const MedicalDials& dials = MedicalDials{});

// The operation. Needs a scan and a bad enough injury; costs the money and
// most of a season, and takes the joint off the books.
bool HaveSurgery(Medical& med, Climber& climber, double& cash, int day,
                 const MedicalDials& dials = MedicalDials{});

// --- the comeback ------------------------------------------------------

// Start the clock on a fresh injury. Called when one lands.
void StartComeback(Medical& med, Climber& climber, int day,
                   const MedicalDials& dials = MedicalDials{});

// Is the current stage done? **The sim always knows; the player only knows
// when they have paid to.** `Told` is what the player is allowed to see.
bool StageIsDone(const Medical& med, int day);
int DaysLeftInStage(const Medical& med, int day);

// Move to the next stage. Rolls for a setback if you are early -- the
// chance rises with how early, and it is rolled once, so this is a
// decision and not a slow leak. Returns true if it set you back.
bool NextStage(Medical& med, Climber& climber, const Rng& worldRng, int day,
               const MedicalDials& dials = MedicalDials{});

// Finish an injury: clear it, and work out what it leaves behind. **One
// function, because there are two ways through the last stage** -- waiting
// it out and choosing to end it -- and the first version healed on only
// one of them, so the other looped forever.
struct MedicalNight;
MedicalNight FinishInjury(Medical& med, Climber& climber, int day,
                          const MedicalDials& dials = MedicalDials{});

// One night. Counts the stage down, keeps `daysLeft` honest for everything
// that reads it, and heals the injury when the last stage runs out.
struct MedicalNight {
  bool healed = false;
  bool scarred = false;
  double scarWeight = 0.0;
  bool untreated = false;
  std::string news;
};

// How many days of the whole comeback are left, across every stage. What
// `Injury::daysLeft` is kept equal to, so the HUD, the sponsor's
// days-hurt count and everything else that reads it still read the truth.
int DaysLeftInComeback(const Medical& med, const Climber& climber, int day,
                       const MedicalDials& dials = MedicalDials{});

MedicalNight MedicalDay(Medical& med, Climber& climber, const Rng& worldRng,
                        int day, const MedicalDials& dials = MedicalDials{});

// What the current stage costs you, as a multiplier on the injury penalty.
double StagePenalty(const Medical& med,
                    const MedicalDials& dials = MedicalDials{});

// --- what it leaves ----------------------------------------------------

// What a joint carries, 0..1: the cortisone in it plus the scars on it.
// **What `AttemptInput::jointDamage` wants** -- the wear, as against
// `JointRisk`, which is the same history read as a multiplier on getting
// hurt again.
double JointWear(const Medical& med, InjuryKind kind, int today,
                 const MedicalDials& dials = MedicalDials{});

// What a scar is worth today. Fades slowly and never to nothing.
double ScarWeight(const Scar& scar, int today,
                  const MedicalDials& dials = MedicalDials{});

// How much a joint's history multiplies a future injury roll on it. One,
// for a joint nothing has happened to. **Takes the day**, because half of
// what it reads is age.
double JointRisk(const Medical& med, InjuryKind kind, int today,
                 const MedicalDials& dials = MedicalDials{});

// The same, read across every joint -- what `TweakSomething`'s `risk`
// argument wants when the hold could hurt more than one thing.
double BodyRisk(const Medical& med, int today,
                const MedicalDials& dials = MedicalDials{});

// --- insurance ---------------------------------------------------------

// Take out a policy. **Refused while you are hurt**, which everybody
// tries -- the climber is an argument rather than the caller's promise,
// because a rule enforced by convention is a rule that is not enforced.
bool BuyInsurance(Medical& med, const Climber& climber, int day,
                  const MedicalDials& dials = MedicalDials{});
void CancelInsurance(Medical& med);

// Is it actually paying yet?
bool CoverIsLive(const Medical& med, int day,
                 const MedicalDials& dials = MedicalDials{});

// The premium, on its own clock. Lands whether or not you are hurt, which
// is what makes it a bet.
bool InsuranceDay(Medical& med, double& cash, double& owed, int day,
                  const MedicalDials& dials = MedicalDials{});

// --- what it reads like ------------------------------------------------

// What you can say about it, which depends on what you have paid to know.
// **Undiagnosed is deliberately vague** -- that is the mechanic, not a
// missing string.
std::string MedicalText(const Medical& med, const Climber& climber, int day,
                        const MedicalDials& dials = MedicalDials{});

// What the joints have been through. Empty when nothing has.
std::string HistoryText(const Medical& med, int today,
                        const MedicalDials& dials = MedicalDials{});

}  // namespace dirtbag
