#include "analyze.h"

#include <algorithm>

namespace {

// Determines the set bonuses present in the given FarmedSet.
int get_set_bonuses(Character& c, FarmedSet& s) {
  int set_count[SET_CT];
  for (int i = 0; i < SET_CT; i++)
    set_count[i] = 0;
  for (int i = 0; i < SLOT_CT; i++)
    set_count[s.artifacts[i].set]++;

  int two_pc = 0, four_pc = 0;
  for (int i = 0; i < SET_CT; i++) {
    // Only consider bonuses for target sets
    if (set_count[i] >= 4 && c.farming_config.target_sets[i][FOUR_PC]) {
      four_pc = 1;
      break;
    } else if (set_count[i] >= 2 && c.farming_config.target_sets[i][TWO_PC]) {
      two_pc++;
    }
  }
  return two_pc + 3 * four_pc;
}

}  // namespace

FarmedSetStats analyze_farmed_set(Character& c, std::vector<FarmedSet>& all_max_sets) {
  // Initialize POD to zero
  FarmedSetStats stats = {};

  // Sort to easily find quantiles
  std::sort(all_max_sets.begin(), all_max_sets.end(), [](FarmedSet& s1, FarmedSet& s2) {
    return s1.damage < s2.damage;
  });
  int size = (int) all_max_sets.size();

  for (int i = 0; i < 101; i++) {
    stats.percentiles[i] = all_max_sets[i*size/100].damage;
  }

  // Calculate mean
  int64_t damage_total = 0;
  for (int i = 0; i < size; i++)
    damage_total += all_max_sets[i].damage;
  stats.mean = double(damage_total) / size;

  // Calculate standard deviation
  stats.stddev = 0.0;
  for (int i = 0; i < size; i++) {
    stats.stddev += (all_max_sets[i].damage - stats.mean) * (all_max_sets[i].damage - stats.mean);
  }
  stats.stddev = sqrt(stats.stddev / size);

  // Count average number of good substat rolls and crit value
  int num_substat_rolls[SUBSTAT_CT];
  for (int i = 0; i < SUBSTAT_CT; i++)
    num_substat_rolls[i] = 0;
  int crit_value = 0;
  for (int i = 0; i < size; i++) {
    // Skip incomplete sets and count them as 0 rolls
    if (all_max_sets[i].damage == 0) continue;
    for (int j = 0; j < SLOT_CT; j++) {
      Artifact& a = all_max_sets[i].artifacts[j];
      for (int k = 0; k < 4; k++) {
        num_substat_rolls[a.substats[k]] += a.substat_values[a.substats[k]] / SUBSTAT_LEVEL[a.substats[k]][0];
      }
      crit_value += 2 * a.substat_values[CR] + a.substat_values[CD];
    }
  }

  static const std::vector<Stat> POSSIBLE_OFFENSIVE_STATS = {HPP, ATKP, DEFP, EM, CR, CD};
  int good_rolls = 0;
  for (unsigned int i = 0; i < POSSIBLE_OFFENSIVE_STATS.size(); i++) {
    Stat s = POSSIBLE_OFFENSIVE_STATS[i];
    if (c.farming_config.stat_score[s] > 0) {
      good_rolls += num_substat_rolls[s];
    }
  }
  stats.good_rolls = round(100.0 * good_rolls / size) / 100.0;

  stats.crit_value = round(10.0 * crit_value / size) / 10.0 / 10.0;

  // Calculate % of artifacts upgraded
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < SLOT_CT; j++) {
      stats.upgrade_ratio[j][0] += all_max_sets[i].upgrade_ratio[j][0];
      stats.upgrade_ratio[j][1] += all_max_sets[i].upgrade_ratio[j][1];
    }
  }
  for (int i = 0; i < SLOT_CT; i++) {
    stats.total_upgrade_ratio[0] += stats.upgrade_ratio[i][0];
    stats.total_upgrade_ratio[1] += stats.upgrade_ratio[i][1];
  }

  // Calculate set bonus distribution
  for (int i = 0; i < size; i++) {
    stats.set_bonus_pcts[get_set_bonuses(c, all_max_sets[i])]++;
  }
  for (int i = 0; i < 4; i++) {
    stats.set_bonus_pcts[i] = round(stats.set_bonus_pcts[i] * 10000.0 / size) / 100.0;
  }

  return stats;
}
