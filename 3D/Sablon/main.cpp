#define _CRT_SECURE_NO_WARNINGS
 
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <GL/glew.h> 
#include <GLFW/glfw3.h>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>

float g_plantScale = 1.0f;
float g_cameraYaw = 0.0f;
bool g_row1Visible = true;
bool g_row2Visible = true;
bool g_row3Visible = true;
bool g_isPerspective = true;

unsigned int g_wWidth = 1920;
unsigned int g_wHeight = 1080;

unsigned int compileShader(GLenum type, const char* source);
unsigned int createShader(const char* vsSource, const char* fsSource);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

struct Character {
    unsigned int TextureID; 
    glm::ivec2   Size;      
    glm::ivec2   Bearing;  
    unsigned int Advance;
};

std::map<GLchar, Character> Characters;
unsigned int textVAO_FT, textVBO_FT;
unsigned int textShader_FT;
void initFreeType();
void RenderText(std::string text, float x, float y, float scale, glm::vec3 color);

//initializing of freetype
void initFreeType() {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }

    FT_Face face;
    if (FT_New_Face(ft, "res/fonts/digital-7.ttf", 0, &face)) {
        std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);

    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 0; c < 128; c++) { 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "ERROR::FREETYPE: Failed to load Glyph: " << c << std::endl;
            continue;
        }
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
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

        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glGenVertexArrays(1, &textVAO_FT);
    glGenBuffers(1, &textVBO_FT);
    glBindVertexArray(textVAO_FT);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO_FT);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    textShader_FT = createShader("text.vert", "text.frag");
}

//text rendering function
void RenderText(std::string text, float x, float y, float scale, glm::vec3 color) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glUseProgram(textShader_FT);
    glUniform3f(glGetUniformLocation(textShader_FT, "textColor"), color.x, color.y, color.z);

    glm::mat4 projection2D = glm::ortho(0.0f, (float)g_wWidth, 0.0f, (float)g_wHeight);
    glUniformMatrix4fv(glGetUniformLocation(textShader_FT, "projection"), 1, GL_FALSE, glm::value_ptr(projection2D));

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO_FT);

    std::string::const_iterator it;
    for (it = text.begin(); it != text.end(); it++) {
        Character ch = Characters[*it];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO_FT);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

      
        x += (ch.Advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND); 
    glEnable(GL_DEPTH_TEST); 

}

glm::vec3 g_groundColor = glm::vec3(0.3f, 0.6f, 0.2f); 
glm::vec3 g_row1Color = glm::vec3(0.0f, 0.4f, 0.1f); 
glm::vec3 g_row2Color = glm::vec3(0.5f, 0.0f, 0.0f);
glm::vec3 g_row3Color = glm::vec3(0.0f, 0.2f, 0.5f);


int main(void)
{

   
    if (!glfwInit())
    {
        std::cout<<"GLFW Biblioteka se nije ucitala! :(\n";
        return 1;
    }


    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    const char wTitle[] = "3D Basta";
    window = glfwCreateWindow(g_wWidth, g_wHeight, wTitle, glfwGetPrimaryMonitor(), NULL);
    
    if (window == NULL)
    {
        std::cout << "Prozor nije napravljen! :(\n";
        glfwTerminate();
        return 2;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    
    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW nije mogao da se ucita! :'(\n";
        return 3;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE); 
    glCullFace(GL_BACK);    
    glFrontFace(GL_CCW);

    initFreeType();

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ PROMJENLJIVE I BAFERI +++++++++++++++++++++++++++++++++++++++++++++++++

    unsigned int unifiedShader = createShader("basic.vert", "basic.frag");
    unsigned int objectColorLoc = glGetUniformLocation(unifiedShader, "uObjectColor");
    

    float planeVertices[] = {
        -0.5f,  0.0f, -0.5f,
         0.5f,  0.0f, -0.5f,
        -0.5f,  0.0f,  0.5f,
         0.5f,  0.0f,  0.5f,
    };
    
    unsigned int planeIndices[] = {
       0, 1, 2,
       1, 3, 2 
    };

    unsigned int planeStride = 3 * sizeof(float);

    unsigned int groundVAO, groundVBO, groundEBO;
    glGenVertexArrays(1, &groundVAO);
    glGenBuffers(1, &groundVBO);
    glGenBuffers(1, &groundEBO);

    glBindVertexArray(groundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, groundEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, planeStride, (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    float plantPlaneVertices[] = {
        -0.5f,  0.0f, 0.0f,
         0.5f,  0.0f, 0.0f,
        -0.5f,  1.0f, 0.0f,
         0.5f,  1.0f, 0.0f,
    };
    unsigned int plantStride = 3 * sizeof(float);

    unsigned int plantVAO, plantVBO, plantEBO;
    glGenVertexArrays(1, &plantVAO);
    glGenBuffers(1, &plantVBO);
    glGenBuffers(1, &plantEBO);

    glBindVertexArray(plantVAO);
    glBindBuffer(GL_ARRAY_BUFFER, plantVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plantPlaneVertices), plantPlaneVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plantEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, plantStride, (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);


    std::vector<glm::vec3> row1Positions = {
        glm::vec3(-3.0f, 0.0f, -2.0f), glm::vec3(-1.5f, 0.0f, -2.0f), glm::vec3(0.0f, 0.0f, -2.0f), glm::vec3(1.5f, 0.0f, -2.0f), glm::vec3(3.0f, 0.0f, -2.0f)
    };
    std::vector<glm::vec3> row2Positions = {
        glm::vec3(-3.0f, 0.0f, 0.0f), glm::vec3(-1.5f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.5f, 0.0f, 0.0f), glm::vec3(3.0f, 0.0f, 0.0f)
    };
    std::vector<glm::vec3> row3Positions = {
        glm::vec3(-3.0f, 0.0f, 2.0f), glm::vec3(-1.5f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(1.5f, 0.0f, 2.0f), glm::vec3(3.0f, 0.0f, 2.0f)
    };

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++            UNIFORME            +++++++++++++++++++++++++++++++++++++++++++++++++

    unsigned int modelLoc = glGetUniformLocation(unifiedShader, "uM");
    unsigned int viewLoc = glGetUniformLocation(unifiedShader, "uV");
    unsigned int projectionLoc = glGetUniformLocation(unifiedShader, "uP");


    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ RENDER LOOP - PETLJA ZA CRTANJE +++++++++++++++++++++++++++++++++++++++++++++++++
    glUseProgram(unifiedShader);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    glClearColor(0.5, 0.5, 0.5, 1.0);
    glCullFace(GL_BACK);
    glfwSwapInterval(1);


    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(unifiedShader);
        glEnable(GL_DEPTH_TEST);

        glm::vec3 cameraPos = glm::vec3(0.0f, 6.0f, 8.0f); // Pocetna pozicija kamere
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f); // Centar baste
        glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

        // Rotacija kamere oko centra baste
        glm::mat4 cameraRotation = glm::rotate(glm::mat4(1.0f), glm::radians(g_cameraYaw), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec4 rotatedCameraPos = cameraRotation * glm::vec4(cameraPos, 1.0f);
        glm::mat4 view = glm::lookAt(glm::vec3(rotatedCameraPos), cameraTarget, cameraUp);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // Projection matrica
        glm::mat4 projection;
        if (g_isPerspective) {
            projection = glm::perspective(glm::radians(45.0f), (float)g_wWidth / (float)g_wHeight, 0.1f, 100.0f);
        }
        else {
            // Ortogonalna projekcija
            float orthoSize = 5.0f;
            projection = glm::ortho(-orthoSize * ((float)g_wWidth / (float)g_wHeight), orthoSize * ((float)g_wWidth / (float)g_wHeight), -orthoSize, orthoSize, 0.1f, 100.0f);
        }
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Crtanje tla
        glBindVertexArray(groundVAO);
        glUniform3f(objectColorLoc, g_groundColor.x, g_groundColor.y, g_groundColor.z);
        glm::mat4 groundModel = glm::mat4(1.0f);
        groundModel = glm::scale(groundModel, glm::vec3(500.0f, 1.0f, 500.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(groundModel));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Crtanje biljaka
        glBindVertexArray(plantVAO);

        // Lambda funkcija za crtanje reda biljaka
        auto drawPlantRow = [&](const std::vector<glm::vec3>& positions, bool visible,glm::vec3 rowColor) {
            glUniform3f(objectColorLoc, rowColor.x, rowColor.y, rowColor.z);
            if (visible) {
                for (const auto& pos : positions) {
                    glm::mat4 plantModel1 = glm::mat4(1.0f);
                    plantModel1 = glm::translate(plantModel1, pos);
                    plantModel1 = glm::scale(plantModel1, glm::vec3(1.0f, g_plantScale, 1.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(plantModel1));
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    glm::mat4 plantModel2 = glm::mat4(1.0f);
                    plantModel2 = glm::translate(plantModel2, pos);
                    plantModel2 = glm::rotate(plantModel2, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    plantModel2 = glm::scale(plantModel2, glm::vec3(1.0f, g_plantScale, 1.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(plantModel2));
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }
            }
            };

        drawPlantRow(row1Positions, g_row1Visible,g_row1Color);
        drawPlantRow(row2Positions, g_row2Visible,g_row2Color);
        drawPlantRow(row3Positions, g_row3Visible,g_row3Color);

        float textScale = 0.5f;
        std::string name = "Gojko Vuckovic SV49/2021";
        float textWidth = 0.0f;
        for (char c : name) {
            Character ch = Characters[c];
            textWidth += (ch.Advance >> 6) * textScale;
        }
        RenderText(name, g_wWidth - textWidth - 20.0f, g_wHeight - 20.0f - Characters['H'].Size.y * textScale, textScale, glm::vec3(1.0f, 1.0f, 1.0f)); // Beli tekst

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ POSPREMANJE +++++++++++++++++++++++++++++++++++++++++++++++++


    glDeleteBuffers(1, &groundVBO);
    glDeleteBuffers(1, &groundEBO);
    glDeleteVertexArrays(1, &groundVAO);

    glDeleteBuffers(1, &plantVBO);
    glDeleteBuffers(1, &plantEBO);
    glDeleteVertexArrays(1, &plantVAO);

    glDeleteProgram(unifiedShader);

    glDeleteBuffers(1, &textVBO_FT);
    glDeleteVertexArrays(1, &textVAO_FT);
    glDeleteProgram(textShader_FT);
    for (auto const& [key, val] : Characters) {
        glDeleteTextures(1, &Characters[key].TextureID);
    }
    Characters.clear();

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    g_wWidth = width;
    g_wHeight = height;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        g_plantScale += 0.01f;
        if (g_plantScale > 5.0f) g_plantScale = 5.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        g_plantScale -= 0.01f;
        if (g_plantScale < 0.1f) g_plantScale = 0.1f;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        g_cameraYaw -= 0.5f;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        g_cameraYaw += 0.5f;
    }
    static bool key1Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && !key1Pressed) {
        g_row1Visible = !g_row1Visible;
        key1Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE) {
        key1Pressed = false;
    }

    static bool key2Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && !key2Pressed) {
        g_row2Visible = !g_row2Visible;
        key2Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE) {
        key2Pressed = false;
    }

    static bool key3Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && !key3Pressed) {
        g_row3Visible = !g_row3Visible;
        key3Pressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE) {
        key3Pressed = false;
    }
    static bool keyOPressed = false;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && !keyOPressed) {
        g_isPerspective = false;
        keyOPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_O) == GLFW_RELEASE) {
        keyOPressed = false;
    }

    static bool keyPPressed = false;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !keyPPressed) {
        g_isPerspective = true;
        keyPPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        keyPPressed = false;
    }
}

unsigned int compileShader(GLenum type, const char* source)
{
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
        std::cout << "Uspjesno procitao fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        ss << "";
        std::cout << "Greska pri citanju fajla sa putanje \"" << source << "\"!" << std::endl;
    }
     std::string temp = ss.str();
     const char* sourceCode = temp.c_str();

    int shader = glCreateShader(type);
    
    int success;
    char infoLog[512];
    glShaderSource(shader, 1, &sourceCode, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf(infoLog);
    }
    return shader;
}
unsigned int createShader(const char* vsSource, const char* fsSource)
{
    unsigned int program;
    unsigned int vertexShader;
    unsigned int fragmentShader;

    program = glCreateProgram();

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource);
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource);

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);
    glValidateProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
        std::cout << "Objedinjeni sejder ima gresku! Greska: \n";
        std::cout << infoLog << std::endl;
    }

    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}
