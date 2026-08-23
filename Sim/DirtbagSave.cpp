#include "DirtbagSave.h"

#include <algorithm>

#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace dirtbag {

namespace {

// %.17g round-trips an IEEE double exactly — the same discipline as the
// RNG golden vectors. Locale-independent by construction ('.' only).
std::string NumToStr(double v) {
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.17g", v);
  return buf;
}

std::string IntToStr(int v) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%d", v);
  return buf;
}

bool ParseDouble(const SaveFields& f, const std::string& key, double& out) {
  auto it = f.find(key);
  if (it == f.end()) return false;
  char* end = nullptr;
  out = std::strtod(it->second.c_str(), &end);
  return end && *end == '\0' && !it->second.empty();
}

bool ParseInt(const SaveFields& f, const std::string& key, int& out) {
  auto it = f.find(key);
  if (it == f.end()) return false;
  char* end = nullptr;
  const long v = std::strtol(it->second.c_str(), &end, 10);
  if (!end || *end != '\0' || it->second.empty()) return false;
  out = static_cast<int>(v);
  return true;
}

bool ParseString(const SaveFields& f, const std::string& key, std::string& out) {
  auto it = f.find(key);
  if (it == f.end()) return false;
  out = it->second;
  return true;
}

std::string ProjKey(int i, const char* field) {
  return "project." + IntToStr(i) + "." + field;
}

std::string BondKey(int i, const char* field) {
  return "bond." + IntToStr(i) + "." + field;
}

}  // namespace

namespace {

// v1 → v2: project ledgers gained the route's guidebook grade. Old ledgers
// name routes that may not exist any more (a gym resets its walls), so the
// honest answer is "unknown" — a guess here would put fictional grades in
// somebody's career history, which is exactly the data you can't fake.
void MigrateV1ToV2(SaveFields& fields) {
  int count = 0;
  if (!ParseInt(fields, "projects", count)) return;
  for (int i = 0; i < count; i++) {
    fields[ProjKey(i, "grade")] = "-1";
  }
}

// v2 → v3: ledgers gained the first-ascent record. Every line a v2 career
// touched was one that already existed in a book, so all of them are clean,
// none of them is a first ascent, and no grade needs confirming — the
// honest defaults, and the reason this migration can be exact rather than a
// guess.
void MigrateV2ToV3(SaveFields& fields) {
  int count = 0;
  if (!ParseInt(fields, "projects", count)) return;
  for (int i = 0; i < count; i++) {
    fields[ProjKey(i, "clean")] = "1";
    fields[ProjKey(i, "given")] = "";
    fields[ProjKey(i, "fa")] = "0";
    fields[ProjKey(i, "confirmed")] = "-1";
  }
}

// v3 → v4: careers gained the people they know. A v3 career knew nobody,
// which is precisely what an empty list means — no guessing required.
void MigrateV3ToV4(SaveFields& fields) {
  fields["bonds"] = "0";
}

// v5 → v6: shoes and the van. A v5 career wore neither, so both arrive new.
// Generous rather than exact on purpose: the honest alternative would be
// inventing wear nobody earned, and a career that predates a mechanic has
// not been dodging it.
void MigrateV5ToV6(SaveFields& fields) {
  fields["shoes.wear"] = "0";
  fields["shoes.resoles"] = "0";
  fields["shoes.pairs"] = "1";
  fields["van.hours"] = "0";
  for (int i = 0; i < 6; i++) {
    const std::string k = "van." + IntToStr(i) + ".";
    fields[k + "wear"] = "0";
    fields[k + "patches"] = "0";
    fields[k + "failed"] = "0";
  }
}

// v7 → v8: the scene. A v7 career had none to stand with, so it arrives
// where a new one does: nobody's friend, nobody's problem, crag open.
void MigrateV7ToV8(SaveFields& fields) {
  for (int i = 0; i < 4; i++) {
    fields["standing." + IntToStr(i)] = "0";
  }
  fields["standing.closed"] = "0";
}

// v8 → v9: the job. Standing was saved from v8 and the job was not, so a
// v8 career woke up unemployed every time it loaded — which is not a
// migration problem, it is the bug this version exists to fix. A v8 save
// arrives as what it actually was: whatever days it had worked are gone,
// and it is not salaried, because it could not have been.
void MigrateV8ToV9(SaveFields& fields) {
  fields["job.salaried"] = "0";
  fields["job.days"] = "0";
  fields["job.weeks"] = "0";
}

// v9 → v10: the kit. A v9 career owned no pads, no board and no
// membership, which is exactly what a new one owns — so it arrives with
// nothing, and the first thing it can do about that is go to work.
void MigrateV9ToV10(SaveFields& fields) {
  fields["kit.pads"] = "1";   // it always had the one, it just never said so
  fields["kit.hangboard"] = "0";
  fields["kit.membership"] = "0";
}

// v10 → v11: the body's second ledger. A v10 career had no training load
// and could not be hurt, so it arrives exactly as it played: rested, and in
// one piece. Which is honest — it never took a risk it did not know about.
void MigrateV10ToV11(SaveFields& fields) {
  fields["load"] = "0";
  fields["injury.active"] = "0";
  fields["injury.kind"] = "0";
  fields["injury.severity"] = "0";
  fields["injury.days"] = "0";
  fields["physio.last"] = "0";
}

// v11 → v12: the ledger learned which ladder its line is on, and careers
// gained the ones that came before. A v11 save predates the rope crag
// entirely, so every line it ever touched was a boulder — which is not a
// guess, it is the only thing that could have been true.
void MigrateV11ToV12(SaveFields& fields) {
  int count = 0;
  if (ParseInt(fields, "projects", count)) {
    for (int i = 0; i < count; i++) {
      fields[ProjKey(i, "disc")] = "0";   // Discipline::Boulder
    }
  }
  fields["legacies"] = "0";   // it is the first life; nobody came before
}

// v12 → v13: who pays you. A v12 career had nobody calling, which is both
// the truth and what a fresh Sponsorship already is.
void MigrateV12ToV13(SaveFields& fields) {
  fields["sponsor.tier"] = "0";
  fields["sponsor.seasons"] = "0";
  fields["sponsor.lastgrade"] = "-1";
  fields["sponsor.stale"] = "0";
  fields["sponsor.hurtdays"] = "0";
}

// v13 → v14: what you did that nobody saw. A v13 career carried nothing,
// which is the honest answer and the common one.
void MigrateV13ToV14(SaveFields& fields) { fields["secrets"] = "0"; }

// v14 → v15: days hurt since the last review. A v14 career had never been
// reviewed at all — ReviewSeason was written and called by nothing, in the
// engine and in the probe alike — so migrating to zero is not an
// approximation, it is the truth: nobody has ever looked at them.
void MigrateV14ToV15(SaveFields& fields) { fields["sponsor.hurtdays"] = "0"; }

// v15 -> v16: the Dirtbag Year. An old save has no record of whether its
// streak was running, and there is no honest way to reconstruct one -- a
// climber who has never been salaried might be four hundred days in, and
// the file does not say. So every old save starts its first streak today.
// That loses history rather than inventing it, which is the right way round:
// awarding somebody a year they may not have lived would put a line in their
// legacy that never happened.
void MigrateV15ToV16(SaveFields& fields) {
  fields["job.sincesalary"] = "0";
  fields["job.dirtbagyears"] = "0";
  fields["job.longeststreak"] = "0";
}

// v16 -> v17: the crew. An old save has no name, which is the correct
// answer rather than a lossy one -- the town had not said it yet because
// the system did not exist. The bonds that earn one are already saved, so
// an existing career starts its month from today and gets named on the
// far side of it, exactly as a new one would.
void MigrateV16ToV17(SaveFields& fields) {
  fields["crew.name"] = "";
  fields["crew.namedon"] = "0";
  fields["crew.days"] = "0";
  fields["crew.members"] = "0";
}

// v17 -> v18: dreams, and the Rig flag on the van. An old career owns
// nothing and is saving for nothing, which is exactly right -- there was
// nothing to own or save for. `van.rig` is false for the same reason: the
// van they have is the van they started with, whatever they have spent on
// it, because a Rig is a van you bought rather than a van you maintained.
void MigrateV17ToV18(SaveFields& fields) {
  fields["dreams.rig"] = "0";
  fields["dreams.warchest"] = "0";
  fields["dreams.homebase"] = "0";
  fields["dreams.working"] = "0";
  fields["dreams.seasonoff"] = "0";
  fields["van.rig"] = "0";
}

// v18 -> v19: dreams are chosen, not browsed. The old `dreams.working` was
// a free note-to-self; `chosen` is a binding, once-per-career commitment.
// A v18 note does NOT become the commitment -- binding somebody to a thing
// they idly clicked last month is exactly the kind of retroactive promise
// a migration must never make. Every old career arrives unchosen and makes
// the choice for real, keeping whatever it already bought under the old
// rules. The stale `dreams.working` key is simply no longer read.
void MigrateV18ToV19(SaveFields& fields) { fields["dreams.chosen"] = "0"; }

// v19 -> v20: the player's name reaches the sim (the crew hash needs it so
// the town names each generation's crew rather than re-issuing the last
// one's). Old saves arrive unnamed, which costs nothing: a crew already
// named stays named, and an unnamed career simply salts the hash with
// nothing, exactly as every career did before names existed.
void MigrateV19ToV20(SaveFields& fields) { fields["player.name"] = ""; }

// v20 → v21: who your climber is. A v20 career predates the four questions,
// so it arrives **unbuilt** — and that is exact rather than generous,
// because an unbuilt character is neutral in every lane. The climber you
// had keeps the numbers they had, and the creation screen does not ambush
// somebody twenty years into a career.
// v21 -> v22: somebody to beat. A v21 career had nobody, so it migrates to
// an **empty** rival -- and `SleepToNextDay` skips a rival with no name, so
// the career plays exactly as it did rather than having a stranger appear
// twenty years in.
// v22 -> v23: the line they are on. v22 shipped a day before the race
// existed, so a v22 career migrates to nothing running -- which is where a
// career sits most of the time anyway, and the roll starts it again the next
// morning.
// v23 -> v24: national ranking points. A v23 career never entered a comp,
// so it starts unranked -- which is the Local tier, where an unranked
// climber belongs.
void MigrateV23ToV24(SaveFields& fields) { fields["ranking"] = "0"; }

void MigrateV22ToV23(SaveFields& fields) {
  fields["rival.race"] = "";
  fields["rival.raceby"] = "0";
  fields["rival.racefa"] = "0";
}

void MigrateV21ToV22(SaveFields& fields) {
  fields["rival.name"] = "";
  fields["rival.style"] = "0";
  fields["rival.vibe"] = "0";
  fields["rival.gen"] = "0";
  fields["rival.born"] = "1";
  fields["rival.startage"] = "26";
  fields["rival.grade"] = "0";
  fields["rival.laststep"] = "0";
  fields["rival.peak"] = "0";
  fields["rival.rivalry"] = "0";
  fields["rival.allied"] = "0";
  fields["rival.offered"] = "0";
  fields["rival.met"] = "0";
  fields["rival.retired"] = "0";
  fields["rival.fas"] = "0";
  fields["pastrivals"] = "0";
}

void MigrateV20ToV21(SaveFields& fields) {
  fields["char.built"] = "0";
  fields["char.archetype"] = "0";
  fields["char.origin"] = "0";
  fields["char.flaw"] = "0";
  fields["char.temperament"] = "0";
  fields["char.discipline"] = "0";
  fields["char.boldness"] = "0";
  fields["char.social"] = "0";
  fields["char.purism"] = "0";
  fields["char.gift"] = "0";
  fields["char.anti"] = "0";
  fields["char.giftKnown"] = "0";
  fields["char.antiKnown"] = "0";
  fields["char.reps0"] = "0";
  fields["char.reps1"] = "0";
  fields["char.reps2"] = "0";
  fields["char.reps3"] = "0";
  fields["char.reps4"] = "0";
  fields["char.startingCash"] = "0";
  fields["char.agePlus"] = "0";
}

// v6 → v7: what you owe. A v6 career could not owe anything, because there
// was nowhere to owe it — the number was simply missing from cash.
void MigrateV6ToV7(SaveFields& fields) { fields["owed"] = "0"; }

// v4 → v5: the dog. A v4 career never met it, so it migrates to exactly the
// stray a new career finds at the Lot: nobody's, unbonded, and hungry.
void MigrateV4ToV5(SaveFields& fields) {
  fields["dog.name"] = "the dog";
  fields["dog.adopted"] = "0";
  fields["dog.bond"] = "0";
  fields["dog.fed"] = "0.4";
}

}  // namespace

const std::vector<Migration>& DefaultMigrations() {
  static const std::vector<Migration> kMigrations = {
      &MigrateV1ToV2, &MigrateV2ToV3, &MigrateV3ToV4, &MigrateV4ToV5,
      &MigrateV5ToV6, &MigrateV6ToV7, &MigrateV7ToV8, &MigrateV8ToV9,
      &MigrateV9ToV10, &MigrateV10ToV11, &MigrateV11ToV12,
      &MigrateV12ToV13, &MigrateV13ToV14, &MigrateV14ToV15,
      &MigrateV15ToV16, &MigrateV16ToV17, &MigrateV17ToV18,
      &MigrateV18ToV19, &MigrateV19ToV20, &MigrateV20ToV21,
      &MigrateV21ToV22, &MigrateV22ToV23, &MigrateV23ToV24};
  return kMigrations;
}

bool ApplyMigrations(SaveFields& fields, int fromVersion, int toVersion,
                     const std::vector<Migration>& migrations) {
  for (int v = fromVersion; v < toVersion; v++) {
    const int index = v - 1;
    if (index < 0 || index >= static_cast<int>(migrations.size())) return false;
    migrations[index](fields);
  }
  return true;
}

std::string SerializeSave(const SaveGame& save) {
  std::ostringstream out;
  out << "version=" << save.version << "\n";
  out << "seed=" << save.seed << "\n";
  // The name is the player's own text, and the format is line-oriented: a
  // newline in it would shear the file in half. '=' is fine -- the parser
  // splits on the first one -- so only line breaks are stripped.
  std::string safeName = save.player.name;
  safeName.erase(std::remove(safeName.begin(), safeName.end(), '\n'),
                 safeName.end());
  safeName.erase(std::remove(safeName.begin(), safeName.end(), '\r'),
                 safeName.end());
  out << "player.name=" << safeName << "\n";
  out << "day=" << save.player.day << "\n";
  out << "cash=" << NumToStr(save.player.cash) << "\n";

  const Climber& c = save.player.climber;
  out << "skills.power=" << NumToStr(c.skills.power) << "\n";
  out << "skills.fingers=" << NumToStr(c.skills.fingers) << "\n";
  out << "skills.technique=" << NumToStr(c.skills.technique) << "\n";
  out << "skills.endurance=" << NumToStr(c.skills.endurance) << "\n";
  out << "skills.head=" << NumToStr(c.skills.head) << "\n";
  out << "morphology=" << IntToStr(static_cast<int>(c.morphology)) << "\n";
  out << "skin=" << NumToStr(c.skin) << "\n";
  out << "psyche=" << NumToStr(c.psyche) << "\n";

  out << "projects=" << static_cast<int>(save.player.projects.size()) << "\n";
  for (size_t i = 0; i < save.player.projects.size(); i++) {
    const ProjectMemory& m = save.player.projects[i];
    const int n = static_cast<int>(i);
    out << ProjKey(n, "name") << "=" << m.routeName << "\n";
    out << ProjKey(n, "grade") << "=" << IntToStr(m.grade) << "\n";
    out << ProjKey(n, "attempts") << "=" << IntToStr(m.attempts) << "\n";
    out << ProjKey(n, "best") << "=" << IntToStr(m.bestHighpoint) << "\n";
    out << ProjKey(n, "beta") << "=" << NumToStr(m.beta) << "\n";
    out << ProjKey(n, "sent") << "=" << (m.sent ? "1" : "0") << "\n";
    out << ProjKey(n, "clean") << "=" << NumToStr(m.cleanliness) << "\n";
    out << ProjKey(n, "given") << "=" << m.givenName << "\n";
    out << ProjKey(n, "fa") << "=" << (m.firstAscent ? "1" : "0") << "\n";
    out << ProjKey(n, "confirmed") << "=" << IntToStr(m.confirmedGrade) << "\n";
    out << ProjKey(n, "style") << "=" << IntToStr(static_cast<int>(m.firstSendStyle))
        << "\n";
    out << ProjKey(n, "disc") << "=" << IntToStr(static_cast<int>(m.discipline))
        << "\n";
  }

  // The ones that came before. Their first ascents are the only reason the
  // guidebook says anything at all about who put a line up.
  out << "legacies=" << IntToStr(static_cast<int>(save.legacies.size()))
      << "\n";
  for (size_t i = 0; i < save.legacies.size(); i++) {
    const Legacy& l = save.legacies[i];
    const std::string k = "legacy." + IntToStr(static_cast<int>(i)) + ".";
    out << k << "name=" << l.name << "\n";
    out << k << "seasons=" << IntToStr(l.seasons) << "\n";
    out << k << "retiredat=" << NumToStr(l.retiredAt) << "\n";
    out << k << "hardest=" << IntToStr(l.hardestSendGrade) << "\n";
    out << k << "hardestname=" << l.hardestSendName << "\n";
    out << k << "hardestdisc="
        << IntToStr(static_cast<int>(l.hardestSendDiscipline)) << "\n";
    out << k << "sends=" << IntToStr(l.totalSends) << "\n";
    out << k << "attempts=" << IntToStr(l.totalAttempts) << "\n";
    out << k << "nemesis=" << l.nemesis << "\n";
    out << k << "nemesisattempts=" << IntToStr(l.nemesisAttempts) << "\n";
    out << k << "open=" << (l.cragOpenAtTheEnd ? "1" : "0") << "\n";
    out << k << "fas=" << IntToStr(static_cast<int>(l.firstAscents.size()))
        << "\n";
    for (size_t j = 0; j < l.firstAscents.size(); j++) {
      const NamedLine& n = l.firstAscents[j];
      const std::string fk = k + "fa." + IntToStr(static_cast<int>(j)) + ".";
      out << fk << "key=" << n.routeKey << "\n";
      out << fk << "given=" << n.givenName << "\n";
      out << fk << "grade=" << IntToStr(n.confirmedGrade) << "\n";
      out << fk << "style=" << IntToStr(static_cast<int>(n.style)) << "\n";
      out << fk << "disc=" << IntToStr(static_cast<int>(n.discipline)) << "\n";
    }
  }

  out << "secrets=" << IntToStr(static_cast<int>(save.player.secrets.size()))
      << "\n";
  for (size_t i = 0; i < save.player.secrets.size(); i++) {
    const Secret& s = save.player.secrets[i];
    const std::string k = "secret." + IntToStr(static_cast<int>(i)) + ".";
    out << k << "act=" << IntToStr(static_cast<int>(s.act)) << "\n";
    out << k << "route=" << s.routeKey << "\n";
    out << k << "done=" << IntToStr(s.dayDone) << "\n";
    out << k << "known=" << (s.known ? "1" : "0") << "\n";
    out << k << "found=" << IntToStr(s.dayFound) << "\n";
  }

  out << "sponsor.tier="
      << IntToStr(static_cast<int>(save.player.sponsor.tier)) << "\n";
  out << "sponsor.seasons=" << IntToStr(save.player.sponsor.seasonsHeld)
      << "\n";
  out << "sponsor.lastgrade="
      << IntToStr(save.player.sponsor.gradeAtLastReview) << "\n";
  out << "sponsor.stale="
      << IntToStr(save.player.sponsor.seasonsWithoutProgress) << "\n";
  out << "sponsor.hurtdays="
      << IntToStr(save.player.sponsor.daysHurtThisSeason) << "\n";

  out << "owed=" << NumToStr(save.player.owed) << "\n";
  for (int i = 0; i < kFactionCount; i++) {
    out << "standing." << IntToStr(i) << "="
        << NumToStr(save.player.standing.with[i]) << "\n";
  }
  out << "load=" << NumToStr(save.player.climber.load) << "\n";
  // Somebody to beat, and the ones who came before. The name goes first
  // because it is the field that says whether there is anybody at all --
  // `SleepToNextDay` skips an empty one, which is what makes a v21 career
  // load unchanged.
  {
    const Rival& rv = save.player.rival;
    out << "rival.name=" << rv.name << "\n";
    out << "rival.style=" << IntToStr(static_cast<int>(rv.style)) << "\n";
    out << "rival.vibe=" << IntToStr(static_cast<int>(rv.vibe)) << "\n";
    out << "rival.gen=" << IntToStr(rv.generation) << "\n";
    out << "rival.born=" << IntToStr(rv.bornOnDay) << "\n";
    out << "rival.startage=" << NumToStr(rv.startAge) << "\n";
    out << "rival.grade=" << NumToStr(rv.grade) << "\n";
    out << "rival.laststep=" << IntToStr(rv.lastStepDay) << "\n";
    out << "rival.peak=" << NumToStr(rv.peakGrade) << "\n";
    out << "rival.rivalry=" << NumToStr(rv.rivalry) << "\n";
    out << "rival.allied=" << IntToStr(rv.allied ? 1 : 0) << "\n";
    out << "rival.offered=" << IntToStr(rv.offered ? 1 : 0) << "\n";
    out << "rival.met=" << IntToStr(rv.met ? 1 : 0) << "\n";
    out << "rival.retired=" << IntToStr(rv.retired ? 1 : 0) << "\n";
    out << "ranking=" << NumToStr(save.player.rankingPoints) << "\n";
    out << "rival.race=" << rv.race.routeName << "\n";
    out << "rival.raceby=" << IntToStr(rv.race.byDay) << "\n";
    out << "rival.racefa=" << IntToStr(rv.race.forFirstAscent ? 1 : 0)
        << "\n";
    out << "rival.fas=" << IntToStr(static_cast<int>(rv.firstAscents.size()))
        << "\n";
    for (std::size_t i = 0; i < rv.firstAscents.size(); i++) {
      out << "rival.fa" << IntToStr(static_cast<int>(i)) << "="
          << rv.firstAscents[i] << "\n";
    }
    out << "pastrivals=" << IntToStr(static_cast<int>(
                                save.player.pastRivals.size()))
        << "\n";
    for (std::size_t i = 0; i < save.player.pastRivals.size(); i++) {
      const PastRival& p = save.player.pastRivals[i];
      const std::string k = "pastrival" + IntToStr(static_cast<int>(i)) + ".";
      out << k << "name=" << p.name << "\n";
      out << k << "role=" << IntToStr(static_cast<int>(p.role)) << "\n";
      out << k << "style=" << IntToStr(static_cast<int>(p.style)) << "\n";
      out << k << "day=" << IntToStr(p.retiredOnDay) << "\n";
      out << k << "age=" << NumToStr(p.age) << "\n";
      out << k << "peak=" << NumToStr(p.peakGrade) << "\n";
      out << k << "gen=" << IntToStr(p.generation) << "\n";
    }
  }

  // Who you are. `built` first, because it is the field the loader has to
  // believe before any of the others mean anything.
  {
    const Character& ch = save.player.character;
    out << "char.built=" << IntToStr(ch.built ? 1 : 0) << "\n";
    out << "char.archetype=" << IntToStr(static_cast<int>(ch.build.archetype))
        << "\n";
    out << "char.origin=" << IntToStr(static_cast<int>(ch.build.origin))
        << "\n";
    out << "char.flaw=" << IntToStr(static_cast<int>(ch.build.flaw)) << "\n";
    out << "char.temperament="
        << IntToStr(static_cast<int>(ch.build.temperament)) << "\n";
    out << "char.discipline=" << NumToStr(ch.personality.discipline) << "\n";
    out << "char.boldness=" << NumToStr(ch.personality.boldness) << "\n";
    out << "char.social=" << NumToStr(ch.personality.social) << "\n";
    out << "char.purism=" << NumToStr(ch.personality.purism) << "\n";
    out << "char.gift=" << IntToStr(static_cast<int>(ch.gift)) << "\n";
    out << "char.anti=" << IntToStr(static_cast<int>(ch.antiTalent)) << "\n";
    out << "char.giftKnown=" << IntToStr(ch.giftKnown ? 1 : 0) << "\n";
    out << "char.antiKnown=" << IntToStr(ch.antiKnown ? 1 : 0) << "\n";
    for (int i = 0; i < kSkillCount; i++) {
      out << "char.reps" << IntToStr(i) << "=" << NumToStr(ch.reps[i])
          << "\n";
    }
    out << "char.startingCash=" << NumToStr(ch.startingCash) << "\n";
    out << "char.agePlus=" << IntToStr(ch.agePlus) << "\n";
  }
  out << "injury.active="
      << IntToStr(save.player.climber.injury.active ? 1 : 0) << "\n";
  out << "injury.kind="
      << IntToStr(static_cast<int>(save.player.climber.injury.kind)) << "\n";
  out << "injury.severity=" << NumToStr(save.player.climber.injury.severity)
      << "\n";
  out << "injury.days=" << IntToStr(save.player.climber.injury.daysLeft)
      << "\n";
  out << "physio.last=" << IntToStr(save.player.lastPhysioDay) << "\n";
  out << "kit.pads=" << IntToStr(save.player.kit.pads) << "\n";
  out << "kit.hangboard=" << IntToStr(save.player.kit.hangboard ? 1 : 0)
      << "\n";
  out << "kit.membership=" << IntToStr(save.player.kit.membershipDaysLeft)
      << "\n";
  out << "job.salaried=" << IntToStr(save.player.job.salaried ? 1 : 0) << "\n";
  out << "job.days=" << IntToStr(save.player.job.daysWorked) << "\n";
  out << "job.weeks=" << IntToStr(save.player.job.weeksSalaried) << "\n";
  out << "job.sincesalary=" << IntToStr(save.player.job.daysSinceSalary)
      << "\n";
  out << "job.dirtbagyears=" << IntToStr(save.player.job.dirtbagYears) << "\n";
  out << "job.longeststreak=" << IntToStr(save.player.job.longestStreak)
      << "\n";
  // The crew name is authored -- one of a fixed set in DirtbagCrew.cpp, all
  // plain ASCII with no '=' or newline -- so it needs no escaping, the same
  // way the seed does not.
  out << "crew.name=" << save.player.crew.name << "\n";
  out << "crew.namedon=" << IntToStr(save.player.crew.namedOnDay) << "\n";
  out << "crew.days=" << IntToStr(save.player.crew.daysReadingAsACrew)
      << "\n";
  out << "crew.members=" << IntToStr(save.player.crew.membersWhenNamed)
      << "\n";
  out << "dreams.rig=" << IntToStr(save.player.dreams.has[0] ? 1 : 0) << "\n";
  out << "dreams.warchest=" << IntToStr(save.player.dreams.has[1] ? 1 : 0)
      << "\n";
  out << "dreams.homebase=" << IntToStr(save.player.dreams.has[2] ? 1 : 0)
      << "\n";
  out << "dreams.chosen=" << IntToStr(static_cast<int>(save.player.dreams.chosen))
      << "\n";
  out << "dreams.seasonoff=" << IntToStr(save.player.dreams.seasonOffDaysLeft)
      << "\n";
  out << "van.rig=" << IntToStr(save.player.van.rig ? 1 : 0) << "\n";
  out << "standing.closed=" << IntToStr(save.player.standing.closedDays)
      << "\n";
  out << "shoes.wear=" << NumToStr(save.player.shoes.wear) << "\n";
  out << "shoes.resoles=" << IntToStr(save.player.shoes.resoles) << "\n";
  out << "shoes.pairs=" << IntToStr(save.player.shoes.pairsOwned) << "\n";
  out << "van.hours=" << NumToStr(save.player.van.hoursDriven) << "\n";
  for (int i = 0; i < kVanPartCount; i++) {
    const std::string k = "van." + IntToStr(i) + ".";
    out << k << "wear=" << NumToStr(save.player.van.parts[i].wear) << "\n";
    out << k << "patches=" << IntToStr(save.player.van.parts[i].patches) << "\n";
    out << k << "failed=" << (save.player.van.parts[i].failed ? "1" : "0")
        << "\n";
  }
  out << "dog.name=" << save.player.dog.name << "\n";
  out << "dog.adopted=" << (save.player.dog.adopted ? "1" : "0") << "\n";
  out << "dog.bond=" << NumToStr(save.player.dog.bond) << "\n";
  out << "dog.fed=" << NumToStr(save.player.dog.fed) << "\n";
  out << "bonds=" << static_cast<int>(save.player.bonds.size()) << "\n";
  for (size_t i = 0; i < save.player.bonds.size(); i++) {
    const PartnerBond& b = save.player.bonds[i];
    const int n = static_cast<int>(i);
    out << BondKey(n, "name") << "=" << b.name << "\n";
    out << BondKey(n, "rapport") << "=" << NumToStr(b.rapport) << "\n";
    out << BondKey(n, "fas") << "="
        << static_cast<int>(b.firstAscents.size()) << "\n";
    for (size_t f = 0; f < b.firstAscents.size(); f++) {
      out << BondKey(n, ("fa." + IntToStr(static_cast<int>(f))).c_str())
          << "=" << b.firstAscents[f] << "\n";
    }
  }
  return out.str();
}

LoadResult DeserializeSave(const std::string& text, SaveGame& out,
                           const std::vector<Migration>& migrations) {
  // Split into key=value on the first '=' per line. Values run to the line
  // end, so route names may contain '=' but never a newline — BuildRoute
  // names satisfy that by construction.
  SaveFields fields;
  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    const size_t eq = line.find('=');
    if (eq == std::string::npos) return LoadResult::BadFormat;
    fields[line.substr(0, eq)] = line.substr(eq + 1);
  }

  int version = 0;
  if (!ParseInt(fields, "version", version)) return LoadResult::BadFormat;
  if (version > kSaveVersion) return LoadResult::FutureVersion;
  if (version < kSaveVersion &&
      !ApplyMigrations(fields, version, kSaveVersion, migrations)) {
    return LoadResult::BadFormat;
  }

  SaveGame save;
  save.version = kSaveVersion;
  int morphology = 0, projectCount = 0;
  Climber& c = save.player.climber;
  if (!ParseString(fields, "seed", save.seed) ||
      !ParseString(fields, "player.name", save.player.name) ||
      !ParseInt(fields, "day", save.player.day) ||
      !ParseDouble(fields, "cash", save.player.cash) ||
      !ParseDouble(fields, "skills.power", c.skills.power) ||
      !ParseDouble(fields, "skills.fingers", c.skills.fingers) ||
      !ParseDouble(fields, "skills.technique", c.skills.technique) ||
      !ParseDouble(fields, "skills.endurance", c.skills.endurance) ||
      !ParseDouble(fields, "skills.head", c.skills.head) ||
      !ParseInt(fields, "morphology", morphology) ||
      !ParseDouble(fields, "skin", c.skin) ||
      !ParseDouble(fields, "psyche", c.psyche) ||
      !ParseInt(fields, "projects", projectCount)) {
    return LoadResult::BadFormat;
  }
  c.morphology = static_cast<Morphology>(morphology);

  save.player.projects.clear();
  for (int i = 0; i < projectCount; i++) {
    ProjectMemory m;
    int sent = 0, style = 0, firstAscent = 0;
    if (!ParseString(fields, ProjKey(i, "name"), m.routeName) ||
        !ParseInt(fields, ProjKey(i, "grade"), m.grade) ||
        !ParseInt(fields, ProjKey(i, "attempts"), m.attempts) ||
        !ParseInt(fields, ProjKey(i, "best"), m.bestHighpoint) ||
        !ParseDouble(fields, ProjKey(i, "beta"), m.beta) ||
        !ParseInt(fields, ProjKey(i, "sent"), sent) ||
        !ParseDouble(fields, ProjKey(i, "clean"), m.cleanliness) ||
        !ParseInt(fields, ProjKey(i, "fa"), firstAscent) ||
        !ParseInt(fields, ProjKey(i, "confirmed"), m.confirmedGrade) ||
        !ParseInt(fields, ProjKey(i, "style"), style)) {
      return LoadResult::BadFormat;
    }
    m.sent = sent != 0;
    m.firstSendStyle = static_cast<Style>(style);
    m.firstAscent = firstAscent != 0;
    // A given name is allowed to be absent and allowed to be empty: an
    // unnamed line is the normal case, not a corrupt one.
    ParseString(fields, ProjKey(i, "given"), m.givenName);
    int disc = 0;
    if (!ParseInt(fields, ProjKey(i, "disc"), disc)) {
      return LoadResult::BadFormat;
    }
    // Clamped rather than trusted: a hand-edited save must not be able to
    // hand the guidebook a ladder that does not exist.
    m.discipline = disc == 1 ? Discipline::Sport : Discipline::Boulder;
    save.player.projects.push_back(m);
  }

  for (int i = 0; i < kFactionCount; i++) {
    if (!ParseDouble(fields, "standing." + IntToStr(i),
                     save.player.standing.with[i])) {
      return LoadResult::BadFormat;
    }
  }
  if (!ParseInt(fields, "standing.closed", save.player.standing.closedDays)) {
    return LoadResult::BadFormat;
  }
  int hurt = 0, injuryKind = 0;
  {
    Rival& rv = save.player.rival;
    int style = 0, vibe = 0, allied = 0, offered = 0, met = 0, retired = 0,
        fas = 0, pastCount = 0, raceFa = 0;
    if (!ParseString(fields, "rival.name", rv.name) ||
        !ParseInt(fields, "rival.style", style) ||
        !ParseInt(fields, "rival.vibe", vibe) ||
        !ParseInt(fields, "rival.gen", rv.generation) ||
        !ParseInt(fields, "rival.born", rv.bornOnDay) ||
        !ParseDouble(fields, "rival.startage", rv.startAge) ||
        !ParseDouble(fields, "rival.grade", rv.grade) ||
        !ParseInt(fields, "rival.laststep", rv.lastStepDay) ||
        !ParseDouble(fields, "rival.peak", rv.peakGrade) ||
        !ParseDouble(fields, "rival.rivalry", rv.rivalry) ||
        !ParseInt(fields, "rival.allied", allied) ||
        !ParseInt(fields, "rival.offered", offered) ||
        !ParseInt(fields, "rival.met", met) ||
        !ParseInt(fields, "rival.retired", retired) ||
        !ParseDouble(fields, "ranking", save.player.rankingPoints) ||
        !ParseString(fields, "rival.race", rv.race.routeName) ||
        !ParseInt(fields, "rival.raceby", rv.race.byDay) ||
        !ParseInt(fields, "rival.racefa", raceFa) ||
        !ParseInt(fields, "rival.fas", fas) ||
        !ParseInt(fields, "pastrivals", pastCount)) {
      return LoadResult::BadFormat;
    }
    // Clamped rather than trusted, same as the character's enums: a file is
    // a thing a person can edit and an out-of-range value indexes off the
    // end of a static table.
    const auto pick = [](int v, int count) {
      return (v >= 0 && v < count) ? v : 0;
    };
    rv.style = static_cast<RouteType>(pick(style, 6));
    rv.vibe = static_cast<RivalVibe>(pick(vibe, kRivalVibeCount));
    rv.allied = allied != 0;
    rv.offered = offered != 0;
    rv.met = met != 0;
    rv.retired = retired != 0;
    rv.race.forFirstAscent = raceFa != 0;
    rv.firstAscents.clear();
    for (int i = 0; i < fas; i++) {
      std::string key;
      if (!ParseString(fields, "rival.fa" + IntToStr(i), key)) {
        return LoadResult::BadFormat;
      }
      rv.firstAscents.push_back(key);
    }
    save.player.pastRivals.clear();
    for (int i = 0; i < pastCount; i++) {
      const std::string k = "pastrival" + IntToStr(i) + ".";
      PastRival p;
      int role = 0, pstyle = 0;
      if (!ParseString(fields, k + "name", p.name) ||
          !ParseInt(fields, k + "role", role) ||
          !ParseInt(fields, k + "style", pstyle) ||
          !ParseInt(fields, k + "day", p.retiredOnDay) ||
          !ParseDouble(fields, k + "age", p.age) ||
          !ParseDouble(fields, k + "peak", p.peakGrade) ||
          !ParseInt(fields, k + "gen", p.generation)) {
        return LoadResult::BadFormat;
      }
      p.role = static_cast<RivalRole>(pick(role, 3));
      p.style = static_cast<RouteType>(pick(pstyle, 6));
      save.player.pastRivals.push_back(p);
    }
  }
  {
    Character& ch = save.player.character;
    int built = 0, arch = 0, orig = 0, flaw = 0, temper = 0, gift = 0,
        anti = 0, gk = 0, ak = 0;
    if (!ParseInt(fields, "char.built", built) ||
        !ParseInt(fields, "char.archetype", arch) ||
        !ParseInt(fields, "char.origin", orig) ||
        !ParseInt(fields, "char.flaw", flaw) ||
        !ParseInt(fields, "char.temperament", temper) ||
        !ParseDouble(fields, "char.discipline", ch.personality.discipline) ||
        !ParseDouble(fields, "char.boldness", ch.personality.boldness) ||
        !ParseDouble(fields, "char.social", ch.personality.social) ||
        !ParseDouble(fields, "char.purism", ch.personality.purism) ||
        !ParseInt(fields, "char.gift", gift) ||
        !ParseInt(fields, "char.anti", anti) ||
        !ParseInt(fields, "char.giftKnown", gk) ||
        !ParseInt(fields, "char.antiKnown", ak) ||
        !ParseDouble(fields, "char.startingCash", ch.startingCash) ||
        !ParseInt(fields, "char.agePlus", ch.agePlus)) {
      return LoadResult::BadFormat;
    }
    // Clamped rather than trusted. A file is a thing a person can edit, and
    // an out-of-range enum here indexes off the end of a static table.
    const auto pick = [](int v, int count) {
      return (v >= 0 && v < count) ? v : 0;
    };
    ch.built = built != 0;
    ch.build.archetype =
        static_cast<Archetype>(pick(arch, kArchetypeCount));
    ch.build.origin = static_cast<Origin>(pick(orig, kOriginCount));
    ch.build.flaw = static_cast<Flaw>(pick(flaw, kFlawCount));
    ch.build.temperament =
        static_cast<Temperament>(pick(temper, kTemperamentCount));
    ch.gift = static_cast<Talent>(pick(gift, kTalentCount));
    ch.antiTalent = static_cast<Talent>(pick(anti, kTalentCount));
    ch.giftKnown = gk != 0;
    ch.antiKnown = ak != 0;
    for (int i = 0; i < kSkillCount; i++) {
      if (!ParseDouble(fields, "char.reps" + IntToStr(i), ch.reps[i])) {
        return LoadResult::BadFormat;
      }
    }
  }
  if (!ParseDouble(fields, "load", save.player.climber.load) ||
      !ParseInt(fields, "injury.active", hurt) ||
      !ParseInt(fields, "injury.kind", injuryKind) ||
      !ParseDouble(fields, "injury.severity",
                   save.player.climber.injury.severity) ||
      !ParseInt(fields, "injury.days", save.player.climber.injury.daysLeft) ||
      !ParseInt(fields, "physio.last", save.player.lastPhysioDay)) {
    return LoadResult::BadFormat;
  }
  save.player.climber.injury.active = hurt != 0;
  // Clamped rather than trusted: a hand-edited save must not be able to
  // hand the resolver an injury kind that is not one of the four.
  save.player.climber.injury.kind = static_cast<InjuryKind>(
      injuryKind >= 0 && injuryKind < kInjuryKindCount ? injuryKind : 0);

  int hangboard = 0;
  if (!ParseInt(fields, "kit.pads", save.player.kit.pads) ||
      !ParseInt(fields, "kit.hangboard", hangboard) ||
      !ParseInt(fields, "kit.membership",
                save.player.kit.membershipDaysLeft)) {
    return LoadResult::BadFormat;
  }
  save.player.kit.hangboard = hangboard != 0;

  int salaried = 0;
  int dreamRig = 0, dreamWarChest = 0, dreamHomeBase = 0, dreamChosen = 0;
  int vanRig = 0;
  if (!ParseInt(fields, "job.salaried", salaried) ||
      !ParseInt(fields, "job.days", save.player.job.daysWorked) ||
      !ParseInt(fields, "job.weeks", save.player.job.weeksSalaried) ||
      !ParseInt(fields, "job.sincesalary", save.player.job.daysSinceSalary) ||
      !ParseInt(fields, "job.dirtbagyears", save.player.job.dirtbagYears) ||
      !ParseInt(fields, "job.longeststreak", save.player.job.longestStreak) ||
      !ParseInt(fields, "crew.namedon", save.player.crew.namedOnDay) ||
      !ParseInt(fields, "crew.days", save.player.crew.daysReadingAsACrew) ||
      !ParseInt(fields, "crew.members", save.player.crew.membersWhenNamed) ||
      // Required rather than optional even though empty is the common
      // value: a missing key means an unmigrated save, and silently
      // leaving the name blank would look exactly like a career the town
      // has not named yet. `line.substr(eq + 1)` yields "" for "crew.name=",
      // so an unnamed crew round-trips as present-and-empty.
      !ParseString(fields, "crew.name", save.player.crew.name) ||
      !ParseInt(fields, "dreams.rig", dreamRig) ||
      !ParseInt(fields, "dreams.warchest", dreamWarChest) ||
      !ParseInt(fields, "dreams.homebase", dreamHomeBase) ||
      !ParseInt(fields, "dreams.chosen", dreamChosen) ||
      !ParseInt(fields, "dreams.seasonoff",
                save.player.dreams.seasonOffDaysLeft) ||
      !ParseInt(fields, "van.rig", vanRig)) {
    return LoadResult::BadFormat;
  }
  save.player.job.salaried = salaried != 0;
  save.player.dreams.has[0] = dreamRig != 0;
  save.player.dreams.has[1] = dreamWarChest != 0;
  save.player.dreams.has[2] = dreamHomeBase != 0;
  // Clamped rather than trusted, the same way the injury kind is: a
  // hand-edited save must not be able to name a dream that does not exist.
  save.player.dreams.chosen =
      dreamChosen >= 0 && dreamChosen <= kDreamCount
          ? static_cast<Dream>(dreamChosen)
          : Dream::None;
  save.player.van.rig = vanRig != 0;
  int secretCount = 0;
  if (!ParseInt(fields, "secrets", secretCount)) return LoadResult::BadFormat;
  for (int i = 0; i < secretCount; i++) {
    const std::string k = "secret." + IntToStr(i) + ".";
    Secret s;
    int act = 0, known = 0;
    if (!ParseInt(fields, k + "act", act) ||
        !ParseInt(fields, k + "done", s.dayDone) ||
        !ParseInt(fields, k + "known", known) ||
        !ParseInt(fields, k + "found", s.dayFound)) {
      return LoadResult::BadFormat;
    }
    // A staged photo is not on a line, so an empty route key is correct.
    ParseString(fields, k + "route", s.routeKey);
    // Clamped rather than trusted, like every other enum out of a save.
    s.act = (act >= 0 && act < kEthicalActCount)
                ? static_cast<EthicalAct>(act)
                : EthicalAct::ChippedAHold;
    s.known = known != 0;
    save.player.secrets.push_back(s);
  }

  int sponsorTier = 0;
  if (!ParseInt(fields, "sponsor.tier", sponsorTier) ||
      !ParseInt(fields, "sponsor.seasons", save.player.sponsor.seasonsHeld) ||
      !ParseInt(fields, "sponsor.lastgrade",
                save.player.sponsor.gradeAtLastReview) ||
      !ParseInt(fields, "sponsor.stale",
                save.player.sponsor.seasonsWithoutProgress) ||
      !ParseInt(fields, "sponsor.hurtdays",
                save.player.sponsor.daysHurtThisSeason)) {
    return LoadResult::BadFormat;
  }
  // Clamped rather than trusted, like every other enum out of a save file.
  save.player.sponsor.tier =
      (sponsorTier >= 0 && sponsorTier < kSponsorTierCount)
          ? static_cast<SponsorTier>(sponsorTier)
          : SponsorTier::None;

  int legacyCount = 0;
  if (!ParseInt(fields, "legacies", legacyCount)) return LoadResult::BadFormat;
  for (int i = 0; i < legacyCount; i++) {
    const std::string k = "legacy." + IntToStr(i) + ".";
    Legacy l;
    int hardestDisc = 0, open = 0, faCount = 0;
    if (!ParseString(fields, k + "name", l.name) ||
        !ParseInt(fields, k + "seasons", l.seasons) ||
        !ParseDouble(fields, k + "retiredat", l.retiredAt) ||
        !ParseInt(fields, k + "hardest", l.hardestSendGrade) ||
        !ParseInt(fields, k + "hardestdisc", hardestDisc) ||
        !ParseInt(fields, k + "sends", l.totalSends) ||
        !ParseInt(fields, k + "attempts", l.totalAttempts) ||
        !ParseInt(fields, k + "nemesisattempts", l.nemesisAttempts) ||
        !ParseInt(fields, k + "open", open) ||
        !ParseInt(fields, k + "fas", faCount)) {
      return LoadResult::BadFormat;
    }
    // Names are allowed to be empty — a career that never sent anything has
    // no hardest line, and a nemesis is a luxury.
    ParseString(fields, k + "hardestname", l.hardestSendName);
    ParseString(fields, k + "nemesis", l.nemesis);
    l.hardestSendDiscipline =
        hardestDisc == 1 ? Discipline::Sport : Discipline::Boulder;
    l.cragOpenAtTheEnd = open != 0;

    for (int j = 0; j < faCount; j++) {
      const std::string fk = k + "fa." + IntToStr(j) + ".";
      NamedLine n;
      int style = 0, disc = 0;
      if (!ParseString(fields, fk + "key", n.routeKey) ||
          !ParseInt(fields, fk + "grade", n.confirmedGrade) ||
          !ParseInt(fields, fk + "style", style) ||
          !ParseInt(fields, fk + "disc", disc)) {
        return LoadResult::BadFormat;
      }
      ParseString(fields, fk + "given", n.givenName);
      n.style = static_cast<Style>(style);
      n.discipline = disc == 1 ? Discipline::Sport : Discipline::Boulder;
      n.by = l.name;
      l.firstAscents.push_back(n);
    }
    save.legacies.push_back(l);
  }

  if (!ParseDouble(fields, "owed", save.player.owed) ||
      !ParseDouble(fields, "shoes.wear", save.player.shoes.wear) ||
      !ParseInt(fields, "shoes.resoles", save.player.shoes.resoles) ||
      !ParseInt(fields, "shoes.pairs", save.player.shoes.pairsOwned) ||
      !ParseDouble(fields, "van.hours", save.player.van.hoursDriven)) {
    return LoadResult::BadFormat;
  }
  for (int i = 0; i < kVanPartCount; i++) {
    const std::string k = "van." + IntToStr(i) + ".";
    int failed = 0;
    if (!ParseDouble(fields, k + "wear", save.player.van.parts[i].wear) ||
        !ParseInt(fields, k + "patches", save.player.van.parts[i].patches) ||
        !ParseInt(fields, k + "failed", failed)) {
      return LoadResult::BadFormat;
    }
    save.player.van.parts[i].failed = failed != 0;
  }

  int adopted = 0;
  if (!ParseString(fields, "dog.name", save.player.dog.name) ||
      !ParseInt(fields, "dog.adopted", adopted) ||
      !ParseDouble(fields, "dog.bond", save.player.dog.bond) ||
      !ParseDouble(fields, "dog.fed", save.player.dog.fed)) {
    return LoadResult::BadFormat;
  }
  save.player.dog.adopted = adopted != 0;

  int bondCount = 0;
  if (!ParseInt(fields, "bonds", bondCount) || bondCount < 0) {
    return LoadResult::BadFormat;
  }
  for (int i = 0; i < bondCount; i++) {
    PartnerBond b;
    int faCount = 0;
    if (!ParseString(fields, BondKey(i, "name"), b.name) ||
        !ParseDouble(fields, BondKey(i, "rapport"), b.rapport) ||
        !ParseInt(fields, BondKey(i, "fas"), faCount) || faCount < 0) {
      return LoadResult::BadFormat;
    }
    for (int f = 0; f < faCount; f++) {
      std::string key;
      if (!ParseString(fields,
                       BondKey(i, ("fa." + IntToStr(f)).c_str()), key)) {
        return LoadResult::BadFormat;
      }
      b.firstAscents.push_back(key);
    }
    save.player.bonds.push_back(b);
  }

  out = save;
  return LoadResult::Ok;
}

}  // namespace dirtbag
