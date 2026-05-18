#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//silly ai user losing control of his own code base

static char* read_file(const char* filepath) {
    FILE* file = fopen(filepath, "rb");  // Open in binary mode to avoid text translation
    if (!file) {
        printf("Failed to open file: %s\n", filepath);
        printf("Current working directory issue - file not found\n");
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer and read file
    char* buffer = malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    
    for (long i = 0; i < size; i++) {
        unsigned char c = (unsigned char)buffer[i];
        if (c > 127 || (c < 32 && c != '\n' && c != '\r' && c != '\t' && c != ' ')) {
            buffer[i] = ' ';  //replace with space
        }
    }
    
    return buffer;
}

static GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    
    // Calculate the length explicitly and pass it to glShaderSource
    GLint length = (GLint)strlen(source);
    glShaderSource(shader, 1, &source, &length);
    glCompileShader(shader);
    
    // Check for compilation errors
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        printf("Shader compilation failed: %s\n", info_log);
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

bool shader_create_from_source(Shader* shader, const char* vertex_source, const char* fragment_source) {
    if (!shader || !vertex_source || !fragment_source) return false;
    
    // Compile vertex shader
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source);
    if (!vertex_shader) return false;
    
    // Compile fragment shader
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    if (!fragment_shader) {
        glDeleteShader(vertex_shader);
        return false;
    }
    
    // Create and link program
    shader->program = glCreateProgram();
    glAttachShader(shader->program, vertex_shader);
    glAttachShader(shader->program, fragment_shader);
    glLinkProgram(shader->program);
    
    // Check for linking errors
    GLint success;
    glGetProgramiv(shader->program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(shader->program, 512, NULL, info_log);
        printf("Shader linking failed: %s\n", info_log);
        glDeleteProgram(shader->program);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return false;
    }
    
    // Clean up individual shaders
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    return true;
}

bool shader_create_from_files(Shader* shader, const char* vertex_path, const char* fragment_path) {
    if (!shader || !vertex_path || !fragment_path) return false;
    
    // Read shader files
    char* vertex_source = read_file(vertex_path);
    char* fragment_source = read_file(fragment_path);
    
    if (!vertex_source || !fragment_source) {
        free(vertex_source);
        free(fragment_source);
        return false;
    }
    
    // Create shader from sources
    bool result = shader_create_from_source(shader, vertex_source, fragment_source);
    
    // Clean up
    free(vertex_source);
    free(fragment_source);
    
    return result;
}

void shader_use(const Shader* shader) {
    if (shader) {
        glUseProgram(shader->program);
    }
}

void shader_set_mat4(const Shader* shader, const char* name, const Mat4* matrix) {
    if (!shader || !name || !matrix) return;
    
    GLint location = glGetUniformLocation(shader->program, name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, matrix->m);
    }
}

void shader_set_vec3(const Shader* shader, const char* name, const Vec3* vector) {
    if (!shader || !name || !vector) return;
    
    GLint location = glGetUniformLocation(shader->program, name);
    if (location != -1) {
        glUniform3f(location, vector->x, vector->y, vector->z);
    }
}

void shader_set_int(const Shader* shader, const char* name, int value) {
    GLint location = glGetUniformLocation(shader->program, name);
    glUniform1i(location, value);
}

void shader_destroy(Shader* shader) {
    if (shader && shader->program) {
        glDeleteProgram(shader->program);
        shader->program = 0;
    }
}