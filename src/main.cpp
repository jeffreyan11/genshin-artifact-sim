#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "farm.h"
#include "gen_artifact.h"
#include "text_io.h"
#include "types.h"

namespace {

MainConfig main_config;
Character character = {};
Weapon weapon = {};

// Initialize all configs
bool initialize_configs() {
  if (!read_character_config(main_config.character, &character)) {
    std::cerr << "Error reading character config." << std::endl;
    return false;
  }
  if (!read_weapon_config(main_config.weapon, &weapon)) {
    std::cerr << "Error reading weapon config." << std::endl;
    return false;
  }
  return true;
}

}  // namespace

int main(/*int argc, char** argv*/) {
  std::cerr << "Genshin Artifact Simulator" << std::endl;

  if (!read_main_config(&main_config)) {
    std::cerr << "Error reading main config." << std::endl;
    return 1;
  }
  if (!initialize_configs()) {
    std::cerr << "Exiting program." << std::endl;
    return 1;
  }

  std::cerr << "Type \"help\" for a list of commands." << std::endl;
  std::string input;
  while (getline(std::cin, input)) {
    std::vector<std::string> input_list = split(input, ' ');
    if (input_list.size() <= 0) continue;

    if (input_list[0] == "farm") {
      int iters = std::stoi(input_list[1]);
      int artifacts_to_farm = std::stoi(input_list[2]);

      auto start = std::chrono::high_resolution_clock::now();

      FarmedSet* all_max_sets = new FarmedSet[iters];
      for (int i = 0; i < iters; i++) {
        all_max_sets[i] = farm(character, weapon, artifacts_to_farm);
      }

      auto end = std::chrono::high_resolution_clock::now();
      std::cerr << "Time: "
                << std::chrono::duration_cast<std::chrono::duration<double>>(end-start).count()
                << "s" << std::endl;

      print_statistics(character, all_max_sets, iters);
      delete[] all_max_sets;
      continue;
    }

    if (input_list[0] == "farm_one") {
      int artifacts_to_farm = std::stoi(input_list[1]);

      auto start = std::chrono::high_resolution_clock::now();

      FarmedSet max_set = farm(character, weapon, artifacts_to_farm);

      auto end = std::chrono::high_resolution_clock::now();
      std::cerr << "Time: "
                << std::chrono::duration_cast<std::chrono::duration<double>>(end-start).count()
                << "s" << std::endl;

      if (max_set.damage > 0) {
        for (int i = 0; i < SLOT_CT; i++)
          print_artifact(&(max_set.artifacts[i]));
        print_character(character, weapon, max_set.artifacts);
      } else {
        std::cerr << "No set with suitable mainstats found" << std::endl;
      }
      std::cerr << "Upgrade ratio: ";
      for (int i = 0; i < SLOT_CT; i++) {
        std::cerr << print_percentage(max_set.upgrade_ratio[i][0], max_set.upgrade_ratio[i][1]) << "% ";
      }
      std::cerr << std::endl;
      std::cerr << "Damage achieved: " << max_set.damage << std::endl;
      std::cerr << std::endl;
      continue;
    }

    if (input_list[0] == "farm_script") {
      int iters = std::stoi(input_list[1]);
      int start_n = std::stoi(input_list[2]);
      int stop_n = std::stoi(input_list[3]);
      int step = std::stoi(input_list[4]);

      std::ofstream output_file("output.csv");
      if (!output_file.is_open()) {
        std::cerr << "Error: failed to open output file for writing." << std::endl;
        continue;
      }
      output_file << "Artifacts farmed,Mean,Stddev,5%ile,25%ile,Median,75%ile,95%ile,Upgrade ratio" << std::endl;

      FarmedSet* all_max_sets = new FarmedSet[iters];
      for (int n = start_n; n <= stop_n; n += step) {
        std::cerr << "Farming " << n << " artifacts " << iters << " times..." << std::endl;
        for (int i = 0; i < iters; i++) {
          all_max_sets[i] = farm(character, weapon, n);
        }

        // Sort to easily find quantiles
        std::sort(all_max_sets, all_max_sets + iters, [](FarmedSet& s1, FarmedSet& s2) {
          return s1.damage < s2.damage;
        });
        // Calculate mean
        int64_t damage_total = 0;
        for (int i = 0; i < iters; i++)
          damage_total += all_max_sets[i].damage;
        double mean = double(damage_total) / iters;
        // Calculate standard deviation
        double stddev = 0.0;
        for (int i = 0; i < iters; i++) {
          stddev += (all_max_sets[i].damage - mean) * (all_max_sets[i].damage - mean);
        }
        stddev = sqrt(stddev / iters);

        // Calculate % of artifacts upgraded
        int64_t total_upgrade_ratio[2] = {0, 0};
        for (int i = 0; i < iters; i++) {
          for (int j = 0; j < SLOT_CT; j++) {
            total_upgrade_ratio[0] += all_max_sets[i].upgrade_ratio[j][0];
            total_upgrade_ratio[1] += all_max_sets[i].upgrade_ratio[j][1];
          }
        }

        output_file << n << ","
                    << mean << ","
                    << stddev << ","
                    << all_max_sets[iters/20].damage << ","
                    << all_max_sets[iters/4].damage << ","
                    << all_max_sets[iters/2].damage << ","
                    << all_max_sets[3*iters/4].damage << ","
                    << all_max_sets[19*iters/20].damage << ","
                    << print_percentage(total_upgrade_ratio[0], total_upgrade_ratio[1]) << "%" << std::endl;
      }
      std::cerr << "Done." << std::endl;
      std::cerr << std::endl;

      delete[] all_max_sets;
      continue;
    }

    if (input_list[0] == "roll") {
      int iters = std::stoi(input_list[1]);

      auto start = std::chrono::high_resolution_clock::now();

      Artifact* all_artis = get_artifact_storage(iters);
      for (int i = 0; i < iters; i++) {
        gen_random(all_artis + i, character.farming_config);
        upgrade_full(all_artis + i);
      }

      auto end = std::chrono::high_resolution_clock::now();
      std::cerr << "Time: "
                << std::chrono::duration_cast<std::chrono::duration<double>>(end-start).count()
                << "s" << std::endl;

      print_statistics(all_artis, iters);
      delete[] all_artis;
      continue;
    }

    if (input_list[0] == "roll_one") {
      Artifact arti;
      gen_random(&arti, character.farming_config);
      upgrade_full(&arti);
      print_artifact(&arti);
      std::cerr << std::endl;
      continue;
    }

    if (input_list[0] == "seed") {
      seed();
      std::cerr << "Random number generator seeded." << std::endl;
      std::cerr << std::endl;
      continue;
    }

    if (input_list[0] == "set") {
      std::string cfg_type = input_list[1];
      std::string filename = input_list[2];
      if (cfg_type == "character") {
        std::string old_character = main_config.character;
        main_config.character = filename;
        if(!initialize_configs()) {
          main_config.character = old_character;
          std::cerr << "Invalid character config given." << std::endl;
        }
      } else if (cfg_type == "weapon") {
        std::string old_weapon = main_config.weapon;
        main_config.weapon = filename;
        if(!initialize_configs()) {
          main_config.weapon = old_weapon;
          std::cerr << "Invalid weapon config given." << std::endl;
        }
      } else {
        std::cerr << "Invalid config_type given." << std::endl;
      }
      std::cerr << std::endl;
      continue;
    }

    if (input_list[0] == "settings") {
      std::cerr << "Current configs used: " << std::endl;
      std::cerr << "Character: " << main_config.character << std::endl;
      std::cerr << "Weapon: " << main_config.weapon << std::endl << std::endl;
      continue;
    }

    if (input_list[0] == "help") {
      std::cerr << "Commands:" << std::endl;
      std::cerr << "farm <iters> <n_artifacts>" << std::endl;
      std::cerr << "  Simulate <iters> people farming <n_artifacts> artifacts each\n"
                << "  and print a distribution of damage achieved." << std::endl;
      std::cerr << "farm_one <n_artifacts>" << std::endl;
      std::cerr << "  Farm <n_artifacts> artifacts and print the best set of artifacts achieved.\n"
                << "  For fun or debugging." << std::endl;
      std::cerr << "farm_script <iters> <start_n> <stop_n> <step>" << std::endl;
      std::cerr << "  Simulate <iters> people farming <n> artifacts each for every value of n from\n"
                << "  <start_n> to <stop_n> stepping by <step> and write results to a output.csv file." << std::endl;
      std::cerr << "roll <n>" << std::endl;
      std::cerr << "  Roll n artifacts and print some statistics." << std::endl;
      std::cerr << "roll_one" << std::endl;
      std::cerr << "  Roll one artifact and print it. For fun or debugging." << std::endl;
      std::cerr << "seed" << std::endl;
      std::cerr << "  Seed the RNG using current system time." << std::endl;
      std::cerr << "set <config_type> <value>" << std::endl;
      std::cerr << "  Change the character or weapon config to <value>." << std::endl;
      std::cerr << "settings" << std::endl;
      std::cerr << "  List current config settings." << std::endl;
      std::cerr << "quit" << std::endl;
      std::cerr << "  Exits the program." << std::endl;
      std::cerr << std::endl;
      continue;
    }

    if (input_list[0] == "quit") break;

    std::cerr << "unknown command: " << input_list[0] << std::endl << std::endl;
  }  // end input loop

  return 0;
}
