/**************************************************************************/
/*  ai_task_handle.h                                                      */
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

#include "core/object/ref_counted.h"
#include "core/os/mutex.h"
#include "core/os/thread_safe.h"
#include "core/variant/type_info.h"
#include "modules/woodot_ai/runtime/ai_backend.h"

class AITaskHandle : public RefCounted {
	GDCLASS(AITaskHandle, RefCounted);
	_THREAD_SAFE_CLASS_

public:
	enum Status {
		STATUS_QUEUED = 0,
		STATUS_RUNNING,
		STATUS_STREAMING,
		STATUS_COMPLETED,
		STATUS_FAILED,
		STATUS_CANCELLED,
	};

	enum CancelReason {
		CANCEL_REASON_NONE = 0,
		CANCEL_REASON_USER_REQUEST,
		CANCEL_REASON_TIMEOUT,
		CANCEL_REASON_SYSTEM_INTERRUPTED,
	};

private:
	mutable Mutex mutex;

	uint64_t job_id = 0;
	Status status = STATUS_QUEUED;
	CancelReason cancel_reason = CANCEL_REASON_NONE;
	bool cancel_requested = false;

	Error error_code = OK;
	String error_message;
	PackedStringArray partial_tokens;
	String final_text;
	PackedFloat32Array embedding;
	uint64_t queue_wait_us = 0;
	uint64_t exec_time_us = 0;
	Dictionary metadata;

	bool _can_transition(Status p_from, Status p_to) const;
	void _emit_state_changed(Status p_old_status, Status p_new_status);

protected:
	static void _bind_methods();

public:
	void set_job_id(uint64_t p_job_id);
	uint64_t get_job_id() const;

	Status get_status() const;
	String get_status_name() const;
	bool is_terminal() const;
	bool is_finished_successfully() const;

	CancelReason get_cancel_reason() const;
	bool request_cancel();
	bool is_cancel_requested() const;

	Error get_error_code() const;
	String get_error_message() const;
	PackedStringArray get_partial_tokens() const;
	String get_final_text() const;
	PackedFloat32Array get_embedding() const;
	uint64_t get_queue_wait_us() const;
	uint64_t get_exec_time_us() const;
	Dictionary get_metadata() const;
	Dictionary get_result_snapshot() const;

	bool mark_running();
	bool append_partial_tokens(const PackedStringArray &p_tokens);
	bool complete(const AIBackendResult &p_result);
	bool fail(Error p_error_code, const String &p_error_message, const Dictionary &p_metadata = Dictionary());
	bool cancel(CancelReason p_reason, const String &p_message = String(), const Dictionary &p_metadata = Dictionary());
	bool apply_backend_result(const AIBackendResult &p_result);

	AITaskHandle();
};

VARIANT_ENUM_CAST(AITaskHandle::Status);
VARIANT_ENUM_CAST(AITaskHandle::CancelReason);
