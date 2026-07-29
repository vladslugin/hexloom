extends Control

const BG := Color("#0B0F0D")
const SURFACE := Color("#111713")
const RAISED := Color("#17201B")
const BORDER := Color("#29362F")
const BORDER_SOFT := Color("#202B25")
const TEXT := Color("#D9E4DE")
const MUTED := Color("#83968C")
const DIM := Color("#53645B")
const LIVE := Color("#67E6A3")
const CYAN := Color("#73B8D4")
const AMBER := Color("#E5B566")
const CORAL := Color("#E27D76")

const TOP_H := 58.0
const RAIL_W := 224.0
const INSPECTOR_W := 318.0
const COMMAND_H := 82.0

var font: Font
var mono: Font
var command_field: LineEdit
var run_button: Button
var pause_button: Button
var selected_agent := 1
var pulse := 0.0
var toast_text := ""
var toast_until := 0


func _ready() -> void:
	font = ThemeDB.fallback_font
	mono = ThemeDB.fallback_font
	set_process(true)
	_create_controls()
	resized.connect(_layout_controls)
	_layout_controls()
	print("HEXLOOM_STUDIO_READY")
	await get_tree().process_frame
	await get_tree().process_frame
	await get_tree().process_frame
	_handle_automation_args()


func _process(delta: float) -> void:
	pulse = fmod(pulse + delta, 2.4)
	if toast_until > 0 and Time.get_ticks_msec() > toast_until:
		toast_until = 0
		toast_text = ""
	queue_redraw()


func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed:
		var mouse: Vector2 = event.position
		if mouse.x < RAIL_W and mouse.y > 244.0 and mouse.y < 428.0:
			selected_agent = clampi(int((mouse.y - 252.0) / 44.0), 0, 3)
			queue_redraw()


func _create_controls() -> void:
	command_field = LineEdit.new()
	command_field.placeholder_text = "Describe the next change to the world..."
	command_field.text = "Make the shrine silhouette colder and more ceremonial"
	command_field.add_theme_font_size_override("font_size", 14)
	command_field.add_theme_color_override("font_color", TEXT)
	command_field.add_theme_color_override("font_placeholder_color", DIM)
	command_field.add_theme_color_override("caret_color", LIVE)
	command_field.add_theme_stylebox_override(
		"normal",
		_style(BG, BORDER, 8, 14, 0)
	)
	command_field.add_theme_stylebox_override(
		"focus",
		_style(BG, LIVE.darkened(0.15), 8, 14, 0, 2)
	)
	command_field.text_submitted.connect(_run_weave)
	add_child(command_field)

	run_button = Button.new()
	run_button.text = "RUN WEAVE  ↵"
	run_button.add_theme_font_size_override("font_size", 12)
	run_button.add_theme_color_override("font_color", BG)
	run_button.add_theme_color_override("font_hover_color", BG)
	run_button.add_theme_color_override("font_pressed_color", BG)
	run_button.add_theme_stylebox_override(
		"normal",
		_style(LIVE, LIVE, 7, 13, 0)
	)
	run_button.add_theme_stylebox_override(
		"hover",
		_style(Color("#82EDB5"), Color("#82EDB5"), 7, 13, 0)
	)
	run_button.add_theme_stylebox_override(
		"pressed",
		_style(Color("#4FCB8B"), Color("#4FCB8B"), 7, 13, 0)
	)
	run_button.pressed.connect(func() -> void: _run_weave(command_field.text))
	add_child(run_button)

	pause_button = Button.new()
	pause_button.text = "Ⅱ  PAUSE"
	pause_button.add_theme_font_size_override("font_size", 11)
	pause_button.add_theme_color_override("font_color", MUTED)
	pause_button.add_theme_color_override("font_hover_color", TEXT)
	pause_button.add_theme_stylebox_override(
		"normal",
		_style(SURFACE, BORDER, 7, 11, 0)
	)
	pause_button.add_theme_stylebox_override(
		"hover",
		_style(RAISED, MUTED, 7, 11, 0)
	)
	pause_button.pressed.connect(_pause_weave)
	add_child(pause_button)


func _layout_controls() -> void:
	var w := size.x
	var inspector_x := w - INSPECTOR_W
	var command_y := size.y - COMMAND_H
	command_field.position = Vector2(RAIL_W + 24.0, command_y + 24.0)
	command_field.size = Vector2(
		maxf(320.0, inspector_x - RAIL_W - 252.0),
		42.0
	)
	run_button.position = Vector2(inspector_x - 138.0, command_y + 24.0)
	run_button.size = Vector2(122.0, 42.0)
	pause_button.position = Vector2(inspector_x - 106.0, 10.0)
	pause_button.size = Vector2(90.0, 36.0)
	queue_redraw()


func _style(
	background: Color,
	border_color: Color,
	radius: int,
	horizontal_padding: int,
	vertical_padding: int,
	border_width := 1
) -> StyleBoxFlat:
	var box := StyleBoxFlat.new()
	box.bg_color = background
	box.border_color = border_color
	box.set_border_width_all(border_width)
	box.set_corner_radius_all(radius)
	box.content_margin_left = horizontal_padding
	box.content_margin_right = horizontal_padding
	box.content_margin_top = vertical_padding
	box.content_margin_bottom = vertical_padding
	return box


func _run_weave(text: String) -> void:
	if text.strip_edges().is_empty():
		toast_text = "Add a direction before starting the weave."
	else:
		toast_text = "Direction queued for the Orchestrator."
	toast_until = Time.get_ticks_msec() + 2600
	queue_redraw()


func _pause_weave() -> void:
	toast_text = "Pause requested after the current safe checkpoint."
	toast_until = Time.get_ticks_msec() + 2600
	queue_redraw()


func _handle_automation_args() -> void:
	for argument in OS.get_cmdline_user_args():
		if argument == "--smoke-test":
			get_tree().quit(0)
			return
		if argument.begins_with("--capture="):
			var path := argument.trim_prefix("--capture=")
			var image := get_viewport().get_texture().get_image()
			var error := image.save_png(path)
			if error != OK:
				push_error("Could not save Hexloom Studio capture: %s" % error)
				get_tree().quit(1)
				return
			print("HEXLOOM_STUDIO_CAPTURED: " + path)
			get_tree().quit(0)
			return


func _draw() -> void:
	var w := size.x
	var h := size.y
	var inspector_x := w - INSPECTOR_W
	var command_y := h - COMMAND_H

	draw_rect(Rect2(Vector2.ZERO, size), BG)
	_draw_background_grid(
		Rect2(RAIL_W, TOP_H, inspector_x - RAIL_W, command_y - TOP_H)
	)
	_draw_top_bar(w, inspector_x)
	_draw_left_rail(command_y)
	_draw_work_area(inspector_x, command_y)
	_draw_inspector(inspector_x, command_y)
	_draw_command_bar(inspector_x, command_y)
	if not toast_text.is_empty():
		_draw_toast(inspector_x, command_y)


func _draw_background_grid(rect: Rect2) -> void:
	var grid_color := Color(0.16, 0.22, 0.19, 0.19)
	var step := 24.0
	var x := rect.position.x + 12.0
	while x < rect.end.x:
		draw_line(
			Vector2(x, rect.position.y),
			Vector2(x, rect.end.y),
			grid_color,
			1.0
		)
		x += step
	var y := rect.position.y + 8.0
	while y < rect.end.y:
		draw_line(
			Vector2(rect.position.x, y),
			Vector2(rect.end.x, y),
			grid_color,
			1.0
		)
		y += step


func _draw_top_bar(w: float, inspector_x: float) -> void:
	draw_rect(Rect2(0, 0, w, TOP_H), SURFACE)
	draw_line(Vector2(0, TOP_H), Vector2(w, TOP_H), BORDER, 1.0)
	draw_line(Vector2(RAIL_W, 0), Vector2(RAIL_W, TOP_H), BORDER, 1.0)
	draw_line(Vector2(inspector_x, 0), Vector2(inspector_x, TOP_H), BORDER, 1.0)

	_draw_hex_mark(Vector2(28, 17), 12.0)
	_text("HEXLOOM", Vector2(51, 25), 14, TEXT, true)
	_text("STUDIO", Vector2(51, 41), 9, MUTED, true)

	_text("⌘", Vector2(RAIL_W + 22, 35), 13, MUTED, true)
	_text("coastal-observatory", Vector2(RAIL_W + 44, 29), 13, TEXT)
	_text("/  world loom", Vector2(RAIL_W + 181, 29), 12, MUTED)
	_badge(Vector2(RAIL_W + 302, 17), "MAIN", CYAN, false)

	var provider_x := inspector_x + 17.0
	_live_dot(Vector2(provider_x + 4, 27), LIVE)
	_text("ANTIGRAVITY", Vector2(provider_x + 17, 25), 10, TEXT, true)
	_text("CONNECTED", Vector2(provider_x + 17, 40), 9, MUTED, true)


func _draw_left_rail(command_y: float) -> void:
	draw_rect(Rect2(0, TOP_H, RAIL_W, command_y - TOP_H), SURFACE)
	draw_line(Vector2(RAIL_W, TOP_H), Vector2(RAIL_W, command_y), BORDER, 1.0)

	_label("WORKSPACE", Vector2(20, 88))
	var navigation := [
		["⌁", "World Loom", true],
		["⌘", "Command Deck", false],
		["◇", "Artifacts", false],
		["◫", "Playground", false]
	]
	var nav_y := 108.0
	for item in navigation:
		_nav_item(nav_y, item[0], item[1], item[2])
		nav_y += 38.0

	_label("AGENT CREW", Vector2(20, 264))
	var agents := [
		["OR", "Orchestrator", "routing", LIVE],
		["AR", "Artisan", "generating", CYAN],
		["EN", "Engineer", "ready", MUTED],
		["QA", "Sentinel", "watching", AMBER]
	]
	var agent_y := 282.0
	for i in agents.size():
		_agent_row(agent_y, agents[i], i == selected_agent)
		agent_y += 44.0

	var memory_y := command_y - 139.0
	draw_line(Vector2(20, memory_y), Vector2(RAIL_W - 20, memory_y), BORDER, 1.0)
	_label("PROJECT MEMORY", Vector2(20, memory_y + 25))
	_text("24", Vector2(20, memory_y + 54), 24, TEXT)
	_text("durable decisions", Vector2(57, memory_y + 50), 11, MUTED)
	_text("style DNA · 8", Vector2(20, memory_y + 77), 10, CYAN, true)
	_text("mechanics · 11", Vector2(112, memory_y + 77), 10, AMBER, true)
	_text("↗  Memory is shared with every agent", Vector2(20, memory_y + 104), 10, MUTED)


func _draw_work_area(inspector_x: float, command_y: float) -> void:
	var x0 := RAIL_W
	var width := inspector_x - x0
	var map_bottom := minf(493.0, command_y - 260.0)

	_text("WORLD LOOM", Vector2(x0 + 24, 88), 10, MUTED, true)
	_text(
		"Weave a colder shrine silhouette",
		Vector2(x0 + 24, 116),
		22,
		TEXT
	)
	_text(
		"One intent, four agents, six inspectable artifacts",
		Vector2(x0 + 24, 139),
		12,
		MUTED
	)
	_badge(Vector2(inspector_x - 105, 78), "● WEAVING", LIVE, true)

	var intent := Vector2(x0 + 60, 232)
	var router := Vector2(x0 + width * 0.37, 232)
	var artisan := Vector2(x0 + width * 0.62, 186)
	var engineer := Vector2(x0 + width * 0.62, 278)
	var artifact := Vector2(inspector_x - 84, 232)

	_thread(intent + Vector2(80, 0), router - Vector2(55, 0), LIVE, true)
	_thread(router + Vector2(55, -7), artisan - Vector2(61, 0), CYAN, true)
	_thread(router + Vector2(55, 8), engineer - Vector2(61, 0), MUTED, false)
	_thread(artisan + Vector2(61, 0), artifact - Vector2(58, 0), CYAN, true)
	_thread(engineer + Vector2(61, 0), artifact - Vector2(58, 0), DIM, false)

	_node(intent, Vector2(160, 82), "INTENT", "Shrine silhouette", "01", LIVE)
	_node(router, Vector2(110, 72), "ROUTER", "Plan split", "02", LIVE)
	_node(artisan, Vector2(122, 72), "ARTISAN", "Mesh draft", "03", CYAN)
	_node(engineer, Vector2(122, 72), "ENGINEER", "Await mesh", "04", MUTED)
	_node(artifact, Vector2(116, 82), "OUTPUT", "shrine.glb", "05", AMBER)
	_draw_weave_trace(
		Rect2(x0 + 24, map_bottom - 83, width - 48, 52)
	)

	_text("LIVE EXECUTION", Vector2(x0 + 24, map_bottom + 31), 10, MUTED, true)
	_text("5 events", Vector2(inspector_x - 70, map_bottom + 31), 10, MUTED, true)
	draw_line(
		Vector2(x0, map_bottom),
		Vector2(inspector_x, map_bottom),
		BORDER,
		1.0
	)

	var stream_y := map_bottom + 54.0
	var stream_width := width - 48.0
	_event_row(
		Rect2(x0 + 24, stream_y, stream_width, 50),
		"AR",
		CYAN,
		"Artisan",
		"Generating a low-poly blockout with ceremonial proportions",
		"NOW",
		true
	)
	_event_row(
		Rect2(x0 + 24, stream_y + 56, stream_width, 46),
		"OR",
		LIVE,
		"Orchestrator",
		"Split direction into mesh, material, lighting, and validation tasks",
		"12s",
		false
	)
	_event_row(
		Rect2(x0 + 24, stream_y + 108, stream_width, 46),
		"↗",
		CYAN,
		"Context",
		"Attached 8 style rules and 3 reference artifacts",
		"14s",
		false
	)
	_event_row(
		Rect2(x0 + 24, stream_y + 160, stream_width, 46),
		"✓",
		LIVE,
		"Sentinel",
		"Baseline scene passed 18 checks before modification",
		"19s",
		false
	)


func _draw_weave_trace(rect: Rect2) -> void:
	draw_rect(rect, Color("#0F1612"), true)
	draw_rect(rect, BORDER_SOFT, false, 1.0)
	_text("WEAVE TRACE", rect.position + Vector2(13, 20), 9, MUTED, true)
	_text("42%", rect.position + Vector2(13, 39), 14, TEXT, true)

	var track_x := rect.position.x + 67.0
	var track_y := rect.position.y + 28.0
	var track_width := rect.size.x - 304.0
	draw_rect(Rect2(track_x, track_y, track_width, 3), BORDER_SOFT, true)
	draw_rect(Rect2(track_x, track_y, track_width * 0.42, 3), LIVE, true)
	for step in 6:
		var step_x := track_x + track_width * float(step) / 5.0
		var completed := step <= 2
		draw_circle(
			Vector2(step_x, track_y + 1.5),
			3.0 if completed else 2.0,
			LIVE if completed else BORDER
		)

	var detail_x := rect.end.x - 210.0
	_text("03 / 06", Vector2(detail_x, rect.position.y + 21), 9, CYAN, true)
	_text(
		"mesh blockout in progress",
		Vector2(detail_x, rect.position.y + 39),
		10,
		MUTED
	)
	_text(
		"ETA  00:38",
		Vector2(rect.end.x - 74, rect.position.y + 31),
		9,
		AMBER,
		true
	)


func _draw_inspector(inspector_x: float, command_y: float) -> void:
	var width := size.x - inspector_x
	draw_rect(
		Rect2(inspector_x, TOP_H, width, command_y - TOP_H),
		SURFACE
	)
	draw_line(
		Vector2(inspector_x, TOP_H),
		Vector2(inspector_x, command_y),
		BORDER,
		1.0
	)

	var x := inspector_x + 20.0
	_label("LIVE CONTEXT", Vector2(x, 88))
	_text("Coastal Observatory", Vector2(x, 118), 18, TEXT)
	_text("world / visual language", Vector2(x, 139), 11, MUTED)

	_draw_section_line(inspector_x, 159.0, width)
	_label("STYLE DNA", Vector2(x, 187))
	_property_row(x, 213, "geometry", "low-poly · soft bevel")
	_property_row(x, 239, "palette", "cold stone · warm fire")
	_property_row(x, 265, "silhouette", "exaggerated · readable")
	_property_row(x, 291, "surface", "stylized PBR")

	_draw_section_line(inspector_x, 316.0, width)
	_label("CONSTRAINTS IN PLAY", Vector2(x, 345))
	_constraint(x, 372, "01", "Mobile triangle budget", "≤ 18k", LIVE)
	_constraint(x, 414, "02", "Shrine remains traversable", "LOCKED", CYAN)
	_constraint(x, 456, "03", "No photoreal materials", "STYLE", AMBER)

	_draw_section_line(inspector_x, 492.0, width)
	_label("ARTIFACTS", Vector2(x, 521))
	_artifact_row(x, 548, "◇", "shrine_blockout.glb", "writing", CYAN)
	_artifact_row(x, 588, "▦", "cold_stone_albedo", "queued", MUTED)
	_artifact_row(x, 628, "⌁", "shrine_test.tscn", "waiting", MUTED)

	var note_y := command_y - 109.0
	draw_rect(
		Rect2(x, note_y, width - 40.0, 84.0),
		RAISED,
		true
	)
	draw_rect(
		Rect2(x, note_y, width - 40.0, 84.0),
		BORDER,
		false,
		1.0
	)
	_text("✦  WORLD MEMORY", Vector2(x + 13, note_y + 24), 10, LIVE, true)
	_text("The last two games favored", Vector2(x + 13, note_y + 47), 11, TEXT)
	_text("broad silhouettes over fine detail.", Vector2(x + 13, note_y + 65), 11, MUTED)


func _draw_command_bar(inspector_x: float, command_y: float) -> void:
	draw_rect(
		Rect2(0, command_y, size.x, COMMAND_H),
		Color("#0E1411")
	)
	draw_line(
		Vector2(0, command_y),
		Vector2(size.x, command_y),
		BORDER,
		1.0
	)
	draw_line(
		Vector2(RAIL_W, command_y),
		Vector2(RAIL_W, size.y),
		BORDER,
		1.0
	)
	draw_line(
		Vector2(inspector_x, command_y),
		Vector2(inspector_x, size.y),
		BORDER,
		1.0
	)
	_text("⌘", Vector2(24, command_y + 36), 18, LIVE, true)
	_text("DIRECT THE LOOM", Vector2(50, command_y + 29), 10, TEXT, true)
	_text("Natural language · project-aware", Vector2(50, command_y + 47), 10, MUTED)
	_text("scope: world", Vector2(inspector_x + 20, command_y + 31), 10, MUTED, true)
	_text("⌘ K  commands", Vector2(inspector_x + 20, command_y + 51), 10, DIM, true)


func _draw_toast(inspector_x: float, command_y: float) -> void:
	var toast_size := Vector2(365, 46)
	var toast_pos := Vector2(
		inspector_x - toast_size.x - 16.0,
		command_y - toast_size.y - 14.0
	)
	draw_rect(Rect2(toast_pos, toast_size), RAISED, true)
	draw_rect(Rect2(toast_pos, toast_size), LIVE.darkened(0.45), false, 1.0)
	_text("✓", toast_pos + Vector2(14, 29), 13, LIVE, true)
	_text(toast_text, toast_pos + Vector2(38, 28), 11, TEXT)


func _draw_hex_mark(position: Vector2, radius: float) -> void:
	var points := PackedVector2Array()
	for i in 6:
		var angle := deg_to_rad(60.0 * i - 30.0)
		points.append(position + Vector2(cos(angle), sin(angle)) * radius)
	points.append(points[0])
	draw_polyline(points, LIVE, 1.5, true)
	draw_circle(position, 3.0, LIVE)


func _nav_item(y: float, icon: String, title: String, active: bool) -> void:
	if active:
		draw_rect(Rect2(12, y, RAIL_W - 24, 32), RAISED, true)
		draw_rect(Rect2(12, y, RAIL_W - 24, 32), BORDER, false, 1.0)
	_text(icon, Vector2(24, y + 21), 12, LIVE if active else MUTED, true)
	_text(title, Vector2(49, y + 21), 12, TEXT if active else MUTED)
	if active:
		_text("⌘1", Vector2(RAIL_W - 48, y + 21), 9, DIM, true)


func _agent_row(y: float, agent: Array, selected: bool) -> void:
	if selected:
		draw_rect(Rect2(12, y, RAIL_W - 24, 38), RAISED, true)
		draw_rect(Rect2(12, y, RAIL_W - 24, 38), BORDER, false, 1.0)
	draw_circle(Vector2(31, y + 19), 12.0, Color(agent[3], 0.12))
	_text(agent[0], Vector2(24, y + 22), 9, agent[3], true)
	_text(agent[1], Vector2(51, y + 17), 11, TEXT if selected else MUTED)
	_text(agent[2], Vector2(51, y + 31), 9, agent[3], true)
	if agent[2] == "generating":
		_live_dot(Vector2(RAIL_W - 27, y + 18), agent[3])
	else:
		draw_circle(Vector2(RAIL_W - 27, y + 18), 3.0, agent[3])


func _node(
	center: Vector2,
	node_size: Vector2,
	kind: String,
	title: String,
	number: String,
	accent: Color
) -> void:
	var rect := Rect2(center - node_size * 0.5, node_size)
	draw_rect(rect, Color("#121A16"), true)
	draw_rect(rect, BORDER, false, 1.0)
	draw_circle(rect.position + Vector2(16, 17), 3.0, accent)
	_text(kind, rect.position + Vector2(26, 21), 9, accent, true)
	_text(title, rect.position + Vector2(12, 48), 11, TEXT)
	_text(number, rect.end - Vector2(24, 10), 8, DIM, true)


func _thread(
	start: Vector2,
	end: Vector2,
	color: Color,
	active: bool
) -> void:
	var midpoint_x := (start.x + end.x) * 0.5
	var points := PackedVector2Array([
		start,
		Vector2(midpoint_x, start.y),
		Vector2(midpoint_x, end.y),
		end
	])
	draw_polyline(points, Color(color, 0.76 if active else 0.34), 1.5, true)
	if active:
		var t := fmod(pulse / 2.4 + start.x * 0.0003, 1.0)
		var moving := start.lerp(end, t)
		draw_circle(moving, 3.0, color)


func _event_row(
	rect: Rect2,
	glyph: String,
	accent: Color,
	actor: String,
	message: String,
	time: String,
	active: bool
) -> void:
	if active:
		draw_rect(rect, Color("#121B17"), true)
		draw_rect(rect, BORDER, false, 1.0)
	draw_circle(rect.position + Vector2(20, rect.size.y * 0.5), 11.0, Color(accent, 0.12))
	_text(glyph, rect.position + Vector2(14, rect.size.y * 0.5 + 4), 9, accent, true)
	_text(actor, rect.position + Vector2(42, 20), 10, accent, true)
	_text(message, rect.position + Vector2(42, 38), 11, TEXT if active else MUTED)
	_text(time, Vector2(rect.end.x - 35, rect.position.y + 28), 9, MUTED, true)


func _property_row(x: float, y: float, key: String, value: String) -> void:
	_text(key, Vector2(x, y), 10, MUTED, true)
	_text(value, Vector2(x + 92, y), 11, TEXT)


func _constraint(
	x: float,
	y: float,
	number: String,
	title: String,
	state: String,
	color: Color
) -> void:
	draw_circle(Vector2(x + 12, y - 4), 11.0, Color(color, 0.12))
	_text(number, Vector2(x + 6, y), 8, color, true)
	_text(title, Vector2(x + 33, y - 4), 11, TEXT)
	_text(state, Vector2(x + 33, y + 12), 8, color, true)


func _artifact_row(
	x: float,
	y: float,
	glyph: String,
	title: String,
	state: String,
	color: Color
) -> void:
	draw_rect(Rect2(x, y - 17, INSPECTOR_W - 40, 34), RAISED, true)
	_text(glyph, Vector2(x + 11, y + 4), 11, color, true)
	_text(title, Vector2(x + 34, y + 3), 10, TEXT)
	_text(state, Vector2(x + 205, y + 3), 8, color, true)


func _badge(
	position: Vector2,
	label_text: String,
	color: Color,
	filled: bool
) -> void:
	var badge_width := mono.get_string_size(label_text, HORIZONTAL_ALIGNMENT_LEFT, -1, 9).x + 18.0
	var rect := Rect2(position, Vector2(badge_width, 24))
	draw_rect(rect, Color(color, 0.12 if filled else 0.04), true)
	draw_rect(rect, Color(color, 0.45), false, 1.0)
	_text(label_text, position + Vector2(9, 16), 9, color, true)


func _live_dot(position: Vector2, color: Color) -> void:
	var wave := (sin(pulse * TAU / 2.4) + 1.0) * 0.5
	draw_circle(position, 6.0 + wave * 2.0, Color(color, 0.06 + wave * 0.05))
	draw_circle(position, 3.0, color)


func _draw_section_line(x: float, y: float, width: float) -> void:
	draw_line(Vector2(x, y), Vector2(x + width, y), BORDER_SOFT, 1.0)


func _label(label_text: String, position: Vector2) -> void:
	_text(label_text, position, 9, MUTED, true)


func _text(
	value: String,
	position: Vector2,
	font_size: int,
	color: Color,
	is_mono := false
) -> void:
	draw_string(
		mono if is_mono else font,
		position,
		value,
		HORIZONTAL_ALIGNMENT_LEFT,
		-1,
		font_size,
		color
	)
