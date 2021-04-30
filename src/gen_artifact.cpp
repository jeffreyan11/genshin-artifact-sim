#include "gen_artifact.h"

#include <chrono>
#include <random>

namespace {

// Global random number generator, seeded deterministically by default
std::default_random_engine rng;

// Determine whether a substat already exists and needs to be rerolled
bool repeated_substat(Artifact* arti, int sub_n, int substat_type) {
  for (int i = 0; i < sub_n; i++) {
    if (arti->substats[i] == substat_type) return true;
  }
  return false;
}

// Rolls one substat for an artifact that does not have all four substats determined yet.
void roll_substat(Artifact* arti, int sub_n) {
  const int mainstat = arti->mainstat;
  int sub_wt_total = SUBSTAT_WEIGHT_TOTAL;
  // Substat can never be the same as mainstat
  if (mainstat < SUBSTAT_CT)
    sub_wt_total -= SUBSTAT_WEIGHT[mainstat];

  // Set up substat lookup table
  int substat_lookup[SUBSTAT_WEIGHT_TOTAL];
  {
    int idx = 0;
    for (int i = 0; i < SUBSTAT_CT; i++) {
      if (i == mainstat) continue;
      for (int j = 0; j < SUBSTAT_WEIGHT[i]; j++) {
        substat_lookup[idx] = i;
        idx++;
      }
    }
  }

  std::uniform_int_distribution<int> substat_dist(0, sub_wt_total-1);
  // Roll the substat, retrying if necessary
  int substat_type = HP;
  do {
    int substat_roll = substat_dist(rng);
    substat_type = substat_lookup[substat_roll];
  } while (repeated_substat(arti, sub_n, substat_type));

  arti->substats[sub_n] = substat_type;
  std::uniform_int_distribution<int> level_dist(0, 3);
  arti->substat_values[substat_type] += SUBSTAT_LEVEL[substat_type][level_dist(rng)];
}

}  // namespace

void seed() {
  rng.seed(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch()).count());
}

void gen_random(Artifact* arti, FarmingConfig& fcfg) {
  // Roll artifact slot
  std::uniform_int_distribution<int> slot_dist(0, SLOT_CT-1);
  int slot = slot_dist(rng);

  // Roll main stat
  std::uniform_int_distribution<int> mainstat_dist(0, MAINSTAT_WEIGHT[slot][MAINSTAT_CT-1]-1);
  int mainstat_roll = mainstat_dist(rng);
  int mainstat = HP;
  while (mainstat < MAINSTAT_CT) {
    if (mainstat_roll < MAINSTAT_WEIGHT[slot][mainstat]) break;
    mainstat++;
  }

  // Update arti
  arti->slot = static_cast<Slot>(slot);
  arti->mainstat = static_cast<Stat>(mainstat);

  // Roll set
  std::uniform_int_distribution<int> set_dist(0, 1);
  arti->set = DOMAIN_TO_SET[fcfg.next_domain()][set_dist(rng)];

  arti->level = 0;

  // Roll substats
  std::uniform_int_distribution<int> starting_sub_dist(0, EXTRA_SUBSTAT_PROB-1);
  int starting_substats = (starting_sub_dist(rng) == 0) ? 4 : 3;
  arti->extra_substat = (starting_substats == 4);
  for (int i = 0; i < starting_substats; i++) {
    roll_substat(arti, i);
  }
}

void upgrade_full(Artifact* arti) {
  // If the artifact currently has 3 substats, roll the 4th
  if (!arti->extra_substat) {
    roll_substat(arti, 3);
  }

  // 4 substat upgrades possible, +1 if the artifact had 4 lines at +0
  int upgrades = 4 + arti->extra_substat;
  std::uniform_int_distribution<int> upgrade_dist(0, 3);
  for (int i = 0; i < upgrades; i++) {
    int substat_to_upgrade = arti->substats[upgrade_dist(rng)];
    arti->substat_values[substat_to_upgrade] += SUBSTAT_LEVEL[substat_to_upgrade][upgrade_dist(rng)];
  }

  arti->level = 20;
}
