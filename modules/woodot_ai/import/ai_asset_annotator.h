/**************************************************************************/
/*  ai_asset_annotator.h                                                  */
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
#include "modules/woodot_ai/runtime/ai_task_handle.h"

class AICompletionRequest;
class AIImportOrchestrator;
class AIRuntimeServer;

class AIAssetAnnotator : public Object {
	GDCLASS(AIAssetAnnotator, Object);

	static AIAssetAnnotator *singleton;

	uint64_t submitted_annotations = 0;
	uint64_t resolved_annotations = 0;
	uint64_t failed_annotations = 0;

	AIImportOrchestrator *_get_orchestrator() const;
	AIRuntimeServer *_get_runtime_server() const;
	static String _sanitize_keyword(const String &p_value);
	static PackedStringArray _variant_to_string_array(const Variant &p_value);
	static Array _packed_to_array(const PackedStringArray &p_values);
	Ref<AITaskHandle> _fail_task(const String &p_message) const;

protected:
	static void _bind_methods();

public:
	static AIAssetAnnotator *get_singleton();

	bool has_import_orchestrator() const;
	bool has_runtime_server() const;
	bool is_ready() const;

	String build_annotation_prompt(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options = Dictionary()) const;
	Ref<AICompletionRequest> prepare_annotation_request(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options = Dictionary()) const;
	Ref<AITaskHandle> submit_annotation(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options = Dictionary());
	Dictionary validate_annotation_output(const String &p_output_text) const;
	Dictionary parse_annotation_output(const String &p_output_text) const;
	Dictionary build_annotation_sidecar(const Dictionary &p_annotation, const String &p_source_path, const String &p_importer_name, const Dictionary &p_import_context = Dictionary()) const;
	Dictionary resolve_annotation_task(const Ref<AITaskHandle> &p_task_handle, const String &p_source_path = String(), const String &p_importer_name = String(), const Dictionary &p_import_context = Dictionary());
	Dictionary get_annotator_status() const;

	AIAssetAnnotator();
	~AIAssetAnnotator();
};
