#ifndef __TEXT_IO_H__
#define __TEXT_IO_H__

#include <string>
#include <vector>

#include "farm.h"
#include "types.h"

// Print some statistics about a profile of damage achieved across a population.
// Modifies damage_achieved.
void print_statistics(Character& c, FarmedSet* all_max_sets, int size);

// Print some basic statistics about a sample of +20 artifacts.
void print_statistics(Artifact* sample, int size);

// Print overall stats for a character (attack, total cr, total cd, etc.) with or without artifacts.
void print_character(Character& c, Weapon& w);
void print_character(Character& c, Weapon& w, Artifact* artifacts);

// Pretty print an artifact.
void print_artifact(const Artifact* arti);

// Other print utility functions.
std::string print_slot(Slot s);
std::string print_stat(Stat s);
std::string print_set(Set s);
double print_stat_value(Stat s, int v);
double print_percentage(int num, int denom);

// Splits a string s with delimiter d.
std::vector<std::string> split(const std::string &s, char d);

// Read a character config from relative path config/characters/<filename>.cfg
bool read_character_config(std::string filename, Character* c);
// Read a weapon config from relative path config/weapons/<filename>.cfg
bool read_weapon_config(std::string filename, Weapon* w);

#endif
