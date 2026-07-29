extends Node3D

const MATERIAL_ID := "ancient_stone_floor"
const STYLE_ID := "soft_neon_scifi"

var validation_result: Dictionary

func _ready() -> void:
	validation_result = _validate_with_cpp()
	_create_environment()
	_create_preview_object()
	_create_interface()
	print(
		"Hexloom Material Lab ready: material=%s style=%s valid=%s"
		% [MATERIAL_ID, STYLE_ID, validation_result.get("valid", false)]
	)


func _validate_with_cpp() -> Dictionary:
	var bridge := HexloomMaterialBridge.new()
	var specification_path := OS.get_environment("HEXLOOM_SPEC_PATH")
	if specification_path.is_empty():
		specification_path = ProjectSettings.globalize_path(
			"res://../../games/material-lab/game.yaml"
		)
	return bridge.validate_material_file(specification_path)


func _create_environment() -> void:
	var world_environment := WorldEnvironment.new()
	var environment := Environment.new()
	environment.background_mode = Environment.BG_COLOR
	environment.background_color = Color("#10131f")
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.ambient_light_color = Color("#7a8bb8")
	environment.ambient_light_energy = 0.7
	world_environment.environment = environment
	add_child(world_environment)

	var key_light := DirectionalLight3D.new()
	key_light.rotation_degrees = Vector3(-48.0, -32.0, 0.0)
	key_light.light_color = Color("#b6c8ff")
	key_light.light_energy = 1.4
	key_light.shadow_enabled = true
	add_child(key_light)

	var camera := Camera3D.new()
	camera.position = Vector3(3.6, 2.7, 4.6)
	camera.look_at_from_position(camera.position, Vector3.ZERO)
	add_child(camera)


func _create_preview_object() -> void:
	var mesh_instance := MeshInstance3D.new()
	var sphere := SphereMesh.new()
	sphere.radius = 1.25
	sphere.height = 2.5
	sphere.radial_segments = 96
	sphere.rings = 48
	mesh_instance.mesh = sphere

	var material := StandardMaterial3D.new()
	var artifact_directory := OS.get_environment(
		"HEXLOOM_ARTIFACT_DIRECTORY"
	)
	if artifact_directory.is_empty():
		material.albedo_color = Color("#5e719d")
		material.metallic = 0.15
		material.roughness = 0.72
	else:
		if not _apply_texture_artifact(material, artifact_directory):
			get_tree().quit(1)
			return
	mesh_instance.material_override = material
	add_child(mesh_instance)


func _apply_texture_artifact(
	material: StandardMaterial3D,
	artifact_directory: String
) -> bool:
	var resolution := int(validation_result.get("resolution", 0))
	var material_id := str(validation_result.get("material_id", ""))
	if resolution <= 0 or material_id.is_empty():
		push_error("Hexloom artifact metadata is invalid")
		return false

	var albedo := _load_raw_texture(
		artifact_directory.path_join(material_id + "_albedo.rgba8"),
		resolution
	)
	var normal := _load_raw_texture(
		artifact_directory.path_join(material_id + "_normal.rgba8"),
		resolution
	)
	var roughness := _load_raw_texture(
		artifact_directory.path_join(material_id + "_roughness.rgba8"),
		resolution
	)
	if albedo == null or normal == null or roughness == null:
		return false

	material.albedo_texture = albedo
	material.normal_enabled = true
	material.normal_texture = normal
	material.roughness = 1.0
	material.roughness_texture = roughness
	material.texture_filter = BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS
	print(
		"Hexloom texture artifact loaded: material=%s resolution=%d"
		% [material_id, resolution]
	)
	return true


func _load_raw_texture(path: String, resolution: int) -> ImageTexture:
	if not FileAccess.file_exists(path):
		push_error("Hexloom texture map is missing: " + path)
		return null

	var bytes := FileAccess.get_file_as_bytes(path)
	var expected_size := resolution * resolution * 4
	if bytes.size() != expected_size:
		push_error(
			"Hexloom texture map has invalid size: %s (%d != %d)"
			% [path, bytes.size(), expected_size]
		)
		return null

	var image := Image.create_from_data(
		resolution,
		resolution,
		false,
		Image.FORMAT_RGBA8,
		bytes
	)
	image.generate_mipmaps()
	return ImageTexture.create_from_image(image)


func _create_interface() -> void:
	var panel := PanelContainer.new()
	panel.position = Vector2(28.0, 28.0)
	panel.custom_minimum_size = Vector2(390.0, 0.0)

	var margin := MarginContainer.new()
	margin.add_theme_constant_override("margin_left", 20)
	margin.add_theme_constant_override("margin_top", 16)
	margin.add_theme_constant_override("margin_right", 20)
	margin.add_theme_constant_override("margin_bottom", 16)
	panel.add_child(margin)

	var label := Label.new()
	var validation_status := (
		"validated by C++ core"
		if validation_result.get("valid", false)
		else "validation failed"
	)
	label.text = (
		"HEXLOOM / MATERIAL LAB\n\n"
		+ "Material: %s\n" % MATERIAL_ID
		+ "Style: %s\n" % STYLE_ID
		+ "Status: %s\n\n" % validation_status
		+ "Next: connect Texture Agent output"
	)
	label.add_theme_font_size_override("font_size", 18)
	margin.add_child(label)

	add_child(panel)
