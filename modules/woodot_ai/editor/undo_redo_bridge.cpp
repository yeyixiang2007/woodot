/**************************************************************************/
/*  undo_redo_bridge.cpp                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "modules/woodot_ai/editor/undo_redo_bridge.h"

#include "core/io/file_access.h"
#include "core/object/class_db.h"
#include "editor/editor_interface.h"
#include "editor/editor_undo_redo_manager.h"
#include "modules/woodot_ai/resources/gdscript_repair_patch.h"
#include "modules/woodot_ai/resources/scene_synthesis_plan.h"
#include "scene/main/node.h"

#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"

UndoRedoBridge *UndoRedoBridge::singleton = nullptr;

static Array _make_scene_plan_supported_operations() {
	Array operations;
	operations.push_back(String("create_node"));
	operations.push_back(String("set_property"));
	return operations;
}

void UndoRedoBridge::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_attach_scene_node", "parent_path", "node"), &UndoRedoBridge::_attach_scene_node);
	ClassDB::bind_method(D_METHOD("_detach_scene_node", "parent_path", "node"), &UndoRedoBridge::_detach_scene_node);
	ClassDB::bind_method(D_METHOD("_set_scene_node_owner", "node"), &UndoRedoBridge::_set_scene_node_owner);
	ClassDB::bind_method(D_METHOD("_write_text_file", "path", "contents"), &UndoRedoBridge::_write_text_file);
	ClassDB::bind_method(D_METHOD("can_apply_scene_plan", "plan"), &UndoRedoBridge::can_apply_scene_plan);
	ClassDB::bind_method(D_METHOD("apply_scene_plan", "plan"), &UndoRedoBridge::apply_scene_plan);
	ClassDB::bind_method(D_METHOD("can_apply_gdscript_patch", "patch"), &UndoRedoBridge::can_apply_gdscript_patch);
	ClassDB::bind_method(D_METHOD("apply_gdscript_patch", "patch"), &UndoRedoBridge::apply_gdscript_patch);
	ClassDB::bind_method(D_METHOD("get_bridge_status"), &UndoRedoBridge::get_bridge_status);
}

UndoRedoBridge *UndoRedoBridge::get_singleton() {
	return singleton;
}

void UndoRedoBridge::_write_text_file(const String &p_path, const String &p_contents) {
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_MSG(file.is_null(), vformat("UndoRedoBridge could not open '%s' for writing.", p_path));
	file->store_string(p_contents);
}

Dictionary UndoRedoBridge::can_apply_scene_plan(const Ref<SceneSynthesisPlan> &p_plan) const {
	if (p_plan.is_null()) {
		return _make_status(ERR_INVALID_PARAMETER, "SceneSynthesisPlan is null.", false);
	}
	if (EditorInterface::get_singleton() == nullptr) {
		return _make_status(ERR_UNAVAILABLE, "EditorInterface is unavailable.", false);
	}

	Node *edited_scene_root = _get_edited_scene_root();
	if (edited_scene_root == nullptr) {
		return _make_status(ERR_UNAVAILABLE, "No edited scene root is available.", false);
	}

	if (!p_plan->get_resource_operations().is_empty()) {
		return _make_status(ERR_UNAVAILABLE, "Scene plan resource operations are not supported yet.", false);
	}

	HashMap<String, Node *> planned_nodes;
	LocalVector<Node *> temporary_nodes;
	auto cleanup_temporary_nodes = [&temporary_nodes]() {
		for (Node *temporary_node : temporary_nodes) {
			memdelete(temporary_node);
		}
	};
	const Array operations = p_plan->get_node_operations();
	for (int32_t i = 0; i < operations.size(); i++) {
		if (operations[i].get_type() != Variant::DICTIONARY) {
			cleanup_temporary_nodes();
			return _make_status(ERR_INVALID_DATA, vformat("node_operations[%d] must be a dictionary.", i), false);
		}

		const Dictionary operation = operations[i];
		const String op_name = operation.has("op") ? String(operation["op"]) : String();
		if (op_name == "create_node") {
			const String parent_path = operation.has("parent_path") ? String(operation["parent_path"]) : String();
			const String node_type = operation.has("node_type") ? String(operation["node_type"]) : String();
			const String node_name = operation.has("name") ? String(operation["name"]) : String();
			if (node_type.is_empty() || node_name.is_empty()) {
				cleanup_temporary_nodes();
				return _make_status(ERR_INVALID_DATA, vformat("create_node operation %d requires node_type and name.", i), false);
			}
			const bool parent_exists = parent_path.is_empty() || parent_path == "." || planned_nodes.has(parent_path) || _resolve_scene_node(parent_path) != nullptr;
			if (!parent_exists) {
				cleanup_temporary_nodes();
				return _make_status(ERR_DOES_NOT_EXIST, vformat("create_node operation %d parent '%s' does not exist.", i, parent_path), false);
			}
			if (!ClassDB::class_exists(node_type) || !ClassDB::is_parent_class(node_type, "Node") || !ClassDB::can_instantiate(node_type)) {
				cleanup_temporary_nodes();
				return _make_status(ERR_INVALID_DATA, vformat("create_node operation %d uses unsupported node type '%s'.", i, node_type), false);
			}
			const String new_path = _join_scene_path(parent_path, node_name);
			if (planned_nodes.has(new_path) || _resolve_scene_node(new_path) != nullptr) {
				cleanup_temporary_nodes();
				return _make_status(ERR_ALREADY_EXISTS, vformat("create_node operation %d would overwrite existing path '%s'.", i, new_path), false);
			}
			Object *instantiated = ClassDB::instantiate(node_type);
			if (instantiated == nullptr) {
				cleanup_temporary_nodes();
				return _make_status(ERR_CANT_CREATE, vformat("create_node operation %d could not instantiate '%s'.", i, node_type), false);
			}
			Node *planned_node = Object::cast_to<Node>(instantiated);
			if (planned_node == nullptr) {
				if (!instantiated->is_ref_counted()) {
					memdelete(instantiated);
				}
				cleanup_temporary_nodes();
				return _make_status(ERR_CANT_CREATE, vformat("create_node operation %d instantiated a non-Node object.", i), false);
			}
			planned_node->set_name(node_name);
			planned_nodes.insert(new_path, planned_node);
			temporary_nodes.push_back(planned_node);
		} else if (op_name == "set_property") {
			const String target_path = operation.has("target_path") ? String(operation["target_path"]) : String();
			const String property_name = operation.has("property") ? String(operation["property"]) : String();
			if (target_path.is_empty() || property_name.is_empty()) {
				cleanup_temporary_nodes();
				return _make_status(ERR_INVALID_DATA, vformat("set_property operation %d requires target_path and property.", i), false);
			}
			Node *target_node = planned_nodes.has(target_path) ? planned_nodes[target_path] : _resolve_scene_node(target_path);
			if (target_node == nullptr) {
				cleanup_temporary_nodes();
				return _make_status(ERR_DOES_NOT_EXIST, vformat("set_property operation %d target '%s' does not exist.", i, target_path), false);
			}
			bool property_valid = false;
			target_node->get(StringName(property_name), &property_valid);
			if (!property_valid) {
				cleanup_temporary_nodes();
				return _make_status(ERR_INVALID_DATA, vformat("set_property operation %d property '%s' does not exist on '%s'.", i, property_name, target_path), false);
			}
		} else {
			cleanup_temporary_nodes();
			return _make_status(ERR_UNAVAILABLE, vformat("Scene plan operation '%s' is not supported in the MVP apply path.", op_name), false);
		}
	}

	cleanup_temporary_nodes();

	Dictionary status = _make_status(OK, "Scene plan can be applied through the MVP bridge.", true);
	status["supported_operations"] = _make_scene_plan_supported_operations();
	status["operation_count"] = operations.size();
	return status;
}

Dictionary UndoRedoBridge::apply_scene_plan(const Ref<SceneSynthesisPlan> &p_plan) {
	const Dictionary applicability = can_apply_scene_plan(p_plan);
	const bool can_apply = applicability.has("can_apply") ? bool(applicability["can_apply"]) : false;
	if (!can_apply) {
		return applicability;
	}

	Node *edited_scene_root = _get_edited_scene_root();
	ERR_FAIL_NULL_V_MSG(edited_scene_root, _make_status(ERR_UNAVAILABLE, "No edited scene root is available.", false), "UndoRedoBridge requires an edited scene root.");

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	ERR_FAIL_NULL_V_MSG(undo_redo, _make_status(ERR_UNAVAILABLE, "EditorUndoRedoManager is unavailable.", false), "UndoRedoBridge requires EditorUndoRedoManager.");

	HashMap<String, Node *> planned_nodes;
	const Array operations = p_plan->get_node_operations();

	undo_redo->create_action("Apply AI scene plan", UndoRedo::MERGE_DISABLE, edited_scene_root);
	for (int32_t i = 0; i < operations.size(); i++) {
		const Dictionary operation = operations[i];
		const String op_name = operation.has("op") ? String(operation["op"]) : String();
		if (op_name == "create_node") {
			const String parent_path = operation.has("parent_path") ? String(operation["parent_path"]) : String();
			const String node_type = operation.has("node_type") ? String(operation["node_type"]) : String();
			const String node_name = operation.has("name") ? String(operation["name"]) : String();

			Object *instantiated = ClassDB::instantiate(node_type);
			ERR_FAIL_NULL_V_MSG(instantiated, _make_status(ERR_CANT_CREATE, vformat("Could not instantiate node type '%s'.", node_type), false), "UndoRedoBridge could not instantiate the requested scene node.");
			Node *new_node = Object::cast_to<Node>(instantiated);
			ERR_FAIL_NULL_V_MSG(new_node, _make_status(ERR_CANT_CREATE, vformat("Type '%s' is not a Node.", node_type), false), "UndoRedoBridge instantiated a non-Node object for scene plan apply.");
			new_node->set_name(node_name);

			const String new_path = _join_scene_path(parent_path, node_name);
			planned_nodes.insert(new_path, new_node);

			undo_redo->add_do_method(this, SNAME("_attach_scene_node"), parent_path, new_node);
			undo_redo->add_do_method(this, SNAME("_set_scene_node_owner"), new_node);
			undo_redo->add_do_reference(new_node);
			undo_redo->add_undo_method(this, SNAME("_detach_scene_node"), parent_path, new_node);
		} else if (op_name == "set_property") {
			const String target_path = operation.has("target_path") ? String(operation["target_path"]) : String();
			const StringName property_name = operation.has("property") ? String(operation["property"]) : String();
			const Variant new_value = operation.has("value") ? operation["value"] : Variant();

			Node *target_node = planned_nodes.has(target_path) ? planned_nodes[target_path] : _resolve_scene_node(target_path);
			ERR_FAIL_NULL_V_MSG(target_node, _make_status(ERR_DOES_NOT_EXIST, vformat("Scene node '%s' does not exist.", target_path), false), "UndoRedoBridge could not resolve the requested scene node.");

			bool property_valid = false;
			const Variant old_value = target_node->get(property_name, &property_valid);
			ERR_FAIL_COND_V_MSG(!property_valid, _make_status(ERR_INVALID_DATA, vformat("Property '%s' does not exist on '%s'.", String(property_name), target_path), false), "UndoRedoBridge could not resolve the requested node property.");

			undo_redo->add_do_property(target_node, property_name, new_value);
			undo_redo->add_undo_property(target_node, property_name, old_value);
		}
	}
	undo_redo->commit_action();

	Dictionary status = _make_status(OK, "Scene plan applied through the MVP bridge.", true);
	status["operation_count"] = operations.size();
	return status;
}

Dictionary UndoRedoBridge::can_apply_gdscript_patch(const Ref<GDScriptRepairPatch> &p_patch) const {
	if (p_patch.is_null()) {
		return _make_status(ERR_INVALID_PARAMETER, "GDScriptRepairPatch is null.", false);
	}
	if (p_patch->get_script_path().is_empty()) {
		return _make_status(ERR_INVALID_PARAMETER, "Patch script_path must not be empty.", false);
	}
	if (p_patch->get_hunks().is_empty() && p_patch->get_replacement_text().is_empty()) {
		return _make_status(ERR_INVALID_PARAMETER, "Patch must contain hunks or replacement_text.", false);
	}
	if (!FileAccess::exists(p_patch->get_script_path())) {
		return _make_status(ERR_DOES_NOT_EXIST, vformat("Script '%s' does not exist.", p_patch->get_script_path()), false);
	}

	String original_text;
	Error read_error = _read_text_file(p_patch->get_script_path(), original_text);
	if (read_error != OK) {
		return _make_status(read_error, vformat("Could not read '%s'.", p_patch->get_script_path()), false);
	}

	if (!p_patch->get_hunks().is_empty()) {
		String candidate_text;
		String error_message;
		const Error patch_error = _apply_patch_hunks_to_text(original_text, p_patch->get_hunks(), candidate_text, error_message);
		if (patch_error != OK) {
			return _make_status(patch_error, error_message, false);
		}
	}

	return _make_status(OK, "Patch can be applied.", true);
}

Dictionary UndoRedoBridge::apply_gdscript_patch(const Ref<GDScriptRepairPatch> &p_patch) {
	const Dictionary applicability = can_apply_gdscript_patch(p_patch);
	const bool can_apply = applicability.has("can_apply") ? bool(applicability["can_apply"]) : false;
	if (!can_apply) {
		return applicability;
	}

	String original_text;
	Error read_error = _read_text_file(p_patch->get_script_path(), original_text);
	if (read_error != OK) {
		return _make_status(read_error, vformat("Could not read '%s'.", p_patch->get_script_path()), false);
	}

	String new_text = p_patch->get_replacement_text();
	if (!p_patch->get_hunks().is_empty()) {
		String error_message;
		const Error patch_error = _apply_patch_hunks_to_text(original_text, p_patch->get_hunks(), new_text, error_message);
		if (patch_error != OK) {
			return _make_status(patch_error, error_message, false);
		}
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	ERR_FAIL_NULL_V_MSG(undo_redo, _make_status(ERR_UNAVAILABLE, "EditorUndoRedoManager is unavailable.", false), "UndoRedoBridge requires EditorUndoRedoManager.");

	const String action_name = vformat("Apply AI GDScript patch: %s", p_patch->get_script_path().get_file());
	undo_redo->create_action(action_name, UndoRedo::MERGE_DISABLE, this);
	undo_redo->add_do_method(this, SNAME("_write_text_file"), p_patch->get_script_path(), new_text);
	undo_redo->add_undo_method(this, SNAME("_write_text_file"), p_patch->get_script_path(), original_text);
	undo_redo->commit_action();

	Dictionary status = _make_status(OK, "Patch applied through EditorUndoRedoManager.", true);
	status["script_path"] = p_patch->get_script_path();
	status["action_name"] = action_name;
	status["used_hunks"] = !p_patch->get_hunks().is_empty();
	return status;
}

Dictionary UndoRedoBridge::get_bridge_status() const {
	Dictionary status;
	status["has_undo_redo_manager"] = EditorUndoRedoManager::get_singleton() != nullptr;
	status["supports_scene_plan_apply"] = true;
	status["scene_plan_supported_operations"] = _make_scene_plan_supported_operations();
	status["supports_gdscript_patch_apply"] = true;
	status["supports_hunk_patch_apply"] = true;
	status["supports_full_file_replace_fallback"] = true;
	return status;
}

Error UndoRedoBridge::_read_text_file(const String &p_path, String &r_contents) const {
	Error err = OK;
	r_contents = FileAccess::get_file_as_string(p_path, &err);
	return err;
}

Node *UndoRedoBridge::_get_edited_scene_root() const {
	if (EditorInterface::get_singleton() == nullptr) {
		return nullptr;
	}
	return EditorInterface::get_singleton()->get_edited_scene_root();
}

Node *UndoRedoBridge::_resolve_scene_node(const String &p_path) const {
	Node *edited_scene_root = _get_edited_scene_root();
	if (edited_scene_root == nullptr) {
		return nullptr;
	}
	if (p_path.is_empty() || p_path == "." || p_path == String(edited_scene_root->get_path()) || p_path == String(edited_scene_root->get_name())) {
		return edited_scene_root;
	}

	Node *resolved = edited_scene_root->get_node_or_null(NodePath(p_path));
	if (resolved != nullptr) {
		return resolved;
	}

	const String absolute_root_path = String(edited_scene_root->get_path());
	if (p_path.begins_with(absolute_root_path + "/")) {
		const String relative_path = p_path.trim_prefix(absolute_root_path + "/");
		return edited_scene_root->get_node_or_null(NodePath(relative_path));
	}

	const String root_name = String(edited_scene_root->get_name());
	if (p_path.begins_with(root_name + "/")) {
		const String relative_path = p_path.trim_prefix(root_name + "/");
		return edited_scene_root->get_node_or_null(NodePath(relative_path));
	}

	return nullptr;
}

String UndoRedoBridge::_join_scene_path(const String &p_parent_path, const String &p_name) const {
	if (p_parent_path.is_empty() || p_parent_path == ".") {
		return p_name;
	}
	return p_parent_path.path_join(p_name);
}

void UndoRedoBridge::_attach_scene_node(const String &p_parent_path, Node *p_node) {
	Node *parent = _resolve_scene_node(p_parent_path);
	ERR_FAIL_NULL(parent);
	ERR_FAIL_NULL(p_node);
	parent->add_child(p_node, true);
}

void UndoRedoBridge::_detach_scene_node(const String &p_parent_path, Node *p_node) {
	Node *parent = _resolve_scene_node(p_parent_path);
	ERR_FAIL_NULL(parent);
	ERR_FAIL_NULL(p_node);
	parent->remove_child(p_node);
}

void UndoRedoBridge::_set_scene_node_owner(Node *p_node) {
	Node *edited_scene_root = _get_edited_scene_root();
	ERR_FAIL_NULL(edited_scene_root);
	ERR_FAIL_NULL(p_node);
	p_node->set_owner(edited_scene_root);
}

Array UndoRedoBridge::_split_text_lines(const String &p_text, bool *r_has_trailing_newline) const {
	const bool has_trailing_newline = p_text.ends_with("\n");
	if (r_has_trailing_newline != nullptr) {
		*r_has_trailing_newline = has_trailing_newline;
	}

	Array lines;
	if (p_text.is_empty()) {
		return lines;
	}

	Vector<String> split_lines = p_text.split("\n", true);
	if (!split_lines.is_empty() && has_trailing_newline && split_lines[split_lines.size() - 1].is_empty()) {
		split_lines.resize(split_lines.size() - 1);
	}
	for (int32_t i = 0; i < split_lines.size(); i++) {
		lines.push_back(split_lines[i]);
	}
	return lines;
}

String UndoRedoBridge::_join_text_lines(const Array &p_lines, bool p_has_trailing_newline) const {
	String text;
	for (int32_t i = 0; i < p_lines.size(); i++) {
		if (i > 0) {
			text += "\n";
		}
		text += String(p_lines[i]);
	}
	if (p_has_trailing_newline && !text.is_empty()) {
		text += "\n";
	}
	return text;
}

Error UndoRedoBridge::_apply_patch_hunks_to_text(const String &p_original_text, const Array &p_hunks, String &r_output_text, String &r_error_message) const {
	bool has_trailing_newline = false;
	Array lines = _split_text_lines(p_original_text, &has_trailing_newline);

	for (int32_t i = 0; i < p_hunks.size(); i++) {
		if (p_hunks[i].get_type() != Variant::DICTIONARY) {
			r_error_message = vformat("Patch hunk %d must be a dictionary.", i);
			return ERR_INVALID_DATA;
		}

		const Dictionary hunk = p_hunks[i];
		const String operation = hunk.has("op") ? String(hunk["op"]) : String();
		const int32_t line_start = hunk.has("line_start") ? int32_t(hunk["line_start"]) : 0;
		const int32_t line_end = hunk.has("line_end") ? int32_t(hunk["line_end"]) : 0;
		const String replacement_text = hunk.has("replacement_text") ? String(hunk["replacement_text"]) : String();
		const Array replacement_lines = _split_text_lines(replacement_text);

		if (operation == "replace_range") {
			if (line_start < 1 || line_end < line_start || line_end > lines.size()) {
				r_error_message = vformat("replace_range hunk %d targets invalid line range %d-%d.", i, line_start, line_end);
				return ERR_INVALID_DATA;
			}
			for (int32_t remove_index = line_end; remove_index >= line_start; remove_index--) {
				lines.remove_at(remove_index - 1);
			}
			for (int32_t insert_index = 0; insert_index < replacement_lines.size(); insert_index++) {
				lines.insert((line_start - 1) + insert_index, replacement_lines[insert_index]);
			}
		} else if (operation == "insert_before") {
			if (line_start < 1 || line_start > lines.size() + 1) {
				r_error_message = vformat("insert_before hunk %d targets invalid line %d.", i, line_start);
				return ERR_INVALID_DATA;
			}
			for (int32_t insert_index = 0; insert_index < replacement_lines.size(); insert_index++) {
				lines.insert((line_start - 1) + insert_index, replacement_lines[insert_index]);
			}
		} else if (operation == "insert_after") {
			if (line_end < 1 || line_end > lines.size()) {
				r_error_message = vformat("insert_after hunk %d targets invalid line %d.", i, line_end);
				return ERR_INVALID_DATA;
			}
			for (int32_t insert_index = 0; insert_index < replacement_lines.size(); insert_index++) {
				lines.insert(line_end + insert_index, replacement_lines[insert_index]);
			}
		} else {
			r_error_message = vformat("Unsupported patch operation '%s'.", operation);
			return ERR_INVALID_DATA;
		}
	}

	r_output_text = _join_text_lines(lines, has_trailing_newline);
	return OK;
}

Dictionary UndoRedoBridge::_make_status(Error p_error, const String &p_message, bool p_can_apply) const {
	Dictionary status;
	status["error"] = p_error;
	status["ok"] = p_error == OK;
	status["can_apply"] = p_can_apply;
	status["message"] = p_message;
	return status;
}

UndoRedoBridge::UndoRedoBridge() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
}

UndoRedoBridge::~UndoRedoBridge() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
