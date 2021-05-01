#ifndef __TYPES_H__
#define __TYPES_H__

#include <string>
#include <vector>

// Define all artifact probability constants.
enum Slot {
  FLOWER = 0, FEATHER, SANDS, GOBLET, CIRCLET
};
constexpr int SLOT_CT = 5;

enum Stat {
  // Possible mainstats: 14
  HP = 0, ATK, DEF, HPP, ATKP, DEFP, EM, ER, CR, CD, HEAL, PHYS, ON_ELE, OFF_ELE,
  // Other damage types: 6
  REACTION, DMG_NONE, DMG_NA, DMG_CA, DMG_SKILL, DMG_BURST
};
constexpr int MAINSTAT_CT = 14;
constexpr int STAT_CT = 20;

enum Set {
  NONE = 0,
  GLADIATORS, WANDERERS,
  NOBLESSE, BLOODSTAINED,
  MAIDENS, VIRIDESCENT,
  BOLIDE, PETRA,
  CRIMSON_WITCH, LAVAWALKER,
  HEART_OF_DEPTH, BLIZZARD,
  THUNDERING_FURY, THUNDERSOOTHER,
  MILLELITH, PALE_FLAME
};
constexpr int SET_CT = 17;

enum Domain {
  BOSS = 0,
  CLEAR_POOL_AND_MOUNTAIN_CAVERN,
  VALLEY_OF_REMEMBRANCE,
  DOMAIN_OF_GUYUN,
  HIDDEN_PALACE_OF_ZHOU_FORMULA,
  PEAK_OF_VINDAGNYR,
  MIDSUMMER_COURTYARD,
  RIDGE_WATCH
};
constexpr int DOMAIN_CT = 8;

// Cumulative probability tables for rolling each type of main stat.
extern const int MAINSTAT_WEIGHT[SLOT_CT][MAINSTAT_CT];
// The amount of each main stat given by a +20 5*.
extern const int MAINSTAT_LEVEL[MAINSTAT_CT];

constexpr int SUBSTAT_CT = 10;
constexpr int SUBSTAT_WEIGHT_TOTAL = 44;
// PDF table for rolling each type of substat.
extern const int SUBSTAT_WEIGHT[SUBSTAT_CT];
// The amount of each substat given per level up.
extern const int SUBSTAT_LEVEL[SUBSTAT_CT][4];

// 1/5 chance of starting with 4 substats
constexpr int EXTRA_SUBSTAT_PROB = 5;

// Stat bonuses from set effects.
struct StatBonus {
  int stats[STAT_CT];
};

// Number of pieces required to form the set
enum SetPieces {
  TWO_PC = 0, FOUR_PC,
  SET_PIECES_CT
};
// Returns the artifact set effect (2 or 4 piece). If a 4p set effect is requested,
// the 2p set effect is not included.
StatBonus set_effect(Set s, SetPieces pieces);

extern const Set DOMAIN_TO_SET[DOMAIN_CT][2];

// Stores the data for one artifact.
struct Artifact {
  Slot slot;
  Stat mainstat;
  Set set;
  int level;
  int stat_score;
  int substats[4];
  int substat_values[SUBSTAT_CT];
  bool extra_substat;

  // Default constructor initializing all values to 0
  Artifact();
};

// Get a dynamically allocated array of zero-initialized Artifacts.
Artifact* get_artifact_storage(int size);

// Stores the stats profile for a character and weapon.
struct Character {
  int base_atk;
  int reaction_multiplier_x10;
  int reaction_percentage;
  // Damage type to optimize for. Should be NONE, NA, CA, SKILL, or BURST.
  Stat damage_type;
  // target_set is true if the set is useful for this character profile.
  // In a config file, this should be expressed as two lists of target sets.
  bool target_sets[SET_CT][SET_PIECES_CT];
  int stats[STAT_CT];

  static bool valid_damage_type(Stat s) {
    return (s == DMG_NONE) || (s == DMG_NA) || (s == DMG_CA) || (s == DMG_SKILL) || (s == DMG_BURST);
  }
};

struct Weapon {
  int base_atk;
  int stats[STAT_CT];
};

// Stores the parameters governing player behavior when farming.
struct FarmingConfig {
  // All domains to farm in a round robin
  std::vector<Domain> domains;
  unsigned int domain_idx;

  // stat_score contains the score assigned for each roll of a stat if it is present
  int stat_score[MAINSTAT_CT];
  // The max value present in stat_score. This controls heuristics for finding the artifact set with highest damage.
  int stat_score_max;
  // The number of substat rolls that a correct mainstat is worth. Usually 6-8 is a good estimate.
  int mainstat_multiplier;
  // The score that an onset piece is worth.
  // 1-2x of stat_score_max is a good estimate, more for high value sets such as CW, BS, VV.
  int set_bonus_value;
  // The minimum score necessary at +0 for an artifact to be leveled to +20
  int min_stat_score[SLOT_CT];

  Domain next_domain() {
    Domain d = domains[domain_idx];
    domain_idx = (domain_idx + 1) % domains.size();
    return d;
  }

  // Returns an overall stat score for the given artifact, based on number of good sub rolls
  // calculated by weights given in the stat_score array.
  int score(Character& c, Artifact& a);

  // Returns whether the artifact should be leveled to +20
  bool upgradeable(Character& c, Artifact& a);
};

// Stores a list of all other configs used
struct MainConfig {
  std::string character;
  std::string weapon;
};

#endif
