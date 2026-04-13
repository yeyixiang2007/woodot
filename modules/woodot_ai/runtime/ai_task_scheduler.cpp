/**************************************************************************/
/*  ai_task_scheduler.cpp                                                 */
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

#include "modules/woodot_ai/runtime/ai_task_scheduler.h"

#include "core/os/os.h"

void AITaskScheduler::_enqueue_task(AIBackend *p_backend, const AIBackendContextHandle &p_context_handle, const Ref<AITaskHandle> &p_handle, const AIComputeJob &p_job) {
	AIInferenceQueue::QueuedTask queued_task;
	queued_task.route_id = p_job.public_model_rid.get_id();
	queued_task.enqueue_tick_us = OS::get_singleton()->get_ticks_usec();
	queued_task.backend = p_backend;
	queued_task.job = p_job;
	queued_task.handle = p_handle;

	{
		MutexLock lock(mutex);
		TaskRecord record;
		record.backend = p_backend;
		record.context_handle = p_context_handle;
		record.handle = p_handle;
		record.stream_requested = p_job.stream;
		AITokenStream::Config token_stream_config;
		token_stream_config.enabled = p_job.stream;
		record.token_stream.configure(token_stream_config);
		running_tasks.insert(p_job.job_id, record);
	}

	inference_queue.enqueue(queued_task);
}

void AITaskScheduler::_process_queued_jobs() {
	AIInferenceQueue::QueuedTask queued_task;
	while (inference_queue.pop_next(queued_task)) {
		if (queued_task.handle.is_null()) {
			continue;
		}

		const uint64_t dequeue_tick_us = OS::get_singleton()->get_ticks_usec();
		const uint64_t measured_queue_wait_us = dequeue_tick_us >= queued_task.enqueue_tick_us ? dequeue_tick_us - queued_task.enqueue_tick_us : 0;

		if (queued_task.handle->is_cancel_requested()) {
			AIResultMailbox::Delivery cancelled_delivery;
			cancelled_delivery.handle = queued_task.handle;
			cancelled_delivery.backend = queued_task.backend;
			cancelled_delivery.context_handle = queued_task.job.context_handle;
			cancelled_delivery.release_context = true;
			cancelled_delivery.result.code = ERR_SKIP;
			cancelled_delivery.result.message = "Task cancelled before backend execution started.";
			cancelled_delivery.result.was_cancelled = true;
			cancelled_delivery.result.queue_wait_us = measured_queue_wait_us;
			result_mailbox.push(cancelled_delivery);
			continue;
		}

		queued_task.handle->mark_running();

		AIResultMailbox::Delivery delivery;
		delivery.handle = queued_task.handle;
		delivery.backend = queued_task.backend;
		delivery.context_handle = queued_task.job.context_handle;
		delivery.release_context = true;
		if (queued_task.backend == nullptr) {
			delivery.result.code = ERR_UNAVAILABLE;
			delivery.result.message = "Queued task does not reference a valid backend.";
			delivery.result.queue_wait_us = measured_queue_wait_us;
		} else {
			const uint64_t exec_begin_us = OS::get_singleton()->get_ticks_usec();
			delivery.result = queued_task.backend->run_job(queued_task.job);
			const uint64_t exec_end_us = OS::get_singleton()->get_ticks_usec();
			if (delivery.result.queue_wait_us == 0) {
				delivery.result.queue_wait_us = measured_queue_wait_us;
			}
			if (delivery.result.exec_time_us == 0 && exec_end_us >= exec_begin_us) {
				delivery.result.exec_time_us = exec_end_us - exec_begin_us;
			}
		}

		if (queued_task.job.timeout_ms > 0) {
			const uint64_t total_runtime_us = delivery.result.queue_wait_us + delivery.result.exec_time_us;
			const uint64_t timeout_us = static_cast<uint64_t>(queued_task.job.timeout_ms) * 1000;
			if (total_runtime_us >= timeout_us && !delivery.result.was_cancelled && !delivery.result.timed_out) {
				delivery.result.code = ERR_TIMEOUT;
				delivery.result.message = delivery.result.message.is_empty() ? String("Task exceeded timeout budget.") : delivery.result.message;
				delivery.result.was_cancelled = false;
				delivery.result.timed_out = true;
				delivery.result.metadata["timeout_ms"] = queued_task.job.timeout_ms;
				delivery.result.metadata["timeout_us"] = static_cast<int64_t>(timeout_us);
				delivery.result.metadata["observed_runtime_us"] = static_cast<int64_t>(total_runtime_us);
			}
		}

		result_mailbox.push(delivery);
	}
}

void AITaskScheduler::_apply_delivery(const AIResultMailbox::Delivery &p_delivery) {
	const uint64_t now_us = OS::get_singleton()->get_ticks_usec();
	const uint64_t job_id = p_delivery.handle.is_valid() ? p_delivery.handle->get_job_id() : 0;
	bool stream_requested = false;
	bool emit_buffered_partial = false;
	bool apply_original_result = true;
	PackedStringArray buffered_tokens;

	{
		MutexLock lock(mutex);
		HashMap<uint64_t, TaskRecord>::Iterator task = running_tasks.find(job_id);
		if (task) {
			stream_requested = task->value.stream_requested;
			if (stream_requested) {
				if (p_delivery.result.is_partial) {
					task->value.token_stream.push_tokens(p_delivery.result.partial_tokens, now_us);
					if (task->value.token_stream.should_flush(now_us)) {
						buffered_tokens = task->value.token_stream.flush(now_us);
						emit_buffered_partial = !buffered_tokens.is_empty();
					}
					apply_original_result = false;
				} else if (task->value.token_stream.has_pending_tokens()) {
					buffered_tokens = task->value.token_stream.finalize(now_us);
					emit_buffered_partial = !buffered_tokens.is_empty();
				}
			}
		}
	}

	profiler.record_delivery(p_delivery.result);

	if (emit_buffered_partial && p_delivery.handle.is_valid()) {
		AIBackendResult partial_result = p_delivery.result;
		partial_result.code = OK;
		partial_result.message = String();
		partial_result.is_partial = true;
		partial_result.is_final = false;
		partial_result.was_cancelled = false;
		partial_result.partial_tokens = buffered_tokens;
		p_delivery.handle->apply_backend_result(partial_result);
	}

	if (p_delivery.handle.is_valid()) {
		if (apply_original_result) {
			p_delivery.handle->apply_backend_result(p_delivery.result);
		}
	}

	if (p_delivery.result.is_partial) {
		return;
	}

	if (p_delivery.release_context && p_delivery.backend != nullptr && p_delivery.context_handle.is_valid()) {
		p_delivery.backend->destroy_context(p_delivery.context_handle);
	}

	MutexLock lock(mutex);
	if (job_id != 0) {
		running_tasks.erase(job_id);
	}
	_record_finish_locked(p_delivery.handle);
	if (p_delivery.handle.is_valid() && p_delivery.handle->get_cancel_reason() == AITaskHandle::CANCEL_REASON_TIMEOUT) {
		timeout_jobs++;
	}
	profiler.record_completion(p_delivery.result);
}

AIComputeJob AITaskScheduler::_build_completion_job(uint64_t p_job_id, const AIBackendModelHandle &p_model_handle, const AIBackendContextHandle &p_context_handle, const RID &p_public_model_rid, const Ref<AICompletionRequest> &p_request) const {
	AIComputeJob job;
	job.job_id = p_job_id;
	job.type = AIBackendJobType::COMPLETION;
	job.model_handle = p_model_handle;
	job.context_handle = p_context_handle;
	job.public_model_rid = p_public_model_rid;
	job.prompt = p_request->get_prompt();
	job.max_tokens = p_request->get_max_tokens();
	job.temperature = p_request->get_temperature();
	job.top_p = p_request->get_top_p();
	job.top_k = p_request->get_top_k();
	job.stream = p_request->is_streaming();
	job.timeout_ms = p_request->get_timeout_ms();
	job.priority = p_request->get_priority();
	job.caller_tag = p_request->get_caller_tag();
	job.metadata = p_request->get_metadata();
	return job;
}

AIComputeJob AITaskScheduler::_build_embedding_job(uint64_t p_job_id, const AIBackendModelHandle &p_model_handle, const AIBackendContextHandle &p_context_handle, const RID &p_public_model_rid, const Ref<AIEmbeddingRequest> &p_request) const {
	AIComputeJob job;
	job.job_id = p_job_id;
	job.type = AIBackendJobType::EMBEDDING;
	job.model_handle = p_model_handle;
	job.context_handle = p_context_handle;
	job.public_model_rid = p_public_model_rid;
	job.embedding_inputs = p_request->get_inputs();
	job.normalize = p_request->is_normalized();
	job.timeout_ms = p_request->get_timeout_ms();
	job.priority = p_request->get_priority();
	job.caller_tag = p_request->get_caller_tag();
	job.metadata = p_request->get_metadata();
	return job;
}

void AITaskScheduler::_record_finish_locked(const Ref<AITaskHandle> &p_handle) {
	finished_jobs++;
	if (p_handle.is_valid()) {
		if (p_handle->get_status() == AITaskHandle::STATUS_CANCELLED) {
			cancelled_jobs++;
		} else if (p_handle->get_status() == AITaskHandle::STATUS_FAILED) {
			failed_jobs++;
		}
	}
}

Ref<AITaskHandle> AITaskScheduler::submit_completion(AIBackend *p_backend, const AIBackendModelHandle &p_model_handle, const RID &p_public_model_rid, const Ref<AICompletionRequest> &p_request) {
	Ref<AITaskHandle> handle;
	handle.instantiate();

	if (p_backend == nullptr) {
		handle->fail(ERR_UNAVAILABLE, "AITaskScheduler requires a valid backend.");
		return handle;
	}

	if (p_request.is_null()) {
		handle->fail(ERR_INVALID_PARAMETER, "AITaskScheduler requires a valid AICompletionRequest.");
		return handle;
	}

	const AIBackendContextCreateResult context_result = p_backend->create_context(p_model_handle);
	if (!context_result.is_ok()) {
		handle->fail(context_result.code, context_result.message, context_result.details);
		return handle;
	}

	uint64_t job_id = 0;
	{
		MutexLock lock(mutex);
		job_id = next_job_id++;
		submitted_jobs++;
		completion_jobs++;
		profiler.record_submission(AIBackendJobType::COMPLETION);
		handle->set_job_id(job_id);
	}

	const AIComputeJob job = _build_completion_job(job_id, p_model_handle, context_result.context_handle, p_public_model_rid, p_request);
	_enqueue_task(p_backend, context_result.context_handle, handle, job);
	if (auto_process_queue) {
		_process_queued_jobs();
	}
	return handle;
}

Ref<AITaskHandle> AITaskScheduler::submit_embedding(AIBackend *p_backend, const AIBackendModelHandle &p_model_handle, const RID &p_public_model_rid, const Ref<AIEmbeddingRequest> &p_request) {
	Ref<AITaskHandle> handle;
	handle.instantiate();

	if (p_backend == nullptr) {
		handle->fail(ERR_UNAVAILABLE, "AITaskScheduler requires a valid backend.");
		return handle;
	}

	if (p_request.is_null()) {
		handle->fail(ERR_INVALID_PARAMETER, "AITaskScheduler requires a valid AIEmbeddingRequest.");
		return handle;
	}

	const AIBackendContextCreateResult context_result = p_backend->create_context(p_model_handle);
	if (!context_result.is_ok()) {
		handle->fail(context_result.code, context_result.message, context_result.details);
		return handle;
	}

	uint64_t job_id = 0;
	{
		MutexLock lock(mutex);
		job_id = next_job_id++;
		submitted_jobs++;
		embedding_jobs++;
		profiler.record_submission(AIBackendJobType::EMBEDDING);
		handle->set_job_id(job_id);
	}

	const AIComputeJob job = _build_embedding_job(job_id, p_model_handle, context_result.context_handle, p_public_model_rid, p_request);
	_enqueue_task(p_backend, context_result.context_handle, handle, job);
	if (auto_process_queue) {
		_process_queued_jobs();
	}
	return handle;
}

void AITaskScheduler::cancel_task(const Ref<AITaskHandle> &p_task_handle) {
	if (p_task_handle.is_null()) {
		return;
	}

	if (!p_task_handle->request_cancel()) {
		return;
	}

	AIInferenceQueue::QueuedTask queued_task;
	if (inference_queue.cancel_queued(p_task_handle->get_job_id(), &queued_task)) {
		AIResultMailbox::Delivery delivery;
		delivery.handle = queued_task.handle;
		delivery.backend = queued_task.backend;
		delivery.context_handle = queued_task.job.context_handle;
		delivery.release_context = true;
		delivery.result.code = ERR_SKIP;
		delivery.result.message = "Task cancelled while waiting in inference queue.";
		delivery.result.was_cancelled = true;
		result_mailbox.push(delivery);
		return;
	}

	MutexLock lock(mutex);
	HashMap<uint64_t, TaskRecord>::Iterator task = running_tasks.find(p_task_handle->get_job_id());
	if (task && task->value.backend != nullptr) {
		task->value.backend->cancel_job(task->key);
	}
}

void AITaskScheduler::set_auto_process_queue_enabled(bool p_enabled) {
	MutexLock lock(mutex);
	auto_process_queue = p_enabled;
}

bool AITaskScheduler::is_auto_process_queue_enabled() const {
	MutexLock lock(mutex);
	return auto_process_queue;
}

int32_t AITaskScheduler::process_pending() {
	const int32_t pending_before = inference_queue.get_pending_count();
	_process_queued_jobs();
	return pending_before;
}

int32_t AITaskScheduler::poll_completed(int32_t p_max_count) {
	const uint64_t poll_begin_us = OS::get_singleton()->get_ticks_usec();
	List<AIResultMailbox::Delivery> deliveries;
	const int32_t drained = result_mailbox.drain(deliveries, p_max_count);
	for (List<AIResultMailbox::Delivery>::Element *element = deliveries.front(); element != nullptr; element = element->next()) {
		_apply_delivery(element->get());
	}
	const uint64_t poll_end_us = OS::get_singleton()->get_ticks_usec();
	profiler.record_poll(drained, poll_end_us >= poll_begin_us ? poll_end_us - poll_begin_us : 0);
	return drained;
}

Dictionary AITaskScheduler::get_stats() const {
	Dictionary stats;
	MutexLock lock(mutex);
	stats["submitted_jobs"] = static_cast<int64_t>(submitted_jobs);
	stats["finished_jobs"] = static_cast<int64_t>(finished_jobs);
	stats["running_jobs"] = static_cast<int64_t>(running_tasks.size());
	stats["cancelled_jobs"] = static_cast<int64_t>(cancelled_jobs);
	stats["failed_jobs"] = static_cast<int64_t>(failed_jobs);
	stats["timeout_jobs"] = static_cast<int64_t>(timeout_jobs);
	stats["completion_jobs"] = static_cast<int64_t>(completion_jobs);
	stats["embedding_jobs"] = static_cast<int64_t>(embedding_jobs);
	stats["auto_process_queue"] = auto_process_queue;
	stats["implementation_stage"] = "queue_mailbox_skeleton";
	stats["inference_queue"] = inference_queue.get_stats();
	stats["result_mailbox"] = result_mailbox.get_stats();
	stats["profiling"] = profiler.get_stats();
	return stats;
}
