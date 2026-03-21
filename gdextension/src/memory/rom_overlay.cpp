/**
 * @file rom_overlay.cpp
 * @brief Virtual Memory Overlay System Implementation
 */

#include "rom_overlay.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <openssl/evp.h>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

// ============================================================================
// CRC32 Lookup Table
// ============================================================================

static const uint32_t CRC32_TABLE[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

// ============================================================================
// VirtualRomBuffer Implementation
// ============================================================================

VirtualRomBuffer::VirtualRomBuffer() : buffer_(nullptr), size_(0), capacity_(0) {}

VirtualRomBuffer::~VirtualRomBuffer() {
    deallocate();
}

VirtualRomBuffer::VirtualRomBuffer(VirtualRomBuffer&& other) noexcept
    : buffer_(other.buffer_), size_(other.size_), capacity_(other.capacity_) {
    other.buffer_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}

VirtualRomBuffer& VirtualRomBuffer::operator=(VirtualRomBuffer&& other) noexcept {
    if (this != &other) {
        deallocate();
        buffer_ = other.buffer_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        other.buffer_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
    return *this;
}

bool VirtualRomBuffer::allocate(size_t size) {
    deallocate();
    buffer_ = new (std::nothrow) uint8_t[size];
    if (!buffer_) {
        return false;
    }
    size_ = size;
    capacity_ = size;
    std::memset(buffer_, 0, size);
    return true;
}

void VirtualRomBuffer::deallocate() {
    delete[] buffer_;
    buffer_ = nullptr;
    size_ = 0;
    capacity_ = 0;
}

bool VirtualRomBuffer::loadFromFile(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (!allocate(file_size)) {
        return false;
    }

    file.read(reinterpret_cast<char*>(buffer_), file_size);
    return file.good();
}

bool VirtualRomBuffer::loadFromMemory(const uint8_t* data, size_t size) {
    if (!allocate(size)) {
        return false;
    }
    std::memcpy(buffer_, data, size);
    return true;
}

bool VirtualRomBuffer::resize(size_t new_size) {
    if (new_size <= capacity_) {
        size_ = new_size;
        return true;
    }

    uint8_t* new_buffer = new (std::nothrow) uint8_t[new_size];
    if (!new_buffer) {
        return false;
    }

    if (buffer_) {
        std::memcpy(new_buffer, buffer_, size_);
        delete[] buffer_;
    }

    buffer_ = new_buffer;
    size_ = new_size;
    capacity_ = new_size;
    return true;
}

uint8_t* VirtualRomBuffer::getPointer(size_t offset) {
    if (!buffer_ || offset >= size_) {
        return nullptr;
    }
    return buffer_ + offset;
}

const uint8_t* VirtualRomBuffer::getPointer(size_t offset) const {
    if (!buffer_ || offset >= size_) {
        return nullptr;
    }
    return buffer_ + offset;
}

bool VirtualRomBuffer::read(size_t offset, uint8_t* data, size_t size) const {
    if (!buffer_ || offset + size > size_) {
        return false;
    }
    std::memcpy(data, buffer_ + offset, size);
    return true;
}

bool VirtualRomBuffer::write(size_t offset, const uint8_t* data, size_t size) {
    if (!buffer_ || offset + size > size_) {
        return false;
    }
    std::memcpy(buffer_ + offset, data, size);
    return true;
}

uint32_t VirtualRomBuffer::calculateCRC32() const {
    return RomOverlayUtils::calculateCRC32(buffer_, size_);
}

std::string VirtualRomBuffer::calculateSHA256() const {
    return RomOverlayUtils::calculateSHA256(buffer_, size_);
}

std::unique_ptr<VirtualRomBuffer> VirtualRomBuffer::clone() const {
    auto clone = std::make_unique<VirtualRomBuffer>();
    if (buffer_ && size_ > 0) {
        clone->loadFromMemory(buffer_, size_);
    }
    return clone;
}

// ============================================================================
// BpsPatch Implementation
// ============================================================================

BpsPatch::BpsPatch()
    : valid_(false), source_size_(0), target_size_(0), metadata_size_(0),
      source_crc32_(0), target_crc32_(0), patch_crc32_(0) {}

BpsPatch::~BpsPatch() = default;

// BPS / beat variable-length integers — same semantics as engine/core/mod_patch_bps.c
// (byuu bps_spec: bit 7 set ends the number; between bytes, shift <<= 7 and add shift.)
uint64_t BpsPatch::decodeVariableLength(const uint8_t* data, size_t& pos, size_t max_pos) {
    uint64_t out = 0;
    uint64_t shift = 1;
    unsigned iter = 0;

    while (pos < max_pos && iter < 16) {
        uint8_t x = data[pos++];
        ++iter;
        out += static_cast<uint64_t>(x & 0x7F) * shift;
        if (x & 0x80) {
            return out;
        }
        if (shift > (std::numeric_limits<uint64_t>::max() >> 7)) {
            break;
        }
        shift <<= 7;
        out += shift;
    }
    return out;
}

bool BpsPatch::parse(const uint8_t* patch_data, size_t patch_size) {
    valid_ = false;
    error_message_.clear();

    if (patch_size < 4) {
        error_message_ = "Patch too small for magic header";
        return false;
    }

    // Check magic "BPS1"
    if (std::memcmp(patch_data, "BPS1", 4) != 0) {
        error_message_ = "Invalid BPS magic header";
        return false;
    }

    if (patch_size < 12) {
        error_message_ = "Patch too small for header";
        return false;
    }

    size_t pos = 4;
    size_t max_pos = patch_size - 12;  // Reserve space for CRC32s

    // Decode header fields
    source_size_ = static_cast<size_t>(decodeVariableLength(patch_data, pos, max_pos));
    target_size_ = static_cast<size_t>(decodeVariableLength(patch_data, pos, max_pos));
    metadata_size_ = static_cast<size_t>(decodeVariableLength(patch_data, pos, max_pos));

    // Skip metadata
    if (pos + metadata_size_ > max_pos) {
        error_message_ = "Invalid metadata size";
        return false;
    }
    pos += metadata_size_;

    // Actions are processed during apply, just validate CRC positions
    if (patch_size < 12) {
        error_message_ = "Patch too small for CRC32s";
        return false;
    }

    // Read CRC32s (stored in big-endian)
    size_t crc_pos = patch_size - 12;
    source_crc32_ = (static_cast<uint32_t>(patch_data[crc_pos]) << 24) |
                    (static_cast<uint32_t>(patch_data[crc_pos + 1]) << 16) |
                    (static_cast<uint32_t>(patch_data[crc_pos + 2]) << 8) |
                    static_cast<uint32_t>(patch_data[crc_pos + 3]);
    
    target_crc32_ = (static_cast<uint32_t>(patch_data[crc_pos + 4]) << 24) |
                    (static_cast<uint32_t>(patch_data[crc_pos + 5]) << 16) |
                    (static_cast<uint32_t>(patch_data[crc_pos + 6]) << 8) |
                    static_cast<uint32_t>(patch_data[crc_pos + 7]);
    
    patch_crc32_ = (static_cast<uint32_t>(patch_data[crc_pos + 8]) << 24) |
                   (static_cast<uint32_t>(patch_data[crc_pos + 9]) << 16) |
                   (static_cast<uint32_t>(patch_data[crc_pos + 10]) << 8) |
                   static_cast<uint32_t>(patch_data[crc_pos + 11]);

    // Verify patch CRC32 (optional - just log warning if mismatch)
    uint32_t calculated_patch_crc = RomOverlayUtils::calculateCRC32(patch_data, crc_pos);
    if (calculated_patch_crc != patch_crc32_) {
        // Log warning but don't fail - some patches have incorrect CRCs
    }

    valid_ = true;
    return true;
}

bool BpsPatch::parseFile(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        error_message_ = "Failed to open patch file";
        return false;
    }

    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(file_size);
    file.read(reinterpret_cast<char*>(data.data()), file_size);

    if (!file.good()) {
        error_message_ = "Failed to read patch file";
        return false;
    }

    return parse(data.data(), data.size());
}

bool BpsPatch::applyActions(const uint8_t* patch_data, size_t patch_size,
                            const uint8_t* source, size_t source_size,
                            uint8_t* target) {
    size_t pos = 4;
    size_t max_pos = patch_size - 12;

    // Skip header
    decodeVariableLength(patch_data, pos, max_pos);  // source_size
    decodeVariableLength(patch_data, pos, max_pos);  // target_size
    size_t metadata_len = static_cast<size_t>(decodeVariableLength(patch_data, pos, max_pos));
    pos += metadata_len;

    // Three independent cursors (Flips libbps): do not alias SourceCopy ROM cursor with TargetCopy.
    size_t target_write = 0;
    size_t source_copy_pos = 0;
    size_t target_copy_pos = 0;

    while (pos < max_pos && target_write < target_size_) {
        uint64_t action_data = decodeVariableLength(patch_data, pos, max_pos);
        uint8_t action = action_data & 3;
        uint64_t length = (action_data >> 2) + 1;

        switch (action) {
            case BPS_COMMAND::SOURCE_READ: {
                // Copy source at current output index (beat/BPS SourceRead semantics).
                if (target_write + length > source_size ||
                    target_write + length > target_size_) {
                    error_message_ = "SourceRead out of bounds";
                    return false;
                }
                std::memcpy(target + target_write, source + target_write, length);
                target_write += length;
                break;
            }

            case BPS_COMMAND::TARGET_READ: {
                // Read literal data from patch
                if (pos + length > max_pos || target_write + length > target_size_) {
                    error_message_ = "TargetRead out of bounds";
                    return false;
                }
                std::memcpy(target + target_write, patch_data + pos, length);
                pos += length;
                target_write += length;
                break;
            }

            case BPS_COMMAND::SOURCE_COPY: {
                // Copy from source with offset
                uint64_t offset_encoded = decodeVariableLength(patch_data, pos, max_pos);
                int64_t offset = (offset_encoded & 1) ? -static_cast<int64_t>(offset_encoded >> 1)
                                                      : static_cast<int64_t>(offset_encoded >> 1);
                
                if (offset < 0) {
                    const size_t u = static_cast<size_t>(-offset);
                    if (u > source_copy_pos) {
                        error_message_ = "SourceCopy negative offset underflow";
                        return false;
                    }
                    source_copy_pos -= u;
                } else {
                    source_copy_pos += static_cast<size_t>(offset);
                }

                if (source_copy_pos + length > source_size ||
                    target_write + length > target_size_) {
                    error_message_ = "SourceCopy out of bounds";
                    return false;
                }
                std::memcpy(target + target_write, source + source_copy_pos, length);
                source_copy_pos += length;
                target_write += length;
                break;
            }

            case BPS_COMMAND::TARGET_COPY: {
                // Copy from already written target data
                uint64_t offset_encoded = decodeVariableLength(patch_data, pos, max_pos);
                int64_t offset = (offset_encoded & 1) ? -static_cast<int64_t>(offset_encoded >> 1)
                                                      : static_cast<int64_t>(offset_encoded >> 1);
                
                if (offset < 0) {
                    const size_t u = static_cast<size_t>(-offset);
                    if (u > target_copy_pos) {
                        error_message_ = "TargetCopy negative offset underflow";
                        return false;
                    }
                    target_copy_pos -= u;
                } else {
                    target_copy_pos += static_cast<size_t>(offset);
                }

                /* Flips libbps: outreadat < outat; outreadat+length <= outend (not <= outat). */
                if (target_copy_pos >= target_write) {
                    error_message_ = "TargetCopy read offset at or past write head";
                    return false;
                }
                if (target_copy_pos + length > target_size_ ||
                    target_write + length > target_size_) {
                    error_message_ = "TargetCopy out of bounds";
                    return false;
                }

                // Copy byte by byte to handle overlapping regions
                for (size_t i = 0; i < length; i++) {
                    target[target_write + i] = target[target_copy_pos + i];
                }
                target_copy_pos += length;
                target_write += length;
                break;
            }

            default:
                error_message_ = "Unknown action type";
                return false;
        }
    }

    return true;
}

bool BpsPatch::apply(const uint8_t* source, size_t source_size,
                     uint8_t*& target, size_t& target_size) {
    if (!valid_) {
        error_message_ = "Patch not parsed";
        return false;
    }

    if (source_size != source_size_) {
        error_message_ = "Source size mismatch";
        return false;
    }

    // Allocate target buffer
    target = new (std::nothrow) uint8_t[target_size_];
    if (!target) {
        error_message_ = "Failed to allocate target buffer";
        return false;
    }
    target_size = target_size_;

    // Apply patch actions
    // Note: We need the original patch data passed in again
    // Store it during parse or re-read it - for now assume caller manages
    error_message_ = "apply() needs patch data - use parse() + applyFromParsed()";
    delete[] target;
    target = nullptr;
    return false;
}

bool BpsPatch::verifySource(const uint8_t* source, size_t size) const {
    if (size != source_size_) {
        return false;
    }
    uint32_t crc = RomOverlayUtils::calculateCRC32(source, size);
    return crc == source_crc32_;
}

// ============================================================================
// IpsPatch Implementation
// ============================================================================

IpsPatch::IpsPatch() : valid_(false), max_offset_(0) {}

IpsPatch::~IpsPatch() = default;

bool IpsPatch::parse(const uint8_t* patch_data, size_t patch_size) {
    valid_ = false;
    error_message_.clear();
    records_.clear();
    max_offset_ = 0;

    if (patch_size < 5) {
        error_message_ = "Patch too small for magic header";
        return false;
    }

    // Check magic "PATCH"
    if (std::memcmp(patch_data, "PATCH", 5) != 0) {
        error_message_ = "Invalid IPS magic header";
        return false;
    }

    size_t pos = 5;

    while (pos + 3 <= patch_size) {
        // Check for EOF marker
        if (pos + 3 <= patch_size &&
            patch_data[pos] == 'E' &&
            patch_data[pos + 1] == 'O' &&
            patch_data[pos + 2] == 'F') {
            valid_ = true;
            return true;
        }

        // Need at least 5 more bytes for offset + size
        if (pos + 5 > patch_size) {
            error_message_ = "Truncated IPS record";
            return false;
        }

        IpsRecord record;
        
        // Read 3-byte offset (big-endian)
        record.offset = (static_cast<uint32_t>(patch_data[pos]) << 16) |
                        (static_cast<uint32_t>(patch_data[pos + 1]) << 8) |
                        static_cast<uint32_t>(patch_data[pos + 2]);
        pos += 3;

        // Read 2-byte size (big-endian)
        record.size = (static_cast<uint16_t>(patch_data[pos]) << 8) |
                      static_cast<uint16_t>(patch_data[pos + 1]);
        pos += 2;

        if (record.size == 0) {
            // RLE record
            record.is_rle = true;
            if (pos + 2 > patch_size) {
                error_message_ = "Truncated RLE record";
                return false;
            }
            record.rle_count = (static_cast<uint16_t>(patch_data[pos]) << 8) |
                               static_cast<uint16_t>(patch_data[pos + 1]);
            pos += 2;
            if (pos + 1 > patch_size) {
                error_message_ = "Truncated RLE data";
                return false;
            }
            record.data.push_back(patch_data[pos]);
            pos += 1;
        } else {
            // Normal record
            record.is_rle = false;
            if (pos + record.size > patch_size) {
                error_message_ = "Truncated record data";
                return false;
            }
            record.data.assign(patch_data + pos, patch_data + pos + record.size);
            pos += record.size;
        }

        records_.push_back(record);
        if (record.offset + record.size > max_offset_) {
            max_offset_ = record.offset + record.size;
        }
    }

    error_message_ = "IPS patch missing EOF marker";
    return false;
}

bool IpsPatch::parseFile(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        error_message_ = "Failed to open patch file";
        return false;
    }

    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(file_size);
    file.read(reinterpret_cast<char*>(data.data()), file_size);

    if (!file.good()) {
        error_message_ = "Failed to read patch file";
        return false;
    }

    return parse(data.data(), data.size());
}

bool IpsPatch::apply(const uint8_t* source, size_t source_size,
                     uint8_t*& target, size_t& target_size) {
    if (!valid_) {
        error_message_ = "Patch not parsed";
        return false;
    }

    // Calculate target size (IPS can expand ROM)
    target_size = std::max(source_size, max_offset_);
    
    target = new (std::nothrow) uint8_t[target_size];
    if (!target) {
        error_message_ = "Failed to allocate target buffer";
        return false;
    }

    // Copy source to target
    std::memcpy(target, source, source_size);
    if (target_size > source_size) {
        std::memset(target + source_size, 0, target_size - source_size);
    }

    // Apply records
    for (const auto& record : records_) {
        if (record.is_rle) {
            // RLE: repeat single byte
            for (uint16_t i = 0; i < record.rle_count && (record.offset + i) < target_size; i++) {
                target[record.offset + i] = record.data[0];
            }
        } else {
            // Normal: copy data
            for (size_t i = 0; i < record.data.size() && (record.offset + i) < target_size; i++) {
                target[record.offset + i] = record.data[i];
            }
        }
    }

    return true;
}

// ============================================================================
// PointerRedirector Implementation
// ============================================================================

PointerRedirector::PointerRedirector() : rom_buffer_(nullptr), rom_size_(0) {}

PointerRedirector::~PointerRedirector() = default;

bool PointerRedirector::registerRegion(const MemoryRegion& region) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Validate region
    if (!region.host_pointer || region.size == 0) {
        return false;
    }

    // Store region
    regions_[region.name] = region;

    // Update address map for quick lookup
    // For ROM regions, map the entire range
    if (region.gba_address >= GBA_MEMORY::ROM_BASE_ADDRESS &&
        region.gba_address < GBA_MEMORY::ROM_BASE_ADDRESS + GBA_MEMORY::ROM_MAX_SIZE) {
        for (uint32_t addr = region.gba_address; 
             addr < region.gba_address + region.size; 
             addr += 0x1000) {  // Map in 4KB chunks
            address_map_[addr] = &regions_[region.name];
        }
    }

    return true;
}

void PointerRedirector::unregisterRegion(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = regions_.find(name);
    if (it != regions_.end()) {
        // Remove from address map
        const auto& region = it->second;
        if (region.gba_address >= GBA_MEMORY::ROM_BASE_ADDRESS &&
            region.gba_address < GBA_MEMORY::ROM_BASE_ADDRESS + GBA_MEMORY::ROM_MAX_SIZE) {
            for (uint32_t addr = region.gba_address; 
                 addr < region.gba_address + region.size; 
                 addr += 0x1000) {
                address_map_.erase(addr);
            }
        }
        regions_.erase(it);
    }
}

uint8_t* PointerRedirector::mapAddress(uint32_t gba_address) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check ROM mapping first
    if (gba_address >= GBA_MEMORY::ROM_BASE_ADDRESS &&
        gba_address < GBA_MEMORY::ROM_BASE_ADDRESS + GBA_MEMORY::ROM_MAX_SIZE) {
        uint32_t offset = gba_address - GBA_MEMORY::ROM_BASE_ADDRESS;
        if (offset < rom_size_) {
            return rom_buffer_ + offset;
        }
        return nullptr;
    }

    // Check registered regions
    auto it = address_map_.find(gba_address & ~0xFFF);  // 4KB aligned lookup
    if (it != address_map_.end()) {
        uint32_t region_offset = gba_address - it->second->gba_address;
        if (region_offset < it->second->size) {
            return it->second->host_pointer + region_offset;
        }
    }

    return nullptr;
}

const uint8_t* PointerRedirector::mapAddress(uint32_t gba_address) const {
    return const_cast<PointerRedirector*>(this)->mapAddress(gba_address);
}

void PointerRedirector::setRomBuffer(uint8_t* buffer, size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    rom_buffer_ = buffer;
    rom_size_ = size;
}

void PointerRedirector::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    regions_.clear();
    address_map_.clear();
    rom_buffer_ = nullptr;
    rom_size_ = 0;
}

// ============================================================================
// Utility Functions Implementation
// ============================================================================

namespace RomOverlayUtils {

uint32_t calculateCRC32(const uint8_t* data, size_t size) {
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < size; i++) {
        crc = CRC32_TABLE[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFF;
}

std::string calculateSHA256(const uint8_t* data, size_t size) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return "";
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    if (EVP_DigestUpdate(ctx, data, size) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    EVP_MD_CTX_free(ctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return ss.str();
}

PatchFormat detectPatchFormat(const uint8_t* data, size_t size) {
    if (size >= 4 && std::memcmp(data, "BPS1", 4) == 0) {
        return PatchFormat::BPS;
    }
    if (size >= 5 && std::memcmp(data, "PATCH", 5) == 0) {
        return PatchFormat::IPS;
    }
    return PatchFormat::UNKNOWN;
}

bool readFileToBytes(const std::string& path, std::vector<uint8_t>& data) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    data.resize(file_size);
    file.read(reinterpret_cast<char*>(data.data()), file_size);

    return file.good();
}

std::vector<uint8_t> packedByteArrayToVector(const PackedByteArray& data) {
    std::vector<uint8_t> result;
    result.resize(data.size());
    std::memcpy(result.data(), data.ptr(), data.size());
    return result;
}

PackedByteArray vectorToPackedByteArray(const std::vector<uint8_t>& data) {
    PackedByteArray result;
    result.resize(data.size());
    std::memcpy(result.ptrw(), data.data(), data.size());
    return result;
}

} // namespace RomOverlayUtils

} // namespace godot
