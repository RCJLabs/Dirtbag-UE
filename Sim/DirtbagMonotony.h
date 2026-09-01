#pragma once

// Repeating anything dulls it -- `CHAR-7` and `PSY-2`, ported from the 2D
// source. Two scales of one idea, which is why they share a file.
//
// ## The training one (`CHAR-7`)
//
// Per-skill monotony. Feed a skill the same stimulus and its gains taper to
// a plateau; feed it a novel one and the monotony sheds -- and breaking a
// *real* plateau pays a breakthrough on top. Rest sheds a little every
// night and a deload sheds a lot, which is the only thing in this port that
// makes periodisation worth anything.
//
// This is what `Sim/DirtbagDay.cpp`'s `teach` lambda has been missing.
// Training there has a ceiling (`headroom`) and no plateau: the same
// hangboard session, every day, forever, paid exactly the same as a varied
// year. The ceiling says *you cannot get much stronger*; monotony says
// *not like this*, and they are different sentences.
//
// ## The venue one (`PSY-2`)
//
// The source's note on this is the most useful paragraph in its file, and
// it is a measurement: psyche *"used to be a closed loop -- sending refilled
// it, and sending is what you do, so it pinned to the ceiling. Measured over
// 400 days it averaged 95/100 and spent ZERO days under the penalty line: a
// whole resource bar that could never once bite."*
//
// So stoke gets a staleness term. Climb the same venue day after day and
// everything the *climbing* gives you dulls -- sends, arriving somewhere,
// the sleep afterwards. Novelty pays: a crag you have never walked into, a
// grade you have never touched.
//
// **And it never touches hobbies, the fire or people.** Those are the cure.
// Dulling them would make a bored climber unfixable, which is the difference
// between a consequence and a trap -- the same reason there is a floor under
// the whole term.
//
// Engine-free like everything in Sim/.

#include <string>

#include "DirtbagCharacter.h"
#include "DirtbagCore.h"

namespace dirtbag {

struct MonotonyDials {
  // --- CHAR-7, the training plateau ------------------------------------
  // Monotony added by repeating a stimulus, and shed by a novel one.
  // Novelty sheds four and a half times what repetition adds, so a plateau
  // takes a fortnight of sameness to build and one different session to
  // break -- which is how it actually works.
  double rise = 0.10;
  double novelDrop = 0.45;
  // Gains at full monotony. Half, and not worse: a plateau is a plateau,
  // not a punishment.
  double penalty = 0.50;
  // Monotony at or above which breaking it pays, and what it pays. **The
  // threshold is what makes it a breakthrough rather than a bonus for
  // variety** -- shed a plateau you had not really built and nothing
  // happens.
  double breakthroughAt = 0.55;
  double breakthroughBonus = 0.60;
  // Shed per night by simply not doing it, and the extra a deload sheds.
  double restShed = 0.05;
  double deloadShed = 0.25;

  // --- PSY-2, the venue ------------------------------------------------
  // Days of sameness before stoke is as flat as it gets, and how fast it
  // moves each way. **Variety fixes it about three times faster than
  // sameness breaks it**, which is what keeps one good trip worth taking.
  double staleMax = 10.0;
  double staleRise = 1.0;
  double staleFall = 3.0;
  // Above one, so the first few repeat days barely register and the long
  // grind hurts a lot. A linear version of this is a slow leak nobody
  // notices; this one has a shape you can feel arriving.
  double stalePower = 1.4;
  // **The floor, and it is not optional.** Without it a player who simply
  // prefers training indoors craters to single-digit psyche and is locked
  // out below the penalty line entirely -- which is punishment for a
  // playstyle rather than consequence for a choice. At the floor a grinder
  // sags and notices; they are never stranded.
  double staleFloor = 0.34;
};

// --- CHAR-7 --------------------------------------------------------------

// Per-skill monotony, and the stimulus each skill last saw. The stimulus is
// a string because it has to be comparable and nothing else -- a route
// type, an exercise name, a crag. Anything that is "the same thing again".
struct Monotony {
  double level[kSkillCount] = {0.0, 0.0, 0.0, 0.0, 0.0};
  std::string lastStimulus[kSkillCount];
};

struct Stepped {
  double gainMultiplier = 1.0;
  bool breakthrough = false;
};

// Step one skill's monotony for one session and say what it is worth.
// Writes the new level and the stimulus, so the caller does not have to.
Stepped Feed(Monotony& monotony, Skill lane, const std::string& stimulus,
             const MonotonyDials& dials = MonotonyDials{});

// A night's shedding. `deloading` for a declared rest day or a deload
// block -- the only reason periodisation pays.
void SleptOn(Monotony& monotony, bool deloading,
             const MonotonyDials& dials = MonotonyDials{});

// What a plateau reads like. Empty unless something is genuinely stuck.
std::string PlateauLine(const Monotony& monotony,
                        const MonotonyDials& dials = MonotonyDials{});

// --- PSY-2 ---------------------------------------------------------------

// How long you have been climbing the same place, and where that is.
struct VenueStaleness {
  double days = 0.0;
  std::string venue;
};

// A day at a venue. Same place as yesterday and it rises; anywhere else and
// it falls three times as fast, and the counter moves to the new place.
void ClimbedAt(VenueStaleness& stale, const std::string& venue,
               const MonotonyDials& dials = MonotonyDials{});

// **How much of what the climbing gives you still reaches you.** Multiply
// climbing-derived stoke by this and nothing else -- see the header for why
// hobbies, the fire and people are exempt.
double Freshness(const VenueStaleness& stale,
                 const MonotonyDials& dials = MonotonyDials{});

// What the sameness reads like. Empty until it is worth saying.
std::string StaleLine(const VenueStaleness& stale,
                      const MonotonyDials& dials = MonotonyDials{});

}  // namespace dirtbag
