/**************************************************************************/
/*  ai_result_mailbox.cpp                                                 */
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

#include "modules/woodot_ai/runtime/ai_result_mailbox.h"

void AIResultMailbox::push(const Delivery &p_delivery) {
	MutexLock lock(mutex);
	deliveries.push_back(p_delivery);
	pushed_updates++;
	const uint64_t pending_count = static_cast<uint64_t>(deliveries.size());
	if (pending_count > peak_pending) {
		peak_pending = pending_count;
	}
}

int32_t AIResultMailbox::drain(List<Delivery> &r_deliveries, int32_t p_max_count) {
	MutexLock lock(mutex);

	int32_t drained = 0;
	while (!deliveries.is_empty() && (p_max_count < 0 || drained < p_max_count)) {
		r_deliveries.push_back(deliveries.front()->get());
		deliveries.pop_front();
		drained++;
	}

	drained_updates += static_cast<uint64_t>(drained);
	return drained;
}

int32_t AIResultMailbox::get_pending_count() const {
	MutexLock lock(mutex);
	return deliveries.size();
}

Dictionary AIResultMailbox::get_stats() const {
	Dictionary stats;
	MutexLock lock(mutex);
	stats["pending_updates"] = static_cast<int64_t>(deliveries.size());
	stats["pushed_updates"] = static_cast<int64_t>(pushed_updates);
	stats["drained_updates"] = static_cast<int64_t>(drained_updates);
	stats["peak_pending"] = static_cast<int64_t>(peak_pending);
	return stats;
}
