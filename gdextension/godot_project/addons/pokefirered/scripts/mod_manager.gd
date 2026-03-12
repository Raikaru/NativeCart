# =============================================================================
# PokeFireRed Mod Manager
# =============================================================================
# High-level GDScript interface for managing ROM patches and game state
# This script provides a user-friendly API for the GDExtension layer
# =============================================================================

class_name PokeFireRedModManager
extends Node

# =============================================================================
# SIGNALS
# =============================================================================

signal rom_loaded(success: bool, message: String)
signal patch_applied(patch_name: String, success: bool)
signal mod_list_changed
signal emulation_state_changed(state: EmulationState)

# =============================================================================
# ENUMS
# =============================================================================

enum EmulationState {
	STOPPED,
	LOADING,
	READY,
	RUNNING,
	PAUSED,
	ERROR
}

enum PatchType {
	BPS,
	IPS,
	UNKNOWN
}

# =============================================================================
# CONSTANTS
# =============================================================================

const GBA_BUTTON_A = 0x0001
const GBA_BUTTON_B = 0x0002
const GBA_BUTTON_SELECT = 0x0004
const GBA_BUTTON_START = 0x0008
const GBA_BUTTON_RIGHT = 0x0010
const GBA_BUTTON_LEFT = 0x0020
const GBA_BUTTON_UP = 0x0040
const GBA_BUTTON_DOWN = 0x0080
const GBA_BUTTON_R = 0x0100
const GBA_BUTTON_L = 0x0200

# GBA display dimensions
const GBA_WIDTH = 240
const GBA_HEIGHT = 160

# =============================================================================
# EXPORT VARIABLES
# =============================================================================

@export var auto_load_rom_on_start: bool = false
@export var rom_file_path: String = ""
@export var patches_folder: String = "user://patches/"
@export var save_folder: String = "user://saves/"

# =============================================================================
# PRIVATE VARIABLES
# =============================================================================

var _patch_manager: PatchManager = null
var _renderer: GBARenderer = null
var _current_state: EmulationState = EmulationState.STOPPED
var _loaded_mods: Array[Dictionary] = []
var _base_rom_crc32: int = 0
var _current_rom_crc32: int = 0

# Input state
var _current_input: int = 0x3FF  # All buttons released (inverted logic)

# =============================================================================
# BUILT-IN VIRTUAL METHODS
# =============================================================================

func _ready():
	# Create directories if they don't exist
	_ensure_directories()
	
	# Initialize GDExtension components
	_initialize_components()
	
	# Auto-load ROM if configured
	if auto_load_rom_on_start and not rom_file_path.is_empty():
		load_base_rom(rom_file_path)

func _process(delta: float):
	if _current_state == EmulationState.RUNNING:
		_update_input()

func _exit_tree():
	# Cleanup
	if _current_state != EmulationState.STOPPED:
		stop_emulation()

# =============================================================================
# INITIALIZATION
# =============================================================================

func _ensure_directories() -> void:
	var dirs = [patches_folder, save_folder]
	for dir in dirs:
		if not DirAccess.dir_exists_absolute(dir):
			DirAccess.make_dir_recursive_absolute(dir)

func _initialize_components() -> void:
	# Create patch manager
	_patch_manager = PatchManager.new()
	add_child(_patch_manager)
	
	# Create renderer (optional, only if rendering needed)
	if Engine.is_editor_hint() == false:
		_renderer = GBARenderer.new()
		_renderer.name = "GBARenderer"
		add_child(_renderer)

# =============================================================================
# ROM MANAGEMENT
# =============================================================================

func load_base_rom(path: String) -> bool:
	# Load the base ROM file before applying patches.
	_set_state(EmulationState.LOADING)
	
	# Validate file exists
	if not FileAccess.file_exists(path):
		var msg = "ROM file not found: " + path
		push_error(msg)
		rom_loaded.emit(false, msg)
		_set_state(EmulationState.ERROR)
		return false
	
	# Load via patch manager
	var result = _patch_manager.load_base_rom(path)
	
	if result == 0:
		_base_rom_crc32 = _patch_manager.get_current_crc32()
		_current_rom_crc32 = _base_rom_crc32
		
		var msg = "ROM loaded successfully. CRC32: 0x%08X" % _base_rom_crc32
		print(msg)
		rom_loaded.emit(true, msg)
		_set_state(EmulationState.READY)
		return true
	else:
		var msg = "Failed to load ROM. Error code: %d" % result
		push_error(msg)
		rom_loaded.emit(false, msg)
		_set_state(EmulationState.ERROR)
		return false

func load_base_rom_from_buffer(buffer: PackedByteArray) -> bool:
    """
    Load ROM from a PackedByteArray (for encrypted or streamed ROMs).
    
    Args:
        buffer: The ROM data as a PackedByteArray
    
    Returns:
        true if successful, false otherwise
    """
	_set_state(EmulationState.LOADING)
	
	if buffer.is_empty():
		push_error("ROM buffer is empty")
		rom_loaded.emit(false, "ROM buffer is empty")
		_set_state(EmulationState.ERROR)
		return false
	
	var result = _patch_manager.load_base_rom_from_bytes(buffer)
	
	if result == 0:
		_base_rom_crc32 = _patch_manager.get_current_crc32()
		_current_rom_crc32 = _base_rom_crc32
		rom_loaded.emit(true, "ROM loaded from buffer")
		_set_state(EmulationState.READY)
		return true
	else:
		var msg = "Failed to load ROM from buffer. Error code: %d" % result
		push_error(msg)
		rom_loaded.emit(false, msg)
		_set_state(EmulationState.ERROR)
		return false

func reset_to_base_rom() -> void:
    """Reset to the base ROM, removing all applied patches."""
	_patch_manager.reset_to_base()
	_current_rom_crc32 = _base_rom_crc32
	_loaded_mods.clear()
	mod_list_changed.emit()
	print("Reset to base ROM")

func get_patched_rom_data() -> PackedByteArray:
    """Get the current patched ROM data as a PackedByteArray."""
	return _patch_manager.get_patched_buffer()

func get_base_rom_crc32() -> int:
    """Get the CRC32 checksum of the base ROM."""
	return _base_rom_crc32

func get_current_crc32() -> int:
    """Get the CRC32 checksum of the current (possibly patched) ROM."""
	return _patch_manager.get_current_crc32()

# =============================================================================
# PATCH MANAGEMENT
# =============================================================================

func apply_patch_file(path: String) -> bool:
    """
    Apply a patch file (.bps or .ips) to the current ROM.
    
    Args:
        path: Path to the patch file
    
    Returns:
        true if successful, false otherwise
    """
	if _current_state != EmulationState.READY and _current_state != EmulationState.STOPPED:
		push_warning("Cannot apply patch while emulation is running")
		return false
	
	if not FileAccess.file_exists(path):
		push_error("Patch file not found: " + path)
		patch_applied.emit(path.get_file(), false)
		return falsexzxx
		zxzxzzx
		
		zxzx
		zxzxzxzx'
		xz
		
		xzzxzxzzz
		zxzx
		
	
	var extension = path.get_extension().to_lower()
	var result: int
	
	match extension:
		"bps":
			result = _patch_manager.apply_bps_patch(path)
		"ips":
			result = _patch_manager.apply_ips_patch(path)
		_:
			push_error("Unknown patch format: " + extension)
			patch_applied.emit(path.get_file(), false)
			return false
	
	if result == 0:
		_current_rom_crc32 = _patch_manager.get_current_crc32()
		
		var mod_info = {
			"name": path.get_file().get_basename(),
			"path": path,
			"type": extension.to_upper(),
			"applied_at": Time.get_datetime_string_from_system()
		}
		_loaded_mods.append(mod_info)
		
		mod_list_changed.emit()
		patch_applied.emit(path.get_file(), true)
		print("Applied patch: " + path.get_file())
		return true
	else:
		var msg = "Failed to apply patch. Error code: %d" % result
		push_error(msg)
		patch_applied.emit(path.get_file(), false)
		return false

func apply_patch_from_buffer(buffer: PackedByteArray, patch_name: String = "unknown") -> bool:
    """
    Apply a patch from a PackedByteArray.
    
    Args:
        buffer: The patch data
        patch_name: Name for this patch (for logging)
    
    Returns:
        true if successful, false otherwise
    """
	var patch_type = _detect_patch_format(buffer)
	var result: int
	
	match patch_type:
		PatchType.BPS:
			result = _patch_manager.apply_bps_patch_from_bytes(buffer)
		PatchType.IPS:
			result = _patch_manager.apply_ips_patch_from_bytes(buffer)
		_:
			push_error("Could not detect patch format")
			patch_applied.emit(patch_name, false)
			return false
	
	if result == 0:
		_current_rom_crc32 = _patch_manager.get_current_crc32()
		
		var mod_info = {
			"name": patch_name,
			"type": str(patch_type),
			"applied_at": Time.get_datetime_string_from_system()
		}
		_loaded_mods.append(mod_info)
		
		mod_list_changed.emit()
		patch_applied.emit(patch_name, true)
		return true
	else:
		patch_applied.emit(patch_name, false)
		return false

func _detect_patch_format(buffer: PackedByteArray) -> PatchType:
    """Detect patch format from buffer content."""
	if buffer.size() < 4:
		return PatchType.UNKNOWN
	
	# Check for BPS magic
	if buffer[0] == 0x42 and buffer[1] == 0x50 and buffer[2] == 0x53 and buffer[3] == 0x31:
		return PatchType.BPS
	
	# Check for IPS magic ("PATCH")
	if buffer[0] == 0x50 and buffer[1] == 0x41 and buffer[2] == 0x54 and buffer[3] == 0x43 and buffer[4] == 0x48:
		return PatchType.IPS
	
	return PatchType.UNKNOWN

func get_loaded_mods() -> Array[Dictionary]:
    """Get list of currently loaded mods."""
	return _loaded_mods.duplicate()

func remove_mod(index: int) -> void:
    """
    Remove a mod by index and reset to base ROM.
    Note: Patches are applied cumulatively, so removing one requires re-applying all.
    """
	if index < 0 or index >= _loaded_mods.size():
		return
	
	# Store mods to re-apply (excluding removed one)
	var mods_to_reapply = _loaded_mods.duplicate()
	mods_to_reapply.remove_at(index)
	
	# Reset and re-apply
	reset_to_base_rom()
	
	for mod in mods_to_reapply:
		if mod.has("path"):
			apply_patch_file(mod.path)
		elif mod.has("buffer"):
			apply_patch_from_buffer(mod.buffer, mod.name)

func clear_all_mods() -> void:
    """Remove all mods and reset to base ROM."""
	reset_to_base_rom()

# =============================================================================
# EMULATION CONTROL
# =============================================================================

func start_emulation() -> bool:
    """Start or resume emulation."""
	if _current_state == EmulationState.STOPPED or _current_state == EmulationState.ERROR:
		push_error("Cannot start emulation: No ROM loaded")
		return false
	
	if _current_state == EmulationState.READY or _current_state == EmulationState.PAUSED:
		_set_state(EmulationState.RUNNING)
		return true
	
	return false

func pause_emulation() -> bool:
    """Pause emulation."""
	if _current_state == EmulationState.RUNNING:
		_set_state(EmulationState.PAUSED)
		return true
	return false

func stop_emulation() -> bool:
    """Stop emulation and reset to ready state."""
	_set_state(EmulationState.READY)
	return true

func _set_state(new_state: EmulationState) -> void:
	if _current_state != new_state:
		_current_state = new_state
		emulation_state_changed.emit(new_state)

func get_emulation_state() -> EmulationState:
    """Get current emulation state."""
	return _current_state

# =============================================================================
# INPUT HANDLING
# =============================================================================

func _update_input() -> void:
    """Update GBA input state from Godot input."""
	var new_input = 0
	
	if Input.is_action_pressed("gba_a"):
		new_input |= GBA_BUTTON_A
	if Input.is_action_pressed("gba_b"):
		new_input |= GBA_BUTTON_B
	if Input.is_action_pressed("gba_select"):
		new_input |= GBA_BUTTON_SELECT
	if Input.is_action_pressed("gba_start"):
		new_input |= GBA_BUTTON_START
	if Input.is_action_pressed("gba_right"):
		new_input |= GBA_BUTTON_RIGHT
	if Input.is_action_pressed("gba_left"):
		new_input |= GBA_BUTTON_LEFT
	if Input.is_action_pressed("gba_up"):
		new_input |= GBA_BUTTON_UP
	if Input.is_action_pressed("gba_down"):
		new_input |= GBA_BUTTON_DOWN
	if Input.is_action_pressed("gba_r"):
		new_input |= GBA_BUTTON_R
	if Input.is_action_pressed("gba_l"):
		new_input |= GBA_BUTTON_L
	
	_current_input = new_input
	
	# Send to GDExtension
	# Note: This would require an exposed method in the GDExtension
	# For now, input is handled within the C++ layer

func set_button_pressed(button: int, pressed: bool) -> void:
    """
    Set a button state programmatically.
    
    Args:
        button: One of the GBA_BUTTON_* constants
        pressed: true if pressed, false if released
    """
	if pressed:
		_current_input |= button
	else:
		_current_input &= ~button

func release_all_buttons() -> void:
    """Release all buttons."""
	_current_input = 0

# =============================================================================
# SAVE MANAGEMENT
# =============================================================================

func save_game(slot: int = 0) -> bool:
    """
    Save the current game state.
    
    Args:
        slot: Save slot number (0-9)
    
    Returns:
        true if successful
    """
	var save_path = save_folder.path_join("save_%d.sav" % slot)
	# Implementation would call into GDExtension save functions
	print("Saving to: " + save_path)
	return true

func load_game(slot: int = 0) -> bool:
    """
    Load a saved game state.
    
    Args:
        slot: Save slot number (0-9)
    
    Returns:
        true if successful
    """
	var save_path = save_folder.path_join("save_%d.sav" % slot)
	if not FileAccess.file_exists(save_path):
		return false
	
	print("Loading from: " + save_path)
	return true

func get_save_slots() -> Array[int]:
    """Get list of available save slots."""
	var slots: Array[int] = []
	var dir = DirAccess.open(save_folder)
	
	if dir:
		dir.list_dir_begin()
		var file_name = dir.get_next()
		
		while file_name != "":
			if file_name.begins_with("save_") and file_name.ends_with(".sav"):
				var slot_str = file_name.trim_prefix("save_").trim_suffix(".sav")
				var slot = slot_str.to_int()
				if slot >= 0 and slot <= 9:
					slots.append(slot)
			
			file_name = dir.get_next()
	
	slots.sort()
	return slots

# =============================================================================
# RENDERING ACCESS
# =============================================================================

func get_frame_texture() -> ImageTexture:
    """Get the current frame texture from the renderer."""
	if _renderer:
		return _renderer.get_frame_texture()
	return null

func set_rendering_enabled(enabled: bool) -> void:
    """Enable or disable frame rendering."""
	if _renderer:
		_renderer.set_rendering_enabled(enabled)

# =============================================================================
# UTILITY
# =============================================================================

func verify_rom(expected_crc32: int) -> bool:
    """
    Verify the current ROM against an expected CRC32 checksum.
    
    Args:
        expected_crc32: The expected CRC32 value
    
    Returns:
        true if checksums match
    """
	return _patch_manager.verify_crc32(expected_crc32)

func get_last_error() -> String:
    """Get the last error message from the patch manager."""
	return _patch_manager.get_last_error()

func has_base_rom() -> bool:
    """Check if a base ROM is loaded."""
	return _patch_manager.has_base_rom()

func get_rom_size() -> int:
    """Get the size of the current ROM in bytes."""
	return _patch_manager.get_patched_size()

func get_patch_count() -> int:
    """Get the number of applied patches."""
	return _patch_manager.get_patch_count()
