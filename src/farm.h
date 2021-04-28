#ifndef __FARM_H__
#define __FARM_H__

#include "types.h"

struct FarmedSet {
  Artifact artifacts[SLOT_CT];
  int damage;
  int upgrade_ratio[2];

  FarmedSet() : damage(0), upgrade_ratio{0, 0} {}
  FarmedSet(const FarmedSet& other) = default;
  FarmedSet& operator=(const FarmedSet& other) = default;
  ~FarmedSet() = default;
};

// Farm n artifacts for given character and weapon and return the damage modifier achieved.
// If no offensive mainstat is achieved for any slot, the optimizer will return 0 damage.
FarmedSet farm(Character& character, Weapon& weapon, FarmingConfig& farming_config, int n);

#endif
