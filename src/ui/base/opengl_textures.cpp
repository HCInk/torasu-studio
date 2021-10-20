// This needs to be included where opengl-stuff is already included

#include "base.hpp"

namespace tstudio {

namespace {
TextureId opengl_textures_gl_to_tex_id(GLuint glTextureId) {
	return static_cast<TextureId>(glTextureId);
}
GLuint opengl_textures_tex_to_gl_id(TextureId textureId) {
	return static_cast<GLuint>(textureId);
}
} // namespace

void* opengl_textures_tex_id_to_imgui_id(TextureId textureId) {
	return (void*)(intptr_t) opengl_textures_tex_to_gl_id(textureId);
}

void opengl_textures_update_texture(TextureId id, uint32_t texWidth, uint32_t texHeight, uint8_t* data) {
	auto glTextureId = opengl_textures_tex_to_gl_id(id);

    glBindTexture(GL_TEXTURE_2D, glTextureId);
    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

}

TextureId opengl_textures_create_texture(uint32_t texWidth, uint32_t texHeight, uint8_t* data) {
	GLuint glTextureId;
	// Create a OpenGL texture identifier
    glGenTextures(1, &glTextureId);
    glBindTexture(GL_TEXTURE_2D, glTextureId);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	auto textureId = opengl_textures_gl_to_tex_id(glTextureId);

	opengl_textures_update_texture(textureId, texWidth, texHeight, data);
	
	return textureId;
}

void opengl_textures_destory_texture(TextureId id) {
	auto glTextureId = opengl_textures_tex_to_gl_id(id);
    glDeleteTextures(1, &glTextureId);
}

} // namespace tstudio