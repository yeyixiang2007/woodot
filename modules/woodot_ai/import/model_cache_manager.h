/**************************************************************************/
/*  model_cache_manager.h                                                 */
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

#pragma once

#include "core/object/object.h"

class AIImportOrchestrator;
class AIModelResource;

class ModelCacheManager : public Object {
	GDCLASS(ModelCacheManager, Object);

	static ModelCacheManager *singleton;

	uint64_t stored_sidecars = 0;
	mutable uint64_t loaded_sidecars = 0;
	uint64_t stored_model_manifests = 0;

	static String _setting_path_enabled();
	static String _setting_path_root_dir();
	static String _setting_path_use_imported_sidecars();

	AIImportOrchestrator *_get_orchestrator() const;
	String _get_effective_root_dir() const;
	String _get_effective_annotation_dir() const;
	Error _ensure_dir_exists(const String &p_dir) const;

protected:
	static void _bind_methods();

public:
	static ModelCacheManager *get_singleton();
	static void register_project_settings();

	bool is_cache_enabled() const;
	bool use_imported_sidecars() const;
	bool has_import_orchestrator() const;

	String get_cache_root_dir() const;
	String get_model_cache_dir() const;
	String get_platform_artifact_cache_dir() const;
	String get_embedding_cache_dir() const;
	String get_annotation_sidecar_dir() const;
	Error ensure_cache_layout() const;

	String build_model_cache_key(const Ref<AIModelResource> &p_model) const;
	String build_import_cache_key(const String &p_source_path, const String &p_importer_name, const Dictionary &p_context = Dictionary()) const;
	String build_platform_artifact_key(const String &p_backend_name, const String &p_platform_tag, const Dictionary &p_options = Dictionary()) const;

	Dictionary build_model_manifest(const Ref<AIModelResource> &p_model, const Dictionary &p_runtime_info = Dictionary()) const;
	Error store_model_manifest(const Ref<AIModelResource> &p_model, const Dictionary &p_runtime_info = Dictionary());
	Dictionary load_model_manifest(const String &p_model_cache_key) const;

	String build_annotation_sidecar_path(const String &p_source_path, const String &p_importer_name, const Dictionary &p_context = Dictionary()) const;
	bool has_annotation_sidecar(const String &p_source_path, const String &p_importer_name, const Dictionary &p_context = Dictionary()) const;
	Error store_annotation_sidecar(const String &p_source_path, const String &p_importer_name, const Dictionary &p_sidecar, const Dictionary &p_context = Dictionary());
	Dictionary load_annotation_sidecar(const String &p_source_path, const String &p_importer_name, const Dictionary &p_context = Dictionary()) const;
	Dictionary get_cached_annotation_status(const String &p_source_path, const String &p_importer_name, const Dictionary &p_context = Dictionary()) const;

	Dictionary get_manager_status() const;

	ModelCacheManager();
	~ModelCacheManager();
};
