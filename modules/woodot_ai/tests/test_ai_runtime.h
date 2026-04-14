/**************************************************************************/
/*  test_ai_runtime.h                                                     */
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

#include "../resources/ai_model_resource.h"
#include "../resources/ai_request_resources.h"
#include "../resources/ai_tensor_resource.h"
#include "../resources/gdscript_repair_patch.h"
#include "../resources/scene_synthesis_plan.h"
#include "../runtime/ai_requests.h"
#include "../runtime/ai_task_scheduler.h"

#ifdef TOOLS_ENABLED
#include "../editor/editor_ai_preview_diff.h"
#include "../editor/editor_ai_service.h"
#include "../editor/gdscript_repair_engine.h"
#include "../editor/node_graph_intent_parser.h"
#include "../editor/undo_redo_bridge.h"
#endif

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/templates/hash_map.h"
#include "core/templates/rid_owner.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

namespace TestWoodotAIRuntime {

class MockAIBackend : public AIBackend {
	struct MockModelState {
		int32_t sequence = 0;
	};

	struct MockContextState {
		RID model_rid;
		bool cancelled = false;
	};

	RID_Owner<MockModelState, true> model_owner;
	RID_Owner<MockContextState, true> context_owner;
	HashMap<uint64_t, RID> running_jobs;
	uint64_t destroy_context_count = 0;
	uint64_t cancel_job_count = 0;

public:
	virtual StringName get_backend_name() const override {
		return StringName("mock_ai");
	}

	virtual AIBackendCapabilities get_capabilities() const override {
		AIBackendCapabilities capabilities;
		capabilities.supports_completion = true;
		capabilities.supports_embedding = true;
		capabilities.supports_streaming = true;
		capabilities.supports_cancellation = true;
		capabilities.metadata["implementation_stage"] = "test";
		return capabilities;
	}

	virtual AIBackendValidationResult validate_model(const Ref<AIModelResource> &p_model) const override {
		AIBackendValidationResult result;
		if (p_model.is_null()) {
			result.code = ERR_INVALID_PARAMETER;
			result.message = "MockAIBackend requires a model resource.";
		}
		return result;
	}

	virtual AIBackendModelLoadResult load_model(const Ref<AIModelResource> &p_model) override {
		AIBackendModelLoadResult result;
		if (!validate_model(p_model).is_ok()) {
			result.code = ERR_INVALID_PARAMETER;
			result.message = "Invalid model resource.";
			return result;
		}

		MockModelState state;
		state.sequence = model_owner.get_rid_count() + 1;
		result.model_handle.rid = model_owner.make_rid(state);
		result.model_handle.backend_name = get_backend_name();
		result.details["sequence"] = state.sequence;
		return result;
	}

	virtual void unload_model(const AIBackendModelHandle &p_model_handle) override {
		if (p_model_handle.rid.is_valid()) {
			model_owner.free(p_model_handle.rid);
		}
	}

	virtual AIBackendContextCreateResult create_context(const AIBackendModelHandle &p_model_handle) override {
		AIBackendContextCreateResult result;
		if (!p_model_handle.is_valid()) {
			result.code = ERR_INVALID_PARAMETER;
			result.message = "Missing model handle.";
			return result;
		}

		MockContextState state;
		state.model_rid = p_model_handle.rid;
		result.context_handle.rid = context_owner.make_rid(state);
		result.context_handle.model_rid = p_model_handle.rid;
		result.context_handle.backend_name = get_backend_name();
		return result;
	}

	virtual void destroy_context(const AIBackendContextHandle &p_context_handle) override {
		if (p_context_handle.rid.is_valid()) {
			context_owner.free(p_context_handle.rid);
			destroy_context_count++;
		}
	}

	virtual AIBackendResult run_job(const AIComputeJob &p_job) override {
		AIBackendResult result;
		result.metadata["backend"] = get_backend_name();
		result.metadata["job_id"] = static_cast<int64_t>(p_job.job_id);

		MockContextState *context = context_owner.get_or_null(p_job.context_handle.rid);
		if (context == nullptr) {
			result.code = ERR_DOES_NOT_EXIST;
			result.message = "Context not found.";
			return result;
		}

		running_jobs.insert(p_job.job_id, p_job.context_handle.rid);

		const Dictionary metadata = p_job.metadata;
		if (metadata.has("exec_time_us")) {
			result.exec_time_us = static_cast<uint64_t>(int64_t(metadata["exec_time_us"]));
		}
		if (metadata.has("queue_wait_us")) {
			result.queue_wait_us = static_cast<uint64_t>(int64_t(metadata["queue_wait_us"]));
		}
		if (metadata.has("force_error")) {
			result.code = Error(int64_t(metadata["force_error"]));
			result.message = "Mock backend forced an error result.";
		} else if (metadata.has("force_cancelled") && bool(metadata["force_cancelled"])) {
			result.was_cancelled = true;
			result.message = "Mock backend cancelled the job.";
		} else {
			result.code = OK;
			result.final_text = p_job.prompt.is_empty() ? String("ok") : p_job.prompt;
		}

		running_jobs.erase(p_job.job_id);
		return result;
	}

	virtual bool cancel_job(uint64_t p_job_id) override {
		cancel_job_count++;
		HashMap<uint64_t, RID>::Iterator job = running_jobs.find(p_job_id);
		if (!job) {
			return false;
		}

		MockContextState *context = context_owner.get_or_null(job->value);
		if (context == nullptr) {
			return false;
		}

		context->cancelled = true;
		return true;
	}

	virtual Dictionary get_runtime_stats() const override {
		Dictionary stats;
		stats["destroy_context_count"] = static_cast<int64_t>(destroy_context_count);
		stats["cancel_job_count"] = static_cast<int64_t>(cancel_job_count);
		stats["loaded_models"] = static_cast<int64_t>(model_owner.get_rid_count());
		stats["active_contexts"] = static_cast<int64_t>(context_owner.get_rid_count());
		return stats;
	}
};

static Ref<AICompletionRequest> make_completion_request(const RID &p_model_rid, const String &p_prompt, int32_t p_timeout_ms = 0, const Dictionary &p_metadata = Dictionary()) {
	Ref<AICompletionRequest> request;
	request.instantiate();
	request->set_model_rid(p_model_rid);
	request->set_prompt(p_prompt);
	request->set_timeout_ms(p_timeout_ms);
	request->set_metadata(p_metadata);
	return request;
}

template <class T>
static Ref<T> save_and_load_resource_roundtrip(const Ref<T> &p_resource, const String &p_extension) {
	const String save_path = TestUtils::get_temp_path(vformat("woodot_ai_%s_roundtrip.%s", T::get_class_static(), p_extension));
	CHECK_EQ(ResourceSaver::save(p_resource, save_path), OK);

	Ref<T> loaded = ResourceLoader::load(save_path);
	CHECK(loaded.is_valid());
	return loaded;
}

static bool property_has_editor_usage(const Object *p_object, const StringName &p_name) {
	List<PropertyInfo> properties;
	p_object->get_property_list(&properties);
	for (const List<PropertyInfo>::Element *property = properties.front(); property != nullptr; property = property->next()) {
		if (property->get().name == p_name) {
			return (property->get().usage & PROPERTY_USAGE_EDITOR) != 0;
		}
	}
	return false;
}

#ifdef TOOLS_ENABLED
template <class T>
class ScopedEditorSingleton {
	T *instance = nullptr;
	bool owned = false;

public:
	ScopedEditorSingleton(T *p_existing) {
		if (p_existing != nullptr) {
			instance = p_existing;
			return;
		}
		instance = memnew(T);
		owned = true;
	}

	~ScopedEditorSingleton() {
		if (owned) {
			memdelete(instance);
		}
	}

	T *operator->() const {
		return instance;
	}

	T *get() const {
		return instance;
	}
};

static Ref<AITaskHandle> make_completed_text_task(const String &p_text, const Dictionary &p_metadata = Dictionary()) {
	Ref<AITaskHandle> handle;
	handle.instantiate();
	handle->mark_running();
	AIBackendResult result;
	result.code = OK;
	result.final_text = p_text;
	result.metadata = p_metadata;
	CHECK(handle->complete(result));
	return handle;
}
#endif

TEST_CASE("[WoodotAI] Queued task cancellation drains through mailbox") {
	MockAIBackend backend;
	AITaskScheduler scheduler;
	scheduler.set_auto_process_queue_enabled(false);

	Ref<AIModelResource> model;
	model.instantiate();
	model->set_backend_type(backend.get_backend_name());

	const AIBackendModelLoadResult model_load = backend.load_model(model);
	REQUIRE(model_load.is_ok());

	Ref<AITaskHandle> handle = scheduler.submit_completion(&backend, model_load.model_handle, RID(), make_completion_request(RID(), "queued cancel"));
	REQUIRE(handle.is_valid());
	CHECK(handle->get_status() == AITaskHandle::STATUS_QUEUED);

	scheduler.cancel_task(handle);
	CHECK(handle->get_status() == AITaskHandle::STATUS_QUEUED);

	CHECK_EQ(scheduler.poll_completed(), 1);
	CHECK(handle->get_status() == AITaskHandle::STATUS_CANCELLED);
	CHECK(handle->get_cancel_reason() == AITaskHandle::CANCEL_REASON_USER_REQUEST);

	const Dictionary backend_stats = backend.get_runtime_stats();
	CHECK(int64_t(backend_stats["destroy_context_count"]) == 1);
}

TEST_CASE("[WoodotAI] Timeout results map to timeout cancellation") {
	MockAIBackend backend;
	AITaskScheduler scheduler;

	Ref<AIModelResource> model;
	model.instantiate();
	model->set_backend_type(backend.get_backend_name());

	const AIBackendModelLoadResult model_load = backend.load_model(model);
	REQUIRE(model_load.is_ok());

	Dictionary metadata;
	metadata["exec_time_us"] = int64_t(5000);
	Ref<AITaskHandle> handle = scheduler.submit_completion(&backend, model_load.model_handle, RID(), make_completion_request(RID(), "timeout", 1, metadata));
	REQUIRE(handle.is_valid());

	CHECK_EQ(scheduler.poll_completed(), 1);
	CHECK(handle->get_status() == AITaskHandle::STATUS_CANCELLED);
	CHECK(handle->get_cancel_reason() == AITaskHandle::CANCEL_REASON_TIMEOUT);
	CHECK(handle->get_error_code() == ERR_SKIP);
	CHECK(handle->get_error_message().contains("timeout"));

	const Dictionary stats = scheduler.get_stats();
	CHECK(int64_t(stats["timeout_jobs"]) == 1);
}

TEST_CASE("[WoodotAI] Synthetic scheduler soak keeps bookkeeping stable") {
	MockAIBackend backend;
	AITaskScheduler scheduler;

	Ref<AIModelResource> model;
	model.instantiate();
	model->set_backend_type(backend.get_backend_name());

	const AIBackendModelLoadResult model_load = backend.load_model(model);
	REQUIRE(model_load.is_ok());

	const int64_t task_count = 256;
	for (int64_t i = 0; i < task_count; i++) {
		Dictionary metadata;
		metadata["exec_time_us"] = int64_t(100 + (i % 7));
		Ref<AITaskHandle> handle = scheduler.submit_completion(&backend, model_load.model_handle, RID(), make_completion_request(RID(), vformat("task_%d", i), 0, metadata));
		REQUIRE(handle.is_valid());
	}

	CHECK_EQ(scheduler.poll_completed(), task_count);

	const Dictionary stats = scheduler.get_stats();
	CHECK(int64_t(stats["submitted_jobs"]) == task_count);
	CHECK(int64_t(stats["finished_jobs"]) == task_count);
	CHECK(int64_t(stats["running_jobs"]) == 0);
	CHECK(int64_t(stats["failed_jobs"]) == 0);
	CHECK(int64_t(stats["cancelled_jobs"]) == 0);

	const Dictionary profiling = stats["profiling"];
	CHECK(int64_t(profiling["completed_jobs"]) == task_count);
	CHECK(int64_t(profiling["mailbox_drained_updates"]) == task_count);
}

TEST_CASE("[WoodotAI] Resource serialization roundtrip preserves woodot AI resources") {
	SUBCASE("AIModelResource") {
		Ref<AIModelResource> resource;
		resource.instantiate();
		resource->set_model_path("res://models/demo.gguf");
		resource->set_backend_type(StringName("llama"));
		resource->set_context_size(8192);
		resource->set_n_threads(12);
		resource->set_n_gpu_layers(16);
		resource->set_quantization("Q4_K_M");
		resource->set_chat_template("{{ prompt }}");
		resource->set_rope_scaling(1.25f);
		resource->set_system_prompt_template("You are a builder.");
		PackedStringArray capability_tags;
		capability_tags.push_back("chat");
		capability_tags.push_back("tool");
		resource->set_capability_tags(capability_tags);
		Dictionary extra_options;
		extra_options["mirostat"] = true;
		resource->set_extra_options(extra_options);

		for (const String extension : { String("tres"), String("res") }) {
			const Ref<AIModelResource> loaded = save_and_load_resource_roundtrip(resource, extension);
			CHECK_EQ(loaded->get_model_path(), resource->get_model_path());
			CHECK_EQ(loaded->get_backend_type(), resource->get_backend_type());
			CHECK_EQ(loaded->get_context_size(), resource->get_context_size());
			CHECK_EQ(loaded->get_n_threads(), resource->get_n_threads());
			CHECK_EQ(loaded->get_n_gpu_layers(), resource->get_n_gpu_layers());
			CHECK_EQ(loaded->get_quantization(), resource->get_quantization());
			CHECK_EQ(loaded->get_chat_template(), resource->get_chat_template());
			CHECK_EQ(loaded->get_rope_scaling(), doctest::Approx(resource->get_rope_scaling()));
			CHECK_EQ(loaded->get_system_prompt_template(), resource->get_system_prompt_template());
			CHECK(loaded->get_capability_tags() == resource->get_capability_tags());
			CHECK(loaded->get_extra_options() == resource->get_extra_options());
			CHECK_EQ(loaded->get_parameter_fingerprint(), resource->get_parameter_fingerprint());
		}
	}

	SUBCASE("AITensorResource") {
		Ref<AITensorResource> resource;
		resource.instantiate();
		PackedInt32Array shape;
		shape.push_back(2);
		shape.push_back(3);
		resource->set_shape(shape);
		resource->set_dtype(StringName("float32"));
		resource->set_storage_type(AITensorResource::STORAGE_TYPE_CPU_MIRROR);
		PackedFloat32Array cpu_values;
		cpu_values.push_back(0.1f);
		cpu_values.push_back(0.2f);
		cpu_values.push_back(0.3f);
		cpu_values.push_back(0.4f);
		cpu_values.push_back(0.5f);
		cpu_values.push_back(0.6f);
		resource->set_cpu_data(cpu_values);
		Dictionary metadata;
		metadata["source"] = "test";
		resource->set_metadata(metadata);

		for (const String extension : { String("tres"), String("res") }) {
			const Ref<AITensorResource> loaded = save_and_load_resource_roundtrip(resource, extension);
			CHECK(loaded->get_shape() == resource->get_shape());
			CHECK_EQ(loaded->get_dtype(), resource->get_dtype());
			CHECK_EQ(loaded->get_storage_type(), resource->get_storage_type());
			CHECK(loaded->get_cpu_data() == resource->get_cpu_data());
			CHECK(loaded->get_metadata() == resource->get_metadata());
		}
	}

	SUBCASE("AICompletionRequestResource") {
		Ref<AICompletionRequestResource> resource;
		resource.instantiate();
		resource->set_prompt("hello");
		resource->set_max_tokens(512);
		resource->set_temperature(0.3f);
		resource->set_top_p(0.8f);
		resource->set_top_k(24);
		resource->set_stream(true);
		resource->set_timeout_ms(1500);
		resource->set_priority(2);
		resource->set_caller_tag("editor");
		Dictionary metadata;
		metadata["mode"] = "test";
		resource->set_metadata(metadata);

		const Ref<AICompletionRequestResource> loaded = save_and_load_resource_roundtrip(resource, "tres");
		CHECK_EQ(loaded->get_prompt(), resource->get_prompt());
		CHECK_EQ(loaded->get_max_tokens(), resource->get_max_tokens());
		CHECK_EQ(loaded->get_temperature(), doctest::Approx(resource->get_temperature()));
		CHECK_EQ(loaded->get_top_p(), doctest::Approx(resource->get_top_p()));
		CHECK_EQ(loaded->get_top_k(), resource->get_top_k());
		CHECK_EQ(loaded->is_streaming(), resource->is_streaming());
		CHECK_EQ(loaded->get_timeout_ms(), resource->get_timeout_ms());
		CHECK_EQ(loaded->get_priority(), resource->get_priority());
		CHECK_EQ(loaded->get_caller_tag(), resource->get_caller_tag());
		CHECK(loaded->get_metadata() == resource->get_metadata());
	}

	SUBCASE("AIEmbeddingRequestResource") {
		Ref<AIEmbeddingRequestResource> resource;
		resource.instantiate();
		PackedStringArray inputs;
		inputs.push_back("alpha");
		inputs.push_back("beta");
		resource->set_inputs(inputs);
		resource->set_normalize(true);
		resource->set_timeout_ms(2200);
		resource->set_priority(4);
		resource->set_caller_tag("batch");
		Dictionary metadata;
		metadata["tenant"] = "ci";
		resource->set_metadata(metadata);

		const Ref<AIEmbeddingRequestResource> loaded = save_and_load_resource_roundtrip(resource, "tres");
		CHECK(loaded->get_inputs() == resource->get_inputs());
		CHECK_EQ(loaded->is_normalized(), resource->is_normalized());
		CHECK_EQ(loaded->get_timeout_ms(), resource->get_timeout_ms());
		CHECK_EQ(loaded->get_priority(), resource->get_priority());
		CHECK_EQ(loaded->get_caller_tag(), resource->get_caller_tag());
		CHECK(loaded->get_metadata() == resource->get_metadata());
	}

	SUBCASE("SceneSynthesisPlan") {
		Ref<SceneSynthesisPlan> resource;
		resource.instantiate();
		resource->set_prompt("build a room");
		resource->set_source_ir("{\"root\":\"Node3D\"}");
		Array node_operations;
		node_operations.push_back(String("add_camera"));
		node_operations.push_back(String("add_light"));
		resource->set_node_operations(node_operations);
		Array resource_operations;
		resource_operations.push_back(String("create_material"));
		resource->set_resource_operations(resource_operations);
		Array warnings;
		warnings.push_back(String("needs_navmesh"));
		resource->set_warnings(warnings);
		Dictionary metadata;
		metadata["planner"] = "test";
		resource->set_metadata(metadata);

		const Ref<SceneSynthesisPlan> loaded = save_and_load_resource_roundtrip(resource, "tres");
		CHECK_EQ(loaded->get_prompt(), resource->get_prompt());
		CHECK_EQ(loaded->get_source_ir(), resource->get_source_ir());
		CHECK(loaded->get_node_operations() == resource->get_node_operations());
		CHECK(loaded->get_resource_operations() == resource->get_resource_operations());
		CHECK(loaded->get_warnings() == resource->get_warnings());
		CHECK(loaded->get_metadata() == resource->get_metadata());
	}

	SUBCASE("GDScriptRepairPatch") {
		Ref<GDScriptRepairPatch> resource;
		resource.instantiate();
		resource->set_script_path("res://scripts/demo.gd");
		resource->set_diagnostic_message("Unexpected indent");
		resource->set_line_start(12);
		resource->set_line_end(14);
		resource->set_replacement_text("pass");
		Array hunks;
		hunks.push_back(String("@@ -12,3 +12,1 @@"));
		resource->set_hunks(hunks);
		Array warnings;
		warnings.push_back(String("manual review required"));
		resource->set_warnings(warnings);
		Dictionary metadata;
		metadata["tool"] = "repair";
		resource->set_metadata(metadata);

		const Ref<GDScriptRepairPatch> loaded = save_and_load_resource_roundtrip(resource, "tres");
		CHECK_EQ(loaded->get_script_path(), resource->get_script_path());
		CHECK_EQ(loaded->get_diagnostic_message(), resource->get_diagnostic_message());
		CHECK_EQ(loaded->get_line_start(), resource->get_line_start());
		CHECK_EQ(loaded->get_line_end(), resource->get_line_end());
		CHECK_EQ(loaded->get_replacement_text(), resource->get_replacement_text());
		CHECK(loaded->get_hunks() == resource->get_hunks());
		CHECK(loaded->get_warnings() == resource->get_warnings());
		CHECK(loaded->get_metadata() == resource->get_metadata());
	}
}

TEST_CASE("[WoodotAI] AIModelResource tracks parameter fingerprint and runtime dirty state") {
	Ref<AIModelResource> resource;
	resource.instantiate();

	const String initial_fingerprint = resource->get_parameter_fingerprint();
	CHECK_FALSE(initial_fingerprint.is_empty());
	CHECK_FALSE(resource->is_runtime_dirty());

	resource->mark_runtime_clean();
	CHECK_FALSE(resource->is_runtime_dirty());

	resource->set_context_size(resource->get_context_size() + 1024);
	CHECK(resource->is_runtime_dirty());
	CHECK_NE(resource->get_parameter_fingerprint(), initial_fingerprint);

	resource->mark_runtime_clean();
	CHECK_FALSE(resource->is_runtime_dirty());

	resource->set_context_size(resource->get_context_size());
	CHECK_FALSE(resource->is_runtime_dirty());

	Dictionary options = resource->get_extra_options();
	options["draft"] = true;
	resource->set_extra_options(options);
	CHECK(resource->is_runtime_dirty());
}

TEST_CASE("[WoodotAI] AITensorResource limits large CPU buffers in inspector metadata") {
	Ref<AITensorResource> resource;
	resource.instantiate();

	PackedFloat32Array cpu_data;
	cpu_data.resize(300);
	for (int i = 0; i < cpu_data.size(); i++) {
		cpu_data.set(i, static_cast<float>(i) * 0.5f);
	}
	resource->set_cpu_data(cpu_data);

	CHECK(resource->is_cpu_data_inspector_limited());
	CHECK(resource->get_cpu_data_preview().contains("inspector display limited"));
	CHECK_FALSE(property_has_editor_usage(resource.ptr(), "cpu_data"));
}

#ifdef TOOLS_ENABLED
TEST_CASE("[WoodotAI] Editor MVP resolves scene synthesis task into plan preview") {
	ScopedEditorSingleton<NodeGraphIntentParser> parser(NodeGraphIntentParser::get_singleton());
	ScopedEditorSingleton<EditorAIPreviewDiff> preview(EditorAIPreviewDiff::get_singleton());
	ScopedEditorSingleton<UndoRedoBridge> bridge(UndoRedoBridge::get_singleton());
	ScopedEditorSingleton<EditorAIService> service(EditorAIService::get_singleton());

	const String scene_ir = "{\"prompt\":\"make a camera rig\",\"node_operations\":[{\"op\":\"create_node\",\"parent_path\":\".\",\"name\":\"GeneratedCamera\",\"node_type\":\"Camera3D\"},{\"op\":\"set_property\",\"target_path\":\"GeneratedCamera\",\"property\":\"current\",\"value\":true}],\"resource_operations\":[],\"warnings\":[]}";
	Ref<AITaskHandle> handle = make_completed_text_task(scene_ir);
	const Dictionary resolved = service->resolve_scene_synthesis_task(handle, "make a camera rig");

	CHECK(bool(resolved["ok"]));
	CHECK(resolved.has("plan"));
	CHECK(resolved.has("preview"));
	CHECK(resolved.has("apply_status"));

	const Dictionary preview_data = resolved["preview"];
	CHECK_EQ(String(preview_data["kind"]), String("scene_plan"));
	CHECK(int64_t(preview_data["operation_count"]) == 2);

	const Dictionary apply_status = resolved["apply_status"];
	CHECK_FALSE(bool(apply_status["can_apply"]));
}

TEST_CASE("[WoodotAI] Editor MVP resolves script repair task into patch preview") {
	ScopedEditorSingleton<GDScriptRepairEngine> repair_engine(GDScriptRepairEngine::get_singleton());
	ScopedEditorSingleton<EditorAIPreviewDiff> preview(EditorAIPreviewDiff::get_singleton());
	ScopedEditorSingleton<UndoRedoBridge> bridge(UndoRedoBridge::get_singleton());
	ScopedEditorSingleton<EditorAIService> service(EditorAIService::get_singleton());

	const String script_path = TestUtils::get_temp_path("woodot_ai_editor_repair_test.gd");
	{
		Ref<FileAccess> file = FileAccess::open(script_path, FileAccess::WRITE);
		REQUIRE(file.is_valid());
		file->store_string("extends Node\n\nfunc ready()\n\tpass\n");
	}

	Dictionary metadata;
	metadata["script_path"] = script_path;
	metadata["diagnostics"] = "Expected ':' after function declaration.";

	const String patch_ir = "{\"script_path\":\"" + script_path.json_escape() + "\",\"diagnostic_message\":\"Expected ':' after function declaration.\",\"hunks\":[{\"op\":\"replace_range\",\"line_start\":3,\"line_end\":3,\"replacement_text\":\"func ready():\"}],\"warnings\":[]}";
	Ref<AITaskHandle> handle = make_completed_text_task(patch_ir, metadata);
	const Dictionary resolved = service->resolve_script_repair_task(handle);

	CHECK(bool(resolved["ok"]));
	const Dictionary preview_data = resolved["preview"];
	CHECK_EQ(String(preview_data["kind"]), String("gdscript_patch"));
	CHECK(bool(preview_data["can_apply"]));

	const Dictionary apply_status = resolved["apply_status"];
	CHECK(bool(apply_status["can_apply"]));
	CHECK_EQ(Error(int64_t(apply_status["error"])), OK);
}

TEST_CASE("[WoodotAI] Editor parsers reject invalid scene and patch IR") {
	ScopedEditorSingleton<NodeGraphIntentParser> parser(NodeGraphIntentParser::get_singleton());
	ScopedEditorSingleton<GDScriptRepairEngine> repair_engine(GDScriptRepairEngine::get_singleton());

	const Dictionary invalid_scene = parser->validate_scene_plan_ir("{\"node_operations\":[{\"op\":\"create_node\",\"node_type\":\"NotARealNode\"}]}");
	CHECK_FALSE(bool(invalid_scene["valid"]));
	const Array scene_errors = invalid_scene["errors"];
	CHECK_FALSE(scene_errors.is_empty());

	const Dictionary invalid_patch = repair_engine->validate_patch_ir("{\"script_path\":12,\"hunks\":[{\"op\":\"delete_all\"}]}");
	CHECK_FALSE(bool(invalid_patch["valid"]));
	const Array patch_errors = invalid_patch["errors"];
	CHECK_FALSE(patch_errors.is_empty());
}
#endif

} // namespace TestWoodotAIRuntime
