/**************************************************************************/
/*  ai_task_scheduler.h                                                   */
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

#include "core/os/thread_safe.h"
#include "core/templates/hash_map.h"
#include "modules/woodot_ai/runtime/ai_backend.h"
#include "modules/woodot_ai/runtime/ai_inference_queue.h"
#include "modules/woodot_ai/runtime/ai_requests.h"
#include "modules/woodot_ai/runtime/ai_result_mailbox.h"
#include "modules/woodot_ai/runtime/ai_runtime_profiler.h"
#include "modules/woodot_ai/runtime/ai_task_handle.h"
#include "modules/woodot_ai/runtime/ai_token_stream.h"

class AITaskScheduler {
	struct TaskRecord {
		AIBackend *backend = nullptr;
		AIBackendContextHandle context_handle;
		Ref<AITaskHandle> handle;
		bool stream_requested = false;
		AITokenStream token_stream;
	};

	mutable Mutex mutex;
	HashMap<uint64_t, TaskRecord> running_tasks;
	AIInferenceQueue inference_queue;
	AIResultMailbox result_mailbox;
	uint64_t next_job_id = 1;
	uint64_t submitted_jobs = 0;
	uint64_t finished_jobs = 0;
	uint64_t cancelled_jobs = 0;
	uint64_t failed_jobs = 0;
	uint64_t timeout_jobs = 0;
	uint64_t completion_jobs = 0;
	uint64_t embedding_jobs = 0;
	bool auto_process_queue = true;
	AIRuntimeProfiler profiler;

	AIComputeJob _build_completion_job(uint64_t p_job_id, const AIBackendModelHandle &p_model_handle, const AIBackendContextHandle &p_context_handle, const RID &p_public_model_rid, const Ref<AICompletionRequest> &p_request) const;
	AIComputeJob _build_embedding_job(uint64_t p_job_id, const AIBackendModelHandle &p_model_handle, const AIBackendContextHandle &p_context_handle, const RID &p_public_model_rid, const Ref<AIEmbeddingRequest> &p_request) const;
	void _record_finish_locked(const Ref<AITaskHandle> &p_handle);
	void _enqueue_task(AIBackend *p_backend, const AIBackendContextHandle &p_context_handle, const Ref<AITaskHandle> &p_handle, const AIComputeJob &p_job);
	void _process_queued_jobs();
	void _apply_delivery(const AIResultMailbox::Delivery &p_delivery);

public:
	Ref<AITaskHandle> submit_completion(AIBackend *p_backend, const AIBackendModelHandle &p_model_handle, const RID &p_public_model_rid, const Ref<AICompletionRequest> &p_request);
	Ref<AITaskHandle> submit_embedding(AIBackend *p_backend, const AIBackendModelHandle &p_model_handle, const RID &p_public_model_rid, const Ref<AIEmbeddingRequest> &p_request);
	void cancel_task(const Ref<AITaskHandle> &p_task_handle);
	void set_auto_process_queue_enabled(bool p_enabled);
	bool is_auto_process_queue_enabled() const;
	int32_t process_pending();
	int32_t poll_completed(int32_t p_max_count = -1);
	Dictionary get_stats() const;
};
