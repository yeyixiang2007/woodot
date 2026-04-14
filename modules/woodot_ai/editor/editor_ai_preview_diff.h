/**************************************************************************/
/*  editor_ai_preview_diff.h                                              */
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

class GDScriptRepairPatch;
class SceneSynthesisPlan;

class EditorAIPreviewDiff : public Object {
	GDCLASS(EditorAIPreviewDiff, Object);

	static EditorAIPreviewDiff *singleton;

	Dictionary current_preview;

	Array _normalize_warnings(const Array &p_warnings) const;
	Dictionary _build_scene_operation_item(const Dictionary &p_operation, int32_t p_index) const;
	Dictionary _build_resource_operation_item(const Dictionary &p_operation, int32_t p_index) const;
	Dictionary _build_patch_hunk_item(const Dictionary &p_hunk, int32_t p_index) const;

protected:
	static void _bind_methods();

public:
	static EditorAIPreviewDiff *get_singleton();

	Dictionary build_scene_plan_preview(const Ref<SceneSynthesisPlan> &p_plan) const;
	Dictionary build_gdscript_patch_preview(const Ref<GDScriptRepairPatch> &p_patch) const;
	void set_current_preview(const Dictionary &p_preview);
	Dictionary get_current_preview() const;
	void clear_preview();

	EditorAIPreviewDiff();
	~EditorAIPreviewDiff();
};
