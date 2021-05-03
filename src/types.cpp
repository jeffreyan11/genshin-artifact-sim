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

// TODO: add conditions for set effects such as 4CW and 4BS.
StatBonus set_effect(Set s, SetPieces pieces) {
  static const StatBonus SET_BONUSES[SET_PIECES_CT][SET_CT] = {
    {  // 2 piece set bonuses
    //   HP,  ATK,  DEF,  HPP, ATKP, DEFP,   EM,   ER,   CR,   CD, HEAL, PHYS,ON_ELE,OFF_ELE,RXN, NONE,   NA,   CA,SKILL,BURST
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // none
    {{    0,    0,    0,    0,  180,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // gladiators
    {{    0,    0,    0,    0,    0,    0,   80,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // wanderers
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  200}},  // noblesse
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  250,    0,    0,    0,    0,    0,    0,    0,    0}},  // bloodstained
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  150,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // maidens
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  150,    0,    0,    0,    0,    0,    0,    0}},  // viridescent
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // bolide
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  150,    0,    0,    0,    0,    0,    0,    0}},  // petra
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  150,    0,    0,    0,    0,    0,    0,    0}},  // crimson witch
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // lavawalker
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  150,    0,    0,    0,    0,    0,    0,    0}},  // heart of depth
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  150,    0,    0,    0,    0,    0,    0,    0}},  // blizzard
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  150,    0,    0,    0,    0,    0,    0,    0}},  // thundering fury
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // thundersoother
    {{    0,    0,    0,  200,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // millelith
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  250,    0,    0,    0,    0,    0,    0,    0,    0}},  // pale flame
    },
    {  // 4 piece set bonuses
    //   HP,  ATK,  DEF,  HPP, ATKP, DEFP,   EM,   ER,   CR,   CD, HEAL, PHYS,ON_ELE,OFF_ELE,RXN, NONE,   NA,   CA,SKILL,BURST
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // none
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  350,    0,    0,    0}},  // gladiators
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  350,    0,    0}},  // wanderers
    {{    0,    0,    0,    0,  200,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // noblesse
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // bloodstained
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  200,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // maidens
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // viridescent
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  400,  400,    0,    0}},  // bolide
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // petra
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   75,    0,   15,    0,    0,    0,    0,    0}},  // crimson witch
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  350,    0,    0,    0,    0,    0,    0,    0}},  // lavawalker
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  300,  300,    0,    0}},  // heart of depth
    {{    0,    0,    0,    0,    0,    0,    0,    0,  400,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // blizzard
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // thundering fury
    {{    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  350,    0,    0,    0,    0,    0,    0,    0}},  // thundersoother
    {{    0,    0,    0,    0,  200,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0}},  // millelith
    {{    0,    0,    0,    0,  180,    0,    0,    0,    0,    0,    0,  250,    0,    0,    0,    0,    0,    0,    0,    0}},  // pale flame
    }
  };
  StatBonus sb = SET_BONUSES[pieces][s];
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
  if (target_sets[a.set][TWO_PC] || target_sets[a.set][FOUR_PC])
    score += set_bonus_value;
  for (int i = 0; i < subs; i++) {
    // Take a weighted estimate of number of good substat rolls
    score += stat_score[a.substats[i]] * a.substat_values[a.substats[i]] / SUBSTAT_LEVEL[a.substats[i]][0];
  }
  return score;
}

bool FarmingConfig::upgradeable(Artifact& a) {
  return score(a) >= min_stat_score[a.slot];
}
