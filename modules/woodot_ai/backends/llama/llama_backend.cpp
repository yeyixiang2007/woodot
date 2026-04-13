/**************************************************************************/
/*  llama_backend.cpp                                                     */
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

#include "modules/woodot_ai/backends/llama/llama_backend.h"

#include "core/error/error_macros.h"
#include "core/os/mutex.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/rid_owner.h"
#include "modules/woodot_ai/runtime/ai_token_stream.h"

#include <llama.h>

namespace {
Mutex llama_api_mutex;
int llama_api_refcount = 0;

void _acquire_llama_api() {
	MutexLock lock(llama_api_mutex);

	if (llama_api_refcount == 0) {
		llama_backend_init();
	}

	llama_api_refcount++;
}

void _release_llama_api() {
	MutexLock lock(llama_api_mutex);

	if (llama_api_refcount == 0) {
		return;
	}

	llama_api_refcount--;

	if (llama_api_refcount == 0) {
		llama_backend_free();
	}
}
} // namespace

struct LlamaBackend::Data {
	struct ModelState {
		uint64_t load_sequence = 0;
		uint32_t active_contexts = 0;
		bool unloading = false;
	};

	struct ContextState {
		RID model_rid;
		uint64_t create_sequence = 0;
		bool busy = false;
		bool cancel_requested = false;
	};

	mutable Mutex mutex;
	mutable RID_Owner<ModelState, true> model_owner;
	mutable RID_Owner<ContextState, true> context_owner;
	mutable HashMap<uint64_t, RID> running_jobs;

	uint64_t load_counter = 0;
	uint64_t context_counter = 0;
	uint64_t rejected_job_count = 0;
	uint64_t cancel_request_count = 0;
	uint64_t streamed_job_count = 0;
	uint64_t stream_flush_count = 0;
	uint64_t streamed_token_count = 0;
};

LlamaBackend::LlamaBackend() {
	data = memnew(Data);
	_acquire_llama_api();
}

LlamaBackend::~LlamaBackend() {
	memdelete(data);
	data = nullptr;
	_release_llama_api();
}

StringName LlamaBackend::get_backend_name() const {
	static const StringName backend_name("llama");
	return backend_name;
}

AIBackendCapabilities LlamaBackend::get_capabilities() const {
	AIBackendCapabilities capabilities;
	capabilities.supports_completion = true;
	capabilities.supports_embedding = true;
	capabilities.supports_streaming = true;
	capabilities.supports_cancellation = true;
	capabilities.supports_context_reuse = true;
	capabilities.supports_cpu_execution = true;
	capabilities.supports_gpu_execution = false;
	capabilities.preferred_parallel_jobs = 1;
	capabilities.supported_devices.push_back("cpu");
	capabilities.metadata["runtime_ready"] = false;
	capabilities.metadata["implementation_stage"] = "skeleton";
	return capabilities;
}

AIBackendValidationResult LlamaBackend::validate_model(const Ref<AIModelResource> &p_model) const {
	AIBackendValidationResult result;

	if (p_model.is_null()) {
		result.code = ERR_INVALID_PARAMETER;
		result.message = "LlamaBackend requires a valid AIModelResource.";
		return result;
	}

	result.details["backend"] = get_backend_name();
	result.details["implementation_stage"] = "skeleton";
	return result;
}

AIBackendModelLoadResult LlamaBackend::load_model(const Ref<AIModelResource> &p_model) {
	AIBackendModelLoadResult result;
	const AIBackendValidationResult validation = validate_model(p_model);
	if (!validation.is_ok()) {
		result.code = validation.code;
		result.message = validation.message;
		result.details = validation.details;
		return result;
	}

	Data::ModelState model_state;
	{
		MutexLock lock(data->mutex);
		model_state.load_sequence = ++data->load_counter;
	}

	const RID model_rid = data->model_owner.make_rid(model_state);

	result.model_handle.rid = model_rid;
	result.model_handle.backend_name = get_backend_name();
	result.model_handle.metadata["implementation_stage"] = "skeleton";
	result.model_handle.metadata["load_sequence"] = static_cast<int64_t>(model_state.load_sequence);
	result.details = result.model_handle.metadata;
	return result;
}

void LlamaBackend::unload_model(const AIBackendModelHandle &p_model_handle) {
	if (p_model_handle.backend_name != StringName() && p_model_handle.backend_name != get_backend_name()) {
		WARN_PRINT(vformat("Ignoring unload for backend '%s' on LlamaBackend.", String(p_model_handle.backend_name)));
		return;
	}

	MutexLock lock(data->mutex);
	Data::ModelState *model = data->model_owner.get_or_null(p_model_handle.rid);
	if (model == nullptr) {
		return;
	}

	if (model->active_contexts > 0) {
		model->unloading = true;
		WARN_PRINT("LlamaBackend skeleton refused to unload a model with active contexts.");
		return;
	}

	data->model_owner.free(p_model_handle.rid);
}

AIBackendContextCreateResult LlamaBackend::create_context(const AIBackendModelHandle &p_model_handle) {
	AIBackendContextCreateResult result;

	if (p_model_handle.backend_name != StringName() && p_model_handle.backend_name != get_backend_name()) {
		result.code = ERR_INVALID_PARAMETER;
		result.message = "AIBackendModelHandle does not belong to LlamaBackend.";
		return result;
	}

	MutexLock lock(data->mutex);
	Data::ModelState *model = data->model_owner.get_or_null(p_model_handle.rid);
	if (model == nullptr) {
		result.code = ERR_DOES_NOT_EXIST;
		result.message = "Cannot create llama context for an unloaded model.";
		return result;
	}

	if (model->unloading) {
		result.code = ERR_BUSY;
		result.message = "Cannot create llama context while model is unloading.";
		return result;
	}

	Data::ContextState context_state;
	context_state.model_rid = p_model_handle.rid;
	context_state.create_sequence = ++data->context_counter;

	const RID context_rid = data->context_owner.make_rid(context_state);
	model->active_contexts++;

	result.context_handle.rid = context_rid;
	result.context_handle.model_rid = p_model_handle.rid;
	result.context_handle.backend_name = get_backend_name();
	result.context_handle.exclusive_decode = true;
	result.context_handle.metadata["implementation_stage"] = "skeleton";
	result.context_handle.metadata["create_sequence"] = static_cast<int64_t>(context_state.create_sequence);
	result.details = result.context_handle.metadata;
	return result;
}

void LlamaBackend::destroy_context(const AIBackendContextHandle &p_context_handle) {
	if (p_context_handle.backend_name != StringName() && p_context_handle.backend_name != get_backend_name()) {
		WARN_PRINT(vformat("Ignoring context destroy for backend '%s' on LlamaBackend.", String(p_context_handle.backend_name)));
		return;
	}

	MutexLock lock(data->mutex);
	Data::ContextState *context = data->context_owner.get_or_null(p_context_handle.rid);
	if (context == nullptr) {
		return;
	}

	const RID model_rid = context->model_rid;
	Data::ModelState *model = data->model_owner.get_or_null(model_rid);
	if (model != nullptr && model->active_contexts > 0) {
		model->active_contexts--;
	}

	data->context_owner.free(p_context_handle.rid);

	if (model != nullptr && model->unloading && model->active_contexts == 0) {
		data->model_owner.free(model_rid);
	}
}

AIBackendResult LlamaBackend::run_job(const AIComputeJob &p_job) {
	AIBackendResult result;
	result.metadata["backend"] = get_backend_name();
	result.metadata["implementation_stage"] = "skeleton";
	result.metadata["job_id"] = static_cast<int64_t>(p_job.job_id);

	if (!p_job.model_handle.is_valid()) {
		result.code = ERR_INVALID_PARAMETER;
		result.message = "LlamaBackend requires a valid model handle.";
		return result;
	}

	if (p_job.model_handle.backend_name != StringName() && p_job.model_handle.backend_name != get_backend_name()) {
		result.code = ERR_INVALID_PARAMETER;
		result.message = "AIComputeJob model handle does not belong to LlamaBackend.";
		return result;
	}

	if (!p_job.context_handle.is_valid()) {
		result.code = ERR_INVALID_PARAMETER;
		result.message = "LlamaBackend requires a valid context handle.";
		return result;
	}

	if (p_job.context_handle.backend_name != StringName() && p_job.context_handle.backend_name != get_backend_name()) {
		result.code = ERR_INVALID_PARAMETER;
		result.message = "AIComputeJob context handle does not belong to LlamaBackend.";
		return result;
	}

	{
		MutexLock lock(data->mutex);
		Data::ModelState *model = data->model_owner.get_or_null(p_job.model_handle.rid);
		Data::ContextState *context = data->context_owner.get_or_null(p_job.context_handle.rid);

		if (model == nullptr) {
			result.code = ERR_DOES_NOT_EXIST;
			result.message = "LlamaBackend model handle is no longer loaded.";
			return result;
		}

		if (context == nullptr) {
			result.code = ERR_DOES_NOT_EXIST;
			result.message = "LlamaBackend context handle no longer exists.";
			return result;
		}

		if (context->model_rid != p_job.model_handle.rid) {
			result.code = ERR_INVALID_PARAMETER;
			result.message = "AIComputeJob context handle does not belong to the provided model handle.";
			return result;
		}

		if (context->busy) {
			result.code = ERR_BUSY;
			result.message = "LlamaBackend context is already executing a job.";
			return result;
		}

		context->busy = true;
		context->cancel_requested = false;
		data->running_jobs.insert(p_job.job_id, p_job.context_handle.rid);
		if (p_job.stream) {
			data->streamed_job_count++;
		}
	}

	AITokenStream token_stream;
	AITokenStream::Config token_stream_config;
	token_stream_config.enabled = p_job.stream;
	token_stream.configure(token_stream_config);
	if (p_job.stream && !p_job.prompt.is_empty()) {
		PackedStringArray preview_tokens;
		preview_tokens.push_back("[streaming-not-implemented]");
		token_stream.push_tokens(preview_tokens, 0);
	}

	{
		MutexLock lock(data->mutex);
		Data::ContextState *context = data->context_owner.get_or_null(p_job.context_handle.rid);
		if (context != nullptr) {
			context->busy = false;
		}
		data->running_jobs.erase(p_job.job_id);
		data->rejected_job_count++;
		data->stream_flush_count += token_stream.get_flush_count();
		data->streamed_token_count += token_stream.get_total_tokens();
	}

	result.code = ERR_UNAVAILABLE;
	result.message = "LlamaBackend skeleton is initialized but inference execution is not implemented yet.";
	result.metadata["streaming_requested"] = p_job.stream;
	result.metadata["token_stream"] = token_stream.get_stats();
	return result;
}

bool LlamaBackend::cancel_job(uint64_t p_job_id) {
	MutexLock lock(data->mutex);
	data->cancel_request_count++;

	HashMap<uint64_t, RID>::Iterator job = data->running_jobs.find(p_job_id);
	if (!job) {
		return false;
	}

	Data::ContextState *context = data->context_owner.get_or_null(job->value);
	if (context == nullptr) {
		data->running_jobs.erase(p_job_id);
		return false;
	}

	context->cancel_requested = true;
	return true;
}

Dictionary LlamaBackend::get_runtime_stats() const {
	Dictionary stats;
	stats["backend"] = get_backend_name();
	stats["implementation_stage"] = "skeleton";
	stats["loaded_models"] = static_cast<int64_t>(data->model_owner.get_rid_count());
	stats["active_contexts"] = static_cast<int64_t>(data->context_owner.get_rid_count());

	{
		MutexLock lock(data->mutex);
		stats["load_count"] = static_cast<int64_t>(data->load_counter);
		stats["context_create_count"] = static_cast<int64_t>(data->context_counter);
		stats["running_jobs"] = static_cast<int64_t>(data->running_jobs.size());
		stats["rejected_job_count"] = static_cast<int64_t>(data->rejected_job_count);
		stats["cancel_request_count"] = static_cast<int64_t>(data->cancel_request_count);
		stats["streamed_job_count"] = static_cast<int64_t>(data->streamed_job_count);
		stats["stream_flush_count"] = static_cast<int64_t>(data->stream_flush_count);
		stats["streamed_token_count"] = static_cast<int64_t>(data->streamed_token_count);
	}

	stats["runtime_ready"] = false;
	stats["device"] = "cpu";
	stats["llama_system_info"] = String::utf8(llama_print_system_info());
	return stats;
}
