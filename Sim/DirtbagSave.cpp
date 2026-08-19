#include "DirtbagSave.h"

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
      &MigrateV1ToV2, &MigrateV2ToV3, &MigrateV3ToV4, &MigrateV4ToV5};
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
    save.player.projects.push_back(m);
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
