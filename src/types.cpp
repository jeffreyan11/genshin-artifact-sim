#include "types.h"

#include <algorithm>
#include <cstring>

// Stat roll distributions taken from https://genshin-impact.fandom.com/wiki/Artifacts/Stat_Roll_Distribution
// Original data source: https://github.com/Dimbreath/GenshinData
const int MAINSTAT_WEIGHT[SLOT_CT][MAINSTAT_CT] = {
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  {0, 0, 0, 80, 160, 240, 270, 300, 300, 300, 300, 300, 300, 300},
  {0, 0, 0, 85, 170, 250, 260, 260, 260, 260, 260, 280, 300, 400},
  {0, 0, 0, 22, 44, 66, 70, 70, 80, 90, 100, 100, 100, 100}
};

const int MAINSTAT_LEVEL[MAINSTAT_CT] = {
  4780, 311, 0,
  466, 466, 583,
  187, 518,
  311, 622,
  359,
  583, 466, 466
};

const int SUBSTAT_WEIGHT[SUBSTAT_CT] = {
  6, 6, 6,        // flat stats
  4, 4, 4, 4, 4,  // % stats
  3, 3            // crit
};

const int SUBSTAT_LEVEL[SUBSTAT_CT][4] = {
  {209, 239, 269, 299},  // flat hp
  {14, 16, 18, 19},      // flat atk
  {16, 19, 21, 23},      // flat def
  {41, 47, 53, 58},      // hp%
  {41, 47, 53, 58},      // atk%
  {51, 58, 66, 73},      // def%
  {16, 19, 21, 23},      // EM
  {45, 52, 58, 65},      // ER
  {27, 31, 35, 39},      // CR
  {54, 62, 70, 78}       // CD
};

// TODO: properly implement all set effects, as well as separate damage types.
StatBonus set_effect(Set s, int pieces) {
  StatBonus sb;
  if (pieces == TWO_PC) {
    switch (s) {
      case NONE:
        break;
      case GLADIATORS:
      case WANDERERS:
        break;
      case NOBLESSE:
        sb.stats[DMG_BURST] = 200;
        break;
      case BLOODSTAINED:
        sb.stats[PHYS] = 250;
        break;
      case MAIDENS:
      case VIRIDESCENT:
      case BOLIDE:
      case PETRA:
      case CRIMSON_WITCH:
      case LAVAWALKER:
      case HEART_OF_DEPTH:
      case BLIZZARD:
        break;
      case THUNDERING_FURY:
        sb.stats[ON_ELE] = 150;
        break;
      case THUNDERSOOTHER:
        // No damage effect
        break;
      case MILLELITH:
      case PALE_FLAME:
        break;
    }
  } else if (pieces == FOUR_PC) {
    switch (s) {
      case NONE:
        break;
      case GLADIATORS:
      case WANDERERS:
        break;
      case NOBLESSE:
        sb.stats[ATKP] = 200;
        break;
      case BLOODSTAINED:
        // No damage effect
        break;
      case MAIDENS:
      case VIRIDESCENT:
      case BOLIDE:
      case PETRA:
      case CRIMSON_WITCH:
      case LAVAWALKER:
      case HEART_OF_DEPTH:
      case BLIZZARD:
        break;
      case THUNDERING_FURY:
        // No damage effect
        break;
      case THUNDERSOOTHER:
        sb.stats[ON_ELE] = 350;
        break;
      case MILLELITH:
      case PALE_FLAME:
        break;
    }
  }
  return sb;
}

const Set DOMAIN_TO_SET[DOMAIN_CT][2] = {
  {GLADIATORS, WANDERERS},
  {NOBLESSE, BLOODSTAINED},
  {MAIDENS, VIRIDESCENT},
  {BOLIDE, PETRA},
  {CRIMSON_WITCH, LAVAWALKER},
  {HEART_OF_DEPTH, BLIZZARD},
  {THUNDERING_FURY, THUNDERSOOTHER},
  {MILLELITH, PALE_FLAME}
};

Artifact::Artifact() {
  std::memset(static_cast<void*>(this), 0, sizeof(Artifact));
}

Artifact* get_artifact_storage(int size) {
  Artifact* artifacts = new Artifact[size];
  std::memset(static_cast<void*>(artifacts), 0, size * sizeof(Artifact));
  return artifacts;
}

int FarmingConfig::score(Artifact& a) {
  int subs = (a.extra_substat || a.level >= 4) ? 4 : 3;
  int score = mainstat_multiplier * stat_score[a.mainstat];
  for (int i = 0; i < subs; i++) {
    // Take a weighted estimate of number of good substat rolls
    score += stat_score[a.substats[i]] * a.substat_values[a.substats[i]] / SUBSTAT_LEVEL[a.substats[i]][0];
  }
  return score;
}

bool FarmingConfig::upgradeable(Artifact& a) {
  return score(a) >= min_stat_score[a.slot];
}
