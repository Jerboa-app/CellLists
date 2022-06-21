#include <iostream>
#include <sstream>
#include <iomanip>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#include <orthoCam.h>

#include <glUtils.h>
#include <utils.h>
#include <shaders.h>

#include <particles.cpp>
#include <Text/textRenderer.cpp>

#include <time.h>
#include <random>
#include <iostream>
#include <math.h>
#include <vector>

const int resX = 720;
const int resY = 720;

const int subSample = 1;
const int N = 100000;
const float L = 1.0;                                                             // box length
const float density = 0.75;
const float r = 2*std::sqrt(L*L*density/(N*M_PI));                                // set r for given density
const int Nc = std::ceil(L/r);                                                    // roughly optimal parameter
const float delta = L/Nc;                                                         // side length of each cell
const float dt = 1.0 / 600.0;
// motion parameters
const float k = 300.0;                                                            // "hardness" of hermonic collision force
const float Dr = 0.01;
const float v0 = 1.0*r;
const float drag = 1.0;
const float rotDrag = 1.0;
const float M = 0.1;
const float J = 0.01;

const float attractionStrength = 0.01;
const float repellingStrength = 0.02;
const int maxAttractors = 8;
const int maxRepellors = 8;
const float wellSize = r*5.0;

std::default_random_engine generator;
std::uniform_real_distribution<float> U(0.0,1.0);

// for smoothing delta numbers
uint8_t frameId = 0;
double deltas[60];
double physDeltas[60];
double renderDeltas[60];

// particle data
float X[N*3];
float Xp[N*3];
float F[N*2];

float noise[N*2];

float offsets[N*2];
float thetas[N];
float thetap[N];
uint64_t cells[Nc*Nc];
uint64_t list[N];

std::vector<glm::vec2> attractors;
std::vector<glm::vec2> repellors;

int main(){

  for (int i = 0; i < 60; i++){deltas[i] = 0.0;}

  sf::ContextSettings contextSettings;
  contextSettings.depthBits = 24;
  contextSettings.antialiasingLevel = 0;

  sf::RenderWindow window(
    sf::VideoMode(resX,resY),
    "Particles",
    sf::Style::Close|sf::Style::Titlebar,
    contextSettings
  );
  window.setVerticalSyncEnabled(true);
  window.setFramerateLimit(60);
  window.setActive();

  glewInit();

  uint8_t debug = 0;

  // init particles in the box
  generator.seed(clock());
  resetLists(cells,list,N,Nc);
  for (int i = 0; i < N; i++){
    X[i*3] = U(generator)*(L-2*r)+r;
    X[i*3+1] = U(generator)*(L-2*r)+r;
    X[i*3+2] = U(generator)*2.0*3.14;
    Xp[i*3] = X[i*3];
    Xp[i*3+1] = X[i*3+1];
    Xp[i*3+2] = X[i*3+2];
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

  glm::mat4 defaultProj = glm::ortho(0.0,double(resX),0.0,double(resY),0.1,100.0);
  glm::mat4 textProj = glm::ortho(0.0,double(resX),0.0,double(resY));

  // for rendering particles using gl_point instances
  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_POINT_SPRITE);
  glEnable( GL_BLEND );
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);

  // for freetype rendering
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // must be initialised before so the shader is in use..?
  TextRenderer textRenderer(textProj);

  Type OD("resources/fonts/","OpenDyslexic-Regular.otf",48);

  GLuint shader = glCreateProgram();
  compileShader(shader,particleVertexShader,particleFragmentShader);

  glUseProgram(shader);

  // generate buffers for positions and thetas
  GLuint offsetVBO, thetaVBO;
  glGenBuffers(1,&offsetVBO);
  glBindBuffer(GL_ARRAY_BUFFER,offsetVBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(float)*N*2,&offsets[0],GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER,0);

  glGenBuffers(1,&thetaVBO);
  glBindBuffer(GL_ARRAY_BUFFER,thetaVBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(float)*N,&thetas[0],GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER,0);

  // little vertices for the particle instance
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

  OrthoCam camera(resX,resY,glm::vec2(0.0,0.0));

  glm::mat4 proj = camera.getVP();

  glUniformMatrix4fv(
    glGetUniformLocation(shader,"proj"),
    1,
    GL_FALSE,
    &proj[0][0]
  );

  glUniform1f(
    glGetUniformLocation(shader,"scale"),
    resX*r
  );

  glUniform1f(
    glGetUniformLocation(shader,"zoom"),
    camera.getZoomLevel()
  );

  // attraction repulsion gl
  GLuint atrepShader = glCreateProgram();
  compileShader(atrepShader,atrepVertexShader,atRepfragmentShader);

  glUseProgram(atrepShader);

  glUniform1f(
    glGetUniformLocation(atrepShader,"maxNANR"),
    float(maxRepellors+maxAttractors)
  );

  GLuint atRepOffsetVBO, atRepVAO;
  float atRepOffsets[maxRepellors+maxAttractors];
  for (int i = 0; i < maxRepellors+maxAttractors; i++){
    atRepOffsets[i] = float(i);
  }
  glGenBuffers(1,&atRepOffsetVBO);
  glGenVertexArrays(1,&atRepVAO);
  glBindVertexArray(atRepVAO);

  glBindBuffer(GL_ARRAY_BUFFER,vertVBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);

  glBindBuffer(GL_ARRAY_BUFFER,atRepOffsetVBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(atRepOffsets),atRepOffsets,GL_STATIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,1*sizeof(float),(void*)0);
  glVertexAttribDivisor(1,1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  glError("Arrays and buffers for atrep drawing :");

  glUseProgram(atrepShader);

  atrepUniforms(
    atrepShader,
    attractors,
    repellors,
    maxAttractors,
    maxRepellors,
    proj,
    wellSize*resX,
    camera.getZoomLevel(),
    frameId,
    60.0,
    maxRepellors+maxAttractors
  );

  glViewport(0,0,resX,resY);

  // box
  GLuint boxShader = glCreateProgram();
  compileShader(boxShader,boxVertexShader,boxFragmentShader);

  GLuint boxVAO, boxVBO;
  glGenVertexArrays(1,&boxVAO);
  glGenBuffers(1,&boxVBO);
  glBindVertexArray(boxVAO);
  glBindBuffer(GL_ARRAY_BUFFER,boxVBO);
  glBufferData(GL_ARRAY_BUFFER,sizeof(float)*6*2,NULL,GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),0);
  glBindBuffer(GL_ARRAY_BUFFER,0);
  glBindVertexArray(0);
  glError("Arrays and buffers for box");

  double oldMouseX = 0.0;
  double oldMouseY = 0.0;

  double mouseX = resX/2.0;
  double mouseY = resX/2.0;

  bool moving = false;

  double avgCollisionsPerFrame = 0.0;

  bool placingAttractor = false;
  bool placingRepellor = false;
  bool pause = false;

  while (window.isOpen()){

    sf::Event event;
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

      if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::A){
        if (placingAttractor){
          placingAttractor = false;
        }
        else{
          placingRepellor = false;
          placingAttractor = true;
        }
      }

      if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R){
        if (placingRepellor){
          placingRepellor = false;
        }
        else{
          placingAttractor = false;
          placingRepellor = true;
        }
      }

      if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space){
        pause = !pause;
      }

      if (event.type == sf::Event::MouseWheelScrolled){
        mouseX = event.mouseWheelScroll.x;
        mouseY = event.mouseWheelScroll.y;
        double z = event.mouseWheelScroll.delta;

        camera.incrementZoom(z);
      }

      if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Middle){
        mouseX = event.mouseButton.x;
        mouseY = event.mouseButton.y;

        glm::vec4 worldPos = camera.screenToWorld(mouseX,mouseY);

        camera.setPosition(worldPos.x,worldPos.y);
      }

      if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left){
        sf::Vector2i pos = sf::Mouse::getPosition(window);
        // multiply by inverse of current projection
        glm::vec4 worldPos = camera.screenToWorld(pos.x,pos.y);

        if(!deleteAttratorRepellor(
          worldPos.x,
          worldPos.y,
          attractors,
          repellors,
          r*0.1
        )){
          if (placingRepellor && repellors.size() < maxRepellors){
            repellors.push_back(glm::vec2(worldPos.x,worldPos.y));
          }
          else if (placingAttractor && attractors.size() < maxAttractors){
            attractors.push_back(glm::vec2(worldPos.x,worldPos.y));
          }
        }
      }

      if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left && !(placingRepellor||placingAttractor)){
        sf::Vector2i pos = sf::Mouse::getPosition(window);
        oldMouseX = pos.x;
        oldMouseY = pos.y;
      }

      if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left && !(placingRepellor||placingAttractor)){
        sf::Vector2i pos = sf::Mouse::getPosition(window);

        // multiply by inverse of current projection
        glm::vec4 worldPosA = camera.screenToWorld(oldMouseX,oldMouseY);

        glm::vec4 worldPosB = camera.screenToWorld(pos.x,pos.y);

        glm::vec4 worldPosDelta = worldPosB-worldPosA;

        camera.move(worldPosDelta.x,worldPosDelta.y);
      }

    }

    window.clear(sf::Color::White);
    glClearColor(1.0f,1.0f,1.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    avgCollisionsPerFrame = 0.0;

    physClock.restart();
    if (!pause){
      for (int s = 0; s < subSample; s++){
        uint64_t nc = step(
          X,
          Xp,
          F,
          noise,
          list,
          cells,
          offsets,
          thetas,
          attractors,
          repellors,
          generator,
          N,
          Nc,
          delta,
          L,
          r,
          v0,
          Dr,
          k,
          drag,
          rotDrag,
          M,
          J,
          attractionStrength,
          repellingStrength,
          dt
        );
        avgCollisionsPerFrame += nc;
      }
    }
    physDeltas[frameId] = physClock.getElapsedTime().asSeconds();
    avgCollisionsPerFrame /= double(subSample);

    renderClock.restart();

    glUseProgram(shader);

    proj = camera.getVP();

    glUniformMatrix4fv(
      glGetUniformLocation(shader,"proj"),
      1,
      GL_FALSE,
      &proj[0][0]
    );

    glUniform1f(
      glGetUniformLocation(shader,"zoom"),
      camera.getZoomLevel()
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

      float cameraX = camera.getPosition().x;
      float cameraY = camera.getPosition().y;

      debugText << "Particles: " << N <<
        "\n" <<
        "Delta: " << fixedLengthNumber(delta,6) <<
        " (FPS: " << fixedLengthNumber(1.0/delta,4) << ")" <<
        "\n" <<
        "Render/Physics: " << fixedLengthNumber(renderDelta,6) << "/" << fixedLengthNumber(physDelta,6) <<
        "\n" <<
        "Mouse (" << fixedLengthNumber(mouse.x,4) << "," << fixedLengthNumber(mouse.y,4) << ")" <<
        "\n" <<
        "Camera [world] (" << fixedLengthNumber(cameraX,4) << ", " << fixedLengthNumber(cameraY,4) << ")" <<
        "\n" <<
        "Collision/Frame: " << fixedLengthNumber(avgCollisionsPerFrame,6) <<
        "\n";

      textRenderer.renderText(
        OD,
        debugText.str(),
        64.0f,resY-64.0f,
        0.5f,
        glm::vec3(0.0f,0.0f,0.0f)
      );
    }

    if (placingRepellor || placingAttractor){
      glUseProgram(boxShader);

      glUniformMatrix4fv(
        glGetUniformLocation(boxShader,"proj"),
        1,
        GL_FALSE,
        &defaultProj[0][0]
      );

      glUniform3f(
        glGetUniformLocation(boxShader,"colour"),
        1.0,1.0,1.0
      );


      float height = 64.0/resY;
      float width = 1.25;
      float offsetWidth = 64.0/resX;
      float offsetHeight = 0.0;

      glBindVertexArray(boxVAO);
      glBindBuffer(GL_ARRAY_BUFFER,boxVBO);

      float verts[6*2] = {
        -1.0f+offsetWidth,          -1.0f+offsetHeight,
        -1.0f+offsetWidth+width,    -1.0f+offsetHeight,
        -1.0f+offsetWidth+width,    -1.0f+offsetHeight+ height,
        -1.0f+offsetWidth,          -1.0f+offsetHeight,
        -1.0f+offsetWidth,          -1.0f+offsetHeight+ height,
        -1.0f+offsetWidth+width,    -1.0f+offsetHeight+ height
      };

      glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(verts),verts);
      glDrawArrays(GL_TRIANGLES,0,6);
      glBindVertexArray(0);
      glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (placingRepellor){
      textRenderer.renderText(
        OD,
        "Placeing repellor (R) to cancel",
        64.0f,8.0f,
        0.5f,
        glm::vec3(1.0f,0.0f,0.0f)
      );
    }

    if (placingAttractor){
      textRenderer.renderText(
        OD,
        "Placeing attractor (A) to cancel",
        64.0f,8.0f,
        0.5f,
        glm::vec3(0.0f,1.0f,0.0f)
      );
    }


    atrepUniforms(
      atrepShader,
      attractors,
      repellors,
      maxAttractors,
      maxRepellors,
      proj,
      wellSize*resX,
      camera.getZoomLevel(),
      float(frameId),
      60.0,
      maxRepellors+maxAttractors
    );

    glBindBuffer(GL_ARRAY_BUFFER,atRepOffsetVBO);
    glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(float)*(maxRepellors+maxAttractors),&atRepOffsets[0]);
    glBindBuffer(GL_ARRAY_BUFFER,0);

    glBindVertexArray(atRepVAO);
    glDrawArraysInstanced(GL_POINTS,0,1,(maxRepellors+maxAttractors));
    glBindVertexArray(0);

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

  glDeleteBuffers(1,&vertVBO);
  glDeleteBuffers(1,&offsetVBO);
  glDeleteBuffers(1,&thetaVBO);

  glDeleteVertexArrays(1,&vertVAO);

  glDeleteProgram(shader);
  return 0;
}
