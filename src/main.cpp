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

#include <ParticleSystem/particleSystem.cpp>
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
// motion parameters

const int maxAttractors = 8;
const int maxRepellors = 8;

// for smoothing delta numbers
uint8_t frameId = 0;
double deltas[60];
double physDeltas[60];
double renderDeltas[60];

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

  ParticleSystem particles(N);

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

  OrthoCam camera(resX,resY,glm::vec2(0.0,0.0));

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
  double mouseY = resY/2.0;

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

        if(!particles.deleteAttratorRepellor(worldPos.x,worldPos.y)){
          if (placingRepellor && particles.nRepellers() < maxRepellors){
            particles.addRepeller(worldPos.x,worldPos.y);
          }
          else if (placingAttractor && particles.nAttractors() < maxAttractors){
            particles.addAttractor(worldPos.x,worldPos.y);
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
      particles.step();
    }
    physDeltas[frameId] = physClock.getElapsedTime().asSeconds();
    avgCollisionsPerFrame /= double(subSample);

    renderClock.restart();

    glm::mat4 proj = camera.getVP();

    particles.setProjection(proj);
    particles.draw(
      frameId,
      camera.getZoomLevel(),
      resX,
      resY
    );

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
