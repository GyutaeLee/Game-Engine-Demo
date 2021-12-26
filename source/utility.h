#ifndef __GL_UTILITY_H__
#define __GL_UTILITY_H__

#include <vector>

void file_open_fill_buffer(const char* path, std::vector<char>& buffer);

void gl_validate_shader(unsigned so, const char* shader_source);
void gl_validate_program(unsigned pso, unsigned vso, unsigned fso);
unsigned gl_load_model_texture(unsigned char* data, int width, int height, int comp);
void gl_check_error(const char* file, int line);
#define GL_CHECK_ERROR() gl_check_error(__FILE__, __LINE__)

#endif