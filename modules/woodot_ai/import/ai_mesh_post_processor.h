/**************************************************************************/
/*  ai_mesh_post_processor.h                                              */
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

class AIImportOrchestrator;

class AIMeshPostProcessor : public Object {
	GDCLASS(AIMeshPostProcessor, Object);

	static AIMeshPostProcessor *singleton;

	uint64_t planned_jobs = 0;
	uint64_t generated_plans = 0;

	AIImportOrchestrator *_get_orchestrator() const;

protected:
	static void _bind_methods();

public:
	static AIMeshPostProcessor *get_singleton();

	bool has_import_orchestrator() const;
	bool is_ready() const;

	Dictionary describe_processing_scope(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options = Dictionary()) const;
	Dictionary build_processing_plan(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options = Dictionary());
	Dictionary build_review_report(const Dictionary &p_plan) const;
	Dictionary get_processor_status() const;

	AIMeshPostProcessor();
	~AIMeshPostProcessor();
};
