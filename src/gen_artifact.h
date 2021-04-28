#ifndef __GEN_ARTIFACT_H__
#define __GEN_ARTIFACT_H__

#include "types.h"

// Seed the RNG using current system time.
void seed();

// Fills arti with a randomly generated +0 artifact.
// Requires arti to be zero-initialized.
void gen_random(Artifact* arti, FarmingConfig& fcfg);
// Upgrades arti from +0 to +20
void upgrade_full(Artifact* arti);

#endif
