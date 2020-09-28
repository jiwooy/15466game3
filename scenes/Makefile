all : \
    ../dist/maze.pnct \
    ../dist/maze.scene \

../dist/maze.scene : maze.blend export-scene.py
    "C:\Program Files\Blender Foundation\Blender 2.90\blender.exe" --background --python export-scene.py -- "maze.blend:Collection" "../dist/maze.scene"

../dist/maze.pnct : maze.blend export-meshes.py
    "C:\Program Files\Blender Foundation\Blender 2.90\blender.exe" --background --python export-meshes.py -- "maze.blend:Collection" "../dist/maze.pnct" 