/**************************************************************************/
/*  ai_runtime_profiler.h                                                 */
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

#include "core/variant/dictionary.h"
#include "modules/woodot_ai/runtime/ai_backend.h"

class AIRuntimeProfiler {
	uint64_t submitted_jobs = 0;
	uint64_t completed_jobs = 0;
	uint64_t failed_jobs = 0;
	uint64_t cancelled_jobs = 0;
	uint64_t completion_jobs = 0;
	uint64_t embedding_jobs = 0;
	uint64_t partial_update_count = 0;
	uint64_t final_update_count = 0;
	uint64_t mailbox_poll_calls = 0;
	uint64_t mailbox_drained_updates = 0;
	uint64_t total_queue_wait_us = 0;
	uint64_t total_exec_time_us = 0;
	uint64_t total_poll_time_us = 0;
	uint64_t peak_queue_wait_us = 0;
	uint64_t peak_exec_time_us = 0;
	uint64_t peak_poll_time_us = 0;
	uint64_t peak_mailbox_batch = 0;

public:
	void record_submission(AIBackendJobType p_type);
	void record_delivery(const AIBackendResult &p_result);
	void record_completion(const AIBackendResult &p_result);
	void record_poll(int32_t p_drained_count, uint64_t p_poll_time_us);
	Dictionary get_stats() const;
};
