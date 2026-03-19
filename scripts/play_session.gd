extends Control

const GBA_A := 0x0001
const GBA_B := 0x0002
const GBA_SELECT := 0x0004
const GBA_START := 0x0008
const GBA_RIGHT := 0x0010
const GBA_LEFT := 0x0020
const GBA_UP := 0x0040
const GBA_DOWN := 0x0080
const GBA_R := 0x0100
const GBA_L := 0x0200

@onready var game_display: TextureRect = $GameDisplay
@onready var status_label: Label = $StatusLabel

var firered = null
var frame_count := 0
var held_keys := 0
var screenshot_requested := false
var quicksave_requested := false
var quickload_requested := false
var start_menu_pulse_frames := 0
var fps_title_timer := 0.0


func _ready() -> void:
	DisplayServer.window_set_title("PokeFired Portable")
	get_window().size = Vector2i(720, 480)

	if not ClassDB.class_exists("FireRedNode"):
		status_label.text = "FireRedNode unavailable"
		push_error("FireRedNode is not registered; extension likely failed to load")
		return

	firered = ClassDB.instantiate("FireRedNode")
	if firered == null:
		status_label.text = "FireRedNode instantiation failed"
		push_error("ClassDB.instantiate(FireRedNode) returned null")
		return

	add_child(firered)

	if not firered.load_rom("res://baserom.gba"):
		status_label.text = "ROM load failed"
		push_error("FireRedNode failed to load res://baserom.gba")
		return

	game_display.custom_minimum_size = Vector2(720, 480)
	game_display.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	status_label.text = _format_status("Frame 0")
	set_process(true)
	set_process_unhandled_input(true)


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey:
		var key_event: InputEventKey = event
		if key_event.pressed and not key_event.echo:
			if key_event.keycode == KEY_F1:
				start_menu_pulse_frames = 1
				get_viewport().set_input_as_handled()
				return
			if key_event.keycode == KEY_F5:
				quicksave_requested = true
				get_viewport().set_input_as_handled()
				return
			if key_event.keycode == KEY_F9:
				quickload_requested = true
				get_viewport().set_input_as_handled()
				return

		if key_event.keycode == KEY_F12 and key_event.pressed and not key_event.echo:
			screenshot_requested = true
			get_viewport().set_input_as_handled()
			return

		if key_event.echo:
			return

		var mask := _mask_for_key(key_event.keycode)
		if mask == 0:
			return

		if key_event.pressed:
			held_keys |= mask
		else:
			held_keys &= ~mask


func _process(delta: float) -> void:
	if firered == null:
		return

	fps_title_timer += delta
	if fps_title_timer >= 0.5:
		DisplayServer.window_set_title("PokeFired Portable - %d FPS" % int(Engine.get_frames_per_second()))
		fps_title_timer = 0.0

	if quicksave_requested:
		quicksave_requested = false
		if firered.save_state(_quick_state_path()):
			status_label.text = _format_status("Frame %d (quick saved)" % frame_count)
		else:
			push_warning("Quick save failed")

	if quickload_requested:
		quickload_requested = false
		held_keys = 0
		firered.clear_input_override()
		if firered.load_state(_quick_state_path()):
			var restored_texture: Texture2D = firered.get_framebuffer_texture()
			if restored_texture != null:
				game_display.texture = restored_texture
			status_label.text = _format_status("Frame %d (quick loaded)" % frame_count)
			return
		else:
			push_warning("Quick load failed")

	var input_mask := held_keys
	if start_menu_pulse_frames > 0:
		input_mask |= GBA_START
		start_menu_pulse_frames -= 1
		status_label.text = _format_status("Frame %d (opening menu)" % frame_count)

	firered.set_input_override(input_mask)
	if not firered.step_frame():
		status_label.text = _format_status("Frame %d failed" % frame_count)
		push_error("FireRedNode failed stepping frame %d" % frame_count)
		set_process(false)
		return

	frame_count += 1
	var texture: Texture2D = firered.get_framebuffer_texture()
	if texture == null:
		status_label.text = _format_status("No framebuffer texture at frame %d" % frame_count)
		push_error("FireRedNode returned no framebuffer texture at frame %d" % frame_count)
		set_process(false)
		return

	game_display.texture = texture
	status_label.text = _format_status("Frame %d" % frame_count)

	if screenshot_requested:
		_write_snapshot(texture)
		screenshot_requested = false


func _mask_for_key(keycode: Key) -> int:
	match keycode:
		KEY_Z:
			return GBA_A
		KEY_X:
			return GBA_B
		KEY_ENTER:
			return GBA_START
		KEY_BACKSPACE:
			return GBA_SELECT
		KEY_UP:
			return GBA_UP
		KEY_DOWN:
			return GBA_DOWN
		KEY_LEFT:
			return GBA_LEFT
		KEY_RIGHT:
			return GBA_RIGHT
		KEY_A:
			return GBA_L
		KEY_S:
			return GBA_R
		_:
			return 0


func _write_snapshot(texture: Texture2D) -> void:
	var suffix := "frame_%d" % frame_count
	var image := texture.get_image()
	var output_dir := ProjectSettings.globalize_path("res://build")
	var png_path := output_dir.path_join("runtime_smoke_%s.png" % suffix)
	var stats_path := output_dir.path_join("runtime_smoke_%s.txt" % suffix)
	var sample_points := [
		Vector2i(0, 0),
		Vector2i(image.get_width() / 2, image.get_height() / 2),
		Vector2i(image.get_width() - 1, image.get_height() - 1),
		Vector2i(image.get_width() / 4, image.get_height() / 4),
		Vector2i((image.get_width() * 3) / 4, (image.get_height() * 3) / 4),
	]
	var alpha_nonzero := 0
	var rgb_nonzero := 0
	var unique_samples: Array[String] = []
	var first_color := image.get_pixel(0, 0)
	var all_same := true
	var summary := PackedStringArray()

	DirAccess.make_dir_recursive_absolute(output_dir)
	image.save_png(png_path)

	for y in image.get_height():
		for x in image.get_width():
			var color := image.get_pixel(x, y)
			if color != first_color:
				all_same = false
			if color.a > 0.0:
				alpha_nonzero += 1
			if color.r > 0.0 or color.g > 0.0 or color.b > 0.0:
				rgb_nonzero += 1

	for point in sample_points:
		var point_color := image.get_pixel(point.x, point.y)
		var sample := "%d,%d=%.3f,%.3f,%.3f,%.3f" % [point.x, point.y, point_color.r, point_color.g, point_color.b, point_color.a]
		if not unique_samples.has(sample):
			unique_samples.append(sample)

	summary.append("size=%dx%d" % [image.get_width(), image.get_height()])
	summary.append("all_same=%s" % str(all_same))
	summary.append("alpha_nonzero=%d" % alpha_nonzero)
	summary.append("rgb_nonzero=%d" % rgb_nonzero)
	summary.append("samples=%s" % ", ".join(unique_samples))

	var file := FileAccess.open(stats_path, FileAccess.WRITE)
	if file != null:
		file.store_string("\n".join(summary) + "\n")
		file.close()

	status_label.text = _format_status("Frame %d (saved snapshot)" % frame_count)


func _format_status(base: String) -> String:
	return "%s | %d FPS" % [base, int(Engine.get_frames_per_second())]


func _quick_state_path() -> String:
	return ProjectSettings.globalize_path("user://quicksave.state")
