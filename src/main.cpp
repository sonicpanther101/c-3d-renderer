#include <iostream>
#include <GL/glew.h> // always ensure glew is included before glfw or your in the run for a billion errors
#include <GLFW/glfw3.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLenum err = glewInit();
	if (err != GLEW_OK) {
		std::cout << "Failed to initialize GLEW: %s" << glewGetErrorString(err) << std::endl;
	}

    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glViewport(0, 0, 800, 600);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glewInit();

    int nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    std::cout << "Maximum nr of vertex attributes supported: " << nrAttributes << std::endl;

    float vertices[] = {
        0.5f,  0.5f, 0.0f,  // top right
        1.0f, -0.5f, 0.0f,  // bottom right
        0.0f, -0.5f, 0.0f   // centre
    };

    float vertices1[] = {
        0.0f, -0.5f, 0.0f,  // centre
       -1.0f, -0.5f, 0.0f,  // bottom left
       -0.5f,  0.5f, 0.0f   // top left 
    };

    unsigned int VAO, VAO1, VBO, VBO1;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO1);
    glGenBuffers(1, &VBO1);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); 

    glBindVertexArray(VAO1);

    glBindBuffer(GL_ARRAY_BUFFER, VBO1);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices1), vertices1, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); 

    const char *vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "void main() {\n"
        "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "}\0";

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int  success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

    if(!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    const char *fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"

        "void main() {\n"
            "FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
        "}\0";
    const char *fragmentShaderSource1 = "#version 330 core\n"
        "out vec4 FragColor;\n"

        "void main() {\n"
            "FragColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);\n"
        "}\0";

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int fragmentShader1;
    fragmentShader1 = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader1, 1, &fragmentShaderSource1, NULL);
    glCompileShader(fragmentShader1);

    unsigned int shaderProgram, shaderProgram1;
    shaderProgram = glCreateProgram();
    shaderProgram1 = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glAttachShader(shaderProgram1, vertexShader);
    glAttachShader(shaderProgram1, fragmentShader1);
    glLinkProgram(shaderProgram1);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    }

    glUseProgram(shaderProgram1);
    glUseProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader); 
    glDeleteShader(fragmentShader1); 

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // GL_FILL or GL_LINE for normal or wireframe

    while(!glfwWindowShouldClose(window)) {

        processInput(window);

        // solid colour
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glBindVertexArray(VAO);
        
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glUseProgram(shaderProgram1);

        glBindVertexArray(VAO1);
        
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();    
    }
  
    glfwTerminate();
    return 0;
}