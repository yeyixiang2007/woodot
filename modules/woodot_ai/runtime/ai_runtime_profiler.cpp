/**************************************************************************/
/*  ai_runtime_profiler.cpp                                               */
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

#include "modules/woodot_ai/runtime/ai_runtime_profiler.h"

void AIRuntimeProfiler::record_submission(AIBackendJobType p_type) {
	submitted_jobs++;
	if (p_type == AIBackendJobType::COMPLETION) {
		completion_jobs++;
	} else {
		embedding_jobs++;
	}
}

void AIRuntimeProfiler::record_delivery(const AIBackendResult &p_result) {
	if (p_result.is_partial) {
		partial_update_count++;
	} else {
		final_update_count++;
	}
}

void AIRuntimeProfiler::record_completion(const AIBackendResult &p_result) {
	completed_jobs++;
	total_queue_wait_us += p_result.queue_wait_us;
	total_exec_time_us += p_result.exec_time_us;
	if (p_result.queue_wait_us > peak_queue_wait_us) {
		peak_queue_wait_us = p_result.queue_wait_us;
	}
	if (p_result.exec_time_us > peak_exec_time_us) {
		peak_exec_time_us = p_result.exec_time_us;
	}

	if (p_result.was_cancelled) {
		cancelled_jobs++;
	} else if (p_result.code != OK) {
		failed_jobs++;
	}
}

void AIRuntimeProfiler::record_poll(int32_t p_drained_count, uint64_t p_poll_time_us) {
	mailbox_poll_calls++;
	const uint64_t drained_count = p_drained_count > 0 ? static_cast<uint64_t>(p_drained_count) : 0;
	mailbox_drained_updates += drained_count;
	total_poll_time_us += p_poll_time_us;
	if (drained_count > peak_mailbox_batch) {
		peak_mailbox_batch = drained_count;
	}
	if (p_poll_time_us > peak_poll_time_us) {
		peak_poll_time_us = p_poll_time_us;
	}
}

Dictionary AIRuntimeProfiler::get_stats() const {
	Dictionary stats;
	stats["submitted_jobs"] = static_cast<int64_t>(submitted_jobs);
	stats["completed_jobs"] = static_cast<int64_t>(completed_jobs);
	stats["failed_jobs"] = static_cast<int64_t>(failed_jobs);
	stats["cancelled_jobs"] = static_cast<int64_t>(cancelled_jobs);
	stats["completion_jobs"] = static_cast<int64_t>(completion_jobs);
	stats["embedding_jobs"] = static_cast<int64_t>(embedding_jobs);
	stats["partial_update_count"] = static_cast<int64_t>(partial_update_count);
	stats["final_update_count"] = static_cast<int64_t>(final_update_count);
	stats["mailbox_poll_calls"] = static_cast<int64_t>(mailbox_poll_calls);
	stats["mailbox_drained_updates"] = static_cast<int64_t>(mailbox_drained_updates);
	stats["total_queue_wait_us"] = static_cast<int64_t>(total_queue_wait_us);
	stats["total_exec_time_us"] = static_cast<int64_t>(total_exec_time_us);
	stats["total_poll_time_us"] = static_cast<int64_t>(total_poll_time_us);
	stats["peak_queue_wait_us"] = static_cast<int64_t>(peak_queue_wait_us);
	stats["peak_exec_time_us"] = static_cast<int64_t>(peak_exec_time_us);
	stats["peak_poll_time_us"] = static_cast<int64_t>(peak_poll_time_us);
	stats["peak_mailbox_batch"] = static_cast<int64_t>(peak_mailbox_batch);
	stats["implementation_stage"] = "runtime_stats";
	return stats;
}
