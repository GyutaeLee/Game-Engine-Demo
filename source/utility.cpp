#include "utility.h"

#include <stdio.h>
#include <assert.h>

#include "glad/glad.h"

bool file_read_until_total_size(FILE* fp, int total_size, void* buffer)
{
    int should_read_size = total_size;
    int total_read_size = 0;
    int cur_read_size = 0;
    while (total_read_size < total_size)
    {
        cur_read_size = fread((void*)((uint8_t*)buffer + total_read_size), 1, should_read_size, fp);
        total_read_size += cur_read_size;
        should_read_size -= cur_read_size;
    }

    return total_read_size == total_size;
};

void file_open_fill_buffer(const char* path, std::vector<char>& buffer)
{
    FILE* fp = fopen(path, "rb");
    if (!fp)
    {
        printf("Fail to open a vertex shader\n");
        assert(false);
    }
    fseek(fp, 0, SEEK_END);
    int file_size = (int)ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buffer.resize(file_size + 1);
    if (false == file_read_until_total_size(fp, file_size, buffer.data()))
    {
        printf("Fail to read a vertex shader\n");
        assert(false);
    }
    buffer[file_size] = '\0';

    fclose(fp);
}

void gl_validate_shader(unsigned so, const char* shader_source)
{
    glShaderSource(so, 1, &shader_source, NULL);
    glCompileShader(so);
    {
        GLint success;
        glGetShaderiv(so, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char info_logs[512];
            glGetShaderInfoLog(so, 512, NULL, info_logs);
            printf("Fail to Compile source : %s\n", info_logs);
            assert(false);
        }
    }
}

void gl_validate_program(unsigned pso, unsigned vso, unsigned fso)
{
    glAttachShader(pso, vso);
    glAttachShader(pso, fso);
    glLinkProgram(pso);
    {
        GLint success;
        glGetProgramiv(pso, GL_LINK_STATUS, &success);
        if (!success)
        {
            char info_logs[512];
            glGetProgramInfoLog(pso, 512, NULL, info_logs);
            printf("Fail to Compile source : %s\n", info_logs);
            assert(false);
        }
    }
}

unsigned gl_load_model_texture(unsigned char* data, int width, int height, int comp)
{
    unsigned gl_id;
    glGenTextures(1, &(gl_id));

    GLenum internalFormat;
    GLenum dataFormat;

    if (comp == 1)
    {
        internalFormat = dataFormat = GL_RED;
    }
    else if (comp == 3)
    {
        internalFormat = GL_RGB;
        dataFormat = GL_RGB;
    }
    else if (comp == 4)
    {
        internalFormat = GL_RGBA;
        dataFormat = GL_RGBA;
    }

    glBindTexture(GL_TEXTURE_2D, gl_id);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return gl_id;
}

void gl_check_error(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        const char* error = "ERROR_LOG_NONE";
        switch (errorCode)
        {
        case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }

        printf("Error %s | %s ( %d )", error, file, line);
    }
}