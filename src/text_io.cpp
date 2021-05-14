#include "text_io.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

#include "analyze.h"

namespace {

std::vector<std::pair<std::string, Stat>> stat_parse = {
  {"hp", HP},
  {"atk", ATK},
  {"def", DEF},
  {"hpp", HPP},
  {"atkp", ATKP},
  {"defp", DEFP},
  {"em", EM},
  {"er", ER},
  {"cr", CR},
  {"cd", CD},
  {"heal", HEAL},
  {"phys", PHYS},
  {"on_ele", ON_ELE},
  {"off_ele", OFF_ELE},
  {"reaction", REACTION},
  {"dmg_na", DMG_NA},
  {"dmg_ca", DMG_CA},
  {"dmg_skill", DMG_SKILL},
  {"dmg_burst", DMG_BURST}
};

std::vector<std::pair<std::string, Set>> set_parse = {
  {"No set", NONE},
  {"Gladiator\'s Finale", GLADIATORS},
  {"Wanderer\'s Troupe", WANDERERS},
  {"Noblesse Oblige", NOBLESSE},
  {"Bloodstained Chivalry", BLOODSTAINED},
  {"Maiden Beloved", MAIDENS},
  {"Viridescent Veneerer", VIRIDESCENT},
  {"Retracing Bolide", BOLIDE},
  {"Archaic Petra", PETRA},
  {"Crimson Witch of Flames", CRIMSON_WITCH},
  {"Lavawalker", LAVAWALKER},
  {"Heart of Depth", HEART_OF_DEPTH},
  {"Blizzard Strayer", BLIZZARD},
  {"Thundering Fury", THUNDERING_FURY},
  {"Thundersoother", THUNDERSOOTHER},
  {"Tenacity of the Millelith", MILLELITH},
  {"Pale Flame", PALE_FLAME}
};

std::vector<std::pair<std::string, Domain>> domain_parse = {
  {"Boss", BOSS},
  {"Clear Pool and Mountain Cavern", CLEAR_POOL_AND_MOUNTAIN_CAVERN},
  {"Valley of Remembrance", VALLEY_OF_REMEMBRANCE},
  {"Domain of Guyun", DOMAIN_OF_GUYUN},
  {"Hidden Palace of Zhou Formula", HIDDEN_PALACE_OF_ZHOU_FORMULA},
  {"Peak of Vindagnyr", PEAK_OF_VINDAGNYR},
  {"Midsummer Courtyard", MIDSUMMER_COURTYARD},
  {"Ridge Watch", RIDGE_WATCH},
};

bool read_farming_config(std::ifstream& config, FarmingConfig* fcfg) {
  // Clear the farming config
  *fcfg = {};

  std::string line;
  while (getline(config, line)) {
    // Ignore comment lines
    if (line[0] == '#') continue;
    // Ignore blank lines
    if (line.empty()) continue;

    // Farming config section is over
    if (line == "}") return true;

    std::vector<std::string> kv_pair = split(line, '=');
    // Ignore invalid lines
    if (kv_pair.size() < 2) {
      std::cerr << "Warning: invalid line " << line << std::endl;
      continue;
    };
    const std::string& key = kv_pair[0];
    const std::string& value = kv_pair[1];

    if (key == "domains") {
      const auto domain_list = split(value, ',');
      for (const auto& domain_str : domain_list) {
        auto it = std::find_if(
            domain_parse.begin(), domain_parse.end(),
            [&domain_str](const std::pair<std::string, Domain>& p) {
              return p.first == domain_str;
            });
        if (it != domain_parse.end()) {
          fcfg->domains.push_back(it->second);
        } else {
          std::cerr << "Invalid domain name " << domain_str << std::endl;
          return false;
        }
      }
    } else if (key == "target_sets_2pc" || key == "target_sets_4pc") {
      const auto set_list = split(value, ',');
      for (const auto& set_str : set_list) {
        auto it = std::find_if(
            set_parse.begin(), set_parse.end(),
            [&set_str](const std::pair<std::string, Set>& p) {
              return p.first == set_str;
            });
        if (it != set_parse.end()) {
          if (key == "target_sets_2pc")
            fcfg->target_sets[it->second][TWO_PC] = true;
          else
            fcfg->target_sets[it->second][FOUR_PC] = true;
        } else {
          std::cerr << "Invalid set " << set_str << std::endl;
          return false;
        }
      }
    } else if (key == "stat_score_max") {
      fcfg->stat_score_max = std::stoi(value);
    } else if (key == "mainstat_multiplier") {
      fcfg->mainstat_multiplier = std::stoi(value);
    } else if (key == "set_bonus_value") {
      fcfg->set_bonus_value = std::stoi(value);
    } else if (key == "required_er") {
      fcfg->required_er = std::stoi(value);
    } else if (key == "min_stat_score") {
      const auto min_score_list = split(value, ',');
      if (min_score_list.size() < SLOT_CT) {
        std::cerr << "Not enough values in min_stat_score list." << std::endl;
        return false;
      }
      for (int i = 0; i < SLOT_CT; i++) {
        fcfg->min_stat_score[i] = std::stoi(min_score_list[i]);
      }
    } else {
      auto it = std::find_if(
          stat_parse.begin(), stat_parse.end(),
          [&key](const std::pair<std::string, Stat>& p) {
            return p.first == key;
          });
      if (it != stat_parse.end()) {
        fcfg->stat_score[it->second] = std::stoi(value);
      } else {
        std::cerr << "Unknown key " << key << std::endl;
        return false;
      }
    }
  }

  return true;
}

}  // namespace

void print_statistics(Character& c, std::vector<FarmedSet>& all_max_sets) {
  FarmedSetStats stats = analyze_farmed_set(c, all_max_sets);

  std::cerr << "Mean damage: " << stats.mean << std::endl;
  std::cerr << "Stddev: " << stats.stddev << std::endl;
  std::cerr << "5%ile: " << stats.percentiles[5] << std::endl;
  std::cerr << "25%ile: " << stats.percentiles[25] << std::endl;
  std::cerr << "median: " << stats.percentiles[50] << std::endl;
  std::cerr << "75%ile: " << stats.percentiles[75] << std::endl;
  std::cerr << "95%ile: " << stats.percentiles[95] << std::endl;
  std::cerr << "Avg number of good (offensive stat) rolls: " << stats.good_rolls << std::endl;
  std::cerr << "Avg number of crit rolls: " << stats.crit_rolls << std::endl;
  std::cerr << "Upgrade ratio: ";
  for (int i = 0; i < SLOT_CT; i++) {
    std::cerr << print_percentage(stats.upgrade_ratio[i][0], stats.upgrade_ratio[i][1]) << "% ";
  }
  std::cerr << std::endl;
  std::cerr << "Set bonuses: 0pc " << stats.set_bonus_pcts[0]
            << "% | 2pc " << stats.set_bonus_pcts[1]
            << "% | 2pc + 2pc " << stats.set_bonus_pcts[2]
            << "% | 4pc " << stats.set_bonus_pcts[3] << "%" << std::endl;
  std::cerr << std::endl;
}

void print_statistics(Artifact* sample, int size) {
  int double_crit[5] = {0, 0, 0, 0, 0};
  for (int i = 0; i < size; i++) {
    const Artifact& a = sample[i];
    if (a.substat_values[CR] > 0 && a.substat_values[CD] > 0) {
      double_crit[a.slot]++;
    }
  }
  std::cerr << "Probability of getting an artifact with both crit substats, by slot: " << std::endl;
  std::cerr << print_percentage(double_crit[0], size) << "% "
            << print_percentage(double_crit[1], size) << "% "
            << print_percentage(double_crit[2], size) << "% "
            << print_percentage(double_crit[3], size) << "% "
            << print_percentage(double_crit[4], size) << "%" << std::endl;
  std::cerr << std::endl;
}

void print_character(Character& c, Weapon& w) {
  int total_stats[STAT_CT];
  // Character + weapon stats
  for (int i = 0; i < STAT_CT; i++)
    total_stats[i] = c.stats[i] + w.stats[i];

  int base_atk = c.base_atk + w.base_atk;
  int total_atk = base_atk * (1000 + total_stats[ATKP]) / 1000 + total_stats[ATK];

  std::cerr << "Total ATK: " << total_atk << std::endl;
  std::cerr << "Elemental Mastery: " << total_stats[EM] << std::endl;
  std::cerr << "Energy Recharge: " << total_stats[ER] / 10.0 << "%" << std::endl;
  std::cerr << "Crit Rate: " << total_stats[CR] / 10.0 << "%" << std::endl;
  std::cerr << "Crit DMG: " << total_stats[CD] / 10.0 << "%" << std::endl;
  std::cerr << "Total DMG%: " << total_stats[ON_ELE] + total_stats[c.damage_type] / 10.0 << "%" << std::endl;
  std::cerr << std::endl;
}

void print_character(Character& c, Weapon& w, Artifact* artifacts) {
  int total_stats[STAT_CT];
  // Character + weapon stats
  for (int i = 0; i < STAT_CT; i++)
    total_stats[i] = c.stats[i] + w.stats[i];

  // Artifact stats
  int set_count[SET_CT];
  for (int i = 0; i < SET_CT; i++)
    set_count[i] = 0;

  for (int i = 0; i < SLOT_CT; i++) {
    set_count[artifacts[i].set]++;
    total_stats[artifacts[i].mainstat] += MAINSTAT_LEVEL[artifacts[i].mainstat];
    for (int j = 0; j < SUBSTAT_CT; j++) {
      total_stats[j] += artifacts[i].substat_values[j];
    }
  }

  // Set bonuses
  std::string set_str = "";
  for (int i = 0; i < SET_CT; i++) {
    // Only consider bonuses for target sets
    if (set_count[i] >= 2 && c.farming_config.target_sets[i][TWO_PC]) {
      set_str += "2 " + print_set(static_cast<Set>(i)) + " ";
      StatBonus sb = set_effect(static_cast<Set>(i), TWO_PC);
      for (int j = 0; j < STAT_CT; j++)
        total_stats[j] += sb.stats[j];
    }
    if (set_count[i] >= 4 && c.farming_config.target_sets[i][FOUR_PC]) {
      set_str = "4 " + print_set(static_cast<Set>(i));
      StatBonus sb = set_effect(static_cast<Set>(i), FOUR_PC);
      for (int j = 0; j < STAT_CT; j++)
        total_stats[j] += sb.stats[j];
    }
  }

  int base_atk = c.base_atk + w.base_atk;
  int total_atk = base_atk * (1000 + total_stats[ATKP]) / 1000 + total_stats[ATK];

  std::cerr << set_str << std::endl;
  std::cerr << "Total ATK: " << total_atk << std::endl;
  std::cerr << "Elemental Mastery: " << total_stats[EM] << std::endl;
  std::cerr << "Energy Recharge: " << total_stats[ER] / 10.0 << "%" << std::endl;
  std::cerr << "Crit Rate: " << total_stats[CR] / 10.0 << "%" << std::endl;
  std::cerr << "Crit DMG: " << total_stats[CD] / 10.0 << "%" << std::endl;
  std::cerr << "Total DMG%: " << (total_stats[ON_ELE] + total_stats[c.damage_type]) / 10.0 << "%" << std::endl;

  std::cerr << "From artifacts:" << std::endl;
  std::cerr << " ATK: " << total_atk - c.base_atk - w.base_atk << std::endl;
  std::cerr << " Elemental Mastery: " << total_stats[EM] - c.stats[EM] - w.stats[EM] << std::endl;
  std::cerr << " Energy Recharge: " << (total_stats[ER] - c.stats[ER] - w.stats[ER]) / 10.0 << "%" << std::endl;
  std::cerr << " Crit Rate: " << (total_stats[CR] - c.stats[CR] - w.stats[CR]) / 10.0 << "%" << std::endl;
  std::cerr << " Crit DMG: " << (total_stats[CD] - c.stats[CD] - w.stats[CD]) / 10.0 << "%" << std::endl;
  std::cerr << " Total DMG%: " << (total_stats[ON_ELE] - c.stats[ON_ELE] - w.stats[ON_ELE]) / 10.0 << "%" << std::endl;
  std::cerr << std::endl;
}

void print_artifact(const Artifact* const arti) {
  std::cerr << print_set(arti->set) << "\n"
            << print_slot(arti->slot) << ": "
            << print_stat_value(arti->mainstat, MAINSTAT_LEVEL[arti->mainstat]) << " "
            << print_stat(arti->mainstat) << std::endl;
  for (int i = 0; i < 4; i++) {
    const Stat substat = static_cast<Stat>(arti->substats[i]);
    std::cerr << print_stat_value(substat, arti->substat_values[substat]) << " "
              << print_stat(substat) << std::endl;
  }
}

std::string print_slot(Slot s) {
  switch (s) {
    case FLOWER:
      return "Flower";
    case FEATHER:
      return "Feather";
    case SANDS:
      return "Sands";
    case GOBLET:
      return "Goblet";
    case CIRCLET:
      return "Circlet";
  }
  return "Unknown slot";
}

std::string print_stat(Stat s) {
  switch (s) {
    case HP:
      return "HP";
    case ATK:
      return "ATK";
    case DEF:
      return "DEF";
    case HPP:
      return "HP%";
    case ATKP:
      return "ATK%";
    case DEFP:
      return "DEF%";
    case EM:
      return "EM";
    case ER:
      return "ER%";
    case CR:
      return "Crit Rate%";
    case CD:
      return "Crit Dmg%";
    case HEAL:
      return "Healing Bonus";
    case PHYS:
      return "Phys%";
    case ON_ELE:
      return "Element% (correct element)";
    case OFF_ELE:
      return "Element% (wrong element)";
    case REACTION:
      return "Reaction Bonus";
    case DMG_NONE:
      return "Dmg% (none)";
    case DMG_NA:
      return "Dmg% (Normal Attack)";
    case DMG_CA:
      return "Dmg% (Charged Attack)";
    case DMG_SKILL:
      return "Dmg% (Skill)";
    case DMG_BURST:
      return "Dmg% (Burst)";
  }
  return "Unknown stat";
}

std::string print_set(Set s) {
  auto it = std::find_if(
      set_parse.begin(), set_parse.end(),
      [&s](const std::pair<std::string, Set>& p) {
        return p.second == s;
      });
  if (it != set_parse.end()) {
    return it->first;
  } else {
    return "Unknown set";
  }
}

double print_stat_value(Stat s, int v) {
  if (s == HP || s == ATK || s == DEF || s == EM) {
    return v;
  }
  return v / 10.0;
}

double print_percentage(int num, int denom) {
  double percentage = 100.0 * num / denom;
  return round(percentage * 100.0) / 100.0;
}

std::vector<std::string> split(const std::string &s, char d) {
    std::vector<std::string> v;
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, d)) {
        v.push_back(item);
    }
    return v;
}

bool read_main_config(MainConfig* mcfg) {
  std::ifstream config("config/main.cfg");
  if (!config.is_open()) return false;

  std::string line;
  while (getline(config, line)) {
    // Ignore comment lines
    if (line[0] == '#') continue;
    // Ignore blank lines
    if (line.empty()) continue;

    std::vector<std::string> kv_pair = split(line, '=');
    // Ignore invalid lines
    if (kv_pair.size() < 2) {
      std::cerr << "Warning: invalid line " << line << std::endl;
      continue;
    };
    const std::string& key = kv_pair[0];
    const std::string& value = kv_pair[1];

    if (key == "character") {
      mcfg->character = value;
    } else if (key == "weapon") {
      mcfg->weapon = value;
    } else {
      std::cerr << "Unknown key " << key << std::endl;
      return false;
    }
  }

  return true;
}

bool read_character_config(std::string filename, Character* c) {
  std::ifstream config("config/characters/" + filename + ".cfg");
  if (!config.is_open()) return false;

  // Clear character config
  *c = {};

  std::string line;
  while (getline(config, line)) {
    // Ignore comment lines
    if (line[0] == '#') continue;
    // Ignore blank lines
    if (line.empty()) continue;

    if (line == "farming_config {") {
      bool result = read_farming_config(config, &c->farming_config);
      if (!result) return false;
      continue;
    }

    std::vector<std::string> kv_pair = split(line, '=');
    // Ignore invalid lines
    if (kv_pair.size() < 2) {
      std::cerr << "Warning: invalid line " << line << std::endl;
      continue;
    };
    const std::string& key = kv_pair[0];
    const std::string& value = kv_pair[1];

    if (key == "base_atk") {
      c->base_atk = std::stoi(value);
    } else if (key == "reaction_multiplier_x10") {
      c->reaction_multiplier_x10 = std::stoi(value);
    } else if (key == "reaction_percentage") {
      c->reaction_percentage = std::stoi(value);
    } else if (key == "damage_type") {
      auto it = std::find_if(
          stat_parse.begin(), stat_parse.end(),
          [&value](const std::pair<std::string, Stat>& p) {
            return p.first == value;
          });
      if (it != stat_parse.end() && Character::valid_damage_type(it->second)) {
        c->damage_type = it->second;
      } else {
        std::cerr << "Invalid damage type " << value << std::endl;
        return false;
      }
    } else {
      auto it = std::find_if(
          stat_parse.begin(), stat_parse.end(),
          [&key](const std::pair<std::string, Stat>& p) {
            return p.first == key;
          });
      if (it != stat_parse.end()) {
        c->stats[it->second] = std::stoi(value);
      } else {
        std::cerr << "Unknown key " << key << std::endl;
        return false;
      }
    }
  }

  return true;
}

bool read_weapon_config(std::string filename, Weapon* w) {
  std::ifstream config("config/weapons/" + filename + ".cfg");
  if (!config.is_open()) return false;

  // Clear weapon config
  *w = {};

  std::string line;
  while (getline(config, line)) {
    // Ignore comment lines
    if (line[0] == '#') continue;
    // Ignore blank lines
    if (line.empty()) continue;

    std::vector<std::string> kv_pair = split(line, '=');
    // Ignore invalid lines
    if (kv_pair.size() < 2) {
      std::cerr << "Warning: invalid line " << line << std::endl;
      continue;
    };
    const std::string& key = kv_pair[0];
    const std::string& value = kv_pair[1];

    if (key == "base_atk") {
      w->base_atk = std::stoi(value);
    } else {
      auto it = std::find_if(
          stat_parse.begin(), stat_parse.end(),
          [&key](const std::pair<std::string, Stat>& p) {
            return p.first == key;
          });
      if (it != stat_parse.end()) {
        w->stats[it->second] = std::stoi(value);
      } else {
        std::cerr << "Unknown key " << key << std::endl;
        return false;
      }
    }
  }

  return true;
}
