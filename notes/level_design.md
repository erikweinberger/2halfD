# Level Files
Level files are ways to represent levels in a file format for saving and loading levels.


## Justification
Will be needed for testing and ability to save levels etc


## Representations

### Textures
texture (0) texture_id file_path

### Walls
wall (1) start.x start.y end.x end.y height texture_id

### SpriteEntity
sprite (2) pos.x pos.y radius height texture_id scale


## File structure
```
-------------------------
Put all textures here (will throw if item gets declared af)
-------------------------
Define rest of stuff below
```

example:
```
0 1 assets/textures/pattern_16.png
0 2 assets/textures/enemy_1.png


1 300 256 1000 256 256 1
1 0 512 300 256 1

2 384 512 32 128 2 1
```