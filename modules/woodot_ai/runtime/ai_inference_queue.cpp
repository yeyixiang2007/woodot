/**************************************************************************/
/*  ai_inference_queue.cpp                                                */
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

#include "modules/woodot_ai/runtime/ai_inference_queue.h"

void AIInferenceQueue::enqueue(const QueuedTask &p_task) {
	MutexLock lock(mutex);
	pending_tasks.push_back(p_task);
	enqueued_jobs++;
	const uint64_t pending_count = static_cast<uint64_t>(pending_tasks.size());
	if (pending_count > peak_depth) {
		peak_depth = pending_count;
	}
}

bool AIInferenceQueue::pop_next(QueuedTask &r_task) {
	MutexLock lock(mutex);
	if (pending_tasks.is_empty()) {
		return false;
	}

	r_task = pending_tasks.front()->get();
	pending_tasks.pop_front();
	dequeued_jobs++;
	return true;
}

bool AIInferenceQueue::cancel_queued(uint64_t p_job_id, QueuedTask *r_task) {
	MutexLock lock(mutex);
	for (List<QueuedTask>::Element *element = pending_tasks.front(); element != nullptr; element = element->next()) {
		if (element->get().job.job_id != p_job_id) {
			continue;
		}

		if (r_task != nullptr) {
			*r_task = element->get();
		}
		pending_tasks.erase(element);
		cancelled_jobs++;
		return true;
	}

	return false;
}

int32_t AIInferenceQueue::get_pending_count() const {
	MutexLock lock(mutex);
	return pending_tasks.size();
}

Dictionary AIInferenceQueue::get_stats() const {
	Dictionary stats;
	MutexLock lock(mutex);
	stats["pending_jobs"] = static_cast<int64_t>(pending_tasks.size());
	stats["enqueued_jobs"] = static_cast<int64_t>(enqueued_jobs);
	stats["dequeued_jobs"] = static_cast<int64_t>(dequeued_jobs);
	stats["cancelled_jobs"] = static_cast<int64_t>(cancelled_jobs);
	stats["peak_depth"] = static_cast<int64_t>(peak_depth);
	return stats;
}
