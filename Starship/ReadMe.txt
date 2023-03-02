 ________  _________  ________  ________  ________  ___  ___  ___  ________                              ___  ________  ________  ________  ___  ________   ________     
|\   ____\|\___   ___|\   __  \|\   __  \|\   ____\|\  \|\  \|\  \|\   __  \                            |\  \|\   __  \|\   ____\|\   __  \|\  \|\   ___  \|\   __  \    
\ \  \___|\|___ \  \_\ \  \|\  \ \  \|\  \ \  \___|\ \  \\\  \ \  \ \  \|\  \      ____________         \ \  \ \  \|\  \ \  \___|\ \  \|\  \ \  \ \  \\ \  \ \  \|\  \   
 \ \_____  \   \ \  \ \ \   __  \ \   _  _\ \_____  \ \   __  \ \  \ \   ____\    |\____________\     __ \ \  \ \  \\\  \ \_____  \ \   ____\ \  \ \  \\ \  \ \   __  \  
  \|____|\  \   \ \  \ \ \  \ \  \ \  \\  \\|____|\  \ \  \ \  \ \  \ \  \___|    \|____________|    |\  \\_\  \ \  \\\  \|____|\  \ \  \___|\ \  \ \  \\ \  \ \  \ \  \ 
    ____\_\  \   \ \__\ \ \__\ \__\ \__\\ _\ ____\_\  \ \__\ \__\ \__\ \__\                          \ \________\ \_______\____\_\  \ \__\    \ \__\ \__\\ \__\ \__\ \__\
   |\_________\   \|__|  \|__|\|__|\|__|\|__|\_________\|__|\|__|\|__|\|__|                           \|________|\|_______|\_________\|__|     \|__|\|__| \|__|\|__|\|__|
   \|_________|                             \|_________|                                                                  \|_________|                                   
                                                                                                                                                                         
                                                                                                                                                         
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

   _____ _____  __             _  _   
  / ____|  __ \/_ |        /\ | || |  
 | (___ | |  | || |______ /  \| || |_ 
  \___ \| |  | || |______/ /\ \__   _|
  ____) | |__| || |     / ____ \ | |  
 |_____/|_____/ |_|    /_/    \_\|_|  
                                      
                                     
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------                                                 
   ____            _             _     
  / ___|___  _ __ | |_ _ __ ___ | |___ 
 | |   / _ \| '_ \| __| '__/ _ \| / __|
 | |__| (_) | | | | |_| | | (_) | \__ \
  \____\___/|_| |_|\__|_|  \___/|_|___/
                                       

S - Rotate to the left at a constant speed
F - Rotate to the right at a constant speed
E - Propel forwards towards where the ship is pointing forwards
space - Shoots a bullet towards where the ship is pointing toward (only 100 bullets at a time or an error window will show up!)
F1 - Enable debugger view where everything currently visible on the screen will be drawn within a physics circle (cyan circle), cosmetic circle (magenta circle).
     The relative forwards direction of each object is drawn in red, relative left in green. The object's velocity is drawn as a yellow line. 
F6 - Kill all the enemies
F8 - Hard-reset the game, recreating the game from scratch. 
T - Slows down game time while being held down
P - Pauses the game
O - Renders a single game frame every time it's pressed, then pauses the game
I - Spawns an asteroids randomly within the screen (only 12 at a time or an error window will show up!)
N - Respawn (You have 4 lives total!)

 __   ___                
 \ \ / / |               
  \ V /| |__   _____  __ 
   > < | '_ \ / _ \ \/ / 
  / . \| |_) | (_) >  <  
 /_/ \_\_.__/ \___/_/\_\ 
                         
                         
Left Joystick - Use the left joystick to make the ship thrust in the direction you point it to
Left Joystick Button - Enable debugger view where everything currently visible on the screen will be drawn within a physics circle (cyan circle), cosmetic circle (magenta circle).
     The relative forwards direction of each object is drawn in red, relative left in green. The object's velocity is drawn as a yellow line. 
Left Shoulder - Spawns an asteroids randomly within the screen (only 12 at a time or an error window will show up!)
Right Joystick - Hard-reset the game, recreating it from scratch
Right Shoulder - Slows down game time while being held down
A Button - Shoot a Bullet where the ship is pointing toward
Up arrow - Renders a single game frame every time it's pressed, then pauses the game
Down arrow - Kills all the enemies
Start Button - Respawn (You have 4 lives total!)
Back Button - Pause Game

----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  _  __                            ____                  
 | |/ /_ __   _____      ___ __   | __ ) _   _  __ _ ___ 
 | ' /| '_ \ / _ \ \ /\ / | '_ \  |  _ \| | | |/ _` / __|
 | . \| | | | (_) \ V  V /| | | | | |_) | |_| | (_| \__ \
 |_|\_|_| |_|\___/ \_/\_/ |_| |_| |____/ \__,_|\__, |___/
                                               |___/     
• When going in slow motion and spawning bullets or asteroids until the error message shows up, if the T key is released and then the game is allowed to carry on, slow mo persists.


----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  _____                    _                           _             
 |  __ \                  | |                         (_)            
 | |  | | ___  ___ _ __   | |     ___  __ _ _ __ _ __  _ _ __   __ _ 
 | |  | |/ _ \/ _ \ '_ \  | |    / _ \/ _` | '__| '_ \| | '_ \ / _` |
 | |__| |  __/  __/ |_) | | |___|  __/ (_| | |  | | | | | | | | (_| |
 |_____/ \___|\___| .__/  |______\___|\__,_|_|  |_| |_|_|_| |_|\__, |
                  | |                                           __/ |
                  |_|                                          |___/
Throughout this project, my approach on about almost every task involved firstly writing down an implementation of what I wanted the code to do on first try.
I sometimes hit the target, most often I miss, if even by just some inches. I believe this has to do with the fact that although I do put thought into my code,
I have embraced an approach in the past of making the code work as intended, regardless of how readable it is, as fast as I can.
Afterwards, if I cared enough about the code, I would refactor it until it made me proud. This habit chased me even to professional standards. 
This is without a doubt a terrible approach, as I have often looked back to projects without understanding a single thing about them, if I did not refactor them,
effectively making them impossible to maintain as there was no one left in this world who understood that messy mix of word and numbers.
I found myself doing that in this project as well, though not as often when not under time pressure, and I did refactor most of it, 
but I find it that due to the short burst nature of this assignments, and yet the maybe not so surprising depth in them, I need to put as much thought as possible into my process, 
so that I can be a better programmer for my future Me’s sake, and later my professional future Me’s sake. 

                                                                                                                                                                                                                                                                                                                 
