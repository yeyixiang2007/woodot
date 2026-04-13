/**************************************************************************/
/*  ai_runtime_server.h                                                   */
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

#include "core/object/object.h"
#include "core/os/thread_safe.h"
#include "modules/woodot_ai/runtime/ai_backend_registry.h"
#include "modules/woodot_ai/runtime/ai_requests.h"
#include "modules/woodot_ai/runtime/ai_task_handle.h"

class AIModelResource;

class AIRuntimeServer : public Object {
	GDCLASS(AIRuntimeServer, Object);
	_THREAD_SAFE_CLASS_

	struct Data;

	static AIRuntimeServer *singleton;

	Data *data = nullptr;

protected:
	static void _bind_methods();

public:
	static AIRuntimeServer *get_singleton();

	RID load_model(const Ref<AIModelResource> &p_model);
	void unload_model(const RID &p_model_rid);
	bool has_model(const RID &p_model_rid) const;
	Ref<AITaskHandle> submit_completion(const Ref<AICompletionRequest> &p_request);
	Ref<AITaskHandle> submit_embedding(const Ref<AIEmbeddingRequest> &p_request);
	void cancel_task(const Ref<AITaskHandle> &p_task_handle);

	Dictionary get_runtime_stats() const;
	Dictionary get_model_info(const RID &p_model_rid) const;
	Dictionary get_backend_capabilities(const StringName &p_backend_name) const;
	PackedStringArray get_registered_backends() const;
	void poll_completed();

	AIRuntimeServer();
	~AIRuntimeServer();
};
