/**************************************************************************/
/*  scene_synthesis_plan.h                                                */
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

class SceneSynthesisPlan : public Resource {
	GDCLASS(SceneSynthesisPlan, Resource);

	String prompt;
	String source_ir;
	Array node_operations;
	Array resource_operations;
	Array warnings;
	Dictionary metadata;

protected:
	static void _bind_methods();

public:
	void set_prompt(const String &p_prompt);
	String get_prompt() const;
	void set_source_ir(const String &p_source_ir);
	String get_source_ir() const;
	void set_node_operations(const Array &p_node_operations);
	Array get_node_operations() const;
	void set_resource_operations(const Array &p_resource_operations);
	Array get_resource_operations() const;
	void set_warnings(const Array &p_warnings);
	Array get_warnings() const;
	void set_metadata(const Dictionary &p_metadata);
	Dictionary get_metadata() const;
};
