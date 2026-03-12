#include "firered_renderer.hpp"

#include <cstring>

#include <godot_cpp/classes/image.hpp>

namespace godot {

void FireRedRenderer::ensure_image(Ref<Image> &image, int width, int height) {
    if (image.is_valid()) {
        return;
    }

    PackedByteArray data;
    data.resize(width * height * 4);
    image = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, data);
}

void FireRedRenderer::update_texture(Ref<Image> &image, Ref<ImageTexture> &texture, const uint8_t *rgba, int width, int height) {
    PackedByteArray data;

    if (rgba == nullptr || width <= 0 || height <= 0) {
        return;
    }

    ensure_image(image, width, height);

    data.resize(width * height * 4);
    memcpy(data.ptrw(), rgba, (size_t)(width * height * 4));
    image->set_data(width, height, false, Image::FORMAT_RGBA8, data);

    texture = ImageTexture::create_from_image(image);
}

}
