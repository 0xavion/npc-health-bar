# NPC Health Bar (GTA:SA Android)

An **Android Mod Loader (AML)** plugin for Grand Theft Auto: San Andreas (Android) that displays a health bar (and optional armour bar) above NPCs when damaged by the player

## Features

- **Damage-Triggered Display**: The health bar appears for **15 seconds** once an NPC takes damage from the player (punches, melee weapons, gunfire, or vehicle collisions)
- **Adaptive Armour Bar**: Automatically renders an armour bar above the health bar if the NPC has armour (e.g. cops, SWAT)
- **Line-of-Sight Check**: Prevents bars from rendering through walls, buildings, or obstacles (`CWorld::GetIsLineOfSightClear`)
- **Dynamic Color Transition**: Health bar changes dynamically based on remaining HP (Green on high HP -> Yellow on medium HP -> Red on low HP)
- **Smooth Visuals**: 1-second fade-out transition before hiding, and a 2-second linger upon death to confirm the kill
- **Zero GC & Memory Safe**: Uses a static, O(1) indexed tracking table tied to engine ped pool slots. Zero dynamic heap allocation (`malloc`/`new`) per frame, eliminating memory leaks and frame drops

## License & Credits

- **NPC Health Bar Plugin**: Licensed under the [MIT License](LICENSE) (Copyright © 2026 Xavion)
- **AML Plugin SDK (aml-psdk)**: Created by [RusJJ](https://github.com/AndroidModLoader/aml-psdk) (GPLv3)
- **Android Mod Loader (AML)**: Created by [RusJJ](https://github.com/AndroidModLoader/AndroidModLoader)
