#ifndef TYPE_H
#define TYPE_H

const char * vertexShader = "#version 330 core\n"
  "layout(location=0) in vec4 postex;\n"
  "out vec2 texCoords;\n"
  "uniform mat4 proj;\n"
  "void main(){\n"
  " gl_Position = proj*vec4(postex.xy,0.0,1.0);\n"
  " texCoords = postex.zw;\n"
  "}";

const char * fragmentShader = "#version 330 core\n"
  "in vec2 texCoords; out vec4 colour;\n"
  "uniform sampler2D glyph;\n"
  "uniform vec3 textColour;\n"
  "void main(){\n"
  " vec4 sample = vec4(1.0,1.0,1.0,texture(glyph,texCoords).r);\n"
  " colour = vec4(textColour,1.0)*sample;\n"
  "}";

struct FreeType {
  FreeType(FT_Library l, FT_Face f)
  : lib(l), face(f) {}
  const FT_Library lib;
  const FT_Face face;
};

struct Glyph {
  Glyph(){}
  Glyph(GLuint u, glm::ivec2 s, glm::ivec2 b, uint64_t o)
  : textureID(u), size(s), bearing(b), offset(o) {}
  GLuint textureID;
  glm::ivec2 size;
  glm::ivec2 bearing;
  uint64_t offset;
};

typedef std::map<char,Glyph> GlyphMap;

FreeType initFreetype(int width = 48){
  FT_Library ftLib;
  if (FT_Init_FreeType(&ftLib)){
    std::cout << "Could not init FreeType\n";
  }
  FT_Face ftFace;
  if (FT_New_Face(ftLib,"resources/fonts/OpenDyslexic-Regular.otf",0,&ftFace)){
    std::cout << "Could not load font\n";
  }

  FT_Set_Pixel_Sizes(ftFace,0,width); //dynamic width for height 48

  return FreeType(ftLib,ftFace);
}

void loadASCIIGlyphs(const FT_Face & face, GlyphMap & g){
  for (unsigned char c = 0; c < 128; c++/*ayy lmao*/){
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)){
      std::cout << "Failed to load glyph " << c << " from ASCII charset\n";
      continue;
    }
    GLuint tex;
    glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D,tex);
    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RED,
      face->glyph->bitmap.width,
      face->glyph->bitmap.rows,
      0,
      GL_RED,
      GL_UNSIGNED_BYTE,
      face->glyph->bitmap.buffer
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    Glyph glyph(
      tex,
      glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
      glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
      face->glyph->advance.x
    );
    glError("Loading Glyph: "+std::to_string(c)+" :");
    g.insert(std::pair<char, Glyph>(c,glyph));
  }
}

void renderText(
  GlyphMap g,
  GLuint & shader,
  std::string text,
  float x,
  float y,
  float scale,
  glm::vec3 colour,
  GLuint & VAO,
  GLuint & VBO){
    glUniform3f(glGetUniformLocation(shader, "textColour"), colour.x, colour.y, colour.z);
    glError("Setting textColour uniform for type: ");
    glUniform1i(glGetUniformLocation(shader, "glyph"), 0);
    glError("Setting texture id uniform for type: ");
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    float initalX = x;

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++){
        Glyph ch = g[*c];

        if (*c == '\n'){
          y -= 32.0f;
          x = initalX;
          continue;
        }

        float xpos = x + ch.bearing.x * scale;
        float ypos = y - (ch.size.y - ch.bearing.y) * scale;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glError("Setting data for Glyph texture " + std::to_string(*c) + " :");
        glBufferStatus("Setting data for Glyph texture "+ std::to_string(*c) + " :");
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.offset >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
#endif
