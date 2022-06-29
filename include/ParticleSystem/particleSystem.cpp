#include <ParticleSystem/particleSystem.h>
#include <time.h>

void ParticleSystem::resetLists(){
  for (int i = 0; i < Nc*Nc; i++){
    cells[i] = NULL_INDEX;
  }
  for (int i = 0; i < nParticles; i++){
    list[i] = NULL_INDEX;
  }
}

void ParticleSystem::insert(uint64_t next, uint64_t particle){
  uint64_t i = next;
  while (list[i] != NULL_INDEX){
    i = list[i];                                                                  // someone here!
  }
  list[i] = particle;                                                             // it's free realestate
}

void ParticleSystem::populateLists(
){
  for (int i = 0; i < nParticles; i++){
    uint64_t c = hash(i);                                                    // flat index for the particle's cell
    if (cells[c] == NULL_INDEX){
      cells[c] = uint64_t(i);                                                     // we are the head!
    }
    else{
      insert(cells[c],uint64_t(i));
    }
  }
}

void ParticleSystem::handleCollision(uint64_t i, uint64_t j){
  if (i == j){return;}
  float rx,ry,dd,d,mag,fx,fy;
  rx = state[j*3]-state[i*3];
  ry = state[j*3+1]-state[i*3+1];
  dd = rx*rx+ry*ry;
  if (dd < radius*radius){
    d = std::sqrt(dd);
    mag = forceStrength*(radius-d)/d;
    fx = mag*rx;
    fy = mag*ry;

    forces[i*2] -= fx;
    forces[i*2+1] -= fy;

    forces[j*2] += fx;
    forces[j*2+1] += fy;
  }
}

void ParticleSystem::cellCollisions(
  uint64_t a1,
  uint64_t b1,
  uint64_t a2,
  uint64_t b2
){
  if (a1 < 0 || a1 >= Nc || b1 < 0 || b1 >= Nc || a2 < 0 || a2 >= Nc || b2 < 0 || b2 >= Nc){
    return;                                                                      // not a cell
  }
  uint64_t p1 = cells[a1*Nc+b1];
  uint64_t p2 = cells[a2*Nc+b2];

  if (p1 == NULL_INDEX || p2 == NULL_INDEX){
    return;                                                                      // nobody here!
  }

  while (p1 != NULL_INDEX){
    p2 = cells[a2*Nc+b2];                                                        // flat index
    while(p2 != NULL_INDEX){
        handleCollision(p1,p2);                                                  // check this potential collision
        p2 = list[p2];                                                           // p2 points to a new particle in the same box (or null)
    }
    p1 = list[p1];                                                               // p1 points to a new particle in the same box (or null)
  }
}

void ParticleSystem::addRepeller(float x, float y){
  repellers.push_back(x);
  repellers.push_back(y);
}

void ParticleSystem::addAttractor(float x, float y){
  attractors.push_back(x);
  attractors.push_back(y);
}

bool ParticleSystem::deleteAttratorRepellor(float x, float y){
  float dist = 0.1*radius*5.0;
  for (int i = 0; i < attractors.size(); i++){
    float rx = x-attractors[i*2];
    float ry = y-attractors[i*2+1];
    if (rx*rx+ry*ry < dist){
      attractors.erase(attractors.begin()+i);
      return true;
    }
  }

  for (int i = 0; i < repellers.size(); i++){
    float rx = x-repellers[i*2];
    float ry = y-repellers[i*2+1];
    if (rx*rx+ry*ry < dist){
      repellers.erase(repellers.begin()+i);
      return true;
    }
  }
  return false;
}

void ParticleSystem::step(){
  clock_t tic = clock();
  for (int i = 0; i < nParticles; i++){
    forces[i*2] = 0.0;
    forces[i*2+1] = 0.0;
  }
  resetLists();
  populateLists();
  float setup = (clock()-tic)/float(CLOCKS_PER_SEC);
  tic = clock();
  for (int a = 0; a < Nc; a++){
    for (int b = 0; b < Nc; b++){
      cellCollisions(a,b,a,b);
      cellCollisions(a,b,a-1,b-1);
      cellCollisions(a,b,a-1,b+1);
      cellCollisions(a,b,a+1,b+1);
      cellCollisions(a,b,a+1,b-1);
      cellCollisions(a,b,a-1,b);
      cellCollisions(a,b,a+1,b);
      cellCollisions(a,b,a,b-1);
      cellCollisions(a,b,a,b+1);
    }
  }
  float col = (clock()-tic)/float(CLOCKS_PER_SEC);
  tic = clock();

  float D = std::sqrt(2.0*rotationalDiffusion/dt);
  float dtdt = dt*dt;

  float cr = (rotationalDrag*dt)/(2.0*momentOfInertia);
  float br = 1.0 / (1.0 + cr);
  float ar = (1.0-cr)*br;

  float ct = (drag*dt)/(2.0*mass);
  float bt = 1.0 / (1.0 + ct);
  float at = (1.0-ct)*bt;

  for (int i = 0; i < nParticles; i++){

    // for (int j = 0; j < attractors.size(); j++){
    //     float rx = attractors[j*2]-state[i*3];
    //     float ry = attractors[j*2+1]-state[i*3+1];
    //
    //     float d = sqrt(rx*rx+ry*ry);
    //
    //     if (d < radius){
    //       std::uniform_real_distribution<float> U(0.0,6.28);
    //       float theta = U(generator);
    //       forces[i*2] -= attractionStrength*cos(theta)/d;
    //       forces[i*2+1] -= attractionStrength*sin(theta)/d;
    //     }
    //     else{
    //       d = d*d*d;
    //       forces[i*2] += attractionStrength*rx/d;
    //       forces[i*2+1] += attractionStrength*ry/d;
    //     }
    //   }
    // for (int j = 0; j < repellers.size(); j++){
    //     float rx = state[i*3]-repellers[j*2];
    //     float ry = state[i*3+1]-repellers[j*2+1];
    //
    //     float dd = rx*rx+ry*ry;
    //
    //     forces[i*2] += repellingStrength*rx/dd;
    //     forces[i*2+1] += repellingStrength*ry/dd;
    // }

    noise[i*2+1] = noise[i*2];
    noise[i*2] = normal(generator);

    float x = state[i*3];
    float y = state[i*3+1];
    float theta = state[i*3+2];

    float xp = lastState[i*3];
    float yp = lastState[i*3+1];
    float thetap = lastState[i*3+2];

    float ax = drag*speed*cos(theta)+forces[i*2];
    float ay = drag*speed*sin(theta)+forces[i*2+1];

    state[i*3] = 2.0*bt*x - at*xp + (bt*dtdt/mass)*ax;
    state[i*3+1] = 2.0*bt*y - at*yp + (bt*dtdt/mass)*ay;
    state[i*3+2] = 2.0*br*theta - ar*thetap + (br*dt/(2.0*momentOfInertia))*(noise[i*2]+noise[i*2+1])*dt*rotationalDrag*D;

    lastState[i*3] = x;
    lastState[i*3+1] = y;
    lastState[i*3+2] = theta;

    float vx = state[i*3]-lastState[i*3];
    float vy = state[i*3+1]-lastState[i*3+1];
    float ux = 0.0; float uy = 0.0;
    float ang = state[i*3+2];
    bool flag = false;

    // kill the particles movement if it's outside the box
    if (state[i*3]-radius < 0 || state[i*3]+radius > 1.0){
      ux = -vx;
      ang = std::atan2(vy,ux);
      flag = true;
    }

    if (state[i*3+1]-radius < 0 || state[i*3+1]+radius > 1.0){
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
      state[i*3+2] = ang;
      state[i*3+1] += uy;
      state[i*3] += ux;

      lastState[i*3+2] = ang;
      lastState[i*3+1] = state[i*3+1]-0.5*uy;
      lastState[i*3] = state[i*3]-0.5*ux;
    }

    if (state[i*3] == 1.0){ state[i*3] -= 0.001;}
    if (state[i*3+1] == 1.0){ state[i*3+1] -= 0.001;}
  }
  float updates = (clock()-tic)/float(CLOCKS_PER_SEC);
  tic = clock();
}

void ParticleSystem::setProjection(glm::mat4 p){
  projection = p;
  glUseProgram(particleShader);
  glUniformMatrix4fv(
    glGetUniformLocation(particleShader,"proj"),
    1,
    GL_FALSE,
    &projection[0][0]
  );
  glUseProgram(arShader);
  glUniformMatrix4fv(
    glGetUniformLocation(arShader,"proj"),
    1,
    GL_FALSE,
    &projection[0][0]
  );
}

void ParticleSystem::initialiseGL(){
  // a buffer of particle states
  glGenBuffers(1,&offsetVBO);
  glBindBuffer(GL_ARRAY_BUFFER,offsetVBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(float)*nParticles*3,&state[0],GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER,0);

  // setup an array object
  glGenVertexArrays(1,&vertVAO);
  glGenBuffers(1,&vertVBO);
  glBindVertexArray(vertVAO);
  glBindBuffer(GL_ARRAY_BUFFER,vertVBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  // place dummy vertices for instanced particles
  glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);

  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, offsetVBO);
  // place states
  glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
  glBindBuffer(GL_ARRAY_BUFFER,0);
  glVertexAttribDivisor(1,1);

  glError("initialised particles");

  particleShader = glCreateProgram();
  compileShader(particleShader,particleVertexShader,particleFragmentShader);
  glUseProgram(particleShader);

  glUniformMatrix4fv(
    glGetUniformLocation(particleShader,"proj"),
    1,
    GL_FALSE,
    &projection[0][0]
  );
  // now for the toys
  for (int i = 0; i < 16; i++){
    arOffsets[i] = i;
  }

  glGenBuffers(1,&arOffsetVBO);
  glGenVertexArrays(1,&arVAO);
  glBindVertexArray(arVAO);

  glBindBuffer(GL_ARRAY_BUFFER,vertVBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);

  glBindBuffer(GL_ARRAY_BUFFER,arOffsetVBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(arOffsets),arOffsets,GL_STATIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,1*sizeof(float),(void*)0);
  glVertexAttribDivisor(1,1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  arShader = glCreateProgram();
  compileShader(arShader,atrepVertexShader,atRepfragmentShader);
  glUseProgram(arShader);

  fillARMatrix();

  glUniform1f(
    glGetUniformLocation(arShader,"maxNANR"),
    float(8)
  );

  glUniformMatrix4fv(
    glGetUniformLocation(arShader,"proj"),
    1,
    GL_FALSE,
    &projection[0][0]
  );

  glUniformMatrix4fv(
    glGetUniformLocation(arShader,"toys"),
    1,
    GL_FALSE,
    &arPositions[0][0]
  );

  glUniform1f(
    glGetUniformLocation(arShader,"scale"),
    radius*5.0
  );

  glUniform1f(
    glGetUniformLocation(arShader,"T"),
    ARPERIOD
  );

  glError("initialised toys");
}

void ParticleSystem::fillARMatrix(){
  arPositions = glm::mat4(0.0);
  for (int i = 0; i < 8; i++){
    if (i < nAttractors()){
      int col = floor(i)/2.0;
      int o = int(2.0*fmod(float(i),2.0));
      arPositions[col][o] = attractors[i*2];
      arPositions[col][o+1] = attractors[i*2+1];
    }
  }

  for (int i = 0; i < 8; i++){
    if (i < nRepellers()){
      int col = floor(i+8)/2.0;
      int o = int(2.0*fmod(float(i+8),2.0));
      arPositions[col][o] = repellers[i*2];
      arPositions[col][o+1] = repellers[i*2+1];
    }
  }
}

void ParticleSystem::draw(
  uint64_t frameId,
  float zoomLevel,
  float resX,
  float resY
){
  glUseProgram(particleShader);

  glUniform1f(
    glGetUniformLocation(particleShader,"zoom"),
    zoomLevel
  );

  glUniform1f(
    glGetUniformLocation(particleShader,"scale"),
    resX*radius
  );

  glBindBuffer(GL_ARRAY_BUFFER,offsetVBO);
  glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(float)*nParticles*3,&state[0]);
  glBindBuffer(GL_ARRAY_BUFFER,0);

  glError("particles buffers");

  glBindVertexArray(vertVAO);
  glDrawArraysInstanced(GL_POINTS,0,1,size());
  glBindVertexArray(0);

  glError("draw particles");

  glUseProgram(arShader);

  glUniform1f(
    glGetUniformLocation(arShader,"zoom"),
    zoomLevel
  );

  glUniform1f(
    glGetUniformLocation(arShader,"t"),
    frameId % ARPERIOD
  );

  glUniform1i(
    glGetUniformLocation(arShader,"na"),
    attractors.size()
  );

  glUniform1i(
    glGetUniformLocation(arShader,"nr"),
    repellers.size()
  );

  glBindBuffer(GL_ARRAY_BUFFER,arOffsetVBO);
  glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(float)*16,&arOffsets[0]);
  glBindBuffer(GL_ARRAY_BUFFER,0);

  glBindVertexArray(arVAO);
  glDrawArraysInstanced(GL_POINTS,0,1,16);
  glBindVertexArray(0);

  glError("Draw toys");

}
