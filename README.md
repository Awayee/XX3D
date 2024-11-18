# XX3D
A 3D game engine, continuously updating.
# Building Prerequisites
## Windows
- Visual Studio 2019 (or more recent)
- CMake 3.19 (or more recent)
- Git 2.1 (or more recent)
- Graphics card supports D3D12 or Vulkan 1.2+
# Build and Run
## Windows
**Generate vs solution**

- Install cmake, make sure cmake path is in your environment, or using cmake path later.
- Run `build.bat`, or use following command
  ```
  cmake -S. -B [directory name]
  ```
**Config your project directory**

Setup the `ProjectPath` in the `EngineConfig.ini`, ensure the path exists.

**Build and Run**

Open the generated .sln file with VS, there are several projects in the solution:
- `Core` is a library project, including math, containers, filesystem and other core functions.
- `Engine` is a library project with engine features.
- `Editor` is a executable project with editor UI and functions.
- `Runtime` is a executable project with game running dependencies. 

The default start project is `Editor`, just build it and run.
You can also switch to `Runtime` to lauch your game.

# Introducing to Directories
- `Assets`: Contains engine dependent resources, including fonts, textures, meshes.
- `Binaries`: Contains built executable files, libraries, and compiled shaders.
- `Shaders`: Shader source files.
- `Source`: Cpp code.
- `DemoProject`: A example project, will be removed.

# Current Features Overview
- RHI supported with D3D12 and Vulkan, configurated by `RHIType` in `ProjectConfig.ini` in your project directory.
- Component system derived from ECS: combine components to determin the feature of an actor.
- Asset manager, with importing, creating.
  ![image](https://github.com/user-attachments/assets/69995458-f29e-418b-a1b8-72792db7dff0)
  Just drag your mesh file or image file to Mesh Importer or Texture Importer, and "Import"
  ![image](https://github.com/user-attachments/assets/a9cc3bca-a523-47aa-b1a5-462fa35c30d0)
  ![image](https://github.com/user-attachments/assets/93b02fe9-0adf-4bc8-9652-35574baac2b9)
- Render Graph and visualize.
  ![image](https://github.com/user-attachments/assets/d35a59d9-4009-4927-ab19-6c1e14b9154e)
- Directional light shadow and debug.
  ![image](https://github.com/user-attachments/assets/2860950f-88df-46ed-bda6-c8be10d9739a)
- Materail system with material template and material instance.
  ![image](https://github.com/user-attachments/assets/dc4bd0ed-fe28-4324-9287-724c452c6bb8)
- GPU driven pipeline for maintaining high performance when rendering massive amount of objects.
  ![image](https://github.com/user-attachments/assets/711c5bf6-21c7-4f98-aa4f-ea66d8485af4)


# Future Supportings
- Deferred texturing
- Multi thread and task graph
- True PBR
- Ray tracing
- Script system
- Physical simulation
