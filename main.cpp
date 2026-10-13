#include <mod/amlmod.h>
#include <mod/logger.h>
#include "src/HealthBar.h"

MYMOD(xavion.npchealthbar, NPCHealthBar, 1.0, Xavion)

ON_MOD_LOAD() {
    logger->SetTag("NPCHealthBar");
    logger->Info("NPC Health Bar plugin loaded");

    HealthBar::Init();
}
