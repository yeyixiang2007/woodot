/**************************************************************************/
/*  ai_extension_api.h                                                    */
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

class AIRuntimeServer;
class AIImportOrchestrator;
class ModelCacheManager;

class AIExtensionAPI : public Object {
	GDCLASS(AIExtensionAPI, Object);

	static AIExtensionAPI *singleton;

protected:
	static void _bind_methods();

public:
	static AIExtensionAPI *get_singleton();

	bool has_runtime_server() const;
	bool has_import_orchestrator() const;
	bool has_model_cache_manager() const;
	bool is_ready() const;

	Dictionary get_runtime_stats() const;
	Dictionary get_import_status() const;
	Dictionary get_cache_status() const;
	Dictionary get_extension_status() const;
	Dictionary build_import_context(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options = Dictionary()) const;
	Dictionary orchestrate_import(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options = Dictionary()) const;
	Dictionary get_cached_annotation_status(const String &p_source_path, const String &p_importer_name, const Dictionary &p_context = Dictionary()) const;
	Dictionary get_export_artifact_whitelist() const;
	bool is_export_artifact_allowed(const String &p_category, const String &p_platform_tag = String(), const Dictionary &p_metadata = Dictionary()) const;
	Dictionary build_export_bundle_plan(const String &p_platform_tag, const Array &p_requested_artifacts = Array(), const Dictionary &p_metadata = Dictionary()) const;

	AIExtensionAPI();
	~AIExtensionAPI();
};
