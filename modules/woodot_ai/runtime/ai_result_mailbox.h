/**************************************************************************/
/*  ai_result_mailbox.h                                                   */
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

class AIResultMailbox {
public:
	struct Delivery {
		Ref<AITaskHandle> handle;
		AIBackend *backend = nullptr;
		AIBackendContextHandle context_handle;
		AIBackendResult result;
		bool release_context = false;
	};

private:
	mutable Mutex mutex;
	List<Delivery> deliveries;
	uint64_t pushed_updates = 0;
	uint64_t drained_updates = 0;
	uint64_t peak_pending = 0;

public:
	void push(const Delivery &p_delivery);
	int32_t drain(List<Delivery> &r_deliveries, int32_t p_max_count = -1);
	int32_t get_pending_count() const;
	Dictionary get_stats() const;
};
