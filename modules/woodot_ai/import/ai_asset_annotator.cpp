/**************************************************************************/
/*  ai_asset_annotator.cpp                                                */
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

#include "modules/woodot_ai/import/ai_asset_annotator.h"

#include "core/io/json.h"
#include "core/object/class_db.h"
#include "modules/woodot_ai/import/ai_import_orchestrator.h"
#include "modules/woodot_ai/runtime/ai_requests.h"
#include "modules/woodot_ai/runtime/ai_runtime_server.h"

AIAssetAnnotator *AIAssetAnnotator::singleton = nullptr;

void AIAssetAnnotator::_bind_methods() {
	ClassDB::bind_method(D_METHOD("has_import_orchestrator"), &AIAssetAnnotator::has_import_orchestrator);
	ClassDB::bind_method(D_METHOD("has_runtime_server"), &AIAssetAnnotator::has_runtime_server);
	ClassDB::bind_method(D_METHOD("is_ready"), &AIAssetAnnotator::is_ready);
	ClassDB::bind_method(D_METHOD("build_annotation_prompt", "source_path", "importer_name", "options"), &AIAssetAnnotator::build_annotation_prompt, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("prepare_annotation_request", "source_path", "importer_name", "options"), &AIAssetAnnotator::prepare_annotation_request, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("submit_annotation", "source_path", "importer_name", "options"), &AIAssetAnnotator::submit_annotation, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("validate_annotation_output", "output_text"), &AIAssetAnnotator::validate_annotation_output);
	ClassDB::bind_method(D_METHOD("parse_annotation_output", "output_text"), &AIAssetAnnotator::parse_annotation_output);
	ClassDB::bind_method(D_METHOD("build_annotation_sidecar", "annotation", "source_path", "importer_name", "import_context"), &AIAssetAnnotator::build_annotation_sidecar, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("resolve_annotation_task", "task_handle", "source_path", "importer_name", "import_context"), &AIAssetAnnotator::resolve_annotation_task, DEFVAL(String()), DEFVAL(String()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("get_annotator_status"), &AIAssetAnnotator::get_annotator_status);

	ADD_SIGNAL(MethodInfo("annotation_submitted",
			PropertyInfo(Variant::STRING, "source_path"),
			PropertyInfo(Variant::STRING, "importer_name"),
			PropertyInfo(Variant::OBJECT, "task_handle", PROPERTY_HINT_RESOURCE_TYPE, "AITaskHandle")));
	ADD_SIGNAL(MethodInfo("annotation_resolved",
			PropertyInfo(Variant::STRING, "source_path"),
			PropertyInfo(Variant::DICTIONARY, "sidecar")));
	ADD_SIGNAL(MethodInfo("annotation_failed",
			PropertyInfo(Variant::STRING, "source_path"),
			PropertyInfo(Variant::STRING, "message")));
}

AIAssetAnnotator *AIAssetAnnotator::get_singleton() {
	return singleton;
}

bool AIAssetAnnotator::has_import_orchestrator() const {
	return _get_orchestrator() != nullptr;
}

bool AIAssetAnnotator::has_runtime_server() const {
	return _get_runtime_server() != nullptr;
}

bool AIAssetAnnotator::is_ready() const {
	return has_import_orchestrator() && has_runtime_server();
}

String AIAssetAnnotator::build_annotation_prompt(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options) const {
	if (p_options.has("annotation_prompt")) {
		return String(p_options["annotation_prompt"]);
	}
	if (p_options.has("prompt")) {
		return String(p_options["prompt"]);
	}

	const String asset_kind = p_options.has("asset_kind") ? String(p_options["asset_kind"]) : p_source_path.get_extension().to_lower();
	const String user_hint = p_options.has("annotation_hint") ? String(p_options["annotation_hint"]) : String();

	String prompt = "You are annotating an imported asset for an editor-side asset database.\n";
	prompt += "Return strict JSON with keys: summary, tags, keywords, confidence.\n";
	prompt += "summary must be a short sentence. tags and keywords must be string arrays. confidence must be a number from 0 to 1.\n";
	prompt += vformat("Source path: %s\n", p_source_path);
	prompt += vformat("Importer: %s\n", p_importer_name);
	if (!asset_kind.is_empty()) {
		prompt += vformat("Asset kind: %s\n", asset_kind);
	}
	if (!user_hint.is_empty()) {
		prompt += vformat("Additional hint: %s\n", user_hint);
	}
	if (p_options.has("context_excerpt")) {
		prompt += vformat("Context excerpt: %s\n", String(p_options["context_excerpt"]));
	}
	prompt += "Focus on searchability and editor organization rather than creative prose.";
	return prompt;
}

Ref<AICompletionRequest> AIAssetAnnotator::prepare_annotation_request(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options) const {
	AIImportOrchestrator *orchestrator = _get_orchestrator();
	ERR_FAIL_NULL_V_MSG(orchestrator, Ref<AICompletionRequest>(), "AIAssetAnnotator requires AIImportOrchestrator.");

	const String prompt = build_annotation_prompt(p_source_path, p_importer_name, p_options);
	Ref<AICompletionRequest> request = orchestrator->create_annotation_request(p_source_path, p_importer_name, prompt, RID(), p_options);
	if (request.is_valid()) {
		Dictionary request_metadata = request->get_metadata();
		request_metadata["annotator"] = "AIAssetAnnotator";
		request_metadata["annotation_schema"] = "woodot_ai.asset_annotation.v1";
		request->set_metadata(request_metadata);
	}
	return request;
}

Ref<AITaskHandle> AIAssetAnnotator::submit_annotation(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options) {
	AIRuntimeServer *runtime_server = _get_runtime_server();
	if (runtime_server == nullptr) {
		emit_signal(SNAME("annotation_failed"), p_source_path, "AIRuntimeServer is unavailable.");
		failed_annotations++;
		return _fail_task("AIRuntimeServer is unavailable.");
	}

	Ref<AICompletionRequest> request = prepare_annotation_request(p_source_path, p_importer_name, p_options);
	if (request.is_null()) {
		emit_signal(SNAME("annotation_failed"), p_source_path, "Annotation request could not be prepared.");
		failed_annotations++;
		return _fail_task("Annotation request could not be prepared.");
	}

	Ref<AITaskHandle> task_handle = runtime_server->submit_completion(request);
	if (task_handle.is_null()) {
		emit_signal(SNAME("annotation_failed"), p_source_path, "AIRuntimeServer rejected the annotation request.");
		failed_annotations++;
		return _fail_task("AIRuntimeServer rejected the annotation request.");
	}

	submitted_annotations++;
	emit_signal(SNAME("annotation_submitted"), p_source_path, p_importer_name, task_handle);
	return task_handle;
}

Dictionary AIAssetAnnotator::validate_annotation_output(const String &p_output_text) const {
	Dictionary status;
	Array errors;
	Array warnings;

	const String trimmed = p_output_text.strip_edges();
	status["valid"] = false;
	status["format"] = "empty";
	status["errors"] = errors;
	status["warnings"] = warnings;

	if (trimmed.is_empty()) {
		errors.push_back("Annotation output is empty.");
		status["errors"] = errors;
		return status;
	}

	const Variant parsed = JSON::parse_string(trimmed);
	if (parsed.get_type() == Variant::DICTIONARY) {
		const Dictionary parsed_dict = parsed;
		status["format"] = "json";
		if (!parsed_dict.has("summary")) {
			errors.push_back("Annotation JSON must contain a summary field.");
		}
		if (!parsed_dict.has("tags")) {
			warnings.push_back("Annotation JSON is missing tags; an empty list will be used.");
		}
		if (!parsed_dict.has("keywords")) {
			warnings.push_back("Annotation JSON is missing keywords; an empty list will be used.");
		}
		status["valid"] = errors.is_empty();
		status["errors"] = errors;
		status["warnings"] = warnings;
		return status;
	}

	warnings.push_back("Annotation output is not JSON; plain-text fallback parsing will be used.");
	status["valid"] = true;
	status["format"] = "plain_text";
	status["warnings"] = warnings;
	return status;
}

Dictionary AIAssetAnnotator::parse_annotation_output(const String &p_output_text) const {
	Dictionary annotation;
	const String trimmed = p_output_text.strip_edges();
	const Variant parsed = JSON::parse_string(trimmed);

	String summary;
	PackedStringArray tags;
	PackedStringArray keywords;
	double confidence = 0.0;

	if (parsed.get_type() == Variant::DICTIONARY) {
		const Dictionary parsed_dict = parsed;
		summary = parsed_dict.has("summary") ? String(parsed_dict["summary"]).strip_edges() : String();
		tags = _variant_to_string_array(parsed_dict.has("tags") ? parsed_dict["tags"] : Variant());
		keywords = _variant_to_string_array(parsed_dict.has("keywords") ? parsed_dict["keywords"] : Variant());
		if (parsed_dict.has("confidence")) {
			confidence = CLAMP(double(parsed_dict["confidence"]), 0.0, 1.0);
		}
	} else {
		summary = trimmed;
	}

	if (summary.is_empty()) {
		summary = "Imported asset annotation is available.";
	}

	annotation["summary"] = summary;
	annotation["tags"] = _packed_to_array(tags);
	annotation["keywords"] = _packed_to_array(keywords);
	annotation["confidence"] = confidence;
	annotation["raw_output"] = trimmed;
	annotation["format"] = parsed.get_type() == Variant::DICTIONARY ? "json" : "plain_text";
	return annotation;
}

Dictionary AIAssetAnnotator::build_annotation_sidecar(const Dictionary &p_annotation, const String &p_source_path, const String &p_importer_name, const Dictionary &p_import_context) const {
	Dictionary sidecar;
	Variant summary = String();
	Variant tags = Array();
	Variant keywords = Array();
	Variant confidence = 0.0;
	Variant format = String("unknown");
	Variant raw_output = String();

	if (p_annotation.has("summary")) {
		summary = p_annotation["summary"];
	}
	if (p_annotation.has("tags")) {
		tags = p_annotation["tags"];
	}
	if (p_annotation.has("keywords")) {
		keywords = p_annotation["keywords"];
	}
	if (p_annotation.has("confidence")) {
		confidence = p_annotation["confidence"];
	}
	if (p_annotation.has("format")) {
		format = p_annotation["format"];
	}
	if (p_annotation.has("raw_output")) {
		raw_output = p_annotation["raw_output"];
	}

	sidecar["schema"] = "woodot_ai.asset_annotation.v1";
	sidecar["source_path"] = p_source_path;
	sidecar["importer_name"] = p_importer_name;
	sidecar["summary"] = summary;
	sidecar["tags"] = tags;
	sidecar["keywords"] = keywords;
	sidecar["confidence"] = confidence;
	sidecar["format"] = format;
	sidecar["raw_output"] = raw_output;
	sidecar["import_context"] = p_import_context;
	return sidecar;
}

Dictionary AIAssetAnnotator::resolve_annotation_task(const Ref<AITaskHandle> &p_task_handle, const String &p_source_path, const String &p_importer_name, const Dictionary &p_import_context) {
	Dictionary result;
	result["ok"] = false;
	result["annotation"] = Dictionary();
	result["sidecar"] = Dictionary();
	result["validation"] = Dictionary();

	if (p_task_handle.is_null()) {
		result["message"] = "Annotation task handle is null.";
		return result;
	}
	if (!p_task_handle->is_finished_successfully()) {
		failed_annotations++;
		result["message"] = "Annotation task did not complete successfully.";
		result["task"] = p_task_handle->get_result_snapshot();
		return result;
	}

	const String output_text = p_task_handle->get_final_text();
	const Dictionary validation = validate_annotation_output(output_text);
	result["validation"] = validation;

	if (!(validation.has("valid") && bool(validation["valid"]))) {
		failed_annotations++;
		result["message"] = "Annotation output validation failed.";
		result["task"] = p_task_handle->get_result_snapshot();
		return result;
	}

	const Dictionary annotation = parse_annotation_output(output_text);
	const Dictionary task_metadata = p_task_handle->get_metadata();
	String resolved_source_path = p_source_path;
	if (resolved_source_path.is_empty() && task_metadata.has("source_path")) {
		resolved_source_path = String(task_metadata["source_path"]);
	}

	String resolved_importer_name = p_importer_name;
	if (resolved_importer_name.is_empty() && task_metadata.has("importer_name")) {
		resolved_importer_name = String(task_metadata["importer_name"]);
	}

	Dictionary merged_context = task_metadata;
	const Array context_keys = p_import_context.keys();
	for (int32_t i = 0; i < context_keys.size(); i++) {
		merged_context[context_keys[i]] = p_import_context[context_keys[i]];
	}

	const Dictionary sidecar = build_annotation_sidecar(annotation, resolved_source_path, resolved_importer_name, merged_context);

	resolved_annotations++;
	result["ok"] = true;
	result["message"] = "Annotation task resolved successfully.";
	result["annotation"] = annotation;
	result["sidecar"] = sidecar;
	result["task"] = p_task_handle->get_result_snapshot();
	emit_signal(SNAME("annotation_resolved"), resolved_source_path, sidecar);
	return result;
}

Dictionary AIAssetAnnotator::get_annotator_status() const {
	Dictionary status;
	status["ready"] = is_ready();
	status["has_import_orchestrator"] = has_import_orchestrator();
	status["has_runtime_server"] = has_runtime_server();
	status["submitted_annotations"] = static_cast<int64_t>(submitted_annotations);
	status["resolved_annotations"] = static_cast<int64_t>(resolved_annotations);
	status["failed_annotations"] = static_cast<int64_t>(failed_annotations);
	return status;
}

AIImportOrchestrator *AIAssetAnnotator::_get_orchestrator() const {
	return AIImportOrchestrator::get_singleton();
}

AIRuntimeServer *AIAssetAnnotator::_get_runtime_server() const {
	return AIRuntimeServer::get_singleton();
}

String AIAssetAnnotator::_sanitize_keyword(const String &p_value) {
	return p_value.strip_edges().replace("\n", " ").replace("\r", " ");
}

PackedStringArray AIAssetAnnotator::_variant_to_string_array(const Variant &p_value) {
	PackedStringArray values;
	if (p_value.get_type() != Variant::ARRAY && p_value.get_type() != Variant::PACKED_STRING_ARRAY) {
		return values;
	}

	if (p_value.get_type() == Variant::PACKED_STRING_ARRAY) {
		const PackedStringArray raw_values = p_value;
		for (int32_t i = 0; i < raw_values.size(); i++) {
			const String normalized = _sanitize_keyword(raw_values[i]);
			if (!normalized.is_empty() && !values.has(normalized)) {
				values.push_back(normalized);
			}
		}
		return values;
	}

	const Array raw_values = p_value;
	for (int32_t i = 0; i < raw_values.size(); i++) {
		const String normalized = _sanitize_keyword(String(raw_values[i]));
		if (!normalized.is_empty() && !values.has(normalized)) {
			values.push_back(normalized);
		}
	}
	return values;
}

Array AIAssetAnnotator::_packed_to_array(const PackedStringArray &p_values) {
	Array values;
	for (int32_t i = 0; i < p_values.size(); i++) {
		values.push_back(p_values[i]);
	}
	return values;
}

Ref<AITaskHandle> AIAssetAnnotator::_fail_task(const String &p_message) const {
	Ref<AITaskHandle> handle;
	handle.instantiate();
	handle->fail(ERR_INVALID_PARAMETER, p_message);
	return handle;
}

AIAssetAnnotator::AIAssetAnnotator() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
}

AIAssetAnnotator::~AIAssetAnnotator() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
