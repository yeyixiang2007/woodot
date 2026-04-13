/**************************************************************************/
/*  test_ai_runtime.h                                                     */
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

#include "../resources/ai_model_resource.h"
#include "../runtime/ai_requests.h"
#include "../runtime/ai_task_scheduler.h"

#include "core/os/os.h"
#include "core/templates/hash_map.h"
#include "core/templates/rid_owner.h"
#include "tests/test_macros.h"

namespace TestWoodotAIRuntime {

class MockAIBackend : public AIBackend {
	struct MockModelState {
		int32_t sequence = 0;
	};

	struct MockContextState {
		RID model_rid;
		bool cancelled = false;
	};

	RID_Owner<MockModelState, true> model_owner;
	RID_Owner<MockContextState, true> context_owner;
	HashMap<uint64_t, RID> running_jobs;
	uint64_t destroy_context_count = 0;
	uint64_t cancel_job_count = 0;

public:
	virtual StringName get_backend_name() const override {
		return StringName("mock_ai");
	}

	virtual AIBackendCapabilities get_capabilities() const override {
		AIBackendCapabilities capabilities;
		capabilities.supports_completion = true;
		capabilities.supports_embedding = true;
		capabilities.supports_streaming = true;
		capabilities.supports_cancellation = true;
		capabilities.metadata["implementation_stage"] = "test";
		return capabilities;
	}

	virtual AIBackendValidationResult validate_model(const Ref<AIModelResource> &p_model) const override {
		AIBackendValidationResult result;
		if (p_model.is_null()) {
			result.code = ERR_INVALID_PARAMETER;
			result.message = "MockAIBackend requires a model resource.";
		}
		return result;
	}

	virtual AIBackendModelLoadResult load_model(const Ref<AIModelResource> &p_model) override {
		AIBackendModelLoadResult result;
		if (!validate_model(p_model).is_ok()) {
			result.code = ERR_INVALID_PARAMETER;
			result.message = "Invalid model resource.";
			return result;
		}

		MockModelState state;
		state.sequence = model_owner.get_rid_count() + 1;
		result.model_handle.rid = model_owner.make_rid(state);
		result.model_handle.backend_name = get_backend_name();
		result.details["sequence"] = state.sequence;
		return result;
	}

	virtual void unload_model(const AIBackendModelHandle &p_model_handle) override {
		if (p_model_handle.rid.is_valid()) {
			model_owner.free(p_model_handle.rid);
		}
	}

	virtual AIBackendContextCreateResult create_context(const AIBackendModelHandle &p_model_handle) override {
		AIBackendContextCreateResult result;
		if (!p_model_handle.is_valid()) {
			result.code = ERR_INVALID_PARAMETER;
			result.message = "Missing model handle.";
			return result;
		}

		MockContextState state;
		state.model_rid = p_model_handle.rid;
		result.context_handle.rid = context_owner.make_rid(state);
		result.context_handle.model_rid = p_model_handle.rid;
		result.context_handle.backend_name = get_backend_name();
		return result;
	}

	virtual void destroy_context(const AIBackendContextHandle &p_context_handle) override {
		if (p_context_handle.rid.is_valid()) {
			context_owner.free(p_context_handle.rid);
			destroy_context_count++;
		}
	}

	virtual AIBackendResult run_job(const AIComputeJob &p_job) override {
		AIBackendResult result;
		result.metadata["backend"] = get_backend_name();
		result.metadata["job_id"] = static_cast<int64_t>(p_job.job_id);

		MockContextState *context = context_owner.get_or_null(p_job.context_handle.rid);
		if (context == nullptr) {
			result.code = ERR_DOES_NOT_EXIST;
			result.message = "Context not found.";
			return result;
		}

		running_jobs.insert(p_job.job_id, p_job.context_handle.rid);

		const Dictionary metadata = p_job.metadata;
		if (metadata.has("exec_time_us")) {
			result.exec_time_us = static_cast<uint64_t>(int64_t(metadata["exec_time_us"]));
		}
		if (metadata.has("queue_wait_us")) {
			result.queue_wait_us = static_cast<uint64_t>(int64_t(metadata["queue_wait_us"]));
		}
		if (metadata.has("force_error")) {
			result.code = Error(int64_t(metadata["force_error"]));
			result.message = "Mock backend forced an error result.";
		} else if (metadata.has("force_cancelled") && bool(metadata["force_cancelled"])) {
			result.was_cancelled = true;
			result.message = "Mock backend cancelled the job.";
		} else {
			result.code = OK;
			result.final_text = p_job.prompt.is_empty() ? String("ok") : p_job.prompt;
		}

		running_jobs.erase(p_job.job_id);
		return result;
	}

	virtual bool cancel_job(uint64_t p_job_id) override {
		cancel_job_count++;
		HashMap<uint64_t, RID>::Iterator job = running_jobs.find(p_job_id);
		if (!job) {
			return false;
		}

		MockContextState *context = context_owner.get_or_null(job->value);
		if (context == nullptr) {
			return false;
		}

		context->cancelled = true;
		return true;
	}

	virtual Dictionary get_runtime_stats() const override {
		Dictionary stats;
		stats["destroy_context_count"] = static_cast<int64_t>(destroy_context_count);
		stats["cancel_job_count"] = static_cast<int64_t>(cancel_job_count);
		stats["loaded_models"] = static_cast<int64_t>(model_owner.get_rid_count());
		stats["active_contexts"] = static_cast<int64_t>(context_owner.get_rid_count());
		return stats;
	}
};

static Ref<AICompletionRequest> make_completion_request(const RID &p_model_rid, const String &p_prompt, int32_t p_timeout_ms = 0, const Dictionary &p_metadata = Dictionary()) {
	Ref<AICompletionRequest> request;
	request.instantiate();
	request->set_model_rid(p_model_rid);
	request->set_prompt(p_prompt);
	request->set_timeout_ms(p_timeout_ms);
	request->set_metadata(p_metadata);
	return request;
}

TEST_CASE("[WoodotAI] Queued task cancellation drains through mailbox") {
	MockAIBackend backend;
	AITaskScheduler scheduler;
	scheduler.set_auto_process_queue_enabled(false);

	Ref<AIModelResource> model;
	model.instantiate();
	model->set_backend_name(backend.get_backend_name());

	const AIBackendModelLoadResult model_load = backend.load_model(model);
	REQUIRE(model_load.is_ok());

	Ref<AITaskHandle> handle = scheduler.submit_completion(&backend, model_load.model_handle, RID(), make_completion_request(RID(), "queued cancel"));
	REQUIRE(handle.is_valid());
	CHECK(handle->get_status() == AITaskHandle::STATUS_QUEUED);

	scheduler.cancel_task(handle);
	CHECK(handle->get_status() == AITaskHandle::STATUS_QUEUED);

	CHECK_EQ(scheduler.poll_completed(), 1);
	CHECK(handle->get_status() == AITaskHandle::STATUS_CANCELLED);
	CHECK(handle->get_cancel_reason() == AITaskHandle::CANCEL_REASON_USER_REQUEST);

	const Dictionary backend_stats = backend.get_runtime_stats();
	CHECK(int64_t(backend_stats["destroy_context_count"]) == 1);
}

TEST_CASE("[WoodotAI] Timeout results map to timeout cancellation") {
	MockAIBackend backend;
	AITaskScheduler scheduler;

	Ref<AIModelResource> model;
	model.instantiate();
	model->set_backend_name(backend.get_backend_name());

	const AIBackendModelLoadResult model_load = backend.load_model(model);
	REQUIRE(model_load.is_ok());

	Dictionary metadata;
	metadata["exec_time_us"] = int64_t(5000);
	Ref<AITaskHandle> handle = scheduler.submit_completion(&backend, model_load.model_handle, RID(), make_completion_request(RID(), "timeout", 1, metadata));
	REQUIRE(handle.is_valid());

	CHECK_EQ(scheduler.poll_completed(), 1);
	CHECK(handle->get_status() == AITaskHandle::STATUS_CANCELLED);
	CHECK(handle->get_cancel_reason() == AITaskHandle::CANCEL_REASON_TIMEOUT);
	CHECK(handle->get_error_code() == ERR_SKIP);
	CHECK(handle->get_error_message().contains("timeout"));

	const Dictionary stats = scheduler.get_stats();
	CHECK(int64_t(stats["timeout_jobs"]) == 1);
}

TEST_CASE("[WoodotAI] Synthetic scheduler soak keeps bookkeeping stable") {
	MockAIBackend backend;
	AITaskScheduler scheduler;

	Ref<AIModelResource> model;
	model.instantiate();
	model->set_backend_name(backend.get_backend_name());

	const AIBackendModelLoadResult model_load = backend.load_model(model);
	REQUIRE(model_load.is_ok());

	const int64_t task_count = 256;
	for (int64_t i = 0; i < task_count; i++) {
		Dictionary metadata;
		metadata["exec_time_us"] = int64_t(100 + (i % 7));
		Ref<AITaskHandle> handle = scheduler.submit_completion(&backend, model_load.model_handle, RID(), make_completion_request(RID(), vformat("task_%d", i), 0, metadata));
		REQUIRE(handle.is_valid());
	}

	CHECK_EQ(scheduler.poll_completed(), task_count);

	const Dictionary stats = scheduler.get_stats();
	CHECK(int64_t(stats["submitted_jobs"]) == task_count);
	CHECK(int64_t(stats["finished_jobs"]) == task_count);
	CHECK(int64_t(stats["running_jobs"]) == 0);
	CHECK(int64_t(stats["failed_jobs"]) == 0);
	CHECK(int64_t(stats["cancelled_jobs"]) == 0);

	const Dictionary profiling = stats["profiling"];
	CHECK(int64_t(profiling["completed_jobs"]) == task_count);
	CHECK(int64_t(profiling["mailbox_drained_updates"]) == task_count);
}

} // namespace TestWoodotAIRuntime
