#include "DirtbagGymFloor.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {
namespace {

int Slot(GymRegular who) { return static_cast<int>(who); }

double FloorDraw(const std::string& label) {
  return Rng::FromSeed(label).NextDouble();
}

// **The cast, in the order the floor is read.** Wave one first, then the
// three cohorts, and the order inside each is the order the arcs come due:
// `NextMomentDue` breaks ties by this table, so this list is a design
// decision and not a list.
const GymRegularDef kCast[] = {
{GymRegular::Dale, "Dale", "the birthday-gift membership", 1, GymSetMix::AllComers,
 {"Dale, 52, membership was a birthday gift from his daughter. He has been "
  "standing under the easiest slab for twenty minutes, chalking up like it "
  "is a ritual. You talk him onto the wall. He gets two moves up and comes "
  "down grinning like he stole something.",
  "Dale topped out the green circuit today. All of it. He tells you this "
  "the way other men announce grandchildren. He is in four mornings a week "
  "now and knows every regular by name.",
  "Dale brings his daughter in -- the one who bought the membership. "
  "\"Show her the thing,\" he says, and flashes his slab. She films it. He "
  "has never been prouder of anything. Neither of them will ever cancel."},
 {"Dale is showing a nervous newcomer where the easy slab is, using almost "
  "exactly the words you used on him.",
  "Dale has a thermos and a folding stool by the slab wall now. Nobody has "
  "told him he cannot. Nobody is going to."},
 "Dale enters the beginner bracket, tops two, and gets the loudest cheer of "
 "the night by a mile."},

{GymRegular::Piper, "Piper", "the eleven-year-old", 1, GymSetMix::AllComers,
 {"Piper is eleven and warming up on things your strongest members project. "
  "Her parents hover in the lobby asking if you do coaching. She hangs the "
  "finish hold of the comp wall one-armed, bored, waiting for an answer.",
  "Piper flashed the setter's new line today in front of a full Tuesday "
  "crowd. The whole gym went quiet, then loud. Somebody has already posted "
  "it. Her mother asked if the youth team has a waitlist. There is no youth "
  "team. There might have to be.",
  "A regional coach came by to watch Piper climb. She pretended not to "
  "notice, then sent her project with the crowd watching and walked off "
  "like it was a warmup. \"This is her gym,\" her father tells you. \"She "
  "will not hear of anywhere else.\""},
 {"Piper is coaching a seven-year-old through a traverse with enormous "
  "seriousness, as though she were not fourteen herself.",
  "Piper has a competition next weekend and three members have already "
  "asked you for the address."},
 "Piper wins the whole thing -- not the youth category, the whole thing -- "
 "and gives her prize chalk to Dale."},

{GymRegular::June, "June", "the six a.m. regular", 1, GymSetMix::AllComers,
 {"June climbs at six a.m. before her nursing shift, alone, headphones in, "
  "methodical. Today she nods at you for the first time -- you have been "
  "officially noticed. She re-brushes every hold she uses. The morning crew "
  "is one person and it runs on rails.",
  "June asks you to spot her on the tall wall -- first words in three "
  "months. It turns out she is training for a granite line out west she has "
  "wanted since nursing school. She shows you a photo. It is beautiful and "
  "terrifying. The six a.m. sessions suddenly make sense.",
  "There is a postcard on the front desk: granite, blue sky, June's "
  "handwriting. \"Sent it. Couldn't have done it without the 6am wall. "
  "-- J.\" You pin it up. The morning crew is four people now -- word "
  "travels, even at that hour."},
 {"June is in at six as always. There are four of them now, and none of "
  "them talk, and it works.",
  "The postcard from the granite trip has a second postcard next to it. "
  "Different rock, same handwriting."},
 "June shows up on her night off, climbs one problem beautifully, and "
 "quietly judges the finals."},

{GymRegular::Bruno, "Bruno", "all power, no feet", 1, GymSetMix::AllComers,
 {"Bruno campuses everything. Everything. He is built like a vending "
  "machine and climbs like one falling downstairs. Today he asked, "
  "mid-flail, why his feet keep cutting. You point at his feet. This is "
  "going to take a while.",
  "Progress: Bruno did an entire route today WITH his feet, narrating every "
  "hold placement out loud like a golf commentator. Two members applauded. "
  "He took a bow and immediately fell off the down-climb jug.",
  "Bruno sent his first V4 tonight -- quiet feet, real technique, the whole "
  "thing. The noise he made brought the front desk running. Then he bought "
  "a round of chalk bags for everyone in the room. Loudest, most beloved "
  "man in the building."},
 {"Bruno is narrating his footwork to a new member, out loud, hold by hold, "
  "and the new member is actually getting it.",
  "Bruno bought the chalk bags again. You have asked him to stop. He will "
  "not stop."},
 "Bruno MCs the finals uninvited and is genuinely great at it. The crowd "
 "chants his name."},

// --- wave two: beginner-friendly walls ------------------------------------
{GymRegular::Marisol, "Marisol", "the after-work three", 2, GymSetMix::Beginner,
 {"Three women from the same office arrive together at six on the dot, in "
  "matching new shoes, and Marisol is clearly the one who made it happen. "
  "They do two easy routes and spend forty minutes talking. This is not a "
  "climbing session. It is better than that.",
  "The after-work three are now the after-work six. Marisol has a group "
  "chat and a spreadsheet of who is coming Thursday. She asks, slightly "
  "embarrassed, whether you do a group rate. You do now.",
  "Marisol's group did a whole evening on the tall wall without asking "
  "anybody for anything. On the way out she says the thing people say when "
  "they mean it: that this is the only place all week she is not somebody's "
  "manager."},
 {"Marisol's group has taken over two benches and a table and is planning "
  "something that is clearly not climbing.",
  "The after-work crowd arrives at six on the dot. You could set a clock by "
  "Marisol."},
 "Marisol enters her entire group as a team, under a name none of them will "
 "admit to choosing, and they come last and do not care."},

{GymRegular::Ade, "Ade", "terrified, and here anyway", 2, GymSetMix::Beginner,
 {"Ade signed up on a dare and has been climbing four feet off the ground "
  "for three weeks. Today they got to the halfway mark and had to be talked "
  "down, shaking. Then they got straight back on. That is the whole thing, "
  "right there.",
  "Ade topped a route today. Actually topped it. They sat on the mats "
  "afterwards for a long time not saying anything, and then asked, very "
  "quietly, what you would have to do to get good at this.",
  "Ade belays now -- properly, attentively, better than half the room. They "
  "caught a stranger's fall today and talked them through the lower like it "
  "was nothing. Nobody who watched would guess they were once frightened of "
  "the fourth bolt."},
 {"Ade is belaying somebody through their first real fall, calm as "
  "anything, saying the thing you said to them.",
  "Ade is on the tall wall without a fuss. You remember when four feet was "
  "the limit."},
 "Ade competes. Ade, who could not get past the fourth bolt in March. They "
 "do not place, and they are the story of the night anyway."},

{GymRegular::Horace, "Horace", "the man who fixes things", 2, GymSetMix::Beginner,
 {"Horace has been a member for a month and has already, unasked, repaired "
  "the loose bench, the sticky door and the wobbly hold on the yellow "
  "route. He climbs V1. He is having the time of his life.",
  "You find Horace under the auto-belay with a wrench and a look of deep "
  "contentment. He has 'just tidied up' the cable routing. It is, "
  "annoyingly, much better. He asks if you have a spare key. You think "
  "about it.",
  "Horace has a key. The place is visibly better maintained than you can "
  "afford for it to be, and he will not take a penny, and the one time you "
  "tried he was genuinely offended. He calls it his shed. It is his shed."},
 {"Horace has fixed something. You do not know what yet. You will find it, "
  "and it will be better.",
  "Horace is under the auto-belay again with the good wrench and an "
  "expression of total peace."},
 "Horace does not enter. Horace runs the scoring table, the raffle and the "
 "barbecue, and refuses a free membership for the fourth time."},

// --- wave two: all-comers -------------------------------------------------
{GymRegular::Nell, "Nell", "came back after twenty years", 2, GymSetMix::AllComers,
 {"Nell climbed hard in the nineties, stopped for a career and two kids, "
  "and walked back in today asking whether the grades had changed. They "
  "have. She got up a 5.9 and looked furious about it in a way you "
  "recognise.",
  "Nell has quietly worked her way back to the middle of the wall and is "
  "now the person the twenty-somethings ask about footwork. She pretends "
  "this is annoying. She has started arriving early.",
  "Nell led a route today for the first time since 1998, clipped every bolt "
  "like she never stopped, and then sat down on the mat and cried for about "
  "four seconds before pulling it together. She has asked about the outdoor "
  "trip."},
 {"Nell is telling a twenty-two-year-old what climbing was like in 1994, "
  "and he is listening, because she can still out-climb him.",
  "Nell is in early again, working the same corner, entirely happy."},
 "Nell quietly wins the masters bracket that you invented an hour before "
 "the comp specifically so she would enter it."},

{GymRegular::Tobias, "Tobias", "the boy with the headphones", 2, GymSetMix::AllComers,
 {"There is a teenager here every day after school, headphones in, climbing "
  "alone until closing. He has never spoken to anybody. He is also, "
  "quietly, one of the three strongest people in the building.",
  "You put a problem up that you knew would suit him. He did it, second go, "
  "and then -- for the first time in four months -- took one headphone out "
  "and asked who set it. The conversation lasted nine words. It was a "
  "landmark.",
  "Tobias has a crew now: three other kids who wait for him. They have "
  "in-jokes. He still wears the headphones, but only on the walk in, and he "
  "takes them out at the door without thinking about it."},
 {"Tobias and his crew have colonised the corner boulder and are laughing "
  "at something. The headphones are in his bag.",
  "Tobias set a problem for the others and is pretending not to watch them "
  "try it."},
 "Tobias makes the final, and his crew makes more noise than the rest of "
 "the room combined, and he pretends to hate it."},

{GymRegular::Esperanza, "Esperanza", "runs a business, hates it", 2, GymSetMix::AllComers,
 {"Esperanza takes calls in the lobby in a suit and then climbs for exactly "
  "fifty minutes with total focus. She books the same slot every week and "
  "has never once been late or stayed long.",
  "Esperanza was still here two hours past her slot today, in borrowed "
  "shoes, working a problem with two strangers. Her phone was in her bag "
  "the whole time. She looked at it on the way out like it was somebody "
  "else's.",
  "Esperanza sold her company. She told you before she told most people, "
  "standing at the desk with her chalk bag still on, and then she asked "
  "whether you needed help with the books. She would be very good at it. "
  "You said you would think about it."},
 {"Esperanza is here on a Tuesday afternoon, which she never used to be, "
  "doing nothing in particular.",
  "Esperanza has your books open on the lobby table and a pen behind her "
  "ear. She looks appallingly happy about it."},
 "Esperanza takes the afternoon off to run the desk for you, then enters at "
 "the last minute in borrowed shoes and makes the semis."},

// --- wave two: hardcore ---------------------------------------------------
{GymRegular::Kestrel, "Kestrel", "trains like it is a job", 2, GymSetMix::Hardcore,
 {"Kestrel does the same warm-up every single session, to the minute, and "
  "then gets on the board. She has a notebook. She asked what the board's "
  "angle actually is, to the degree, and did not accept 'about forty'.",
  "Kestrel has a spreadsheet of every session in your gym for four months "
  "and offered to show you which holds nobody touches. She is right about "
  "all of it. You have quietly changed two things because of her.",
  "Kestrel sent the hardest thing anyone has ever done in this building, on "
  "a Tuesday morning, with three people watching. She wrote it in the "
  "notebook, closed the notebook, and went to work. She will be back at "
  "six."},
 {"Kestrel is on the board, on schedule, notebook open. The warm-up was "
  "exactly the same as always.",
  "Kestrel has left a note at the desk about two holds that need replacing. "
  "She is right."},
 "Kestrel wins by a margin that is almost rude, thanks nobody, and is back "
 "on the board at seven the next morning."},

{GymRegular::Dom, "Dom", "strong, fragile, learning", 2, GymSetMix::Hardcore,
 {"Dom can do more pull-ups than anyone here and has been injured twice "
  "this year. Today he asked you -- without being prompted -- whether it "
  "was normal for a finger to feel like that. It is not. You told him.",
  "Dom has been doing the boring rehab for six weeks and hating every "
  "minute and doing it anyway. He is climbing less and better. He tells "
  "everyone about it now, with the fervour of the converted.",
  "Dom has been unhurt for a year, which for him is unprecedented. He runs "
  "an informal warm-up circle before the evening session and is insufferable "
  "about tendon health, and two of the strong kids listen to him. That is "
  "two injuries that will not happen."},
 {"Dom is running the warm-up circle and telling three strong kids about "
  "tendons. They are, astonishingly, listening.",
  "Dom is climbing well within himself and looks like a man who intends to "
  "still be here at fifty."},
 "Dom does not compete -- he is mid-block and he says so out loud, twice -- "
 "and instead spends the night talking three kids out of doing something "
 "stupid."},

{GymRegular::Rafferty, "Rafferty", "the old guard, unconvinced", 2, GymSetMix::Hardcore,
 {"Rafferty has been climbing since before this building was a gym and has "
  "opinions about the boards, the grades, the music, and the concept of a "
  "coffee bar. He is here five days a week. He has not paid a membership on "
  "time yet.",
  "Rafferty told you a story today about a route he did in 1987 that you "
  "have actually heard of, and then -- without making anything of it -- "
  "corrected a kid's footwork so gently the kid thought he had worked it "
  "out himself.",
  "Rafferty brought in a box of slides. Actual slides. Half the gym stayed "
  "late for it. He grumbled the entire time and set the whole thing up an "
  "hour early, and he has started paying his membership on the first, "
  "without being asked."},
 {"Rafferty is complaining about the music. He has also fixed the sign-in "
  "sheet, straightened the mats, and made two people feel welcome.",
  "Rafferty is telling the 1987 story again. Two members have not heard it. "
  "He is delighted."},
 "Rafferty judges, complains about the format, and awards a prize he paid "
 "for himself to the youngest person there."},
};

constexpr int kCastCount = static_cast<int>(sizeof(kCast) / sizeof(kCast[0]));

// The line the cohort walks in on -- what a set mix collects, said out
// loud, which is the only place the mix's most permanent consequence is
// visible at the moment it happens.
const char* CohortLine(GymSetMix mix) {
  switch (mix) {
    case GymSetMix::Beginner:
      return "Word got round that this is a place you can be bad at "
             "climbing. Three new faces this week, and none of them can "
             "climb, and all of them came back.";
    case GymSetMix::Hardcore:
      return "The word on your walls has reached the people who care about "
             "walls. Three new regulars this week, and every one of them "
             "has a project already.";
    default:
      return "The room has filled out with people who do not have much in "
             "common except the hour they spend here. Three new regulars "
             "this week. Different lives, same wall.";
  }
}

// Ten and ten, multiplied out to a hundred names that never repeat inside
// a career. The stride of three on the second word keeps the pairs from
// marching in lockstep.
const char* kLineA[] = {"Velvet", "Static", "Dust Bowl", "Neon", "Sandbag",
                        "Porcelain", "Thunder", "Slab Cabin", "Mono",
                        "Full Value"};
const char* kLineB[] = {"Sermon", "Arete", "Shuffle", "Gospel", "City",
                        "Express", "Physics", "Roulette", "Standard",
                        "Diner"};
constexpr int kLineACount = static_cast<int>(sizeof(kLineA) / sizeof(kLineA[0]));
constexpr int kLineBCount = static_cast<int>(sizeof(kLineB) / sizeof(kLineB[0]));

}  // namespace

const GymRegularDef* GymRegularOf(GymRegular who) {
  if (who == GymRegular::None) return nullptr;
  for (int i = 0; i < kCastCount; i++) {
    if (kCast[i].who == who) return &kCast[i];
  }
  return nullptr;
}

std::vector<GymRegular> TheCast(const GymFloor& floor) {
  std::vector<GymRegular> out;
  for (int i = 0; i < kCastCount; i++) {
    if (kCast[i].wave == 1) {
      out.push_back(kCast[i].who);
    } else if (floor.waveTwoArrived && kCast[i].cohort == floor.waveTwo) {
      out.push_back(kCast[i].who);
    }
  }
  return out;
}

bool WaveOneIsLivedOut(const GymFloor& floor) {
  for (int i = 0; i < kCastCount; i++) {
    if (kCast[i].wave != 1) continue;
    if (floor.stage[Slot(kCast[i].who)] < kArcStages) return false;
  }
  return true;
}

GymRegular NextMomentDue(const GymFloor& floor) {
  GymRegular best = GymRegular::None;
  int bestStage = kArcStages;
  for (GymRegular who : TheCast(floor)) {
    const int st = floor.stage[Slot(who)];
    if (st >= kArcStages) continue;
    if (best == GymRegular::None || st < bestStage) {
      best = who;
      bestStage = st;
    }
  }
  return best;
}

std::string AmbientLine(GymRegular who, int day) {
  const GymRegularDef* def = GymRegularOf(who);
  if (def == nullptr) return "";
  const int which = static_cast<int>(
      FloorDraw("gymafter|" + std::string(def->name) + "|" +
                std::to_string(day)) * 2) % 2;
  return def->after[which];
}

std::string SetterLineOfTheWeek(const Gym& gym, int day) {
  if (!gym.owned || !gym.setter) return "";
  const int week = std::max(0, day) / 7;
  // **Ten by ten really is a hundred, but only if the second index is not
  // linear in the week.** The source strides the second word by three,
  // which cannot help: both indices are then linear in `week` modulo ten,
  // so the *pair* repeats every ten weeks and a thirty-year career sees ten
  // route names, over and over, a hundred and fifty times each. Indexing
  // the second word by the decade of weeks instead makes the cycle a
  // hundred weeks, which is about two years. Caught by the test that
  // asserted a hundred and got ten.
  const std::string named =
      std::string(kLineA[week % kLineACount]) + " " +
      kLineB[(week / kLineACount) % kLineBCount];
  const GymStaffer* who = WhoIsOn(gym, false);
  if (who == nullptr || who->name.empty()) {
    return "New this week: \"" + named + "\".";
  }
  return who->name + " put \"" + named + "\" up this week.";
}

FloorWalk WalkTheFloor(GymFloor& floor, Gym& gym, int day,
                       const FloorDials& dials) {
  FloorWalk out;
  if (!gym.owned) return out;
  // Once a day. Let the members breathe.
  if (floor.lastWalkDay == day) return out;
  floor.lastWalkDay = day;
  out.walked = true;

  // **The cohort first.** Wave one lived out is the trigger, and the mix on
  // the walls *that day* decides who walks in -- after which it is theirs.
  if (WaveOneIsLivedOut(floor) && !floor.waveTwoArrived) {
    floor.waveTwoArrived = true;
    floor.waveTwo = gym.mix;
    gym.members += dials.memberBump;
    out.members = dials.memberBump;
    out.psyche = dials.waveTwoPsyche;
    out.cohortArrived = true;
    out.said = CohortLine(floor.waveTwo);
    return out;
  }

  const GymRegular due = NextMomentDue(floor);
  if (due == GymRegular::None) {
    // Every arc is lived, and the floor is still full of them. **One line,
    // not all of them** -- a walk that reads out thirteen people is a
    // dashboard, and the source records the same lesson arriving as a word
    // count that split its own toast in two.
    const std::vector<GymRegular> cast = TheCast(floor);
    out.psyche = dials.ambientPsyche;
    if (!cast.empty()) {
      const int which = static_cast<int>(
          FloorDraw("gymwalk|" + std::to_string(day)) * cast.size()) %
          static_cast<int>(cast.size());
      out.who = cast[static_cast<size_t>(which)];
      out.said = AmbientLine(out.who, day);
    }
    if (out.said.empty()) {
      out.said = "You walk the floor. It is full, and it is yours.";
    }
    return out;
  }

  const GymRegularDef* def = GymRegularOf(due);
  const int stage = floor.stage[Slot(due)];
  floor.stage[Slot(due)] = stage + 1;
  gym.members += dials.memberBump;
  out.who = due;
  out.members = dials.memberBump;
  out.psyche = dials.walkPsyche;
  out.said = def != nullptr ? def->stages[stage] : "";
  return out;
}

std::string WhyNotACompNight(const GymFloor& floor, const Gym& gym,
                             double cash, int day, const FloorDials& dials) {
  if (!gym.owned) return "";
  if (cash < dials.compCost) {
    return "A comp night runs $" +
           std::to_string(static_cast<int>(dials.compCost)) +
           " -- pizza, prizes, staff hours.";
  }
  if (gym.members < dials.compMinMembers) {
    return "You need " + std::to_string(static_cast<int>(dials.compMinMembers)) +
           " members before a comp night draws a room. Build the base first.";
  }
  const int left = dials.compCooldown - (day - floor.lastCompDay);
  if (left > 0) {
    return "Too soon after the last one -- give it " + std::to_string(left) +
           (left == 1 ? " more day. " : " more days. ") +
           "Scarcity is the draw.";
  }
  return "";
}

CompNight HostACompNight(GymFloor& floor, Gym& gym, double& cash,
                         const Rng& worldRng, int day,
                         const FloorDials& dials) {
  CompNight out;
  // **Owning it is checked here and not in the refusal**, because the
  // refusal answers "why is this button greyed out" and a career with no
  // gym is not looking at the button. A gate that borrows another
  // function's empty string for "yes" is a gate that opens by accident,
  // which is what the test caught.
  if (!gym.owned) return out;
  if (!WhyNotACompNight(floor, gym, cash, day, dials).empty()) return out;

  // Its own derived stream, so the night's turnout cannot shift the floor's
  // daily noise or anybody else's roll.
  Rng rng = worldRng.Derive("compnight#" + std::to_string(day));
  const double signups =
      std::round(rng.FloatRange(dials.compSignupsMin, dials.compSignupsMax));

  // **The takings are a function of the membership you built.** Roughly
  // sixty per cent of the room enters, at eight dollars a head -- so comp
  // night pays out of the business rather than out of nowhere, and a bigger
  // room is a better night.
  const double fees =
      std::round(gym.members * dials.compTurnout) * dials.compEntry;
  cash -= dials.compCost;
  gym.members += signups;
  floor.lastCompDay = day;

  out.held = true;
  out.takings = fees;
  out.cost = dials.compCost;
  out.members = signups;
  out.standing = dials.compStanding;
  out.psyche = dials.compPsyche;

  // The night's story is one of the people whose arc you have actually
  // lived. A table rather than a chain, because the source's chain ended on
  // Bruno and printed his line for every wave-two star.
  const std::vector<GymRegular> cast = TheCast(floor);
  std::vector<GymRegular> lived;
  for (GymRegular who : cast) {
    if (floor.stage[Slot(who)] >= kArcStages) lived.push_back(who);
  }
  if (lived.empty()) {
    out.said = "A good crowd, close finals, and the kind of noise that makes "
               "the room feel like a scene.";
    return out;
  }
  const GymRegularDef* star =
      GymRegularOf(lived[static_cast<size_t>(day) % lived.size()]);
  out.said = star != nullptr ? star->compStar : "";
  return out;
}

std::string FloorLine(const GymFloor& floor, const Gym& gym, int day) {
  if (!gym.owned) return "";
  std::string out;
  const GymRegular due = NextMomentDue(floor);
  if (due != GymRegular::None) {
    const GymRegularDef* def = GymRegularOf(due);
    if (def != nullptr) {
      out = std::string(def->name) + ", " + def->tag;
    }
  } else {
    // Nobody is due, so the line says what the floor is instead of naming
    // somebody -- the readout stays quiet about people it has nothing new
    // to say about.
    int lived = 0;
    for (GymRegular who : TheCast(floor)) {
      if (floor.stage[Slot(who)] >= kArcStages) lived++;
    }
    out = std::to_string(lived) + " regulars, every one of them a story you "
                                  "were there for";
  }
  const std::string setting = SetterLineOfTheWeek(gym, day);
  if (!setting.empty()) out += "  --  " + setting;
  return out;
}

}  // namespace dirtbag
