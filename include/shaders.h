#ifndef SHADERS_H
#define SHADERS_H

const char * boxVertexShader = "#version 330 core\n"
  "layout(location=0) in vec2 a_position;\n"
  "uniform vec3 colour; out vec4 o_colour;\n"
  "uniform mat4 proj;\n"
  "void main(){\n"
  " vec4 pos = vec4(a_position.xy,0.0,1.0);\n"
  " o_colour = vec4(colour,.5);\n"
  " gl_Position = pos;\n"
  "}";

const char * boxFragmentShader = "#version 330 core\n"
  "in vec4 o_colour; out vec4 colour;\n"
  "void main(){colour=o_colour;\n}";

// basic particle shader
// cmap(t) defines a periodic RGB colour map for t \in [0,1] using cubic
// interpolation that's hard coded, it's based upon the PHASE4 colour map
// from https://github.com/peterkovesi/PerceptualColourMaps.jl
// which is derived from ColorCET https://colorcet.com/
const char * particleVertexShader = "#version 330 core\n"
  "#define PI 3.14159265359\n"
  "precision highp float;\n"
  "layout(location = 0) in vec3 a_position;\n"
  "layout(location = 1) in vec3 a_offset;\n"
  "float poly(float x, vec4 param){return clamp(x*param.x+pow(x,2.0)*param.y+"
  " pow(x,3.0)*param.z+param.w,0.0,1.0);\n}"
  "vec4 cmap(float t){\n"
  " return vec4( poly(t,vec4(1.2,-8.7,7.6,0.9)), poly(t,vec4(5.6,-13.4,7.9,0.2)), poly(t,vec4(-7.9,16.0,-8.4,1.2)), 1.0 );}"
  "uniform mat4 proj; uniform float scale; uniform float zoom;\n"
  "out vec4 o_colour;\n"
  "void main(){\n"
  " vec4 pos = proj*vec4(a_offset.xy,0.0,1.0);\n"
  " gl_Position = vec4(a_position.xy+pos.xy,0.0,1.0);\n"
  " gl_PointSize = scale*zoom;\n"
  " o_colour = cmap(mod(a_offset.z,2.0*PI)/(2.0*PI));\n"
  "}";
const char * particleFragmentShader = "#version 330 core\n"
  "in vec4 o_colour; out vec4 colour;\n"
  "void main(){\n"
  " vec2 c = 2.0*gl_PointCoord-1.0;\n"
  " float d = length(c);\n"
  // bit of simple AA
  " float alpha = 1.0-smoothstep(0.99,1.01,d);\n"
  " colour = vec4(o_colour.rgb,alpha);\n"
  " if (colour.a == 0.0){discard;}"
  "}";

const char * atrepVertexShader = "#version 330 core\n"
  "precision highp float; precision highp int;\n"
  "layout(location = 0) in vec3 a_position;\n"
  "layout(location = 1) in float a_offset;\n"
  "out vec4 o_colour; out float o_time;\n"
  "uniform int na; uniform int nr; uniform mat4 attr; uniform mat4 rep;\n"
  "uniform float maxNANR;\n"
  "uniform float scale; uniform mat4 proj; uniform float zoom;\n"
  "uniform float t; uniform float T;\n"
  "void main(void){\n"
  "   int a = int(floor(a_offset)); float x = 0.0; float y = 0.0; float drawa = 0.0; float drawr = 0.0;\n"
  "   if (a < na){ int col = int(floor(float(a)/2.0)); int o = int(2.0*mod(float(a),2.0));\n"
  "        x = attr[col][0+o]; y = attr[col][1+o]; drawa = 0.33;}\n"
  "   if (a >= 8 && a < 8+nr){ a = a-8; int col = int(floor(float(a)/2.0)); int o = int(2.0*mod(float(a),2.0));\n"
  "        x = rep[col][0+o]; y = rep[col][1+o]; drawr = 0.33;}\n"
  "   vec4 pos = proj*vec4(x,y,0.0,1.0);\n"
  "   gl_Position = vec4(a_position.xy+pos.xy,0.0,1.0);\n"
  "   o_colour = vec4(1.0,1.0,1.0,0.0); float time = 1.0;\n"
  "   if (drawr > 0.0 ){ o_colour = vec4(1.0,0.0,0.0,1.0); time = t/T;}"
  "   else if (drawa > 0.0){ o_colour = vec4(0.0,1.0,0.0,1.0); time = 1.0-t/T;}\n"
  "   gl_PointSize = scale*zoom*time;\n"
  "}";

const char * atRepfragmentShader= "#version 330 core\n"
  "in vec4 o_colour; out vec4 colour;\n"
  "void main(){\n"
  " if (o_colour.a == 0.0){discard;}"
  " vec2 c = 2.0*gl_PointCoord-1.0;\n"
  " float d = length(c);\n"
  // bit of simple AA
  " float alpha = 1.0-smoothstep(0.99,1.01,d);\n"
  " colour = vec4(o_colour.rgb,alpha);\n"
  " if (colour.a == 0.0){discard;}"
  "}";

#endif
