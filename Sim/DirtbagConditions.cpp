#include "DirtbagConditions.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace dirtbag {

namespace {
constexpr double kPi = 3.14159265358979323846;

// Peak sun hour per aspect. The sun tracks east to west, so the face it hits
// hardest walks with it; north never gets a direct hit at these latitudes.
double PeakSunHour(Aspect a) {
  switch (a) {
    case Aspect::East:  return 9.5;
    case Aspect::South: return 13.0;
    case Aspect::West:  return 16.5;
    case Aspect::North: return 13.0;  // unused; north takes no direct sun
  }
  return 13.0;
}

// Formats an hour as "4:15pm" — the way a climber says it, not 16.25.
std::string ClockText(double hour) {
  int h = static_cast<int>(hour);
  int m = static_cast<int>((hour - h) * 60.0 + 0.5);
  if (m >= 60) { m -= 60; h += 1; }
  const char* suffix = h >= 12 ? "pm" : "am";
  int display = h % 12;
  if (display == 0) display = 12;
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%d:%02d%s", display, m, suffix);
  return std::string(buf);
}
}  // namespace

const char* AspectName(Aspect a) {
  switch (a) {
    case Aspect::North: return "north-facing";
    case Aspect::East:  return "east-facing";
    case Aspect::South: return "south-facing";
    case Aspect::West:  return "west-facing";
  }
  return "north-facing";
}

double SeasonalCentreF(int day, const ConditionsDials& dials) {
  const int period = std::max(1, dials.daysPerYear);
  const double phase =
      2.0 * kPi * static_cast<double>(day - dials.warmestDay) / period;
  return dials.baseTempF + dials.seasonSwingF * std::cos(phase);
}

const char* SeasonName(int day, const ConditionsDials& dials) {
  // Named off the temperature rather than the calendar, because that is
  // what a climber means by "the season".
  const double centre = SeasonalCentreF(day, dials);
  const double high = dials.baseTempF + dials.seasonSwingF * 0.5;
  const double low = dials.baseTempF - dials.seasonSwingF * 0.5;
  if (centre >= high) return "summer";
  if (centre <= low) return "winter";
  // Rising or falling decides which shoulder you are on.
  return SeasonalCentreF(day + 5, dials) > centre ? "spring" : "autumn";
}

Weather GenerateWeather(const Rng& worldRng, int day,
                        const ConditionsDials& dials) {
  // Its own stream: weather must never move worldgen or session vectors.
  Rng rng = worldRng.Derive("weather#" + std::to_string(day));

  // The day's weather sits around wherever the year currently is.
  const double centre = SeasonalCentreF(day, dials);
  const double swing = (rng.NextDouble() * 2.0 - 1.0) * dials.tempSwingF;
  Weather w;
  w.highTempF = centre + swing + dials.diurnalSwingF * 0.5;
  w.lowTempF = centre + swing - dials.diurnalSwingF * 0.5;

  // Humidity skews low-ish: most days are workable, a few are a grease-fest.
  const double hRoll = rng.NextDouble();
  w.humidity = Clamp01(hRoll * hRoll * 1.15);

  w.cloud = Clamp01(rng.NextDouble());
  w.wind = Clamp01(rng.NextDouble() * 0.8);
  return w;
}

double TemperatureAt(const Weather& w, double hour,
                     const ConditionsDials& dials) {
  const double mid = (w.highTempF + w.lowTempF) * 0.5;
  const double amp = (w.highTempF - w.lowTempF) * 0.5;
  // Cosine anchored so the trough lands on coldestHour and the crest a
  // half-cycle later — close enough to a real diurnal curve to plan a day by.
  const double period = 2.0 * (dials.hottestHour - dials.coldestHour);
  const double phase = 2.0 * kPi * (hour - dials.coldestHour) / period;
  return mid - amp * std::cos(phase);
}

double SunOnRock(Aspect aspect, double hour, const Weather& w,
                 const ConditionsDials& dials) {
  if (hour < dials.firstLight || hour > dials.lastLight) return 0.0;
  if (aspect == Aspect::North) return 0.0;  // the summer dirtbag's whole plan

  // A raised cosine centred on the aspect's peak: the sun swings onto the
  // face, cooks it, and swings off. Six hours wide either side of peak.
  const double spread = 5.0;
  const double delta = std::fabs(hour - PeakSunHour(aspect));
  if (delta >= spread) return 0.0;
  const double lit = 0.5 * (1.0 + std::cos(kPi * delta / spread));

  // Cloud is the reprieve that saves a south-facing day.
  return Clamp01(lit * (1.0 - 0.85 * w.cloud));
}

double RockTempAt(const Weather& w, Aspect aspect, double hour,
                  const ConditionsDials& dials) {
  // Forward-integrate the day's solar loading from first light. The wall
  // gains heat while the sun is on it and sheds a fixed fraction of its
  // excess every hour, so it peaks well after the sun has moved off and is
  // still warm when the air has already cooled.
  const double dt = 0.25;
  double excess = 0.0;  // degrees above air temperature
  for (double h = dials.firstLight; h < hour - 1e-9; h += dt) {
    const double sun = SunOnRock(aspect, h, w, dials);
    excess += (dials.solarGainF * sun - excess * dials.rockCoolRate) * dt;
    if (excess < 0.0) excess = 0.0;
  }
  return TemperatureAt(w, hour, dials) + excess;
}

Conditions ConditionsAt(const Weather& w, Aspect aspect, double hour,
                        const ConditionsDials& dials) {
  const double tempF = RockTempAt(w, aspect, hour, dials);

  // Gaussian around the ideal: too cold is as bad as too warm, which is the
  // part non-climbers get wrong — numb fingers do not read holds.
  const double z = (tempF - dials.idealTempF) / dials.tempToleranceF;
  const double tempQuality = std::exp(-z * z);

  const double humid = 1.0 - dials.humidityBite * w.humidity;
  const double sun = SunOnRock(aspect, hour, w, dials);

  Conditions c;
  c.friction = Clamp01(tempQuality * humid - dials.sunPenalty * sun +
                       dials.windGain * w.wind);
  return c;
}

PrimeWindow FindPrimeWindow(const Weather& w, Aspect aspect,
                            const ConditionsDials& dials) {
  PrimeWindow best;

  // Quarter-hour resolution: finer than any decision the player makes, and
  // the same grid the day clock advances on.
  const double step = 0.25;

  // Find the peak first — it is reported even on a day with no window, so
  // the forecast can say "best it gets is 2pm, and it is not good enough".
  for (double h = dials.firstLight; h <= dials.lastLight + 1e-9; h += step) {
    const double f = ConditionsAt(w, aspect, h, dials).friction;
    if (f > best.peakFriction) {
      best.peakFriction = f;
      best.peakHour = h;
    }
  }

  // A day that never gets good has no window — only a peak, reported so the
  // forecast can tell you not to bother.
  if (best.peakFriction < dials.primeThreshold) return best;

  // The window is the contiguous span around the peak that stays within
  // windowBand of it. Walking out from the peak (rather than scanning the
  // whole day) guarantees the span actually contains the best moment.
  const double bar = best.peakFriction - dials.windowBand;
  double start = best.peakHour;
  while (start - step >= dials.firstLight &&
         ConditionsAt(w, aspect, start - step, dials).friction >= bar) {
    start -= step;
  }
  double end = best.peakHour;
  while (end + step <= dials.lastLight &&
         ConditionsAt(w, aspect, end + step, dials).friction >= bar) {
    end += step;
  }
  best.exists = true;
  best.startHour = start;
  best.endHour = end;

  return best;
}

const char* ConditionsText(double friction) {
  if (friction >= 0.80) return "sticky — this is the day";
  if (friction >= 0.62) return "good — it's on";
  if (friction >= 0.45) return "workable";
  if (friction >= 0.28) return "greasy";
  return "hopeless — save your skin";
}

std::string WindowText(const PrimeWindow& window) {
  if (!window.exists) {
    return "no window today; best it gets is " + ClockText(window.peakHour) +
           ", and that's not good enough";
  }
  return "window " + ClockText(window.startHour) + " to " +
         ClockText(window.endHour) + ", best at " + ClockText(window.peakHour);
}

}  // namespace dirtbag
