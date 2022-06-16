#ifndef GLUTILS_H
#define GLUTILS_H

// print buffer status errors
GLuint glBufferStatus(const std::string c = ""){
    GLuint e = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (e != GL_FRAMEBUFFER_COMPLETE){std::cout << c;}
    switch(e){
        case GL_FRAMEBUFFER_UNDEFINED:
        std::cout << "GLERROR: GL_FRAMEBUFFER_UNDEFINED\n";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            std::cout << "GLERROR: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n";
            break;

        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            std::cout << "GLERROR: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n";
            break;

        case GL_FRAMEBUFFER_UNSUPPORTED:
            std::cout << "GLERROR: GL_FRAMEBUFFER_UNSUPPORTED\n";
            break;

        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            std::cout << "GLERROR: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\n";
            break;
    }
    return e;
}

// print gl error codes
GLuint glError(const std::string c = ""){
    GLuint e = glGetError();
    if (e != GL_NO_ERROR){std::cout << c;}
    switch(e){
        case GL_NO_ERROR:
         break;
        case GL_INVALID_ENUM:
            std::cout << "GL_INVALID_ENUM\n";
            break;
        case GL_INVALID_VALUE:
            std::cout << "GLERROR: GL_INVALID_VALUE\n";
            break;
        case GL_INVALID_OPERATION:
            std::cout << "GLERROR: GL_INVALID_OPERATION\n";
            break;
        case GL_OUT_OF_MEMORY:
            std::cout << "GLERROR: GL_OUT_OF_MEMORY\n";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            std::cout << "GLERROR: GL_INVALID_FRAMEBUFFER_OPERATION\n";
            break;
    }
    return e;
}

// compile a gl shader given a program and source code as const char *
void compileShader(GLuint & shaderProgram, const char * vert, const char * frag){

    GLuint vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader,1,&vert,NULL);
    glCompileShader(vertexShader);

    // check it worked!
    int  success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

    if(!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader,1,&frag,NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

    if(!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    glAttachShader(shaderProgram,vertexShader);
    glAttachShader(shaderProgram,fragmentShader);
    glLinkProgram(shaderProgram);

    // check it linked
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::LINK::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
}


#endif
