  __  __       _______ _    _  __      _______  _____ _    _         _        _______ ______  _____ _______ _____ 
 |  \/  |   /\|__   __| |  | | \ \    / /_   _|/ ____| |  | |  /\   | |      |__   __|  ____|/ ____|__   __/ ____|
 | \  / |  /  \  | |  | |__| |  \ \  / /  | | | (___ | |  | | /  \  | |         | |  | |__  | (___    | | | (___  
 | |\/| | / /\ \ | |  |  __  |   \ \/ /   | |  \___ \| |  | |/ /\ \ | |         | |  |  __|  \___ \   | |  \___ \ 
 | |  | |/ ____ \| |  | |  | |    \  /   _| |_ ____) | |__| / ____ \| |____     | |  | |____ ____) |  | |  ____) |
 |_|  |_/_/    \_\_|  |_|  |_|     \/   |_____|_____/ \____/_/    \_\______|    |_|  |______|_____/   |_| |_____/ 
                                                                                                                  
                                                                                                                  
--------------------------------------------------------------------------------------------------------------------

Project made for visually testing math assignments. Instructions display on screen for each mode:

Game Modes:
• Nearest Point on Shapes2D
• Raycast Vs Discs 2D
• Billiards
• Raycast Vs Boxes
• Shapes 3D
• Pachinko Machine 2D
• Raycast Vs OBB2
• Raycast Vs Line Segments 2D
• Pachinko Machine
• Raycast Vs ConvexPoly2D



Shapes (2D):
• AABB2
• OBB2
• Capsule
• Disc
• Line Segment
• Infinite Line
• ConvexPoly2D

Shapes (3D):
• AABB3
• Cylinder
• Sphere

Controls for Raycast Vs ConvexPoly2D

W/R - Rotate
N/M - Double/Halve rays
,/. - Double/Halve amount of Shapes
U/i - Increase/Decrease max BVH depth
F1 - Toggle debug draw
F2 - Toggle BVH


Raycasts/MS (BVH OFF) Debug:
128 Objects → ~ 16
1024 Objects → ~ 2
16384 Objects ~ 0.1


Raycasts/MS (BVH ON) Debug: 
128 Objects → ~ 22
1024 Objects → ~ 2
16384 Objects ~ 0  (Constructing the BVH takes too long)
----------------------------------------------------------------
Raycasts/MS (BVH OFF)  DebugInline:
128 Objects → ~ 17
1024 Objects → ~ 2
16384 Objects ~ 0.1


Raycasts/MS (BVH ON) Debug Inline:
128 Objects → ~ 24
1024 Objects → ~ 3
16384 Objects ~ 0  (Constructing the BVH takes too long)
----------------------------------------------------------------
Raycasts/MS (BVH OFF) FastBreak:
128 Objects → ~ 40
1024 Objects → ~ 4
16384 Objects ~ 0.2


Raycasts/MS (BVH ON) FastBreak:
128 Objects → ~ 60
1024 Objects → ~ 7
16384 Objects ~ 0 (Constructing the BVH takes too long)
----------------------------------------------------------------
Raycasts/MS (BVH OFF) Release:
128 Objects → ~ 200
1024 Objects → ~ 23
16384 Objects ~ 1


Raycasts/MS (BVH ON) Release:
128 Objects → ~ 260
1024 Objects → ~ 30
16384 Objects ~ 0 (Constructing the BVH takes too long)
----------------------------------------------------------------

Using partinioning schemes seems to improve the metrics by about 30% (compareds using the raycast/ms metric)
level, it balances out for a slightly faster performance

The speed seems to be O(n) related to both the number of rays, and Objects
The performance gained for the partinioning scheme are quickly lost when going even slightly deeper into the tree
The metrics get better when increasing the depth up to ~10, however, this costs much more as the queue used for traversing the tree takes the rest of the gained time
On differrent configurations, the speed gets higher as the configuration is more optimized
----------------------------------------------------------------

LOADING AND SAVING SCENES

Scenes can be saved to a .ghcs file, which will be in Data/Scenes/ folder
SaveScene filename="nameofthefile" 

To load a scene, it must also be in Data/Scenes/ folder, and can be loaded like this
LoadScene filenane="nameofthefile"