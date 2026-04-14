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
#include "editor/editor_data.h"
#include "editor/editor_interface.h"
#include "scene/main/node.h"

EditorContextCollector *EditorContextCollector::singleton = nullptr;

void EditorContextCollector::_bind_methods() {
	ClassDB::bind_method(D_METHOD("has_editor_interface"), &EditorContextCollector::has_editor_interface);
	ClassDB::bind_method(D_METHOD("set_scene_synthesis_budget", "budget"), &EditorContextCollector::set_scene_synthesis_budget);
	ClassDB::bind_method(D_METHOD("get_scene_synthesis_budget"), &EditorContextCollector::get_scene_synthesis_budget);
	ClassDB::bind_method(D_METHOD("set_script_repair_budget", "budget"), &EditorContextCollector::set_script_repair_budget);
	ClassDB::bind_method(D_METHOD("get_script_repair_budget"), &EditorContextCollector::get_script_repair_budget);
	ClassDB::bind_method(D_METHOD("collect_scene_request_context", "overrides"), &EditorContextCollector::collect_scene_request_context, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("collect_script_repair_context", "script_path", "diagnostics", "code_snippet", "overrides"), &EditorContextCollector::collect_script_repair_context, DEFVAL(String()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("get_collector_status"), &EditorContextCollector::get_collector_status);

	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "scene_synthesis_budget"), "set_scene_synthesis_budget", "get_scene_synthesis_budget");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "script_repair_budget"), "set_script_repair_budget", "get_script_repair_budget");

	BIND_ENUM_CONSTANT(BUDGET_PROFILE_SCENE_SYNTHESIS);
	BIND_ENUM_CONSTANT(BUDGET_PROFILE_SCRIPT_REPAIR);
}

EditorContextCollector *EditorContextCollector::get_singleton() {
	return singleton;
}

bool EditorContextCollector::has_editor_interface() const {
	return EditorInterface::get_singleton() != nullptr;
}

void EditorContextCollector::set_scene_synthesis_budget(const Dictionary &p_budget) {
	scene_synthesis_budget = _sanitize_budget(p_budget, scene_synthesis_budget);
}

Dictionary EditorContextCollector::get_scene_synthesis_budget() const {
	return _serialize_budget(scene_synthesis_budget);
}

void EditorContextCollector::set_script_repair_budget(const Dictionary &p_budget) {
	script_repair_budget = _sanitize_budget(p_budget, script_repair_budget);
}

Dictionary EditorContextCollector::get_script_repair_budget() const {
	return _serialize_budget(script_repair_budget);
}

Dictionary EditorContextCollector::collect_scene_request_context(const Dictionary &p_overrides) const {
	Dictionary context = _build_project_context();
	context["context_format_version"] = 1;
	context["collector"] = "EditorContextCollector";
	context["request_kind"] = "scene_synthesis";
	context["budget_profile"] = BUDGET_PROFILE_SCENE_SYNTHESIS;
	context["scene"] = _build_scene_context(scene_synthesis_budget);
	context["budget"] = _serialize_budget(scene_synthesis_budget);
	context["budget_policy"] = _build_budget_info();
	return _merge_context(context, p_overrides);
}

Dictionary EditorContextCollector::collect_script_repair_context(const String &p_script_path, const String &p_diagnostics, const String &p_code_snippet, const Dictionary &p_overrides) const {
	Dictionary context = _build_project_context();
	context["context_format_version"] = 1;
	context["collector"] = "EditorContextCollector";
	context["request_kind"] = "script_repair";
	context["budget_profile"] = BUDGET_PROFILE_SCRIPT_REPAIR;
	context["scene"] = _build_scene_context(script_repair_budget);
	context["script"] = _build_script_context(p_script_path, p_diagnostics, p_code_snippet, script_repair_budget);
	context["budget"] = _serialize_budget(script_repair_budget);
	context["budget_policy"] = _build_budget_info();
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

EditorContextCollector::ContextBudget EditorContextCollector::_sanitize_budget(const Dictionary &p_budget, const ContextBudget &p_fallback) const {
	ContextBudget budget = p_fallback;

	if (p_budget.has("scene_node_budget")) {
		budget.scene_node_budget = CLAMP(int32_t(p_budget["scene_node_budget"]), MIN_SCENE_NODE_BUDGET, MAX_SCENE_NODE_BUDGET);
	}
	if (p_budget.has("scene_depth_budget")) {
		budget.scene_depth_budget = CLAMP(int32_t(p_budget["scene_depth_budget"]), MIN_SCENE_DEPTH_BUDGET, MAX_SCENE_DEPTH_BUDGET);
	}
	if (p_budget.has("selection_budget")) {
		budget.selection_budget = CLAMP(int32_t(p_budget["selection_budget"]), MIN_SELECTION_BUDGET, MAX_SELECTION_BUDGET);
	}
	if (p_budget.has("text_preview_budget")) {
		budget.text_preview_budget = CLAMP(int32_t(p_budget["text_preview_budget"]), MIN_TEXT_PREVIEW_BUDGET, MAX_TEXT_PREVIEW_BUDGET);
	}

	return budget;
}

Dictionary EditorContextCollector::_serialize_budget(const ContextBudget &p_budget) const {
	Dictionary budget;
	budget["scene_node_budget"] = p_budget.scene_node_budget;
	budget["scene_depth_budget"] = p_budget.scene_depth_budget;
	budget["selection_budget"] = p_budget.selection_budget;
	budget["text_preview_budget"] = p_budget.text_preview_budget;
	return budget;
}

Dictionary EditorContextCollector::_build_scene_context(const ContextBudget &p_budget) const {
	Dictionary scene_context;
	if (!has_editor_interface()) {
		return scene_context;
	}

	EditorInterface *editor_interface = EditorInterface::get_singleton();
	Node *edited_scene_root = editor_interface->get_edited_scene_root();
	const PackedStringArray open_scenes = editor_interface->get_open_scenes();
	const PackedStringArray unsaved_scenes = editor_interface->get_unsaved_scenes();
	const Array selection = _snapshot_selection(p_budget);

	scene_context["open_scenes"] = open_scenes;
	scene_context["open_scene_count"] = open_scenes.size();
	scene_context["unsaved_scenes"] = unsaved_scenes;
	scene_context["unsaved_scene_count"] = unsaved_scenes.size();
	scene_context["edited_scene_path"] = edited_scene_root != nullptr ? edited_scene_root->get_scene_file_path() : String();
	scene_context["edited_scene_name"] = edited_scene_root != nullptr ? String(edited_scene_root->get_name()) : String();
	scene_context["edited_scene_root_class"] = edited_scene_root != nullptr ? String(edited_scene_root->get_class()) : String();
	scene_context["selection"] = selection;
	scene_context["selection_count"] = selection.size();

	if (edited_scene_root != nullptr && p_budget.scene_node_budget > 0) {
		int32_t remaining_budget = p_budget.scene_node_budget;
		scene_context["tree_snapshot"] = _snapshot_node(edited_scene_root, 0, remaining_budget, p_budget);
		scene_context["tree_nodes_captured"] = p_budget.scene_node_budget - remaining_budget;
		scene_context["tree_node_budget_exhausted"] = remaining_budget == 0;
	} else {
		scene_context["tree_nodes_captured"] = 0;
		scene_context["tree_node_budget_exhausted"] = false;
	}

	return scene_context;
}

Dictionary EditorContextCollector::_build_script_context(const String &p_script_path, const String &p_diagnostics, const String &p_code_snippet, const ContextBudget &p_budget) const {
	Dictionary script_context;
	const String snippet = p_code_snippet.is_empty() ? _load_script_snippet(p_script_path) : p_code_snippet;
	script_context["script_path"] = p_script_path;
	script_context["diagnostics"] = _truncate_text(p_diagnostics, p_budget.text_preview_budget);
	script_context["code_snippet"] = _truncate_text(snippet, p_budget.text_preview_budget);
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
	Dictionary limits;
	limits["scene_node_budget_min"] = MIN_SCENE_NODE_BUDGET;
	limits["scene_node_budget_max"] = MAX_SCENE_NODE_BUDGET;
	limits["scene_depth_budget_min"] = MIN_SCENE_DEPTH_BUDGET;
	limits["scene_depth_budget_max"] = MAX_SCENE_DEPTH_BUDGET;
	limits["selection_budget_min"] = MIN_SELECTION_BUDGET;
	limits["selection_budget_max"] = MAX_SELECTION_BUDGET;
	limits["text_preview_budget_min"] = MIN_TEXT_PREVIEW_BUDGET;
	limits["text_preview_budget_max"] = MAX_TEXT_PREVIEW_BUDGET;
	budget["scene_synthesis"] = _serialize_budget(scene_synthesis_budget);
	budget["script_repair"] = _serialize_budget(script_repair_budget);
	budget["limits"] = limits;
	return budget;
}

Dictionary EditorContextCollector::_snapshot_node(Node *p_node, int32_t p_depth, int32_t &r_remaining_budget, const ContextBudget &p_budget) const {
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

	if (p_depth >= p_budget.scene_depth_budget || r_remaining_budget <= 0) {
		snapshot["children_truncated"] = p_node->get_child_count() > 0;
		return snapshot;
	}

	Array children;
	const int32_t child_count = p_node->get_child_count();
	for (int32_t i = 0; i < child_count && r_remaining_budget > 0; i++) {
		Node *child = p_node->get_child(i);
		children.push_back(_snapshot_node(child, p_depth + 1, r_remaining_budget, p_budget));
	}
	snapshot["children"] = children;
	snapshot["children_truncated"] = children.size() < child_count;
	return snapshot;
}

Array EditorContextCollector::_snapshot_selection(const ContextBudget &p_budget) const {
	Array selection_snapshot;
	if (!has_editor_interface()) {
		return selection_snapshot;
	}

	EditorSelection *selection = EditorInterface::get_singleton()->get_selection();
	if (selection == nullptr) {
		return selection_snapshot;
	}

	const TypedArray<Node> selected_nodes = selection->get_selected_nodes();
	const int32_t selection_count = MIN(selected_nodes.size(), p_budget.selection_budget);
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
	scene_synthesis_budget.scene_node_budget = 64;
	scene_synthesis_budget.scene_depth_budget = 4;
	scene_synthesis_budget.selection_budget = 16;
	scene_synthesis_budget.text_preview_budget = 2000;
	script_repair_budget.scene_node_budget = 24;
	script_repair_budget.scene_depth_budget = 2;
	script_repair_budget.selection_budget = 8;
	script_repair_budget.text_preview_budget = 4000;
	singleton = this;
}

EditorContextCollector::~EditorContextCollector() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
