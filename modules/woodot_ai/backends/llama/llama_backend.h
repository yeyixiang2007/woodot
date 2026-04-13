/**************************************************************************/
/*  llama_backend.h                                                       */
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

#include "modules/woodot_ai/runtime/ai_backend.h"

class LlamaBackend : public AIBackend {
	struct Data;

	Data *data = nullptr;

public:
	LlamaBackend();
	virtual ~LlamaBackend() override;

	virtual StringName get_backend_name() const override;
	virtual AIBackendCapabilities get_capabilities() const override;

	virtual AIBackendValidationResult validate_model(const Ref<AIModelResource> &p_model) const override;
	virtual AIBackendModelLoadResult load_model(const Ref<AIModelResource> &p_model) override;
	virtual void unload_model(const AIBackendModelHandle &p_model_handle) override;

	virtual AIBackendContextCreateResult create_context(const AIBackendModelHandle &p_model_handle) override;
	virtual void destroy_context(const AIBackendContextHandle &p_context_handle) override;

	virtual AIBackendResult run_job(const AIComputeJob &p_job) override;
	virtual bool cancel_job(uint64_t p_job_id) override;
	virtual Dictionary get_runtime_stats() const override;
};
