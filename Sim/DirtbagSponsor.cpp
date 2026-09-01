#include "DirtbagSponsor.h"

#include <algorithm>

namespace dirtbag {

const char* SponsorTierName(SponsorTier tier) {
  switch (tier) {
    case SponsorTier::None:  return "nobody is calling";
    case SponsorTier::Shoes: return "a shoe deal";
    case SponsorTier::Gear:  return "a gear sponsor";
    case SponsorTier::Title: return "a title sponsor";
  }
  return "nobody is calling";
}

SponsorTier OfferFor(int hardestSend, int firstAscents,
                     const Standing& standing, const SponsorDials& dials) {
  // What people can see, not what you can do. A crusher nobody has heard of
  // gets nothing, and that is the whole reason the Scene axis exists.
  const double visibility =
      StandingWith(standing, Faction::Scene) +
      dials.sceneCreditPerFirstAscent * static_cast<double>(firstAscents);

  if (hardestSend >= dials.gradeForTitle && visibility >= dials.sceneForTitle) {
    return SponsorTier::Title;
  }
  if (hardestSend >= dials.gradeForGear && visibility >= dials.sceneForGear) {
    return SponsorTier::Gear;
  }
  if (hardestSend >= dials.gradeForShoes && visibility >= dials.sceneForShoes) {
    return SponsorTier::Shoes;
  }
  return SponsorTier::None;
}

double MonthlyStipend(SponsorTier tier, const SponsorDials& dials) {
  switch (tier) {
    case SponsorTier::None:  return 0.0;
    case SponsorTier::Shoes: return dials.monthlyShoes;
    case SponsorTier::Gear:  return dials.monthlyGear;
    case SponsorTier::Title: return dials.monthlyTitle;
  }
  return 0.0;
}

bool CoversShoes(const Sponsorship& deal) {
  // Every rung includes rubber. The bottom one is *only* rubber, and for
  // somebody living on $6,000 a year that is most of a deal.
  return deal.tier != SponsorTier::None;
}

namespace {

int ObligationDaysFor(SponsorTier tier, const SponsorDials& dials) {
  switch (tier) {
    case SponsorTier::None:  return 0;
    case SponsorTier::Shoes: return dials.obligationDaysShoes;
    case SponsorTier::Gear:  return dials.obligationDaysGear;
    case SponsorTier::Title: return dials.obligationDaysTitle;
  }
  return 0;
}

}  // namespace

bool ObligationToday(const Sponsorship& deal, const Rng& worldRng, int day,
                     bool dayHasAWindow, const SponsorDials& dials) {
  const int perMonth = ObligationDaysFor(deal.tier, dials);
  if (perMonth <= 0) return false;

  // Only ever a day with a window. You cannot shoot climbing photos in the
  // rain and nobody runs a comp in February for the love of it — so a
  // sponsor's days are precisely the days you wanted, which is what makes
  // this a trade rather than a reward.
  if (!dayHasAWindow) return false;

  // Its own stream: who wants you on a Tuesday must never shift the weather
  // or how an attempt resolved.
  Rng rng = worldRng.Derive("sponsor#" + std::to_string(day));
  // Roughly `perMonth` days out of the ~20 in a month that come good.
  const double chance = std::min(0.9, static_cast<double>(perMonth) / 20.0);
  return rng.Chance(chance);
}

SponsorTier ReviewSeason(Sponsorship& deal, int hardestSendNow,
                         int daysHurtThisSeason, const SponsorDials& dials) {
  if (deal.tier == SponsorTier::None) return SponsorTier::None;

  deal.seasonsHeld++;

  // Being hurt is not failing, and a sponsor who dropped you for it would
  // be a worse sponsor than most real ones. A long injury pauses the clock.
  const bool wasHurtAllSeason =
      daysHurtThisSeason >= dials.injuryDaysThatPauseReview;

  if (hardestSendNow > deal.gradeAtLastReview) {
    deal.seasonsWithoutProgress = 0;
  } else if (!wasHurtAllSeason) {
    deal.seasonsWithoutProgress++;
  }
  deal.gradeAtLastReview = std::max(deal.gradeAtLastReview, hardestSendNow);

  if (deal.seasonsWithoutProgress >= dials.seasonsOfNothingBeforeDropped) {
    // They stop returning calls. One rung down rather than out — the shoe
    // people will still have you, and a career that ends in one review
    // would be a punishment rather than a story.
    deal.tier = deal.tier == SponsorTier::Title  ? SponsorTier::Gear
                : deal.tier == SponsorTier::Gear ? SponsorTier::Shoes
                                                 : SponsorTier::None;
    deal.seasonsWithoutProgress = 0;
  }

  deal.obligationsMetThisSeason = 0;
  deal.obligationsMissedThisSeason = 0;
  return deal.tier;
}

void SignedWith(SponsorTier tier, Standing& standing,
                const FactionDials& dials) {
  if (tier == SponsorTier::None) return;
  // The Scene likes a sponsored climber and the old guard has opinions
  // about one. Taking money to climb is the oldest argument in the sport,
  // and the axis was built for exactly this.
  const double weight = tier == SponsorTier::Title  ? 0.30
                        : tier == SponsorTier::Gear ? 0.18
                                                    : 0.07;
  Shift(standing, Faction::Scene, weight, dials);
}

std::string SponsorText(const Sponsorship& deal, const SponsorDials& dials) {
  switch (deal.tier) {
    case SponsorTier::None:
      return "nobody is calling";
    case SponsorTier::Shoes:
      return "free shoes, and they want nothing";
    case SponsorTier::Gear:
      return "a bag of kit and $" +
             std::to_string(static_cast<int>(dials.monthlyGear)) +
             " a month; they want a day, and it will not be a rainy one";
    case SponsorTier::Title:
      return "$" + std::to_string(static_cast<int>(dials.monthlyTitle)) +
             " a month, and they want three days — and they will not be the "
             "rainy ones";
  }
  return "nobody is calling";
}

}  // namespace dirtbag
