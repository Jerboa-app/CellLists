#include <mpiParticles.h>

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

bool handleCollision(
  float * X,
  float * F,
  uint64_t i,
  uint64_t j,
  float r,
  float k,
  float dt
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
    mag = dt*k*(r-d)/d;
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
  float k,
  float dt
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
        nCollisions += handleCollision(X,F,p1,p2,r,k,dt);                          // check this potential collision
        p2 = list[p2];                                                           // p2 points to a new particle in the same box (or null)
    }
    p1 = list[p1];                                                               // p1 points to a new particle in the same box (or null)
  }
  return nCollisions;
}
