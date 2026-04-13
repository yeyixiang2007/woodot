/**************************************************************************/
/*  ai_model_resource.h                                                   */
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

#include "core/io/resource.h"

class AIModelResource : public Resource {
	GDCLASS(AIModelResource, Resource);

	String model_path;
	StringName backend_type = StringName("llama");
	int32_t context_size = 4096;
	int32_t n_threads = 0;
	int32_t n_gpu_layers = 0;
	String quantization;
	String chat_template;
	float rope_scaling = 1.0f;
	String system_prompt_template;
	PackedStringArray capability_tags;
	Dictionary extra_options;

protected:
	static void _bind_methods();

public:
	void set_model_path(const String &p_model_path);
	String get_model_path() const;

	void set_backend_type(const StringName &p_backend_type);
	StringName get_backend_type() const;

	void set_context_size(int32_t p_context_size);
	int32_t get_context_size() const;

	void set_n_threads(int32_t p_n_threads);
	int32_t get_n_threads() const;

	void set_n_gpu_layers(int32_t p_n_gpu_layers);
	int32_t get_n_gpu_layers() const;

	void set_quantization(const String &p_quantization);
	String get_quantization() const;

	void set_chat_template(const String &p_chat_template);
	String get_chat_template() const;

	void set_rope_scaling(float p_rope_scaling);
	float get_rope_scaling() const;

	void set_system_prompt_template(const String &p_system_prompt_template);
	String get_system_prompt_template() const;

	void set_capability_tags(const PackedStringArray &p_capability_tags);
	PackedStringArray get_capability_tags() const;

	void set_extra_options(const Dictionary &p_extra_options);
	Dictionary get_extra_options() const;
};
