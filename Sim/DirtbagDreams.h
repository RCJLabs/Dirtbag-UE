#pragma once

// Dreams: the thing the money is for.
//
// `concepts/DIRTBAG.md` section 4 lists "dreams (Rig/War Chest/Home Base)"
// among the systems to port wholesale and never defines them, so the rules
// below are a design rather than a port and are logged as one.
//
// The shape came out of a measurement (notes/dreams-what-money-is-worth.md)
// that killed the obvious version. Money is not scarce: a career that banks
// hard climbs **970 days against 422** and ends with sixty thousand dollars,
// because a float stops you bodging the van and bodging is what costs you
// your season. Saving is not a sacrifice; it is the best climbing decision
// in the game.
//
// So a dream cannot cost only money -- that would be a timer. **A dream
// costs the buffer.** You spend the float that was keeping the van alive,
// and the reward arrives with a lean, fragile year attached: back to
// bodging, back to being stranded, until you rebuild. The decision is
// *when*, never *whether*, which is exactly the shape of a real climber
// buying a van they cannot quite afford.
//
// Each dream buys a different currency, so the choice is not a ladder:
//
//   the Rig        van uptime -- parts that last, and one that starts new
//   the War Chest  time       -- a year you do not have to work at all
//   Home Base      the body   -- a bed, a shower, and no frost on the inside
//
// Engine-free like everything in Sim/.

#include <string>

#include "DirtbagVan.h"

namespace dirtbag {

enum class Dream { None, Rig, WarChest, HomeBase };
constexpr int kDreamCount = 3;

const char* DreamName(Dream d);

// What it is actually for, in the second person. The HUD and the shop both
// want this and neither should be writing it.
const char* DreamBlurb(Dream d);

struct DreamDials {
  // Priced off the savings sweep, not off vibes. A career passes $10,000
  // comfortably and $25,000 is where the climbing curve flattens, so the
  // Rig lands mid-career, the War Chest is a real decision, and Home Base
  // is the end of one. Every price is well above the float that keeps the
  // van healthy, which is the whole design: buying drops you back into the
  // trap you climbed out of.
  double cost[kDreamCount] = {9000.0, 14000.0, 30000.0};

  // What the Rig does to wear lives on `VanDials::rigLifeMultiplier`, not
  // here: DriveVan is what applies it and DriveVan has the van's dials.
  // Two copies of one number is how dials drift apart.

  // The War Chest: a year of not working. Not a lump you spend down -- a
  // year you have already bought, which is the dirtbag version of rich.
  int warChestDays = 365;

  // Home Base: rent, forever, whether or not you are there. The only
  // ongoing cost any dream carries, because an address is the one that
  // does not stop asking.
  double homeBaseRentPerDay = 22.0;
  // And what it buys: a night indoors is worth about a third again on skin.
  double homeBaseSkinBonus = 0.5;
};

struct Dreams {
  bool has[kDreamCount] = {false, false, false};

  // What you have said you are saving for. Free to declare, free to change,
  // and it changes nothing on its own -- it is a note to yourself that the
  // HUD can read back. Declaring it is not the commitment; buying is.
  Dream working = Dream::None;

  // The War Chest, being lived.
  int seasonOffDaysLeft = 0;
};

bool HasDream(const Dreams& dreams, Dream d);
double CostOf(Dream d, const DreamDials& dials = DreamDials{});

// Can you put the money down today? Says nothing about whether you should.
bool CanAfford(const Dreams& dreams, Dream d, double cash,
               const DreamDials& dials = DreamDials{});

// Buy it. Spends the cash -- all of it, if that is what it takes -- and
// applies whatever the dream does. False if you cannot afford it or have it
// already. The van is passed because the Rig is a van, and a Rig arrives
// with everything new.
bool BuyDream(Dreams& dreams, Van& van, double& cash, Dream d,
              const DreamDials& dials = DreamDials{});

// A day of having them: rent comes off, and a bought year runs down.
void DreamDay(Dreams& dreams, double& cash, double& owed,
              const DreamDials& dials = DreamDials{});

// Is this a day you genuinely do not have to take a gig? Only the War
// Chest ever says yes, and only while it lasts.
bool NoNeedToWork(const Dreams& dreams);

// Extra skin per night from sleeping somewhere with a door.
double SkinBonus(const Dreams& dreams, const DreamDials& dials = DreamDials{});

// "The Rig, and Home Base. 212 days of the War Chest left." Empty until
// there is something to say.
std::string DreamText(const Dreams& dreams);

}  // namespace dirtbag
