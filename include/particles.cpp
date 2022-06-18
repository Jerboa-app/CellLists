#include <particles.h>

/*
  X is an array of position-orientation states (x,y,theta) for N particles.

  We divide the simulation area (L by L) into Nc by Nc boxes. Each box we compute
  collisions for instead of looping over the whole space for each particle.

  To that end, each particle has an index 0,1,2,...,N < 2^64-1 which we can
  identify them by. uint64_t * cells is a list of Nc*Nc cells where each cell
  has a "lead" particle in it, uint64_t * list is a linked-list where each entry
  is either a new position in the list (particle index) or the
  NULL_INDEX (2^64-1).

  The lead particle begins the linked list in each cell.

  To handle collisions we loop over particles in cells like so

      while (p1 != NULL_INDEX){
        p2 = cells[a2*Nc+b2];                                 // flat index
        while(p2 != NULL_INDEX){
            nCollisions += handleCollision(X,p1,p2,r,k,dt);   // check this potential collision
            p2 = list[p2];                                    // p2 points to a new particle in the same box (or null)
        }
        p1 = list[p1];                                        // p1 points to a new particle in the same box (or null)
      }

  by first checking if a pair of particles are in the same of a neighbouring
  box, this dramatically reduces the number of cases to check (O(n^2) -> O(n))
*/


// kill the linked lists
void resetLists(uint64_t * cells, uint64_t * list, int N, int Nc){
  for (int i = 0; i < Nc*Nc; i++){
    cells[i] = NULL_INDEX;
  }
  for (int i = 0; i < N; i++){
    list[i] = NULL_INDEX;
  }
}

// insert a particle into the given position if null or move down the list
void insert(uint64_t next, uint64_t particle, uint64_t * list){
  uint64_t i = next;
  while (list[i] != NULL_INDEX){
    i = list[i];                                                                  // someone here!
  }
  list[i] = particle;                                                             // it's free realestate
}

void populateLists(
  float * X,
  uint64_t * cells,
  uint64_t * list,
  int N,
  int Nc,
  float delta
){
  for (int i = 0; i < N; i++){
    uint64_t c = int(floor(X[i*3]/delta))*Nc + int(floor(X[i*3+1]/delta));        // flat index for the particle's cell
    if (cells[c] == NULL_INDEX){
      cells[c] = uint64_t(i);                                                     // we are the head!
    }
    else{
      insert(cells[c],uint64_t(i),list);
    }
  }
}

// "harmonic" contact force gives a repulsion to stop overlaps, not quite
//  momentum conserving... but it's cheapish
bool handleCollision(
  float * X,
  float * F,
  uint64_t i,
  uint64_t j,
  float r,
  float k
){
  if (i == j){
    return false;
  }
  float rx, ry, dd, d, mag, fx, fy;
  rx = X[j*3]-X[i*3];
  ry = X[j*3+1]-X[i*3+1];
  dd = rx*rx+ry*ry;
  if (dd < r*r){
    d = std::sqrt(dd);
    mag = k*(r-d)/d;
    fx = mag*rx;
    fy = mag*ry;

    F[i*2] -= fx;
    F[i*2+1] -= fy;
    F[j*2] += fx;
    F[j*2+1] += fy;
    return true;
  }
  return false;
}

// find all pairs of particles in the same (or neighbouring cells) then check if
// they have collided in full detail
uint64_t cellCollisions(
  uint64_t * cells,
  uint64_t * list,
  float * X,
  float * F,
  int Nc,
  uint64_t a1,
  uint64_t b1,
  uint64_t a2,
  uint64_t b2,
  float r,
  float k
){
  if (a1 < 0 || a1 >= Nc || b1 < 0 || b1 >= Nc || a2 < 0 || a2 >= Nc || b2 < 0 || b2 >= Nc){
    return uint64_t(0);                                                          // not a cell
  }
  uint64_t p1 = cells[a1*Nc+b1];
  uint64_t p2 = cells[a2*Nc+b2];

  uint64_t nCollisions = 0;

  if (p1 == NULL_INDEX || p2 == NULL_INDEX){
    return uint64_t(0);                                                          // nobody here!
  }

  while (p1 != NULL_INDEX){
    p2 = cells[a2*Nc+b2];                                                        // flat index
    while(p2 != NULL_INDEX){
        nCollisions += handleCollision(X,F,p1,p2,r,k);                          // check this potential collision
        p2 = list[p2];                                                           // p2 points to a new particle in the same box (or null)
    }
    p1 = list[p1];                                                               // p1 points to a new particle in the same box (or null)
  }
  return nCollisions;
}

/*
  Handle all collisions using the cell linked list and apply
all forces (including stochastic ones) using a Niels-Oded 2nd order
integrator (see ArXiv and published version below).

This is simply a Verlet style integrator that handles the Stochastic
terms properly.

We will use Eq 24 from their paper (velocity free)

https://arxiv.org/abs/1212.1244

@article{NielsOded,
author = { Niels   Grønbech-Jensen  and  Oded   Farago },
title = {A simple and effective Verlet-type algorithm for simulating Langevin dynamics},
journal = {Molecular Physics},
volume = {111},
number = {8},
pages = {983-991},
year  = {2013},
publisher = {Taylor & Francis},
doi = {10.1080/00268976.2012.760055},

URL = {
        https://doi.org/10.1080/00268976.2012.760055

},
eprint = {
        https://doi.org/10.1080/00268976.2012.760055

}

}

*/
uint64_t step(
  float * X,
  float * Xp,
  float * F,
  float * noise,
  uint64_t * list,
  uint64_t * cells,
  float * offsets,
  float * thetas,
  std::vector<glm::vec2> attractors,
  std::vector<glm::vec2> repellors,
  std::default_random_engine generator,
  int N,
  int Nc,
  float delta,
  float L,
  float r,
  float v0,
  float Dr,
  float k,
  float drag,
  float rotDrag,
  float M,
  float J,
  float attractionStrength,
  float repellingStrength,
  float dt
){
  for (int i = 0; i < N; i++){
    for (int j = 0; j < 3; j++){

      if (j<2){F[i*2+j]=0.0;}
    }
  }
  resetLists(cells, list, N, Nc);
  populateLists(X, cells, list, N, Nc, delta);
  uint64_t nCollisions = 0;
  for (int a = 0; a < Nc; a++){
    for (int b = 0; b < Nc; b++){
      nCollisions += cellCollisions(cells,list,X,F,Nc,a,b,a,b,r,k);
      nCollisions += cellCollisions(cells,list,X,F,Nc,a,b,a-1,b-1,r,k);
      nCollisions += cellCollisions(cells,list,X,F,Nc,a,b,a-1,b+1,r,k);
      nCollisions += cellCollisions(cells,list,X,F,Nc,a,b,a+1,b+1,r,k);
      nCollisions += cellCollisions(cells,list,X,F,Nc,a,b,a+1,b-1,r,k);
      nCollisions += cellCollisions(cells,list,X,F,Nc,a,b,a-1,b,r,k);
      nCollisions += cellCollisions(cells,list,X,F,Nc,a,b,a+1,b,r,k);
      nCollisions += cellCollisions(cells,list,X,F,Nc,a,b,a,b-1,r,k);
      nCollisions += cellCollisions(cells,list,X,F,Nc,a,b,a,b+1,r,k);
    }
  }

  float D = std::sqrt(2.0*Dr/dt);
  float dtdt = dt*dt;

  float cr = (rotDrag*dt)/(2.0*J);
  float br = 1.0 / (1.0 + cr);
  float ar = (1.0-cr)*br;

  float ct = (drag*dt)/(2.0*M);
  float bt = 1.0 / (1.0 + ct);
  float at = (1.0-ct)*bt;

  for (int i = 0; i < N; i++){

  for (int j = 0; j < attractors.size(); j++){
      float rx = attractors[j].x-X[i*3];
      float ry = attractors[j].y-X[i*3+1];

      float d = sqrt(rx*rx+ry*ry);

      if (d < r){
        std::uniform_real_distribution<float> U(0.0,6.28);
        float theta = U(generator);
        F[i*2] -= attractionStrength*cos(theta)/d;
        F[i*2+1] -= attractionStrength*sin(theta)/d;
      }
      else{
        d = d*d*d;
        F[i*2] += attractionStrength*rx/d;
        F[i*2+1] += attractionStrength*ry/d;
      }
    }
    for (int j = 0; j < repellors.size(); j++){
        float rx = X[i*3]-repellors[j].x;
        float ry = X[i*3+1]-repellors[j].y;

        float dd = rx*rx+ry*ry;

        F[i*2] += repellingStrength*rx/dd;
        F[i*2+1] += repellingStrength*ry/dd;
    }


    noise[i*2+1] = noise[i*2];
    noise[i*2] = normal(generator);

    float x = X[i*3];
    float y = X[i*3+1];
    float theta = X[i*3+2];

    float xp = Xp[i*3];
    float yp = Xp[i*3+1];
    float thetap = Xp[i*3+2];

    float ax = drag*v0*cos(theta)+F[i*2];
    float ay = drag*v0*sin(theta)+F[i*2+1];

    X[i*3] = 2.0*bt*x - at*xp + (bt*dtdt/M)*ax;
    X[i*3+1] = 2.0*bt*y - at*yp + (bt*dtdt/M)*ay;
    X[i*3+2] = 2.0*br*theta - ar*thetap + (br*dt/(2.0*J))*(noise[i*2]+noise[i*2+1])*dt*rotDrag*D;

    Xp[i*3] = x;
    Xp[i*3+1] = y;
    Xp[i*3+2] = theta;

    float vx = X[i*3]-Xp[i*3];
    float vy = X[i*3+1]-Xp[i*3+1];
    float ux = 0.0; float uy = 0.0;
    float ang = X[i*3+2];
    bool flag = false;

    // kill the particles movement if it's outside the box
    if (X[i*3]-r < 0 || X[i*3]+r > L){
      ux = -vx;
      ang = std::atan2(vy,ux);
      flag = true;
    }

    if (X[i*3+1]-r < 0 || X[i*3+1]+r > L){
      uy = -vy;
      if (flag){
        ang = std::atan2(uy,ux);
      }
      else{
        ang = std::atan2(uy,vx);
        flag = true;
      }
    }

    if (flag){
      X[i*3+2] = ang;
      X[i*3+1] += uy;
      X[i*3] += ux;

      Xp[i*3+2] = ang;
      Xp[i*3+1] = X[i*3+1]-0.5*uy;
      Xp[i*3] = X[i*3]-0.5*ux;
    }

    if (X[i*3] == L){ X[i*3] -= 0.001;}
    if (X[i*3+1] == L){ X[i*3+1] -= 0.001;}
    // for openGL
    offsets[i*2] = X[i*3];
    offsets[i*2+1] = X[i*3+1];
    thetas[i] = std::fmod(X[i*3+2],2.0*M_PI);
  }

  return nCollisions;
}
