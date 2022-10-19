# Jackal Control

## Building

### Linux
Clone emscripten 

```bash
git submodule init && git submodule update
```

Visit Urho [docs](https://urho3d.io/documentation/HEAD/_building.html) and make sure you have dependencies installed - should get most required with build-essential.

Build Debug and Release versions of Urho3D:

```bash
mkdir build && cd build
mkdir Debug && cd Debug 
cmake -DCMAKE_BUILD_TYPE=Debug -DURHO3D_PACKAGING=1 -DVIDEO_WAYLAND=0 ../..
make -j
cd .. && mkdir Release && cd Release
cmake -DCMAKE_BUILD_TYPE=Release -DURHO3D_PACKAGING=1 -DVIDEO_WAYLAND=0 ../..
make -j
```

Urho Tools, Editor, and Samples should be built - the binaries are under deps/Urho3d/build/CMAKE_BUILD_TYPE/bin and tools folders.

Nothing else is required to build Build and Battle. Just set CMAKE_BUILD_TYPE to Debug or Release, and build with cmake.

## More Info

A list of versions and changes can be found [here](Changelog.md).

Coding standards can be viewed [here](CodingStandards.md).