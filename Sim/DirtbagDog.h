// The dog.
//
// Every dirtbag crag has one, and it is nobody's until it is yours. This one
// is a stray around the Lot: feed it enough times and it stops being a stray,
// which is the only adoption ceremony the life provides.
//
// It is deliberately not a mood modifier with fur. The sharp part is that a
// van gets hot, and the game already knows exactly how hot, because the
// conditions system computes the day's temperature hour by hour. So a dog
// gives you a reason to care about the forecast on days you were not going
// to climb anyway, and puts a cost on the shift that pays your bills. There
// is no lock: you can leave it and work. It just costs you something, which
// is how the rest of this game says no.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>

#include "DirtbagCore.h"

namespace dirtbag {

struct DogDials {
  // A stray turns up hungry. Feeding costs about what a burrito does, and
  // three or four square meals is the whole adoption process.
  double foodCost = 3.0;
  double fedPerMeal = 0.5;
  double hungerPerDay = 0.3;

  // Bond, and what it takes. Feeding a stray earns more than feeding your
  // own dog does, because the first ones are the ones that count.
  double bondPerMeal = 0.18;
  double bondPerDayTogether = 0.05;
  double bondDecayAlone = 0.02;
  double adoptAtBond = 0.5;

  // Company, priced like the Lot's: psyche is ability in the resolver, so a
  // dog is worth something and never worth a grade.
  double psycheFromBond = 0.12;

  // A hungry dog is a bad day for everyone.
  double psycheWhenHungry = 0.2;
  double hungryBelow = 0.35;

  // The van, on a warm day. Not a hard rule — you can leave it and go to
  // work, and the game will not stop you. It will just be true that you
  // did, and psyche is where that lands.
  double warmVanF = 78.0;
  double guiltAtWarm = 0.15;
  double guiltPerTenDegrees = 0.12;
};

struct Dog {
  std::string name = "the dog";
  bool adopted = false;    // a stray until you have fed it enough
  double bond = 0.0;       // 0..1
  double fed = 0.4;        // 0..1; a stray turns up hungry
};

// Feed it. Returns false if you cannot afford to, which happens.
bool FeedDog(Dog& dog, double& cash, const DogDials& dials = DogDials{});

// A day passes: it gets hungrier, and the bond moves depending on whether
// you were around.
void DogDay(Dog& dog, bool together, const DogDials& dials = DogDials{});

// What having it around does to your head. Negative when it is hungry,
// because that is also true.
double DogPsyche(const Dog& dog, const DogDials& dials = DogDials{});

// Leaving it in the van at this temperature, for a few hours. Returns what
// it costs you in psyche — zero on a cool day, and rising fast on a warm
// one. The game never refuses; it just tells the truth about the number.
double VanGuilt(const Dog& dog, double vanTempF,
                const DogDials& dials = DogDials{});

// Whether the van is somewhere a dog can be left today.
bool VanIsSafe(double vanTempF, const DogDials& dials = DogDials{});

// The dog in the game's voice: "the dog is asleep under the van" /
// "the dog has not eaten since yesterday".
std::string DogText(const Dog& dog, const DogDials& dials = DogDials{});

}  // namespace dirtbag
