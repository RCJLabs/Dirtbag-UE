// The youth team.
//
// `GYM-2` wrote the hook and left it hanging on purpose. The second stage
// of Piper's arc ends: *"Her mother asked if the youth team has a waitlist.
// There is no youth team. There might have to be."* That line has been in
// this game since `notes/who-the-gym-is-for.md` shipped, with nothing
// behind it. This is what is behind it.
//
// `GYM-8`, ported from the 2D source.
//
// ## Why it is worth more here than it is there
//
// The source calls this *"the only system in the game where the thing you
// build outlives you in somebody else's name"*: a kid you coached from
// eleven turns up in the national field as a rookie, and from then on they
// are one of the names you are chasing.
//
// This port's national field is a fixed table rather than a roster, so that
// promotion cannot be ported as written. It does not need to be.
// `DirtbagRival.h` has carried this comment since long before the gym
// existed:
//
//     // Somebody steps up. Sometimes it is a kid from the gym.
//     Rival Succeed(...)
//
// So a graduate goes into `Youth::steppingUp`, and the next time the rival
// generation turns over, the successor has their name. The port foretold
// this system before it had one.
//
// ## The shape, which is the gym's own
//
// Deliberately borrowed from the building it sits in: **you can run it with
// your own hours, or you can pay somebody**, and the second is worse for
// the kids and better for your week -- the same trade `SetHandsOff` makes
// with the business, one floor down.
//
// ## The numbers are re-derived, not copied
//
// The source's per-session gains are tuned against a year that is eighteen
// days long. This port's year is three hundred and sixty-five. Copied
// across, a squad would max out in about six months against a five-year age
// gate, **so the birthday would bind in every single case** -- which is
// precisely the bug the source's own comment records tuning away from.
//
// What is ported is the intent, stated there in one sentence: *"the
// CLIMBING is the gate when you don't know what you're doing and the
// BIRTHDAY is the gate when you do."* See `YouthDials` for the arithmetic.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagAge.h"
#include "DirtbagRng.h"

namespace dirtbag {

// One of them. Fourteen names in the pool and a squad holds four, so a
// career meets most of them and never all at once.
struct YouthKid {
  std::string name;
  std::string tag;
  // 0 to `levelMax`. **Not a grade** -- it is how far along they are, and
  // `YouthBand` is what it reads as on the wall.
  double level = 0.0;
  int ageAtJoin = 11;
  int joinedDay = 0;
};

struct Graduate {
  std::string name;
  int day = 0;
  int age = 0;
};

struct Youth {
  bool going = false;
  int foundedDay = 0;
  std::vector<YouthKid> kids;

  // Yours, or somebody's you pay.
  bool youCoach = true;
  std::string coachName;

  int sessions = 0;
  int lastSessionDay = -99;

  // **Coaching craft lives here rather than on the career**, because this
  // port has no coaching roster to share it with yet. In the source it is
  // the Cave job's second axis and the youth team is one of three things
  // that grow it. When `COACH-*` lands, this moves out and nothing else
  // about the file changes.
  double craft = 0.0;

  std::vector<Graduate> graduated;

  // **Waiting to step up.** A graduate's name, held until the rival
  // generation turns over -- see the header.
  std::vector<std::string> steppingUp;
};

struct YouthDials {
  // Mats, kit, insurance, and a mountain of paperwork.
  double foundCost = 1200.0;

  // Squad size. A graduation opens a place and somebody fills it, so the
  // squad is always four and never the same four.
  int squadSize = 4;

  // Rungs from "here for the pizza" to a national junior result.
  double levelMax = 10.0;

  // **And you cannot rush this bit however good they are.** Neither gate
  // substitutes for the other: ready means the climbing *and* the birthday.
  int gradAge = 16;

  double sessionHours = 2.0;
  double sessionEnergy = 12.0;

  // Days between sessions. A squad trains; it does not grind.
  int sessionGapDays = 3;

  // --- how fast they come on ---------------------------------------------
  // **Re-derived for a 365-day year**, from the source's stated intent
  // rather than its numbers. Target: a kid is a seven-year project when you
  // do not know what you are doing and a four-year one when you do, and
  // four years is the floor.
  //
  // At one session per three days that is 852 sessions over seven years and
  // 487 over four. Every fifth session is a trip worth two and a half times
  // a training night, so the average per session is `gainBase +
  // compBoost / 5`. Solving both ends:
  //
  //   gainBase     + compBoost/5              = 10 / 852
  //   gainBase + 12*gainPerCraft + compBoost/5 = 10 / 487
  //
  // which is what these four numbers are. Copying the source's 0.16 would
  // have maxed a squad out in six months against a five-year age gate.
  double gainBase = 0.0078;
  double gainPerCraft = 0.00073;

  // A paid coach, working while you are elsewhere. Steady. **Not you** --
  // the source runs them at 0.625 of an uncoached session, which here is
  // about nine years a kid instead of seven.
  double gainHired = 0.0049;

  // A weekend of competing is worth more than a night of drills.
  double compBoost = 0.0196;
  int compEvery = 5;
  // Named for the trip and not the comp, because `FloorDials` has a
  // `compStanding` of its own for comp night and they are different numbers
  // meaning different things -- `check-dials.py` caught them sharing a name
  // and treated it, correctly, as an unpinned mirror.
  //
  // **Only when you took them.** A hired coach's weekend is not your
  // weekend and does not read as yours in the scene.
  double tripStanding = 3.0;

  // Coaching them teaches you something too, on the trips. Capped, and the
  // cap is reached inside the first six months -- so the craft gate is
  // really "your first squad", and every squad after it gets the four-year
  // version of you.
  double craftPerTrip = 1.0;
  double craftMax = 12.0;

  // Per day, and it comes out of the gym's books like any other wage.
  double coachWage = 30.0;

  // How far either side of the age a successor's seat wants a graduate can
  // be and still take it. Four years each way, so the window is a real one
  // and not a coincidence.
  double stepsUpSlack = 4.0;

  double psycheSession = 0.03;
  double psycheGraduation = 0.08;
};

// What a kid's level reads as on the wall. Five bands over ten rungs.
const char* YouthBand(double level, const YouthDials& dials = YouthDials{});

int YouthAge(const YouthKid& kid, int day,
             const AgeDials& age = AgeDials{});

// **Ready in the only two senses that matter**, and neither substitutes for
// the other.
bool ReadyToGraduate(const YouthKid& kid, int day,
                     const YouthDials& dials = YouthDials{},
                     const AgeDials& age = AgeDials{});

// Found it. Needs the money; the caller decides whether the room has earned
// one. Returns false if it already exists or you cannot pay.
bool FoundTheYouthTeam(Youth& youth, double& cash, int day,
                       const Rng& worldRng,
                       const YouthDials& dials = YouthDials{});

// Hand the squad over, or take it back. `hired` picks a name. Returns false
// if there is no team or it is already that way round.
bool SetYouthCoach(Youth& youth, bool hired, const Rng& worldRng, int day);

// Why you cannot run one tonight, or empty. **The refusal carries the
// reason**, like every other gate in this port.
std::string WhyNotASession(const Youth& youth, int day, double energy,
                           const YouthDials& dials = YouthDials{});

struct YouthSession {
  bool ran = false;
  std::string said;
  bool wasATrip = false;
  double standing = 0.0;
  double psyche = 0.0;
  // Who outgrew you tonight, if anybody. **One a session**, so it always
  // gets its own moment rather than being one of three.
  std::string graduated;
  std::string joined;
};

// An evening with the squad. Every fifth one is a trip rather than a
// training night. Progression, graduation and recruitment all happen here.
//
// **Runs whether or not the evening is yours.** A hired coach's session is
// this one at `gainHired`, and it teaches you nothing and earns you no
// standing -- the source's own comment says the paid coach "runs it without
// you", and see the header for why that sentence was not true in the
// original. `WhyNotASession` is what refuses a *player's* session; this is
// the squad training, and the squad trains either way.
YouthSession RunASession(Youth& youth, const Rng& worldRng, int day,
                         double wingHelp = 0.0,
                         const YouthDials& dials = YouthDials{},
                         const AgeDials& age = AgeDials{});

// Is the squad due a session tonight? What the night tick asks before it
// runs one on a hired coach's behalf.
bool SquadIsDue(const Youth& youth, int day,
                const YouthDials& dials = YouthDials{});

// What a hired coach costs the gym today, or zero.
double YouthWageToday(const Youth& youth,
                      const YouthDials& dials = YouthDials{});

// **A graduate old enough to be the one chasing you**, taken off the queue.
//
// The queue is not first-in-first-out, and the reason is arithmetic: a
// successor starts at `RivalDials::rivalStartAge - 2`, and a kid who left
// at sixteen is that age about eight years later. Popping the front would
// hand the seat to somebody who graduated twenty-five years ago and is now
// forty-one; popping the back would hand it to a seventeen-year-old. So it
// takes whoever is **closest to the age the seat wants**, and forgets
// anybody who is past it by more than the slack -- they had their chance
// and went and had a life instead.
//
// Empty when nobody is close enough, which is most of a career and all of
// one without a youth team.
std::string SomebodyStepsUp(Youth& youth, int day, double wantedAge,
                            const YouthDials& dials = YouthDials{},
                            const AgeDials& age = AgeDials{});

// One line for the counter: the squad, and who is closest.
std::string YouthLine(const Youth& youth, int day,
                      const YouthDials& dials = YouthDials{},
                      const AgeDials& age = AgeDials{});

}  // namespace dirtbag
