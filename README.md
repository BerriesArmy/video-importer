# Video Importer

A Geode mod that turns GIFs, images and folders of frames into animated pixel art in the Geometry Dash level editor. It is made by BerriesArmy.

The code was written with the help of AI. I am not hiding that, and it is the reason this mod is listed on the Vibemodded Index.

## Using it

Open a level in the editor, open the pause menu and press Video. Pick a GIF or image, or pick a folder of frames named in order. The mod reads the frames, builds the objects and triggers, and places them in your level. When it finishes, a window asks which extras you want.

Placement and size live in the mod settings. Background puts the video behind normal gameplay, Movie puts it in front. Grid width, frame rate, position, color levels and the change threshold control quality against how many triggers the level ends up with.

## How it works

The video is a grid of square blocks, each on its own color channel. Frame zero sets every block. After that, each frame only changes blocks whose color moved past the change threshold. Every frame has its own group of color triggers and one spawn trigger that fires it after a delay, so playback does not depend on how fast the player moves.

The color channel range and group range are set in the settings, so you can keep them away from ranges your level already uses.

## Not tested in game yet

The first version was written without being able to run the game. Camera lock and hiding the player use trigger properties that still need checking in game. If the video scrolls away or the player stays visible, please open an issue with the game version.

Importing cannot be undone with the editor undo button yet, so save the level first.

## Building

Push to GitHub and the Build workflow makes the .geode files for Windows, macOS and Android. iOS is attempted on every run but is allowed to fail, because the Geode bindings currently do not compile for iOS here. Pushing a tag that starts with v also publishes a release.

## Supported versions

Geometry Dash 2.2081 with Geode 5.11.2 on Windows, macOS and Android. iOS is not supported yet.
