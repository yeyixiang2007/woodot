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
#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/object/class_db.h"
#include "modules/woodot_ai/import/ai_asset_annotator.h"
#include "modules/woodot_ai/import/ai_extension_api.h"
#include "modules/woodot_ai/import/ai_import_orchestrator.h"
#include "modules/woodot_ai/import/ai_mesh_post_processor.h"
#include "modules/woodot_ai/import/ai_texture_enhancer.h"
#include "modules/woodot_ai/import/model_cache_manager.h"
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
static AIAssetAnnotator *woodot_ai_asset_annotator = nullptr;
static AIMeshPostProcessor *woodot_ai_mesh_post_processor = nullptr;
static AITextureEnhancer *woodot_ai_texture_enhancer = nullptr;
static ModelCacheManager *woodot_ai_model_cache_manager = nullptr;
static AIExtensionAPI *woodot_ai_extension_api = nullptr;
static AIImportOrchestrator *woodot_ai_import_orchestrator = nullptr;
static NodeGraphIntentParser *woodot_ai_node_graph_intent_parser = nullptr;
static UndoRedoBridge *woodot_ai_undo_redo_bridge = nullptr;
static EditorAIService *woodot_ai_editor_service = nullptr;
#endif

static void woodot_ai_remove_engine_singleton(const char *p_name) {
	Engine *engine = Engine::get_singleton();
	if (engine == nullptr) {
		return;
	}

	if (engine->has_singleton(p_name)) {
		engine->remove_singleton(p_name);
	}
}

template <typename T>
static void woodot_ai_free_singleton(const char *p_name, T *&p_ptr) {
	woodot_ai_remove_engine_singleton(p_name);
	if (p_ptr != nullptr) {
		memdelete(p_ptr);
		p_ptr = nullptr;
	}
}

void initialize_woodot_ai_module(ModuleInitializationLevel p_level) {
	switch (p_level) {
		case MODULE_INITIALIZATION_LEVEL_CORE:
			AIImportOrchestrator::register_project_settings();
			ModelCacheManager::register_project_settings();
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
			ERR_FAIL_COND_MSG(woodot_ai_runtime_server != nullptr, "AIRuntimeServer already initialized.");
			ERR_FAIL_COND_MSG(Engine::get_singleton()->has_singleton("AIRuntimeServer"), "AIRuntimeServer singleton already registered.");
			woodot_ai_runtime_server = memnew(AIRuntimeServer);
			Engine::get_singleton()->add_singleton(Engine::Singleton("AIRuntimeServer", woodot_ai_runtime_server, "AIRuntimeServer"));
		} break;
		case MODULE_INITIALIZATION_LEVEL_SCENE:
			break;
		case MODULE_INITIALIZATION_LEVEL_EDITOR:
#ifdef TOOLS_ENABLED
			ERR_FAIL_COND_MSG(woodot_ai_runtime_server == nullptr, "AIRuntimeServer must be initialized before woodot_ai editor singletons.");
			GDREGISTER_CLASS(EditorAIPreviewDiff);
			GDREGISTER_CLASS(EditorContextCollector);
			GDREGISTER_CLASS(GDScriptRepairEngine);
			GDREGISTER_CLASS(AIAssetAnnotator);
			GDREGISTER_CLASS(AIMeshPostProcessor);
			GDREGISTER_CLASS(AITextureEnhancer);
			GDREGISTER_CLASS(ModelCacheManager);
			GDREGISTER_CLASS(AIExtensionAPI);
			GDREGISTER_CLASS(AIImportOrchestrator);
			GDREGISTER_CLASS(NodeGraphIntentParser);
			GDREGISTER_CLASS(UndoRedoBridge);
			GDREGISTER_CLASS(EditorAIService);
			ERR_FAIL_COND_MSG(woodot_ai_preview_diff != nullptr, "EditorAIPreviewDiff already initialized.");
			ERR_FAIL_COND_MSG(Engine::get_singleton()->has_singleton("EditorAIPreviewDiff"), "EditorAIPreviewDiff singleton already registered.");
			woodot_ai_preview_diff = memnew(EditorAIPreviewDiff);
			{
				Engine::Singleton singleton("EditorAIPreviewDiff", woodot_ai_preview_diff, "EditorAIPreviewDiff");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
			ERR_FAIL_COND_MSG(woodot_ai_editor_context_collector != nullptr, "EditorContextCollector already initialized.");
			ERR_FAIL_COND_MSG(Engine::get_singleton()->has_singleton("EditorContextCollector"), "EditorContextCollector singleton already registered.");
			woodot_ai_editor_context_collector = memnew(EditorContextCollector);
			{
				Engine::Singleton singleton("EditorContextCollector", woodot_ai_editor_context_collector, "EditorContextCollector");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
			ERR_FAIL_COND_MSG(woodot_ai_gdscript_repair_engine != nullptr, "GDScriptRepairEngine already initialized.");
			ERR_FAIL_COND_MSG(Engine::get_singleton()->has_singleton("GDScriptRepairEngine"), "GDScriptRepairEngine singleton already registered.");
			woodot_ai_gdscript_repair_engine = memnew(GDScriptRepairEngine);
			{
				Engine::Singleton singleton("GDScriptRepairEngine", woodot_ai_gdscript_repair_engine, "GDScriptRepairEngine");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
			ERR_FAIL_COND_MSG(woodot_ai_asset_annotator != nullptr, "AIAssetAnnotator already initialized.");
			ERR_FAIL_COND_MSG(Engine::get_singleton()->has_singleton("AIAssetAnnotator"), "AIAssetAnnotator singleton already registered.");
			woodot_ai_asset_annotator = memnew(AIAssetAnnotator);
			{
				Engine::Singleton singleton("AIAssetAnnotator", woodot_ai_asset_annotator, "AIAssetAnnotator");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
			ERR_FAIL_COND_MSG(woodot_ai_mesh_post_processor != nullptr, "AIMeshPostProcessor already initialized.");
			ERR_FAIL_COND_MSG(Engine::get_singleton()->has_singleton("AIMeshPostProcessor"), "AIMeshPostProcessor singleton already registered.");
			woodot_ai_mesh_post_processor = memnew(AIMeshPostProcessor);
			{
				Engine::Singleton singleton("AIMeshPostProcessor", woodot_ai_mesh_post_processor, "AIMeshPostProcessor");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
			ERR_FAIL_COND_MSG(woodot_ai_texture_enhancer != nullptr, "AITextureEnhancer already initialized.");
			ERR_FAIL_COND_MSG(Engine::get_singleton()->has_singleton("AITextureEnhancer"), "AITextureEnhancer singleton already registered.");
			woodot_ai_texture_enhancer = memnew(AITextureEnhancer);
			{
				Engine::Singleton singleton("AITextureEnhancer", woodot_ai_texture_enhancer, "AITextureEnhancer");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
			ERR_FAIL_COND_MSG(woodot_ai_model_cache_manager != nullptr, "ModelCacheManager already initialized.");
			ERR_FAIL_COND_MSG(Engine::get_singleton()->has_singleton("ModelCacheManager"), "ModelCacheManager singleton already registered.");
			woodot_ai_model_cache_manager = memnew(ModelCacheManager);
			{
				Engine::Singleton singleton("ModelCacheManager", woodot_ai_model_cache_manager, "ModelCacheManager");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
			ERR_FAIL_COND_MSG(woodot_ai_extension_api != nullptr, "AIExtensionAPI already initialized.");
			ERR_FAIL_COND_MSG(Engine::get_singleton()->has_singleton("AIExtensionAPI"), "AIExtensionAPI singleton already registered.");
			woodot_ai_extension_api = memnew(AIExtensionAPI);
			{
				Engine::Singleton singleton("AIExtensionAPI", woodot_ai_extension_api, "AIExtensionAPI");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
			ERR_FAIL_COND_MSG(woodot_ai_import_orchestrator != nullptr, "AIImportOrchestrator already initialized.");
			ERR_FAIL_COND_MSG(Engine::get_singleton()->has_singleton("AIImportOrchestrator"), "AIImportOrchestrator singleton already registered.");
			woodot_ai_import_orchestrator = memnew(AIImportOrchestrator);
			{
				Engine::Singleton singleton("AIImportOrchestrator", woodot_ai_import_orchestrator, "AIImportOrchestrator");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
			ERR_FAIL_COND_MSG(woodot_ai_node_graph_intent_parser != nullptr, "NodeGraphIntentParser already initialized.");
			ERR_FAIL_COND_MSG(Engine::get_singleton()->has_singleton("NodeGraphIntentParser"), "NodeGraphIntentParser singleton already registered.");
			woodot_ai_node_graph_intent_parser = memnew(NodeGraphIntentParser);
			{
				Engine::Singleton singleton("NodeGraphIntentParser", woodot_ai_node_graph_intent_parser, "NodeGraphIntentParser");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
			ERR_FAIL_COND_MSG(woodot_ai_undo_redo_bridge != nullptr, "UndoRedoBridge already initialized.");
			ERR_FAIL_COND_MSG(Engine::get_singleton()->has_singleton("UndoRedoBridge"), "UndoRedoBridge singleton already registered.");
			woodot_ai_undo_redo_bridge = memnew(UndoRedoBridge);
			{
				Engine::Singleton singleton("UndoRedoBridge", woodot_ai_undo_redo_bridge, "UndoRedoBridge");
				singleton.editor_only = true;
				Engine::get_singleton()->add_singleton(singleton);
			}
			ERR_FAIL_COND_MSG(woodot_ai_editor_service != nullptr, "EditorAIService already initialized.");
			ERR_FAIL_COND_MSG(Engine::get_singleton()->has_singleton("EditorAIService"), "EditorAIService singleton already registered.");
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
			woodot_ai_free_singleton("AIRuntimeServer", woodot_ai_runtime_server);
			break;
		case MODULE_INITIALIZATION_LEVEL_SCENE:
			break;
		case MODULE_INITIALIZATION_LEVEL_EDITOR:
#ifdef TOOLS_ENABLED
			// Destroy in reverse order of initialization to reduce dependency risks.
			woodot_ai_free_singleton("EditorAIService", woodot_ai_editor_service);
			woodot_ai_free_singleton("UndoRedoBridge", woodot_ai_undo_redo_bridge);
			woodot_ai_free_singleton("NodeGraphIntentParser", woodot_ai_node_graph_intent_parser);
			woodot_ai_free_singleton("AIImportOrchestrator", woodot_ai_import_orchestrator);
			woodot_ai_free_singleton("AIExtensionAPI", woodot_ai_extension_api);
			woodot_ai_free_singleton("ModelCacheManager", woodot_ai_model_cache_manager);
			woodot_ai_free_singleton("AITextureEnhancer", woodot_ai_texture_enhancer);
			woodot_ai_free_singleton("AIMeshPostProcessor", woodot_ai_mesh_post_processor);
			woodot_ai_free_singleton("AIAssetAnnotator", woodot_ai_asset_annotator);
			woodot_ai_free_singleton("GDScriptRepairEngine", woodot_ai_gdscript_repair_engine);
			woodot_ai_free_singleton("EditorContextCollector", woodot_ai_editor_context_collector);
			woodot_ai_free_singleton("EditorAIPreviewDiff", woodot_ai_preview_diff);
#endif
			break;
	}
}
