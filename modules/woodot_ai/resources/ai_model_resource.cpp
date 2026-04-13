/**************************************************************************/
/*  ai_model_resource.cpp                                                 */
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

#include "modules/woodot_ai/resources/ai_model_resource.h"

#include "core/object/class_db.h"

void AIModelResource::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_model_path", "model_path"), &AIModelResource::set_model_path);
	ClassDB::bind_method(D_METHOD("get_model_path"), &AIModelResource::get_model_path);
	ClassDB::bind_method(D_METHOD("set_backend_type", "backend_type"), &AIModelResource::set_backend_type);
	ClassDB::bind_method(D_METHOD("get_backend_type"), &AIModelResource::get_backend_type);
	ClassDB::bind_method(D_METHOD("set_context_size", "context_size"), &AIModelResource::set_context_size);
	ClassDB::bind_method(D_METHOD("get_context_size"), &AIModelResource::get_context_size);
	ClassDB::bind_method(D_METHOD("set_n_threads", "n_threads"), &AIModelResource::set_n_threads);
	ClassDB::bind_method(D_METHOD("get_n_threads"), &AIModelResource::get_n_threads);
	ClassDB::bind_method(D_METHOD("set_n_gpu_layers", "n_gpu_layers"), &AIModelResource::set_n_gpu_layers);
	ClassDB::bind_method(D_METHOD("get_n_gpu_layers"), &AIModelResource::get_n_gpu_layers);
	ClassDB::bind_method(D_METHOD("set_quantization", "quantization"), &AIModelResource::set_quantization);
	ClassDB::bind_method(D_METHOD("get_quantization"), &AIModelResource::get_quantization);
	ClassDB::bind_method(D_METHOD("set_chat_template", "chat_template"), &AIModelResource::set_chat_template);
	ClassDB::bind_method(D_METHOD("get_chat_template"), &AIModelResource::get_chat_template);
	ClassDB::bind_method(D_METHOD("set_rope_scaling", "rope_scaling"), &AIModelResource::set_rope_scaling);
	ClassDB::bind_method(D_METHOD("get_rope_scaling"), &AIModelResource::get_rope_scaling);
	ClassDB::bind_method(D_METHOD("set_system_prompt_template", "system_prompt_template"), &AIModelResource::set_system_prompt_template);
	ClassDB::bind_method(D_METHOD("get_system_prompt_template"), &AIModelResource::get_system_prompt_template);
	ClassDB::bind_method(D_METHOD("set_capability_tags", "capability_tags"), &AIModelResource::set_capability_tags);
	ClassDB::bind_method(D_METHOD("get_capability_tags"), &AIModelResource::get_capability_tags);
	ClassDB::bind_method(D_METHOD("set_extra_options", "extra_options"), &AIModelResource::set_extra_options);
	ClassDB::bind_method(D_METHOD("get_extra_options"), &AIModelResource::get_extra_options);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "model_path"), "set_model_path", "get_model_path");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "backend_type"), "set_backend_type", "get_backend_type");
	ADD_PROPERTY_DEFAULT("backend_type", StringName("llama"));
	ADD_PROPERTY(PropertyInfo(Variant::INT, "context_size"), "set_context_size", "get_context_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "n_threads"), "set_n_threads", "get_n_threads");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "n_gpu_layers"), "set_n_gpu_layers", "get_n_gpu_layers");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "quantization"), "set_quantization", "get_quantization");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "chat_template", PROPERTY_HINT_MULTILINE_TEXT), "set_chat_template", "get_chat_template");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rope_scaling"), "set_rope_scaling", "get_rope_scaling");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "system_prompt_template", PROPERTY_HINT_MULTILINE_TEXT), "set_system_prompt_template", "get_system_prompt_template");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "capability_tags"), "set_capability_tags", "get_capability_tags");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "extra_options"), "set_extra_options", "get_extra_options");
}

#define AI_MODEL_RESOURCE_ACCESSORS(m_type, m_name) \
	void AIModelResource::set_##m_name(const m_type &p_##m_name) { m_name = p_##m_name; } \
	m_type AIModelResource::get_##m_name() const { return m_name; }

AI_MODEL_RESOURCE_ACCESSORS(String, model_path)
AI_MODEL_RESOURCE_ACCESSORS(StringName, backend_type)
AI_MODEL_RESOURCE_ACCESSORS(int32_t, context_size)
AI_MODEL_RESOURCE_ACCESSORS(int32_t, n_threads)
AI_MODEL_RESOURCE_ACCESSORS(int32_t, n_gpu_layers)
AI_MODEL_RESOURCE_ACCESSORS(String, quantization)
AI_MODEL_RESOURCE_ACCESSORS(String, chat_template)
AI_MODEL_RESOURCE_ACCESSORS(float, rope_scaling)
AI_MODEL_RESOURCE_ACCESSORS(String, system_prompt_template)
AI_MODEL_RESOURCE_ACCESSORS(PackedStringArray, capability_tags)
AI_MODEL_RESOURCE_ACCESSORS(Dictionary, extra_options)
