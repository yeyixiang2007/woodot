/**************************************************************************/
/*  editor_ai_service.h                                                   */
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
#include "core/variant/type_info.h"

class AIModelResource;
class EditorContextCollector;
class EditorAIPreviewDiff;
class GDScriptRepairEngine;
class NodeGraphIntentParser;
class UndoRedoBridge;
class AIRuntimeServer;
class GDScriptRepairPatch;
class SceneSynthesisPlan;

class EditorAIService : public Object {
	GDCLASS(EditorAIService, Object);

public:
	enum RequestKind {
		REQUEST_KIND_SCENE_SYNTHESIS = 0,
		REQUEST_KIND_SCRIPT_REPAIR,
	};

private:
	static EditorAIService *singleton;

	Ref<AIModelResource> default_model;
	RID default_model_rid;
	Ref<AITaskHandle> last_scene_synthesis_task;
	Ref<AITaskHandle> last_script_repair_task;
	uint64_t submitted_scene_requests = 0;
	uint64_t submitted_script_repairs = 0;

	void _record_task(RequestKind p_kind, const Ref<AITaskHandle> &p_task_handle);
	AIRuntimeServer *_get_runtime_server() const;
	Ref<AITaskHandle> _fail_task(const String &p_message) const;

protected:
	static void _bind_methods();

public:
	static EditorAIService *get_singleton();

	void set_default_model(const Ref<AIModelResource> &p_model);
	Ref<AIModelResource> get_default_model() const;
	bool has_runtime_server() const;
	bool is_ready() const;
	bool has_context_collector() const;
	bool has_gdscript_repair_engine() const;
	bool has_node_graph_intent_parser() const;
	bool has_preview_diff() const;
	bool has_undo_redo_bridge() const;
	bool has_loaded_default_model() const;
	Error ensure_default_model_loaded();
	void unload_default_model();

	Ref<AITaskHandle> request_scene_synthesis(const String &p_prompt, const Dictionary &p_context = Dictionary());
	Ref<AITaskHandle> request_script_repair(const String &p_script_path, const String &p_diagnostics, const String &p_code_snippet = String(), const Dictionary &p_context = Dictionary());
	void cancel_task(const Ref<AITaskHandle> &p_task_handle);
	void poll();

	Ref<AITaskHandle> get_last_scene_synthesis_task() const;
	Ref<AITaskHandle> get_last_script_repair_task() const;
	Dictionary collect_scene_request_context(const Dictionary &p_overrides = Dictionary()) const;
	Dictionary collect_script_repair_context(const String &p_script_path, const String &p_diagnostics, const String &p_code_snippet = String(), const Dictionary &p_overrides = Dictionary()) const;
	Dictionary validate_gdscript_patch_ir(const String &p_source_ir) const;
	Ref<GDScriptRepairPatch> parse_gdscript_patch_ir(const String &p_source_ir, const String &p_script_path = String(), const String &p_diagnostic_message = String(), const Dictionary &p_metadata = Dictionary()) const;
	Dictionary validate_scene_plan_ir(const String &p_source_ir) const;
	Ref<SceneSynthesisPlan> parse_scene_plan_ir(const String &p_source_ir, const String &p_prompt = String(), const Dictionary &p_metadata = Dictionary()) const;
	Dictionary build_scene_plan_preview(const Ref<SceneSynthesisPlan> &p_plan) const;
	Dictionary build_gdscript_patch_preview(const Ref<GDScriptRepairPatch> &p_patch) const;
	Dictionary can_apply_scene_plan(const Ref<SceneSynthesisPlan> &p_plan) const;
	Dictionary apply_scene_plan(const Ref<SceneSynthesisPlan> &p_plan);
	Dictionary can_apply_gdscript_patch(const Ref<GDScriptRepairPatch> &p_patch) const;
	Dictionary apply_gdscript_patch(const Ref<GDScriptRepairPatch> &p_patch);
	Dictionary resolve_scene_synthesis_task(const Ref<AITaskHandle> &p_task_handle, const String &p_prompt = String(), const Dictionary &p_metadata = Dictionary()) const;
	Dictionary resolve_script_repair_task(const Ref<AITaskHandle> &p_task_handle, const String &p_script_path = String(), const String &p_diagnostic_message = String(), const Dictionary &p_metadata = Dictionary()) const;
	Dictionary get_service_status() const;

	EditorAIService();
	~EditorAIService();
};

VARIANT_ENUM_CAST(EditorAIService::RequestKind);
