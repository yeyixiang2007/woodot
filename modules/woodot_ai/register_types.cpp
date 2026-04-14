/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "register_types.h"

#include "core/config/engine.h"
#include "core/object/class_db.h"
#include "modules/woodot_ai/resources/ai_request_resources.h"
#include "modules/woodot_ai/resources/ai_model_resource.h"
#include "modules/woodot_ai/resources/ai_tensor_resource.h"
#include "modules/woodot_ai/resources/gdscript_repair_patch.h"
#include "modules/woodot_ai/resources/scene_synthesis_plan.h"
#include "modules/woodot_ai/runtime/ai_requests.h"
#include "modules/woodot_ai/runtime/ai_task_handle.h"
#include "modules/woodot_ai/runtime/ai_runtime_server.h"

#ifdef TOOLS_ENABLED
#include "modules/woodot_ai/editor/editor_ai_preview_diff.h"
#include "modules/woodot_ai/editor/editor_context_collector.h"
#include "modules/woodot_ai/editor/editor_ai_service.h"
#include "modules/woodot_ai/editor/gdscript_repair_engine.h"
#include "modules/woodot_ai/editor/node_graph_intent_parser.h"
#include "modules/woodot_ai/editor/undo_redo_bridge.h"
#endif

#ifdef TESTS_ENABLED
#include "modules/woodot_ai/tests/test_ai_runtime.h"
#endif

static AIRuntimeServer *woodot_ai_runtime_server = nullptr;
#ifdef TOOLS_ENABLED
static EditorAIPreviewDiff *woodot_ai_preview_diff = nullptr;
static EditorContextCollector *woodot_ai_editor_context_collector = nullptr;
static GDScriptRepairEngine *woodot_ai_gdscript_repair_engine = nullptr;
static NodeGraphIntentParser *woodot_ai_node_graph_intent_parser = nullptr;
static UndoRedoBridge *woodot_ai_undo_redo_bridge = nullptr;
static EditorAIService *woodot_ai_editor_service = nullptr;
#endif

void initialize_woodot_ai_module(ModuleInitializationLevel p_level) {
	switch (p_level) {
		case MODULE_INITIALIZATION_LEVEL_CORE:
			GDREGISTER_CLASS(AIModelResource);
			GDREGISTER_CLASS(AITensorResource);
			GDREGISTER_CLASS(AICompletionRequestResource);
			GDREGISTER_CLASS(AIEmbeddingRequestResource);
			GDREGISTER_CLASS(SceneSynthesisPlan);
			GDREGISTER_CLASS(GDScriptRepairPatch);
			GDREGISTER_CLASS(AICompletionRequest);
			GDREGISTER_CLASS(AIEmbeddingRequest);
			GDREGISTER_CLASS(AITaskHandle);
			GDREGISTER_CLASS(AIRuntimeServer);
			break;
		case MODULE_INITIALIZATION_LEVEL_SERVERS: {
			woodot_ai_runtime_server = memnew(AIRuntimeServer);
			Engine::get_singleton()->add_singleton(Engine::Singleton("AIRuntimeServer", woodot_ai_runtime_server, "AIRuntimeServer"));
		} break;
		case MODULE_INITIALIZATION_LEVEL_SCENE:
			break;
		case MODULE_INITIALIZATION_LEVEL_EDITOR:
#ifdef TOOLS_ENABLED
			GDREGISTER_CLASS(EditorAIPreviewDiff);
			GDREGISTER_CLASS(EditorContextCollector);
			GDREGISTER_CLASS(GDScriptRepairEngine);
			GDREGISTER_CLASS(NodeGraphIntentParser);
			GDREGISTER_CLASS(UndoRedoBridge);
			GDREGISTER_CLASS(EditorAIService);
			woodot_ai_preview_diff = memnew(EditorAIPreviewDiff);
			{
				Engine::Singleton singleton("EditorAIPreviewDiff", woodot_ai_preview_diff, "EditorAIPreviewDiff");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
			woodot_ai_editor_context_collector = memnew(EditorContextCollector);
			{
				Engine::Singleton singleton("EditorContextCollector", woodot_ai_editor_context_collector, "EditorContextCollector");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
			woodot_ai_gdscript_repair_engine = memnew(GDScriptRepairEngine);
			{
				Engine::Singleton singleton("GDScriptRepairEngine", woodot_ai_gdscript_repair_engine, "GDScriptRepairEngine");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
			woodot_ai_node_graph_intent_parser = memnew(NodeGraphIntentParser);
			{
				Engine::Singleton singleton("NodeGraphIntentParser", woodot_ai_node_graph_intent_parser, "NodeGraphIntentParser");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
			woodot_ai_undo_redo_bridge = memnew(UndoRedoBridge);
			{
				Engine::Singleton singleton("UndoRedoBridge", woodot_ai_undo_redo_bridge, "UndoRedoBridge");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
			woodot_ai_editor_service = memnew(EditorAIService);
			{
				Engine::Singleton singleton("EditorAIService", woodot_ai_editor_service, "EditorAIService");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
#endif
			break;
	}
}

void uninitialize_woodot_ai_module(ModuleInitializationLevel p_level) {
	switch (p_level) {
		case MODULE_INITIALIZATION_LEVEL_CORE:
			break;
		case MODULE_INITIALIZATION_LEVEL_SERVERS:
			if (woodot_ai_runtime_server != nullptr) {
				if (Engine::get_singleton()->has_singleton("AIRuntimeServer")) {
					Engine::get_singleton()->remove_singleton("AIRuntimeServer");
				}
				memdelete(woodot_ai_runtime_server);
				woodot_ai_runtime_server = nullptr;
			}
			break;
		case MODULE_INITIALIZATION_LEVEL_SCENE:
			break;
		case MODULE_INITIALIZATION_LEVEL_EDITOR:
#ifdef TOOLS_ENABLED
			if (woodot_ai_editor_service != nullptr) {
				if (Engine::get_singleton()->has_singleton("EditorAIService")) {
					Engine::get_singleton()->remove_singleton("EditorAIService");
				}
				memdelete(woodot_ai_editor_service);
				woodot_ai_editor_service = nullptr;
			}
			if (woodot_ai_undo_redo_bridge != nullptr) {
				if (Engine::get_singleton()->has_singleton("UndoRedoBridge")) {
					Engine::get_singleton()->remove_singleton("UndoRedoBridge");
				}
				memdelete(woodot_ai_undo_redo_bridge);
				woodot_ai_undo_redo_bridge = nullptr;
			}
			if (woodot_ai_editor_context_collector != nullptr) {
				if (Engine::get_singleton()->has_singleton("EditorContextCollector")) {
					Engine::get_singleton()->remove_singleton("EditorContextCollector");
				}
				memdelete(woodot_ai_editor_context_collector);
				woodot_ai_editor_context_collector = nullptr;
			}
			if (woodot_ai_gdscript_repair_engine != nullptr) {
				if (Engine::get_singleton()->has_singleton("GDScriptRepairEngine")) {
					Engine::get_singleton()->remove_singleton("GDScriptRepairEngine");
				}
				memdelete(woodot_ai_gdscript_repair_engine);
				woodot_ai_gdscript_repair_engine = nullptr;
			}
			if (woodot_ai_node_graph_intent_parser != nullptr) {
				if (Engine::get_singleton()->has_singleton("NodeGraphIntentParser")) {
					Engine::get_singleton()->remove_singleton("NodeGraphIntentParser");
				}
				memdelete(woodot_ai_node_graph_intent_parser);
				woodot_ai_node_graph_intent_parser = nullptr;
			}
			if (woodot_ai_preview_diff != nullptr) {
				if (Engine::get_singleton()->has_singleton("EditorAIPreviewDiff")) {
					Engine::get_singleton()->remove_singleton("EditorAIPreviewDiff");
				}
				memdelete(woodot_ai_preview_diff);
				woodot_ai_preview_diff = nullptr;
			}
#endif
			break;
	}
}
