#ifndef ENGINE_SHELLS_GODOT_FIRERED_NODE_HPP
#define ENGINE_SHELLS_GODOT_FIRERED_NODE_HPP

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <stdint.h>

namespace godot {

class FireRedNode : public Node {
    GDCLASS(FireRedNode, Node)

private:
    bool rom_loaded = false;
    bool running = false;
    bool input_override_active = false;
    uint16_t input_override_buttons = 0;

    Ref<Image> framebuffer_image;
    Ref<ImageTexture> framebuffer_texture;
    uint16_t collect_input() const;
    bool init_runtime(const uint8_t *rom_data, size_t rom_size);
    void update_video();
    void update_audio();

protected:
    static void _bind_methods();

public:
    FireRedNode();
    ~FireRedNode();

    void _ready() override;
    void _process(double delta) override;
    void _exit_tree() override;

    bool load_rom(const String &path);
    bool load_rom_bytes(const PackedByteArray &rom_bytes);
    void start_engine();
    void stop_engine();
    void reset_engine();
    bool open_start_menu();
    bool save_state(const String &path);
    bool load_state(const String &path);
    void set_input_override(int buttons);
    void clear_input_override();
    bool step_frame();

    bool is_running() const;
    bool has_rom_loaded() const;

    Ref<ImageTexture> get_framebuffer_texture() const;
};

}

#endif
