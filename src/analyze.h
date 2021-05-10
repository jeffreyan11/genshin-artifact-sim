#ifndef __ANALYZE_H__
#define __ANALYZE_H__

#include "farm.h"
#include "types.h"

// Contains statistics about a farmed set such as mean, median, quantiles, sets present, etc.
struct FarmedSetStats {
  double mean, stddev;

  int percentiles[101];

  // Good rolls are CR, CD, ATK% (DEF%/HP% for DEF/HP scalers), EM if reaction-based
  double good_rolls, crit_rolls;

  // % of farmed artifacts that are rolled to +20, per slot and overall
  int64_t upgrade_ratio[SLOT_CT][2];
  int64_t total_upgrade_ratio[2];

  // % of sets with each set bonus combination
  // idx 0: no set bonus
  // idx 1: 2pc + 3 offset
  // idx 2: 2pc + 2pc
  // idx 3: 4pc
  double set_bonus_pcts[4];
};

// Takes a sample of farmed artifacts and returns interesting statistics about the sample.
FarmedSetStats analyze_farmed_set(Character& c, std::vector<FarmedSet>& all_max_sets);

#endif
