/**************************************************************************/
/*  ai_texture_enhancer.cpp                                               */
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

#include "modules/woodot_ai/import/ai_texture_enhancer.h"

#include "core/object/class_db.h"
#include "modules/woodot_ai/import/ai_import_orchestrator.h"

AITextureEnhancer *AITextureEnhancer::singleton = nullptr;

void AITextureEnhancer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("has_import_orchestrator"), &AITextureEnhancer::has_import_orchestrator);
	ClassDB::bind_method(D_METHOD("is_ready"), &AITextureEnhancer::is_ready);
	ClassDB::bind_method(D_METHOD("describe_enhancement_scope", "source_path", "importer_name", "options"), &AITextureEnhancer::describe_enhancement_scope, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("build_enhancement_plan", "source_path", "importer_name", "options"), &AITextureEnhancer::build_enhancement_plan, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("build_review_report", "plan"), &AITextureEnhancer::build_review_report);
	ClassDB::bind_method(D_METHOD("get_enhancer_status"), &AITextureEnhancer::get_enhancer_status);

	ADD_SIGNAL(MethodInfo("plan_generated",
			PropertyInfo(Variant::STRING, "source_path"),
			PropertyInfo(Variant::DICTIONARY, "plan")));
}

AITextureEnhancer *AITextureEnhancer::get_singleton() {
	return singleton;
}

bool AITextureEnhancer::has_import_orchestrator() const {
	return _get_orchestrator() != nullptr;
}

bool AITextureEnhancer::is_ready() const {
	return has_import_orchestrator();
}

Dictionary AITextureEnhancer::describe_enhancement_scope(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options) const {
	Dictionary scope;
	bool texture_pass_enabled = false;
	Variant target_profile = String("default");
	Variant enable_super_resolution = true;
	Variant enable_denoise = true;
	Variant enable_auxiliary_map_generation = false;

	if (has_import_orchestrator()) {
		texture_pass_enabled = _get_orchestrator()->is_texture_enhancement_enabled();
	}
	if (p_options.has("target_profile")) {
		target_profile = p_options["target_profile"];
	}
	if (p_options.has("enable_super_resolution")) {
		enable_super_resolution = p_options["enable_super_resolution"];
	}
	if (p_options.has("enable_denoise")) {
		enable_denoise = p_options["enable_denoise"];
	}
	if (p_options.has("enable_auxiliary_map_generation")) {
		enable_auxiliary_map_generation = p_options["enable_auxiliary_map_generation"];
	}

	scope["source_path"] = p_source_path;
	scope["importer_name"] = p_importer_name;
	scope["texture_pass_enabled"] = texture_pass_enabled;
	scope["target_profile"] = target_profile;
	scope["enable_super_resolution"] = enable_super_resolution;
	scope["enable_denoise"] = enable_denoise;
	scope["enable_auxiliary_map_generation"] = enable_auxiliary_map_generation;
	scope["options"] = p_options;
	return scope;
}

Dictionary AITextureEnhancer::build_enhancement_plan(const String &p_source_path, const String &p_importer_name, const Dictionary &p_options) {
	planned_jobs++;

	Dictionary plan;
	const Dictionary scope = describe_enhancement_scope(p_source_path, p_importer_name, p_options);
	Array recommended_steps;
	Array deferred_steps;
	Array warnings;

	if (bool(scope["enable_super_resolution"])) {
		recommended_steps.push_back("super_resolution_review");
	}
	if (bool(scope["enable_denoise"])) {
		recommended_steps.push_back("denoise_review");
	}
	if (bool(scope["enable_auxiliary_map_generation"])) {
		recommended_steps.push_back("auxiliary_map_review");
	}

	deferred_steps.push_back("pixel_rewrite");
	deferred_steps.push_back("batch_upscale");
	deferred_steps.push_back("normal_roughness_generation");
	warnings.push_back("AITextureEnhancer is currently a planning skeleton and does not modify imported textures.");

	plan["schema"] = "woodot_ai.texture_enhancement_plan.v1";
	plan["scope"] = scope;
	plan["recommended_steps"] = recommended_steps;
	plan["deferred_steps"] = deferred_steps;
	plan["warnings"] = warnings;
	plan["requires_manual_review"] = true;
	plan["can_apply_automatically"] = false;

	generated_plans++;
	emit_signal(SNAME("plan_generated"), p_source_path, plan);
	return plan;
}

Dictionary AITextureEnhancer::build_review_report(const Dictionary &p_plan) const {
	Dictionary report;
	report["kind"] = "texture_enhancement_review";
	report["ok"] = p_plan.has("schema");
	report["summary"] = "Texture enhancement plan is advisory only in the current MVP.";
	report["plan"] = p_plan;
	report["requires_manual_review"] = true;
	return report;
}

Dictionary AITextureEnhancer::get_enhancer_status() const {
	Dictionary status;
	status["ready"] = is_ready();
	status["has_import_orchestrator"] = has_import_orchestrator();
	status["planned_jobs"] = static_cast<int64_t>(planned_jobs);
	status["generated_plans"] = static_cast<int64_t>(generated_plans);
	return status;
}

AIImportOrchestrator *AITextureEnhancer::_get_orchestrator() const {
	return AIImportOrchestrator::get_singleton();
}

AITextureEnhancer::AITextureEnhancer() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
}

AITextureEnhancer::~AITextureEnhancer() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
