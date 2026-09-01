#pragma once

// What you climb makes you who you are -- `DEPTH-6`, `DEPTH-13` and
// `CHAR-6`, ported from the 2D source.
//
// The port has had five trained skills since Phase 0 and a route type on
// every line since Phase 2, and **nothing has ever connected the two**. A
// career of nothing but crimping and a career of nothing but dynos built
// the same climber, because the only thing a route type did was pick which
// skills the resolver read.
//
// The source's line is the design: *"a specialist emerges from what you
// actually climb."* Every attempt hones a style a little and a send teaches
// it more; over-index a style and it becomes a strength, neglect one and it
// rots into an anti-style.
//
// ## Three things fall out of one tally, and the third is the good one
//
// **Odds.** A specialty is worth a little; an anti-style costs more, because
// neglect bites harder than mastery rewards. The asymmetry is the source's
// and it is what stops a career of one style being free.
//
// **Gains.** Backwards from the odds, deliberately: **your anti-style trains
// fastest and your specialty plateaus.** That is what makes the whole thing
// self-correcting rather than a spiral -- a crimper who finally gets on a
// dyno improves at it quickly, which is both true and the only reason
// specialising is a choice rather than a trap.
//
// **A name.** Send enough of one style and you have a move people know you
// for. It is worth a standing bump on that style, it is the only thing in
// this game the *player* names about their own climbing, and it goes in the
// legacy. A second one unlocks much later and in a different style, because
// two signatures in the same style is not a second identity.
//
// ## Where it reaches an attempt
//
// Through `BodyContext`, like everything else that follows a climber -- a
// specialist is a specialist at a comp exactly as a bad tooth is a bad
// tooth there. Not through the caller, which is how half the attempt paths
// would forget it.
//
// Engine-free like everything in Sim/.

#include <string>

#include "DirtbagCore.h"

namespace dirtbag {

struct StyleDials {
  // What an attempt and a send are worth. A send teaches three times what
  // flailing does, which is the source's ratio and the right one: mileage
  // is not the same as figuring it out.
  double perAttempt = 1.0;
  double perSend = 3.0;

  // Below this much total volume you have no emergent style at all -- a
  // gumby at everything, which is what a new climber is. Fourteen, so it
  // is a couple of sessions and not a season.
  double minVolume = 14.0;

  // **The asymmetry is the whole design.** A specialty is worth a little
  // and an anti-style costs nearly twice as much, because neglect bites
  // harder than mastery rewards -- without that, specialising is free.
  //
  // **Re-derived, not copied.** The source states these as *odds* -- +0.07
  // for a specialty, -0.12 for an anti-style, slopes of 0.30 and 0.72 --
  // and this port's `oddsPenalty` seam is in **grade units**, because that
  // is the currency every other modifier on an attempt already speaks. Feed
  // 0.07 into a grade field and a lifetime of specialising is worth
  // nothing at all.
  //
  // Measured, on a limit route at the point of the curve where a lean
  // actually decides something: a shift of 0.10 grades moves send odds
  // 19.4% -> 23.9%, 0.25 grades -> 31.8%, 0.50 grades -> 45.7%. Nearer the
  // top of the curve it flattens (1.00 grade is worth 24 points there
  // against 51 here), so the honest conversion across a career is about
  // **0.35 odds to the grade**, and these are the source's numbers through
  // it. Fourth time this project has had to re-derive rather than copy;
  // see `GYM-6`, `GYM-8` and `TAX-1`.
  double specialtyCap = 0.20;      // 0.07 odds
  double antiStyleCap = 0.34;      // 0.12 odds
  double specialtySlope = 0.86;    // 0.30 odds
  double antiStyleSlope = 2.06;    // 0.72 odds

  // And backwards on gains, which is what keeps it from being a spiral.
  double gainOnAntiStyle = 1.45;
  double gainOnSpecialty = 0.85;

  // Where the labels sit. Through the same conversion, so they still land
  // at the same shares of your climbing the source put them at.
  double specialtyAt = 0.13;
  double solidAt = 0.034;
  double weakAt = -0.034;
  double antiStyleAt = -0.17;

  // `CHAR-6`: sends of one style before it has a name, and `CHAR-6b`'s much
  // later second one. Forty is the same bar the source's route-type mastery
  // uses -- a genuine late-career milestone rather than a quick follow-up.
  double signatureAt = 15.0;
  double secondSignatureAt = 40.0;
  // What a named move is worth on its own style, and the one-time bump to
  // your standing when the scene starts calling it something. The bonus is
  // the source's 0.10 odds through the same conversion.
  double signatureBonus = 0.29;
  double signatureRep = 12.0;
};

// Every attempt and every send you have ever made, by style. Not decayed:
// this is who you turned into, and a career does not forget how to crimp
// over a quiet winter. (Contrast `Logbook`, which decays by design because
// it answers "what have you been doing *lately*".)
struct StyleLog {
  double xp[kRouteTypeCount] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  // Sends only, which is what a signature is counted in. A move you have
  // never actually done is not a move you are known for.
  double sends[kRouteTypeCount] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
};

// A named move. `type` is meaningless while `name` is empty.
struct Signature {
  RouteType type = RouteType::Crimp;
  std::string name;
};

// Log one attempt. Call it once per burn, wherever a burn is resolved.
void Climbed(StyleLog& log, RouteType type, bool sent,
             const StyleDials& dials = StyleDials{});

double StyleVolume(const StyleLog& log);

// Have you climbed enough for any of this to mean anything?
bool HasAStyle(const StyleLog& log, const StyleDials& dials = StyleDials{});

// This style's share of your climbing, above or below an even spread.
// Zero when there is not enough mileage to say.
double StyleDeviation(const StyleLog& log, RouteType type,
                      const StyleDials& dials = StyleDials{});

// What it is worth on the odds. Positive for a specialty, negative for an
// anti-style, and capped both ways.
double AffinityOdds(const StyleLog& log, RouteType type,
                    const StyleDials& dials = StyleDials{});

// What it does to what a session teaches you. Above one on an anti-style,
// below one on a specialty -- see the header.
double StyleGainMultiplier(const StyleLog& log, RouteType type,
                           const StyleDials& dials = StyleDials{});

// "specialty" / "solid" / "neutral" / "weak" / "anti-style", or empty
// before there is enough mileage to have an opinion.
const char* StyleTierName(const StyleLog& log, RouteType type,
                          const StyleDials& dials = StyleDials{});

// The style you are best and worst at. Both meaningless before `HasAStyle`.
RouteType YourStyle(const StyleLog& log, const StyleDials& dials = StyleDials{});
RouteType YourAntiStyle(const StyleLog& log,
                        const StyleDials& dials = StyleDials{});

// "You are known for crimping, and everybody knows you cannot dyno."
// Empty before there is enough mileage.
std::string StyleLine(const StyleLog& log,
                      const StyleDials& dials = StyleDials{});

// --- the named move -----------------------------------------------------

// Which style is ready to be named, if any. `already` is the signature you
// have (empty name for none) and `second` the one after it, because
// `CHAR-6b` deliberately excludes the first one's own type.
//
// Returns false when nothing is ready. The caller asks the player for a
// name; nothing is named without them.
bool AStyleWantsAName(const StyleLog& log, const Signature& already,
                      const Signature& second, RouteType& out,
                      const StyleDials& dials = StyleDials{});

// What both named moves are worth on this style's odds. They stack, because
// having two in one style is already excluded at naming time.
double SignatureBonus(const Signature& first, const Signature& second,
                      RouteType type,
                      const StyleDials& dials = StyleDials{});

// "The Cauldron -- your crimp sequence. The scene knows you for it now."
std::string SignatureLine(const Signature& sig);

// What kind of move it is, in words: "crimp sequence", "dyno", "crack
// pitch". Used by the naming prompt and by the legacy.
const char* SignatureKind(RouteType type);

}  // namespace dirtbag
