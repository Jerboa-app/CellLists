#include <particles.h>

void resetLists(uint64_t * cells, uint64_t * list, int N, int Nc){
  for (int i = 0; i < Nc*Nc; i++){
    cells[i] = NULL_INDEX;
  }
  for (int i = 0; i < N; i++){
    list[i] = NULL_INDEX;
  }
}

void insert(uint64_t next, uint64_t particle, uint64_t * list){
  uint64_t i = next;
  while (list[i] != NULL_INDEX){
    i = list[i];
  }
  list[i] = particle;
}

void populateLists(float * X, uint64_t * cells, uint64_t * list, int N, int Nc, float delta){
  for (int i = 0; i < N; i++){
    uint64_t c = int(floor(X[i*3]/delta))*Nc + int(floor(X[i*3+1]/delta));
    if (cells[c] == NULL_INDEX){
      cells[c] = uint64_t(i);
    }
    else{
      insert(cells[c],uint64_t(i),list);
    }
  }
}

bool handleCollision(float * X, uint64_t i, uint64_t j, float r, float k, float dt){
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

    X[i*3] -= fx;
    X[i*3+1] -= fy;
    X[j*3] += fx;
    X[j*3+1] += fy;
    return true;
  }
  return false;
}

uint64_t cellCollisions(uint64_t * cells, uint64_t * list, float * X, int Nc, uint64_t a1, uint64_t b1, uint64_t a2, uint64_t b2, float r, float k, float dt){
  if (a1 < 0 || a1 >= Nc || b1 < 0 || b1 >= Nc || a2 < 0 || a2 >= Nc || b2 < 0 || b2 >= Nc){
    return uint64_t(0);
  }
  uint64_t p1 = cells[a1*Nc+b1];
  uint64_t p2 = cells[a2*Nc+b2];

  uint64_t nCollisions = 0;

  if (p1 == NULL_INDEX || p2 == NULL_INDEX){
    return uint64_t(0);
  }

  while (p1 != NULL_INDEX){
    p2 = cells[a2*Nc+b2];
    while(p2 != NULL_INDEX){
        nCollisions += handleCollision(X,p1,p2,r,k,dt);
        p2 = list[p2];
    }
    p1 = list[p1];
  }
  return nCollisions;
}

uint64_t step(float * X, float * Xp, uint64_t * list, uint64_t * cells, float * offsets, float * thetas, std::default_random_engine generator, int N, int Nc, float delta, float L, float r, float v0, float Dr, float k, float dt){
  for (int i = 0; i < N; i++){
    for (int j = 0; j < 3; j++){
      Xp[i*3+j] = X[i*3+j];
    }
  }
  resetLists(cells, list, N, Nc);
  populateLists(X, cells, list, N, Nc, delta);
  uint64_t nCollisions = 0;
  for (int a = 0; a < Nc; a++){
    for (int b = 0; b < Nc; b++){
      nCollisions += cellCollisions(cells,list,X,Nc,a,b,a,b,r,k,dt);
      nCollisions += cellCollisions(cells,list,X,Nc,a,b,a-1,b-1,r,k,dt);
      nCollisions += cellCollisions(cells,list,X,Nc,a,b,a-1,b+1,r,k,dt);
      nCollisions += cellCollisions(cells,list,X,Nc,a,b,a+1,b+1,r,k,dt);
      nCollisions += cellCollisions(cells,list,X,Nc,a,b,a+1,b-1,r,k,dt);
      nCollisions += cellCollisions(cells,list,X,Nc,a,b,a-1,b,r,k,dt);
      nCollisions += cellCollisions(cells,list,X,Nc,a,b,a+1,b,r,k,dt);
      nCollisions += cellCollisions(cells,list,X,Nc,a,b,a,b-1,r,k,dt);
      nCollisions += cellCollisions(cells,list,X,Nc,a,b,a,b+1,r,k,dt);
    }
  }

  float D = std::sqrt(2.0*Dr/dt);
  for (int i = 0; i < N; i++){
    X[i*3+2] += dt*normal(generator)*D;
    X[i*3] += dt*v0*std::cos(X[i*3+2]);
    X[i*3+1] += dt*v0*std::sin(X[i*3+2]);

    float vx = X[i*3]-Xp[i*3];
    float vy = X[i*3+1]-Xp[i*3+1];
    float ux = 0.0; float uy = 0.0;
    float ang = X[i*3+2];
    bool flag = false;

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
    }


    if (X[i*3] == L){ X[i*3] -= 0.001;}
    if (X[i*3+1] == L){ X[i*3+1] -= 0.001;}
    offsets[i*2] = X[i*3];
    offsets[i*2+1] = X[i*3+1];
    thetas[i] = std::fmod(X[i*3+2],2.0*M_PI);
  }

  return nCollisions;
}
