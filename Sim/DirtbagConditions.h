// Conditions — the day's weather, the shade line, and the prime window.
//
// Design pillar #4 from concepts/DIRTBAG.md: "Conditions are content. The
// daily prime window — temperature, humidity, sun angle — is the day's
// structure. Waiting in the shade for the 4pm window is gameplay."
//
// The resolver has always consumed a `friction` scalar and nothing has ever
// produced one. This is the producer: seed + day -> weather, weather +
// aspect + hour -> friction, friction over the day -> the window.
//
// Why the window is deliberately SHORT (measured, see notes/phase2-window.md):
// skin allows only 8-12 burns a day, which is 2-3 hours of climbing. A
// window longer than that lets you fit the whole day inside it, and waiting
// becomes free — the same trap that killed the Phase 1 day clock. At ~1 hour
// the day becomes a real question: burns outside the window buy beta but
// spend the skin you need when conditions arrive.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>

#include "DirtbagCore.h"
#include "DirtbagRng.h"

namespace dirtbag {

// Which way the rock faces — this, not the forecast, decides what time of
// day the crag is climbable. An east-facing wall bakes at breakfast and
// comes into the shade mid-afternoon; that shade line is the window.
enum class Aspect { North, East, South, West };

const char* AspectName(Aspect a);

// Every number a designer might turn, with its reason.
struct ConditionsDials {
  // Ideal sending temperature. Cold rock is sticky rock; below this your
  // fingers go numb and stop reading the holds, above it everything greases.
  // 50F sits in the boulderer's folklore band and, against the season below,
  // is crossed twice a day rather than approached at one end of it.
  double idealTempF = 50.0;
  // Degrees either side of ideal before friction falls off badly (gaussian
  // width). Tight on purpose: a wide band made every daylight hour roughly
  // as good as every other, which pinned the day's best moment to whichever
  // end of it was coldest — half of all days peaked at first light. At 10 the
  // day has to pass *through* condition, which is what puts the peak in the
  // middle of the day where a decision can live.
  double tempToleranceF = 10.0;

  // Humidity is the dirtbag's real enemy — worse than heat, and the reason
  // the forecast gets checked twice. At 1.0 it takes most of the friction.
  double humidityBite = 0.85;

  // Direct sun on the rock. The single biggest lever, because it is the one
  // you can wait out: the shade line moves whether you like it or not.
  // Small, because sun mostly acts through the rock's temperature below —
  // this residual is just the misery of climbing in your own shadow.
  double sunPenalty = 0.12;

  // --- Rock temperature -------------------------------------------------
  // The wall is a storage heater. It loads all the time the sun is on it and
  // keeps giving that heat back long after the shade line has passed, which
  // is exactly why you sit and wait rather than pulling on the moment it
  // goes shady. This lag is what makes the window narrow: prime is the
  // crossing, when rock temperature passes down through ideal.
  double solarGainF = 13.0;   // degrees/hour of loading under full sun
  double rockCoolRate = 0.75; // fraction of the excess shed per hour

  // Wind dries skin and rock — the thing that saves a marginal day.
  double windGain = 0.18;

  // --- Weather generation, per day, from the "weather" rng stream --------
  // The year's centre. Dawn is genuinely too cold to pull hard (numb
  // fingers read nothing) and mid-afternoon is greasy, so the rock comes
  // into condition on the way up and again on the way down — and which of
  // those two crossings is the good one is what the aspect decides.
  double baseTempF = 48.0;      // the YEAR's centre, not the season's
  double tempSwingF = 12.0;     // day-to-day variation either side
  double diurnalSwingF = 30.0;  // within one day, dawn to mid-afternoon

  // --- Seasons ----------------------------------------------------------
  // How much hotter high summer is than the year's mean, and when that
  // falls. At 20 the year runs from a winter centred near 28F to a summer
  // centred near 68F, which moves the window rather than merely making it
  // better or worse: in summer only dawn is cool enough and only the
  // north-facing rock is worth walking to, in winter the good hours are the
  // middle of the day, and the shoulder seasons are what bouldering is for.
  //
  // This is what makes a job cost something. Measured without seasons, a
  // nine-to-five and an east-facing crag never conflicted, because the
  // window sat in the evening all year (notes/phase3-jobs.md). A winter
  // window at midday is a window you cannot have if you are at work.
  double seasonSwingF = 20.0;
  int warmestDay = 200;    // day 1 is midwinter-ish; the peak is high summer
  int daysPerYear = 365;
  double coldestHour = 3.0;     // when the low lands
  double hottestHour = 15.0;    // when the high lands

  // --- The window -------------------------------------------------------
  // A day has a window at all only if its best moment clears this bar. Below
  // it you drove out for nothing: the day is content precisely because it
  // can refuse you.
  double primeThreshold = 0.62;

  // The window is the span within this much friction of the day's peak —
  // peak-relative, not absolute, so it stays tight on a north-facing wall
  // that is merely good all day as well as on a west face that comes into
  // condition for forty minutes.
  //
  // Sized against skin, which is the real budget: a day holds 8-12 burns, so
  // a window of about an hour (4 burns) forces the split that makes
  // projecting a decision — recon burns outside it buy the beta you need,
  // and spend the skin you were saving. Measured wide-open (a window big
  // enough for a whole day's skin) the decision collapses: 8 burns in prime
  // conditions send a limit project ~98% of the time with no beta at all,
  // and the projecting loop stops existing. See notes/phase2-window.md.
  double windowBand = 0.05;

  // --- Daylight ---------------------------------------------------------
  // Outside these hours you are climbing by headlamp, which the sim does
  // not model. They move with the year, and that movement is the whole
  // reason a job costs anything.
  //
  // Held fixed at 6-to-20 all year — as they were through the first Phase 3
  // measurement — a nine-to-five still left three hours of evening light
  // every single day, and a salaried season climbed as many burns as an
  // unemployed one (521 against 518) for $19,900 more. The job was free.
  // A winter Tuesday that ends before you get home is what makes it cost
  // something, and it is also just true.
  // Solar noon, not clock noon — the light is centred half an hour before
  // the hour, and that half hour is the difference between a midwinter
  // sunset you can drive to after work and one you cannot.
  double middayHour = 12.5;
  double daylightHoursMean = 12.2;   // hours of light at the equinoxes
  double daylightSwingHours = 4.3;   // 16.5h midsummer, 7.9h midwinter
  // The longest day leads the warmest day: the ground keeps loading heat for
  // weeks after the sun has started coming back. Thirty days is the usual
  // lag, and it is why the best rock temperatures of spring arrive with the
  // evenings already long.
  int solsticeLeadDays = 30;
};

// One day's weather. Generated once per day and then read all day.
struct Weather {
  // Which day this is. Carried on the weather rather than threaded through
  // every signature, because everything that reads the weather also needs
  // to know how much light the day has.
  int day = 1;
  double highTempF = 60.0;
  double lowTempF = 40.0;
  double humidity = 0.5;  // 0..1
  double cloud = 0.3;     // 0..1; cloud cover blunts the sun penalty
  double wind = 0.2;      // 0..1
};

// Hours of daylight on this day, and where they start and end. Everything
// that asks "is there light" asks these, not a fixed pair of numbers.
double DaylightHours(int day, const ConditionsDials& dials = ConditionsDials{});
double FirstLightHour(int day, const ConditionsDials& dials = ConditionsDials{});
double LastLightHour(int day, const ConditionsDials& dials = ConditionsDials{});

// The year's temperature centre on this day — what `baseTempF` used to be
// for every day of the year.
double SeasonalCentreF(int day, const ConditionsDials& dials = ConditionsDials{});

// What the season is called, for anything that wants to say it.
const char* SeasonName(int day, const ConditionsDials& dials = ConditionsDials{});

// Deterministic per world-seed and day, on its own named rng stream so
// adding weather cannot shift worldgen or session vectors.
Weather GenerateWeather(const Rng& worldRng, int day,
                        const ConditionsDials& dials = ConditionsDials{});

// Air temperature at an hour of the day: low before dawn, high mid-afternoon.
double TemperatureAt(const Weather& w, double hour,
                     const ConditionsDials& dials = ConditionsDials{});

// What the holds actually feel like — air temperature plus the heat the wall
// has banked from the sun so far today, shed on a lag. This, not the air, is
// what decides whether the rock is sticky.
double RockTempAt(const Weather& w, Aspect aspect, double hour,
                  const ConditionsDials& dials = ConditionsDials{});

// How much direct sun is on this aspect at this hour, 0..1. North-facing
// rock never takes a direct hit (northern hemisphere) and is the summer
// dirtbag's whole strategy; east bakes in the morning, west in the evening.
double SunOnRock(Aspect aspect, double hour, const Weather& w,
                 const ConditionsDials& dials = ConditionsDials{});

// The number the resolver actually eats.
Conditions ConditionsAt(const Weather& w, Aspect aspect, double hour,
                        const ConditionsDials& dials = ConditionsDials{});

// The day's best span. `exists` is false on a day that never gets good —
// the day you drive out, feel the grease, and go work a shift instead.
struct PrimeWindow {
  bool exists = false;
  double startHour = 0.0;
  double endHour = 0.0;
  double peakHour = 0.0;
  double peakFriction = 0.0;
  double hours() const { return endHour - startHour; }
};

PrimeWindow FindPrimeWindow(const Weather& w, Aspect aspect,
                            const ConditionsDials& dials = ConditionsDials{});

// The forecast in the game's voice, for the van window and the HUD.
const char* ConditionsText(double friction);
std::string WindowText(const PrimeWindow& window);

}  // namespace dirtbag
