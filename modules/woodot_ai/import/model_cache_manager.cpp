/**************************************************************************/
/*  model_cache_manager.cpp                                               */
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

#include "modules/woodot_ai/import/model_cache_manager.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/object/class_db.h"
#include "modules/woodot_ai/import/ai_import_orchestrator.h"
#include "modules/woodot_ai/resources/ai_model_resource.h"

ModelCacheManager *ModelCacheManager::singleton = nullptr;

String ModelCacheManager::_setting_path_enabled() {
	return "woodot_ai/cache/enabled";
}

String ModelCacheManager::_setting_path_root_dir() {
	return "woodot_ai/cache/root_dir";
}

String ModelCacheManager::_setting_path_use_imported_sidecars() {
	return "woodot_ai/cache/use_imported_sidecars";
}

String ModelCacheManager::_setting_path_export_allowed_categories() {
	return "woodot_ai/export/allowed_artifact_categories";
}

String ModelCacheManager::_setting_path_export_allowed_platform_tags() {
	return "woodot_ai/export/allowed_platform_tags";
}

void ModelCacheManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_cache_enabled"), &ModelCacheManager::is_cache_enabled);
	ClassDB::bind_method(D_METHOD("use_imported_sidecars"), &ModelCacheManager::use_imported_sidecars);
	ClassDB::bind_method(D_METHOD("has_import_orchestrator"), &ModelCacheManager::has_import_orchestrator);
	ClassDB::bind_method(D_METHOD("get_export_allowed_categories"), &ModelCacheManager::get_export_allowed_categories);
	ClassDB::bind_method(D_METHOD("get_export_allowed_platform_tags"), &ModelCacheManager::get_export_allowed_platform_tags);
	ClassDB::bind_method(D_METHOD("get_cache_root_dir"), &ModelCacheManager::get_cache_root_dir);
	ClassDB::bind_method(D_METHOD("get_model_cache_dir"), &ModelCacheManager::get_model_cache_dir);
	ClassDB::bind_method(D_METHOD("get_platform_artifact_cache_dir"), &ModelCacheManager::get_platform_artifact_cache_dir);
	ClassDB::bind_method(D_METHOD("get_embedding_cache_dir"), &ModelCacheManager::get_embedding_cache_dir);
	ClassDB::bind_method(D_METHOD("get_annotation_sidecar_dir"), &ModelCacheManager::get_annotation_sidecar_dir);
	ClassDB::bind_method(D_METHOD("ensure_cache_layout"), &ModelCacheManager::ensure_cache_layout);
	ClassDB::bind_method(D_METHOD("build_model_cache_key", "model"), &ModelCacheManager::build_model_cache_key);
	ClassDB::bind_method(D_METHOD("build_import_cache_key", "source_path", "importer_name", "context"), &ModelCacheManager::build_import_cache_key, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("build_platform_artifact_key", "backend_name", "platform_tag", "options"), &ModelCacheManager::build_platform_artifact_key, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("build_model_manifest", "model", "runtime_info"), &ModelCacheManager::build_model_manifest, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("store_model_manifest", "model", "runtime_info"), &ModelCacheManager::store_model_manifest, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("load_model_manifest", "model_cache_key"), &ModelCacheManager::load_model_manifest);
	ClassDB::bind_method(D_METHOD("build_annotation_sidecar_path", "source_path", "importer_name", "context"), &ModelCacheManager::build_annotation_sidecar_path, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("has_annotation_sidecar", "source_path", "importer_name", "context"), &ModelCacheManager::has_annotation_sidecar, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("store_annotation_sidecar", "source_path", "importer_name", "sidecar", "context"), &ModelCacheManager::store_annotation_sidecar, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("load_annotation_sidecar", "source_path", "importer_name", "context"), &ModelCacheManager::load_annotation_sidecar, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("get_cached_annotation_status", "source_path", "importer_name", "context"), &ModelCacheManager::get_cached_annotation_status, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("get_export_artifact_whitelist"), &ModelCacheManager::get_export_artifact_whitelist);
	ClassDB::bind_method(D_METHOD("is_export_artifact_allowed", "category", "platform_tag", "metadata"), &ModelCacheManager::is_export_artifact_allowed, DEFVAL(String()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("build_export_bundle_plan", "platform_tag", "requested_artifacts", "metadata"), &ModelCacheManager::build_export_bundle_plan, DEFVAL(Array()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("get_manager_status"), &ModelCacheManager::get_manager_status);
}

ModelCacheManager *ModelCacheManager::get_singleton() {
	return singleton;
}

void ModelCacheManager::register_project_settings() {
	GLOBAL_DEF(PropertyInfo(Variant::BOOL, _setting_path_enabled()), true);
	GLOBAL_DEF(PropertyInfo(Variant::STRING, _setting_path_root_dir()), "");
	GLOBAL_DEF(PropertyInfo(Variant::BOOL, _setting_path_use_imported_sidecars()), true);
	PackedStringArray default_categories;
	default_categories.push_back("platform_artifact");
	default_categories.push_back("annotation_sidecar");
	GLOBAL_DEF(PropertyInfo(Variant::PACKED_STRING_ARRAY, _setting_path_export_allowed_categories()), default_categories);
	GLOBAL_DEF(PropertyInfo(Variant::PACKED_STRING_ARRAY, _setting_path_export_allowed_platform_tags()), PackedStringArray());
}

bool ModelCacheManager::is_cache_enabled() const {
	return bool(GLOBAL_GET(_setting_path_enabled()));
}

bool ModelCacheManager::use_imported_sidecars() const {
	return bool(GLOBAL_GET(_setting_path_use_imported_sidecars()));
}

bool ModelCacheManager::has_import_orchestrator() const {
	return _get_orchestrator() != nullptr;
}

PackedStringArray ModelCacheManager::get_export_allowed_categories() const {
	return PackedStringArray(GLOBAL_GET(_setting_path_export_allowed_categories()));
}

PackedStringArray ModelCacheManager::get_export_allowed_platform_tags() const {
	return PackedStringArray(GLOBAL_GET(_setting_path_export_allowed_platform_tags()));
}

String ModelCacheManager::get_cache_root_dir() const {
	return _get_effective_root_dir();
}

String ModelCacheManager::get_model_cache_dir() const {
	return get_cache_root_dir().path_join("models");
}

String ModelCacheManager::get_platform_artifact_cache_dir() const {
	return get_cache_root_dir().path_join("artifacts");
}

String ModelCacheManager::get_embedding_cache_dir() const {
	return get_cache_root_dir().path_join("embeddings");
}

String ModelCacheManager::get_annotation_sidecar_dir() const {
	return _get_effective_annotation_dir();
}

Error ModelCacheManager::ensure_cache_layout() const {
	if (!is_cache_enabled()) {
		return OK;
	}

	Error err = _ensure_dir_exists(get_cache_root_dir());
	ERR_FAIL_COND_V(err != OK, err);
	err = _ensure_dir_exists(get_model_cache_dir());
	ERR_FAIL_COND_V(err != OK, err);
	err = _ensure_dir_exists(get_platform_artifact_cache_dir());
	ERR_FAIL_COND_V(err != OK, err);
	err = _ensure_dir_exists(get_embedding_cache_dir());
	ERR_FAIL_COND_V(err != OK, err);
	err = _ensure_dir_exists(get_annotation_sidecar_dir());
	ERR_FAIL_COND_V(err != OK, err);
	return OK;
}

String ModelCacheManager::build_model_cache_key(const Ref<AIModelResource> &p_model) const {
	ERR_FAIL_COND_V_MSG(p_model.is_null(), String(), "ModelCacheManager requires a valid AIModelResource.");

	String serialized = String(p_model->get_backend_type()) + "|";
	serialized += p_model->get_model_path() + "|";
	serialized += p_model->get_parameter_fingerprint();
	return serialized.sha256_text();
}

String ModelCacheManager::build_import_cache_key(const String &p_source_path, const String &p_importer_name, const Dictionary &p_context) const {
	String serialized = p_source_path + "|" + p_importer_name + "|";
	serialized += JSON::stringify(p_context, String(), true, true);
	return serialized.sha256_text();
}

String ModelCacheManager::build_platform_artifact_key(const String &p_backend_name, const String &p_platform_tag, const Dictionary &p_options) const {
	String serialized = p_backend_name + "|" + p_platform_tag + "|";
	serialized += JSON::stringify(p_options, String(), true, true);
	return serialized.sha256_text();
}

Dictionary ModelCacheManager::build_model_manifest(const Ref<AIModelResource> &p_model, const Dictionary &p_runtime_info) const {
	Dictionary manifest;
	if (p_model.is_null()) {
		return manifest;
	}

	manifest["schema"] = "woodot_ai.model_manifest.v1";
	manifest["cache_key"] = build_model_cache_key(p_model);
	manifest["backend_type"] = String(p_model->get_backend_type());
	manifest["model_path"] = p_model->get_model_path();
	manifest["parameter_fingerprint"] = p_model->get_parameter_fingerprint();
	manifest["context_size"] = p_model->get_context_size();
	manifest["n_threads"] = p_model->get_n_threads();
	manifest["n_gpu_layers"] = p_model->get_n_gpu_layers();
	manifest["quantization"] = p_model->get_quantization();
	manifest["runtime_info"] = p_runtime_info;
	return manifest;
}

Error ModelCacheManager::store_model_manifest(const Ref<AIModelResource> &p_model, const Dictionary &p_runtime_info) {
	ERR_FAIL_COND_V(p_model.is_null(), ERR_INVALID_PARAMETER);
	if (!is_cache_enabled()) {
		return OK;
	}

	Error err = ensure_cache_layout();
	ERR_FAIL_COND_V(err != OK, err);

	const String cache_key = build_model_cache_key(p_model);
	const String manifest_path = get_model_cache_dir().path_join(cache_key + ".json");
	Ref<FileAccess> file = FileAccess::open(manifest_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_V(file.is_null(), err == OK ? ERR_CANT_OPEN : err);

	file->store_string(JSON::stringify(build_model_manifest(p_model, p_runtime_info), "\t", true, true));
	stored_model_manifests++;
	return OK;
}

Dictionary ModelCacheManager::load_model_manifest(const String &p_model_cache_key) const {
	Dictionary manifest;
	if (p_model_cache_key.is_empty()) {
		return manifest;
	}

	const String manifest_path = get_model_cache_dir().path_join(p_model_cache_key + ".json");
	if (!FileAccess::exists(manifest_path)) {
		return manifest;
	}

	const String file_contents = FileAccess::get_file_as_string(manifest_path);
	const Variant parsed = JSON::parse_string(file_contents);
	if (parsed.get_type() == Variant::DICTIONARY) {
		manifest = parsed;
	}
	return manifest;
}

String ModelCacheManager::build_annotation_sidecar_path(const String &p_source_path, const String &p_importer_name, const Dictionary &p_context) const {
	const String cache_key = build_import_cache_key(p_source_path, p_importer_name, p_context);
	return get_annotation_sidecar_dir().path_join(cache_key + ".json");
}

bool ModelCacheManager::has_annotation_sidecar(const String &p_source_path, const String &p_importer_name, const Dictionary &p_context) const {
	return FileAccess::exists(build_annotation_sidecar_path(p_source_path, p_importer_name, p_context));
}

Error ModelCacheManager::store_annotation_sidecar(const String &p_source_path, const String &p_importer_name, const Dictionary &p_sidecar, const Dictionary &p_context) {
	if (!is_cache_enabled()) {
		return OK;
	}

	Error err = ensure_cache_layout();
	ERR_FAIL_COND_V(err != OK, err);

	Dictionary payload = p_sidecar;
	payload["cache_key"] = build_import_cache_key(p_source_path, p_importer_name, p_context);
	payload["source_path"] = p_source_path;
	payload["importer_name"] = p_importer_name;

	const String file_path = build_annotation_sidecar_path(p_source_path, p_importer_name, p_context);
	Ref<FileAccess> file = FileAccess::open(file_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_V(file.is_null(), err == OK ? ERR_CANT_OPEN : err);

	file->store_string(JSON::stringify(payload, "\t", true, true));
	stored_sidecars++;
	return OK;
}

Dictionary ModelCacheManager::load_annotation_sidecar(const String &p_source_path, const String &p_importer_name, const Dictionary &p_context) const {
	Dictionary sidecar;
	const String file_path = build_annotation_sidecar_path(p_source_path, p_importer_name, p_context);
	if (!FileAccess::exists(file_path)) {
		return sidecar;
	}

	const String file_contents = FileAccess::get_file_as_string(file_path);
	const Variant parsed = JSON::parse_string(file_contents);
	if (parsed.get_type() == Variant::DICTIONARY) {
		sidecar = parsed;
		loaded_sidecars++;
	}
	return sidecar;
}

Dictionary ModelCacheManager::get_cached_annotation_status(const String &p_source_path, const String &p_importer_name, const Dictionary &p_context) const {
	Dictionary status;
	const String file_path = build_annotation_sidecar_path(p_source_path, p_importer_name, p_context);
	const bool exists = FileAccess::exists(file_path);

	status["cache_enabled"] = is_cache_enabled();
	status["cache_key"] = build_import_cache_key(p_source_path, p_importer_name, p_context);
	status["sidecar_path"] = file_path;
	status["exists"] = exists;
	status["loaded"] = false;
	status["sidecar"] = Dictionary();

	if (exists) {
		status["loaded"] = true;
		status["sidecar"] = load_annotation_sidecar(p_source_path, p_importer_name, p_context);
	}

	return status;
}

Dictionary ModelCacheManager::get_export_artifact_whitelist() const {
	Dictionary whitelist;
	whitelist["allowed_categories"] = get_export_allowed_categories();
	whitelist["allowed_platform_tags"] = get_export_allowed_platform_tags();
	whitelist["cache_enabled"] = is_cache_enabled();
	whitelist["use_imported_sidecars"] = use_imported_sidecars();
	return whitelist;
}

bool ModelCacheManager::is_export_artifact_allowed(const String &p_category, const String &p_platform_tag, const Dictionary &p_metadata) const {
	const PackedStringArray allowed_categories = get_export_allowed_categories();
	if (!allowed_categories.has(p_category)) {
		return false;
	}

	const PackedStringArray allowed_platform_tags = get_export_allowed_platform_tags();
	if (!p_platform_tag.is_empty() && !allowed_platform_tags.is_empty() && !allowed_platform_tags.has(p_platform_tag)) {
		return false;
	}

	if (p_metadata.has("required") && !bool(p_metadata["required"]) && p_category == "platform_artifact" && !allowed_categories.has("optional_platform_artifact")) {
		return false;
	}

	return true;
}

Dictionary ModelCacheManager::build_export_bundle_plan(const String &p_platform_tag, const Array &p_requested_artifacts, const Dictionary &p_metadata) const {
	Dictionary plan;
	Array allowed_artifacts;
	Array rejected_artifacts;
	Array warnings;

	for (int32_t i = 0; i < p_requested_artifacts.size(); i++) {
		if (p_requested_artifacts[i].get_type() != Variant::DICTIONARY) {
			continue;
		}

		Dictionary artifact = p_requested_artifacts[i];
		const String category = artifact.has("category") ? String(artifact["category"]) : String();
		const String artifact_platform = artifact.has("platform_tag") ? String(artifact["platform_tag"]) : p_platform_tag;
		const Dictionary artifact_metadata = artifact.has("metadata") && artifact["metadata"].get_type() == Variant::DICTIONARY ? Dictionary(artifact["metadata"]) : Dictionary();

		if (is_export_artifact_allowed(category, artifact_platform, artifact_metadata)) {
			allowed_artifacts.push_back(artifact);
		} else {
			rejected_artifacts.push_back(artifact);
		}
	}

	if (rejected_artifacts.size() > 0) {
		warnings.push_back("Some requested artifacts were excluded by the export whitelist.");
	}

	plan["schema"] = "woodot_ai.export_bundle_plan.v1";
	plan["platform_tag"] = p_platform_tag;
	plan["whitelist"] = get_export_artifact_whitelist();
	plan["allowed_artifacts"] = allowed_artifacts;
	plan["rejected_artifacts"] = rejected_artifacts;
	plan["warnings"] = warnings;
	plan["metadata"] = p_metadata;
	return plan;
}

Dictionary ModelCacheManager::get_manager_status() const {
	Dictionary status;
	status["cache_enabled"] = is_cache_enabled();
	status["use_imported_sidecars"] = use_imported_sidecars();
	status["has_import_orchestrator"] = has_import_orchestrator();
	status["cache_root_dir"] = get_cache_root_dir();
	status["model_cache_dir"] = get_model_cache_dir();
	status["platform_artifact_cache_dir"] = get_platform_artifact_cache_dir();
	status["embedding_cache_dir"] = get_embedding_cache_dir();
	status["annotation_sidecar_dir"] = get_annotation_sidecar_dir();
	status["export_whitelist"] = get_export_artifact_whitelist();
	status["stored_sidecars"] = static_cast<int64_t>(stored_sidecars);
	status["loaded_sidecars"] = static_cast<int64_t>(loaded_sidecars);
	status["stored_model_manifests"] = static_cast<int64_t>(stored_model_manifests);
	return status;
}

AIImportOrchestrator *ModelCacheManager::_get_orchestrator() const {
	return AIImportOrchestrator::get_singleton();
}

String ModelCacheManager::_get_effective_root_dir() const {
	const String configured_root = String(GLOBAL_GET(_setting_path_root_dir())).strip_edges();
	if (!configured_root.is_empty()) {
		return configured_root.simplify_path();
	}
	return ProjectSettings::get_singleton()->get_project_data_path().path_join("woodot_ai_cache");
}

String ModelCacheManager::_get_effective_annotation_dir() const {
	if (use_imported_sidecars()) {
		return ProjectSettings::get_singleton()->get_imported_files_path().path_join("woodot_ai_annotation");
	}
	return get_cache_root_dir().path_join("annotation_sidecars");
}

Error ModelCacheManager::_ensure_dir_exists(const String &p_dir) const {
	Ref<DirAccess> dir_access = DirAccess::create_for_path(p_dir);
	ERR_FAIL_COND_V(dir_access.is_null(), ERR_CANT_CREATE);
	return dir_access->make_dir_recursive(p_dir);
}

ModelCacheManager::ModelCacheManager() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
}

ModelCacheManager::~ModelCacheManager() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
