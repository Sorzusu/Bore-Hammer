#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>

// Camera & Movement logic
glm::vec3 cameraPos   = glm::vec3(0.0f, 1.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f, pitch = 0.0f;
float lastX = 640, lastY = 360;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float timer = 0.0f; // For bobbing

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    lastX = xpos; lastY = ypos;

    float sensitivity = 0.1f;
    yaw += xoffset * sensitivity;
    pitch += yoffset * sensitivity;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

const char *vShader = "#version 330 core\nlayout (location = 0) in vec3 aPos; uniform mat4 mvp; void main() { gl_Position = mvp * vec4(aPos, 1.0); }";
const char *fShader = "#version 330 core\nout vec4 FragColor; uniform vec3 color; void main() { FragColor = vec4(color, 1.0); }";

int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(1280, 720, "BORE HAMMER", NULL, NULL);
    glfwMakeContextCurrent(window);
    glewInit();

    // i3 Fixes
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glEnable(GL_DEPTH_TEST);

    // Geometry Data
    float terrain[] = { -50,0,-50, 50,0,-50, 50,0,50, -50,0,-50, 50,0,50, -50,0,50 };
    float hand[] = { -0.1f,-0.1f,-0.4f, 0.1f,-0.1f,-0.4f, 0.1f,0.1f,-0.4f, -0.1f,-0.1f,-0.4f, 0.1f,0.1f,-0.4f, -0.1f,0.1f,-0.4f };

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);

    unsigned int vs = glCreateShader(GL_VERTEX_SHADER); glShaderSource(vs, 1, &vShader, NULL); glCompileShader(vs);
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fs, 1, &fShader, NULL); glCompileShader(fs);
    unsigned int prog = glCreateProgram(); glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);
    glUseProgram(prog);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input & Bobbing Logic
        float speed = 4.0f * deltaTime;
        bool moving = false;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) { cameraPos += speed * cameraFront; moving = true; }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) { cameraPos -= speed * cameraFront; moving = true; }
        
        float bobAmount = 0.0f;
        if (moving) {
            timer += deltaTime * 10.0f;
            bobAmount = sin(timer) * 0.05f; // Vertical bob
        }

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int w, h; glfwGetFramebufferSize(window, &w, &h); glViewport(0, 0, w, h);
        glm::mat4 proj = glm::perspective(glm::radians(90.0f), (float)w/(float)h, 0.1f, 100.0f);
        
        // Apply Bobbing to View Matrix
        glm::vec3 eye = cameraPos + glm::vec3(0, bobAmount, 0);
        glm::mat4 view = glm::lookAt(eye, eye + cameraFront, cameraUp);

        // Draw Terrain
        glUniformMatrix4fv(glGetUniformLocation(prog, "mvp"), 1, GL_FALSE, glm::value_ptr(proj * view));
        glUniform3f(glGetUniformLocation(prog, "color"), 0.2f, 0.2f, 0.2f);
        glBufferData(GL_ARRAY_BUFFER, sizeof(terrain), terrain, GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Draw Hand (Relative to camera with slight sway)
        glm::mat4 model = glm::translate(glm::mat4(1.0f), eye + (cameraFront * 0.5f) + (glm::normalize(glm::cross(cameraFront, cameraUp)) * 0.3f) + glm::vec3(0, -0.2f, 0));
        glUniformMatrix4fv(glGetUniformLocation(prog, "mvp"), 1, GL_FALSE, glm::value_ptr(proj * view * model));
        glUniform3f(glGetUniformLocation(prog, "color"), 0.5f, 0.0f, 0.0f);
        glBufferData(GL_ARRAY_BUFFER, sizeof(hand), hand, GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Draw Crosshair (Simple Point in Center of Screen)
        glUseProgram(0); // Use fixed function for the quick dot
        glPointSize(5.0f);
        glBegin(GL_POINTS);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex2f(0.0f, 0.0f); 
        glEnd();
        glUseProgram(prog);

        glfwSwapBuffers(window);
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    }
    glfwTerminate();
    return 0;
}
