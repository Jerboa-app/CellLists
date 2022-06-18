#ifndef UTILS_H
#define UTILS_H

bool deleteAttratorRepellor(
  float x,
  float y,
  std::vector<glm::vec2> & attractors,
  std::vector<glm::vec2> & repellors,
  float dist
){

  for (int i = 0; i < attractors.size(); i++){
    float rx = x-attractors[i].x;
    float ry = y-attractors[i].y;
    if (rx*rx+ry*ry < dist){
      attractors.erase(attractors.begin()+i);
      return true;
    }
  }

  for (int i = 0; i < repellors.size(); i++){
    float rx = x-repellors[i].x;
    float ry = y-repellors[i].y;
    if (rx*rx+ry*ry < dist){
      repellors.erase(repellors.begin()+i);
      return true;
    }
  }
  return false;
}

#endif
