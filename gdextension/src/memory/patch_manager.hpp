#pragma once

/**
 * @file patch_manager.hpp
 * @brief GDExtension class exposing ROM patching to GDScript.
 *
 * Must extend a Godot type (Node here so it can be added as a child)
 * and use the GDCLASS macro so ClassDB can register it.
 */

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <vector>
#include <string>
#include <cstdint>

namespace godot {

class PatchManager : public Node {
    GDCLASS(PatchManager, Node)

public:
    PatchManager();
    ~PatchManager();

    // -------------------------------------------------------------------------
    // ROM loading
    // -------------------------------------------------------------------------
    int load_base_rom(const String &p_path);
    int load_base_rom_from_bytes(const PackedByteArray &p_buffer);
    void reset_to_base();

    // -------------------------------------------------------------------------
    // Patch application
    // -------------------------------------------------------------------------
    int apply_bps_patch(const String &p_path);
    int apply_bps_patch_from_bytes(const PackedByteArray &p_data);
    int apply_ips_patch(const String &p_path);
    int apply_ips_patch_from_bytes(const PackedByteArray &p_data);
    int get_patch_count() const;

    // -------------------------------------------------------------------------
    // Buffer access
    // -------------------------------------------------------------------------
    PackedByteArray get_patched_buffer() const;
    int get_patched_size() const;

    // -------------------------------------------------------------------------
    // Verification / state
    // -------------------------------------------------------------------------
    int64_t get_current_crc32() const;
    bool verify_crc32(int64_t p_expected) const;
    bool has_base_rom() const;
    String get_last_error() const;

protected:
    // Required by GDCLASS — binds all methods so GDScript can call them.
    static void _bind_methods();

private:
    std::vector<uint8_t> _base_rom;
    std::vector<uint8_t> _patched_rom;
    int _patch_count = 0;
    std::string _last_error;
};

} // namespace godot
