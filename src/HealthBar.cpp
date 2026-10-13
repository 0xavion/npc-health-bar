#include "HealthBar.h"

#include <plugin-sdk/game_sa/Events.h>
#include <plugin-sdk/game_sa/base/Timer.h>
#include <plugin-sdk/game_sa/engine/RsGlobal.h>
#include <plugin-sdk/game_sa/engine/Sprite.h>
#include <plugin-sdk/game_sa/engine/Sprite2d.h>
#include <plugin-sdk/game_sa/engine/World.h>
#include <plugin-sdk/game_sa/entity/Camera.h>
#include <plugin-sdk/game_sa/entity/Ped.h>
#include <plugin-sdk/game_sa/entity/PlayerPed.h>
#include <plugin-sdk/game_sa/other/Pools.h>
#include <plugin-sdk/gta_base/RGBA.h>
#include <plugin-sdk/gta_base/Rect.h>
#include <plugin-sdk/gta_base/Vector.h>
#include <plugin-sdk/renderware/RwVector.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace HealthBar {

// Configuration constants
static constexpr int MAX_TRACKED_PEDS = 256;
static constexpr uint32_t HIT_DISPLAY_DURATION_MS = 15000; // 15 seconds after last hit
static constexpr uint32_t DEATH_FADE_DURATION_MS = 2000;   // 2 seconds linger on death
static constexpr float MAX_DRAW_DISTANCE = 35.0f;
static constexpr float MAX_DRAW_DIST_SQR = MAX_DRAW_DISTANCE * MAX_DRAW_DISTANCE;

// Fixed array tracking per pool slot: zero dynamic allocation, zero leak risk
struct PedTrackInfo {
    int pedRef;
    uint32_t showUntil;
    float lastHealth;
    float lastArmour;
};

static PedTrackInfo s_PedTrack[MAX_TRACKED_PEDS];

static void ResetAllTracking() {
    std::memset(s_PedTrack, 0, sizeof(s_PedTrack));
}

static void DrawBars(CPed *ped, float screenX, float screenY, float dist, float alphaFactor) {
    const float screenW = (float)RsGlobal.maximumWidth;
    const float screenH = (float)RsGlobal.maximumHeight;

    const float baseWidth = screenW * 0.055f;
    const float baseHeight = screenH * 0.010f;

    // Scale down slightly as distance increases (closer = clear, farther = unobtrusive)
    const float distScale = std::clamp(1.0f - (dist / MAX_DRAW_DISTANCE) * 0.45f, 0.55f, 1.0f);
    const float barWidth = std::max(baseWidth * distScale, 36.0f);
    const float barHeight = std::max(baseHeight * distScale, 6.0f);
    const float halfW = barWidth * 0.5f;
    const float halfH = barHeight * 0.5f;

    const unsigned char alphaMain = (unsigned char)(230.0f * alphaFactor);
    const unsigned char alphaBg = (unsigned char)(160.0f * alphaFactor);
    const unsigned char alphaBrd = (unsigned char)(200.0f * alphaFactor);

    if (alphaMain == 0)
        return;

    const bool hasArmour = (ped->m_fArmour > 0.0f);
    float healthCenterY = screenY;

    if (hasArmour) {
        // Draw armour bar directly above health bar
        const float armourH = barHeight * 0.65f;
        const float armourHalfH = armourH * 0.5f;
        const float armourCenterY = screenY - halfH - armourHalfH - 2.0f;
        healthCenterY = screenY + armourHalfH + 1.0f;

        // Armour border
        CSprite2d::DrawRect(
            CRect(screenX - halfW - 1.5f, armourCenterY - armourHalfH - 1.5f,
                  screenX + halfW + 1.5f, armourCenterY + armourHalfH + 1.5f),
            CRGBA(0, 0, 0, alphaBrd));

        // Armour background
        CSprite2d::DrawRect(
            CRect(screenX - halfW, armourCenterY - armourHalfH,
                  screenX + halfW, armourCenterY + armourHalfH),
            CRGBA(30, 40, 50, alphaBg));

        // Armour fill (max armour in GTA SA is 100.0f)
        const float armourPct = std::clamp(ped->m_fArmour / 100.0f, 0.0f, 1.0f);
        if (armourPct > 0.0f) {
            CSprite2d::DrawRect(
                CRect(screenX - halfW, armourCenterY - armourHalfH,
                      screenX - halfW + (barWidth * armourPct), armourCenterY + armourHalfH),
                CRGBA(210, 225, 245, alphaMain));
        }
    }

    // Health bar border
    CSprite2d::DrawRect(
        CRect(screenX - halfW - 1.5f, healthCenterY - halfH - 1.5f,
              screenX + halfW + 1.5f, healthCenterY + halfH + 1.5f),
        CRGBA(0, 0, 0, alphaBrd));

    // Health bar background
    CSprite2d::DrawRect(
        CRect(screenX - halfW, healthCenterY - halfH,
              screenX + halfW, healthCenterY + halfH),
        CRGBA(50, 20, 20, alphaBg));

    // Health bar fill
    const float maxHp = std::max(1.0f, ped->m_fMaxHealth);
    const float healthPct = std::clamp(ped->m_fHealth / maxHp, 0.0f, 1.0f);

    if (healthPct > 0.0f) {
        unsigned char r = 0, g = 0, b = 30;
        if (healthPct > 0.5f) {
            // High health: Green to Yellow
            const float t = (healthPct - 0.5f) * 2.0f;
            r = (unsigned char)((1.0f - t) * 255.0f);
            g = 220;
        } else {
            // Low health: Yellow to Red
            const float t = healthPct * 2.0f;
            r = 230;
            g = (unsigned char)(t * 220.0f);
        }

        CSprite2d::DrawRect(
            CRect(screenX - halfW, healthCenterY - halfH,
                  screenX - halfW + (barWidth * healthPct), healthCenterY + halfH),
            CRGBA(r, g, b, alphaMain));
    }
}

static void OnDrawHud() {
    if (CTimer::IsPaused())
        return;

    if (!CPools::ms_pPedPool || !CPools::ms_pPedPool->m_pObjects)
        return;

    CPlayerPed *player = FindPlayerPed(-1);
    if (!player)
        return;

    const int poolSize = std::min(CPools::ms_pPedPool->m_nSize, MAX_TRACKED_PEDS);
    const uint32_t now = CTimer::GetTimeMS();
    const CVector camPos = TheCamera.GetPosition();
    CVehicle *playerVehicle = player->m_pVehicle;

    for (int i = 0; i < poolSize; ++i) {
        if (CPools::ms_pPedPool->IsFreeSlotAtIndex(i)) {
            s_PedTrack[i] = {};
            continue;
        }

        CPed *ped = CPools::ms_pPedPool->GetAt(i);
        if (!ped || ped == player || ped->IsPlayer())
            continue;

        const int currentRef = CPools::ms_pPedPool->GetRef(ped);
        PedTrackInfo &track = s_PedTrack[i];

        // Slot recycled for a different ped: reset tracking data
        if (track.pedRef != currentRef) {
            track.pedRef = currentRef;
            track.showUntil = 0;
            track.lastHealth = ped->m_fHealth;
            track.lastArmour = ped->m_fArmour;
        }

        // Attacker check: Did player (on foot or inside vehicle) hit this ped?
        CEntity *attacker = ped->m_pLastEntityDamage;
        const bool attackerIsPlayer = (attacker == player) || (playerVehicle && attacker == (CEntity *)playerVehicle);

        // Check damage drop
        const bool healthDropped = (ped->m_fHealth < track.lastHealth - 0.01f);
        const bool armourDropped = (ped->m_fArmour < track.lastArmour - 0.01f);

        if (attackerIsPlayer && (healthDropped || armourDropped)) {
            if (ped->IsAlive() && ped->m_fHealth > 0.0f) {
                track.showUntil = now + HIT_DISPLAY_DURATION_MS;
            } else {
                track.showUntil = now + DEATH_FADE_DURATION_MS;
            }
        } else if (attackerIsPlayer && ped->m_nLastDamagedTime > 0 &&
                   now >= ped->m_nLastDamagedTime &&
                   (now - ped->m_nLastDamagedTime <= HIT_DISPLAY_DURATION_MS)) {
            if (track.showUntil == 0) {
                if (ped->IsAlive() && ped->m_fHealth > 0.0f) {
                    track.showUntil = ped->m_nLastDamagedTime + HIT_DISPLAY_DURATION_MS;
                } else {
                    track.showUntil = now + DEATH_FADE_DURATION_MS;
                }
            }
        }

        // If ped died while health bar is still active, shorten timer to 2 seconds
        if ((!ped->IsAlive() || ped->m_fHealth <= 0.0f) && track.showUntil > now + DEATH_FADE_DURATION_MS) {
            track.showUntil = now + DEATH_FADE_DURATION_MS;
        }

        track.lastHealth = ped->m_fHealth;
        track.lastArmour = ped->m_fArmour;

        // Skip if health bar is not currently active
        if (now >= track.showUntil)
            continue;

        // Distance check
        const CVector pedPos = ped->GetPosition();
        const float distSqr = (pedPos - camPos).MagnitudeSqr();
        if (distSqr > MAX_DRAW_DIST_SQR)
            continue;

        // Calculate 3D position above ped head
        CVector headPos;
        bool gotBone = false;
        if (ped->m_pRwClump) {
            RwV3d bonePos{};
            ped->GetBonePosition(bonePos, 8, false); // BONE_HEAD = 8
            if (bonePos.z != 0.0f && fabsf(bonePos.z - pedPos.z) < 3.0f) {
                headPos = CVector(bonePos.x, bonePos.y, bonePos.z + 0.35f);
                gotBone = true;
            }
        }
        if (!gotBone) {
            headPos = pedPos;
            headPos.z += (ped->m_pVehicle ? 0.85f : 1.15f);
        }

        // Line-of-sight check: Don't show through buildings, walls or objects
        if (!CWorld::GetIsLineOfSightClear(camPos, headPos, true, false, false, true, true, false, true))
            continue;

        // 3D world to 2D screen projection
        RwV3d screenPos{};
        float scaleX = 1.0f, scaleY = 1.0f;
        const RwV3d worldHead{headPos.x, headPos.y, headPos.z};
        if (!CSprite::CalcScreenCoors(worldHead, &screenPos, &scaleX, &scaleY, true, true))
            continue;

        // Frustum boundary check
        if (screenPos.x < 0.0f || screenPos.x > (float)RsGlobal.maximumWidth ||
            screenPos.y < 0.0f || screenPos.y > (float)RsGlobal.maximumHeight)
            continue;

        // Smooth fade out in the last 1.0 second
        const float remainingSec = (float)(track.showUntil - now) / 1000.0f;
        float alphaFactor = 1.0f;
        if (remainingSec < 1.0f) {
            alphaFactor = std::max(0.0f, remainingSec);
        }

        DrawBars(ped, screenPos.x, screenPos.y, sqrtf(distSqr), alphaFactor);
    }
}

void Init() {
    ResetAllTracking();
    Events::drawHudEvent += OnDrawHud;
    Events::restartGameEvent += ResetAllTracking;
}

} // namespace HealthBar
