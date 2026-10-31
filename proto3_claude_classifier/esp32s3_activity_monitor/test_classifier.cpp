// Host test: replays the recorded CSVs through the exact firmware classifier.
// Build/run: g++ -std=c++17 -O2 -I.. test_classifier.cpp -o t && ./t <csv_dir>
#include <cstdio>
#include <cstring>
#include <string>
#include "activity_classifier.h"

using namespace act;

static bool replay(const std::string& path, int (&counts)[8]) {
  FILE* f = std::fopen(path.c_str(), "r");
  if (!f) { std::fprintf(stderr, "cannot open %s\n", path.c_str()); return false; }
  char line[256];
  if (!std::fgets(line, sizeof line, f)) { std::fclose(f); return false; }  // header
  Classifier c; Sample s; double t;
  std::memset(counts, 0, sizeof counts);
  while (std::fgets(line, sizeof line, f)) {
    if (std::sscanf(line, "%lf,%f,%f,%f,%f,%f,%f", &t, &s.ax, &s.ay, &s.az, &s.gx, &s.gy, &s.gz) != 7) continue;
    if (c.push(s)) ++counts[static_cast<int>(c.activity())];
  }
  std::fclose(f);
  return true;
}

static int fails = 0;
static void expectShare(const char* name, int (&n)[8], Activity a, double minShare) {
  int total = 0; for (int v : n) total += v;
  const double share = total ? double(n[static_cast<int>(a)]) / total : 0;
  const bool ok = share >= minShare;
  std::printf("%-26s %-9s %5.1f%% (min %.0f%%) %s\n", name, label(a), share * 100, minShare * 100, ok ? "PASS" : "FAIL");
  if (!ok) ++fails;
}

struct Case { const char* file; Activity expect; double minShare; };

int main(int argc, char** argv) {
  const std::string dir = argc > 1 ? argv[1] : ".";
  const Case cases[] = {
      {"sitting_still.csv", Activity::Sitting, 0.95},
      {"standing_still.csv", Activity::Standing, 0.95},
      {"sleeping_front_up.csv", Activity::Lying, 0.95},
      {"walking_y-axis_up.csv", Activity::Walking, 0.90},
      {"stand_sit_repeat_motion.csv", Activity::Sitting, 0.15},
      {"stand_sit_repeat_motion.csv", Activity::SitStand, 0.20},
  };
  for (const Case& c : cases) {
    int n[8];
    if (!replay(dir + "/" + c.file, n)) return 2;
    expectShare(c.file, n, c.expect, c.minShare);
  }
  return fails ? 1 : 0;
}
