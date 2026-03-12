extends Control

@onready var game_display: TextureRect = $GameDisplay
@onready var status_label: Label = $StatusLabel

var firered = null
var frame_count := 0
var frames_per_tick := 20
var snapshot_frames := {1: true, 16: true, 60: true, 120: true, 180: true, 300: true, 600: true, 900: true, 1200: true, 1500: true, 1650: true, 1750: true, 1800: true, 1850: true, 1900: true, 1950: true, 2000: true, 2100: true, 2400: true, 2700: true, 3000: true, 3500: true, 4000: true, 4500: true, 5000: true, 5500: true, 6000: true, 7000: true, 7500: true, 8000: true, 8500: true, 9000: true, 9500: true, 10000: true, 10500: true, 11000: true, 12000: true, 13000: true, 14000: true, 15000: true}
const FINAL_FRAME := 15000
var input_sequence := [
	{"start": 0, "end": 6800, "buttons": 0x0001, "pulse": true},
	{"start": 6800, "end": 7200, "buttons": 0x0000, "pulse": false},
	{"start": 7200, "end": 7264, "buttons": 0x0010, "pulse": false},
	{"start": 7264, "end": 7328, "buttons": 0x0040, "pulse": false},
	{"start": 7328, "end": 7428, "buttons": 0x0020, "pulse": false},
	{"start": 7428, "end": 7600, "buttons": 0x0000, "pulse": false},
	{"start": 7600, "end": 7664, "buttons": 0x0020, "pulse": false},
	{"start": 7664, "end": 7860, "buttons": 0x0080, "pulse": false},
	{"start": 7844, "end": 8500, "buttons": 0x0000, "pulse": false},
	{"start": 8500, "end": 9100, "buttons": 0x0080, "pulse": false},
	{"start": 9100, "end": 9200, "buttons": 0x0000, "pulse": false},
	{"start": 9200, "end": 9500, "buttons": 0x0001, "pulse": false},
	{"start": 9500, "end": FINAL_FRAME + 1, "buttons": 0x0000, "pulse": false},
]


func _input_override_for_frame(frame: int) -> int:
	for step in input_sequence:
		if frame >= step.start and frame < step.end:
			if step.pulse:
				var pulse: int = (frame - step.start) % 24
				if pulse < 4:
					return step.buttons
				return 0x0000
			return step.buttons

	return -1


func _write_smoke_artifacts(texture: Texture2D, suffix: String) -> void:
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


func _ready() -> void:
	if not ClassDB.class_exists("FireRedNode"):
		status_label.text = "FireRedNode class unavailable"
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

	game_display.custom_minimum_size = Vector2(480, 320)
	game_display.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	status_label.text = "ROM loaded, running real-time playback"
	set_process(true)


func _process(_delta: float) -> void:
	if firered == null:
		return

	for _i in range(frames_per_tick):
		if not firered.step_frame():
			status_label.text = "Frame %d failed" % frame_count
			push_error("FireRedNode failed stepping frame %d" % frame_count)
			set_process(false)
			return

		frame_count += 1

		var override := _input_override_for_frame(frame_count)
		if override >= 0:
			firered.set_input_override(override)
		else:
			firered.clear_input_override()

		if snapshot_frames.has(frame_count):
			var texture = firered.get_framebuffer_texture()
			if texture == null:
				status_label.text = "No framebuffer texture at frame %d" % frame_count
				push_error("FireRedNode returned no framebuffer texture at frame %d" % frame_count)
				set_process(false)
				return

			game_display.texture = texture
			_write_smoke_artifacts(texture, "frame_%d" % frame_count)

		if frame_count >= FINAL_FRAME:
			status_label.text = "Playback verification complete"
			set_process(false)
			get_tree().quit()
			return

	status_label.text = "Frame %d" % frame_count
