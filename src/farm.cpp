#include "farm.h"

#include <cstdint>
#include <iostream>

#include "gen_artifact.h"
#include "text_io.h"

namespace {

// When looking for the best artifact set, pieces that have GOOD_ROLLS_MARGIN fewer
// good substat rolls than the piece used in the current best set will be skipped.
constexpr int GOOD_ROLLS_MARGIN = 3;

// Calculate the damage modifier for given character profile and artifacts.
int calc_damage(Character& c, Weapon& w, int* artifact_stats, int* set_count) {
  // Aggregate all flat and percentage stats from character, weapon, and artifacts
  int total_stats[STAT_CT];
  // Character + weapon stats
  for (int i = 0; i < STAT_CT; i++)
    total_stats[i] = c.stats[i] + w.stats[i];

  // Artifact stats
  for (int i = 0; i < STAT_CT; i++) {
    total_stats[i] += artifact_stats[i];
  }

  // Set bonuses
  for (int i = 0; i < SET_CT; i++) {
    if (set_count[i] >= 2) {
      StatBonus sb = set_effect(static_cast<Set>(i), TWO_PC);
      for (int j = 0; j < STAT_CT; j++)
        total_stats[j] += sb.stats[j];
    }
    if (set_count[i] >= 4) {
      StatBonus sb = set_effect(static_cast<Set>(i), FOUR_PC);
      for (int j = 0; j < STAT_CT; j++)
        total_stats[j] += sb.stats[j];
    }
  }

  // Calculate using ints instead of floats, average error < 0.01%
  int base_atk = c.base_atk + w.base_atk;
  int64_t total_atk = base_atk * (1000 + total_stats[ATKP]) / 1000 + total_stats[ATK];
  // Denominator: 10^6 from CR * CD, 10^3 from DMG%
  int64_t reactionless_dmg = total_atk * (1000000 + total_stats[CR] * total_stats[CD]) * (1000 + total_stats[ON_ELE]) / 1000000000;
  // Reaction bonus multiplier (1 + reaction bonus %)
  int64_t reaction_bonus = 100 + 278 * total_stats[EM] / (1400 + total_stats[EM]);
  int64_t unreacted_fraction = 1000 * reactionless_dmg * (100-c.reaction_percentage);
  int64_t reacted_fraction = reactionless_dmg * c.reaction_percentage * c.reaction_multiplier_x10 * reaction_bonus;
  // Denominator: 10^2 from reaction bonus, 10^2 from reaction percentage, 10 from reaction multiplier
  return (int) ((unreacted_fraction + reacted_fraction) / 100000);
}

// Returns whether an artifact in given slot has an offensive mainstat.
// TODO: Combine this with FarmingConfig
bool is_offensive_stat(Slot E, Stat s) {
  if (E == SANDS) {
    return (s == ATKP) || (s == ER) || (s == EM);
  } else if (E == GOBLET) {
    return (s == ATKP) || (s == EM) || (s == ON_ELE) || (s == PHYS);
  } else if (E == CIRCLET) {
    return (s == ATKP) || (s == CR) || (s == CD) || (s == EM);
  } else {
    return true;
  }
}

// Returns an approximation of a single artifact's substat quality
// TODO: Combine this with FarmingConfig
int offensive_substat_ct(Artifact& a) {
  int ct = 0;
  ct += a.substat_values[ATK] / SUBSTAT_LEVEL[ATK][0];
  ct += 2 * a.substat_values[ATKP] / SUBSTAT_LEVEL[ATKP][0];
  ct += a.substat_values[EM] / SUBSTAT_LEVEL[EM][0];
  ct += 2 * a.substat_values[CR] / SUBSTAT_LEVEL[CR][0];
  ct += 2 * a.substat_values[CD] / SUBSTAT_LEVEL[CD][0];
  return ct / 2;
}

void add_artifact_stats(int* total_stats, Artifact& a) {
  total_stats[a.mainstat] += MAINSTAT_LEVEL[a.mainstat];
  for (int i = 0; i < SUBSTAT_CT; i++) {
    total_stats[i] += a.substat_values[i];
  }
}

void subtract_artifact_stats(int* total_stats, Artifact& a) {
  total_stats[a.mainstat] -= MAINSTAT_LEVEL[a.mainstat];
  for (int i = 0; i < SUBSTAT_CT; i++) {
    total_stats[i] -= a.substat_values[i];
  }
}

}  // namespace

FarmedSet farm(Character& character, Weapon& weapon, FarmingConfig& farming_config, int n) {
  FarmedSet max_set;

  // Step 1: Generate n artifacts
  Artifact* all_artis = get_artifact_storage(n);
  int fully_upgraded = 0;
  for (int i = 0; i < n; i++) {
    gen_random(all_artis + i, farming_config);
    // Only upgrade if satisfying basic quality constraints
    if (farming_config.upgradeable(all_artis[i])) {
      upgrade_full(all_artis + i);
      fully_upgraded++;
    }
  }
  max_set.upgrade_ratio[0] = fully_upgraded;
  max_set.upgrade_ratio[1] = n;

  // Step 2: Categorize artifacts by slot
  int size[SLOT_CT] = {0, 0, 0, 0, 0};
  Artifact* by_slot[SLOT_CT];
  for (int i = 0; i < SLOT_CT; i++)
    by_slot[i] = new Artifact[n];
  for (int i = 0; i < n; i++) {
    // Do not use artifacts that aren't +20
    if (all_artis[i].level < 20) continue;
    // Do not use pieces with a useless mainstat
    if (!is_offensive_stat(all_artis[i].slot, all_artis[i].mainstat)) continue;
    by_slot[all_artis[i].slot][size[all_artis[i].slot]] = all_artis[i];
    size[all_artis[i].slot]++;
  }

  // Step 3: Brute force the set that gives the most damage by checking all possibilities
  // Track the total stats gained from artifacts as we go
  int artifact_stats[STAT_CT];
  for (int i = 0; i < STAT_CT; i++)
    artifact_stats[i] = 0;
  int set_count[SET_CT];
  for (int i = 0; i < SET_CT; i++)
    set_count[i] = 0;

  for (int a = 0; a < size[FLOWER]; a++) {
    // For flowers and feathers, roughly check that they aren't garbage using # of good sub rolls
    if (offensive_substat_ct(by_slot[FLOWER][a]) <= offensive_substat_ct(max_set.artifacts[FLOWER]) - GOOD_ROLLS_MARGIN) continue;
    // Incrementally add and subtract each artifact from total stats
    add_artifact_stats(artifact_stats, by_slot[FLOWER][a]);
    set_count[by_slot[FLOWER][a].set]++;

    // And repeat for all 5 slots
    for (int b = 0; b < size[FEATHER]; b++) {
      if (offensive_substat_ct(by_slot[FEATHER][b]) <= offensive_substat_ct(max_set.artifacts[FEATHER]) - GOOD_ROLLS_MARGIN) continue;
      add_artifact_stats(artifact_stats, by_slot[FEATHER][b]);
      set_count[by_slot[FEATHER][b].set]++;

      for (int c = 0; c < size[SANDS]; c++) {
        // For sands, also require the same mainstat
        if (by_slot[SANDS][c].mainstat == max_set.artifacts[SANDS].mainstat
         && offensive_substat_ct(by_slot[SANDS][c]) <= offensive_substat_ct(max_set.artifacts[SANDS]) - GOOD_ROLLS_MARGIN) continue;
        add_artifact_stats(artifact_stats, by_slot[SANDS][c]);
        set_count[by_slot[SANDS][c].set]++;

        for (int d = 0; d < size[GOBLET]; d++) {
          add_artifact_stats(artifact_stats, by_slot[GOBLET][d]);
          set_count[by_slot[GOBLET][d].set]++;

          for (int e = 0; e < size[CIRCLET]; e++) {
            add_artifact_stats(artifact_stats, by_slot[CIRCLET][e]);
            set_count[by_slot[CIRCLET][e].set]++;
            int curr_damage = calc_damage(character, weapon, artifact_stats, set_count);
            if (curr_damage > max_set.damage) {
              max_set.damage = curr_damage;
              max_set.artifacts[FLOWER] = by_slot[FLOWER][a];
              max_set.artifacts[FEATHER] = by_slot[FEATHER][b];
              max_set.artifacts[SANDS] = by_slot[SANDS][c];
              max_set.artifacts[GOBLET] = by_slot[GOBLET][d];
              max_set.artifacts[CIRCLET] = by_slot[CIRCLET][e];
            }
            subtract_artifact_stats(artifact_stats, by_slot[CIRCLET][e]);
            set_count[by_slot[CIRCLET][e].set]--;
          }
          subtract_artifact_stats(artifact_stats, by_slot[GOBLET][d]);
          set_count[by_slot[GOBLET][d].set]--;
        }
        subtract_artifact_stats(artifact_stats, by_slot[SANDS][c]);
        set_count[by_slot[SANDS][c].set]--;
      }
      subtract_artifact_stats(artifact_stats, by_slot[FEATHER][b]);
      set_count[by_slot[FEATHER][b].set]--;
    }
    subtract_artifact_stats(artifact_stats, by_slot[FLOWER][a]);
    set_count[by_slot[FLOWER][a].set]--;
  }

  for (int i = 0; i < SLOT_CT; i++)
    delete[] by_slot[i];
  delete[] all_artis;

  return max_set;
}
