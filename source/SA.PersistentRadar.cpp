#include "plugin.h"
#include "common.h"
#include "CRadar.h"
#include "CStreaming.h"
#include "CStreamingInfo.h"
#include "CCutsceneMgr.h"
#include <windows.h>

using namespace plugin;

static const int RADAR_TILE_COUNT = 144;
static const int RESOURCE_ID_TXD  = 20000;
static const int TILE_FLAGS       = GAME_REQUIRED | KEEP_IN_MEMORY;
static const unsigned int CHECK_INTERVAL_MS = 1000;

static const uintptr_t ADDR_STREAM_RADAR_SECTIONS = 0x584C50;
static const uintptr_t ADDR_REMOVE_MAP_SECTION    = 0x584BB0;

class PersistentRadar {
public:
    static bool patched;
    static unsigned int lastCheck;

    static int TileStreamId(int i) {
        int slot = gRadarTxdIds[i];
        return slot < 0 ? -1 : slot + RESOURCE_ID_TXD;
    }

    static bool TilesRegistered() {
        return gRadarTxdIds[0] >= 0;
    }

    static int CountMissing() {
        int missing = 0;
        for (int i = 0; i < RADAR_TILE_COUNT; i++) {
            int id = TileStreamId(i);
            if (id < 0)
                continue;
            if (CStreaming::ms_aInfoForModel[id].m_nLoadState != LOADSTATE_LOADED)
                missing++;
        }
        return missing;
    }

    static void RequestAll() {
        for (int i = 0; i < RADAR_TILE_COUNT; i++) {
            int id = TileStreamId(i);
            if (id >= 0)
                CStreaming::RequestModel(id, TILE_FLAGS);
        }
    }

    static void Process() {
        if (!patched || !TilesRegistered())
            return;

        unsigned int now = GetTickCount();
        if (now - lastCheck < CHECK_INTERVAL_MS)
            return;
        lastCheck = now;

        if (CountMissing() == 0)
            return;

        RequestAll();

        if (!CCutsceneMgr::ms_running)
            CStreaming::LoadAllRequestedModels(false);
    }
};

bool PersistentRadar::patched = false;
unsigned int PersistentRadar::lastCheck = 0;

class PersistentRadarPlugin {
public:
    PersistentRadarPlugin() {
        if (*(unsigned char *)ADDR_STREAM_RADAR_SECTIONS != 0x51 ||
            *(unsigned char *)ADDR_REMOVE_MAP_SECTION != 0x8B)
            return;

        patch::PutRetn(ADDR_STREAM_RADAR_SECTIONS);
        patch::PutRetn(ADDR_REMOVE_MAP_SECTION);
        PersistentRadar::patched = true;

        Events::gameProcessEvent += [] { PersistentRadar::Process(); };
    }
} persistentRadarPlugin;
