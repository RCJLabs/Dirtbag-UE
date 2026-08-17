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

}  // namespace

const std::vector<Migration>& DefaultMigrations() {
  // Empty at version 1 — see the header's contract for how it grows.
  static const std::vector<Migration> kMigrations;
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
    out << ProjKey(n, "attempts") << "=" << IntToStr(m.attempts) << "\n";
    out << ProjKey(n, "best") << "=" << IntToStr(m.bestHighpoint) << "\n";
    out << ProjKey(n, "beta") << "=" << NumToStr(m.beta) << "\n";
    out << ProjKey(n, "sent") << "=" << (m.sent ? "1" : "0") << "\n";
    out << ProjKey(n, "style") << "=" << IntToStr(static_cast<int>(m.firstSendStyle))
        << "\n";
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
    int sent = 0, style = 0;
    if (!ParseString(fields, ProjKey(i, "name"), m.routeName) ||
        !ParseInt(fields, ProjKey(i, "attempts"), m.attempts) ||
        !ParseInt(fields, ProjKey(i, "best"), m.bestHighpoint) ||
        !ParseDouble(fields, ProjKey(i, "beta"), m.beta) ||
        !ParseInt(fields, ProjKey(i, "sent"), sent) ||
        !ParseInt(fields, ProjKey(i, "style"), style)) {
      return LoadResult::BadFormat;
    }
    m.sent = sent != 0;
    m.firstSendStyle = static_cast<Style>(style);
    save.player.projects.push_back(m);
  }

  out = save;
  return LoadResult::Ok;
}

}  // namespace dirtbag
