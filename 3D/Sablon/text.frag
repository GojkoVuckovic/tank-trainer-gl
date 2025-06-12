#version 330 core
in vec2 TexCoords;

out vec4 color;

uniform sampler2D text; // Tekstura font atlasa
uniform vec3 textColor;  // Boja teksta

void main()
{
    // Uzorkuj boju iz glifa teksture (koja je jednokanalna R)
    // Zatim pomnozi sa zeljenom bojom teksta
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    color = vec4(textColor, 1.0) * sampled;
}