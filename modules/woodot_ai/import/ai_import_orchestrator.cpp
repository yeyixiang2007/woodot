/**************************************************************************/
/*  ai_import_orchestrator.cpp                                            */
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

#include "modules/woodot_ai/import/ai_import_orchestrator.h"

#include "core/config/project_settings.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "modules/woodot_ai/import/ai_asset_annotator.h"
#include "modules/woodot_ai/import/ai_mesh_post_processor.h"
#include "modules/woodot_ai/import/model_cache_manager.h"
#include "modules/woodot_ai/import/ai_texture_enhancer.h"
#include "modules/woodot_ai/resources/ai_model_resource.h"
#include "modules/woodot_ai/runtime/ai_runtime_server.h"

#ifdef TOOLS_ENABLED
#include "modules/woodot_ai/editor/editor_ai_service.h"
#endif

AIImportOrchestrator *AIImportOrchestrator::singleton = nullptr;

String AIImportOrchestrator::_setting_path_enabled() {
	return "woodot_ai/import/enabled";
}

String AIImportOrchestrator::_setting_path_fail_open() {
	return "woodot_ai/import/fail_open";
}

String AIImportOrchestrator::_setting_path_asset_annotation_enabled() {
	return "woodot_ai/import/passes/asset_annotation_enabled";
}

String AIImportOrchestrator::_setting_path_mesh_postprocess_enabled() {
	return "woodot_ai/import/passes/mesh_postprocess_enabled";
}

String AIImportOrchestrator::_setting_path_texture_enhancement_enabled() {
	return "woodot_ai/import/passes/texture_enhancement_enabled";
}

void AIImportOrchestrator::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &AIImportOrchestrator::set_enabled);
	ClassDB::bind_method(D_METHOD("is_enabled"), &AIImportOrchestrator::is_enabled);
	ClassDB::bind_method(D_METHOD("set_fail_open", "fail_open"), &AIImportOrchestrator::set_fail_open);
	ClassDB::bind_method(D_METHOD("is_fail_open"), &AIImportOrchestrator::is_fail_open);
	ClassDB::bind_method(D_METHOD("set_asset_annotation_enabled", "enabled"), &AIImportOrchestrator::set_asset_annotation_enabled);
	ClassDB::bind_method(D_METHOD("is_asset_annotation_enabled"), &AIImportOrchestrator::is_asset_annotation_enabled);
	ClassDB::bind_method(D_METHOD("set_mesh_postprocess_enabled", "enabled"), &AIImportOrchestrator::set_mesh_postprocess_enabled);
	ClassDB::bind_method(D_METHOD("is_mesh_postprocess_enabled"), &AIImportOrchestrator::is_mesh_postprocess_enabled);
	ClassDB::bind_method(D_METHOD("set_texture_enhancement_enabled", "enabled"), &AIImportOrchestrator::set_texture_enhancement_enabled);
	ClassDB::bind_method(D_METHOD("is_texture_enhancement_enabled"), &AIImportOrchestrator::is_texture_enhancement_enabled);
	ClassDB::bind_method(D_METHOD("set_pass_enabled", "pass_type", "enabled"), &AIImportOrchestrator::set_pass_enabled);
	ClassDB::bind_method(D_METHOD("is_pass_enabled", "pass_type"), &AIImportOrchestrator::is_pass_enabled);
	ClassDB::bind_method(D_METHOD("set_default_model", "model"), &AIImportOrchestrator::set_default_model);
	ClassDB::bind_method(D_METHOD("get_default_model"), &AIImportOrchestrator::get_default_model);
	ClassDB::bind_method(D_METHOD("has_loaded_default_model"), &AIImportOrchestrator::has_loaded_default_model);
	ClassDB::bind_method(D_METHOD("ensure_default_model_loaded"), &AIImportOrchestrator::ensure_default_model_loaded);
	ClassDB::bind_method(D_METHOD("unload_default_model"), &AIImportOrchestrator::unload_default_model);
	ClassDB::bind_method(D_METHOD("has_runtime_server"), &AIImportOrchestrator::has_runtime_server);
	ClassDB::bind_method(D_METHOD("has_editor_ai_service"), &AIImportOrchestrator::has_editor_ai_service);
	ClassDB::bind_method(D_METHOD("has_asset_annotator"), &AIImportOrchestrator::has_asset_annotator);
	ClassDB::bind_method(D_METHOD("has_mesh_post_processor"), &AIImportOrchestrator::has_mesh_post_processor);
	ClassDB::bind_method(D_METHOD("has_texture_enhancer"), &AIImportOrchestrator::has_texture_enhancer);
	ClassDB::bind_method(D_METHOD("has_model_cache_manager"), &AIImportOrchestrator::has_model_cache_manager);
	ClassDB::bind_method(D_METHOD("is_ready"), &AIImportOrchestrator::is_ready);
	ClassDB::bind_method(D_METHOD("reload_project_settings"), &AIImportOrchestrator::reload_project_settings);
	ClassDB::bind_method(D_METHOD("save_project_settings"), &AIImportOrchestrator::save_project_settings);
	ClassDB::bind_method(D_METHOD("get_policy_settings"), &AIImportOrchestrator::get_policy_settings);
	ClassDB::bind_method(D_METHOD("build_import_context", "source_path", "importer_name", "options"), &AIImportOrchestrator::build_import_context, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("create_annotation_request", "source_path", "importer_name", "prompt", "model_rid", "options"), &AIImportOrchestrator::create_annotation_request, DEFVAL(RID()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("orchestrate_import", "source_path", "importer_name", "options"), &AIImportOrchestrator::orchestrate_import, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("get_orchestrator_status"), &AIImportOrchestrator::get_orchestrator_status);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "fail_open"), "set_fail_open", "is_fail_open");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "asset_annotation_enabled"), "set_asset_annotation_enabled", "is_asset_annotation_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "mesh_postprocess_enabled"), "set_mesh_postprocess_enabled", "is_mesh_postprocess_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "texture_enhancement_enabled"), "set_texture_enhancement_enabled", "is_texture_enhancement_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "default_model", PROPERTY_HINT_RESOURCE_TYPE, "AIModelResource"), "set_default_model", "get_default_model");

	ADD_SIGNAL(MethodInfo("orchestrator_state_changed"));
	ADD_SIGNAL(MethodInfo("request_prepared",
			PropertyInfo(Variant::STRING, "source_path"),
			PropertyInfo(Variant::STRING, "importer_name"),
			PropertyInfo(Variant::OBJECT, "request", PROPERTY_HINT_RESOURCE_TYPE, "AICompletionRequest")));
	ADD_SIGNAL(MethodInfo("fallback_triggered",
			PropertyInfo(Variant::STRING, "source_path"),
			PropertyInfo(Variant::STRING, "reason")));

	BIND_ENUM_CONSTANT(PASS_ASSET_ANNOTATION);
	BIND_ENUM_CONSTANT(PASS_MESH_POSTPROCESS);
	BIND_ENUM_CONSTANT(PASS_TEXTURE_ENHANCEMENT);
}

AIImportOrchestrator *AIImportOrchestrator::get_singleton() {
	return singleton;
}

void AIImportOrchestrator::register_project_settings() {
	GLOBAL_DEF(PropertyInfo(Variant::BOOL, _setting_path_enabled()), false);
	GLOBAL_DEF(PropertyInfo(Variant::BOOL, _setting_path_fail_open()), true);
	GLOBAL_DEF(PropertyInfo(Variant::BOOL, _setting_path_asset_annotation_enabled()), true);
	GLOBAL_DEF(PropertyInfo(Variant::BOOL, _setting_path_mesh_postprocess_enabled()), false);
	GLOBAL_DEF(PropertyInfo(Variant::BOOL, _setting_path_texture_enhancement_enabled()), false);
}

void AIImportOrchestrator::set_enabled(bool p_enabled) {
	if (enabled == p_enabled) {
		return;
	}

	enabled = p_enabled;
	emit_signal(SNAME("orchestrator_state_changed"));
}

bool AIImportOrchestrator::is_enabled() const {
	return enabled;
}

void AIImportOrchestrator::set_fail_open(bool p_fail_open) {
	if (fail_open == p_fail_open) {
		return;
	}

	fail_open = p_fail_open;
	emit_signal(SNAME("orchestrator_state_changed"));
}

bool AIImportOrchestrator::is_fail_open() const {
	return fail_open;
}

void AIImportOrchestrator::set_asset_annotation_enabled(bool p_enabled) {
	set_pass_enabled(PASS_ASSET_ANNOTATION, p_enabled);
}

bool AIImportOrchestrator::is_asset_annotation_enabled() const {
	return asset_annotation_enabled;
}

void AIImportOrchestrator::set_mesh_postprocess_enabled(bool p_enabled) {
	set_pass_enabled(PASS_MESH_POSTPROCESS, p_enabled);
}

bool AIImportOrchestrator::is_mesh_postprocess_enabled() const {
	return mesh_postprocess_enabled;
}

void AIImportOrchestrator::set_texture_enhancement_enabled(bool p_enabled) {
	set_pass_enabled(PASS_TEXTURE_ENHANCEMENT, p_enabled);
}

bool AIImportOrchestrator::is_texture_enhancement_enabled() const {
	return texture_enhancement_enabled;
}

void AIImportOrchestrator::set_pass_enabled(PassType p_pass_type, bool p_enabled) {
	bool changed = false;

	switch (p_pass_type) {
		case PASS_ASSET_ANNOTATION:
			changed = asset_annotation_enabled != p_enabled;
			asset_annotation_enabled = p_enabled;
			break;
		case PASS_MESH_POSTPROCESS:
			changed = mesh_postprocess_enabled != p_enabled;
			mesh_postprocess_enabled = p_enabled;
			break;
		case PASS_TEXTURE_ENHANCEMENT:
			changed = texture_enhancement_enabled != p_enabled;
			texture_enhancement_enabled = p_enabled;
			break;
		default:
			return;
	}

	if (changed) {
		emit_signal(SNAME("orchestrator_state_changed"));
	}
}

bool AIImportOrchestrator::is_pass_enabled(PassType p_pass_type) const {
	switch (p_pass_type) {
		case PASS_ASSET_ANNOTATION:
			return asset_annotation_enabled;
		case PASS_MESH_POSTPROCESS:
			return mesh_postprocess_enabled;
		case PASS_TEXTURE_ENHANCEMENT:
			return texture_enhancement_enabled;
		default:
			return false;
	}
}

void AIImportOrchestrator::set_default_model(const Ref<AIModelResource> &p_model) {
	if (default_model == p_model) {
		return;
	}

	if (default_model_rid.is_valid()) {
		unload_default_model();
	}

	default_model = p_model;
	emit_signal(SNAME("orchestrator_state_changed"));
}

Ref<AIModelResource> AIImportOrchestrator::get_default_model() const {
	return default_model;
}

bool AIImportOrchestrator::has_loaded_default_model() const {
	AIRuntimeServer *runtime_server = _get_runtime_server();
	if (runtime_server == nullptr || !default_model_rid.is_valid()) {
		return false;
	}

	return runtime_server->has_model(default_model_rid);
}

Error AIImportOrchestrator::ensure_default_model_loaded() {
	AIRuntimeServer *runtime_server = _get_runtime_server();
	ERR_FAIL_NULL_V_MSG(runtime_server, ERR_UNAVAILABLE, "AIImportOrchestrator requires AIRuntimeServer.");
	ERR_FAIL_COND_V_MSG(default_model.is_null(), ERR_INVALID_PARAMETER, "AIImportOrchestrator requires a default AIModelResource.");

	if (default_model_rid.is_valid()) {
		if (!runtime_server->has_model(default_model_rid) || default_model->is_runtime_dirty()) {
			runtime_server->unload_model(default_model_rid);
			default_model_rid = RID();
		}
	}

	if (!default_model_rid.is_valid()) {
		default_model_rid = runtime_server->load_model(default_model);
		ERR_FAIL_COND_V_MSG(!default_model_rid.is_valid(), ERR_CANT_CREATE, "AIImportOrchestrator failed to load the default AI model.");
		default_model->mark_runtime_clean();
	}

	return OK;
}

void AIImportOrchestrator::unload_default_model() {
	AIRuntimeServer *runtime_server = _get_runtime_server();
	if (runtime_server != nullptr && default_model_rid.is_valid()) {
		runtime_server->unload_model(default_model_rid);
	}

	default_model_rid = RID();
}

bool AIImportOrchestrator::has_runtime_server() const {
	return _get_runtime_server() != nullptr;
}

bool AIImportOrchestrator::has_editor_ai_service() const {
	return _get_editor_ai_service() != nullptr;
}

bool AIImportOrchestrator::has_asset_annotator() const {
	return _get_asset_annotator() != nullptr;
}

bool AIImportOrchestrator::has_mesh_post_processor() const {
	return _get_mesh_post_processor() != nullptr;
}

bool AIImportOrchestrator::has_texture_enhancer() const {
	return _get_texture_enhancer() != nullptr;
}

bool AIImportOrchestrator::has_model_cache_manager() const {
	return _get_model_cache_manager() != nullptr;
}

bool AIImportOrchestrator::is_ready() const {
	return enabled && has_runtime_server() && (default_model.is_valid() || has_editor_ai_service());
}

void AIImportOrchestrator::reload_project_settings() {
	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	ERR_FAIL_NULL(project_settings);

	const bool configured_enabled = project_settings->has_setting(_setting_path_enabled()) ? bool(project_settings->get_setting_with_override(_setting_path_enabled())) : false;
	const bool configured_fail_open = project_settings->has_setting(_setting_path_fail_open()) ? bool(project_settings->get_setting_with_override(_setting_path_fail_open())) : true;
	const bool configured_asset_annotation = project_settings->has_setting(_setting_path_asset_annotation_enabled()) ? bool(project_settings->get_setting_with_override(_setting_path_asset_annotation_enabled())) : true;
	const bool configured_mesh_postprocess = project_settings->has_setting(_setting_path_mesh_postprocess_enabled()) ? bool(project_settings->get_setting_with_override(_setting_path_mesh_postprocess_enabled())) : false;
	const bool configured_texture_enhancement = project_settings->has_setting(_setting_path_texture_enhancement_enabled()) ? bool(project_settings->get_setting_with_override(_setting_path_texture_enhancement_enabled())) : false;

	_apply_settings_values(
			configured_enabled,
			configured_fail_open,
			configured_asset_annotation,
			configured_mesh_postprocess,
			configured_texture_enhancement);
}

void AIImportOrchestrator::save_project_settings() const {
	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	ERR_FAIL_NULL(project_settings);

	project_settings->set_setting(_setting_path_enabled(), enabled);
	project_settings->set_setting(_setting_path_fail_open(), fail_open);
	project_settings->set_setting(_setting_path_asset_annotation_enabled(), asset_annotation_enabled);
	project_settings->set_setting(_setting_path_mesh_postprocess_enabled(), mesh_postprocess_enabled);
	project_settings->set_setting(_setting_path_texture_enhancement_enabled(), texture_enhancement_enabled);
}

Dictionary AIImportOrchestrator::get_policy_settings() const {
	Dictionary settings;
	PackedStringArray project_setting_paths;
	project_setting_paths.push_back(_setting_path_enabled());
	project_setting_paths.push_back(_setting_path_fail_open());
	project_setting_paths.push_back(_setting_path_asset_annotation_enabled());
	project_setting_paths.push_back(_setting_path_mesh_postprocess_enabled());
	project_setting_paths.push_back(_setting_path_texture_enhancement_enabled());

	settings["enabled"] = enabled;
	settings["fail_open"] = fail_open;
	settings["asset_annotation_enabled"] = asset_annotation_enabled;
	settings["mesh_postprocess_enabled"] = mesh_postprocess_enabled;
	settings["texture_enhancement_enabled"] = texture_enhancement_enabled;
	settings["project_setting_paths"] = project_setting_paths;
	return settings;
}

Dictionary AIImportOrchestrator::build_import_context(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options) const {
	Dictionary context;
	context["schema"] = "woodot_ai.import_context.v1";
	context["source_path"] = p_source_path;
	context["source_file"] = p_source_path.get_file();
	context["importer_name"] = p_importer_name;
	context["enabled"] = enabled;
	context["fail_open"] = fail_open;
	context["enabled_passes"] = _get_enabled_pass_names();
	context["options"] = p_options;
	context["runtime_available"] = has_runtime_server();
	context["editor_service_available"] = has_editor_ai_service();
	context["cache_manager_available"] = has_model_cache_manager();
	context["default_model_assigned"] = default_model.is_valid();
	context["default_model_loaded"] = has_loaded_default_model();
	context["cached_annotation_status"] = has_model_cache_manager() ? _get_model_cache_manager()->get_cached_annotation_status(p_source_path, p_importer_name, p_options) : Dictionary();

	if (default_model.is_valid()) {
		context["default_model_backend"] = String(default_model->get_backend_type());
		context["default_model_fingerprint"] = default_model->get_parameter_fingerprint();
	}

	return context;
}

Ref<AICompletionRequest> AIImportOrchestrator::create_annotation_request(const String &p_source_path, const String &p_importer_name, const String &p_prompt, const RID &p_model_rid, const Dictionary &p_options) {
	ERR_FAIL_COND_V_MSG(p_prompt.strip_edges().is_empty(), Ref<AICompletionRequest>(), "AIImportOrchestrator annotation prompt must not be empty.");

	RID resolved_model_rid = p_model_rid;
	if (!resolved_model_rid.is_valid()) {
		if (default_model.is_valid()) {
			Error err = ensure_default_model_loaded();
			ERR_FAIL_COND_V_MSG(err != OK, Ref<AICompletionRequest>(), "AIImportOrchestrator could not load the default model for annotation requests.");
			resolved_model_rid = default_model_rid;
		}
#ifdef TOOLS_ENABLED
		else if (has_editor_ai_service()) {
			EditorAIService *editor_service = _get_editor_ai_service();
			Ref<AIModelResource> service_model = editor_service->get_default_model();
			if (service_model.is_valid()) {
				if (default_model != service_model) {
					if (default_model_rid.is_valid()) {
						unload_default_model();
					}
					default_model = service_model;
				}

				Error err = ensure_default_model_loaded();
				ERR_FAIL_COND_V_MSG(err != OK, Ref<AICompletionRequest>(), "AIImportOrchestrator could not load the EditorAIService default model for annotation requests.");
				resolved_model_rid = default_model_rid;
			}
		}
#endif
	}

	ERR_FAIL_COND_V_MSG(!resolved_model_rid.is_valid(), Ref<AICompletionRequest>(), "AIImportOrchestrator requires a valid model RID or default model to prepare annotation requests.");

	Ref<AICompletionRequest> request;
	request.instantiate();
	request->set_model_rid(resolved_model_rid);
	request->set_prompt(p_prompt);
	request->set_caller_tag("import_asset_annotation");

	Dictionary request_metadata = build_import_context(p_source_path, p_importer_name, p_options);
	request_metadata["request_kind"] = "import_annotation";
	request_metadata["output_format"] = "import_annotation";
	request_metadata["pass_type"] = _get_pass_name(PASS_ASSET_ANNOTATION);
	request->set_metadata(request_metadata);
	return request;
}

Dictionary AIImportOrchestrator::orchestrate_import(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options) {
	inspected_imports++;

	Dictionary result = build_import_context(p_source_path, p_importer_name, p_options);
	result["ok"] = true;
	result["ai_applied"] = false;
	result["ai_planned"] = false;
	result["fallback_to_base_import"] = true;
	result["request_prepared"] = false;
	result["annotation_request"] = Variant();
	result["annotation_task"] = Variant();
	result["mesh_processing_plan"] = Dictionary();
	result["texture_enhancement_plan"] = Dictionary();
	result["message"] = "AI import pass is disabled.";

	const bool has_any_pass = asset_annotation_enabled || mesh_postprocess_enabled || texture_enhancement_enabled;
	if (!enabled) {
		fallback_imports++;
		emit_signal(SNAME("fallback_triggered"), p_source_path, "orchestrator_disabled");
		result["reason"] = "orchestrator_disabled";
		return result;
	}
	if (!has_any_pass) {
		fallback_imports++;
		emit_signal(SNAME("fallback_triggered"), p_source_path, "no_passes_enabled");
		result["message"] = "No import AI pass is enabled.";
		result["reason"] = "no_passes_enabled";
		return result;
	}

	ai_candidate_imports++;

	if (!has_runtime_server()) {
		fallback_imports++;
		emit_signal(SNAME("fallback_triggered"), p_source_path, "runtime_unavailable");
		result["ok"] = fail_open;
		result["message"] = "AIRuntimeServer is unavailable; falling back to base importer.";
		result["reason"] = "runtime_unavailable";
		return result;
	}

	if (asset_annotation_enabled) {
		if (has_asset_annotator()) {
			AIAssetAnnotator *annotator = _get_asset_annotator();
			Ref<AICompletionRequest> request = annotator->prepare_annotation_request(p_source_path, p_importer_name, p_options);
			if (request.is_valid()) {
				prepared_requests++;
				result["ai_planned"] = true;
				result["request_prepared"] = true;
				result["annotation_request"] = request;
				emit_signal(SNAME("request_prepared"), p_source_path, p_importer_name, request);

				Ref<AITaskHandle> task_handle = annotator->submit_annotation(p_source_path, p_importer_name, p_options);
				if (task_handle.is_valid()) {
					result["annotation_task"] = task_handle;
					result["fallback_to_base_import"] = fail_open;
					result["message"] = "Submitted asset annotation task.";
					result["reason"] = "annotation_submitted";
					return result;
				}
			}
		} else {
			const String annotation_prompt = p_options.has("annotation_prompt") ? String(p_options["annotation_prompt"]) : (p_options.has("prompt") ? String(p_options["prompt"]) : String());
			if (!annotation_prompt.is_empty()) {
				Ref<AICompletionRequest> request = create_annotation_request(p_source_path, p_importer_name, annotation_prompt, RID(), p_options);
				if (request.is_valid()) {
					prepared_requests++;
					result["ai_planned"] = true;
					result["request_prepared"] = true;
					result["annotation_request"] = request;
					result["fallback_to_base_import"] = fail_open;
					result["message"] = "Prepared asset annotation request.";
					result["reason"] = "request_prepared";
					emit_signal(SNAME("request_prepared"), p_source_path, p_importer_name, request);
					return result;
				}
			}
		}
	}

	if (mesh_postprocess_enabled && has_mesh_post_processor()) {
		result["mesh_processing_plan"] = _get_mesh_post_processor()->build_processing_plan(p_source_path, p_importer_name, p_options);
	}
	if (texture_enhancement_enabled && has_texture_enhancer()) {
		result["texture_enhancement_plan"] = _get_texture_enhancer()->build_enhancement_plan(p_source_path, p_importer_name, p_options);
	}

	fallback_imports++;
	emit_signal(SNAME("fallback_triggered"), p_source_path, "annotation_prompt_missing");
	result["message"] = "No annotation prompt was provided; falling back to base importer.";
	result["reason"] = "annotation_prompt_missing";
	return result;
}

Dictionary AIImportOrchestrator::get_orchestrator_status() const {
	Dictionary status;
	status["enabled"] = enabled;
	status["fail_open"] = fail_open;
	status["ready"] = is_ready();
	status["has_runtime_server"] = has_runtime_server();
	status["has_editor_ai_service"] = has_editor_ai_service();
	status["has_asset_annotator"] = has_asset_annotator();
	status["has_mesh_post_processor"] = has_mesh_post_processor();
	status["has_texture_enhancer"] = has_texture_enhancer();
	status["has_model_cache_manager"] = has_model_cache_manager();
	status["has_default_model"] = default_model.is_valid();
	status["has_loaded_default_model"] = has_loaded_default_model();
	status["default_model_rid"] = default_model_rid;
	status["enabled_passes"] = _get_enabled_pass_names();
	status["policy_settings"] = get_policy_settings();
	status["inspected_imports"] = static_cast<int64_t>(inspected_imports);
	status["ai_candidate_imports"] = static_cast<int64_t>(ai_candidate_imports);
	status["prepared_requests"] = static_cast<int64_t>(prepared_requests);
	status["fallback_imports"] = static_cast<int64_t>(fallback_imports);
	status["runtime_stats"] = has_runtime_server() ? _get_runtime_server()->get_runtime_stats() : Dictionary();
	status["editor_service_status"] = has_editor_ai_service() ? _get_editor_ai_service()->get_service_status() : Dictionary();
	status["asset_annotator_status"] = has_asset_annotator() ? _get_asset_annotator()->get_annotator_status() : Dictionary();
	status["mesh_post_processor_status"] = has_mesh_post_processor() ? _get_mesh_post_processor()->get_processor_status() : Dictionary();
	status["texture_enhancer_status"] = has_texture_enhancer() ? _get_texture_enhancer()->get_enhancer_status() : Dictionary();
	status["model_cache_manager_status"] = has_model_cache_manager() ? _get_model_cache_manager()->get_manager_status() : Dictionary();
	return status;
}

AIRuntimeServer *AIImportOrchestrator::_get_runtime_server() const {
	return AIRuntimeServer::get_singleton();
}

EditorAIService *AIImportOrchestrator::_get_editor_ai_service() const {
#ifdef TOOLS_ENABLED
	return EditorAIService::get_singleton();
#else
	return nullptr;
#endif
}

AIAssetAnnotator *AIImportOrchestrator::_get_asset_annotator() const {
	return AIAssetAnnotator::get_singleton();
}

AIMeshPostProcessor *AIImportOrchestrator::_get_mesh_post_processor() const {
	return AIMeshPostProcessor::get_singleton();
}

AITextureEnhancer *AIImportOrchestrator::_get_texture_enhancer() const {
	return AITextureEnhancer::get_singleton();
}

ModelCacheManager *AIImportOrchestrator::_get_model_cache_manager() const {
	return ModelCacheManager::get_singleton();
}

void AIImportOrchestrator::_apply_settings_values(bool p_enabled, bool p_fail_open, bool p_asset_annotation_enabled, bool p_mesh_postprocess_enabled, bool p_texture_enhancement_enabled) {
	const bool changed = enabled != p_enabled ||
			fail_open != p_fail_open ||
			asset_annotation_enabled != p_asset_annotation_enabled ||
			mesh_postprocess_enabled != p_mesh_postprocess_enabled ||
			texture_enhancement_enabled != p_texture_enhancement_enabled;

	enabled = p_enabled;
	fail_open = p_fail_open;
	asset_annotation_enabled = p_asset_annotation_enabled;
	mesh_postprocess_enabled = p_mesh_postprocess_enabled;
	texture_enhancement_enabled = p_texture_enhancement_enabled;

	if (changed) {
		emit_signal(SNAME("orchestrator_state_changed"));
	}
}

String AIImportOrchestrator::_get_pass_name(PassType p_pass_type) const {
	switch (p_pass_type) {
		case PASS_ASSET_ANNOTATION:
			return "asset_annotation";
		case PASS_MESH_POSTPROCESS:
			return "mesh_postprocess";
		case PASS_TEXTURE_ENHANCEMENT:
			return "texture_enhancement";
		default:
			return "unknown";
	}
}

Array AIImportOrchestrator::_get_enabled_pass_names() const {
	Array passes;

	if (asset_annotation_enabled) {
		passes.push_back(_get_pass_name(PASS_ASSET_ANNOTATION));
	}
	if (mesh_postprocess_enabled) {
		passes.push_back(_get_pass_name(PASS_MESH_POSTPROCESS));
	}
	if (texture_enhancement_enabled) {
		passes.push_back(_get_pass_name(PASS_TEXTURE_ENHANCEMENT));
	}

	return passes;
}

AIImportOrchestrator::AIImportOrchestrator() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
	ProjectSettings *project_settings = ProjectSettings::get_singleton();
	if (project_settings != nullptr) {
		reload_project_settings();
		project_settings->connect("settings_changed", callable_mp(this, &AIImportOrchestrator::reload_project_settings));
	}
}

AIImportOrchestrator::~AIImportOrchestrator() {
	if (singleton == this) {
		if (ProjectSettings::get_singleton() != nullptr && ProjectSettings::get_singleton()->is_connected("settings_changed", callable_mp(this, &AIImportOrchestrator::reload_project_settings))) {
			ProjectSettings::get_singleton()->disconnect("settings_changed", callable_mp(this, &AIImportOrchestrator::reload_project_settings));
		}
		unload_default_model();
		singleton = nullptr;
	}
}
