/**************************************************************************/
/*  editor_ai_preview_diff.cpp                                            */
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

#include "modules/woodot_ai/editor/editor_ai_preview_diff.h"

#include "core/object/class_db.h"
#include "modules/woodot_ai/resources/gdscript_repair_patch.h"
#include "modules/woodot_ai/resources/scene_synthesis_plan.h"

EditorAIPreviewDiff *EditorAIPreviewDiff::singleton = nullptr;

void EditorAIPreviewDiff::_bind_methods() {
	ClassDB::bind_method(D_METHOD("build_scene_plan_preview", "plan"), &EditorAIPreviewDiff::build_scene_plan_preview);
	ClassDB::bind_method(D_METHOD("build_gdscript_patch_preview", "patch"), &EditorAIPreviewDiff::build_gdscript_patch_preview);
	ClassDB::bind_method(D_METHOD("set_current_preview", "preview"), &EditorAIPreviewDiff::set_current_preview);
	ClassDB::bind_method(D_METHOD("get_current_preview"), &EditorAIPreviewDiff::get_current_preview);
	ClassDB::bind_method(D_METHOD("clear_preview"), &EditorAIPreviewDiff::clear_preview);
}

EditorAIPreviewDiff *EditorAIPreviewDiff::get_singleton() {
	return singleton;
}

Dictionary EditorAIPreviewDiff::build_scene_plan_preview(const Ref<SceneSynthesisPlan> &p_plan) const {
	Dictionary preview;
	Array warnings;
	Array items;

	preview["kind"] = "scene_plan";
	preview["summary"] = "No scene synthesis plan available.";
	preview["can_apply"] = false;
	preview["warnings"] = warnings;
	preview["items"] = items;
	preview["source_metadata"] = Dictionary();

	if (p_plan.is_null()) {
		warnings.push_back("SceneSynthesisPlan is null.");
		preview["warnings"] = warnings;
		return preview;
	}

	const Array node_operations = p_plan->get_node_operations();
	const Array resource_operations = p_plan->get_resource_operations();
	for (int32_t i = 0; i < node_operations.size(); i++) {
		if (node_operations[i].get_type() == Variant::DICTIONARY) {
			items.push_back(_build_scene_operation_item(node_operations[i], i));
		}
	}
	for (int32_t i = 0; i < resource_operations.size(); i++) {
		if (resource_operations[i].get_type() == Variant::DICTIONARY) {
			items.push_back(_build_resource_operation_item(resource_operations[i], i));
		}
	}

	warnings = _normalize_warnings(p_plan->get_warnings());
	warnings.push_back("Scene plan apply is not implemented yet; preview is read-only.");

	preview["summary"] = vformat("Scene plan with %d node operations and %d resource operations.", node_operations.size(), resource_operations.size());
	preview["can_apply"] = false;
	preview["warnings"] = warnings;
	preview["items"] = items;
	preview["operation_count"] = node_operations.size() + resource_operations.size();
	preview["source_metadata"] = p_plan->get_metadata();
	preview["prompt"] = p_plan->get_prompt();
	return preview;
}

Dictionary EditorAIPreviewDiff::build_gdscript_patch_preview(const Ref<GDScriptRepairPatch> &p_patch) const {
	Dictionary preview;
	Array warnings;
	Array items;

	preview["kind"] = "gdscript_patch";
	preview["summary"] = "No GDScript repair patch available.";
	preview["can_apply"] = false;
	preview["warnings"] = warnings;
	preview["items"] = items;
	preview["source_metadata"] = Dictionary();

	if (p_patch.is_null()) {
		warnings.push_back("GDScriptRepairPatch is null.");
		preview["warnings"] = warnings;
		return preview;
	}

	const Array hunks = p_patch->get_hunks();
	for (int32_t i = 0; i < hunks.size(); i++) {
		if (hunks[i].get_type() == Variant::DICTIONARY) {
			items.push_back(_build_patch_hunk_item(hunks[i], i));
		}
	}

	if (hunks.is_empty() && !p_patch->get_replacement_text().is_empty()) {
		Dictionary item;
		item["kind"] = "file_replace";
		item["label"] = "Replace file contents";
		item["detail"] = vformat("Patch replaces the full script at %s.", p_patch->get_script_path());
		item["risk"] = "high";
		item["path"] = p_patch->get_script_path();
		items.push_back(item);
	}

	warnings = _normalize_warnings(p_patch->get_warnings());
	if (hunks.is_empty() && p_patch->get_replacement_text().is_empty()) {
		warnings.push_back("Patch does not contain any hunks or replacement text.");
	}
	if (hunks.is_empty() && !p_patch->get_replacement_text().is_empty()) {
		warnings.push_back("Patch will fall back to full file replacement.");
	}

	preview["summary"] = vformat("Patch for %s with %d hunks.", p_patch->get_script_path(), hunks.size());
	preview["can_apply"] = !p_patch->get_script_path().is_empty() && (!hunks.is_empty() || !p_patch->get_replacement_text().is_empty());
	preview["warnings"] = warnings;
	preview["items"] = items;
	preview["operation_count"] = items.size();
	preview["source_metadata"] = p_patch->get_metadata();
	preview["script_path"] = p_patch->get_script_path();
	preview["diagnostic_message"] = p_patch->get_diagnostic_message();
	preview["line_start"] = p_patch->get_line_start();
	preview["line_end"] = p_patch->get_line_end();
	return preview;
}

void EditorAIPreviewDiff::set_current_preview(const Dictionary &p_preview) {
	current_preview = p_preview;
}

Dictionary EditorAIPreviewDiff::get_current_preview() const {
	return current_preview;
}

void EditorAIPreviewDiff::clear_preview() {
	current_preview.clear();
}

Array EditorAIPreviewDiff::_normalize_warnings(const Array &p_warnings) const {
	Array warnings;
	for (int32_t i = 0; i < p_warnings.size(); i++) {
		warnings.push_back(String(p_warnings[i]));
	}
	return warnings;
}

Dictionary EditorAIPreviewDiff::_build_scene_operation_item(const Dictionary &p_operation, int32_t p_index) const {
	Dictionary item;
	const String operation = p_operation.has("op") ? String(p_operation["op"]) : String("unknown");
	const String target_path = p_operation.has("target_path") ? String(p_operation["target_path"]) : String();
	const String parent_path = p_operation.has("parent_path") ? String(p_operation["parent_path"]) : String();
	const String node_name = p_operation.has("name") ? String(p_operation["name"]) : String();
	const String node_type = p_operation.has("node_type") ? String(p_operation["node_type"]) : String();

	item["kind"] = "node_operation";
	item["index"] = p_index;
	item["label"] = vformat("%s %s", operation.capitalize(), node_name.is_empty() ? target_path : node_name);
	item["detail"] = vformat("node_type=%s parent=%s target=%s", node_type, parent_path, target_path);
	item["risk"] = operation == "remove_node" ? "high" : "medium";
	item["path"] = !target_path.is_empty() ? target_path : parent_path;
	return item;
}

Dictionary EditorAIPreviewDiff::_build_resource_operation_item(const Dictionary &p_operation, int32_t p_index) const {
	Dictionary item;
	const String operation = p_operation.has("op") ? String(p_operation["op"]) : String("unknown");
	const String path = p_operation.has("path") ? String(p_operation["path"]) : String();
	const String resource_type = p_operation.has("resource_type") ? String(p_operation["resource_type"]) : String();
	const String assign_to = p_operation.has("assign_to") ? String(p_operation["assign_to"]) : String();

	item["kind"] = "resource_operation";
	item["index"] = p_index;
	item["label"] = vformat("%s %s", operation.capitalize(), path);
	item["detail"] = vformat("resource_type=%s assign_to=%s", resource_type, assign_to);
	item["risk"] = "medium";
	item["path"] = path;
	return item;
}

Dictionary EditorAIPreviewDiff::_build_patch_hunk_item(const Dictionary &p_hunk, int32_t p_index) const {
	Dictionary item;
	const String operation = p_hunk.has("op") ? String(p_hunk["op"]) : String("unknown");
	const int32_t line_start = p_hunk.has("line_start") ? int32_t(p_hunk["line_start"]) : 0;
	const int32_t line_end = p_hunk.has("line_end") ? int32_t(p_hunk["line_end"]) : 0;
	const String context = p_hunk.has("context") ? String(p_hunk["context"]) : String();

	item["kind"] = "patch_hunk";
	item["index"] = p_index;
	item["label"] = vformat("%s lines %d-%d", operation.capitalize(), line_start, line_end);
	item["detail"] = context.is_empty() ? String("No hunk context provided.") : context;
	item["risk"] = operation == "replace_range" ? "medium" : "low";
	item["line_start"] = line_start;
	item["line_end"] = line_end;
	return item;
}

EditorAIPreviewDiff::EditorAIPreviewDiff() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
}

EditorAIPreviewDiff::~EditorAIPreviewDiff() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
