/**************************************************************************/
/*  ai_tensor_resource.h                                                  */
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
#include "core/variant/type_info.h"

class AITensorResource : public Resource {
	GDCLASS(AITensorResource, Resource);

public:
	enum StorageType {
		STORAGE_TYPE_CPU = 0,
		STORAGE_TYPE_CPU_MIRROR,
		STORAGE_TYPE_EXTERNAL_DEVICE,
	};

private:
	PackedInt32Array shape;
	StringName dtype = StringName("float32");
	StorageType storage_type = STORAGE_TYPE_CPU;
	PackedFloat32Array cpu_data;
	Dictionary metadata;

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;

public:
	void set_shape(const PackedInt32Array &p_shape);
	PackedInt32Array get_shape() const;

	void set_dtype(const StringName &p_dtype);
	StringName get_dtype() const;

	void set_storage_type(StorageType p_storage_type);
	StorageType get_storage_type() const;

	void set_cpu_data(const PackedFloat32Array &p_cpu_data);
	PackedFloat32Array get_cpu_data() const;

	void set_metadata(const Dictionary &p_metadata);
	Dictionary get_metadata() const;

	bool is_device_backed() const;
	int64_t get_element_count() const;
	bool is_cpu_data_inspector_limited() const;
	String get_cpu_data_preview() const;
};

VARIANT_ENUM_CAST(AITensorResource::StorageType);
