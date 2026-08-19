#include "DirtbagTown.h"

#include <algorithm>
#include <cstdio>

namespace dirtbag {

const char* ServiceName(Service s) {
  switch (s) {
    case Service::Meal:      return "food";
    case Service::Gear:      return "gear";
    case Service::VanRepair: return "the garage";
    case Service::Gym:       return "the gym";
    case Service::Work:      return "work";
  }
  return "something";
}

Town DirtbagTown() {
  // Authored, because a town is content. Prices and hours are the whole
  // characterisation: you learn what a place is by when it is open and what
  // it charges, long before anyone says a word.
  Town t;
  t.name = "Kerrick";
  t.venues = {
      {"the gas station", Service::Meal, 5.0, 23.0, 0.75, 0.7, 0.3,
       "hot food in a warmer, and it has been in there a while"},
      {"the Ridgeline Diner", Service::Meal, 7.0, 21.0, 1.4, 1.35, 0.45,
       "eggs, and somebody who remembers what you ordered last time"},
      {"Kerrick Outdoor", Service::Gear, 9.0, 17.0, 1.0, 1.0, 0.45,
       "resoles in a week, and an opinion about your footwork"},
      {"the garage on Third", Service::VanRepair, 8.0, 17.0, 1.0, 1.0, 0.5,
       "they will not laugh at the bodge, but they will look at it"},
      {"the climbing gym", Service::Gym, 6.0, 22.0, 1.0, 1.0, 0.5,
       "plastic, warm, and open when the rock is not"},
      {"the gym front desk", Service::Work, 9.0, 21.0, 1.0, 1.0, 0.5,
       "four hours of scanning passes and telling people where the chalk is"},
  };
  return t;
}

bool IsOpen(const Venue& v, double hour) {
  return hour >= v.opensAt && hour < v.closesAt;
}

std::vector<const Venue*> VenuesFor(const Town& town, Service service) {
  std::vector<const Venue*> out;
  for (const Venue& v : town.venues) {
    if (v.service == service) out.push_back(&v);
  }
  return out;
}

const Venue* OpenVenueFor(const Town& town, Service service, double hour) {
  const Venue* best = nullptr;
  for (const Venue& v : town.venues) {
    if (v.service != service || !IsOpen(v, hour)) continue;
    // Cheapest that is open. A dirtbag orders by price and everyone knows
    // it; wanting the good one is what makes the diner a decision.
    if (!best || v.priceFactor < best->priceFactor) best = &v;
  }
  return best;
}

std::string VenueText(const Venue& v, double hour) {
  char buf[64];
  if (IsOpen(v, hour)) {
    std::snprintf(buf, sizeof(buf), " — open until %02d:%02d",
                  static_cast<int>(v.closesAt),
                  static_cast<int>((v.closesAt - static_cast<int>(v.closesAt)) *
                                   60.0));
    return v.name + buf;
  }
  std::snprintf(buf, sizeof(buf), " — closed until %02d:%02d",
                static_cast<int>(v.opensAt),
                static_cast<int>((v.opensAt - static_cast<int>(v.opensAt)) *
                                 60.0));
  return v.name + buf;
}

}  // namespace dirtbag
