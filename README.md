# SA.PersistentRadar

A GTA: San Andreas ASI mod that keeps **all 144 radar map tiles resident in
memory**, so the minimap and the pause menu map never show missing blue squares.

SA streams its radar map as 144 TXDs (`radar00`–`radar143`, a 12x12 grid) and
keeps only a 3x3 window around the player loaded — a PS2 memory budget that
modern hardware has no reason to respect. Fast travel, teleporting, flying, or
panning the pause map faster than the streamer can keep up all leave holes in
the map. This loads the whole grid once and stops the engine throwing tiles away.

Built with [plugin-sdk](https://github.com/DK22Pac/plugin-sdk). Targets
**GTA SA 1.0 US (HOODLUM)**.

## Install

Drop `SA.PersistentRadar.asi` into your game's `scripts` folder, or into a
[modloader](https://github.com/thelink2012/sa-modloader) subfolder. Requires an
ASI loader (Silent's / Ultimate ASI Loader). No configuration.

Works with HD radar replacements — it locks whatever tiles are installed.

## Memory cost

The full grid is held resident for the whole session:

| Radar | Per tile | All 144 tiles |
| --- | --- | --- |
| Vanilla (128x128 DXT1) | 8 KB | ~1.1 MB |
| 2x HD (256x256 DXT1) | 32 KB | ~4.5 MB |
| 4x HD (512x512 DXT1) | 128 KB | ~18 MB |

DXT1 stays compressed in memory, so this is also roughly the VRAM cost. It comes
out of the streaming budget, so a high-resolution radar pairs well with a
streaming memory fix.

## How it works

The radar tiles are TXD models in the streaming system (`gRadarTxdIds[144]` at
`0xBA8478`, stream id = TXD slot + 20000). The engine already requests them with
`GAME_REQUIRED | KEEP_IN_MEMORY`, which protects them from the streamer's normal
eviction — so the pop-in is not a flags problem. It comes from two functions that
remove tiles explicitly:

- `CRadar::StreamRadarSections` (`0x584C50`), called every frame from
  `CRadar::DrawRadarMap`, which requests a 3x3 window and `RemoveModel`s the
  other 135 tiles.
- `CRadar::RemoveMapSection` (`0x584BB0`), whose only caller is
  `CMenuManager::ProcessStreaming` (`0x573CF0`) — it purges every tile outside
  the current pause map view rect, which is where map panning loses tiles.

Both are patched to return immediately. A tick then requests all 144 tiles and
force-loads them once, and re-requests anything that goes missing.

`CRadar::RemoveRadarSections` (`0x584BF0`) is deliberately left alone: it is the
cutscene memory purge and `CGame::ShutDownForRestart`. The tick restores the
tiles afterwards rather than blocking a purge the engine genuinely wants.

Nothing is hooked in the drawing path. `CRadar::DrawRadarSection` and
`DrawRadarSectionMap` never request anything — they silently skip a tile whose
TXD is not resident, which is the blue square. Residency alone is the whole fix.

## Build

Requires [plugin-sdk](https://github.com/DK22Pac/plugin-sdk) with the
`PLUGIN_SDK_DIR` environment variable pointing at it, and its `Plugin.lib` built
for Win32 Release.

```
MSBuild SA.PersistentRadar.vcxproj /p:Configuration="Release GTA-SA" /p:Platform=Win32
```

## License

MIT
