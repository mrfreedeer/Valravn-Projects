   _____ _    _          _____   ______          __  __  __          _____   _____ 
  / ____| |  | |   /\   |  __ \ / __ \ \        / / |  \/  |   /\   |  __ \ / ____|
 | (___ | |__| |  /  \  | |  | | |  | \ \  /\  / /  | \  / |  /  \  | |__) | (___  
  \___ \|  __  | / /\ \ | |  | | |  | |\ \/  \/ /   | |\/| | / /\ \ |  ___/ \___ \ 
  ____) | |  | |/ ____ \| |__| | |__| | \  /\  /    | |  | |/ ____ \| |     ____) |
 |_____/|_|  |_/_/    \_\_____/ \____/   \/  \/     |_|  |_/_/    \_\_|    |_____/ 
                                                                                   
----------------------------------------------------------------------
Project where shadow maps are implemented
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

Numpad 1,2,3,4,5 - For enabling/disabling effects. (G,H,K,L if there's no numpad available)
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

You can save and load scenes, where the ConvexPoly3Ds, camera and light positions will be stored, by using the following commands:

SaveScene filename="nameoffile"
LoadScene filename="nameoffile"