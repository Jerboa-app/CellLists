Simple demonstration of the power of cell linked lists for particle simulations one single thread CPUs.

Watch 100,000 colliding particles in realtime, and play with them by adding attractors and repulsors!

Uses C++ with SFML and OpenGL

![](https://github.com/Jerboa-app/CellLists/blob/main/resources/out.gif)

The physics is computed using a cell linked list method (partition space to check collisions), with a Niels-Oded
integrator (essentially Stochastic Verlet), which you can read about here (Eq 24) https://arxiv.org/abs/1212.1244, or in
print if you have access https://doi.org/10.1080/00268976.2012.760055

### Controls

| Function     | Keys |
| ----------- | ----------- |
| (toggle) Debug menu      | F1      |
| Zoom   | mouse wheel        |
| Pan | left-click and drag |
| (toggle) Place attractor (max 8)| A | 
| (toggle) Place repulsor (max 8)| R 
| Pause particles | Space 
| Quit | Esc 

![](https://github.com/Jerboa-app/CellLists/blob/main/resources/s3.png)

![](https://github.com/Jerboa-app/CellLists/blob/main/resources/s1.png)

![](https://github.com/Jerboa-app/CellLists/blob/main/resources/s2.png)
