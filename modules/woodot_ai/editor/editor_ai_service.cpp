/**************************************************************************/
/*  editor_ai_service.cpp                                                 */
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

#include "modules/woodot_ai/editor/editor_ai_service.h"

#include "core/object/class_db.h"
#include "modules/woodot_ai/editor/editor_ai_preview_diff.h"
#include "modules/woodot_ai/editor/editor_context_collector.h"
#include "modules/woodot_ai/editor/gdscript_repair_engine.h"
#include "modules/woodot_ai/editor/node_graph_intent_parser.h"
#include "modules/woodot_ai/editor/undo_redo_bridge.h"
#include "modules/woodot_ai/resources/ai_model_resource.h"
#include "modules/woodot_ai/resources/gdscript_repair_patch.h"
#include "modules/woodot_ai/resources/scene_synthesis_plan.h"
#include "modules/woodot_ai/runtime/ai_requests.h"
#include "modules/woodot_ai/runtime/ai_runtime_server.h"

EditorAIService *EditorAIService::singleton = nullptr;

void EditorAIService::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_default_model", "model"), &EditorAIService::set_default_model);
	ClassDB::bind_method(D_METHOD("get_default_model"), &EditorAIService::get_default_model);
	ClassDB::bind_method(D_METHOD("has_runtime_server"), &EditorAIService::has_runtime_server);
	ClassDB::bind_method(D_METHOD("is_ready"), &EditorAIService::is_ready);
	ClassDB::bind_method(D_METHOD("has_context_collector"), &EditorAIService::has_context_collector);
	ClassDB::bind_method(D_METHOD("has_gdscript_repair_engine"), &EditorAIService::has_gdscript_repair_engine);
	ClassDB::bind_method(D_METHOD("has_node_graph_intent_parser"), &EditorAIService::has_node_graph_intent_parser);
	ClassDB::bind_method(D_METHOD("has_preview_diff"), &EditorAIService::has_preview_diff);
	ClassDB::bind_method(D_METHOD("has_undo_redo_bridge"), &EditorAIService::has_undo_redo_bridge);
	ClassDB::bind_method(D_METHOD("has_loaded_default_model"), &EditorAIService::has_loaded_default_model);
	ClassDB::bind_method(D_METHOD("ensure_default_model_loaded"), &EditorAIService::ensure_default_model_loaded);
	ClassDB::bind_method(D_METHOD("unload_default_model"), &EditorAIService::unload_default_model);
	ClassDB::bind_method(D_METHOD("collect_scene_request_context", "overrides"), &EditorAIService::collect_scene_request_context, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("collect_script_repair_context", "script_path", "diagnostics", "code_snippet", "overrides"), &EditorAIService::collect_script_repair_context, DEFVAL(String()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("validate_gdscript_patch_ir", "source_ir"), &EditorAIService::validate_gdscript_patch_ir);
	ClassDB::bind_method(D_METHOD("parse_gdscript_patch_ir", "source_ir", "script_path", "diagnostic_message", "metadata"), &EditorAIService::parse_gdscript_patch_ir, DEFVAL(String()), DEFVAL(String()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("validate_scene_plan_ir", "source_ir"), &EditorAIService::validate_scene_plan_ir);
	ClassDB::bind_method(D_METHOD("parse_scene_plan_ir", "source_ir", "prompt", "metadata"), &EditorAIService::parse_scene_plan_ir, DEFVAL(String()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("build_scene_plan_preview", "plan"), &EditorAIService::build_scene_plan_preview);
	ClassDB::bind_method(D_METHOD("build_gdscript_patch_preview", "patch"), &EditorAIService::build_gdscript_patch_preview);
	ClassDB::bind_method(D_METHOD("can_apply_scene_plan", "plan"), &EditorAIService::can_apply_scene_plan);
	ClassDB::bind_method(D_METHOD("apply_scene_plan", "plan"), &EditorAIService::apply_scene_plan);
	ClassDB::bind_method(D_METHOD("can_apply_gdscript_patch", "patch"), &EditorAIService::can_apply_gdscript_patch);
	ClassDB::bind_method(D_METHOD("apply_gdscript_patch", "patch"), &EditorAIService::apply_gdscript_patch);
	ClassDB::bind_method(D_METHOD("resolve_scene_synthesis_task", "task_handle", "prompt", "metadata"), &EditorAIService::resolve_scene_synthesis_task, DEFVAL(String()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("resolve_script_repair_task", "task_handle", "script_path", "diagnostic_message", "metadata"), &EditorAIService::resolve_script_repair_task, DEFVAL(String()), DEFVAL(String()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("request_scene_synthesis", "prompt", "context"), &EditorAIService::request_scene_synthesis, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("request_script_repair", "script_path", "diagnostics", "code_snippet", "context"), &EditorAIService::request_script_repair, DEFVAL(String()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("cancel_task", "task_handle"), &EditorAIService::cancel_task);
	ClassDB::bind_method(D_METHOD("poll"), &EditorAIService::poll);
	ClassDB::bind_method(D_METHOD("get_last_scene_synthesis_task"), &EditorAIService::get_last_scene_synthesis_task);
	ClassDB::bind_method(D_METHOD("get_last_script_repair_task"), &EditorAIService::get_last_script_repair_task);
	ClassDB::bind_method(D_METHOD("get_service_status"), &EditorAIService::get_service_status);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "default_model", PROPERTY_HINT_RESOURCE_TYPE, "AIModelResource"), "set_default_model", "get_default_model");

	ADD_SIGNAL(MethodInfo("request_submitted",
			PropertyInfo(Variant::INT, "kind"),
			PropertyInfo(Variant::OBJECT, "task_handle", PROPERTY_HINT_RESOURCE_TYPE, "AITaskHandle")));
	ADD_SIGNAL(MethodInfo("request_failed",
			PropertyInfo(Variant::INT, "kind"),
			PropertyInfo(Variant::STRING, "message")));
	ADD_SIGNAL(MethodInfo("default_model_changed"));

	BIND_ENUM_CONSTANT(REQUEST_KIND_SCENE_SYNTHESIS);
	BIND_ENUM_CONSTANT(REQUEST_KIND_SCRIPT_REPAIR);
}

EditorAIService *EditorAIService::get_singleton() {
	return singleton;
}

void EditorAIService::set_default_model(const Ref<AIModelResource> &p_model) {
	if (default_model == p_model) {
		return;
	}

	if (default_model_rid.is_valid()) {
		unload_default_model();
	}

	default_model = p_model;
	emit_signal(SNAME("default_model_changed"));
}

Ref<AIModelResource> EditorAIService::get_default_model() const {
	return default_model;
}

bool EditorAIService::has_runtime_server() const {
	return _get_runtime_server() != nullptr;
}

bool EditorAIService::is_ready() const {
	return has_runtime_server() && default_model.is_valid();
}

bool EditorAIService::has_context_collector() const {
	return EditorContextCollector::get_singleton() != nullptr;
}

bool EditorAIService::has_gdscript_repair_engine() const {
	return GDScriptRepairEngine::get_singleton() != nullptr;
}

bool EditorAIService::has_node_graph_intent_parser() const {
	return NodeGraphIntentParser::get_singleton() != nullptr;
}

bool EditorAIService::has_preview_diff() const {
	return EditorAIPreviewDiff::get_singleton() != nullptr;
}

bool EditorAIService::has_undo_redo_bridge() const {
	return UndoRedoBridge::get_singleton() != nullptr;
}

bool EditorAIService::has_loaded_default_model() const {
	AIRuntimeServer *runtime_server = _get_runtime_server();
	if (runtime_server == nullptr || !default_model_rid.is_valid()) {
		return false;
	}

	return runtime_server->has_model(default_model_rid);
}

Error EditorAIService::ensure_default_model_loaded() {
	AIRuntimeServer *runtime_server = _get_runtime_server();
	ERR_FAIL_NULL_V_MSG(runtime_server, ERR_UNAVAILABLE, "EditorAIService requires AIRuntimeServer.");
	ERR_FAIL_COND_V_MSG(default_model.is_null(), ERR_INVALID_PARAMETER, "EditorAIService requires a default AIModelResource.");

	if (default_model_rid.is_valid()) {
		if (!runtime_server->has_model(default_model_rid) || default_model->is_runtime_dirty()) {
			runtime_server->unload_model(default_model_rid);
			default_model_rid = RID();
		}
	}

	if (!default_model_rid.is_valid()) {
		default_model_rid = runtime_server->load_model(default_model);
		ERR_FAIL_COND_V_MSG(!default_model_rid.is_valid(), ERR_CANT_CREATE, "EditorAIService failed to load the default AI model.");
		default_model->mark_runtime_clean();
	}

	return OK;
}

void EditorAIService::unload_default_model() {
	AIRuntimeServer *runtime_server = _get_runtime_server();
	if (runtime_server != nullptr && default_model_rid.is_valid()) {
		runtime_server->unload_model(default_model_rid);
	}
	default_model_rid = RID();
}

Dictionary EditorAIService::collect_scene_request_context(const Dictionary &p_overrides) const {
	if (!has_context_collector()) {
		return p_overrides;
	}

	return EditorContextCollector::get_singleton()->collect_scene_request_context(p_overrides);
}

Dictionary EditorAIService::collect_script_repair_context(const String &p_script_path, const String &p_diagnostics, const String &p_code_snippet, const Dictionary &p_overrides) const {
	if (!has_context_collector()) {
		Dictionary fallback = p_overrides;
		fallback["script_path"] = p_script_path;
		fallback["diagnostics"] = p_diagnostics;
		fallback["code_snippet"] = p_code_snippet;
		return fallback;
	}

	return EditorContextCollector::get_singleton()->collect_script_repair_context(p_script_path, p_diagnostics, p_code_snippet, p_overrides);
}

Dictionary EditorAIService::validate_gdscript_patch_ir(const String &p_source_ir) const {
	if (!has_gdscript_repair_engine()) {
		Dictionary status;
		Array errors;
		status["valid"] = false;
		errors.push_back("GDScriptRepairEngine is unavailable.");
		status["errors"] = errors;
		status["warnings"] = Array();
		return status;
	}

	return GDScriptRepairEngine::get_singleton()->validate_patch_ir(p_source_ir);
}

Ref<GDScriptRepairPatch> EditorAIService::parse_gdscript_patch_ir(const String &p_source_ir, const String &p_script_path, const String &p_diagnostic_message, const Dictionary &p_metadata) const {
	if (!has_gdscript_repair_engine()) {
		return Ref<GDScriptRepairPatch>();
	}

	return GDScriptRepairEngine::get_singleton()->parse_patch_ir(p_source_ir, p_script_path, p_diagnostic_message, p_metadata);
}

Dictionary EditorAIService::validate_scene_plan_ir(const String &p_source_ir) const {
	if (!has_node_graph_intent_parser()) {
		Dictionary status;
		Array errors;
		status["valid"] = false;
		errors.push_back("NodeGraphIntentParser is unavailable.");
		status["errors"] = errors;
		status["warnings"] = Array();
		return status;
	}

	return NodeGraphIntentParser::get_singleton()->validate_scene_plan_ir(p_source_ir);
}

Ref<SceneSynthesisPlan> EditorAIService::parse_scene_plan_ir(const String &p_source_ir, const String &p_prompt, const Dictionary &p_metadata) const {
	if (!has_node_graph_intent_parser()) {
		return Ref<SceneSynthesisPlan>();
	}

	return NodeGraphIntentParser::get_singleton()->parse_scene_plan_ir(p_source_ir, p_prompt, p_metadata);
}

Dictionary EditorAIService::build_scene_plan_preview(const Ref<SceneSynthesisPlan> &p_plan) const {
	if (!has_preview_diff()) {
		Dictionary preview;
		Array warnings;
		warnings.push_back("EditorAIPreviewDiff is unavailable.");
		preview["kind"] = "scene_plan";
		preview["summary"] = "Preview service unavailable.";
		preview["can_apply"] = false;
		preview["warnings"] = warnings;
		preview["items"] = Array();
		return preview;
	}

	const Dictionary preview = EditorAIPreviewDiff::get_singleton()->build_scene_plan_preview(p_plan);
	EditorAIPreviewDiff::get_singleton()->set_current_preview(preview);
	return preview;
}

Dictionary EditorAIService::build_gdscript_patch_preview(const Ref<GDScriptRepairPatch> &p_patch) const {
	if (!has_preview_diff()) {
		Dictionary preview;
		Array warnings;
		warnings.push_back("EditorAIPreviewDiff is unavailable.");
		preview["kind"] = "gdscript_patch";
		preview["summary"] = "Preview service unavailable.";
		preview["can_apply"] = false;
		preview["warnings"] = warnings;
		preview["items"] = Array();
		return preview;
	}

	const Dictionary preview = EditorAIPreviewDiff::get_singleton()->build_gdscript_patch_preview(p_patch);
	EditorAIPreviewDiff::get_singleton()->set_current_preview(preview);
	return preview;
}

Dictionary EditorAIService::can_apply_scene_plan(const Ref<SceneSynthesisPlan> &p_plan) const {
	if (!has_undo_redo_bridge()) {
		Dictionary status;
		status["ok"] = false;
		status["can_apply"] = false;
		status["error"] = ERR_UNAVAILABLE;
		status["message"] = "UndoRedoBridge is unavailable.";
		return status;
	}

	return UndoRedoBridge::get_singleton()->can_apply_scene_plan(p_plan);
}

Dictionary EditorAIService::apply_scene_plan(const Ref<SceneSynthesisPlan> &p_plan) {
	if (!has_undo_redo_bridge()) {
		Dictionary status;
		status["ok"] = false;
		status["can_apply"] = false;
		status["error"] = ERR_UNAVAILABLE;
		status["message"] = "UndoRedoBridge is unavailable.";
		return status;
	}

	return UndoRedoBridge::get_singleton()->apply_scene_plan(p_plan);
}

Dictionary EditorAIService::can_apply_gdscript_patch(const Ref<GDScriptRepairPatch> &p_patch) const {
	if (!has_undo_redo_bridge()) {
		Dictionary status;
		status["ok"] = false;
		status["can_apply"] = false;
		status["error"] = ERR_UNAVAILABLE;
		status["message"] = "UndoRedoBridge is unavailable.";
		return status;
	}

	return UndoRedoBridge::get_singleton()->can_apply_gdscript_patch(p_patch);
}

Dictionary EditorAIService::apply_gdscript_patch(const Ref<GDScriptRepairPatch> &p_patch) {
	if (!has_undo_redo_bridge()) {
		Dictionary status;
		status["ok"] = false;
		status["can_apply"] = false;
		status["error"] = ERR_UNAVAILABLE;
		status["message"] = "UndoRedoBridge is unavailable.";
		return status;
	}

	return UndoRedoBridge::get_singleton()->apply_gdscript_patch(p_patch);
}

Dictionary EditorAIService::resolve_scene_synthesis_task(const Ref<AITaskHandle> &p_task_handle, const String &p_prompt, const Dictionary &p_metadata) const {
	Dictionary result;
	result["ok"] = false;
	result["plan"] = Variant();
	result["preview"] = Dictionary();
	result["apply_status"] = Dictionary();

	if (p_task_handle.is_null()) {
		result["message"] = "Scene synthesis task handle is null.";
		return result;
	}
	if (!p_task_handle->is_finished_successfully()) {
		result["message"] = "Scene synthesis task did not complete successfully.";
		result["task"] = p_task_handle->get_result_snapshot();
		return result;
	}

	const String source_ir = p_task_handle->get_final_text();
	if (source_ir.strip_edges().is_empty()) {
		result["message"] = "Scene synthesis task returned empty IR.";
		result["task"] = p_task_handle->get_result_snapshot();
		return result;
	}

	Dictionary merged_metadata = p_task_handle->get_metadata();
	const Array metadata_keys = p_metadata.keys();
	for (int32_t i = 0; i < metadata_keys.size(); i++) {
		merged_metadata[metadata_keys[i]] = p_metadata[metadata_keys[i]];
	}

	const Dictionary validation = validate_scene_plan_ir(source_ir);
	result["validation"] = validation;
	if (!(validation.has("valid") && bool(validation["valid"]))) {
		result["message"] = "Scene synthesis IR validation failed.";
		result["task"] = p_task_handle->get_result_snapshot();
		return result;
	}

	const Ref<SceneSynthesisPlan> plan = parse_scene_plan_ir(source_ir, p_prompt, merged_metadata);
	if (plan.is_null()) {
		result["message"] = "Scene synthesis IR could not be parsed into a plan.";
		result["task"] = p_task_handle->get_result_snapshot();
		return result;
	}

	result["ok"] = true;
	result["message"] = "Scene synthesis task resolved successfully.";
	result["plan"] = plan;
	result["preview"] = build_scene_plan_preview(plan);
	result["apply_status"] = can_apply_scene_plan(plan);
	result["task"] = p_task_handle->get_result_snapshot();
	return result;
}

Dictionary EditorAIService::resolve_script_repair_task(const Ref<AITaskHandle> &p_task_handle, const String &p_script_path, const String &p_diagnostic_message, const Dictionary &p_metadata) const {
	Dictionary result;
	result["ok"] = false;
	result["patch"] = Variant();
	result["preview"] = Dictionary();
	result["apply_status"] = Dictionary();

	if (p_task_handle.is_null()) {
		result["message"] = "Script repair task handle is null.";
		return result;
	}
	if (!p_task_handle->is_finished_successfully()) {
		result["message"] = "Script repair task did not complete successfully.";
		result["task"] = p_task_handle->get_result_snapshot();
		return result;
	}

	const String source_ir = p_task_handle->get_final_text();
	if (source_ir.strip_edges().is_empty()) {
		result["message"] = "Script repair task returned empty IR.";
		result["task"] = p_task_handle->get_result_snapshot();
		return result;
	}

	Dictionary task_metadata = p_task_handle->get_metadata();
	Dictionary merged_metadata = task_metadata;
	const Array metadata_keys = p_metadata.keys();
	for (int32_t i = 0; i < metadata_keys.size(); i++) {
		merged_metadata[metadata_keys[i]] = p_metadata[metadata_keys[i]];
	}

	const String resolved_script_path = !p_script_path.is_empty() ? p_script_path : (task_metadata.has("script_path") ? String(task_metadata["script_path"]) : String());
	const String resolved_diagnostic = !p_diagnostic_message.is_empty() ? p_diagnostic_message : (task_metadata.has("diagnostics") ? String(task_metadata["diagnostics"]) : String());

	const Dictionary validation = validate_gdscript_patch_ir(source_ir);
	result["validation"] = validation;
	if (!(validation.has("valid") && bool(validation["valid"]))) {
		result["message"] = "Script repair IR validation failed.";
		result["task"] = p_task_handle->get_result_snapshot();
		return result;
	}

	const Ref<GDScriptRepairPatch> patch = parse_gdscript_patch_ir(source_ir, resolved_script_path, resolved_diagnostic, merged_metadata);
	if (patch.is_null()) {
		result["message"] = "Script repair IR could not be parsed into a patch.";
		result["task"] = p_task_handle->get_result_snapshot();
		return result;
	}

	result["ok"] = true;
	result["message"] = "Script repair task resolved successfully.";
	result["patch"] = patch;
	result["preview"] = build_gdscript_patch_preview(patch);
	result["apply_status"] = can_apply_gdscript_patch(patch);
	result["task"] = p_task_handle->get_result_snapshot();
	return result;
}

Ref<AITaskHandle> EditorAIService::request_scene_synthesis(const String &p_prompt, const Dictionary &p_context) {
	if (p_prompt.is_empty()) {
		emit_signal(SNAME("request_failed"), REQUEST_KIND_SCENE_SYNTHESIS, "Scene synthesis prompt must not be empty.");
		return _fail_task("Scene synthesis prompt must not be empty.");
	}

	Error err = ensure_default_model_loaded();
	if (err != OK) {
		const String message = "EditorAIService could not load the default AI model.";
		emit_signal(SNAME("request_failed"), REQUEST_KIND_SCENE_SYNTHESIS, message);
		return _fail_task(message);
	}

	Ref<AICompletionRequest> request;
	request.instantiate();
	request->set_model_rid(default_model_rid);
	request->set_prompt(p_prompt);
	request->set_caller_tag("editor_scene_synthesis");

	Dictionary metadata = collect_scene_request_context(p_context);
	metadata["editor_service"] = "EditorAIService";
	metadata["request_kind"] = "scene_synthesis";
	metadata["output_format"] = "scene_plan_ir";
	request->set_metadata(metadata);

	Ref<AITaskHandle> handle = _get_runtime_server()->submit_completion(request);
	_record_task(REQUEST_KIND_SCENE_SYNTHESIS, handle);
	return handle;
}

Ref<AITaskHandle> EditorAIService::request_script_repair(const String &p_script_path, const String &p_diagnostics, const String &p_code_snippet, const Dictionary &p_context) {
	if (p_script_path.is_empty() || p_diagnostics.is_empty()) {
		emit_signal(SNAME("request_failed"), REQUEST_KIND_SCRIPT_REPAIR, "Script repair requires both script_path and diagnostics.");
		return _fail_task("Script repair requires both script_path and diagnostics.");
	}

	Error err = ensure_default_model_loaded();
	if (err != OK) {
		const String message = "EditorAIService could not load the default AI model.";
		emit_signal(SNAME("request_failed"), REQUEST_KIND_SCRIPT_REPAIR, message);
		return _fail_task(message);
	}

	Ref<AICompletionRequest> request;
	request.instantiate();
	request->set_model_rid(default_model_rid);
	request->set_caller_tag("editor_script_repair");

	String prompt = vformat("Repair GDScript file: %s\nDiagnostics:\n%s", p_script_path, p_diagnostics);
	if (!p_code_snippet.is_empty()) {
		prompt += vformat("\nCode Snippet:\n%s", p_code_snippet);
	}
	request->set_prompt(prompt);

	Dictionary metadata = collect_script_repair_context(p_script_path, p_diagnostics, p_code_snippet, p_context);
	metadata["editor_service"] = "EditorAIService";
	metadata["request_kind"] = "script_repair";
	metadata["script_path"] = p_script_path;
	metadata["output_format"] = "gdscript_patch";
	request->set_metadata(metadata);

	Ref<AITaskHandle> handle = _get_runtime_server()->submit_completion(request);
	_record_task(REQUEST_KIND_SCRIPT_REPAIR, handle);
	return handle;
}

void EditorAIService::cancel_task(const Ref<AITaskHandle> &p_task_handle) {
	AIRuntimeServer *runtime_server = _get_runtime_server();
	if (runtime_server == nullptr) {
		return;
	}

	runtime_server->cancel_task(p_task_handle);
}

void EditorAIService::poll() {
	AIRuntimeServer *runtime_server = _get_runtime_server();
	if (runtime_server != nullptr) {
		runtime_server->poll_completed();
	}
}

Ref<AITaskHandle> EditorAIService::get_last_scene_synthesis_task() const {
	return last_scene_synthesis_task;
}

Ref<AITaskHandle> EditorAIService::get_last_script_repair_task() const {
	return last_script_repair_task;
}

Dictionary EditorAIService::get_service_status() const {
	Dictionary status;
	status["ready"] = is_ready();
	status["has_runtime_server"] = has_runtime_server();
	status["has_context_collector"] = has_context_collector();
	status["has_gdscript_repair_engine"] = has_gdscript_repair_engine();
	status["has_node_graph_intent_parser"] = has_node_graph_intent_parser();
	status["has_preview_diff"] = has_preview_diff();
	status["has_undo_redo_bridge"] = has_undo_redo_bridge();
	status["has_default_model"] = default_model.is_valid();
	status["has_loaded_default_model"] = has_loaded_default_model();
	status["default_model_rid"] = default_model_rid;
	status["submitted_scene_requests"] = static_cast<int64_t>(submitted_scene_requests);
	status["submitted_script_repairs"] = static_cast<int64_t>(submitted_script_repairs);
	status["collector_status"] = has_context_collector() ? EditorContextCollector::get_singleton()->get_collector_status() : Dictionary();
	status["repair_engine_status"] = has_gdscript_repair_engine() ? GDScriptRepairEngine::get_singleton()->get_engine_status() : Dictionary();
	status["scene_parser_status"] = has_node_graph_intent_parser() ? NodeGraphIntentParser::get_singleton()->get_parser_status() : Dictionary();
	status["preview_status"] = has_preview_diff() ? EditorAIPreviewDiff::get_singleton()->get_current_preview() : Dictionary();
	status["undo_redo_status"] = has_undo_redo_bridge() ? UndoRedoBridge::get_singleton()->get_bridge_status() : Dictionary();
	status["runtime_stats"] = has_runtime_server() ? _get_runtime_server()->get_runtime_stats() : Dictionary();
	return status;
}

void EditorAIService::_record_task(RequestKind p_kind, const Ref<AITaskHandle> &p_task_handle) {
	if (p_kind == REQUEST_KIND_SCENE_SYNTHESIS) {
		last_scene_synthesis_task = p_task_handle;
		submitted_scene_requests++;
	} else {
		last_script_repair_task = p_task_handle;
		submitted_script_repairs++;
	}

	emit_signal(SNAME("request_submitted"), p_kind, p_task_handle);
}

AIRuntimeServer *EditorAIService::_get_runtime_server() const {
	return AIRuntimeServer::get_singleton();
}

Ref<AITaskHandle> EditorAIService::_fail_task(const String &p_message) const {
	Ref<AITaskHandle> handle;
	handle.instantiate();
	handle->fail(ERR_INVALID_PARAMETER, p_message);
	return handle;
}

EditorAIService::EditorAIService() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
}

EditorAIService::~EditorAIService() {
	if (singleton == this) {
		unload_default_model();
		singleton = nullptr;
	}
}
