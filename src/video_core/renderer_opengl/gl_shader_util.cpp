// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <string>
#include <vector>
#include <glad/glad.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "video_core/renderer_opengl/gl_shader_util.h"
#include "video_core/renderer_opengl/gl_vars.h"

namespace OpenGL {

GLuint LoadShader(std::string_view source, GLenum type, const std::string& debug_name) {
    std::string preamble;
#if defined(__EMSCRIPTEN__)
    preamble = "#version 300 es\nprecision highp float;\nprecision highp int;\n";
#else
    if (GLES) {
        preamble = R"(#version 320 es

#if defined(GL_ANDROID_extension_pack_es31a)
#extension GL_ANDROID_extension_pack_es31a : enable
#endif // defined(GL_ANDROID_extension_pack_es31a)

#if defined(GL_EXT_clip_cull_distance)
#extension GL_EXT_clip_cull_distance : enable
#endif // defined(GL_EXT_clip_cull_distance)
)";
    } else {
        preamble = "#version 430 core\n";
    }
#endif

    std::string_view debug_type;
    switch (type) {
    case GL_VERTEX_SHADER:
        debug_type = "vertex";
        break;
    case GL_GEOMETRY_SHADER:
        debug_type = "geometry";
        break;
    case GL_FRAGMENT_SHADER:
        debug_type = "fragment";
        break;
    default:
        UNREACHABLE();
    }

#if defined(__EMSCRIPTEN__)
    if (type == GL_GEOMETRY_SHADER) {
        return 0;
    }
#endif

    std::array<const GLchar*, 2> src_arr{preamble.data(), source.data()};
    std::array<GLint, 2> lengths{static_cast<GLint>(preamble.size()),
                                 static_cast<GLint>(source.size())};
    GLuint shader_id = glCreateShader(type);
    if (shader_id == 0) {
        GLenum err = glGetError();
        LOG_ERROR(Render_OpenGL, "glCreateShader failed for {} shader {} (err {})", debug_type,
                  debug_name, err);
        return shader_id;
    }

    glShaderSource(shader_id, static_cast<GLsizei>(src_arr.size()), src_arr.data(), lengths.data());
    LOG_DEBUG(Render_OpenGL, "Compiling {} shader {}...", debug_type, debug_name);
    glCompileShader(shader_id);

    GLint result = GL_FALSE;
    GLint info_log_length;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_length);

    if (info_log_length > 1) {
        std::vector<char> shader_error(info_log_length);
        glGetShaderInfoLog(shader_id, info_log_length, nullptr, &shader_error[0]);
        if (result == GL_TRUE) {
            LOG_DEBUG(Render_OpenGL, "Compile message for {} shader {}:\n{}", debug_type,
                      debug_name, &shader_error[0]);
        } else {
            LOG_ERROR(Render_OpenGL, "Error compiling {} shader {}:\n{}", debug_type, debug_name,
                      &shader_error[0]);
            LOG_ERROR(Render_OpenGL, "Shader source code:\n{}{}", src_arr[0], src_arr[1]);
        }
    } else if (result == GL_FALSE) {
        LOG_ERROR(Render_OpenGL, "Error compiling {} shader {}:\nNo log produced.", debug_type,
                  debug_name);
    }
    return shader_id;
}

GLuint LoadProgram(bool separable_program, std::span<const GLuint> shaders,
                   const std::string& debug_name) {
    // Link the program
    LOG_DEBUG(Render_OpenGL, "Linking program...");

    GLuint program_id = glCreateProgram();

    for (GLuint shader : shaders) {
        if (shader != 0) {
            glAttachShader(program_id, shader);
        }
    }

#if !defined(__EMSCRIPTEN__)
    if (separable_program) {
        glProgramParameteri(program_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
    }
    glProgramParameteri(program_id, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
#else
    (void)separable_program;
#endif
    glLinkProgram(program_id);

    // Check the program
    GLint result = GL_FALSE;
    GLint info_log_length;
    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);

    if (info_log_length > 1) {
        std::vector<char> program_error(info_log_length);
        glGetProgramInfoLog(program_id, info_log_length, nullptr, &program_error[0]);
        if (result == GL_TRUE) {
            LOG_DEBUG(Render_OpenGL, "Link message for shader {}:\n{}", debug_name,
                      &program_error[0]);
        } else {
            LOG_ERROR(Render_OpenGL, "Error linking shader {}:\n{}", debug_name, &program_error[0]);
        }
    } else if (result == GL_FALSE) {
        LOG_ERROR(Render_OpenGL, "Error linking shader {}: No log produced.", debug_name);
    }

    for (GLuint shader : shaders) {
        if (shader != 0) {
            glDetachShader(program_id, shader);
        }
    }

#if defined(__EMSCRIPTEN__)
    if (result == GL_TRUE) {
        glUseProgram(program_id);
        const auto set_sampler = [&](const char* name, GLint unit) {
            const GLint loc = glGetUniformLocation(program_id, name);
            if (loc >= 0) {
                glUniform1i(loc, unit);
            }
        };
        set_sampler("tex0", 0);
        set_sampler("tex1", 1);
        set_sampler("tex2", 2);
        set_sampler("texture_buffer_lut_lf", 3);
        set_sampler("texture_buffer_lut_rg", 4);
        set_sampler("texture_buffer_lut_rgba", 5);
        set_sampler("tex_normal", 6);
        set_sampler("tex_color", 7);

        const auto set_block = [&](const char* name, GLuint binding) {
            const GLuint idx = glGetUniformBlockIndex(program_id, name);
            if (idx != GL_INVALID_INDEX) {
                glUniformBlockBinding(program_id, idx, binding);
            }
        };
        set_block("vs_pica_data", 0);
        set_block("vs_data", 1);
        set_block("fs_data", 2);
        glUseProgram(0);
    }
#endif

    return program_id;
}

} // namespace OpenGL
