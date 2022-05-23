# Geometry Dash Frame Dividing

An inverse physics bypass. If rendering is a slowdown for GD, this may allow you the benefits of increased physics ticks with less lag. By being an inverse physics bypass, you should be able to click in between drawing frames, which would not be possible with traditional physics bypasses.

## Usage

As of right now, this mod can only be configured with Mega Hack v6+.  
By default, the hack is enabled and will draw the game at 1/4th your chosen FPS.

The FPS of your game (chosen through bypass, with VSync, or as 60 FPS) is the rate at which update functions are called (such as physics and input). The factor (4 by default, can be configured in Mega Hack) determines the rate at which frames are rendered at.  
For example, at 240 FPS bypass with a factor of 4, the game will appear to be drawing at 60 FPS. However, physics will be running as if you are playing at 240 FPS. This can be verified using a physics-based FPS detector (such as the one at in-game ID 66886242) and a drawing-based FPS detector (such as the one built into MegaHack).

## Credits

* blanket addict

