// The town block, as data.
//
// Content is data, never code (CLAUDE.md), and a town is the most obviously
// data-shaped thing in the game: a list of places, each with hours, prices,
// and one thing it is for. Adding the bakery should be a row, not a commit
// to the day loop.
//
// Every venue here is something the player already needed and was getting
// from thin air. Meals came from nowhere at a flat $8; shoes were resoled by
// an implied shop; the gym existed as a venue with no door. This gives them
// addresses, opening hours, and prices that differ — which turns "eat" into
// "eat where", and that is the whole point of a town.
//
// Opening hours are the mechanic that makes it more than a price list. The
// diner shuts at nine and the gear shop keeps banker's hours, so a day that
// ran long is a day you eat badly, and the drive into town has to happen
// while the shop is open — which competes with the window.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCore.h"

namespace dirtbag {

// What a place is for. One primary service each: a venue that does
// everything is a menu, not a place.
enum class Service {
  Meal,        // the diner, the gas station
  Gear,        // resoles and rubber
  VanRepair,   // the garage, when you would rather not bodge it
  Gym,         // plastic, and the only climbing that ignores the weather
  Work,        // somewhere that hires by the shift
};

const char* ServiceName(Service s);

struct Venue {
  std::string name;
  Service service = Service::Meal;
  double opensAt = 8.0;
  double closesAt = 18.0;

  // What it costs relative to the baseline the day loop already uses. The
  // gas station is cheap and grim; the diner is dear and actually feeds you.
  double priceFactor = 1.0;
  double qualityFactor = 1.0;

  // Hours from the Lot. Town is a drive, and a drive is van wear.
  double travelHours = 0.4;

  // A line of who they are, in the game's register.
  std::string flavour;
};

struct Town {
  std::string name;
  std::vector<Venue> venues;
};

// The one town this valley has.
Town DirtbagTown();

// Is it open at this hour? Places that wrap past midnight are not a thing
// here, which keeps this honest and simple.
bool IsOpen(const Venue& v, double hour);

// The venues offering a service, in book order. Empty is a real answer.
std::vector<const Venue*> VenuesFor(const Town& town, Service service);

// The best open venue for a service right now, cheapest first among those
// that are open — or null if the town is shut.
const Venue* OpenVenueFor(const Town& town, Service service, double hour);

// "The Ridgeline Diner — open until 21:00" / "closed until 07:00"
std::string VenueText(const Venue& v, double hour);

}  // namespace dirtbag
