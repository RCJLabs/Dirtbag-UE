#pragma once

// The annual reckoning -- `TAX-1`, ported from the 2D source.
//
// The port has spent a fortnight building things that pay in lumps: a
// circuit with a season podium, a national team with a stipend, a comp
// board, a World Cup and the Games. **None of it was taxed**, and the
// source's line on why that matters is one sentence: prize money is the
// only money in this game that arrives all at once, and the only money the
// government has ever heard of.
//
// Wages are not here. A dirtbag's shifts are cash, off the books, at a
// diner and a gear shop and a rescue callout -- which is the whole point of
// the life and is exactly why the tax bill, when it lands, lands on the one
// part of the career that is legitimate.
//
// ## What it is, mechanically
//
// A running total of the year's prize money, and one day a year the tax man
// takes a fifth of it. That is all. It is not a system you interact with;
// it is a **date you either saw coming or did not**, which is the entire
// design -- the money arrived in spring, you spent it, and the bill is in
// autumn.
//
// A bill you cannot cover behaves like every other bill in this game: cash
// floors at nothing and the shortfall goes to `owed`. See
// `PlayerState::owed` -- *"a bill you cannot pay does not evaporate, it
// waits."*
//
// ## The one number that had to be re-derived
//
// The source puts tax day at offset 9 of an 18-day year -- **halfway
// through it, deliberately away from the birthday**, so the two events do
// not land on one screen. This port's year is 365 days and its birthday is
// day 1 of each year (`AgeOn`), so halfway is day 182. Copying `9` across
// would have put the reckoning in the first week of January, nine days
// after the birthday, which is the same bug this project has now hit three
// times: a number tuned against an eighteen-day year, ported as a number.
//
// Engine-free like everything in Sim/.

#include <string>

namespace dirtbag {

struct TaxDials {
  // A rough effective rate on prize winnings. The source's, and it is
  // deliberately round: this is a game's idea of a tax bill, not a
  // schedule.
  double rate = 0.20;

  // Which day of the year the reckoning lands on. **Half a year off the
  // birthday**, so the two never share a morning -- see the header note for
  // why this is 182 and not the source's 9.
  int dayOfYear = 182;
  int daysPerYear = 365;

  // How much warning the nudge gives. Three days, matching every other
  // warning in this game, and it is the difference between a date you saw
  // coming and an ambush.
  int warnDays = 3;
};

// What the year owes. **`taxable` is only ever prize money** -- see the
// header for what is deliberately not in it.
struct Tax {
  double taxable = 0.0;

  // Which tax year was last settled, so a day replayed or a save reloaded
  // on the morning of the reckoning cannot be billed twice. -1 is a career
  // that has never seen one.
  int lastSettledYear = -1;

  // Career total handed over. Pure record -- nothing reads it but the
  // legacy tally, which is exactly the sort of number a retiring climber
  // wants to know and cannot otherwise find out.
  double paidLifetime = 0.0;

  // **The morning's news, kept rather than returned.** The reckoning
  // happens inside the night tick and the engine's Sleep does not plumb a
  // return value through the Blueprint library -- it reads the ledger
  // across the call, which is how every other piece of overnight news in
  // this game already works (see `WorldCupSeasonNews`). Without this the
  // tax man takes a fifth of the year in silence, and the whole design is
  // a date you either saw coming or did not.
  int lastBillDay = -1;
  double lastTaxable = 0.0;
  double lastBilled = 0.0;
  double lastShortfall = 0.0;
};

// Bank prize money. Ignores nothing and rounds nothing: the bill is
// computed once, at the reckoning, from the total.
void BankTaxable(Tax& tax, double prizeMoney);

// Which tax year a day falls in. Days before the first reckoning are year
// zero, so a career that starts in autumn is not billed in its first week.
int TaxYearOf(int day, const TaxDials& dials = TaxDials{});

bool IsTaxDay(int day, const TaxDials& dials = TaxDials{});

// Days until the next reckoning. Zero on the day itself.
int DaysUntilTax(int day, const TaxDials& dials = TaxDials{});

struct TaxBill {
  bool due = false;        // false when it is not the day, or there is nothing owed
  double taxableWas = 0.0;
  double billed = 0.0;
  double paid = 0.0;
  double shortfall = 0.0;  // what went onto `owed`
};

// Settle the year. Takes cash and the owed ledger the same way every other
// bill in this game does, zeroes the running total, and refuses to bill the
// same year twice.
TaxBill SettleTheYear(Tax& tax, double& cash, double& owed, int day,
                      const TaxDials& dials = TaxDials{});

// What the reckoning reads like. Empty when nothing was due.
std::string TaxLine(const TaxBill& bill);

// The same sentence, rebuilt from the ledger on the morning it happened.
// Empty on every other day. This is the one the engine reads.
std::string TaxNews(const Tax& tax, int day);

// The warning, inside `warnDays` of the date and only when there is
// something to warn about. Empty otherwise.
std::string TaxWarning(const Tax& tax, int day,
                       const TaxDials& dials = TaxDials{});

}  // namespace dirtbag
