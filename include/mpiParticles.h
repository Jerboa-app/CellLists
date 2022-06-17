#ifndef MPIPARTICLES_H
#define MPIPARTICLES_H

#include <cstdint>
#include <math.h>
#include <random>

std::normal_distribution<double> normal(0.0,1.0);

const uint64_t NULL_INDEX = uint64_t(-1);

void resetLists(uint64_t * cells, uint64_t * list, int N, int Nc);

void insert(uint64_t next, uint64_t particle, uint64_t * list);

void populateLists(float * X, uint64_t * cells, uint64_t * list, int N, int Nc, float delta);

bool handleCollision(float * X, float * F, uint64_t i, uint64_t j, float k = 300.0, float dt = 1.0/60.0);

uint64_t cellCollisions(uint64_t * cells, uint64_t * list, float * X, float * F, int Nc, uint64_t a1, uint64_t b1, uint64_t a2, uint64_t b2, float r, float k, float dt);

#endif
