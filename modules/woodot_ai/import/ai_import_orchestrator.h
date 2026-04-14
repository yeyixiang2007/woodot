/**************************************************************************/
/*  ai_import_orchestrator.h                                              */
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
#include "core/variant/type_info.h"
#include "modules/woodot_ai/runtime/ai_requests.h"

class AIModelResource;
class AIRuntimeServer;
class EditorAIService;

class AIImportOrchestrator : public Object {
	GDCLASS(AIImportOrchestrator, Object);

public:
	enum PassType {
		PASS_ASSET_ANNOTATION = 0,
		PASS_MESH_POSTPROCESS,
		PASS_TEXTURE_ENHANCEMENT,
	};

private:
	static AIImportOrchestrator *singleton;

	bool enabled = false;
	bool fail_open = true;
	bool asset_annotation_enabled = true;
	bool mesh_postprocess_enabled = false;
	bool texture_enhancement_enabled = false;

	Ref<AIModelResource> default_model;
	RID default_model_rid;

	uint64_t inspected_imports = 0;
	uint64_t ai_candidate_imports = 0;
	uint64_t prepared_requests = 0;
	uint64_t fallback_imports = 0;

	AIRuntimeServer *_get_runtime_server() const;
	EditorAIService *_get_editor_ai_service() const;
	String _get_pass_name(PassType p_pass_type) const;
	Array _get_enabled_pass_names() const;

protected:
	static void _bind_methods();

public:
	static AIImportOrchestrator *get_singleton();

	void set_enabled(bool p_enabled);
	bool is_enabled() const;

	void set_fail_open(bool p_fail_open);
	bool is_fail_open() const;

	void set_asset_annotation_enabled(bool p_enabled);
	bool is_asset_annotation_enabled() const;

	void set_mesh_postprocess_enabled(bool p_enabled);
	bool is_mesh_postprocess_enabled() const;

	void set_texture_enhancement_enabled(bool p_enabled);
	bool is_texture_enhancement_enabled() const;

	void set_pass_enabled(PassType p_pass_type, bool p_enabled);
	bool is_pass_enabled(PassType p_pass_type) const;

	void set_default_model(const Ref<AIModelResource> &p_model);
	Ref<AIModelResource> get_default_model() const;
	bool has_loaded_default_model() const;
	Error ensure_default_model_loaded();
	void unload_default_model();

	bool has_runtime_server() const;
	bool has_editor_ai_service() const;
	bool is_ready() const;

	Dictionary build_import_context(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options = Dictionary()) const;
	Ref<AICompletionRequest> create_annotation_request(const String &p_source_path, const String &p_importer_name, const String &p_prompt, const RID &p_model_rid = RID(), const Dictionary &p_options = Dictionary());
	Dictionary orchestrate_import(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options = Dictionary());
	Dictionary get_orchestrator_status() const;

	AIImportOrchestrator();
	~AIImportOrchestrator();
};

VARIANT_ENUM_CAST(AIImportOrchestrator::PassType);
