#include "farm.h"

#include <algorithm>
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
  for (int i = 0; i < MAINSTAT_CT; i++) {
    total_stats[i] += artifact_stats[i];
  }

  // Set bonuses
  for (int i = 0; i < SET_CT; i++) {
    // Only consider bonuses for target sets
    if (set_count[i] >= 2 && c.farming_config.target_sets[i][TWO_PC]) {
      StatBonus sb = set_effect(static_cast<Set>(i), TWO_PC);
      for (int j = 0; j < STAT_CT; j++)
        total_stats[j] += sb.stats[j];
    }
    if (set_count[i] >= 4 && c.farming_config.target_sets[i][FOUR_PC]) {
      StatBonus sb = set_effect(static_cast<Set>(i), FOUR_PC);
      for (int j = 0; j < STAT_CT; j++)
        total_stats[j] += sb.stats[j];
    }
  }

  // Calculate using ints instead of floats, average error < 0.01%
  int base_atk = c.base_atk + w.base_atk;
  int64_t total_atk = base_atk * (1000 + total_stats[ATKP]) / 1000 + total_stats[ATK];
  int64_t total_dmg_bonus = total_stats[ON_ELE] + total_stats[c.damage_type];
  // Denominator: 10^6 from CR * CD, 10^3 from DMG%
  int64_t reactionless_dmg = total_atk * (1000000 + std::min(1000, total_stats[CR]) * total_stats[CD]) * (1000 + total_dmg_bonus) / 1000000000;
  // Reaction bonus multiplier (1 + reaction bonus %)
  int64_t reaction_bonus = 100 + 278 * total_stats[EM] / (1400 + total_stats[EM]) + total_stats[REACTION];
  int64_t unreacted_fraction = 1000 * reactionless_dmg * (100-c.reaction_percentage);
  int64_t reacted_fraction = reactionless_dmg * c.reaction_percentage * c.reaction_multiplier_x10 * reaction_bonus;
  // Denominator: 10^2 from reaction bonus, 10^2 from reaction percentage, 10 from reaction multiplier
  return (int) ((unreacted_fraction + reacted_fraction) / 100000);
}

void add_artifact_stats(int* total_stats, Artifact& a) {
  total_stats[a.mainstat] += MAINSTAT_LEVEL[a.mainstat];
  for (int i = 0; i < 4; i++) {
    total_stats[a.substats[i]] += a.substat_values[a.substats[i]];
  }
}

void subtract_artifact_stats(int* total_stats, Artifact& a) {
  total_stats[a.mainstat] -= MAINSTAT_LEVEL[a.mainstat];
  for (int i = 0; i < 4; i++) {
    total_stats[a.substats[i]] -= a.substat_values[a.substats[i]];
  }
}

}  // namespace

FarmedSet farm(Character& character, Weapon& weapon, int n) {
  FarmingConfig& farming_config = character.farming_config;
  FarmedSet max_set;

  // Step 1: Generate n artifacts
  Artifact* all_artis = get_artifact_storage(n);
  for (int i = 0; i < n; i++) {
    gen_random(all_artis + i, farming_config);
    max_set.upgrade_ratio[all_artis[i].slot][1]++;
    // Only upgrade if satisfying basic quality constraints
    if (farming_config.upgradeable(all_artis[i])) {
      upgrade_full(all_artis + i);
      max_set.upgrade_ratio[all_artis[i].slot][0]++;
    }
    all_artis[i].stat_score = farming_config.score(all_artis[i]);
  }
  // Sort from greatest to least score, so that the best set is found as quickly as possible
  std::sort(all_artis, all_artis+n, [](Artifact& a, Artifact& b) {
    return a.stat_score > b.stat_score;
  });

  // Step 2: Categorize artifacts by slot
  int size[SLOT_CT] = {0, 0, 0, 0, 0};
  Artifact* by_slot[SLOT_CT];
  for (int i = 0; i < SLOT_CT; i++)
    by_slot[i] = new Artifact[n];
  for (int i = 0; i < n; i++) {
    // Do not use artifacts that aren't +20
    if (all_artis[i].level < 20) continue;
    // Do not use pieces with a useless mainstat
    if (all_artis[i].slot >= SANDS && farming_config.stat_score[all_artis[i].mainstat] == 0) continue;
    by_slot[all_artis[i].slot][size[all_artis[i].slot]] = all_artis[i];
    size[all_artis[i].slot]++;
  }

  // Step 3: Brute force the set that gives the most damage by checking all possibilities
  // Track the total stats gained from artifacts as we go
  int artifact_stats[MAINSTAT_CT];
  for (int i = 0; i < MAINSTAT_CT; i++)
    artifact_stats[i] = 0;
  int set_count[SET_CT];
  for (int i = 0; i < SET_CT; i++)
    set_count[i] = 0;

  for (int a = 0; a < size[FLOWER]; a++) {
    // Roughly check that the piece isn't garbage using # of good sub rolls
    if (by_slot[FLOWER][a].stat_score <= max_set.artifacts[FLOWER].stat_score - farming_config.stat_score_max * GOOD_ROLLS_MARGIN) continue;
    // Incrementally add and subtract each artifact from total stats
    add_artifact_stats(artifact_stats, by_slot[FLOWER][a]);
    set_count[by_slot[FLOWER][a].set]++;

    // And repeat for all 5 slots
    for (int b = 0; b < size[FEATHER]; b++) {
      if (by_slot[FEATHER][b].stat_score <= max_set.artifacts[FEATHER].stat_score - farming_config.stat_score_max * GOOD_ROLLS_MARGIN) continue;
      add_artifact_stats(artifact_stats, by_slot[FEATHER][b]);
      set_count[by_slot[FEATHER][b].set]++;

      for (int c = 0; c < size[SANDS]; c++) {
        if (by_slot[SANDS][c].stat_score <= max_set.artifacts[SANDS].stat_score - farming_config.stat_score_max * GOOD_ROLLS_MARGIN) continue;
        add_artifact_stats(artifact_stats, by_slot[SANDS][c]);
        set_count[by_slot[SANDS][c].set]++;

        for (int d = 0; d < size[GOBLET]; d++) {
          if (by_slot[GOBLET][d].stat_score <= max_set.artifacts[GOBLET].stat_score - farming_config.stat_score_max * GOOD_ROLLS_MARGIN) continue;
          add_artifact_stats(artifact_stats, by_slot[GOBLET][d]);
          set_count[by_slot[GOBLET][d].set]++;

          for (int e = 0; e < size[CIRCLET]; e++) {
            if (by_slot[CIRCLET][e].stat_score <= max_set.artifacts[CIRCLET].stat_score - farming_config.stat_score_max * GOOD_ROLLS_MARGIN) continue;
            add_artifact_stats(artifact_stats, by_slot[CIRCLET][e]);
            set_count[by_slot[CIRCLET][e].set]++;

            // Check for sufficient ER
            if (artifact_stats[ER] + character.stats[ER] + weapon.stats[ER] >= farming_config.required_er) {
              int curr_damage = calc_damage(character, weapon, artifact_stats, set_count);
              if (curr_damage > max_set.damage) {
                max_set.damage = curr_damage;
                max_set.artifacts[FLOWER] = by_slot[FLOWER][a];
                max_set.artifacts[FEATHER] = by_slot[FEATHER][b];
                max_set.artifacts[SANDS] = by_slot[SANDS][c];
                max_set.artifacts[GOBLET] = by_slot[GOBLET][d];
                max_set.artifacts[CIRCLET] = by_slot[CIRCLET][e];
              }
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
