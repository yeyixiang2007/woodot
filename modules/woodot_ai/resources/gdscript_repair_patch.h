/**************************************************************************/
/*  gdscript_repair_patch.h                                               */
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

class GDScriptRepairPatch : public Resource {
	GDCLASS(GDScriptRepairPatch, Resource);

	String script_path;
	String diagnostic_message;
	int32_t line_start = 0;
	int32_t line_end = 0;
	String replacement_text;
	Array hunks;
	Array warnings;
	Dictionary metadata;

protected:
	static void _bind_methods();

public:
	void set_script_path(const String &p_script_path);
	String get_script_path() const;
	void set_diagnostic_message(const String &p_diagnostic_message);
	String get_diagnostic_message() const;
	void set_line_start(int32_t p_line_start);
	int32_t get_line_start() const;
	void set_line_end(int32_t p_line_end);
	int32_t get_line_end() const;
	void set_replacement_text(const String &p_replacement_text);
	String get_replacement_text() const;
	void set_hunks(const Array &p_hunks);
	Array get_hunks() const;
	void set_warnings(const Array &p_warnings);
	Array get_warnings() const;
	void set_metadata(const Dictionary &p_metadata);
	Dictionary get_metadata() const;
};
