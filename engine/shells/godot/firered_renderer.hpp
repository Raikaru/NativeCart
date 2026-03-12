#ifndef ENGINE_SHELLS_GODOT_FIRERED_RENDERER_HPP
#define ENGINE_SHELLS_GODOT_FIRERED_RENDERER_HPP

#include <stdint.h>

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>

namespace godot {

class FireRedRenderer {
public:
    static void ensure_image(Ref<Image> &image, int width, int height);
    static void update_texture(Ref<Image> &image, Ref<ImageTexture> &texture, const uint8_t *rgba, int width, int height);
};

}

#endif
