/**************************************************************************/
/*  ai_tensor_resource.cpp                                                */
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

#include "modules/woodot_ai/resources/ai_tensor_resource.h"

#include "core/object/class_db.h"

void AITensorResource::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_shape", "shape"), &AITensorResource::set_shape);
	ClassDB::bind_method(D_METHOD("get_shape"), &AITensorResource::get_shape);
	ClassDB::bind_method(D_METHOD("set_dtype", "dtype"), &AITensorResource::set_dtype);
	ClassDB::bind_method(D_METHOD("get_dtype"), &AITensorResource::get_dtype);
	ClassDB::bind_method(D_METHOD("set_storage_type", "storage_type"), &AITensorResource::set_storage_type);
	ClassDB::bind_method(D_METHOD("get_storage_type"), &AITensorResource::get_storage_type);
	ClassDB::bind_method(D_METHOD("set_cpu_data", "cpu_data"), &AITensorResource::set_cpu_data);
	ClassDB::bind_method(D_METHOD("get_cpu_data"), &AITensorResource::get_cpu_data);
	ClassDB::bind_method(D_METHOD("set_metadata", "metadata"), &AITensorResource::set_metadata);
	ClassDB::bind_method(D_METHOD("get_metadata"), &AITensorResource::get_metadata);
	ClassDB::bind_method(D_METHOD("is_device_backed"), &AITensorResource::is_device_backed);
	ClassDB::bind_method(D_METHOD("get_element_count"), &AITensorResource::get_element_count);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "shape"), "set_shape", "get_shape");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "dtype"), "set_dtype", "get_dtype");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "storage_type"), "set_storage_type", "get_storage_type");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "cpu_data"), "set_cpu_data", "get_cpu_data");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "metadata"), "set_metadata", "get_metadata");

	BIND_ENUM_CONSTANT(STORAGE_TYPE_CPU);
	BIND_ENUM_CONSTANT(STORAGE_TYPE_CPU_MIRROR);
	BIND_ENUM_CONSTANT(STORAGE_TYPE_EXTERNAL_DEVICE);
}

#define AI_TENSOR_RESOURCE_ACCESSORS(m_type, m_name) \
	void AITensorResource::set_##m_name(const m_type &p_##m_name) { m_name = p_##m_name; } \
	m_type AITensorResource::get_##m_name() const { return m_name; }

AI_TENSOR_RESOURCE_ACCESSORS(PackedInt32Array, shape)
AI_TENSOR_RESOURCE_ACCESSORS(StringName, dtype)
AI_TENSOR_RESOURCE_ACCESSORS(AITensorResource::StorageType, storage_type)
AI_TENSOR_RESOURCE_ACCESSORS(PackedFloat32Array, cpu_data)
AI_TENSOR_RESOURCE_ACCESSORS(Dictionary, metadata)

bool AITensorResource::is_device_backed() const {
	return storage_type != STORAGE_TYPE_CPU;
}

int64_t AITensorResource::get_element_count() const {
	if (shape.is_empty()) {
		return static_cast<int64_t>(cpu_data.size());
	}

	int64_t count = 1;
	for (int i = 0; i < shape.size(); i++) {
		count *= shape[i];
	}
	return count;
}
