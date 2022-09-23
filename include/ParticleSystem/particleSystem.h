#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

const uint64_t NULL_INDEX = uint64_t(-1);
const int ARPERIOD = 60;
const float attractionStrength = 0.01;
const float repellingStrength = 0.02;

#include <vector>
#include <time.h>
#include <math.h>
#include <random>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <shaders.h>
#include <glUtils.h>

std::default_random_engine generator;
std::uniform_real_distribution<float> U(0.0,1.0);
std::normal_distribution<double> normal(0.0,1.0);

class ParticleSystem{
public:

  ParticleSystem(uint64_t N, float dt = 1.0/120.0, float density = 0.5, uint64_t seed = clock())
  : nParticles(N), radius(std::sqrt(density/(N*M_PI))),speed(std::sqrt(density/(N*M_PI))/0.2),drag(1.0),rotationalDrag(1.0),mass(0.1),
    momentOfInertia(0.01), forceStrength(300.0),rotationalDiffusion(0.001),
    dt(dt)
  {
    generator.seed(seed);
    Nc = std::ceil(1.0/(4.0*radius));
    delta = 1.0 / Nc;

    for (int c = 0; c < Nc*Nc; c++){
      cells.push_back(NULL_INDEX);
    }

    for (int i = 0; i < N; i++){
      float x = U(generator)*(1.0-2*radius)+radius;
      float y = U(generator)*(1.0-2*radius)+radius;
      float theta = U(generator)*2.0*3.14;

      addParticle(x,y,theta);
      uint64_t c = hash(i);
      if (cells[c] == NULL_INDEX){
        cells[c] = i;
      }
      else{
        insert(cells[c],uint64_t(i));
      }
    }
    initialiseGL();
  }

  void step();

  void addParticle(float x, float y, float theta){
    state.push_back(x);
    state.push_back(y);
    state.push_back(theta);

    lastState.push_back(x);
    lastState.push_back(y);
    lastState.push_back(theta);

    forces.push_back(0.0);
    forces.push_back(0.0);

    noise.push_back(0.0);
    noise.push_back(0.0);

    list.push_back(NULL_INDEX);
  }

  void removeParticle(uint64_t i){
    if (state.size() >= 3*i){
      state.erase(
        state.begin()+3*i,
        state.begin()+3*i+3
      );

      lastState.erase(
        lastState.begin()+3*i,
        lastState.begin()+3*i+3
      );

      forces.erase(
        forces.begin()+2*i,
        forces.begin()+2*i+1
      );

      noise.erase(
        noise.begin()+2*i,
        noise.begin()+2*i+1
      );

      list.erase(list.begin()+i);
    }
  }

  uint64_t size(){
    return uint64_t(std::floor(state.size() / 3));
  }

  uint8_t nAttractors(){return uint8_t(attractors.size());}
  uint8_t nRepellers(){return uint8_t(repellers.size());}

  void addRepeller(float x, float y);
  void addAttractor(float x, float y);
  bool deleteAttratorRepellor(float x, float y);
  // GL public members
  void setProjection(glm::mat4 p);
  void draw(uint64_t frameId, float zoomLevel, float resX, float resY);

  ~ParticleSystem(){
    // kill some GL stuff
    glDeleteProgram(particleShader);
    glDeleteProgram(arShader);

    glDeleteBuffers(1,&offsetVBO);
    glDeleteBuffers(1,&vertVBO);
    glDeleteBuffers(1,&arOffsetVBO);

    glDeleteVertexArrays(1,&vertVAO);
    glDeleteVertexArrays(1,&arVAO);
  }

private:

  std::vector<float> state;
  std::vector<float> lastState;
  std::vector<float> noise;

  std::vector<float> forces;
  std::vector<std::pair<float,float>> attractors;
  std::vector<std::pair<float,float>> repellers;

  std::vector<uint64_t> cells;
  std::vector<uint64_t> list;

  uint64_t Nc;
  float delta;

  uint64_t nParticles;

  float forceStrength;
  float rotationalDiffusion;
  float speed;
  float radius;
  float drag;
  float rotationalDrag;
  float mass;
  float momentOfInertia;
  float dt;

  // GL data members
  GLuint particleShader, offsetVBO, vertVAO, vertVBO;
  glm::mat4 projection;
  glm::mat4 arPositions;

  GLuint arShader, arOffsetVBO, arVAO;

  float vertices[3] = {0.0,0.0,0.0};
  float arOffsets[16];

  void resetLists();
  void insert(uint64_t next, uint64_t particle);
  void populateLists();
  void handleCollision(uint64_t i, uint64_t j);
  void cellCollisions(
    uint64_t a1,
    uint64_t b1,
    uint64_t a2,
    uint64_t b2
  );

  uint64_t hash(uint64_t particle){
    return uint64_t(floor(state[particle*3]/delta))*Nc + uint64_t(floor(state[particle*3+1]/delta));
  }

  void fillARMatrix();

  // GL private members
  void initialiseGL();
};

#endif
