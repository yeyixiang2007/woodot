/**************************************************************************/
/*  ai_task_handle.cpp                                                    */
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

#include "modules/woodot_ai/runtime/ai_task_handle.h"

#include "core/object/class_db.h"

void AITaskHandle::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_job_id", "job_id"), &AITaskHandle::set_job_id);
	ClassDB::bind_method(D_METHOD("get_job_id"), &AITaskHandle::get_job_id);
	ClassDB::bind_method(D_METHOD("get_status"), &AITaskHandle::get_status);
	ClassDB::bind_method(D_METHOD("get_status_name"), &AITaskHandle::get_status_name);
	ClassDB::bind_method(D_METHOD("is_terminal"), &AITaskHandle::is_terminal);
	ClassDB::bind_method(D_METHOD("is_finished_successfully"), &AITaskHandle::is_finished_successfully);
	ClassDB::bind_method(D_METHOD("get_cancel_reason"), &AITaskHandle::get_cancel_reason);
	ClassDB::bind_method(D_METHOD("request_cancel"), &AITaskHandle::request_cancel);
	ClassDB::bind_method(D_METHOD("is_cancel_requested"), &AITaskHandle::is_cancel_requested);
	ClassDB::bind_method(D_METHOD("get_error_code"), &AITaskHandle::get_error_code);
	ClassDB::bind_method(D_METHOD("get_error_message"), &AITaskHandle::get_error_message);
	ClassDB::bind_method(D_METHOD("get_partial_tokens"), &AITaskHandle::get_partial_tokens);
	ClassDB::bind_method(D_METHOD("get_final_text"), &AITaskHandle::get_final_text);
	ClassDB::bind_method(D_METHOD("get_embedding"), &AITaskHandle::get_embedding);
	ClassDB::bind_method(D_METHOD("get_queue_wait_us"), &AITaskHandle::get_queue_wait_us);
	ClassDB::bind_method(D_METHOD("get_exec_time_us"), &AITaskHandle::get_exec_time_us);
	ClassDB::bind_method(D_METHOD("get_metadata"), &AITaskHandle::get_metadata);
	ClassDB::bind_method(D_METHOD("get_result_snapshot"), &AITaskHandle::get_result_snapshot);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "job_id"), "set_job_id", "get_job_id");

	ADD_SIGNAL(MethodInfo("status_changed", PropertyInfo(Variant::INT, "old_status"), PropertyInfo(Variant::INT, "new_status")));
	ADD_SIGNAL(MethodInfo("cancel_requested"));
	ADD_SIGNAL(MethodInfo("completed"));
	ADD_SIGNAL(MethodInfo("failed", PropertyInfo(Variant::INT, "error_code"), PropertyInfo(Variant::STRING, "error_message")));
	ADD_SIGNAL(MethodInfo("cancelled", PropertyInfo(Variant::INT, "cancel_reason"), PropertyInfo(Variant::STRING, "message")));
	ADD_SIGNAL(MethodInfo("partial_result", PropertyInfo(Variant::PACKED_STRING_ARRAY, "tokens")));

	BIND_ENUM_CONSTANT(STATUS_QUEUED);
	BIND_ENUM_CONSTANT(STATUS_RUNNING);
	BIND_ENUM_CONSTANT(STATUS_STREAMING);
	BIND_ENUM_CONSTANT(STATUS_COMPLETED);
	BIND_ENUM_CONSTANT(STATUS_FAILED);
	BIND_ENUM_CONSTANT(STATUS_CANCELLED);

	BIND_ENUM_CONSTANT(CANCEL_REASON_NONE);
	BIND_ENUM_CONSTANT(CANCEL_REASON_USER_REQUEST);
	BIND_ENUM_CONSTANT(CANCEL_REASON_TIMEOUT);
	BIND_ENUM_CONSTANT(CANCEL_REASON_SYSTEM_INTERRUPTED);
}

bool AITaskHandle::_can_transition(Status p_from, Status p_to) const {
	switch (p_from) {
		case STATUS_QUEUED:
			return p_to == STATUS_RUNNING || p_to == STATUS_CANCELLED;
		case STATUS_RUNNING:
			return p_to == STATUS_STREAMING || p_to == STATUS_COMPLETED || p_to == STATUS_FAILED || p_to == STATUS_CANCELLED;
		case STATUS_STREAMING:
			return p_to == STATUS_STREAMING || p_to == STATUS_COMPLETED || p_to == STATUS_FAILED || p_to == STATUS_CANCELLED;
		case STATUS_COMPLETED:
		case STATUS_FAILED:
		case STATUS_CANCELLED:
			return false;
	}

	return false;
}

void AITaskHandle::_emit_state_changed(Status p_old_status, Status p_new_status) {
	if (p_old_status != p_new_status) {
		emit_signal(SNAME("status_changed"), p_old_status, p_new_status);
	}
}

void AITaskHandle::set_job_id(uint64_t p_job_id) {
	MutexLock lock(mutex);
	job_id = p_job_id;
}

uint64_t AITaskHandle::get_job_id() const {
	MutexLock lock(mutex);
	return job_id;
}

AITaskHandle::Status AITaskHandle::get_status() const {
	MutexLock lock(mutex);
	return status;
}

String AITaskHandle::get_status_name() const {
	MutexLock lock(mutex);
	switch (status) {
		case STATUS_QUEUED:
			return "QUEUED";
		case STATUS_RUNNING:
			return "RUNNING";
		case STATUS_STREAMING:
			return "STREAMING";
		case STATUS_COMPLETED:
			return "COMPLETED";
		case STATUS_FAILED:
			return "FAILED";
		case STATUS_CANCELLED:
			return "CANCELLED";
	}

	return "UNKNOWN";
}

bool AITaskHandle::is_terminal() const {
	MutexLock lock(mutex);
	return status == STATUS_COMPLETED || status == STATUS_FAILED || status == STATUS_CANCELLED;
}

bool AITaskHandle::is_finished_successfully() const {
	MutexLock lock(mutex);
	return status == STATUS_COMPLETED && error_code == OK;
}

AITaskHandle::CancelReason AITaskHandle::get_cancel_reason() const {
	MutexLock lock(mutex);
	return cancel_reason;
}

bool AITaskHandle::request_cancel() {
	bool should_emit = false;
	{
		MutexLock lock(mutex);
		if (cancel_requested) {
			return false;
		}

		if (status == STATUS_COMPLETED || status == STATUS_FAILED || status == STATUS_CANCELLED) {
			return false;
		}

		cancel_requested = true;
		should_emit = true;
	}

	if (should_emit) {
		emit_signal(SNAME("cancel_requested"));
	}

	return true;
}

bool AITaskHandle::is_cancel_requested() const {
	MutexLock lock(mutex);
	return cancel_requested;
}

Error AITaskHandle::get_error_code() const {
	MutexLock lock(mutex);
	return error_code;
}

String AITaskHandle::get_error_message() const {
	MutexLock lock(mutex);
	return error_message;
}

PackedStringArray AITaskHandle::get_partial_tokens() const {
	MutexLock lock(mutex);
	return partial_tokens;
}

String AITaskHandle::get_final_text() const {
	MutexLock lock(mutex);
	return final_text;
}

PackedFloat32Array AITaskHandle::get_embedding() const {
	MutexLock lock(mutex);
	return embedding;
}

uint64_t AITaskHandle::get_queue_wait_us() const {
	MutexLock lock(mutex);
	return queue_wait_us;
}

uint64_t AITaskHandle::get_exec_time_us() const {
	MutexLock lock(mutex);
	return exec_time_us;
}

Dictionary AITaskHandle::get_metadata() const {
	MutexLock lock(mutex);
	return metadata;
}

Dictionary AITaskHandle::get_result_snapshot() const {
	MutexLock lock(mutex);

	Dictionary snapshot;
	snapshot["job_id"] = static_cast<int64_t>(job_id);
	snapshot["status"] = status;
	snapshot["status_name"] = get_status_name();
	snapshot["cancel_reason"] = cancel_reason;
	snapshot["cancel_requested"] = cancel_requested;
	snapshot["error_code"] = error_code;
	snapshot["error_message"] = error_message;
	snapshot["partial_tokens"] = partial_tokens;
	snapshot["final_text"] = final_text;
	snapshot["embedding"] = embedding;
	snapshot["queue_wait_us"] = static_cast<int64_t>(queue_wait_us);
	snapshot["exec_time_us"] = static_cast<int64_t>(exec_time_us);
	snapshot["metadata"] = metadata;
	return snapshot;
}

bool AITaskHandle::mark_running() {
	Status old_status;
	{
		MutexLock lock(mutex);
		if (!_can_transition(status, STATUS_RUNNING)) {
			return false;
		}

		old_status = status;
		status = STATUS_RUNNING;
	}

	_emit_state_changed(old_status, STATUS_RUNNING);
	return true;
}

bool AITaskHandle::append_partial_tokens(const PackedStringArray &p_tokens) {
	if (p_tokens.is_empty()) {
		return true;
	}

	Status old_status;
	{
		MutexLock lock(mutex);
		if (!_can_transition(status, STATUS_STREAMING)) {
			return false;
		}

		old_status = status;
		status = STATUS_STREAMING;
		for (int i = 0; i < p_tokens.size(); i++) {
			partial_tokens.push_back(p_tokens[i]);
		}
	}

	_emit_state_changed(old_status, STATUS_STREAMING);
	emit_signal(SNAME("partial_result"), p_tokens);
	return true;
}

bool AITaskHandle::complete(const AIBackendResult &p_result) {
	Status old_status;
	{
		MutexLock lock(mutex);
		if (!_can_transition(status, STATUS_COMPLETED)) {
			return false;
		}

		old_status = status;
		status = STATUS_COMPLETED;
		error_code = p_result.code;
		error_message = p_result.message;
		if (!p_result.partial_tokens.is_empty()) {
			for (int i = 0; i < p_result.partial_tokens.size(); i++) {
				partial_tokens.push_back(p_result.partial_tokens[i]);
			}
		}
		final_text = p_result.final_text;
		embedding = p_result.embedding;
		queue_wait_us = p_result.queue_wait_us;
		exec_time_us = p_result.exec_time_us;
		metadata = p_result.metadata;
	}

	_emit_state_changed(old_status, STATUS_COMPLETED);
	emit_signal(SNAME("completed"));
	return true;
}

bool AITaskHandle::fail(Error p_error_code, const String &p_error_message, const Dictionary &p_metadata) {
	Status old_status;
	{
		MutexLock lock(mutex);
		if (!_can_transition(status, STATUS_FAILED)) {
			return false;
		}

		old_status = status;
		status = STATUS_FAILED;
		error_code = p_error_code;
		error_message = p_error_message;
		metadata = p_metadata;
	}

	_emit_state_changed(old_status, STATUS_FAILED);
	emit_signal(SNAME("failed"), p_error_code, p_error_message);
	return true;
}

bool AITaskHandle::cancel(CancelReason p_reason, const String &p_message, const Dictionary &p_metadata) {
	Status old_status;
	{
		MutexLock lock(mutex);
		if (!_can_transition(status, STATUS_CANCELLED)) {
			return false;
		}

		old_status = status;
		status = STATUS_CANCELLED;
		cancel_reason = p_reason;
		cancel_requested = true;
		error_code = ERR_SKIP;
		error_message = p_message;
		metadata = p_metadata;
	}

	_emit_state_changed(old_status, STATUS_CANCELLED);
	emit_signal(SNAME("cancelled"), p_reason, p_message);
	return true;
}

bool AITaskHandle::apply_backend_result(const AIBackendResult &p_result) {
	if (p_result.timed_out) {
		return cancel(CANCEL_REASON_TIMEOUT, p_result.message.is_empty() ? String("Task timed out.") : p_result.message, p_result.metadata);
	}

	if (p_result.was_cancelled) {
		bool was_cancel_requested = false;
		{
			MutexLock lock(mutex);
			was_cancel_requested = cancel_requested;
		}

		return cancel(was_cancel_requested ? CANCEL_REASON_USER_REQUEST : CANCEL_REASON_SYSTEM_INTERRUPTED, p_result.message, p_result.metadata);
	}

	if (p_result.is_partial) {
		return append_partial_tokens(p_result.partial_tokens);
	}

	if (p_result.code != OK) {
		return fail(p_result.code, p_result.message, p_result.metadata);
	}

	return complete(p_result);
}

AITaskHandle::AITaskHandle() {
	metadata["created_status"] = "QUEUED";
}
