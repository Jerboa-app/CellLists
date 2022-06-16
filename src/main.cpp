#include <iostream>
#include <sstream>
#include <iomanip>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#include <glUtils.h>
#include <typeUtils.h>

#include <particles.cpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <type.h>

#include <time.h>
#include <random>
#include <iostream>
#include <math.h>

const int resX = 720;
const int resY = 720;

const int subSample = 2;
const int N = 100000;
const float L = resX;
const float density = 0.75;
const float r = 2*std::sqrt(L*L*density/(N*M_PI));
const int Nc = std::ceil(L/r);
const float delta = L/Nc;
const float dt = 1.0 / 60.0;
const float k = 300.0;
const float Dr = 0.001;
const float v0 = 1.0*r;

std::default_random_engine generator;
std::uniform_real_distribution<float> U(0.0,1.0);

uint8_t frameId = 0;
double deltas[60];
double physDeltas[60];
double renderDeltas[60];

float X[N*3];
float Xp[N*3];
float offsets[N*2];
float thetas[N];
uint64_t cells[Nc*Nc];
uint64_t list[N];

int main(){

  for (int i = 0; i < 60; i++){deltas[i] = 0.0;}

  sf::ContextSettings contextSettings;
  contextSettings.depthBits = 24;
  contextSettings.antialiasingLevel = 0;

  sf::RenderWindow window(
    sf::VideoMode(resX,resY),
    "SFML",
    sf::Style::Close|sf::Style::Titlebar,
    contextSettings
  );
  window.setVerticalSyncEnabled(true);
  window.setFramerateLimit(60);
  window.setActive();

  glewInit();

  uint8_t debug = 0;

  generator.seed(clock());
  resetLists(cells,list,N,Nc);
  for (int i = 0; i < N; i++){
    X[i*3] = U(generator)*(L-2*r)+r;
    X[i*3+1] = U(generator)*(L-2*r)+r;
    X[i*3+2] = U(generator)*2.0*3.14;
    uint64_t c = int(floor(X[i*3]/delta))*Nc + int(floor(X[i*3+1]/delta));
    if (cells[c] == NULL_INDEX){
      cells[c] = uint64_t(i);
    }
    else{
      insert(cells[c],uint64_t(i), list);
    }
  }

  sf::Clock clock;
  sf::Clock physClock, renderClock;

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_POINT_SPRITE);
  glEnable( GL_BLEND );
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);

  GLuint freeTypeShader = glCreateProgram();
  compileShader(freeTypeShader,vertexShader,fragmentShader);
  glUseProgram(freeTypeShader);

  glm::mat4 textProj = glm::ortho(0.0,double(resX),0.0,double(resY));
  glUniformMatrix4fv(
    glGetUniformLocation(freeTypeShader,"proj"),
    1,
    GL_FALSE,
    &textProj[0][0]
  );

  FreeType freeType = initFreetype();

  GlyphMap ASCII;
  loadASCIIGlyphs(freeType.face,ASCII);

  const char * vert = "#version 330 core\n"
    "#define PI 3.14159265359\n"
    "precision highp float;\n"
    "layout(location = 0) in vec3 a_position;\n"
    "layout(location = 1) in vec2 a_offset;\n"
    "layout(location = 2) in float a_theta;\n"
    "float poly(float x, vec4 param){return clamp(x*param.x+pow(x,2.0)*param.y+"
    " pow(x,3.0)*param.z+param.w,0.0,1.0);\n}"
    "vec4 cmap(float t){\n"
    " return vec4( poly(t,vec4(1.2,-8.7,7.6,0.9)), poly(t,vec4(5.6,-13.4,7.9,0.2)), poly(t,vec4(-7.9,16.0,-8.4,1.2)), 1.0 );}"
    "uniform mat4 proj; uniform float scale; uniform float zoom;\n"
    "out vec4 o_colour;\n"
    "void main(){\n"
    " vec4 pos = proj*vec4(a_offset,0.0,1.0);\n"
    " gl_Position = vec4(a_position.xy+pos.xy,0.0,1.0);\n"
    " gl_PointSize = scale*zoom;\n"
    " o_colour = cmap(a_theta/(2.0*PI));\n"
    "}";
  const char * frag = "#version 330 core\n"
  "in vec4 o_colour; out vec4 colour;\n"
  "void main(){\n"
  " vec2 c = 2.0*gl_PointCoord-1.0;\n"
  " float d = length(c);\n"
  " float alpha = 1.0-smoothstep(0.99,1.01,d);\n"
  " colour = vec4(o_colour.rgb,alpha);\n"
  " if (colour.a == 0.0){discard;}"
  "}";

  GLuint shader = glCreateProgram();
  compileShader(shader,vert,frag);

  glUseProgram(shader);

  GLuint offsetVBO, thetaVBO;
  glGenBuffers(1,&offsetVBO);
  glBindBuffer(GL_ARRAY_BUFFER,offsetVBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(float)*N*2,&offsets[0],GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER,0);

  glGenBuffers(1,&thetaVBO);
  glBindBuffer(GL_ARRAY_BUFFER,thetaVBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(float)*N,&thetas[0],GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER,0);

  float vertices[] = {
    0.0,0.0,0.0
  };

  GLuint vertVAO, vertVBO;
  glGenVertexArrays(1,&vertVAO);
  glGenBuffers(1,&vertVBO);
  glBindVertexArray(vertVAO);
  glBindBuffer(GL_ARRAY_BUFFER,vertVBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);

  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, offsetVBO);
  glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);
  glBindBuffer(GL_ARRAY_BUFFER,0);
  glVertexAttribDivisor(1,1);

  glEnableVertexAttribArray(2);
  glBindBuffer(GL_ARRAY_BUFFER, thetaVBO);
  glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,sizeof(float),(void*)0);
  glBindBuffer(GL_ARRAY_BUFFER,0);
  glVertexAttribDivisor(2,1);

  glError("Arrays and buffers for particles drawing :");

  double zoomLevel = 1.0;

  glm::mat4 trans = glm::mat4(1.0f);
  glm::mat4 view = glm::translate(trans,glm::vec3(0.0,0.0,0.0));
  glm::mat4 proj = glm::ortho(0.0,double(resX)/zoomLevel,0.0,double(resY)/zoomLevel,0.1,100.0);
  proj = proj*view;
  glUniformMatrix4fv(
    glGetUniformLocation(shader,"proj"),
    1,
    GL_FALSE,
    &proj[0][0]
  );

  glUniform1f(
    glGetUniformLocation(shader,"scale"),
    r
  );

  glUniform1f(
    glGetUniformLocation(shader,"zoom"),
    zoomLevel
  );

  glViewport(0,0,resX,resY);

  // text buffers
  GLuint glyphVAO, glyphVBO;
  glGenVertexArrays(1,&glyphVAO);
  glGenBuffers(1,&glyphVBO);
  glBindVertexArray(glyphVAO);
  glBindBuffer(GL_ARRAY_BUFFER, glyphVBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(float)*6*4,NULL,GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,4*sizeof(float),0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glError("Arrays and buffers for type :");


  double mouseX = 220.0;
  double mouseY = 220.0;

  double x = 220.0;
  double y = 220.0;

  bool scrolled = false;
  bool moving = false;

  double avgCollisionsPerFrame = 0.0;

  while (window.isOpen()){

    sf::Event event;
    scrolled = false;
    while (window.pollEvent(event)){
      if (event.type == sf::Event::Closed){
        return 0;
      }
      if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape){
        return 0;
      }
      if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F1){
        debug = !debug;
      }
      if (event.type == sf::Event::MouseWheelScrolled){
        mouseX = event.mouseWheelScroll.x;
        mouseY = event.mouseWheelScroll.y;
        double z = event.mouseWheelScroll.delta;
        if (zoomLevel >= 1.0){
          zoomLevel += z;
          zoomLevel < 1.0 ? zoomLevel = 1.0 : 0;
        }
        else{
          zoomLevel += 1.0/z;
        }
        scrolled = true;
      }
      if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left){
      }
      if (event.type == sf::Event::MouseMoved && sf::Mouse::isButtonPressed(sf::Mouse::Left)){
        moving = true;
      }
      if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left){
        moving = false;
      }
      if (moving && sf::Mouse::isButtonPressed(sf::Mouse::Left)){
        sf::Vector2i mouse = sf::Mouse::getPosition(window);
        mouseX = 540.0-mouse.x;
        mouseY = 540.0-mouse.y;
      }
    }

    window.clear(sf::Color::White);
    glClearColor(1.0f,1.0f,1.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    avgCollisionsPerFrame = 0.0;

    physClock.restart();
    for (int s = 0; s < subSample; s++){
      uint64_t nc = step(
        X,
        Xp,
        list,
        cells,
        offsets,
        thetas,
        generator,
        N,
        Nc,
        delta,
        L,
        r,
        v0,
        Dr,
        k,
        1.0/300.0
      );
      avgCollisionsPerFrame += nc;
    }
    physDeltas[frameId] = physClock.getElapsedTime().asSeconds();
    avgCollisionsPerFrame /= double(subSample);

    renderClock.restart();

    glUseProgram(shader);
    x = mouseX; y = (resX-mouseY);
    glm::mat4 view = glm::translate(glm::mat4(1.0),glm::vec3(x,y,0.0)) *
      glm::scale(glm::mat4(1.0),glm::vec3(zoomLevel,zoomLevel,0.0)) *
      glm::translate(glm::mat4(1.0),glm::vec3(-x,-y,0.0));
    proj = glm::ortho(0.0,double(resX),0.0,double(resY),0.1,100.0);
    proj = proj*view;
    glUniformMatrix4fv(
      glGetUniformLocation(shader,"proj"),
      1,
      GL_FALSE,
      &proj[0][0]
    );

    glUniform1f(
      glGetUniformLocation(shader,"zoom"),
      zoomLevel
    );
    glBindBuffer(GL_ARRAY_BUFFER,offsetVBO);
    glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(float)*N*2,&offsets[0]);
    glBindBuffer(GL_ARRAY_BUFFER,0);

    glBindBuffer(GL_ARRAY_BUFFER,thetaVBO);
    glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(float)*N,&thetas[0]);
    glBindBuffer(GL_ARRAY_BUFFER,0);

    glUseProgram(shader);
    glBindVertexArray(vertVAO);
    glDrawArraysInstanced(GL_POINTS,0,1,N);
    glBindVertexArray(0);

    if (debug){
      glError();
      glUseProgram(freeTypeShader);
      glError("Setting type projection matrix: ");

      double delta = 0.0;
      double renderDelta = 0.0;
      double physDelta = 0.0;
      for (int n = 0; n < 60; n++){
        delta += deltas[n];
        renderDelta += renderDeltas[n];
        physDelta += physDeltas[n];
      }
      delta /= 60.0;
      renderDelta /= 60.0;
      physDelta /= 60.0;
      std::stringstream debugText;

      sf::Vector2i mouse = sf::Mouse::getPosition(window);

      debugText << "Delta: " << fixedLengthNumber(delta,6) <<
        " (FPS: " << fixedLengthNumber(1.0/delta,4) << ")" <<
        "\n" <<
        "Render/Physics: " << fixedLengthNumber(renderDelta,6) << "/" << fixedLengthNumber(physDelta,6) <<
        "\n" <<
        "Mouse (" << fixedLengthNumber(mouse.x,4) << "," << fixedLengthNumber(mouse.y,4) << ")" <<
        "\n" <<
        "Collision/Frame: " << fixedLengthNumber(avgCollisionsPerFrame,6) <<
        "\n";

      renderText(
        ASCII,
        freeTypeShader,
        debugText.str(),
        64.0f,resY-64.0f,
        0.5f,
        glm::vec3(0.0f,0.0f,0.0f),
        glyphVAO,glyphVBO
      );
    }

    window.display();

    deltas[frameId] = clock.getElapsedTime().asSeconds();
    renderDeltas[frameId] = renderClock.getElapsedTime().asSeconds();

    clock.restart();

    if (frameId == 60){
      frameId = 0;
    }
    else{
      frameId++;
    }
  }
  return 0;
}
