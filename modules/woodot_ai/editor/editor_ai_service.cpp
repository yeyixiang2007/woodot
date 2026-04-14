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
#include "modules/woodot_ai/resources/ai_model_resource.h"
#include "modules/woodot_ai/runtime/ai_requests.h"
#include "modules/woodot_ai/runtime/ai_runtime_server.h"

EditorAIService *EditorAIService::singleton = nullptr;

void EditorAIService::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_default_model", "model"), &EditorAIService::set_default_model);
	ClassDB::bind_method(D_METHOD("get_default_model"), &EditorAIService::get_default_model);
	ClassDB::bind_method(D_METHOD("has_runtime_server"), &EditorAIService::has_runtime_server);
	ClassDB::bind_method(D_METHOD("is_ready"), &EditorAIService::is_ready);
	ClassDB::bind_method(D_METHOD("has_loaded_default_model"), &EditorAIService::has_loaded_default_model);
	ClassDB::bind_method(D_METHOD("ensure_default_model_loaded"), &EditorAIService::ensure_default_model_loaded);
	ClassDB::bind_method(D_METHOD("unload_default_model"), &EditorAIService::unload_default_model);
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

	Dictionary metadata = p_context;
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

	Dictionary metadata = p_context;
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
	status["has_default_model"] = default_model.is_valid();
	status["has_loaded_default_model"] = has_loaded_default_model();
	status["default_model_rid"] = default_model_rid;
	status["submitted_scene_requests"] = static_cast<int64_t>(submitted_scene_requests);
	status["submitted_script_repairs"] = static_cast<int64_t>(submitted_script_repairs);
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
