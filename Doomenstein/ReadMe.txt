  _____   ____   ____  __  __ ______ _   _  _____ _______ ______ _____ _   _ 
 |  __ \ / __ \ / __ \|  \/  |  ____| \ | |/ ____|__   __|  ____|_   _| \ | |
 | |  | | |  | | |  | | \  / | |__  |  \| | (___    | |  | |__    | | |  \| |
 | |  | | |  | | |  | | |\/| |  __| | . ` |\___ \   | |  |  __|   | | | . ` |
 | |__| | |__| | |__| | |  | | |____| |\  |____) |  | |  | |____ _| |_| |\  |
 |_____/ \____/ \____/|_|  |_|______|_| \_|_____/   |_|  |______|_____|_| \_|
----------------------------------------------------------------------
(Not so) Faithful recreation of Doom

----------------------------------------------------------------------
   _____ ____  _   _ _______ _____   ____  _       _____ 
  / ____/ __ \| \ | |__   __|  __ \ / __ \| |     / ____|
 | |   | |  | |  \| |  | |  | |__) | |  | | |    | (___  
 | |   | |  | | . ` |  | |  |  _  /| |  | | |     \___ \ 
 | |___| |__| | |\  |  | |  | | \ \| |__| | |____ ____) |
  \_____\____/|_| \_|  |_|  |_|  \_\\____/|______|_____/ 
                                                         
---------------------------------  
  _  __          _                         _ 
 | |/ /         | |                       | |
 | ' / ___ _   _| |__   ___   __ _ _ __ __| |
 |  < / _ \ | | | '_ \ / _ \ / _` | '__/ _` |
 | . \  __/ |_| | |_) | (_) | (_| | | | (_| |
 |_|\_\___|\__, |_.__/ \___/ \__,_|_|  \__,_|
            __/ |                            
           |___/                                                                                    
---------------------------------
W - Move Forward
A - Move Left
S - Move Back 
D - Move Right
Z - Move Up the Z axis
C - Move Down the Z axis
Q - Rotate Left
E - Rotate Right
Shift - Sprint Speed
H - Reset Position and Rotation
1 - Equip Weapon 1
2 - Equip Weapon 2
Left & Right Arrows - Cycle through weapons
N - Possess other entity
F - Toggle Freefly Camera
---------------------------------

 __   ___                  _____            _             _ _           
 \ \ / / |                / ____|          | |           | | |          
  \ V /| |__   _____  __ | |     ___  _ __ | |_ _ __ ___ | | | ___ _ __ 
   > < | '_ \ / _ \ \/ / | |    / _ \| '_ \| __| '__/ _ \| | |/ _ \ '__|
  / . \| |_) | (_) >  <  | |___| (_) | | | | |_| | | (_) | | |  __/ |   
 /_/ \_\_.__/ \___/_/\_\  \_____\___/|_| |_|\__|_|  \___/|_|_|\___|_|   

---------------------------------                                                                       
Left joystick - Move Around
Right joystick - Move the Camera View      
Left Bumper - Move down the Z Axis                                                               
Right Bumper - Move up the Z axis 
Left Trigger - Rotate Left
Right Trigger - Rotate Right
A Button - Sprint Speed
Start Button - Reset Position and Rotation
Dpad Left & Right - Cycle through weapons
Right Shoulder - Possess other entity
Dpad Up - Toggle Freefly Camera
---------------------------------
----------------------------------------------------------------------
  _____  ________      __   _____ ____  _   _  _____  ____  _      ______ 
 |  __ \|  ____\ \    / /  / ____/ __ \| \ | |/ ____|/ __ \| |    |  ____|
 | |  | | |__   \ \  / /  | |   | |  | |  \| | (___ | |  | | |    | |__   
 | |  | |  __|   \ \/ /   | |   | |  | | . ` |\___ \| |  | | |    |  __|  
 | |__| | |____   \  /    | |___| |__| | |\  |____) | |__| | |____| |____ 
 |_____/|______|   \/      \_____\____/|_| \_|_____/ \____/|______|______|
----------------------------------------------------------------------
Opens using the `(tilde) key. 

There are the following available commands:
Help [Filter="filter"]: Filters the available commands, showing those that contain the filter
Clear: Clears the command history and dev console displayed commands

Commands are case insensitive.       
 
----------------------------------------------------------------------
  _____                _____ _                _      _____          _           
 |  __ \              / ____| |              | |    / ____|        | |          
 | |  | | _____   __ | |    | |__   ___  __ _| |_  | |     ___   __| | ___  ___ 
 | |  | |/ _ \ \ / / | |    | '_ \ / _ \/ _` | __| | |    / _ \ / _` |/ _ \/ __|
 | |__| |  __/\ V /  | |____| | | |  __/ (_| | |_  | |___| (_) | (_| |  __/\__ \
 |_____/ \___| \_/    \_____|_| |_|\___|\__,_|\__|  \_____\___/ \__,_|\___||___/
 ----------------------------------------------------------------------
Shift + F2 - Player becomes invulnerable
Shift + F6 - Kill all enemies                                                                               
                                                                                
----------------------------------------------------------------------
  _   _  ____ _______ ______  _____ 
 | \ | |/ __ \__   __|  ____|/ ____|
 |  \| | |  | | | |  | |__  | (___  
 | . ` | |  | | | |  |  __|  \___ \ 
 | |\  | |__| | | |  | |____ ____) |
 |_| \_|\____/  |_|  |______|_____/ 
----------------------------------------------------------------------
The raycast implemented will hit only valid tiles within the map boundaries. 
Redundant Map quads are not rendered

New Gold Stuff Implemented:
• New Spider enemy with 3D model AND a couple of animations. Includes sounds
• Wandering behaviour (for the spider enemy) using heatmaps
• Vision color will get distorted depending on how much life the player has left
• Life regeneration
• New weapon, Gravity Gun: Shoot a projectile that upon impact pulls all actors around a radius, and damages actors within a smaller radius (including owner, be careful!). Includes animations and sounds.
• Doom Guy face animation on the Hud for each player
• New map with Horde mode. In this mode, there will be an amount of waves with enemies spawning to kill the player. If the players survive all waves, they win. 


                                                                                                