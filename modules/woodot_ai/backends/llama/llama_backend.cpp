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

#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/io/file_access.h"
#include "core/os/mutex.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid_owner.h"
#include "modules/woodot_ai/runtime/ai_token_stream.h"
#include "modules/woodot_ai/resources/ai_model_resource.h"

#include <llama.h>

namespace {
Mutex llama_api_mutex;
int llama_api_refcount = 0;

String _resolve_model_path(const String &p_model_path) {
	if (p_model_path.is_empty()) {
		return String();
	}

	if (p_model_path.is_resource_file() || p_model_path.begins_with("user://")) {
		return ProjectSettings::get_singleton()->globalize_path(p_model_path);
	}

	return p_model_path;
}

String _read_model_meta_string(const llama_model *p_model, const char *p_key) {
	char buffer[512] = {};
	const int32_t written = llama_model_meta_val_str(p_model, p_key, buffer, sizeof(buffer));
	if (written <= 0) {
		return String();
	}

	return String::utf8(buffer);
}

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

uint32_t _coerce_u32_option(const Dictionary &p_options, const StringName &p_key, uint32_t p_default_value, uint32_t p_min_value = 0) {
	if (!p_options.has(p_key)) {
		return p_default_value;
	}

	const Variant value = p_options[p_key];
	if (value.get_type() != Variant::INT && value.get_type() != Variant::FLOAT && value.get_type() != Variant::BOOL) {
		return p_default_value;
	}

	const int64_t numeric = int64_t(value);
	return static_cast<uint32_t>(MAX(numeric, int64_t(p_min_value)));
}

int32_t _coerce_i32_option(const Dictionary &p_options, const StringName &p_key, int32_t p_default_value, int32_t p_min_value = 0) {
	if (!p_options.has(p_key)) {
		return p_default_value;
	}

	const Variant value = p_options[p_key];
	if (value.get_type() != Variant::INT && value.get_type() != Variant::FLOAT && value.get_type() != Variant::BOOL) {
		return p_default_value;
	}

	return MAX(static_cast<int32_t>(int64_t(value)), p_min_value);
}

float _coerce_float_option(const Dictionary &p_options, const StringName &p_key, float p_default_value) {
	if (!p_options.has(p_key)) {
		return p_default_value;
	}

	const Variant value = p_options[p_key];
	if (value.get_type() != Variant::INT && value.get_type() != Variant::FLOAT && value.get_type() != Variant::BOOL) {
		return p_default_value;
	}

	return float(value);
}

bool _coerce_bool_option(const Dictionary &p_options, const StringName &p_key, bool p_default_value) {
	if (!p_options.has(p_key)) {
		return p_default_value;
	}

	return static_cast<bool>(p_options[p_key]);
}

llama_flash_attn_type _coerce_flash_attn_type_option(const Dictionary &p_options, const StringName &p_key, llama_flash_attn_type p_default_value) {
	if (!p_options.has(p_key)) {
		return p_default_value;
	}

	const Variant value = p_options[p_key];
	if (value.get_type() == Variant::BOOL) {
		return static_cast<bool>(value) ? LLAMA_FLASH_ATTN_TYPE_ENABLED : LLAMA_FLASH_ATTN_TYPE_DISABLED;
	}
	if (value.get_type() == Variant::INT || value.get_type() == Variant::FLOAT) {
		const int64_t numeric = int64_t(value);
		if (numeric == LLAMA_FLASH_ATTN_TYPE_DISABLED || numeric == LLAMA_FLASH_ATTN_TYPE_ENABLED || numeric == LLAMA_FLASH_ATTN_TYPE_AUTO) {
			return static_cast<llama_flash_attn_type>(numeric);
		}
		return p_default_value;
	}
	if (value.get_type() == Variant::STRING || value.get_type() == Variant::STRING_NAME) {
		const String normalized = String(value).strip_edges().to_lower();
		if (normalized == "auto") {
			return LLAMA_FLASH_ATTN_TYPE_AUTO;
		}
		if (normalized == "enabled" || normalized == "enable" || normalized == "on" || normalized == "true") {
			return LLAMA_FLASH_ATTN_TYPE_ENABLED;
		}
		if (normalized == "disabled" || normalized == "disable" || normalized == "off" || normalized == "false") {
			return LLAMA_FLASH_ATTN_TYPE_DISABLED;
		}
	}

	return p_default_value;
}
} // namespace

struct LlamaBackend::Data {
	struct ModelState {
		uint64_t load_sequence = 0;
		uint32_t active_contexts = 0;
		bool unloading = false;
		String source_path;
		Dictionary context_metadata;
		llama_context_params context_params = llama_context_default_params();
		llama_model *native_model = nullptr;
	};

	struct ContextState {
		RID model_rid;
		uint64_t create_sequence = 0;
		bool busy = false;
		bool destroy_requested = false;
		bool cancel_requested = false;
		llama_context *native_context = nullptr;
	};

	mutable Mutex mutex;
	mutable RID_Owner<ModelState, true> model_owner;
	mutable RID_Owner<ContextState, true> context_owner;
	mutable HashMap<uint64_t, RID> running_jobs;

	uint64_t load_counter = 0;
	uint64_t context_counter = 0;
	uint64_t context_destroy_counter = 0;
	uint64_t rejected_job_count = 0;
	uint64_t cancel_request_count = 0;
	uint64_t streamed_job_count = 0;
	uint64_t stream_flush_count = 0;
	uint64_t streamed_token_count = 0;
};

bool _tokenize_prompt(const llama_vocab *p_vocab, const String &p_prompt, LocalVector<llama_token> &r_tokens, String &r_error) {
	ERR_FAIL_NULL_V(p_vocab, false);

	const CharString prompt_utf8 = p_prompt.utf8();
	const int32_t prompt_len = prompt_utf8.length();
	int32_t token_capacity = MAX<int32_t>(prompt_len + 8, 32);
	r_tokens.resize(token_capacity);

	const int32_t token_count = llama_tokenize(
			p_vocab,
			prompt_utf8.get_data(),
			prompt_len,
			r_tokens.ptr(),
			token_capacity,
			true,
			true);
	if (token_count == INT32_MIN) {
		r_error = "Prompt tokenization overflowed int32_t capacity.";
		r_tokens.clear();
		return false;
	}
	if (token_count < 0) {
		token_capacity = -token_count;
		r_tokens.resize(token_capacity);
		const int32_t retry_count = llama_tokenize(
				p_vocab,
				prompt_utf8.get_data(),
				prompt_len,
				r_tokens.ptr(),
				token_capacity,
				true,
				true);
		if (retry_count < 0) {
			r_error = "LlamaBackend failed to tokenize prompt.";
			r_tokens.clear();
			return false;
		}
		r_tokens.resize(retry_count);
		return true;
	}

	r_tokens.resize(token_count);
	return true;
}

String _token_to_string(const llama_vocab *p_vocab, llama_token p_token) {
	char stack_buffer[256];
	int32_t piece_size = llama_token_to_piece(p_vocab, p_token, stack_buffer, sizeof(stack_buffer), 0, true);
	if (piece_size == INT32_MIN) {
		return String();
	}
	if (piece_size < 0) {
		LocalVector<char> dynamic_buffer;
		dynamic_buffer.resize(-piece_size + 1);
		piece_size = llama_token_to_piece(p_vocab, p_token, dynamic_buffer.ptr(), dynamic_buffer.size(), 0, true);
		if (piece_size < 0) {
			return String();
		}
		dynamic_buffer[piece_size] = '\0';
		return String::utf8(dynamic_buffer.ptr(), piece_size);
	}

	return String::utf8(stack_buffer, piece_size);
}

String _describe_decode_error(int32_t p_decode_result) {
	switch (p_decode_result) {
		case 1:
			return "llama_decode() could not reserve a KV slot for the submitted batch.";
		case 2:
			return "llama_decode() aborted before the batch completed.";
		case -1:
			return "llama_decode() rejected the submitted batch as invalid.";
		default:
			return vformat("llama_decode() failed with code %d.", p_decode_result);
	}
}

Error _map_decode_error_code(int32_t p_decode_result) {
	switch (p_decode_result) {
		case 1:
			return ERR_OUT_OF_MEMORY;
		case 2:
			return ERR_SKIP;
		case -1:
			return ERR_INVALID_PARAMETER;
		default:
			return ERR_CANT_CREATE;
	}
}

llama_sampler *_build_completion_sampler(const AIComputeJob &p_job) {
	llama_sampler_chain_params sampler_params = llama_sampler_chain_default_params();
	llama_sampler *sampler = llama_sampler_chain_init(sampler_params);
	ERR_FAIL_NULL_V(sampler, nullptr);

	if (p_job.top_k > 0) {
		llama_sampler_chain_add(sampler, llama_sampler_init_top_k(p_job.top_k));
	}
	if (p_job.top_p > 0.0f && p_job.top_p < 1.0f) {
		llama_sampler_chain_add(sampler, llama_sampler_init_top_p(p_job.top_p, 1));
	}

	if (p_job.temperature <= 0.0f) {
		llama_sampler_chain_add(sampler, llama_sampler_init_greedy());
		return sampler;
	}

	llama_sampler_chain_add(sampler, llama_sampler_init_temp(p_job.temperature));

	uint32_t seed = LLAMA_DEFAULT_SEED;
	if (p_job.metadata.has("seed")) {
		const Variant seed_value = p_job.metadata["seed"];
		if (seed_value.get_type() == Variant::INT || seed_value.get_type() == Variant::FLOAT || seed_value.get_type() == Variant::BOOL) {
			seed = static_cast<uint32_t>(MAX(int64_t(seed_value), int64_t(0)));
		}
	}
	llama_sampler_chain_add(sampler, llama_sampler_init_dist(seed));
	return sampler;
}

LlamaBackend::LlamaBackend() {
	data = memnew(Data);
	_acquire_llama_api();
}

LlamaBackend::~LlamaBackend() {
	{
		MutexLock lock(data->mutex);

		const LocalVector<RID> context_rids = data->context_owner.get_owned_list();
		for (uint32_t i = 0; i < context_rids.size(); i++) {
			Data::ContextState *context = data->context_owner.get_or_null(context_rids[i]);
			if (context == nullptr) {
				continue;
			}
			if (context->native_context != nullptr) {
				llama_free(context->native_context);
				context->native_context = nullptr;
			}
			data->context_owner.free(context_rids[i]);
		}

		const LocalVector<RID> model_rids = data->model_owner.get_owned_list();
		for (uint32_t i = 0; i < model_rids.size(); i++) {
			Data::ModelState *model = data->model_owner.get_or_null(model_rids[i]);
			if (model == nullptr) {
				continue;
			}
			if (model->native_model != nullptr) {
				llama_model_free(model->native_model);
				model->native_model = nullptr;
			}
			data->model_owner.free(model_rids[i]);
		}
	}

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
	capabilities.supports_streaming = false;
	capabilities.supports_cancellation = true;
	capabilities.supports_context_reuse = true;
	capabilities.supports_cpu_execution = true;
	capabilities.supports_gpu_execution = llama_supports_gpu_offload();
	capabilities.preferred_parallel_jobs = 1;
	capabilities.supported_devices.push_back("cpu");
	if (capabilities.supports_gpu_execution) {
		capabilities.supported_devices.push_back("gpu");
	}
	capabilities.metadata["runtime_ready"] = true;
	capabilities.metadata["implementation_stage"] = "completion-minimal";
	return capabilities;
}

AIBackendValidationResult LlamaBackend::validate_model(const Ref<AIModelResource> &p_model) const {
	AIBackendValidationResult result;

	if (p_model.is_null()) {
		result.code = ERR_INVALID_PARAMETER;
		result.message = "LlamaBackend requires a valid AIModelResource.";
		return result;
	}

	if (p_model->get_backend_type() != StringName() && p_model->get_backend_type() != get_backend_name()) {
		result.code = ERR_INVALID_PARAMETER;
		result.message = vformat("AIModelResource backend_type '%s' does not belong to LlamaBackend.", String(p_model->get_backend_type()));
		return result;
	}

	const String model_path = p_model->get_model_path().strip_edges();
	if (model_path.is_empty()) {
		result.code = ERR_INVALID_PARAMETER;
		result.message = "LlamaBackend requires AIModelResource.model_path to point to a local .gguf file.";
		return result;
	}

	if (model_path.get_extension().to_lower() != "gguf") {
		result.code = ERR_FILE_UNRECOGNIZED;
		result.message = vformat("LlamaBackend only supports '.gguf' model files, got '%s'.", model_path.get_file());
		return result;
	}

	const String resolved_model_path = _resolve_model_path(model_path);
	if (resolved_model_path.is_empty() || !FileAccess::exists(resolved_model_path)) {
		result.code = ERR_FILE_NOT_FOUND;
		result.message = vformat("LlamaBackend could not find model file '%s'.", model_path);
		result.details["resolved_model_path"] = resolved_model_path;
		return result;
	}

	result.details["backend"] = get_backend_name();
	result.details["implementation_stage"] = "model-loading";
	result.details["resolved_model_path"] = resolved_model_path;
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

	const String source_path = p_model->get_model_path().strip_edges();
	const String resolved_model_path = validation.details.get("resolved_model_path", _resolve_model_path(source_path));

	llama_model_params model_params = llama_model_default_params();
	model_params.use_mmap = llama_supports_mmap();
	model_params.use_mlock = false;
	model_params.check_tensors = true;
	model_params.n_gpu_layers = p_model->get_n_gpu_layers();

	const CharString resolved_model_path_utf8 = resolved_model_path.utf8();
	llama_model *native_model = llama_model_load_from_file(resolved_model_path_utf8.get_data(), model_params);
	if (native_model == nullptr) {
		result.code = ERR_CANT_OPEN;
		result.message = vformat("LlamaBackend failed to load GGUF model from '%s'.", source_path);
		result.details = validation.details;
		result.details["n_gpu_layers"] = p_model->get_n_gpu_layers();
		result.details["mmap_enabled"] = model_params.use_mmap;
		result.details["check_tensors"] = model_params.check_tensors;
		return result;
	}

	Data::ModelState model_state;
	{
		MutexLock lock(data->mutex);
		model_state.load_sequence = ++data->load_counter;
	}
	model_state.source_path = resolved_model_path;
	model_state.native_model = native_model;
	model_state.context_params = llama_context_default_params();

	const int32_t n_threads = p_model->get_n_threads() > 0 ? p_model->get_n_threads() : model_state.context_params.n_threads;
	const Dictionary extra_options = p_model->get_extra_options();
	model_state.context_params.n_ctx = MAX(0, p_model->get_context_size());
	model_state.context_params.n_threads = n_threads;
	model_state.context_params.n_threads_batch = _coerce_i32_option(extra_options, StringName("n_threads_batch"), n_threads, 1);
	model_state.context_params.n_batch = _coerce_u32_option(extra_options, StringName("n_batch"), model_state.context_params.n_batch, 1);
	model_state.context_params.n_ubatch = _coerce_u32_option(extra_options, StringName("n_ubatch"), MIN(model_state.context_params.n_batch, model_state.context_params.n_ubatch), 1);
	model_state.context_params.n_ubatch = MIN(model_state.context_params.n_batch, model_state.context_params.n_ubatch);
	model_state.context_params.n_seq_max = _coerce_u32_option(extra_options, StringName("n_seq_max"), model_state.context_params.n_seq_max, 1);
	model_state.context_params.flash_attn_type = _coerce_flash_attn_type_option(extra_options, StringName("flash_attn"), model_state.context_params.flash_attn_type);
	model_state.context_params.embeddings = _coerce_bool_option(extra_options, StringName("embeddings"), model_state.context_params.embeddings);
	model_state.context_params.offload_kqv = _coerce_bool_option(extra_options, StringName("offload_kqv"), model_state.context_params.offload_kqv);
	model_state.context_params.no_perf = _coerce_bool_option(extra_options, StringName("no_perf"), model_state.context_params.no_perf);
	model_state.context_params.op_offload = _coerce_bool_option(extra_options, StringName("op_offload"), model_state.context_params.op_offload);
	model_state.context_params.swa_full = _coerce_bool_option(extra_options, StringName("swa_full"), model_state.context_params.swa_full);
	model_state.context_params.kv_unified = _coerce_bool_option(extra_options, StringName("kv_unified"), model_state.context_params.kv_unified);
	if (p_model->get_rope_scaling() > 0.0f && p_model->get_rope_scaling() != 1.0f) {
		model_state.context_params.rope_scaling_type = LLAMA_ROPE_SCALING_TYPE_LINEAR;
		model_state.context_params.rope_freq_scale = 1.0f / p_model->get_rope_scaling();
	}
	model_state.context_params.yarn_orig_ctx = _coerce_u32_option(extra_options, StringName("yarn_orig_ctx"), model_state.context_params.yarn_orig_ctx);
	model_state.context_params.yarn_ext_factor = _coerce_float_option(extra_options, StringName("yarn_ext_factor"), model_state.context_params.yarn_ext_factor);
	model_state.context_params.yarn_attn_factor = _coerce_float_option(extra_options, StringName("yarn_attn_factor"), model_state.context_params.yarn_attn_factor);
	model_state.context_params.yarn_beta_fast = _coerce_float_option(extra_options, StringName("yarn_beta_fast"), model_state.context_params.yarn_beta_fast);
	model_state.context_params.yarn_beta_slow = _coerce_float_option(extra_options, StringName("yarn_beta_slow"), model_state.context_params.yarn_beta_slow);
	model_state.context_metadata["context_size"] = static_cast<int64_t>(model_state.context_params.n_ctx);
	model_state.context_metadata["n_batch"] = static_cast<int64_t>(model_state.context_params.n_batch);
	model_state.context_metadata["n_ubatch"] = static_cast<int64_t>(model_state.context_params.n_ubatch);
	model_state.context_metadata["n_seq_max"] = static_cast<int64_t>(model_state.context_params.n_seq_max);
	model_state.context_metadata["n_threads"] = model_state.context_params.n_threads;
	model_state.context_metadata["n_threads_batch"] = model_state.context_params.n_threads_batch;
	model_state.context_metadata["flash_attn_type"] = static_cast<int64_t>(model_state.context_params.flash_attn_type);
	model_state.context_metadata["embeddings"] = model_state.context_params.embeddings;
	model_state.context_metadata["offload_kqv"] = model_state.context_params.offload_kqv;
	model_state.context_metadata["no_perf"] = model_state.context_params.no_perf;
	model_state.context_metadata["op_offload"] = model_state.context_params.op_offload;
	model_state.context_metadata["swa_full"] = model_state.context_params.swa_full;
	model_state.context_metadata["kv_unified"] = model_state.context_params.kv_unified;
	model_state.context_metadata["rope_scaling"] = p_model->get_rope_scaling();
	model_state.context_metadata["rope_freq_scale"] = model_state.context_params.rope_freq_scale;

	const RID model_rid = data->model_owner.make_rid(model_state);

	result.model_handle.rid = model_rid;
	result.model_handle.backend_name = get_backend_name();
	result.model_handle.metadata["implementation_stage"] = "model-loaded";
	result.model_handle.metadata["load_sequence"] = static_cast<int64_t>(model_state.load_sequence);
	result.model_handle.metadata["resolved_model_path"] = resolved_model_path;
	result.model_handle.metadata["n_gpu_layers"] = p_model->get_n_gpu_layers();
	result.model_handle.metadata["n_ctx_train"] = llama_model_n_ctx_train(native_model);
	result.model_handle.metadata["n_layer"] = llama_model_n_layer(native_model);
	result.model_handle.metadata["n_embd"] = llama_model_n_embd(native_model);
	result.model_handle.metadata["n_params"] = static_cast<int64_t>(llama_model_n_params(native_model));
	result.model_handle.metadata["model_size_bytes"] = static_cast<int64_t>(llama_model_size(native_model));
	result.model_handle.metadata["supports_gpu_offload"] = llama_supports_gpu_offload();
	result.model_handle.metadata["context"] = model_state.context_metadata;

	char description[512] = {};
	if (llama_model_desc(native_model, description, sizeof(description)) > 0) {
		result.model_handle.metadata["model_description"] = String::utf8(description);
	}

	const llama_vocab *vocab = llama_model_get_vocab(native_model);
	if (vocab != nullptr) {
		result.model_handle.metadata["vocab_size"] = llama_vocab_n_tokens(vocab);
	}

	const String architecture = _read_model_meta_string(native_model, "general.architecture");
	if (!architecture.is_empty()) {
		result.model_handle.metadata["architecture"] = architecture;
	}

	const String name = _read_model_meta_string(native_model, "general.name");
	if (!name.is_empty()) {
		result.model_handle.metadata["model_name"] = name;
	}

	result.details = validation.details;
	result.details.merge(result.model_handle.metadata, true);
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
		WARN_PRINT("LlamaBackend refused to unload a model with active contexts.");
		return;
	}

	if (model->native_model != nullptr) {
		llama_model_free(model->native_model);
		model->native_model = nullptr;
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

	Data::ContextState context_state;
	Dictionary context_metadata;
	llama_model *native_model = nullptr;
	llama_context_params context_params = llama_context_default_params();

	{
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

		if (model->native_model == nullptr) {
			result.code = ERR_DOES_NOT_EXIST;
			result.message = "Cannot create llama context for a model that is no longer loaded.";
			return result;
		}

		context_state.model_rid = p_model_handle.rid;
		context_state.create_sequence = ++data->context_counter;
		context_metadata = model->context_metadata;
		context_params = model->context_params;
		native_model = model->native_model;
		model->active_contexts++;
	}

	context_state.native_context = llama_init_from_model(native_model, context_params);
	if (context_state.native_context == nullptr) {
		MutexLock lock(data->mutex);
		Data::ModelState *model = data->model_owner.get_or_null(p_model_handle.rid);
		if (model != nullptr && model->active_contexts > 0) {
			model->active_contexts--;
			if (model->unloading && model->active_contexts == 0) {
				if (model->native_model != nullptr) {
					llama_model_free(model->native_model);
					model->native_model = nullptr;
				}
				data->model_owner.free(p_model_handle.rid);
			}
		}
		result.code = ERR_CANT_CREATE;
		result.message = "LlamaBackend failed to create llama_context from the loaded model.";
		result.details = context_metadata;
		return result;
	}

	const RID context_rid = data->context_owner.make_rid(context_state);

	result.context_handle.rid = context_rid;
	result.context_handle.model_rid = p_model_handle.rid;
	result.context_handle.backend_name = get_backend_name();
	result.context_handle.exclusive_decode = true;
	result.context_handle.metadata["implementation_stage"] = "context-created";
	result.context_handle.metadata["create_sequence"] = static_cast<int64_t>(context_state.create_sequence);
	result.context_handle.metadata["n_ctx"] = static_cast<int64_t>(llama_n_ctx(context_state.native_context));
	result.context_handle.metadata["n_batch"] = static_cast<int64_t>(llama_n_batch(context_state.native_context));
	result.context_handle.metadata.merge(context_metadata, false);
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

	if (context->busy) {
		context->destroy_requested = true;
		return;
	}

	const RID model_rid = context->model_rid;
	Data::ModelState *model = data->model_owner.get_or_null(model_rid);
	if (model != nullptr && model->active_contexts > 0) {
		model->active_contexts--;
	}

	if (context->native_context != nullptr) {
		llama_free(context->native_context);
		context->native_context = nullptr;
	}

	data->context_owner.free(p_context_handle.rid);
	data->context_destroy_counter++;

	if (model != nullptr && model->unloading && model->active_contexts == 0) {
		if (model->native_model != nullptr) {
			llama_model_free(model->native_model);
			model->native_model = nullptr;
		}
		data->model_owner.free(model_rid);
	}
}

AIBackendResult LlamaBackend::run_job(const AIComputeJob &p_job) {
	AIBackendResult result;
	result.metadata["backend"] = get_backend_name();
	result.metadata["implementation_stage"] = "completion-minimal";
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

	if (p_job.type != AIBackendJobType::COMPLETION) {
		result.code = ERR_UNAVAILABLE;
		result.message = "LlamaBackend embedding execution is not implemented yet.";
		return result;
	}

	Data::ModelState *model = nullptr;
	Data::ContextState *context = nullptr;
	llama_model *native_model = nullptr;
	llama_context *native_context = nullptr;

	{
		MutexLock lock(data->mutex);
		model = data->model_owner.get_or_null(p_job.model_handle.rid);
		context = data->context_owner.get_or_null(p_job.context_handle.rid);

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

		if (context->native_context == nullptr) {
			result.code = ERR_DOES_NOT_EXIST;
			result.message = "LlamaBackend native context is no longer available.";
			return result;
		}

		native_model = model->native_model;
		native_context = context->native_context;
		context->busy = true;
		context->cancel_requested = false;
		data->running_jobs.insert(p_job.job_id, p_job.context_handle.rid);
		if (p_job.stream) {
			data->streamed_job_count++;
		}
	}

	const llama_vocab *vocab = llama_model_get_vocab(native_model);
	if (vocab == nullptr) {
		result.code = ERR_DOES_NOT_EXIST;
		result.message = "LlamaBackend model vocabulary is unavailable.";
	} else if (!llama_model_has_decoder(native_model)) {
		result.code = ERR_UNAVAILABLE;
		result.message = "LlamaBackend completion requires a decoder-capable model.";
	} else {
		const uint64_t started_us = OS::get_singleton()->get_ticks_usec();
		const uint64_t deadline_us = p_job.timeout_ms > 0 ? started_us + static_cast<uint64_t>(p_job.timeout_ms) * 1000 : 0;

		String tokenization_error;
		LocalVector<llama_token> prompt_tokens;
		if (!_tokenize_prompt(vocab, p_job.prompt, prompt_tokens, tokenization_error)) {
			result.code = ERR_INVALID_PARAMETER;
			result.message = tokenization_error;
		} else {
			if (prompt_tokens.is_empty()) {
				const llama_token decoder_start = llama_model_decoder_start_token(native_model);
				if (decoder_start >= 0) {
					prompt_tokens.push_back(decoder_start);
				} else {
					const llama_token bos_token = llama_vocab_bos(vocab);
					if (bos_token >= 0) {
						prompt_tokens.push_back(bos_token);
					}
				}
			}

			const int32_t max_context = static_cast<int32_t>(llama_n_ctx(native_context));
			if (prompt_tokens.is_empty()) {
				result.code = ERR_INVALID_PARAMETER;
				result.message = "LlamaBackend requires a non-empty prompt after tokenization.";
			} else if (max_context > 0 && static_cast<int32_t>(prompt_tokens.size()) >= max_context) {
				result.code = ERR_OUT_OF_MEMORY;
				result.message = vformat("Prompt requires %d tokens but context only supports %d.", static_cast<int32_t>(prompt_tokens.size()), max_context);
			} else {
				llama_sampler *sampler = _build_completion_sampler(p_job);
				if (sampler == nullptr) {
					result.code = ERR_CANT_CREATE;
					result.message = "LlamaBackend failed to create a completion sampler.";
				} else {
					llama_batch batch = llama_batch_init(static_cast<int32_t>(prompt_tokens.size()), 0, 1);
					for (int32_t i = 0; i < static_cast<int32_t>(prompt_tokens.size()); i++) {
						batch.token[i] = prompt_tokens[i];
						batch.pos[i] = i;
						batch.n_seq_id[i] = 1;
						batch.seq_id[i][0] = 0;
						batch.logits[i] = i == static_cast<int32_t>(prompt_tokens.size()) - 1;
					}
					batch.n_tokens = static_cast<int32_t>(prompt_tokens.size());

					int32_t decode_result = llama_decode(native_context, batch);
					llama_batch_free(batch);

					if (decode_result != 0) {
						result.code = _map_decode_error_code(decode_result);
						result.message = _describe_decode_error(decode_result);
						if (decode_result == 2) {
							result.was_cancelled = true;
							result.timed_out = deadline_us > 0 && OS::get_singleton()->get_ticks_usec() >= deadline_us;
							if (result.timed_out) {
								result.code = ERR_TIMEOUT;
							}
						}
					} else {
						String final_text;
						PackedStringArray final_tokens;
						int32_t generated_token_count = 0;
						int32_t current_pos = static_cast<int32_t>(prompt_tokens.size());
						const int32_t max_generation_tokens = MAX(p_job.max_tokens, 0);

						while (generated_token_count < max_generation_tokens) {
							if (deadline_us > 0 && OS::get_singleton()->get_ticks_usec() >= deadline_us) {
								result.code = ERR_TIMEOUT;
								result.message = "LlamaBackend completion exceeded the timeout budget.";
								result.timed_out = true;
								break;
							}

							{
								MutexLock lock(data->mutex);
								Data::ContextState *live_context = data->context_owner.get_or_null(p_job.context_handle.rid);
								if (live_context == nullptr || live_context->cancel_requested) {
									result.code = ERR_SKIP;
									result.message = "LlamaBackend completion was cancelled.";
									result.was_cancelled = true;
									break;
								}
							}

							const llama_token next_token = llama_sampler_sample(sampler, native_context, -1);
							if (next_token < 0 || llama_vocab_is_eog(vocab, next_token)) {
								result.code = OK;
								break;
							}

							const String token_text = _token_to_string(vocab, next_token);
							if (!token_text.is_empty()) {
								final_text += token_text;
								final_tokens.push_back(token_text);
							}

							llama_batch token_batch = llama_batch_init(1, 0, 1);
							token_batch.token[0] = next_token;
							token_batch.pos[0] = current_pos++;
							token_batch.n_seq_id[0] = 1;
							token_batch.seq_id[0][0] = 0;
							token_batch.logits[0] = 1;
							token_batch.n_tokens = 1;

							decode_result = llama_decode(native_context, token_batch);
							llama_batch_free(token_batch);
							generated_token_count++;
							if (decode_result != 0) {
								result.code = _map_decode_error_code(decode_result);
								result.message = _describe_decode_error(decode_result);
								if (decode_result == 2) {
									result.was_cancelled = true;
									result.timed_out = deadline_us > 0 && OS::get_singleton()->get_ticks_usec() >= deadline_us;
									if (result.timed_out) {
										result.code = ERR_TIMEOUT;
									}
								}
								break;
							}

							result.code = OK;
						}

						if (result.code == OK) {
							result.final_text = final_text;
							result.metadata["prompt_token_count"] = static_cast<int64_t>(prompt_tokens.size());
							result.metadata["generated_token_count"] = generated_token_count;
							result.metadata["generated_pieces"] = final_tokens;
							result.metadata["streaming_requested"] = p_job.stream;
							result.metadata["finish_reason"] = generated_token_count >= max_generation_tokens ? String("max_tokens") : String("stop");
						}
					}

					llama_sampler_free(sampler);
				}
			}
		}
	}

	{
		MutexLock lock(data->mutex);
		Data::ContextState *live_context = data->context_owner.get_or_null(p_job.context_handle.rid);
		Data::ModelState *live_model = nullptr;
		if (live_context != nullptr) {
			live_context->busy = false;
			live_model = data->model_owner.get_or_null(live_context->model_rid);
			if (live_context->destroy_requested) {
				if (live_model != nullptr && live_model->active_contexts > 0) {
					live_model->active_contexts--;
				}
				if (live_context->native_context != nullptr) {
					llama_free(live_context->native_context);
					live_context->native_context = nullptr;
				}
				data->context_owner.free(p_job.context_handle.rid);
				data->context_destroy_counter++;
			}
		}
		data->running_jobs.erase(p_job.job_id);

		if (live_model != nullptr && live_model->unloading && live_model->active_contexts == 0) {
			if (live_model->native_model != nullptr) {
				llama_model_free(live_model->native_model);
				live_model->native_model = nullptr;
			}
			data->model_owner.free(p_job.model_handle.rid);
		}
	}

	if (result.message.is_empty() && result.code == OK) {
		result.message = "LlamaBackend completion finished.";
	}
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
	stats["implementation_stage"] = "completion-minimal";
	stats["loaded_models"] = static_cast<int64_t>(data->model_owner.get_rid_count());
	stats["active_contexts"] = static_cast<int64_t>(data->context_owner.get_rid_count());

	{
		MutexLock lock(data->mutex);
		stats["load_count"] = static_cast<int64_t>(data->load_counter);
		stats["context_create_count"] = static_cast<int64_t>(data->context_counter);
		stats["context_destroy_count"] = static_cast<int64_t>(data->context_destroy_counter);
		stats["running_jobs"] = static_cast<int64_t>(data->running_jobs.size());
		stats["rejected_job_count"] = static_cast<int64_t>(data->rejected_job_count);
		stats["cancel_request_count"] = static_cast<int64_t>(data->cancel_request_count);
		stats["streamed_job_count"] = static_cast<int64_t>(data->streamed_job_count);
		stats["stream_flush_count"] = static_cast<int64_t>(data->stream_flush_count);
		stats["streamed_token_count"] = static_cast<int64_t>(data->streamed_token_count);
	}

	stats["runtime_ready"] = true;
	stats["device"] = "cpu";
	stats["supports_gpu_offload"] = llama_supports_gpu_offload();
	stats["llama_system_info"] = String::utf8(llama_print_system_info());
	return stats;
}
