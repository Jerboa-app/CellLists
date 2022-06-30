Simple demonstration of the power of cell linked lists for particle simulations.

Watch 100,000 colliding particles in realtime!

![](https://github.com/Jerboa-app/CellLists/blob/main/resources/out.gif)

Uses C++ with SFML and OpenGL

### Controls

| Function     | Keys |
| ----------- | ----------- |
| toggle debug menu      | F1      |
| zoom   | mouse wheel        |
| pan | left-click and drag |
| add attractor mode | A |
| add repeller mode | R |
| add/delete attractor/repeller | left-click |


![](https://github.com/Jerboa-app/CellLists/blob/main/resources/s3.png)

![](https://github.com/Jerboa-app/CellLists/blob/main/resources/s1.png)

![](https://github.com/Jerboa-app/CellLists/blob/main/resources/s2.png)

### Building

#### If building SFML from source (as included)

First ensure you have the SFML dependencies installed in ubuntu this can be achieved via

```console
apt-get install build-essential mesa-common-dev libx11-dev libxrandr-dev libgl1-mesa-dev liblgu1-mesa-dev libfreetype6-dev libopenal-dev libsndfile1-dev libudev-dev
```

Additionally install the glm developement libraries

```console
apt-get install libglm-dev
```

Run 

```console
./dependencies.sh && ./build.sh
```

which will build sfml statically and run cmake to build the game

#### If not building SFML from source

Ensure the ```SFML_DIR``` environment variable is set correctly and run 

```console
./build.sh
```
