- Create a new `Empty Project`
- Copy over the `ArenaPlayerInterface.hpp`
- Create an implementation `.cpp` file...
  - For example, `PlayerImpl.cpp`
- Implement the Interface

- Go to Project Properties
  - Change the project to a `DLL`
  - Change the project to use C++2017

- Compile an x64 .dll
- Copy this dll to the Player's folder of the Arena
- Play (should see your AI show up)



## Auto Play
- Automatically copy the DLL to the player's directory
  - Add a post-build event to copy the dll, for example;
    - `xcopy /Y /F /I "$(OutDir)$(TargetFileName)" "$(SolutionDir)..\..\Arena\Run_Windows\Players"`

- Set Debugging Command/Target Path to run the Arena executable
- Set the working directory to that executable's directory.

Play should now run the arena and break points should work; 