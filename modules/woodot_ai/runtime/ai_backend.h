/**************************************************************************/
/*  ai_backend.h                                                          */
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

#include "core/error/error_list.h"
#include "core/object/ref_counted.h"
#include "core/string/string_name.h"
#include "core/templates/rid.h"
#include "core/variant/dictionary.h"
#include "core/variant/variant.h"

#include <cstdint>

class AIModelResource;

enum class AIBackendJobType {
	COMPLETION,
	EMBEDDING,
};

struct AIBackendCapabilities {
	bool supports_completion = true;
	bool supports_embedding = false;
	bool supports_streaming = false;
	bool supports_cancellation = false;
	bool supports_context_reuse = true;
	bool supports_cpu_execution = true;
	bool supports_gpu_execution = false;
	uint32_t preferred_parallel_jobs = 1;
	PackedStringArray supported_devices;
	Dictionary metadata;
};

struct AIBackendModelHandle {
	RID rid;
	StringName backend_name;
	Dictionary metadata;

	_FORCE_INLINE_ bool is_valid() const { return rid.is_valid(); }
};

struct AIBackendContextHandle {
	RID rid;
	RID model_rid;
	StringName backend_name;
	bool exclusive_decode = true;
	Dictionary metadata;

	_FORCE_INLINE_ bool is_valid() const { return rid.is_valid(); }
};

struct AIBackendValidationResult {
	Error code = OK;
	String message;
	Dictionary details;

	_FORCE_INLINE_ bool is_ok() const { return code == OK; }
};

struct AIBackendModelLoadResult {
	Error code = OK;
	String message;
	AIBackendModelHandle model_handle;
	Dictionary details;

	_FORCE_INLINE_ bool is_ok() const { return code == OK && model_handle.is_valid(); }
};

struct AIBackendContextCreateResult {
	Error code = OK;
	String message;
	AIBackendContextHandle context_handle;
	Dictionary details;

	_FORCE_INLINE_ bool is_ok() const { return code == OK && context_handle.is_valid(); }
};

struct AIComputeJob {
	uint64_t job_id = 0;
	AIBackendJobType type = AIBackendJobType::COMPLETION;
	AIBackendModelHandle model_handle;
	AIBackendContextHandle context_handle;
	RID public_model_rid;

	String prompt;
	PackedStringArray embedding_inputs;
	bool normalize = false;

	int32_t max_tokens = 256;
	float temperature = 0.8f;
	float top_p = 0.95f;
	int32_t top_k = 40;
	bool stream = false;
	bool allow_fallback = true;
	int32_t timeout_ms = 0;
	int32_t priority = 0;
	String caller_tag;
	Dictionary metadata;
};

struct AIBackendResult {
	Error code = OK;
	String message;
	PackedStringArray partial_tokens;
	String final_text;
	PackedFloat32Array embedding;
	uint64_t queue_wait_us = 0;
	uint64_t exec_time_us = 0;
	bool is_partial = false;
	bool is_final = true;
	bool was_cancelled = false;
	Dictionary metadata;

	_FORCE_INLINE_ bool is_ok() const { return code == OK; }
};

class AIBackend {
public:
	virtual ~AIBackend() = default;

	virtual StringName get_backend_name() const = 0;
	virtual AIBackendCapabilities get_capabilities() const = 0;

	virtual AIBackendValidationResult validate_model(const Ref<AIModelResource> &p_model) const = 0;
	virtual AIBackendModelLoadResult load_model(const Ref<AIModelResource> &p_model) = 0;
	virtual void unload_model(const AIBackendModelHandle &p_model_handle) = 0;

	virtual AIBackendContextCreateResult create_context(const AIBackendModelHandle &p_model_handle) = 0;
	virtual void destroy_context(const AIBackendContextHandle &p_context_handle) = 0;

	virtual AIBackendResult run_job(const AIComputeJob &p_job) = 0;
	virtual bool cancel_job(uint64_t p_job_id) = 0;
	virtual Dictionary get_runtime_stats() const = 0;
};
