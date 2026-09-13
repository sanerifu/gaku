#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static APIENTRY void debugCallback(
    GLenum source,
    GLenum type,
    unsigned int id,
    GLenum severity,
    GLsizei length,
    char const* message,
    void const* userParam
);
static GLuint createProgram(char const* source);
static char const* SHADER;

typedef struct InstanceData {
    uint16_t x;
    uint16_t y;
} InstanceData;

static uint16_t makeUshort(float v) {
    if (v < 0.0f) {
        v = 0.0f;
    } else if (v > 1.0f) {
        v = 1.0f;
    }
    return (uint16_t)roundf(v * 65535.0f);
}

int main() {
    GLFWwindow* window;
    GLuint vao;
    GLuint program;
    GLuint instance_buffer;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
    window = glfwCreateWindow(800, 600, "Gaku Test", NULL, NULL);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)&glfwGetProcAddress);

    InstanceData sample_data[] = {
        (InstanceData){.x = makeUshort(32.0f / 800.0f), .y = makeUshort(32.0f / 600.0f)},
    };

    /* Init */
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(&debugCallback, NULL);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);

        glGenBuffers(1, &instance_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, instance_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(sample_data), sample_data, GL_DYNAMIC_DRAW);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,
            2,
            GL_UNSIGNED_SHORT,
            GL_TRUE,
            sizeof(InstanceData),
            (void const*)(offsetof(InstanceData, x))
        );
        glVertexAttribDivisor(0, 1);
        glBindVertexArray(0);

        program = createProgram(SHADER);
    }

    while (!glfwWindowShouldClose(window)) {
        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /* Render */
        {
            glUseProgram(program);
            glBindVertexArray(vao);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, sizeof(sample_data) / sizeof(sample_data[0]));
        }

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

static char const* VERTEX_PRELUDE;
static char const* FRAGMENT_PRELUDE;
static GLuint createShader(GLenum type, char const* prelude, char const* source);

static GLuint createProgram(char const* source) {
    GLuint program = glCreateProgram();
    GLuint vertex_shader = createShader(GL_VERTEX_SHADER, VERTEX_PRELUDE, source);
    GLuint fragment_shader = createShader(GL_FRAGMENT_SHADER, FRAGMENT_PRELUDE, source);
    GLint status;

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint log_length;
        char* log;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        log = calloc(log_length, sizeof(char));
        glGetProgramInfoLog(program, log_length, &log_length, log);
        fprintf(stderr, "Link error:\n%.*s\n", (int)log_length, log);
        abort();
    }

    return program;
}

static GLuint createShader(GLenum type, char const* prelude, char const* source) {
    GLuint shader = glCreateShader(type);
    char const* sources[2];
    GLint status;

    sources[0] = prelude;
    sources[1] = source;

    glShaderSource(shader, 2, (GLchar const* const*)sources, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint log_length = 0;
        char* log;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        log = calloc(log_length, sizeof(char));
        glGetShaderInfoLog(shader, log_length, &log_length, log);
        fprintf(stderr, "Compile error:\n%.*s\n", (int)log_length, log);
        abort();
    }

    return shader;
}

#define STRINGIFY(x) #x
#define EVAL(x) STRINGIFY(x)
#define LINE_TO_STRING EVAL(__LINE__)

static const char* SHADER = "#line " LINE_TO_STRING
                            "\n"
                            "varying vec2 vTex;\n"
                            "#if VERTEX\n"
                            "in vec2 aPosition;\n"
                            "void main() {\n"
                            "vec2 uv = vec2(float((gl_VertexID & 2) >> 1), float(1 - (gl_VertexID & 1)));\n"
                            "vTex = uv;\n"
                            "gl_Position = vec4(2.0f * (uv * aPosition) - 1.0f, 0.0f, 1.0f);\n"
                            "}\n"
                            "#endif\n"
                            "#if FRAGMENT\n"
                            "out vec4 oColor;\n"
                            "void main() {\n"
                            "oColor = vec4(vTex, 0.0f, 1.0f);\n"
                            "}\n"
                            "#endif\n";
static const char* VERTEX_PRELUDE =
    "#version 460 core\n"
    "#line " LINE_TO_STRING
    "\n"
    "#define varying out\n"
    "#define VERTEX 1\n"
    "#define FRAGMENT 0\n";
static const char* FRAGMENT_PRELUDE =
    "#version 460 core\n"
    "#line " LINE_TO_STRING
    "\n"
    "#define varying in\n"
    "#define FRAGMENT 1\n"
    "#define VERTEX 0\n";

static APIENTRY void debugCallback(
    GLenum source,
    GLenum type,
    unsigned int id,
    GLenum severity,
    GLsizei length,
    char const* message,
    void const* userParam
) {
    (void)length;
    (void)userParam;
    /* ignore non-significant error/warning codes */
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
        return;

    fprintf(stderr, "Debug message (%d): %s\n", id, message);

    switch (source) {
        case GL_DEBUG_SOURCE_API:
            fprintf(stderr, "Source: API\n");
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            fprintf(stderr, "Source: Window System\n");
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            fprintf(stderr, "Source: Shader Compiler\n");
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            fprintf(stderr, "Source: Third Party\n");
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            fprintf(stderr, "Source: Application\n");
            break;
        case GL_DEBUG_SOURCE_OTHER:
            fprintf(stderr, "Source: Other\n");
            break;
    }

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            fprintf(stderr, "Type: Error\n");
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            fprintf(stderr, "Type: Deprecated Behaviour\n");
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            fprintf(stderr, "Type: Undefined Behaviour\n");
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            fprintf(stderr, "Type: Portability\n");
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            fprintf(stderr, "Type: Performance\n");
            break;
        case GL_DEBUG_TYPE_MARKER:
            fprintf(stderr, "Type: Marker\n");
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            fprintf(stderr, "Type: Push Group\n");
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            fprintf(stderr, "Type: Pop Group\n");
            break;
        case GL_DEBUG_TYPE_OTHER:
            fprintf(stderr, "Type: Other\n");
            break;
    }

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            fprintf(stderr, "Severity: high\n");
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            fprintf(stderr, "Severity: medium\n");
            break;
        case GL_DEBUG_SEVERITY_LOW:
            fprintf(stderr, "Severity: low\n");
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            fprintf(stderr, "Severity: notification\n");
            break;
    }
}
