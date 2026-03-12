/**
 * @file patch_manager.cpp
 * @brief Implementation of the PatchManager class
 */

#include "patch_manager.hpp"
#include "rom_overlay.hpp"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <cstring>
#include <algorithm>

namespace godot {

void PatchManager::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_base_rom", "path"), &PatchManager::load_base_rom);
    ClassDB::bind_method(D_METHOD("load_base_rom_from_bytes", "data"), &PatchManager::load_base_rom_from_bytes);
    ClassDB::bind_method(D_METHOD("reset_to_base"), &PatchManager::reset_to_base);
    ClassDB::bind_method(D_METHOD("apply_bps_patch", "path"), &PatchManager::apply_bps_patch);
    ClassDB::bind_method(D_METHOD("apply_bps_patch_from_bytes", "data"), &PatchManager::apply_bps_patch_from_bytes);
    ClassDB::bind_method(D_METHOD("apply_ips_patch", "path"), &PatchManager::apply_ips_patch);
    ClassDB::bind_method(D_METHOD("apply_ips_patch_from_bytes", "data"), &PatchManager::apply_ips_patch_from_bytes);
    ClassDB::bind_method(D_METHOD("get_patch_count"), &PatchManager::get_patch_count);
    ClassDB::bind_method(D_METHOD("get_patched_buffer"), &PatchManager::get_patched_buffer);
    ClassDB::bind_method(D_METHOD("get_patched_size"), &PatchManager::get_patched_size);
    ClassDB::bind_method(D_METHOD("get_current_crc32"), &PatchManager::get_current_crc32);
    ClassDB::bind_method(D_METHOD("verify_crc32", "expected"), &PatchManager::verify_crc32);
    ClassDB::bind_method(D_METHOD("has_base_rom"), &PatchManager::has_base_rom);
    ClassDB::bind_method(D_METHOD("get_last_error"), &PatchManager::get_last_error);
}

PatchManager::PatchManager() : _patch_count(0), _last_error("") {}

PatchManager::~PatchManager() = default;

int PatchManager::load_base_rom(const String &p_path) {
    // Read file using Godot's FileAccess
    Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
    if (file.is_null()) {
        _last_error = "Failed to open ROM file: " + std::string(p_path.utf8().get_data());
        return -1;
    }
    
    uint64_t file_size = file->get_length();
    if (file_size == 0 || file_size > GBA_MEMORY::ROM_MAX_SIZE) {
        _last_error = "Invalid ROM file size";
        return -2;
    }
    
    PackedByteArray data = file->get_buffer(file_size);
    file->close();
    
    return load_base_rom_from_bytes(data);
}

int PatchManager::load_base_rom_from_bytes(const PackedByteArray &p_buffer) {
    if (p_buffer.is_empty() || static_cast<size_t>(p_buffer.size()) > GBA_MEMORY::ROM_MAX_SIZE) {
        _last_error = "Invalid ROM data size";
        return -1;
    }
    
    // Copy to base ROM buffer
    _base_rom.resize(p_buffer.size());
    memcpy(_base_rom.data(), p_buffer.ptr(), _base_rom.size());
    
    // Copy to patched ROM buffer (initially identical to base)
    _patched_rom = _base_rom;
    
    _patch_count = 0;
    _last_error.clear();
    
    return 0;
}

void PatchManager::reset_to_base() {
    if (_base_rom.empty()) {
        return;
    }
    
    // Reset patched ROM to base
    _patched_rom = _base_rom;
    _patch_count = 0;
    _last_error.clear();
}

int PatchManager::apply_bps_patch(const String &p_path) {
    Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
    if (file.is_null()) {
        _last_error = "Failed to open BPS patch file";
        return -1;
    }
    
    PackedByteArray data = file->get_buffer(file->get_length());
    file->close();
    
    return apply_bps_patch_from_bytes(data);
}

int PatchManager::apply_bps_patch_from_bytes(const PackedByteArray &p_data) {
    if (_base_rom.empty()) {
        _last_error = "No base ROM loaded";
        return -1;
    }
    
    // Parse and apply BPS patch using BpsPatch class
    BpsPatch bps_patch;
    if (!bps_patch.parse(p_data.ptr(), static_cast<size_t>(p_data.size()))) {
        _last_error = "Failed to parse BPS patch: " + bps_patch.getError();
        return -2;
    }
    
    uint8_t* output_buffer = nullptr;
    size_t output_size = 0;
    
    if (!bps_patch.apply(_patched_rom.data(), _patched_rom.size(), output_buffer, output_size)) {
        _last_error = "Failed to apply BPS patch: " + bps_patch.getError();
        return -3;
    }
    
    // Copy result to patched ROM buffer
    _patched_rom.resize(output_size);
    memcpy(_patched_rom.data(), output_buffer, output_size);
    
    // Clean up allocated buffer from BpsPatch::apply
    delete[] output_buffer;
    
    _patch_count++;
    _last_error.clear();
    
    return 0;
}

int PatchManager::apply_ips_patch(const String &p_path) {
    Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
    if (file.is_null()) {
        _last_error = "Failed to open IPS patch file";
        return -1;
    }
    
    PackedByteArray data = file->get_buffer(file->get_length());
    file->close();
    
    return apply_ips_patch_from_bytes(data);
}

int PatchManager::apply_ips_patch_from_bytes(const PackedByteArray &p_data) {
    if (_base_rom.empty()) {
        _last_error = "No base ROM loaded";
        return -1;
    }
    
    // Parse and apply IPS patch using IpsPatch class
    IpsPatch ips_patch;
    if (!ips_patch.parse(p_data.ptr(), static_cast<size_t>(p_data.size()))) {
        _last_error = "Failed to parse IPS patch: " + ips_patch.getError();
        return -2;
    }
    
    uint8_t* output_buffer = nullptr;
    size_t output_size = 0;
    
    if (!ips_patch.apply(_patched_rom.data(), _patched_rom.size(), output_buffer, output_size)) {
        _last_error = "Failed to apply IPS patch: " + ips_patch.getError();
        return -3;
    }
    
    // Copy result to patched ROM buffer
    _patched_rom.resize(output_size);
    memcpy(_patched_rom.data(), output_buffer, output_size);
    
    // Clean up allocated buffer from IpsPatch::apply
    delete[] output_buffer;
    
    _patch_count++;
    _last_error.clear();
    
    return 0;
}

PackedByteArray PatchManager::get_patched_buffer() const {
    PackedByteArray result;
    if (!_patched_rom.empty()) {
        result.resize(static_cast<int>(_patched_rom.size()));
        memcpy(result.ptrw(), _patched_rom.data(), _patched_rom.size());
    }
    return result;
}

int PatchManager::get_patched_size() const {
    return static_cast<int>(_patched_rom.size());
}

int64_t PatchManager::get_current_crc32() const {
    if (_patched_rom.empty()) {
        return 0;
    }
    return RomOverlayUtils::calculateCRC32(_patched_rom.data(), _patched_rom.size());
}

bool PatchManager::verify_crc32(int64_t p_expected) const {
    return get_current_crc32() == p_expected;
}

bool PatchManager::has_base_rom() const {
    return !_base_rom.empty();
}

String PatchManager::get_last_error() const {
    return String(_last_error.c_str());
}

int PatchManager::get_patch_count() const {
    return _patch_count;
}

} // namespace godot
