# =============================================================================
# Example Player Script
# =============================================================================
# Demonstrates usage of PokeFireRedModManager GDScript API
# Attach this to a Node in your scene
# =============================================================================

extends Node

@onready var mod_manager: PokeFireRedModManager = %PokeFireRedModManager
@onready var game_display: TextureRect = %GameDisplay
@onready var status_label: Label = $"../UI/MainMenu/VBoxContainer/StatusLabel"

var rom_loaded: bool = false
var patches_applied: bool = false

func _ready():
	# Connect to mod manager signals
	mod_manager.rom_loaded.connect(_on_rom_loaded)
	mod_manager.patch_applied.connect(_on_patch_applied)
	mod_manager.emulation_state_changed.connect(_on_emulation_state_changed)

func _on_load_rom_pressed():
	# File dialog for ROM selection
	var file_dialog = FileDialog.new()
	file_dialog.file_mode = FileDialog.FILE_MODE_OPEN_FILE
	file_dialog.access = FileDialog.ACCESS_FILESYSTEM
	file_dialog.filters = ["*.gba ; Game Boy Advance ROMs"]
	file_dialog.file_selected.connect(_on_rom_selected)
	add_child(file_dialog)
	file_dialog.popup_centered(Vector2(800, 600))

func _on_rom_selected(path: String):
	status_label.text = "Loading ROM..."
	var success = mod_manager.load_base_rom(path)
	if success:
		status_label.text = "ROM loaded successfully!"
	else:
		status_label.text = "Failed to load ROM!"

func _on_rom_loaded(success: bool, message: String):
	if success:
		rom_loaded = true
		print("ROM loaded: " + message)
	else:
		printerr("ROM load failed: " + message)

func _on_apply_patches_pressed():
	if not rom_loaded:
		status_label.text = "Load ROM first!"
		return
	
	# Example: Apply some patches
	var patches = [
		"res://patches/exp_share.bps",
        "res://patches/qol_improvements.bps"
	]
	
	for patch in patches:
		if FileAccess.file_exists(patch):
			mod_manager.apply_patch_file(patch)
	
	status_label.text = "Patches applied!"
	patches_applied = true

func _on_patch_applied(patch_name: String, success: bool):
	if success:
		print("Applied patch: " + patch_name)
	else:
		printerr("Failed to apply patch: " + patch_name)

func _on_start_pressed():
	if not rom_loaded:
		status_label.text = "Load ROM first!"
		return
	
	mod_manager.start_emulation()
	
	# Show game display
	game_display.visible = true
	$"../UI/MainMenu".visible = false
	
	# Get frame texture from renderer
	var texture = mod_manager.get_frame_texture()
	if texture:
		game_display.texture = texture

func _on_save_pressed():
	if mod_manager.get_emulation_state() == PokeFireRedModManager.EmulationState.RUNNING:
		mod_manager.save_game(0)
		status_label.text = "Game saved!"

func _on_load_save_pressed():
	if mod_manager.get_emulation_state() == PokeFireRedModManager.EmulationState.RUNNING:
		mod_manager.load_game(0)
		status_label.text = "Save loaded!"

func _on_emulation_state_changed(state: PokeFireRedModManager.EmulationState):
	match state:
		PokeFireRedModManager.EmulationState.RUNNING:
			print("Emulation started")
		PokeFireRedModManager.EmulationState.PAUSED:
			print("Emulation paused")
		PokeFireRedModManager.EmulationState.STOPPED:
			print("Emulation stopped")

func _input(event):
	# Emergency exit with Escape
	if event.is_action_pressed("ui_cancel"):
		mod_manager.stop_emulation()
		get_tree().quit()
