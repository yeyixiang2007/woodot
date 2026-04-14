/**************************************************************************/
/*  gdscript_repair_engine.h                                              */
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
#include "modules/woodot_ai/resources/gdscript_repair_patch.h"

class GDScriptRepairEngine : public Object {
	GDCLASS(GDScriptRepairEngine, Object);

	static GDScriptRepairEngine *singleton;

	Dictionary _validate_hunk(const Dictionary &p_hunk, int32_t p_index) const;
	Array _normalize_string_array(const Variant &p_value) const;
	bool _is_allowed_patch_operation(const String &p_operation) const;

protected:
	static void _bind_methods();

public:
	static GDScriptRepairEngine *get_singleton();

	Dictionary validate_patch_ir(const String &p_source_ir) const;
	Ref<GDScriptRepairPatch> parse_patch_ir(const String &p_source_ir, const String &p_script_path = String(), const String &p_diagnostic_message = String(), const Dictionary &p_metadata = Dictionary()) const;
	Dictionary get_engine_status() const;

	GDScriptRepairEngine();
	~GDScriptRepairEngine();
};
