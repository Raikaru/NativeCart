/**
 * @file rom_overlay.hpp
 * @brief Virtual Memory Overlay System for Pokefirered GDExtension
 * 
 * This system enables runtime ROM patching without file modifications by:
 * - Loading base ROM into memory buffers
 * - Applying BPS and IPS patches at runtime
 * - Redirecting C pointers to use patched memory
 * - Supporting multiple patch layers
 * 
 * @author GDExtension Team
 * @version 1.0.0
 */

#ifndef ROM_OVERLAY_HPP
#define ROM_OVERLAY_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <mutex>

namespace godot {

// Forward declarations
class RomOverlay;

/**
 * @brief GBA Memory Map Constants
 */
namespace GBA_MEMORY {
    constexpr uint32_t ROM_BASE_ADDRESS = 0x08000000;
    constexpr uint32_t ROM_MAX_SIZE = 0x02000000;  // 32MB max
    constexpr uint32_t EWRAM_BASE = 0x02000000;
    constexpr uint32_t IWRAM_BASE = 0x03000000;
    constexpr uint32_t IO_REGS_BASE = 0x04000000;
    constexpr uint32_t PALETTE_RAM_BASE = 0x05000000;
    constexpr uint32_t VRAM_BASE = 0x06000000;
    constexpr uint32_t OAM_BASE = 0x07000000;
}

/**
 * @brief Patch format enumeration
 */
enum class PatchFormat {
    UNKNOWN,
    BPS,    // Binary Patching System
    IPS,    // International Patching System
    UPS     // Universal Patching System (future support)
};

/**
 * @brief Result of patch verification
 */
struct PatchVerificationResult {
    bool success;
    uint32_t crc32;
    std::string sha256;
    std::string error_message;
};

/**
 * @brief Memory region descriptor for pointer redirection
 */
struct MemoryRegion {
    uint32_t gba_address;
    uint32_t size;
    uint8_t* host_pointer;
    std::string name;
    bool is_executable;
    bool is_writable;
};

/**
 * @brief BPS Patch Command Types
 */
namespace BPS_COMMAND {
    constexpr uint8_t SOURCE_READ = 0;
    constexpr uint8_t TARGET_READ = 1;
    constexpr uint8_t SOURCE_COPY = 2;
    constexpr uint8_t TARGET_COPY = 3;
}

/**
 * @brief Virtual ROM buffer with overlay support
 * 
 * Manages a memory buffer that represents the GBA ROM space.
 * Supports layered patching where multiple patches can be applied
 * in sequence without modifying the original base ROM.
 */
class VirtualRomBuffer {
public:
    VirtualRomBuffer();
    ~VirtualRomBuffer();

    // Disable copy, enable move
    VirtualRomBuffer(const VirtualRomBuffer&) = delete;
    VirtualRomBuffer& operator=(const VirtualRomBuffer&) = delete;
    VirtualRomBuffer(VirtualRomBuffer&&) noexcept;
    VirtualRomBuffer& operator=(VirtualRomBuffer&&) noexcept;

    /**
     * @brief Allocate buffer of specified size
     * @param size Size in bytes
     * @return true if allocation succeeded
     */
    bool allocate(size_t size);

    /**
     * @brief Deallocate buffer
     */
    void deallocate();

    /**
     * @brief Load data from file into buffer
     * @param file_path Path to file
     * @return true if load succeeded
     */
    bool loadFromFile(const std::string& file_path);

    /**
     * @brief Load data from byte array
     * @param data Pointer to data
     * @param size Size in bytes
     * @return true if load succeeded
     */
    bool loadFromMemory(const uint8_t* data, size_t size);

    /**
     * @brief Resize buffer (preserves existing data)
     * @param new_size New size in bytes
     * @return true if resize succeeded
     */
    bool resize(size_t new_size);

    /**
     * @brief Get pointer to buffer at offset
     * @param offset Byte offset
     * @return Pointer to data, or nullptr if invalid
     */
    uint8_t* getPointer(size_t offset = 0);

    /**
     * @brief Get const pointer to buffer at offset
     * @param offset Byte offset
     * @return Const pointer to data, or nullptr if invalid
     */
    const uint8_t* getPointer(size_t offset = 0) const;

    /**
     * @brief Read data from buffer
     * @param offset Start offset
     * @param data Output buffer
     * @param size Number of bytes to read
     * @return true if read succeeded
     */
    bool read(size_t offset, uint8_t* data, size_t size) const;

    /**
     * @brief Write data to buffer
     * @param offset Start offset
     * @param data Input buffer
     * @param size Number of bytes to write
     * @return true if write succeeded
     */
    bool write(size_t offset, const uint8_t* data, size_t size);

    /**
     * @brief Get buffer size
     */
    size_t getSize() const { return size_; }

    /**
     * @brief Check if buffer is valid
     */
    bool isValid() const { return buffer_ != nullptr && size_ > 0; }

    /**
     * @brief Calculate CRC32 checksum
     */
    uint32_t calculateCRC32() const;

    /**
     * @brief Calculate SHA256 hash
     */
    std::string calculateSHA256() const;

    /**
     * @brief Copy entire buffer
     * @return New buffer with copied data
     */
    std::unique_ptr<VirtualRomBuffer> clone() const;

private:
    uint8_t* buffer_;
    size_t size_;
    size_t capacity_;
};

/**
 * @brief BPS Patch Parser and Applier
 * 
 * Implements the BPS (Binary Patching System) format as described in:
 * https://github.com/blakebd/BPS-JS
 * 
 * BPS Format Structure:
 * - Magic: "BPS1" (4 bytes)
 * - Source size (variable length)
 * - Target size (variable length)
 * - Patch metadata size (variable length)
 * - Patch metadata (variable length, optional)
 * - Patch actions (variable length)
 * - Source CRC32 (4 bytes)
 * - Target CRC32 (4 bytes)
 * - Patch CRC32 (4 bytes)
 */
class BpsPatch {
public:
    BpsPatch();
    ~BpsPatch();

    /**
     * @brief Parse BPS patch data
     * @param patch_data Pointer to patch data
     * @param patch_size Size of patch data
     * @return true if parsing succeeded
     */
    bool parse(const uint8_t* patch_data, size_t patch_size);

    /**
     * @brief Parse BPS patch from file
     * @param file_path Path to .bps file
     * @return true if parsing succeeded
     */
    bool parseFile(const std::string& file_path);

    /**
     * @brief Apply patch to source buffer
     * @param source Source ROM buffer
     * @param source_size Size of source buffer
     * @param target Output buffer (will be allocated)
     * @param target_size Output size
     * @return true if application succeeded
     */
    bool apply(const uint8_t* source, size_t source_size,
               uint8_t*& target, size_t& target_size);

    /**
     * @brief Get source CRC32 from patch header
     */
    uint32_t getSourceCRC32() const { return source_crc32_; }

    /**
     * @brief Get target CRC32 from patch header
     */
    uint32_t getTargetCRC32() const { return target_crc32_; }

    /**
     * @brief Get expected source size
     */
    size_t getSourceSize() const { return source_size_; }

    /**
     * @brief Get expected target size
     */
    size_t getTargetSize() const { return target_size_; }

    /**
     * @brief Verify source buffer matches expected CRC
     */
    bool verifySource(const uint8_t* source, size_t size) const;

    /**
     * @brief Check if patch is valid
     */
    bool isValid() const { return valid_; }

    /**
     * @brief Get last error message
     */
    std::string getError() const { return error_message_; }

    // BPS variable-length integer encoding (public for PatchManager)
    static uint64_t decodeVariableLength(const uint8_t* data, size_t& pos, size_t max_pos);

private:
    // Parse and apply patch actions
    bool applyActions(const uint8_t* patch_data, size_t patch_size,
                      const uint8_t* source, size_t source_size,
                      uint8_t* target);

    bool valid_;
    size_t source_size_;
    size_t target_size_;
    size_t metadata_size_;
    uint32_t source_crc32_;
    uint32_t target_crc32_;
    uint32_t patch_crc32_;
    std::string error_message_;
};

/**
 * @brief IPS Patch Parser and Applier
 * 
 * Implements the IPS (International Patching System) format.
 * 
 * IPS Format Structure:
 * - Magic: "PATCH" (5 bytes)
 * - Records:
 *   - Offset (3 bytes, big-endian)
 *   - Size (2 bytes, big-endian)
 *   - Data (Size bytes, OR if Size == 0, RLE encoded)
 * - End marker: "EOF" (3 bytes)
 */
class IpsPatch {
public:
    IpsPatch();
    ~IpsPatch();

    /**
     * @brief Parse IPS patch data
     * @param patch_data Pointer to patch data
     * @param patch_size Size of patch data
     * @return true if parsing succeeded
     */
    bool parse(const uint8_t* patch_data, size_t patch_size);

    /**
     * @brief Parse IPS patch from file
     * @param file_path Path to .ips file
     * @return true if parsing succeeded
     */
    bool parseFile(const std::string& file_path);

    /**
     * @brief Apply patch to source buffer
     * @param source Source ROM buffer
     * @param source_size Size of source buffer
     * @param target Output buffer (will be allocated, may be larger than source)
     * @param target_size Output size
     * @return true if application succeeded
     */
    bool apply(const uint8_t* source, size_t source_size,
               uint8_t*& target, size_t& target_size);

    /**
     * @brief Check if patch is valid
     */
    bool isValid() const { return valid_; }

    /**
     * @brief Get last error message
     */
    std::string getError() const { return error_message_; }

    /**
     * @brief Get number of patch records
     */
    size_t getRecordCount() const { return records_.size(); }

    /**
     * @brief Get maximum offset in patch
     */
    size_t getMaxOffset() const { return max_offset_; }

private:
    struct IpsRecord {
        uint32_t offset;    // 3-byte offset
        uint16_t size;      // 2-byte size
        bool is_rle;        // Run-length encoded
        uint16_t rle_count; // For RLE records
        std::vector<uint8_t> data;
    };

    bool valid_;
    std::vector<IpsRecord> records_;
    size_t max_offset_;
    std::string error_message_;
};

/**
 * @brief GBA Pointer Redirector
 * 
 * Manages redirection of GBA pointers to host memory.
 * GBA ROM addresses (0x08000000+) are mapped to the patched ROM buffer.
 * IWRAM/EWRAM addresses are mapped to separate buffers.
 */
class PointerRedirector {
public:
    PointerRedirector();
    ~PointerRedirector();

    /**
     * @brief Register a memory region
     * @param region Memory region descriptor
     * @return true if registration succeeded
     */
    bool registerRegion(const MemoryRegion& region);

    /**
     * @brief Unregister a memory region by name
     * @param name Region name
     */
    void unregisterRegion(const std::string& name);

    /**
     * @brief Map GBA address to host pointer
     * @param gba_address GBA address (0x08000000+)
     * @return Host pointer, or nullptr if invalid
     */
    uint8_t* mapAddress(uint32_t gba_address);

    /**
     * @brief Map GBA address to host pointer (const)
     * @param gba_address GBA address (0x08000000+)
     * @return Const host pointer, or nullptr if invalid
     */
    const uint8_t* mapAddress(uint32_t gba_address) const;

    /**
     * @brief Read value from GBA address
     * @tparam T Type to read (uint8_t, uint16_t, uint32_t)
     * @param gba_address GBA address
     * @return Value at address
     */
    template<typename T>
    T read(uint32_t gba_address) const;

    /**
     * @brief Write value to GBA address
     * @tparam T Type to write
     * @param gba_address GBA address
     * @param value Value to write
     */
    template<typename T>
    void write(uint32_t gba_address, T value);

    /**
     * @brief Set ROM buffer for automatic mapping
     * @param buffer ROM buffer
     * @param size Buffer size
     */
    void setRomBuffer(uint8_t* buffer, size_t size);

    /**
     * @brief Get pointer to GBA structure
     * @tparam T Structure type
     * @param gba_address GBA address of structure
     * @return Pointer to structure, or nullptr if invalid
     */
    template<typename T>
    T* getStructure(uint32_t gba_address) {
        return reinterpret_cast<T*>(mapAddress(gba_address));
    }

    /**
     * @brief Clear all registered regions
     */
    void clear();

private:
    std::unordered_map<std::string, MemoryRegion> regions_;
    std::unordered_map<uint32_t, MemoryRegion*> address_map_;
    uint8_t* rom_buffer_;
    size_t rom_size_;
    mutable std::mutex mutex_;
};

/**
 * @brief Utility functions for memory operations
 */
namespace RomOverlayUtils {
    /**
     * @brief Calculate CRC32 checksum
     * @param data Data pointer
     * @param size Data size
     * @return CRC32 value
     */
    uint32_t calculateCRC32(const uint8_t* data, size_t size);

    /**
     * @brief Calculate SHA256 hash
     * @param data Data pointer
     * @param size Data size
     * @return SHA256 hex string
     */
    std::string calculateSHA256(const uint8_t* data, size_t size);

    /**
     * @brief Detect patch format from data
     * @param data Data pointer
     * @param size Data size
     * @return Detected format
     */
    PatchFormat detectPatchFormat(const uint8_t* data, size_t size);

    /**
     * @brief Read file to byte array
     * @param path File path
     * @param data Output data vector
     * @return true if successful
     */
    bool readFileToBytes(const std::string& path, std::vector<uint8_t>& data);
}

} // namespace godot

#endif // ROM_OVERLAY_HPP
