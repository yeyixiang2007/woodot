/**************************************************************************/
/*  ai_inference_queue.h                                                  */
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
#include "core/templates/list.h"
#include "modules/woodot_ai/runtime/ai_backend.h"
#include "modules/woodot_ai/runtime/ai_task_handle.h"

class AIInferenceQueue {
public:
	struct QueuedTask {
		uint64_t route_id = 0;
		uint64_t enqueue_tick_us = 0;
		AIBackend *backend = nullptr;
		AIComputeJob job;
		Ref<AITaskHandle> handle;
	};

private:
	mutable Mutex mutex;
	List<QueuedTask> pending_tasks;
	uint64_t enqueued_jobs = 0;
	uint64_t dequeued_jobs = 0;
	uint64_t cancelled_jobs = 0;
	uint64_t peak_depth = 0;

public:
	void enqueue(const QueuedTask &p_task);
	bool pop_next(QueuedTask &r_task);
	bool cancel_queued(uint64_t p_job_id, QueuedTask *r_task = nullptr);
	int32_t get_pending_count() const;
	Dictionary get_stats() const;
};
