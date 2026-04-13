/**************************************************************************/
/*  ai_runtime_server.cpp                                                 */
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

#include "modules/woodot_ai/runtime/ai_runtime_server.h"

#include "core/error/error_macros.h"
#include "core/object/class_db.h"
#include "core/os/mutex.h"
#include "core/templates/rid_owner.h"
#include "modules/woodot_ai/backends/llama/llama_backend.h"
#include "modules/woodot_ai/resources/ai_model_resource.h"
#include "modules/woodot_ai/runtime/ai_task_scheduler.h"

struct AIRuntimeServer::Data {
	struct ModelRecord {
		StringName backend_name;
		Ref<AIModelResource> resource;
		AIBackendModelHandle backend_model_handle;
		Dictionary metadata;
	};

	mutable Mutex mutex;
	RID_Owner<ModelRecord, true> loaded_models;
	AIBackendRegistry backend_registry;
	AITaskScheduler scheduler;
	LlamaBackend *llama_backend = nullptr;
	uint64_t load_requests = 0;
	uint64_t unload_requests = 0;
	uint64_t poll_count = 0;
};

AIRuntimeServer *AIRuntimeServer::singleton = nullptr;

void AIRuntimeServer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load_model", "model"), &AIRuntimeServer::load_model);
	ClassDB::bind_method(D_METHOD("unload_model", "model_rid"), &AIRuntimeServer::unload_model);
	ClassDB::bind_method(D_METHOD("has_model", "model_rid"), &AIRuntimeServer::has_model);
	ClassDB::bind_method(D_METHOD("submit_completion", "request"), &AIRuntimeServer::submit_completion);
	ClassDB::bind_method(D_METHOD("submit_embedding", "request"), &AIRuntimeServer::submit_embedding);
	ClassDB::bind_method(D_METHOD("cancel_task", "task_handle"), &AIRuntimeServer::cancel_task);
	ClassDB::bind_method(D_METHOD("get_runtime_stats"), &AIRuntimeServer::get_runtime_stats);
	ClassDB::bind_method(D_METHOD("get_model_info", "model_rid"), &AIRuntimeServer::get_model_info);
	ClassDB::bind_method(D_METHOD("get_backend_capabilities", "backend_name"), &AIRuntimeServer::get_backend_capabilities, DEFVAL(StringName()));
	ClassDB::bind_method(D_METHOD("get_registered_backends"), &AIRuntimeServer::get_registered_backends);
	ClassDB::bind_method(D_METHOD("poll_completed"), &AIRuntimeServer::poll_completed);
}

AIRuntimeServer *AIRuntimeServer::get_singleton() {
	return singleton;
}

RID AIRuntimeServer::load_model(const Ref<AIModelResource> &p_model) {
	ERR_FAIL_COND_V_MSG(p_model.is_null(), RID(), "AIRuntimeServer requires a valid AIModelResource.");

	const StringName backend_name = p_model->get_backend_name() == StringName() ? StringName("llama") : p_model->get_backend_name();

	AIBackend *backend = nullptr;
	{
		MutexLock lock(data->mutex);
		data->load_requests++;
		backend = data->backend_registry.get_backend(backend_name);
	}

	ERR_FAIL_NULL_V_MSG(backend, RID(), vformat("AIRuntimeServer could not find backend '%s'.", String(backend_name)));

	const AIBackendModelLoadResult load_result = backend->load_model(p_model);
	ERR_FAIL_COND_V_MSG(!load_result.is_ok(), RID(), load_result.message);

	Data::ModelRecord record;
	record.backend_name = backend_name;
	record.resource = p_model;
	record.backend_model_handle = load_result.model_handle;
	record.metadata = load_result.details;

	MutexLock lock(data->mutex);
	return data->loaded_models.make_rid(record);
}

void AIRuntimeServer::unload_model(const RID &p_model_rid) {
	if (!p_model_rid.is_valid()) {
		return;
	}

	MutexLock lock(data->mutex);
	data->unload_requests++;

	Data::ModelRecord *record = data->loaded_models.get_or_null(p_model_rid);
	if (record == nullptr) {
		return;
	}

	AIBackend *backend = data->backend_registry.get_backend(record->backend_name);
	if (backend != nullptr) {
		backend->unload_model(record->backend_model_handle);
	}

	data->loaded_models.free(p_model_rid);
}

bool AIRuntimeServer::has_model(const RID &p_model_rid) const {
	if (!p_model_rid.is_valid()) {
		return false;
	}

	MutexLock lock(data->mutex);
	return data->loaded_models.get_or_null(p_model_rid) != nullptr;
}

Ref<AITaskHandle> AIRuntimeServer::submit_completion(const Ref<AICompletionRequest> &p_request) {
	Ref<AITaskHandle> handle;
	handle.instantiate();

	if (p_request.is_null()) {
		handle->fail(ERR_INVALID_PARAMETER, "AIRuntimeServer requires a valid AICompletionRequest.");
		return handle;
	}

	AIBackend *backend = nullptr;
	AIBackendModelHandle backend_model_handle;
	const RID model_rid = p_request->get_model_rid();

	{
		MutexLock lock(data->mutex);
		Data::ModelRecord *record = data->loaded_models.get_or_null(model_rid);
		if (record == nullptr) {
			handle->fail(ERR_DOES_NOT_EXIST, "AICompletionRequest model_rid does not reference a loaded model.");
			return handle;
		}

		backend = data->backend_registry.get_backend(record->backend_name);
		backend_model_handle = record->backend_model_handle;
	}

	return data->scheduler.submit_completion(backend, backend_model_handle, model_rid, p_request);
}

Ref<AITaskHandle> AIRuntimeServer::submit_embedding(const Ref<AIEmbeddingRequest> &p_request) {
	Ref<AITaskHandle> handle;
	handle.instantiate();

	if (p_request.is_null()) {
		handle->fail(ERR_INVALID_PARAMETER, "AIRuntimeServer requires a valid AIEmbeddingRequest.");
		return handle;
	}

	AIBackend *backend = nullptr;
	AIBackendModelHandle backend_model_handle;
	const RID model_rid = p_request->get_model_rid();

	{
		MutexLock lock(data->mutex);
		Data::ModelRecord *record = data->loaded_models.get_or_null(model_rid);
		if (record == nullptr) {
			handle->fail(ERR_DOES_NOT_EXIST, "AIEmbeddingRequest model_rid does not reference a loaded model.");
			return handle;
		}

		backend = data->backend_registry.get_backend(record->backend_name);
		backend_model_handle = record->backend_model_handle;
	}

	return data->scheduler.submit_embedding(backend, backend_model_handle, model_rid, p_request);
}

void AIRuntimeServer::cancel_task(const Ref<AITaskHandle> &p_task_handle) {
	data->scheduler.cancel_task(p_task_handle);
}

Dictionary AIRuntimeServer::get_runtime_stats() const {
	Dictionary stats;
	stats["runtime_server"] = "woodot_ai";
	stats["loaded_models"] = static_cast<int64_t>(data->loaded_models.get_rid_count());
	stats["registered_backends"] = get_registered_backends();

	Dictionary backend_stats;
	const PackedStringArray backend_names = get_registered_backends();
	for (int i = 0; i < backend_names.size(); i++) {
		const StringName backend_name = StringName(backend_names[i]);
		const AIBackend *backend = data->backend_registry.get_backend(backend_name);
		if (backend != nullptr) {
			backend_stats[backend_name] = backend->get_runtime_stats();
		}
	}
	stats["backends"] = backend_stats;

	{
		MutexLock lock(data->mutex);
		stats["load_requests"] = static_cast<int64_t>(data->load_requests);
		stats["unload_requests"] = static_cast<int64_t>(data->unload_requests);
		stats["poll_count"] = static_cast<int64_t>(data->poll_count);
	}

	stats["scheduler"] = data->scheduler.get_stats();
	return stats;
}

Dictionary AIRuntimeServer::get_model_info(const RID &p_model_rid) const {
	Dictionary info;
	if (!p_model_rid.is_valid()) {
		return info;
	}

	MutexLock lock(data->mutex);
	const Data::ModelRecord *record = data->loaded_models.get_or_null(p_model_rid);
	if (record == nullptr) {
		return info;
	}

	info["model_rid"] = p_model_rid;
	info["backend_name"] = record->backend_name;
	info["source_path"] = record->resource.is_valid() ? record->resource->get_source_path() : String();
	info["backend_options"] = record->resource.is_valid() ? record->resource->get_backend_options() : Dictionary();
	info["backend_metadata"] = record->metadata;
	return info;
}

Dictionary AIRuntimeServer::get_backend_capabilities(const StringName &p_backend_name) const {
	Dictionary result;
	const PackedStringArray backend_names = get_registered_backends();

	auto _serialize_capabilities = [](const AIBackendCapabilities &p_capabilities) {
		Dictionary info;
		info["supports_completion"] = p_capabilities.supports_completion;
		info["supports_embedding"] = p_capabilities.supports_embedding;
		info["supports_streaming"] = p_capabilities.supports_streaming;
		info["supports_cancellation"] = p_capabilities.supports_cancellation;
		info["supports_context_reuse"] = p_capabilities.supports_context_reuse;
		info["supports_cpu_execution"] = p_capabilities.supports_cpu_execution;
		info["supports_gpu_execution"] = p_capabilities.supports_gpu_execution;
		info["preferred_parallel_jobs"] = static_cast<int64_t>(p_capabilities.preferred_parallel_jobs);
		info["supported_devices"] = p_capabilities.supported_devices;
		info["metadata"] = p_capabilities.metadata;
		return info;
	};

	if (p_backend_name != StringName()) {
		const AIBackend *backend = data->backend_registry.get_backend(p_backend_name);
		if (backend != nullptr) {
			result = _serialize_capabilities(backend->get_capabilities());
		}
		return result;
	}

	for (int i = 0; i < backend_names.size(); i++) {
		const StringName backend_name = StringName(backend_names[i]);
		const AIBackend *backend = data->backend_registry.get_backend(backend_name);
		if (backend != nullptr) {
			result[backend_name] = _serialize_capabilities(backend->get_capabilities());
		}
	}

	return result;
}

PackedStringArray AIRuntimeServer::get_registered_backends() const {
	MutexLock lock(data->mutex);
	return data->backend_registry.get_backend_names();
}

void AIRuntimeServer::poll_completed() {
	{
		MutexLock lock(data->mutex);
		data->poll_count++;
	}

	data->scheduler.poll_completed();
}

AIRuntimeServer::AIRuntimeServer() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;

	data = memnew(Data);
	data->llama_backend = memnew(LlamaBackend);
	data->backend_registry.register_backend(data->llama_backend->get_backend_name(), data->llama_backend);
}

AIRuntimeServer::~AIRuntimeServer() {
	if (data != nullptr) {
		if (data->llama_backend != nullptr) {
			data->backend_registry.unregister_backend(data->llama_backend->get_backend_name());
			memdelete(data->llama_backend);
			data->llama_backend = nullptr;
		}

		memdelete(data);
		data = nullptr;
	}

	singleton = nullptr;
}
