#include "DirtbagTax.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {
namespace {

std::string Money(double amount) {
  const long whole = static_cast<long>(std::llround(amount));
  return "$" + std::to_string(whole);
}

int Period(const TaxDials& dials) { return std::max(1, dials.daysPerYear); }

}  // namespace

void BankTaxable(Tax& tax, double prizeMoney) {
  if (prizeMoney <= 0.0) return;
  tax.taxable += prizeMoney;
}

int TaxYearOf(int day, const TaxDials& dials) {
  // A career that starts after the reckoning is in year zero until the next
  // one comes round, which is what stops a climber who arrived in autumn
  // being billed in their first week for money they have not won.
  const int period = Period(dials);
  return (day - dials.dayOfYear + period) / period;
}

bool IsTaxDay(int day, const TaxDials& dials) {
  return day % Period(dials) == dials.dayOfYear % Period(dials);
}

int DaysUntilTax(int day, const TaxDials& dials) {
  const int period = Period(dials);
  const int target = ((dials.dayOfYear % period) + period) % period;
  return ((target - (day % period)) % period + period) % period;
}

TaxBill SettleTheYear(Tax& tax, double& cash, double& owed, int day,
                      const TaxDials& dials) {
  TaxBill bill;
  if (!IsTaxDay(day, dials)) return bill;

  const int year = TaxYearOf(day, dials);
  if (tax.lastSettledYear == year) return bill;
  tax.lastSettledYear = year;

  // **A year with no prize money still settles.** Marking the year done is
  // the point of the call, not the bill -- otherwise a career that won
  // nothing this year would be re-checked every hour of the day and would
  // hand over a bill the moment it placed at an evening comp.
  if (tax.taxable <= 0.0) return bill;

  bill.due = true;
  bill.taxableWas = tax.taxable;
  bill.billed = std::round(tax.taxable * dials.rate);
  tax.taxable = 0.0;

  bill.paid = std::min(cash, bill.billed);
  cash = std::max(0.0, cash - bill.billed);
  bill.shortfall = bill.billed - bill.paid;
  owed += bill.shortfall;
  tax.paidLifetime += bill.paid;

  tax.lastBillDay = day;
  tax.lastTaxable = bill.taxableWas;
  tax.lastBilled = bill.billed;
  tax.lastShortfall = bill.shortfall;
  return bill;
}

std::string TaxLine(const TaxBill& bill) {
  if (!bill.due) return std::string();
  const std::string head = "Tax season. " + Money(bill.billed) + " on " +
                           Money(bill.taxableWas) + " of prize money.";
  if (bill.shortfall <= 0.0) return head;
  return head + " You could not cover " + Money(bill.shortfall) +
         " of it, and it waits.";
}

std::string TaxNews(const Tax& tax, int day) {
  if (tax.lastBillDay != day) return std::string();
  TaxBill bill;
  bill.due = true;
  bill.taxableWas = tax.lastTaxable;
  bill.billed = tax.lastBilled;
  bill.shortfall = tax.lastShortfall;
  bill.paid = tax.lastBilled - tax.lastShortfall;
  return TaxLine(bill);
}

std::string TaxWarning(const Tax& tax, int day, const TaxDials& dials) {
  if (tax.taxable <= 0.0) return std::string();
  const int until = DaysUntilTax(day, dials);
  if (until > dials.warnDays) return std::string();
  const double bill = std::round(tax.taxable * dials.rate);
  if (until == 0) {
    return "The reckoning is today: " + Money(bill) + " on the year's prize "
           "money.";
  }
  return "The reckoning is " +
         (until == 1 ? std::string("tomorrow")
                     : "in " + std::to_string(until) + " days") +
         ", and it is " + Money(bill) + ".";
}

}  // namespace dirtbag
