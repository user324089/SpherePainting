#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>

constexpr unsigned int windowSide = 800;
constexpr unsigned int updateParts = 10;

struct simpleVertexData {
    float coordinates3d [3];
    float coordinates2d [2];
};

template <typename vertexType, unsigned int vertCount>
class vertexPositions {
    public:
    private:
        GLuint VBO = 0, VAO = 0;
        void init () {
            glCreateBuffers (1, &VBO);
            glGenVertexArrays (1, &VAO);
        }
    public:
        vertexPositions () {
            init();
        }
        ~vertexPositions () {
            glDeleteBuffers (1, &VBO);
            glDeleteVertexArrays (1, &VAO);
        }
        vertexPositions (const vertexPositions& r) = delete;

        void sendData (const vertexType* data) {
            glNamedBufferData (VBO, vertCount * sizeof(vertexType), data, GL_STATIC_DRAW);
        }

        vertexPositions (const vertexType (& data) [vertCount] ) {
            init();
            sendData (data);
        }
        const vertexPositions& operator = (const vertexPositions& r) = delete;
        void attribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, unsigned int offset) {

            glBindVertexArray (VAO);
                glBindBuffer (GL_ARRAY_BUFFER, VBO);
                glVertexAttribPointer (index, size, type, normalized, sizeof (vertexType), reinterpret_cast<void*>(offset));
                glEnableVertexAttribArray (index);
                glBindBuffer (GL_ARRAY_BUFFER, 0);
            glBindVertexArray (0);
        }
        void draw (GLenum mode) const {
            glBindVertexArray (VAO);
                glDrawArrays (mode, 0, vertCount);
            glBindVertexArray (0);
        }
        GLuint getVAO () {return VAO;}
        GLuint getVBO () {return VBO;}
};

class shader {
    private:
        GLuint name = 0;
        const GLenum type;
    public:
        shader (GLenum _type) : type (_type) {
            name = glCreateShader (type);
        }
        shader (const shader & r) = delete;
        const shader& operator = (const shader & r) = delete;
        shader (shader && r) : name (r.name), type (r.type) {
            r.name = 0;
        }
        ~shader () {
            glDeleteShader (name);
        }
        void make (GLsizei count, const GLchar *const *string) {
            glShaderSource (name, count, string, nullptr);
            glCompileShader (name);

            GLint success;
            glGetShaderiv (name, GL_COMPILE_STATUS, &success);
            if (success != GL_TRUE) {
                GLchar buffer [512];
                glGetShaderInfoLog (name, 512, nullptr, buffer);
                throw std::runtime_error (std::string ("shader compilation error:\n") + buffer);
            }
        }
        GLuint getName () const {return name;}
};

class program {
    private:
        GLuint name;
    public:
        program () : name (glCreateProgram()) {}
        program (const program & r) = delete;
        const program & operator = (const program & r) = delete;
        program (program && r) : name (r.name) {
            r.name = 0;
        }
        ~program () {
            glDeleteProgram (name);
        }
        template <typename... T>
        void make (const T&... shaders) {
            (... , glAttachShader (name, shaders.getName()));
            glLinkProgram (name);
            (... , glDetachShader (name, shaders.getName()));

            GLint success;
            glGetProgramiv (name, GL_LINK_STATUS, &success);

            if (success != GL_TRUE) {
                GLchar buffer [512];
                glGetProgramInfoLog (name, 512, nullptr, buffer);
                throw std::runtime_error (std::string ("program linking error:\n") + buffer);
            }
        }
        GLuint getName () const {return name;}
};


class simpleTexture2D {
    private:
        GLuint name = 0;
        int width = 0, height = 0;
        void initialize () {
            glGenTextures (1, &name);
        }
    public:
        simpleTexture2D () {
            initialize();
        }
        simpleTexture2D (const simpleTexture2D & r) = delete;
        const simpleTexture2D & operator = (const simpleTexture2D & r) = delete;
        simpleTexture2D (simpleTexture2D && r) : name (r.name), width (r.width), height (r.height){
            r.initialize();
        }
        ~simpleTexture2D () {
            glDeleteTextures (1, &name);
        }
        void makeFromDimensions (int _width, int _height) {
            width = _width;
            height = _height;

            glBindTexture (GL_TEXTURE_2D, name);
                glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                glClearTexImage (name, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glBindTexture (GL_TEXTURE_2D, 0);
        }
        void genMipmaps () {
            glBindTexture (GL_TEXTURE_2D, name);
                glGenerateMipmap (GL_TEXTURE_2D);
            glBindTexture (GL_TEXTURE_2D, 0);
        }
        GLuint getName () const {return name;}
        int getWidth () const {return width;}
        int getHeight () const {return height;}
};

class simpleSampler {
    private:
        GLuint name = 0;
        void initialize () {
            glGenSamplers (1, &name);
            glSamplerParameteri (name, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glSamplerParameteri (name, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glSamplerParameteri (name, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glSamplerParameteri (name, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glSamplerParameteri (name, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
    public:
        simpleSampler () {
            initialize();
        }
        simpleSampler (const simpleSampler & r) = delete;
        const simpleSampler & operator = (const simpleSampler & r) = delete;
        simpleSampler (simpleSampler && r) : name (r.name) {
            r.initialize();
        }
        ~simpleSampler () {
            glDeleteSamplers (1, &name);
        }
        void changeParameter (GLenum parameter, GLint value) {
            glSamplerParameteri (name, parameter, value);
        }

        GLuint getName () {return name;}
};

class frameBuffer {
    private:
        GLuint name = 0;
        void init () {
            glGenFramebuffers (1, &name);
        }
    public:
        frameBuffer () {
            init();
        }
        frameBuffer (const frameBuffer & r) = delete;
        const frameBuffer & operator = (const frameBuffer & r) = delete;
        frameBuffer (frameBuffer && r) : name (r.name) {
            r.init();
        }
        GLuint getName () {return name;}
};

const GLchar* vertexShaderSource = R"(
#version 460
layout (location = 0) in vec3 position3d;
layout (location = 1) in vec2 position2d;
layout (std140, binding=0) uniform uniMat {
    mat4 mat;
};
out vec2 fragPositionLocal;
out vec2 fragPositionScreen;
void main () {
    fragPositionScreen = (mat * vec4 (position3d, 1.0)).xy;
    fragPositionLocal = position2d;
    gl_Position = mat * vec4 (position3d, 1.0);
}
)";

const GLchar* fragmentShaderSource = R"(
#version 460
in vec2 fragPositionLocal;
out vec4 color;
layout (std140, binding=0) uniform uniMat {
    mat4 mat;
};
uniform sampler2D sideTexture;
void main () {
    color = texture (sideTexture, fragPositionLocal*0.5 + vec2(0.5,0.5));
}
)";

const GLchar* vertexShaderSource2 = R"(
#version 460
layout (location = 0) in vec3 position3d;
layout (location = 1) in vec2 position2d;
layout (std140, binding=0) uniform uniMat {
    mat4 mat;
};
out vec2 fragPositionLocal;
out vec3 fragPosition;
void main () {
    vec4 totalPos = mat * vec4 (position3d, 1.0);
    fragPosition = totalPos.xyz;
    fragPositionLocal = position2d;
    //gl_Position = mat * vec4 (position, 1.0);
    gl_Position = vec4 (fragPositionLocal, 0.0, 1.0);
}
)";


const GLchar* fragmentShaderSource2 = R"(
#version 460
out vec4 color;
in vec3 fragPosition;
in vec2 fragPositionLocal;
void main () {
    if (length (fragPosition.xy) < 0.01 && fragPosition.z > 0) {
        color = vec4 (1.0,1.0,1.0,1.0);
    } else {
        discard;
    }
}
)";

class buffer {
    private:
        GLuint name = 0;
    public:
        buffer () {
            glCreateBuffers (1, &name);
        }
        buffer (const buffer&) = delete;
        buffer& operator = (const buffer&) = delete;
        ~buffer () {
            glDeleteBuffers (1, &name);
        }
        GLuint getName () {
            return name;
        }
        void sendData (GLsizeiptr size, const void *data, GLenum usage) {
            glNamedBufferData (name, size, data, usage);
        }
};

class renderer {
    private:

        constexpr static unsigned int textureSides = 1000;

        program pr, pr2;
        vertexPositions<simpleVertexData, 4> sim [6];
        constexpr static simpleVertexData simPositions [6][4] = {
            {{{-1.0f, -1.0f,  1.0f},{-1.0f, -1.0f}}, //FRONT
             {{ 1.0f, -1.0f,  1.0f},{ 1.0f, -1.0f}},
             {{-1.0f,  1.0f,  1.0f},{-1.0f,  1.0f}},
             {{ 1.0f,  1.0f,  1.0f},{ 1.0f,  1.0f}}},

            {{{-1.0f, -1.0f, -1.0f},{-1.0f, -1.0f}}, // LEFT
             {{-1.0f, -1.0f,  1.0f},{ 1.0f, -1.0f}},
             {{-1.0f,  1.0f, -1.0f},{-1.0f,  1.0f}},
             {{-1.0f,  1.0f,  1.0f},{ 1.0f,  1.0f}}},

            {{{ 1.0f,  1.0f,  1.0f},{-1.0f, -1.0f}}, // RIGHT
             {{ 1.0f, -1.0f,  1.0f},{ 1.0f, -1.0f}},
             {{ 1.0f,  1.0f, -1.0f},{-1.0f,  1.0f}},
             {{ 1.0f, -1.0f, -1.0f},{ 1.0f,  1.0f}}},

            {{{ 1.0f, -1.0f, -1.0f},{-1.0f, -1.0f}}, // BACK
             {{-1.0f, -1.0f, -1.0f},{ 1.0f, -1.0f}},
             {{ 1.0f,  1.0f, -1.0f},{-1.0f,  1.0f}},
             {{-1.0f,  1.0f, -1.0f},{ 1.0f,  1.0f}}},

            {{{-1.0f,  1.0f, -1.0f},{-1.0f, -1.0f}}, // TOP
             {{-1.0f,  1.0f,  1.0f},{ 1.0f, -1.0f}},
             {{ 1.0f,  1.0f, -1.0f},{-1.0f,  1.0f}},
             {{ 1.0f,  1.0f,  1.0f},{ 1.0f,  1.0f}}},

            {{{-1.0f, -1.0f,  1.0f},{-1.0f, -1.0f}}, // BOTTOM
             {{-1.0f, -1.0f, -1.0f},{ 1.0f, -1.0f}},
             {{ 1.0f, -1.0f,  1.0f},{-1.0f,  1.0f}},
             {{ 1.0f, -1.0f, -1.0f},{ 1.0f,  1.0f}}},
        };
        buffer matrixUniformBuffer {};
        glm::mat4 perspectiveMatrix = glm::perspective (1.0f, 1.0f, 0.0f, 10.0f);
        glm::mat4 rotationMatrix {1.0f};
        frameBuffer paintingFrameBuffer {};
        simpleTexture2D sideTextures [6];
        simpleSampler textureSampler {};
    public:
        renderer () : pr (), pr2 () {

            for (unsigned int i = 0; i < 6; i++) {
                sim[i].sendData (simPositions[i]);

                sim[i].attribPointer (0, 3, GL_FLOAT, GL_FALSE, offsetof (simpleVertexData, coordinates3d));
                sim[i].attribPointer (1, 2, GL_FLOAT, GL_FALSE, offsetof (simpleVertexData, coordinates2d));
            }

            shader vs (GL_VERTEX_SHADER);
            shader fs (GL_FRAGMENT_SHADER);

            vs.make (1, &vertexShaderSource);
            fs.make (1, &fragmentShaderSource);

            pr.make (vs, fs);


            shader vs2 (GL_VERTEX_SHADER);
            shader fs2 (GL_FRAGMENT_SHADER);

            vs2.make (1, &vertexShaderSource2);
            fs2.make (1, &fragmentShaderSource2);

            pr2.make (vs2, fs2);

            for (int i = 0; i < 6; i++) {
                sideTextures[i].makeFromDimensions (textureSides, textureSides);
                //sideTextures->loadFromFile ("pepe.png");
            }
            textureSampler.changeParameter (GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        }
        renderer (const renderer &) = delete;
        renderer & operator = (const renderer&) = delete;

        void paint () {
            glm::mat4 totalMat = perspectiveMatrix * rotationMatrix;
            matrixUniformBuffer.sendData (16*sizeof (GLfloat), glm::value_ptr (totalMat), GL_STREAM_DRAW);

            glViewport (0,0,textureSides, textureSides);
            glBindFramebuffer (GL_DRAW_FRAMEBUFFER, paintingFrameBuffer.getName());

                glBindBufferBase (GL_UNIFORM_BUFFER, 0, matrixUniformBuffer.getName());
                glUseProgram (pr2.getName());

                for (unsigned int i = 0; i < 6; i++) {
                    glFramebufferTexture (GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, sideTextures[i].getName(), 0);

                    sim[i].draw (GL_TRIANGLE_STRIP);
                }

            glBindFramebuffer (GL_DRAW_FRAMEBUFFER, 0);
            glBindBufferBase (GL_UNIFORM_BUFFER, 0, 0);

        }

        void update (int horizontalMove, int verticalMove, bool isPainting, double deltaTime) {
            if (horizontalMove == 0 && verticalMove == 0) {
                if (isPainting) {
                    paint ();
                }
                return;
            }
            glm::vec3 axis (verticalMove, -horizontalMove, 0.0f);
            rotationMatrix = glm::rotate (glm::mat4 {1.0}, static_cast<float>(deltaTime), axis) * rotationMatrix;
            if (isPainting) {
                paint ();
            }
        }
        void draw () {
            glm::mat4 totalMat = perspectiveMatrix * rotationMatrix;
            matrixUniformBuffer.sendData (16*sizeof (GLfloat), glm::value_ptr (totalMat), GL_STREAM_DRAW);
            glUseProgram (pr.getName());
            glBindBufferBase (GL_UNIFORM_BUFFER, 0, matrixUniformBuffer.getName());

            glBindSampler (0, textureSampler.getName());
            
            for (unsigned int i = 0; i < 6; i++) {
                glBindTextureUnit (0, sideTextures[i].getName());
                sim[i].draw (GL_TRIANGLE_STRIP);
            }

            glBindBufferBase (GL_UNIFORM_BUFFER, 0, 0);
        }
};

double totalX = 0, totalY = 0;

double getDeltaTime (double & prevCurrentTime) {
    double currentTime = glfwGetTime();
    double deltaTime = currentTime - prevCurrentTime;
    prevCurrentTime = currentTime;
    return deltaTime;
}

int main () {
    glfwInit ();

    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow * window = glfwCreateWindow (windowSide, windowSide, "app", nullptr, nullptr);

    glfwMakeContextCurrent (window);

    //glfwSetWindowPos (window, 100, 100);

    //glfwSetInputMode (window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //glfwSetCursorPosCallback (window, cursorPosCallback);

    glewExperimental = true;
    glewInit ();

    //glEnable (GL_BLEND);
    //glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable (GL_CULL_FACE);

    glViewport (0,0,windowSide,windowSide);

    renderer rend;

    double currentTime = glfwGetTime();

    while (!glfwWindowShouldClose (window)) {


        glfwPollEvents();

        glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
        glClear (GL_COLOR_BUFFER_BIT);

        bool wPressed = glfwGetKey (window, GLFW_KEY_W);
        bool aPressed = glfwGetKey (window, GLFW_KEY_A);
        bool sPressed = glfwGetKey (window, GLFW_KEY_S);
        bool dPressed = glfwGetKey (window, GLFW_KEY_D);

        int verticalMove = 0;
        int horizontalMove = 0;

        if (wPressed) verticalMove++;
        if (sPressed) verticalMove--;
        if (dPressed) horizontalMove++;
        if (aPressed) horizontalMove--;

        double deltaTime = getDeltaTime (currentTime);
        double updateDeltaTime = deltaTime / updateParts;

        bool isPainting = glfwGetKey (window, GLFW_KEY_SPACE);
        for (unsigned int i = 0; i < updateParts; i++) {
            rend.update (horizontalMove, verticalMove, isPainting, updateDeltaTime);
        }
        int frameBufferWidth, frameBufferHeight;
        glfwGetFramebufferSize (window, &frameBufferWidth, &frameBufferHeight);
        glViewport (0,0, frameBufferWidth, frameBufferHeight);
        rend.draw();

        glfwSwapBuffers (window);
    }

    glfwTerminate();
}
