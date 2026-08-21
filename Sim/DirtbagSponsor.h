// Sponsorship: getting paid to climb, and what that costs you.
//
// Every other source of money in this game competes with climbing — a shift
// is four hours you were not on the rock. Sponsorship is the opposite
// shape: money that arrives *because* you climbed well. That difference is
// why it is worth building, and it is the last untried answer to the
// Phase 3 gate, which is still open because money has never once bought
// climbing (notes/phase3-economy.md, notes/phase3-kit.md).
//
// The mechanic that makes it a real trade rather than a reward:
//
//     **A shift lands on any day. A photo shoot lands on a good one.**
//
// You cannot shoot climbing photos in the rain, and nobody runs a comp in
// February for the love of it. So a sponsor's obligations fall on exactly
// the days you wanted for yourself — which is the thing sponsored climbers
// actually complain about, and the only version of this that is a decision
// rather than a bonus.
//
// What earns a deal is what people can *see*: what you have sent, what you
// have put up, and what the Scene thinks of you. Not ability — nobody can
// see ability, and a crusher nobody has heard of gets nothing. That makes
// the Scene faction load-bearing for the first time.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>

#include "DirtbagCore.h"
#include "DirtbagFactions.h"
#include "DirtbagRng.h"

namespace dirtbag {

// The ladder. Each rung pays more and owns more of your calendar.
enum class SponsorTier {
  None,
  Shoes,    // free rubber, and that is the whole deal. No strings.
  Gear,     // a small stipend, a bag of kit, and they want photos
  Title,    // real money, and they own days you would rather have
};
constexpr int kSponsorTierCount = 4;

// unwired-ok: a formatter; SponsorText is what the HUD prints
const char* SponsorTierName(SponsorTier tier);

struct SponsorDials {
  // What each rung asks of you, as a hardest-send grade. Sized against the
  // crag: Roadside tops out at V7 and the cave at 5.14a, so a shoe deal is
  // reachable in a first season and a title deal is a career.
  int gradeForShoes = 5;
  int gradeForGear = 7;
  int gradeForTitle = 9;

  // And what the Scene has to think of you. A crusher nobody has heard of
  // gets nothing, which is the whole point of the Scene axis existing.
  double sceneForShoes = 0.0;
  double sceneForGear = 0.25;
  double sceneForTitle = 0.55;

  // First ascents count as visibility, because they are the thing that
  // actually gets written about. Each is worth this much Scene standing
  // toward an offer, without moving the standing itself.
  double sceneCreditPerFirstAscent = 0.08;

  // What they pay, a month. A shoe deal pays nothing and saves you $165 a
  // pair, which for a dirtbag is most of a deal.
  double monthlyShoes = 0.0;
  double monthlyGear = 120.0;
  double monthlyTitle = 640.0;

  // --- What it costs ----------------------------------------------------
  // Obligation days a month, and they land on days with a window. This is
  // the entire mechanic: money that costs you the good days rather than the
  // spare ones.
  int obligationDaysShoes = 0;
  int obligationDaysGear = 1;
  int obligationDaysTitle = 3;

  // An obligation eats the day the way a media day does: hours, and the
  // window with them.
  double obligationHours = 6.0;
  double obligationEnergy = 30.0;

  // --- Losing it --------------------------------------------------------
  // Deals are reviewed. Stop performing and it goes — not viciously, but it
  // goes. Seasons you can go without moving your hardest grade before they
  // stop returning calls.
  int seasonsOfNothingBeforeDropped = 2;

  // Being hurt is not failing, and a sponsor who dropped you for it would
  // be a worse sponsor than most real ones. A long injury pauses the clock
  // rather than running it.
  int injuryDaysThatPauseReview = 30;
};

struct Sponsorship {
  SponsorTier tier = SponsorTier::None;
  int seasonsHeld = 0;
  // The hardest grade you had sent when they last looked at you. Review is
  // measured against this, not against your peak — you are being asked what
  // you have done lately.
  int gradeAtLastReview = -1;
  int seasonsWithoutProgress = 0;
  int obligationsMetThisSeason = 0;
  int obligationsMissedThisSeason = 0;
};

// What they would offer somebody with this record right now. `firstAscents`
// is what you have put up; `hardestSend` is what you have actually done.
// Returns None when nobody is interested, which is most careers.
SponsorTier OfferFor(int hardestSend, int firstAscents,
                     const Standing& standing,
                     const SponsorDials& dials = SponsorDials{});

// unwired-ok: NOT WIRED -- a sponsored player is never paid. Tracked in
// notes/engine-bridge-gaps.md
double MonthlyStipend(SponsorTier tier,
                      const SponsorDials& dials = SponsorDials{});

// Does the deal cover your rubber? The shoe deal's whole value, and it is
// worth more to a dirtbag than the tier above it looks.
bool CoversShoes(const Sponsorship& deal);

// Does the sponsor own today? Only ever a day with a window — you cannot
// shoot climbing photos in the rain, and that is what makes this cost
// something. Deterministic on its own named stream.
bool ObligationToday(const Sponsorship& deal, const Rng& worldRng, int day,
                     bool dayHasAWindow,
                     const SponsorDials& dials = SponsorDials{});

// End of season: do they keep you? Returns the tier you hold afterwards.
// `daysHurtThisSeason` pauses the clock rather than running it, because
// being injured is not the same as not trying.
// unwired-ok: NOT WIRED -- a deal is never reviewed, so no rung is ever
// won or lost. Tracked in notes/engine-bridge-gaps.md
SponsorTier ReviewSeason(Sponsorship& deal, int hardestSendNow,
                         int daysHurtThisSeason,
                         const SponsorDials& dials = SponsorDials{});

// Taking a deal says something about you: the Scene likes it and the old
// guard does not, which is the axis working exactly as designed.
void SignedWith(SponsorTier tier, Standing& standing,
                const FactionDials& dials = FactionDials{});

// "Free shoes, and they want nothing." / "They want three days a month, and
// they will not be the rainy ones."
std::string SponsorText(const Sponsorship& deal,
                        const SponsorDials& dials = SponsorDials{});

}  // namespace dirtbag
