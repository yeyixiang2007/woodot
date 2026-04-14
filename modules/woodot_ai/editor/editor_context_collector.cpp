/**************************************************************************/
/*  editor_context_collector.cpp                                          */
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

#include "modules/woodot_ai/editor/editor_context_collector.h"

#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/object/class_db.h"
#include "editor/editor_interface.h"
#include "scene/main/node.h"

EditorContextCollector *EditorContextCollector::singleton = nullptr;

void EditorContextCollector::_bind_methods() {
	ClassDB::bind_method(D_METHOD("has_editor_interface"), &EditorContextCollector::has_editor_interface);
	ClassDB::bind_method(D_METHOD("collect_scene_request_context", "overrides"), &EditorContextCollector::collect_scene_request_context, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("collect_script_repair_context", "script_path", "diagnostics", "code_snippet", "overrides"), &EditorContextCollector::collect_script_repair_context, DEFVAL(String()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("get_collector_status"), &EditorContextCollector::get_collector_status);
}

EditorContextCollector *EditorContextCollector::get_singleton() {
	return singleton;
}

bool EditorContextCollector::has_editor_interface() const {
	return EditorInterface::get_singleton() != nullptr;
}

Dictionary EditorContextCollector::collect_scene_request_context(const Dictionary &p_overrides) const {
	Dictionary context = _build_project_context();
	context["context_format_version"] = 1;
	context["collector"] = "EditorContextCollector";
	context["request_kind"] = "scene_synthesis";
	context["scene"] = _build_scene_context();
	context["budget"] = _build_budget_info();
	return _merge_context(context, p_overrides);
}

Dictionary EditorContextCollector::collect_script_repair_context(const String &p_script_path, const String &p_diagnostics, const String &p_code_snippet, const Dictionary &p_overrides) const {
	Dictionary context = _build_project_context();
	context["context_format_version"] = 1;
	context["collector"] = "EditorContextCollector";
	context["request_kind"] = "script_repair";
	context["scene"] = _build_scene_context();
	context["script"] = _build_script_context(p_script_path, p_diagnostics, p_code_snippet);
	context["budget"] = _build_budget_info();
	return _merge_context(context, p_overrides);
}

Dictionary EditorContextCollector::get_collector_status() const {
	Dictionary status;
	status["has_editor_interface"] = has_editor_interface();
	status["context_format_version"] = 1;
	status["budget"] = _build_budget_info();
	if (has_editor_interface()) {
		EditorInterface *editor_interface = EditorInterface::get_singleton();
		status["open_scenes"] = editor_interface->get_open_scenes();
		status["unsaved_scenes"] = editor_interface->get_unsaved_scenes();
		status["edited_scene_root"] = editor_interface->get_edited_scene_root() != nullptr ? String(editor_interface->get_edited_scene_root()->get_name()) : String();
	}
	return status;
}

Dictionary EditorContextCollector::_build_scene_context() const {
	Dictionary scene_context;
	if (!has_editor_interface()) {
		return scene_context;
	}

	EditorInterface *editor_interface = EditorInterface::get_singleton();
	Node *edited_scene_root = editor_interface->get_edited_scene_root();
	const PackedStringArray open_scenes = editor_interface->get_open_scenes();
	const PackedStringArray unsaved_scenes = editor_interface->get_unsaved_scenes();
	const Array selection = _snapshot_selection();

	scene_context["open_scenes"] = open_scenes;
	scene_context["open_scene_count"] = open_scenes.size();
	scene_context["unsaved_scenes"] = unsaved_scenes;
	scene_context["unsaved_scene_count"] = unsaved_scenes.size();
	scene_context["edited_scene_path"] = edited_scene_root != nullptr ? edited_scene_root->get_scene_file_path() : String();
	scene_context["edited_scene_name"] = edited_scene_root != nullptr ? String(edited_scene_root->get_name()) : String();
	scene_context["edited_scene_root_class"] = edited_scene_root != nullptr ? String(edited_scene_root->get_class()) : String();
	scene_context["selection"] = selection;
	scene_context["selection_count"] = selection.size();

	if (edited_scene_root != nullptr) {
		int32_t remaining_budget = SCENE_NODE_BUDGET;
		scene_context["tree_snapshot"] = _snapshot_node(edited_scene_root, 0, remaining_budget);
		scene_context["tree_nodes_captured"] = SCENE_NODE_BUDGET - remaining_budget;
		scene_context["tree_node_budget_exhausted"] = remaining_budget == 0;
	}

	return scene_context;
}

Dictionary EditorContextCollector::_build_script_context(const String &p_script_path, const String &p_diagnostics, const String &p_code_snippet) const {
	Dictionary script_context;
	const String snippet = p_code_snippet.is_empty() ? _load_script_snippet(p_script_path) : p_code_snippet;
	script_context["script_path"] = p_script_path;
	script_context["diagnostics"] = _truncate_text(p_diagnostics, TEXT_PREVIEW_BUDGET);
	script_context["code_snippet"] = _truncate_text(snippet, TEXT_PREVIEW_BUDGET);
	script_context["has_code_snippet"] = !snippet.is_empty();
	script_context["code_snippet_source"] = p_code_snippet.is_empty() ? (snippet.is_empty() ? "unavailable" : "file_access") : "request";
	return script_context;
}

Dictionary EditorContextCollector::_build_project_context() const {
	Dictionary project_context;
	project_context["project_name"] = ProjectSettings::get_singleton()->get_setting_with_override("application/config/name");
	project_context["main_scene"] = ProjectSettings::get_singleton()->get_setting_with_override("application/run/main_scene");
	project_context["feature_tags"] = ProjectSettings::get_singleton()->get_setting_with_override("application/config/features");
	return project_context;
}

Dictionary EditorContextCollector::_build_budget_info() const {
	Dictionary budget;
	budget["scene_node_budget"] = SCENE_NODE_BUDGET;
	budget["scene_depth_budget"] = SCENE_DEPTH_BUDGET;
	budget["selection_budget"] = SELECTION_BUDGET;
	budget["text_preview_budget"] = TEXT_PREVIEW_BUDGET;
	return budget;
}

Dictionary EditorContextCollector::_snapshot_node(Node *p_node, int32_t p_depth, int32_t &r_remaining_budget) const {
	Dictionary snapshot;
	if (p_node == nullptr || r_remaining_budget <= 0) {
		return snapshot;
	}

	r_remaining_budget--;
	snapshot["name"] = p_node->get_name();
	snapshot["class"] = p_node->get_class();
	snapshot["path"] = String(p_node->get_path());
	snapshot["child_count"] = p_node->get_child_count();
	snapshot["owner"] = p_node->get_owner() != nullptr ? String(p_node->get_owner()->get_name()) : String();
	snapshot["scene_file_path"] = p_node->get_scene_file_path();
	snapshot["has_script"] = p_node->get_script().get_type() != Variant::NIL;
	snapshot["is_unique_name_in_owner"] = p_node->is_unique_name_in_owner();

	if (p_depth >= SCENE_DEPTH_BUDGET || r_remaining_budget <= 0) {
		snapshot["children_truncated"] = p_node->get_child_count() > 0;
		return snapshot;
	}

	Array children;
	const int32_t child_count = p_node->get_child_count();
	for (int32_t i = 0; i < child_count && r_remaining_budget > 0; i++) {
		Node *child = p_node->get_child(i);
		children.push_back(_snapshot_node(child, p_depth + 1, r_remaining_budget));
	}
	snapshot["children"] = children;
	snapshot["children_truncated"] = children.size() < child_count;
	return snapshot;
}

Array EditorContextCollector::_snapshot_selection() const {
	Array selection_snapshot;
	if (!has_editor_interface()) {
		return selection_snapshot;
	}

	EditorSelection *selection = EditorInterface::get_singleton()->get_selection();
	if (selection == nullptr) {
		return selection_snapshot;
	}

	const TypedArray<Node> selected_nodes = selection->get_selected_nodes();
	const int32_t selection_count = MIN(selected_nodes.size(), SELECTION_BUDGET);
	for (int32_t i = 0; i < selection_count; i++) {
		Node *node = Object::cast_to<Node>(selected_nodes[i]);
		if (node == nullptr) {
			continue;
		}

		Dictionary selected;
		selected["name"] = node->get_name();
		selected["class"] = node->get_class();
		selected["path"] = String(node->get_path());
		selected["child_count"] = node->get_child_count();
		selected["owner"] = node->get_owner() != nullptr ? String(node->get_owner()->get_name()) : String();
		selected["scene_file_path"] = node->get_scene_file_path();
		selected["has_script"] = node->get_script().get_type() != Variant::NIL;
		selection_snapshot.push_back(selected);
	}

	return selection_snapshot;
}

String EditorContextCollector::_load_script_snippet(const String &p_script_path) const {
	if (p_script_path.is_empty() || !FileAccess::exists(p_script_path)) {
		return String();
	}

	return FileAccess::get_file_as_string(p_script_path);
}

String EditorContextCollector::_truncate_text(const String &p_text, int32_t p_limit) const {
	if (p_text.length() <= p_limit) {
		return p_text;
	}

	return p_text.substr(0, p_limit) + "\n...[truncated]";
}

Dictionary EditorContextCollector::_merge_context(const Dictionary &p_base, const Dictionary &p_overrides) const {
	Dictionary merged = p_base;
	const Array keys = p_overrides.keys();
	for (int i = 0; i < keys.size(); i++) {
		merged[keys[i]] = p_overrides[keys[i]];
	}
	return merged;
}

EditorContextCollector::EditorContextCollector() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
}

EditorContextCollector::~EditorContextCollector() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
