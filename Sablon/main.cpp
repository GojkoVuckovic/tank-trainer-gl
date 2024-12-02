#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#define CRES 100 
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <thread>
#include <chrono>
#include <string>
#include <filesystem>
#include <GL/glew.h>  
#include <GLFW/glfw3.h>
#include <ft2build.h> 
#include FT_FREETYPE_H
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

unsigned int compileShader(GLenum type, const char* source);
unsigned int createShader(const char* vsSource, const char* fsSource);
void makeIndicator(unsigned int& vao, unsigned int& vbo, unsigned int shader, const float red, const float green, const float blue,float r,float centerX,float centerY);

struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;

float elapsedTime = 60.0f;
float lastFrameTime = 0.0f;

void RenderText(unsigned int shader, std::string text, float x, float y, float scale, glm::vec3 color, unsigned int vao, unsigned int vbo)
{
    // activate corresponding render state	
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
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
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void makeIndicator(unsigned int& vao, unsigned int& vbo, unsigned int shader, const float red, const float green, const float blue, float r,float centerX,float centerY)
{
    float circle[(CRES + 2) * 5];

    circle[0] = centerX;
    circle[1] = centerY; 
    circle[2] = red;
    circle[3] = green;
    circle[4] = blue;

    for (int i = 0; i <= CRES; i++)
    {
        circle[5 + i * 5] = r * cos((3.141592 / 180) * (i * 360 / CRES)) + centerX;
        circle[6 + i * 5] = r * sin((3.141592 / 180) * (i * 360 / CRES)) + centerY;
        circle[7 + i * 5] = red;
        circle[8 + i * 5] = green;
        circle[9 + i * 5] = blue;
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(circle), circle, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void makeOutlineCircle(unsigned int& vao, unsigned int& vbo, unsigned int shader, const float red, const float green, const float blue)
{
    float circle[(CRES + 1) * 5];
    float r = 0.625f;

    for (int i = 0; i <= CRES; i++)
    {
        float angle = (i * 360.0f / CRES) * (3.141592f / 180.0f);
        circle[i * 5] = r * cos(angle);  
        circle[i * 5 + 1] = r * sin(angle) -0.375f;
        circle[i * 5 + 2] = red;     
        circle[i * 5 + 3] = green;          
        circle[i * 5 + 4] = blue;      
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(circle), circle, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void drawAmmoIndicator(unsigned int& vao, unsigned int& vbo, unsigned int shader, int ammo) {
    float blockWidth = 0.05f;
    float blockHeight = 0.02f;

    // 2 triangles per block, 6 indices per block
    float ammoIndicator[10 * 6 * 5]; // 10 blocks, each consisting of 2 triangles (6 vertices)

    for (int i = 0; i < 10; ++i) {
        float xPos = -0.95f + i * (blockWidth + 0.01f);
        float colorR = (i < ammo) ? 0.0f : 0.5f;
        float colorG = (i < ammo) ? 1.0f : 0.0f;
        float colorB = 0.0f;

        ammoIndicator[i * 30 + 0] = xPos;                // Bottom-left x
        ammoIndicator[i * 30 + 1] = 0.85f;               // Bottom-left y
        ammoIndicator[i * 30 + 2] = colorR;              // Bottom-left r
        ammoIndicator[i * 30 + 3] = colorG;              // Bottom-left g
        ammoIndicator[i * 30 + 4] = colorB;              // Bottom-left b

        ammoIndicator[i * 30 + 5] = xPos + blockWidth;   // Bottom-right x
        ammoIndicator[i * 30 + 6] = 0.85f;               // Bottom-right y
        ammoIndicator[i * 30 + 7] = colorR;              // Bottom-right r
        ammoIndicator[i * 30 + 8] = colorG;              // Bottom-right g
        ammoIndicator[i * 30 + 9] = colorB;              // Bottom-right b

        ammoIndicator[i * 30 + 10] = xPos;               // Top-left x
        ammoIndicator[i * 30 + 11] = 0.85f + blockHeight; // Top-left y
        ammoIndicator[i * 30 + 12] = colorR;             // Top-left r
        ammoIndicator[i * 30 + 13] = colorG;             // Top-left g
        ammoIndicator[i * 30 + 14] = colorB;             // Top-left b

        ammoIndicator[i * 30 + 15] = xPos + blockWidth;  // Bottom-right x
        ammoIndicator[i * 30 + 16] = 0.85f;              // Bottom-right y
        ammoIndicator[i * 30 + 17] = colorR;             // Bottom-right r
        ammoIndicator[i * 30 + 18] = colorG;             // Bottom-right g
        ammoIndicator[i * 30 + 19] = colorB;             // Bottom-right b

        ammoIndicator[i * 30 + 20] = xPos + blockWidth;  // Top-right x
        ammoIndicator[i * 30 + 21] = 0.85f + blockHeight; // Top-right y
        ammoIndicator[i * 30 + 22] = colorR;             // Top-right r
        ammoIndicator[i * 30 + 23] = colorG;             // Top-right g
        ammoIndicator[i * 30 + 24] = colorB;             // Top-right b

        ammoIndicator[i * 30 + 25] = xPos;               // Top-left x
        ammoIndicator[i * 30 + 26] = 0.85f + blockHeight; // Top-left y
        ammoIndicator[i * 30 + 27] = colorR;             // Top-left r
        ammoIndicator[i * 30 + 28] = colorG;             // Top-left g
        ammoIndicator[i * 30 + 29] = colorB;             // Top-left b
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ammoIndicator), ammoIndicator, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float))); // Color
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void drawVoltmeterNeedle(unsigned int& vao, unsigned int& vbo, unsigned int shader, float voltage, bool hydraulics) {

    float needle[10];
    float length = 0.5f; 

   
    float angle = M_PI - (voltage*1000 * M_PI);

    if (hydraulics) {
        angle += ((rand() % 5 - 2) * 0.01f);
    }

    needle[0] = 0.0f;  
    needle[1] = 0.0f; 
    needle[2] = 0.0f;   
    needle[3] = 0.0f; 
    needle[4] = 0.0f;  

    needle[5] = length * cos(angle);
    needle[6] = length * sin(angle);
    needle[7] = 1.0f;       
    needle[8] = 0.0f;          
    needle[9] = 0.0f;            

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(needle), needle, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void drawVoltmeterDial(unsigned int& vao, unsigned int& vbo, unsigned int shader) {
    float dial[(CRES + 2) * 5];
    float radius = 0.6f;

    dial[0] = 0.0f;   
    dial[1] = 0.0f;  
    dial[2] = 0.9f;   
    dial[3] = 0.9f;
    dial[4] = 0.9f;

    for (int i = 0; i <= CRES; i++) {
        dial[5 + i * 5] = radius * cos((2 * M_PI / CRES) * i);
        dial[6 + i * 5] = radius * sin((2 * M_PI / CRES) * i);
        dial[7 + i * 5] = 0.8f;
        dial[8 + i * 5] = 0.8f;
        dial[9 + i * 5] = 0.8f;
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(dial), dial, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static unsigned loadImageToTexture(const char* filePath,int channels) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels,channels);
    if (ImageData != NULL)
    {
       
        stbi__vertical_flip(ImageData, TextureWidth, TextureHeight, TextureChannels);

       
        GLint InternalFormat = -1;
        switch (TextureChannels) {
        case 1: InternalFormat = GL_RED; break;
        case 2: InternalFormat = GL_RG; break;
        case 3: InternalFormat = GL_RGB; break;
        case 4: InternalFormat = GL_RGBA; break;
        default: InternalFormat = GL_RGB; break;
        }

        unsigned int Texture;
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, TextureWidth, TextureHeight, 0, InternalFormat, GL_UNSIGNED_BYTE, ImageData);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(ImageData);
        return Texture;
    }
    else
    {
        std::cout << "Textura nije ucitana! Putanja texture: " << filePath << std::endl;
        stbi_image_free(ImageData);
        return 0;
    }
}

std::pair<unsigned, unsigned> AddTexture(unsigned int& vao, unsigned int& vbo, unsigned int shader) {

    float vertices[] = {
        // Positions   // TexCoords1   // TexCoords2
        -1.0f,  1.0f,   0.0f, 1.0f,    0.0f, 1.0f,
         1.0f,  1.0f,   1.0f, 1.0f,    1.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 0.0f,    1.0f, 0.0f,
        -1.0f, -1.0f,   0.0f, 0.0f,    0.0f, 0.0f
    };

    unsigned int stride = 6 * sizeof(float);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0); 

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
     
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float))); 
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0);

    unsigned texture1 = loadImageToTexture("res/titelski_breg.jpg",0); //Ucitavamo teksturu
    glBindTexture(GL_TEXTURE_2D, texture1); //Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); //Generisemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//S = U = X    GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);// T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);   //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);
     
    unsigned texture2 = loadImageToTexture("res/tank_tube.png",4); 
    glBindTexture(GL_TEXTURE_2D, texture2); 
    glGenerateMipmap(GL_TEXTURE_2D); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(shader);
    unsigned uTexLoc = glGetUniformLocation(shader, "uTex");
    glUniform1i(uTexLoc, 0);
    int uTex1Loc = glGetUniformLocation(shader, "uTex1"); 
    glUniform1i(uTex1Loc, 1); 

    glUseProgram(0);
    return std::make_pair(texture1, texture2);
}

float verticesX[] = {

        -0.01f, -0.01f, 0.0f,
        0.01f, 0.01f, 0.0f,

        -0.01f, 0.01f, 0.0f,
        0.01f, -0.01f, 0.0f,
};

void makeX(unsigned int& vao, unsigned int& vbo, unsigned int shader) {
    

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesX), verticesX, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void moveX(float& xPos, float& yPos, float cursorX, float cursorY, float speed) {
    float dirX = cursorX - xPos;
    float dirY = cursorY - yPos;

    float distance = std::sqrt(dirX * dirX + dirY * dirY);

    if (distance > 0.001f) { // Only move if the distance is noticeable
        // Normalize direction vector
        dirX /= distance;
        dirY /= distance;

        // Move towards the cursor
        float newXPos = xPos + dirX * speed;
        float newYPos = yPos + dirY * speed;

        // Check if the new position is outside the elliptical boundary
        float ellipseDistance = (newXPos * newXPos) / (0.625f * 0.625f) + (newYPos * newYPos) / (1.6f * 1.6f * 0.625f * 0.625f);

        if (ellipseDistance <= 1.0f) {
            xPos = newXPos;
            yPos = newYPos;
        }
        
    }
}


void updateX(float xPos, float yPos) {

    verticesX[0] = -0.01f + xPos; // Update the starting point of the first line
    verticesX[1] = -0.01f + yPos;

    verticesX[3] = 0.01f + xPos; // Update the ending point of the first line
    verticesX[4] = 0.01f + yPos;

    verticesX[6] = -0.01f + xPos; // Update the starting point of the second line
    verticesX[7] = 0.01f + yPos;

    verticesX[9] = 0.01f + xPos; // Update the ending point of the second line
    verticesX[10] = -0.01f + yPos;
}

float lineVertices[] = {
    0.0f, 0.0f, 0.0f,  
    0.0f, 0.0f, 0.0f   
};

void makeLine(unsigned int& vao, unsigned int& vbo, unsigned int shader) {

    glBindVertexArray(vao); 

    glBindBuffer(GL_ARRAY_BUFFER, vbo); 
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_DYNAMIC_DRAW); 

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}




int main(void)
{
    srand(static_cast<unsigned>(time(0)));
    if (!glfwInit()) 
    {
        std::cout<<"GLFW Biblioteka se nije ucitala! :(\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    unsigned int wWidth = 1920;
    unsigned int wHeight = 1200;
    const char wTitle[] = "[Tenk trener]";
    window = glfwCreateWindow(wWidth, wHeight, wTitle, glfwGetPrimaryMonitor(), nullptr);
  
    if (window == NULL)
    {
        std::cout << "Prozor nije napravljen! :(\n";
        glfwTerminate();
        return 2;
    }

    glfwMakeContextCurrent(window); 

    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW nije mogao da se ucita! :'(\n";
        return 3;
    }

    unsigned int shader = createShader("basic.vert", "basic.frag");
    unsigned int backgroundTextureShader = createShader("background_texture.vert","background_texture.frag");
    unsigned int textShader = createShader("text.vert", "text.frag");

    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    std::string font_name = std::filesystem::absolute("res/fonts/digital-7.ttf").string();
    if (font_name.empty())
    {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
        return -1;
    }

    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return -1;
    }
    else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++)
        {
            // Load character glyph 
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
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
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
            Characters.insert(std::pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    unsigned int VAO[13], VBO[13];
    glGenVertexArrays(13, VAO);
    glGenBuffers(13, VBO);

    glBindVertexArray(VAO[12]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[12]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);



    glClearColor(0.47f, 0.53f, 0.42f, 1.0);

    bool ready = true;
    int ammo = 10;
    bool outside = false;
    float voltage = 0.0f;
    bool hydraulic = false;
    float turretSpeed = 1.0f;
    bool buttonLag = true;
    float textureOffsetX = 0.0f;
    float maxOffset = 0.2f; // Maximum scrolling amount (since the texture is 1.0 wide, we can scroll up to 20% on either side)
    float minOffset = -0.2f;
    float xPos = 0.0f;
    float yPos = 0.0f;
    float messageDisplayedTime = 0.0f;


    std::pair<unsigned, unsigned> textures = AddTexture(VAO[5], VBO[5], backgroundTextureShader);
    unsigned texture1 = textures.first;
    unsigned texture2 = textures.second;

    float targetCoords[] = {
        0.0f,0.0f,
        0.0f,0.0f,
        0.0f,0.0f
    };

    bool targetHit[] = {
        false,
        false,
        false
    };

    for (int i = 0; i < 3; i++) {
        float randomX = (rand() / (float)RAND_MAX) * 2.4f - 1.2f;
        float randomY = (rand() % 3200 - 1600) / 1600.0f;
        targetCoords[i*2] = randomX;
        targetCoords[(i*2)+1] = randomY;
    }

    glfwSwapInterval(1);
    glEnable(GL_BLEND); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

    while (!glfwWindowShouldClose(window)) //Beskonacna petlja iz koje izlazimo tek kada prozor treba da se zatvori
    {

        //Clearing of screen
        glClearColor(0.47f, 0.53f, 0.42f, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
         
        
       

        //Cursor position
        double cursorX, cursorY; 
        glfwGetCursorPos(window, &cursorX, &cursorY);

        cursorX = (cursorX / 960) - 1.0; 
        cursorY = -(cursorY / 600) + 1.0;

        //Escape key
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);

        //Going outside
        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS)
            outside = true;

        //Going inside
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
            outside = false;

        

        //Inside drawing
        if (!outside) {

            //Shooting indicator
            if (ready) {
                makeIndicator(VAO[0], VBO[0], shader, 1.0, 1.0, 0.0,0.25f,0,0);
            }
            else {
                makeIndicator(VAO[0], VBO[0], shader, 0.0, 0.0, 0.0,0.25f,0,0);
            }

            glViewport(wWidth/16, 0, wWidth / 4, (wHeight / 4) * 1.6);
            glUseProgram(shader);
            glBindVertexArray(VAO[0]);
            glDrawArrays(GL_TRIANGLE_FAN, 0, CRES + 2);

            //Ammo indicator
            drawAmmoIndicator(VAO[1], VBO[1], shader, ammo);
            glViewport(wWidth/16, 0, wWidth, wHeight / 3);
            glUseProgram(shader);
            glBindVertexArray(VAO[1]);
            glDrawArrays(GL_TRIANGLES, 0, 60);
            glBindVertexArray(0);

            if (hydraulic) {
                turretSpeed = voltage; 
            }
            else {
                turretSpeed = voltage / 10.0f;
            }

            //Voltmeter dial
            drawVoltmeterDial(VAO[3], VBO[3], shader);
            glViewport(wWidth / 2, wHeight / 16, wWidth / 4, (wHeight / 4)*1.6);
            glUseProgram(shader);
            glBindVertexArray(VAO[3]);
            glDrawArrays(GL_TRIANGLE_FAN, 0, CRES + 2);
            glBindVertexArray(0);

            //Voltmeter needle
            drawVoltmeterNeedle(VAO[2], VBO[2], shader, voltage,hydraulic); 
            glViewport(wWidth / 2, wHeight/16, wWidth / 4, (wHeight / 4)*1.6); 
            glUseProgram(shader); 
            glBindVertexArray(VAO[2]);
            glDrawArrays(GL_LINES, 0, 2); 
            glBindVertexArray(0);

            if (hydraulic) {
                makeIndicator(VAO[4], VBO[4], shader, 0.0, 1.0, 0.0,0.25f,0,0);
            }
            else {
                makeIndicator(VAO[4], VBO[4], shader, 1.0, 0.0, 0.0,0.25f,0,0); 
            }
            glViewport(wWidth*2/3, wHeight/16, wWidth / 8, (wHeight / 8) * 1.6);
            glUseProgram(shader);
            glBindVertexArray(VAO[4]);
            glDrawArrays(GL_TRIANGLE_FAN, 0, CRES + 2);

            
            //Hydraulic toggle
            if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
                hydraulic = !hydraulic; 
                buttonLag = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                buttonLag = true;
            }

            //Voltage up and down
            if (hydraulic) {
                if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
                    buttonLag = false;
                    voltage = fmin(voltage + 0.00002f, 0.001f);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    buttonLag = true;
                }
                if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) { 
                    voltage = fmax(voltage - 0.00002f, 0.0f); 
                    buttonLag = false;
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    buttonLag = true;
                }
            }
        }
        else {


            glfwSetCursor(window, glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR));


            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                textureOffsetX += 0.001f;
                if (textureOffsetX > maxOffset) textureOffsetX = maxOffset;
                else {
                    for (int i = 0; i < 3; i++) {
                        targetCoords[i * 2] -= 0.004f;
                    }
                }
            }
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                textureOffsetX -= 0.001f;
                if (textureOffsetX < minOffset) textureOffsetX = minOffset;
                else {
                    for (int i = 0; i < 3; i++) {
                        targetCoords[i * 2] += 0.004f;
                    }
                }
            }

            glViewport(0, 0, wWidth, wHeight);
            glUseProgram(backgroundTextureShader);
            glBindVertexArray(VAO[5]);
            glUniform1f(glGetUniformLocation(backgroundTextureShader, "uOffsetX"), textureOffsetX);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texture2);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glBindVertexArray(0);

            for (int i = 0; i < 3; i++) {
                
                if (!targetHit[i]) {
                    makeIndicator(VAO[9 + i], VBO[9 + i], shader, 1.0, 1.0, 0.0, 0.1f, targetCoords[(i * 2)], targetCoords[(i * 2) + 1]);
                    glUseProgram(shader);
                    glBindVertexArray(VAO[9 + i]);
                    glDrawArrays(GL_TRIANGLE_FAN, 0, CRES + 2);
                    glUseProgram(0);
                }
            }

            glViewport(0, 0, wWidth, wHeight*1.6);
            makeOutlineCircle(VAO[6], VBO[6], shader, 0, 0, 0);
            glUseProgram(shader); 
            glBindVertexArray(VAO[6]);
            glDrawArrays(GL_LINE_LOOP, 0, CRES + 1);

            moveX(xPos, yPos, cursorX, cursorY, turretSpeed);
            updateX(xPos, yPos);

            glViewport(0, 0, wWidth, wHeight);
            makeX(VAO[7], VBO[7], shader);
            glBindVertexArray(VAO[7]);
            glDrawArrays(GL_LINES, 0, 4);
            glBindVertexArray(0);

            lineVertices[3] = cursorX;
            lineVertices[4] = cursorY;
            lineVertices[0] = xPos;
            lineVertices[1] = yPos;

            makeLine(VAO[8], VBO[8], shader);
            glBindBuffer(GL_ARRAY_BUFFER, VBO[8]);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineVertices), lineVertices);
            glBindVertexArray(VAO[8]);
            glDrawArrays(GL_LINES, 0, 2); 
            glBindVertexArray(0);

            //Shooting with left click
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && outside) {
                if (ready && ammo > 0) {
                    ammo--;
                    ready = false;
                    std::thread([&]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(7500));
                        ready = true;
                        }).detach();
                    for (int i = 0; i < 3; i++) {
                        float centerX = targetCoords[i * 2];
                        float centerY = targetCoords[i * 2 + 1];
                        float dx = xPos - centerX;
                        float dy = yPos - centerY;
                        if ((dx * dx + dy * dy) <= (0.1f * 0.1f)) {
                            targetHit[i] = true;
                            break;
                        }
                    }
                }
            }
        }

        float currentFrameTime = glfwGetTime();
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        elapsedTime -= deltaTime;

        glViewport(0, 0, 1920, 1200);
        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(1920), 0.0f, static_cast<float>(1200));
        glUseProgram(textShader);
        glUniformMatrix4fv(glGetUniformLocation(textShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        if (targetHit[0] == true && targetHit[1] == true && targetHit[2] == true) {
            RenderText(textShader, "Uspesna misija", 850.0f, 1100.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f), VAO[12], VBO[12]);
            if (messageDisplayedTime == 0.0f) {
                messageDisplayedTime = glfwGetTime(); 
            }
            if (glfwGetTime() - messageDisplayedTime >= 3.0f) {
                glfwSetWindowShouldClose(window, GL_TRUE);
            }
        }
        else {
            if (elapsedTime >= 0.0f) {
                RenderText(textShader, std::to_string(elapsedTime), 850.0f, 1100.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f), VAO[12], VBO[12]);
            }
            else {
                RenderText(textShader, "Neuspijeh", 850.0f, 1100.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f), VAO[12], VBO[12]);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    glDeleteBuffers(9, VBO);
    glDeleteVertexArrays(9, VAO);
    glDeleteProgram(shader);
    glDeleteProgram(backgroundTextureShader);

    glfwTerminate();
    return 0;
}


unsigned int compileShader(GLenum type, const char* source)
{
    //Uzima kod u fajlu na putanji "source", kompajlira ga i vraca sejder tipa "type"
    //Citanje izvornog koda iz fajla
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
     const char* sourceCode = temp.c_str(); //Izvorni kod sejdera koji citamo iz fajla na putanji "source"

    int shader = glCreateShader(type); //Napravimo prazan sejder odredjenog tipa (vertex ili fragment)
    
    int success; //Da li je kompajliranje bilo uspjesno (1 - da)
    char infoLog[512]; //Poruka o gresci (Objasnjava sta je puklo unutar sejdera)
    glShaderSource(shader, 1, &sourceCode, NULL); //Postavi izvorni kod sejdera
    glCompileShader(shader); //Kompajliraj sejder

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); //Provjeri da li je sejder uspjesno kompajliran
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog); //Pribavi poruku o gresci
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
    //Pravi objedinjeni sejder program koji se sastoji od Vertex sejdera ciji je kod na putanji vsSource

    unsigned int program; //Objedinjeni sejder
    unsigned int vertexShader; //Verteks sejder (za prostorne podatke)
    unsigned int fragmentShader; //Fragment sejder (za boje, teksture itd)

    program = glCreateProgram(); //Napravi prazan objedinjeni sejder program

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource); //Napravi i kompajliraj vertex sejder
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource); //Napravi i kompajliraj fragment sejder

    //Zakaci verteks i fragment sejdere za objedinjeni program
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program); //Povezi ih u jedan objedinjeni sejder program
    glValidateProgram(program); //Izvrsi provjeru novopecenog programa

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success); //Slicno kao za sejdere
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
        std::cout << "Objedinjeni sejder ima gresku! Greska: \n";
        std::cout << infoLog << std::endl;
    }

    //Posto su kodovi sejdera u objedinjenom sejderu, oni pojedinacni programi nam ne trebaju, pa ih brisemo zarad ustede na memoriji
    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}
