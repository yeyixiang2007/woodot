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

static constexpr int64_t AI_TENSOR_INSPECTOR_PREVIEW_LIMIT = 256;
static constexpr int32_t AI_TENSOR_PREVIEW_SAMPLE_COUNT = 8;

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
	ClassDB::bind_method(D_METHOD("is_cpu_data_inspector_limited"), &AITensorResource::is_cpu_data_inspector_limited);
	ClassDB::bind_method(D_METHOD("get_cpu_data_preview"), &AITensorResource::get_cpu_data_preview);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "shape"), "set_shape", "get_shape");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "dtype"), "set_dtype", "get_dtype");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "storage_type"), "set_storage_type", "get_storage_type");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "cpu_data"), "set_cpu_data", "get_cpu_data");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "metadata"), "set_metadata", "get_metadata");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "cpu_data_preview", PROPERTY_HINT_MULTILINE_TEXT, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY), "", "get_cpu_data_preview");

	BIND_ENUM_CONSTANT(STORAGE_TYPE_CPU);
	BIND_ENUM_CONSTANT(STORAGE_TYPE_CPU_MIRROR);
	BIND_ENUM_CONSTANT(STORAGE_TYPE_EXTERNAL_DEVICE);
}

#define AI_TENSOR_RESOURCE_REF_ACCESSORS(m_type, m_name) \
	void AITensorResource::set_##m_name(const m_type &p_##m_name) { m_name = p_##m_name; } \
	m_type AITensorResource::get_##m_name() const { return m_name; }

AI_TENSOR_RESOURCE_REF_ACCESSORS(PackedInt32Array, shape)
AI_TENSOR_RESOURCE_REF_ACCESSORS(StringName, dtype)
AI_TENSOR_RESOURCE_REF_ACCESSORS(Dictionary, metadata)

void AITensorResource::set_cpu_data(const PackedFloat32Array &p_cpu_data) {
	const bool was_limited = is_cpu_data_inspector_limited();
	cpu_data = p_cpu_data;
	if (was_limited != is_cpu_data_inspector_limited()) {
		notify_property_list_changed();
	}
}

PackedFloat32Array AITensorResource::get_cpu_data() const {
	return cpu_data;
}

void AITensorResource::set_storage_type(StorageType p_storage_type) {
	storage_type = p_storage_type;
}

AITensorResource::StorageType AITensorResource::get_storage_type() const {
	return storage_type;
}

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

bool AITensorResource::is_cpu_data_inspector_limited() const {
	return cpu_data.size() > AI_TENSOR_INSPECTOR_PREVIEW_LIMIT;
}

String AITensorResource::get_cpu_data_preview() const {
	const int32_t element_count = cpu_data.size();
	if (element_count == 0) {
		return "No CPU tensor data.";
	}

	const int32_t preview_count = MIN(element_count, AI_TENSOR_PREVIEW_SAMPLE_COUNT);
	PackedStringArray preview_values;
	preview_values.resize(preview_count);
	for (int32_t i = 0; i < preview_count; i++) {
		preview_values.set(i, String::num_real(cpu_data[i]));
	}

	if (!is_cpu_data_inspector_limited()) {
		return vformat("elements=%d values=[%s]", element_count, String(", ").join(preview_values));
	}

	return vformat("elements=%d preview=[%s] ... inspector display limited to %d values", element_count, String(", ").join(preview_values), AI_TENSOR_INSPECTOR_PREVIEW_LIMIT);
}

void AITensorResource::_validate_property(PropertyInfo &p_property) const {
	if (p_property.name == "cpu_data" && is_cpu_data_inspector_limited()) {
		p_property.usage = PROPERTY_USAGE_STORAGE;
	}
}
