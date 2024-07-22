# Changes between fCavEX 0.3.0\_f1 and CavEX 0.3.0

## Fixes
* Fixed the game randomly crashing or corrupting the player data when spawning an entity.
* Fixed the game crashing when crafting or breaking a chest.

## Additions
* Compile-time video settings (set in `source/graphics/gfx_settings.h`).
* The video settings allow disabling/enabling clouds, changing rsolution on PC, GUI scaling and setting liquids to fancy or fast.
* Fancy liquids are transparent, slightly lower than full blocks and animated.
* Fast liquids are opaque, non-animated and full block height, which makes them a lot less demanding on the GPU. Unlike fancy liquids, surfaces behind fast liquids are culled when the player isn't underwater.
* GUI scaling makes the game playable at very low resolutions like 320x200.
* After changing video settings, the game must be recompiled to recognize them.
* The player now has up to 160 health, with one heart being 16 health points.
* In the original game, the player has up to 20 health, so loading a save from it will start the game with only one heart. This can be fixed by eating some food or editing the health value in `level.dat`. This will no longer be necessary when fCavEX gets its own world generator.
* Health can be lost by falling, drowning, eating raw food or swimming in lava.
* Health can be gained back by eating food.
* The player dies when reaching 0 health, dropping all items and instantly respawning on the world's original spawn point with half max health.
* An oxygen bar is displayed when underwater.
* The player starts drowning when the oxygen bar is fully depleted.
* The oxygen bar is instantly restored when surfacing above water.
* Iron chests, which have double the capacity of wooden chests.
* Iron chests are crafted like wooden chests, but with iron ingots instead of planks.
* Aside from the double capacity, iron chests act like wooden chests.
* Signs can now be crafted and placed.
* Signs can be viewed and edited by right-clicking them. Sign text is only visible in the editing GUI.
* While editing a sign, press up/down to edit the current character and press left/right to move the cursor.
* There can be up to 256 signs placed in a world.
* For now, signs do not require a supporting block.
* Sign text is saved in a binary file called `signs.dat` in the world's save directory. This is an incompatible save format - sign texts in saves from the original game cannot be read by fCavEX.
* Already placed signs need to be broken and placed again to keep their text.
* "2D tree" blocks obtainable only by inventory editing, meant as a WIP alternative to 3D trees which uses less vertices/quads.
* 2D trees use a vertically stretched sapling texture, can exist in 2 heights, can be sheared for leaves and chopped for logs and saplings.

## Changes
* Leaves are now rendered differently, which uses less vertices/quads, improving performance on low-end GPUs.
* Item entities are now rendered as 2D billboarded sprites, which uses less vertices/quads (3D item entities may come back in future releases).
* Item durability values are now only 8-bit, which is not enough for high durability tools and slightly breaks save compatibility, but tools in fCavEX are unbreakable.
* Workbenches, furnaces and chests now have textured GUIs.
*	Chests can store items.
* Chests save their items in a binary file called `chests.dat` in the world's save directory. This is an incompatible save format - chest contents in saves from the original game cannot be read by fCavEX.
* Already placed chests need to be broken and placed again to store items.
* There can be up to 256 chests and/or iron chests placed in a world.
* Chests no longer form double chests.
* Chests can be always placed next to other chests.
* Chests drop their contents when destroyed.
* Furnaces now require fuel.
* Furnaces are fueled by right-clicking them while holding a fuel item.
* Furnaces can have up to 15 fuel units in them.
* Smelting is instant and drains 1 fuel unit per item.
* Furnaces now have the same texture on all sides, as metadata is used for storing their fuel value.
* Furnaces can only be opened when fueled.
* Furnaces can not store items.
* Furnaces keep their fuel when broken and replaced.
* Trapdoors, wooden doors and iron doors can now be placed, opened and closed.
* Double doors do not work yet.
* Trapdoors and doors do not require a supporting block.
* Door halves can be broken separately, with only the bottom half dropping the door item.
* Opening or closing a door half also opens or closes the half placed above and the half placed below, allowing for 3 block tall doors. This only works with door halves of the same type (wooden or iron).
* Iron doors are opened and closed by hand, as there is no redstone/automation for now.
* 9 iron ingots/gold ingots/diamonds now craft only 1 block as opposed to 4.
* Apples, golden apples, bread, mushroom stew, cooked porkchops and cooked fish can now be eaten to restore health.
* Food can not be eaten when the player is at full health.
* Raw porkchops and raw fish hurt the player when eaten.
* The default `config.json` file is now meant for the PC - when building for the Wii, replace it with `config_wii.json`.

## Removals
* 3D item rendering (may come back in future releases).
* Tool durability.

