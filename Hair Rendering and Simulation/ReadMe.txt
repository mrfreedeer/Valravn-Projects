  _    _          _____ _____    _____  ______ _   _ _____  ______ _____  _____ _   _  _____            _   _ _____     _____ _____ __  __ _    _ _            _______ _____ ____  _   _ 
 | |  | |   /\   |_   _|  __ \  |  __ \|  ____| \ | |  __ \|  ____|  __ \|_   _| \ | |/ ____|     /\   | \ | |  __ \   / ____|_   _|  \/  | |  | | |        /\|__   __|_   _/ __ \| \ | |
 | |__| |  /  \    | | | |__) | | |__) | |__  |  \| | |  | | |__  | |__) | | | |  \| | |  __     /  \  |  \| | |  | | | (___   | | | \  / | |  | | |       /  \  | |    | || |  | |  \| |
 |  __  | / /\ \   | | |  _  /  |  _  /|  __| | . ` | |  | |  __| |  _  /  | | | . ` | | |_ |   / /\ \ | . ` | |  | |  \___ \  | | | |\/| | |  | | |      / /\ \ | |    | || |  | | . ` |
 | |  | |/ ____ \ _| |_| | \ \  | | \ \| |____| |\  | |__| | |____| | \ \ _| |_| |\  | |__| |  / ____ \| |\  | |__| |  ____) |_| |_| |  | | |__| | |____ / ____ \| |   _| || |__| | |\  |
 |_|  |_/_/    \_\_____|_|  \_\ |_|  \_\______|_| \_|_____/|______|_|  \_\_____|_| \_|\_____| /_/    \_\_| \_|_____/  |_____/|_____|_|  |_|\____/|______/_/    \_\_|  |_____\____/|_| \_|
----------------------------------------------------------------------
The artifact presents thousands of hairs being rendered on screen in real-time. These hairs are illuminated using industry-standard lighting techniques, Kajiya-Kay and Marschner, which allow for realistic light scattering.
The hairs are also shadowed using Screen-Space Ambient Occlusion (SSAO), which adds depth and realism to the rendered image.
The thousands of hairs that are created and simulated are called guide hairs or guide strands. To achieve a fuller look, virtual hair is generated from guide hairs using the DirectX 11 tessellation engine and full pipeline.
During this process thousands more virtual hairs are rendered, which follow the same general shape and movement as the guide hair, resulting in a more voluminous appearance. 
Each guide hair, and consequently the virtual hair, is simulated in real-time using two different techniques. The first is a mass spring system that mimics the movement of individuals straight and curly hair strands using 
springs distributed along them. The second technique is Nvidia’s Dynamic Follow-The-Leader (DFTL) based on rope physics, that allows straight hair simulation with a relatively simpler algorithm. 


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
F1 - Toggle Debug Draw
Ctrl + Shift + B - Toggle Skybox
Shift + Right Click Hold + Mouse movement - Move Sphere object up and down
Arrows - Move Light Objects
---------------------------------
--------------
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

DebugRenderToggle - Toggle light visualization and stats back on