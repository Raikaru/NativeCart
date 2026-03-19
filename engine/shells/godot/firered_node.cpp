#define NOMINMAX

#include "firered_node.hpp"

#include "firered_audio_bridge.hpp"
#include "firered_renderer.hpp"

extern "C" {
#include "../../core/core_loader.h"
#include "../../core/engine_audio.h"
#include "../../core/engine_input.h"
#include "../../core/engine_runtime.h"
#include "../../core/engine_video.h"
#include "../../../cores/firered/firered_core.h"
}

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include <fstream>
#include <string>
#ifdef _WIN32
#include <cstdlib>
#endif

namespace godot {

static void write_node_trace(const char *message) {
#ifdef _WIN32
    const char *temp_dir = std::getenv("TEMP");
    std::string path = (temp_dir != nullptr ? temp_dir : ".");
    path += "\\pokefirered_gdextension.log";
#else
    std::string path = "/tmp/pokefirered_gdextension.log";
#endif

    std::ofstream out(path.c_str(), std::ios::app);
    if (out.is_open()) {
        out << message << '\n';
    }
}

void FireRedNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_rom", "path"), &FireRedNode::load_rom);
    ClassDB::bind_method(D_METHOD("load_rom_bytes", "rom_bytes"), &FireRedNode::load_rom_bytes);
    ClassDB::bind_method(D_METHOD("start_engine"), &FireRedNode::start_engine);
    ClassDB::bind_method(D_METHOD("stop_engine"), &FireRedNode::stop_engine);
    ClassDB::bind_method(D_METHOD("reset_engine"), &FireRedNode::reset_engine);
    ClassDB::bind_method(D_METHOD("open_start_menu"), &FireRedNode::open_start_menu);
    ClassDB::bind_method(D_METHOD("save_state", "path"), &FireRedNode::save_state);
    ClassDB::bind_method(D_METHOD("load_state", "path"), &FireRedNode::load_state);
    ClassDB::bind_method(D_METHOD("set_input_override", "buttons"), &FireRedNode::set_input_override);
    ClassDB::bind_method(D_METHOD("clear_input_override"), &FireRedNode::clear_input_override);
    ClassDB::bind_method(D_METHOD("step_frame"), &FireRedNode::step_frame);
    ClassDB::bind_method(D_METHOD("is_running"), &FireRedNode::is_running);
    ClassDB::bind_method(D_METHOD("has_rom_loaded"), &FireRedNode::has_rom_loaded);
    ClassDB::bind_method(D_METHOD("get_framebuffer_texture"), &FireRedNode::get_framebuffer_texture);
}

FireRedNode::FireRedNode() {
    write_node_trace("FireRedNode::FireRedNode");
}

FireRedNode::~FireRedNode() {
    if (running || rom_loaded) {
        engine_shutdown();
    }
}

void FireRedNode::_ready() {
    write_node_trace("FireRedNode::_ready");
    set_process(true);
}

void FireRedNode::_process(double delta) {
    if (!running) {
        return;
    }

    fps_title_timer += delta;
    if (fps_title_timer >= 0.5) {
        Window *window = get_window();

        if (window != nullptr) {
            int fps = Engine::get_singleton()->get_frames_per_second();
            window->set_title(String("NativeCart Godot - ") + String::num_int64(fps) + " FPS");
        }
        fps_title_timer = 0.0;
    }

    step_frame();
}

void FireRedNode::_exit_tree() {
    if (running || rom_loaded) {
        engine_shutdown();
        running = false;
        rom_loaded = false;
    }
}

bool FireRedNode::load_rom(const String &path) {
    write_node_trace("FireRedNode::load_rom enter");
    Ref<FileAccess> file = FileAccess::open(path, FileAccess::ModeFlags::READ);

    if (file.is_null()) {
        write_node_trace("FireRedNode::load_rom file open failed");
        return false;
    }

    PackedByteArray buffer = file->get_buffer(file->get_length());
    file->close();

    write_node_trace("FireRedNode::load_rom bytes read");

    return load_rom_bytes(buffer);
}

bool FireRedNode::load_rom_bytes(const PackedByteArray &rom_bytes) {
    write_node_trace("FireRedNode::load_rom_bytes enter");
    if (rom_bytes.is_empty()) {
        write_node_trace("FireRedNode::load_rom_bytes empty");
        return false;
    }

    return init_runtime(reinterpret_cast<const uint8_t *>(rom_bytes.ptr()), (size_t)rom_bytes.size());
}

bool FireRedNode::init_runtime(const uint8_t *rom_data, size_t rom_size) {
    write_node_trace("FireRedNode::init_runtime enter");
    if (rom_data == nullptr || rom_size == 0) {
        write_node_trace("FireRedNode::init_runtime invalid args");
        return false;
    }

    engine_set_core(firered_core_get());

    write_node_trace("FireRedNode::init_runtime pre-shutdown");
    engine_shutdown();
    running = false;

    write_node_trace("FireRedNode::init_runtime pre-load_rom");
    rom_loaded = engine_load_rom(rom_data, rom_size) != 0;
    write_node_trace(rom_loaded ? "FireRedNode::init_runtime init ok" : "FireRedNode::init_runtime init failed");
    if (rom_loaded) {
        write_node_trace("FireRedNode::init_runtime pre-update_video");
        update_video();
        write_node_trace("FireRedNode::init_runtime post-update_video");
    }
    return rom_loaded;
}

void FireRedNode::start_engine() {
    if (rom_loaded) {
        running = true;
    }
}

void FireRedNode::stop_engine() {
    running = false;
}

void FireRedNode::reset_engine() {
    if (!rom_loaded) {
        return;
    }
    engine_reset();
    update_video();
}

bool FireRedNode::open_start_menu() {
    if (!rom_loaded) {
        return false;
    }
    return engine_open_start_menu() != 0;
}

bool FireRedNode::save_state(const String &path) {
    CharString utf8_path;

    if (!rom_loaded) {
        return false;
    }

    utf8_path = path.utf8();
    return engine_save_state(utf8_path.get_data()) != 0;
}

bool FireRedNode::load_state(const String &path) {
    CharString utf8_path;
    bool ok;

    if (!rom_loaded) {
        return false;
    }

    utf8_path = path.utf8();
    ok = engine_load_state(utf8_path.get_data()) != 0;
    if (ok) {
        update_video();
        update_audio();
    }
    return ok;
}

void FireRedNode::set_input_override(int buttons) {
    input_override_active = true;
    input_override_buttons = static_cast<uint16_t>(buttons);
}

void FireRedNode::clear_input_override() {
    input_override_active = false;
    input_override_buttons = 0;
}

bool FireRedNode::step_frame() {
    write_node_trace("FireRedNode::step_frame enter");
    if (!rom_loaded) {
        write_node_trace("FireRedNode::step_frame no rom");
        return false;
    }

    engine_input_set_buttons(collect_input());
    write_node_trace("FireRedNode::step_frame pre-run_frame");
    engine_run_frame();
    write_node_trace("FireRedNode::step_frame post-run_frame");
    update_video();
    update_audio();
    write_node_trace("FireRedNode::step_frame done");
    return true;
}

bool FireRedNode::is_running() const {
    return running;
}

bool FireRedNode::has_rom_loaded() const {
    return rom_loaded;
}

Ref<ImageTexture> FireRedNode::get_framebuffer_texture() const {
    return framebuffer_texture;
}

uint16_t FireRedNode::collect_input() const {
    if (input_override_active) {
        return input_override_buttons;
    }

    Input *input = Input::get_singleton();
    uint16_t buttons = 0;

    if (input == nullptr) {
        return 0;
    }

    if (input->is_key_pressed(Key::KEY_X)) buttons |= 0x0001;
    if (input->is_key_pressed(Key::KEY_Z)) buttons |= 0x0002;
    if (input->is_key_pressed(Key::KEY_BACKSPACE)) buttons |= 0x0004;
    if (input->is_key_pressed(Key::KEY_ENTER)) buttons |= 0x0008;
    if (input->is_key_pressed(Key::KEY_RIGHT)) buttons |= 0x0010;
    if (input->is_key_pressed(Key::KEY_LEFT)) buttons |= 0x0020;
    if (input->is_key_pressed(Key::KEY_UP)) buttons |= 0x0040;
    if (input->is_key_pressed(Key::KEY_DOWN)) buttons |= 0x0080;
    if (input->is_key_pressed(Key::KEY_S)) buttons |= 0x0100;
    if (input->is_key_pressed(Key::KEY_A)) buttons |= 0x0200;

    return buttons;
}

void FireRedNode::update_video() {
    EngineVideoFrame frame = engine_video_get_frame();

    write_node_trace("FireRedNode::update_video");
    FireRedRenderer::update_texture(
        framebuffer_image,
        framebuffer_texture,
        frame.rgba,
        frame.width,
        frame.height);
}

void FireRedNode::update_audio() {
    FireRedAudioBridge::consume_runtime_audio(engine_audio_get_buffer());
}

}
