/**************************************************************************/
/*  ai_extension_api.cpp                                                  */
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

#include "modules/woodot_ai/import/ai_extension_api.h"

#include "core/object/class_db.h"
#include "modules/woodot_ai/import/ai_import_orchestrator.h"
#include "modules/woodot_ai/import/model_cache_manager.h"
#include "modules/woodot_ai/runtime/ai_runtime_server.h"

AIExtensionAPI *AIExtensionAPI::singleton = nullptr;

void AIExtensionAPI::_bind_methods() {
	ClassDB::bind_method(D_METHOD("has_runtime_server"), &AIExtensionAPI::has_runtime_server);
	ClassDB::bind_method(D_METHOD("has_import_orchestrator"), &AIExtensionAPI::has_import_orchestrator);
	ClassDB::bind_method(D_METHOD("has_model_cache_manager"), &AIExtensionAPI::has_model_cache_manager);
	ClassDB::bind_method(D_METHOD("is_ready"), &AIExtensionAPI::is_ready);
	ClassDB::bind_method(D_METHOD("get_runtime_stats"), &AIExtensionAPI::get_runtime_stats);
	ClassDB::bind_method(D_METHOD("get_import_status"), &AIExtensionAPI::get_import_status);
	ClassDB::bind_method(D_METHOD("get_cache_status"), &AIExtensionAPI::get_cache_status);
	ClassDB::bind_method(D_METHOD("get_extension_status"), &AIExtensionAPI::get_extension_status);
	ClassDB::bind_method(D_METHOD("build_import_context", "source_path", "importer_name", "options"), &AIExtensionAPI::build_import_context, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("orchestrate_import", "source_path", "importer_name", "options"), &AIExtensionAPI::orchestrate_import, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("get_cached_annotation_status", "source_path", "importer_name", "context"), &AIExtensionAPI::get_cached_annotation_status, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("get_export_artifact_whitelist"), &AIExtensionAPI::get_export_artifact_whitelist);
	ClassDB::bind_method(D_METHOD("is_export_artifact_allowed", "category", "platform_tag", "metadata"), &AIExtensionAPI::is_export_artifact_allowed, DEFVAL(String()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("build_export_bundle_plan", "platform_tag", "requested_artifacts", "metadata"), &AIExtensionAPI::build_export_bundle_plan, DEFVAL(Array()), DEFVAL(Dictionary()));
}

AIExtensionAPI *AIExtensionAPI::get_singleton() {
	return singleton;
}

bool AIExtensionAPI::has_runtime_server() const {
	return AIRuntimeServer::get_singleton() != nullptr;
}

bool AIExtensionAPI::has_import_orchestrator() const {
	return AIImportOrchestrator::get_singleton() != nullptr;
}

bool AIExtensionAPI::has_model_cache_manager() const {
	return ModelCacheManager::get_singleton() != nullptr;
}

bool AIExtensionAPI::is_ready() const {
	return has_runtime_server() && has_import_orchestrator() && has_model_cache_manager();
}

Dictionary AIExtensionAPI::get_runtime_stats() const {
	return has_runtime_server() ? AIRuntimeServer::get_singleton()->get_runtime_stats() : Dictionary();
}

Dictionary AIExtensionAPI::get_import_status() const {
	return has_import_orchestrator() ? AIImportOrchestrator::get_singleton()->get_orchestrator_status() : Dictionary();
}

Dictionary AIExtensionAPI::get_cache_status() const {
	return has_model_cache_manager() ? ModelCacheManager::get_singleton()->get_manager_status() : Dictionary();
}

Dictionary AIExtensionAPI::get_extension_status() const {
	Dictionary status;
	status["ready"] = is_ready();
	status["has_runtime_server"] = has_runtime_server();
	status["has_import_orchestrator"] = has_import_orchestrator();
	status["has_model_cache_manager"] = has_model_cache_manager();
	status["runtime_stats"] = get_runtime_stats();
	status["import_status"] = get_import_status();
	status["cache_status"] = get_cache_status();
	return status;
}

Dictionary AIExtensionAPI::build_import_context(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options) const {
	return has_import_orchestrator() ? AIImportOrchestrator::get_singleton()->build_import_context(p_source_path, p_importer_name, p_options) : Dictionary();
}

Dictionary AIExtensionAPI::orchestrate_import(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options) const {
	return has_import_orchestrator() ? AIImportOrchestrator::get_singleton()->orchestrate_import(p_source_path, p_importer_name, p_options) : Dictionary();
}

Dictionary AIExtensionAPI::get_cached_annotation_status(const String &p_source_path, const String &p_importer_name, const Dictionary &p_context) const {
	return has_model_cache_manager() ? ModelCacheManager::get_singleton()->get_cached_annotation_status(p_source_path, p_importer_name, p_context) : Dictionary();
}

Dictionary AIExtensionAPI::get_export_artifact_whitelist() const {
	return has_model_cache_manager() ? ModelCacheManager::get_singleton()->get_export_artifact_whitelist() : Dictionary();
}

bool AIExtensionAPI::is_export_artifact_allowed(const String &p_category, const String &p_platform_tag, const Dictionary &p_metadata) const {
	return has_model_cache_manager() && ModelCacheManager::get_singleton()->is_export_artifact_allowed(p_category, p_platform_tag, p_metadata);
}

Dictionary AIExtensionAPI::build_export_bundle_plan(const String &p_platform_tag, const Array &p_requested_artifacts, const Dictionary &p_metadata) const {
	return has_model_cache_manager() ? ModelCacheManager::get_singleton()->build_export_bundle_plan(p_platform_tag, p_requested_artifacts, p_metadata) : Dictionary();
}

AIExtensionAPI::AIExtensionAPI() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
}

AIExtensionAPI::~AIExtensionAPI() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
