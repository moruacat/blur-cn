#include "render.h"

#include <cmath>
#include <cstdlib>
#include <system_error>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>
#include <misc/freetype/imgui_freetype.h>

#include "../fonts/dejavu_sans.h"
#include "../fonts/eb_garamond.h"
#include "../fonts/icons.h"
#include "imgui_internal.h"

#include "common/utils.h"

namespace {
	gfx::Color interpolate_color(const std::vector<gfx::Color>& colors, const std::vector<float>& positions, float t) {
		// Find the segment containing t
		size_t i = 0;
		for (; i < positions.size() - 1; i++) {
			if (t >= positions[i] && t <= positions[i + 1])
				break;
		}

		// If t is outside the range, clamp to the nearest color
		if (i >= positions.size() - 1) {
			if (t < positions[0])
				return colors[0];
			return colors[colors.size() - 1];
		}

		// Normalize t to the segment
		float segment_t = (t - positions[i]) / (positions[i + 1] - positions[i]);

		// Linear interpolation between colors
		const gfx::Color& c1 = colors[i];
		const gfx::Color& c2 = colors[i + 1];

		return {
			static_cast<uint8_t>(c1.r + (segment_t * (c2.r - c1.r))),
			static_cast<uint8_t>(c1.g + (segment_t * (c2.g - c1.g))),
			static_cast<uint8_t>(c1.b + (segment_t * (c2.b - c1.b))),
			static_cast<uint8_t>(c1.a + (segment_t * (c2.a - c1.a))),
		};
	}
}

bool render::ImGuiWrap::init(SDL_Window* window, const SDL_GLContext& context) {
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ctx = ImGui::CreateContext();
	io = &ImGui::GetIO();
	io->IniFilename = nullptr;
	io->LogFilename = nullptr;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100 (WebGL 1.0)
	const char* glsl_version = "#version 100";
#elif defined(IMGUI_IMPL_OPENGL_ES3)
	// GL ES 3.0 + GLSL 300 es (WebGL 2.0)
	const char* glsl_version = "#version 300 es";
#elif defined(__APPLE__)
	// GL 3.2 Core + GLSL 150
	const char* glsl_version = "#version 150";
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
#endif

	ImGui_ImplSDL3_InitForOpenGL(window, context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	return true;
}

bool render::init(SDL_Window* window, const SDL_GLContext& context) {
	if (!imgui.init(window, context))
		return false;

	// TODO: Consider using ImFontGlyphRangesBuilder to build glyph ranges from textual data.

	// ImFontGlyphRangesBuilder builder;
	// builder.AddRanges(imgui.io->Fonts->GetGlyphRangesDefault());
	// // builder.AddRanges(imgui.io->Fonts->GetGlyphRangesGreek());
	// // builder.AddRanges(imgui.io->Fonts->GetGlyphRangesKorean());
	// // builder.AddRanges(imgui.io->Fonts->GetGlyphRangesJapanese());
	// // builder.AddRanges(imgui.io->Fonts->GetGlyphRangesChineseSimplifiedCommon());
	// // builder.AddRanges(imgui.io->Fonts->GetGlyphRangesCyrillic());
	// // builder.AddRanges(imgui.io->Fonts->GetGlyphRangesThai());
	// // builder.AddRanges(imgui.io->Fonts->GetGlyphRangesVietnamese());
	// ImVector<ImWchar> custom_ranges;
	// builder.BuildRanges(&custom_ranges);

	// ^ TODO: RANDOM CRASH TODO REFACTOR  IMPORTANT FIX  YES
	const auto* ranges = imgui.io->Fonts->GetGlyphRangesDefault();

	// Glyphs are baked at the physical resolution of the framebuffer so the GPU
	// never has to resample them. SDL_GetWindowPixelDensity() is not enough here
	// (it only reflects HiDPI window flags), so measure the ratio the same way
	// the ImGui backend does when it sets DisplayFramebufferScale.
	int window_w = 0;
	int window_h = 0;
	int pixel_w = 0;
	int pixel_h = 0;
	SDL_GetWindowSize(window, &window_w, &window_h);
	SDL_GetWindowSizeInPixels(window, &pixel_w, &pixel_h);
	const float framebuffer_scale =
		(window_w > 0 && window_h > 0) ? static_cast<float>(pixel_w) / static_cast<float>(window_w) : 1.f;

	ImFontConfig font_cfg;
	font_cfg.RasterizerDensity = framebuffer_scale; // TODO PORT: update when changing screen

	// Chinese-capable UI font. It's shipped next to the executable instead of
	// being embedded because the font file is quite large (~8MB). If it's
	// missing we fall back to the embedded latin-only font so the app still
	// starts (Chinese text will show as placeholder glyphs).
	//
	// imgui's "common" simplified Chinese set is missing a few characters used
	// by the zh-CN translation, so append them to a copy of the range list.
	static ImVector<ImWchar> cjk_ranges_storage;
	if (cjk_ranges_storage.empty()) {
		const ImWchar* common = imgui.io->Fonts->GetGlyphRangesChineseSimplifiedCommon();
		for (const ImWchar* range = common; *range; range += 2) {
			cjk_ranges_storage.push_back(range[0]);
			cjk_ranges_storage.push_back(range[1]);
		}

		static constexpr ImWchar extra_glyphs[] = {
			0x6E32, // 渲
			0x9891, // 频
			0x5E27, // 帧
			0x4F3D, // 伽
			0x9608, // 阈
			0x5D4C, // 嵌
		};

		for (const ImWchar glyph : extra_glyphs) {
			cjk_ranges_storage.push_back(glyph);
			cjk_ranges_storage.push_back(glyph);
		}

		cjk_ranges_storage.push_back(0);
	}

	const ImWchar* cjk_ranges = cjk_ranges_storage.Data;

	// Medium keeps small Chinese text readable: Regular strokes are thin enough
	// that they smear together at UI sizes. Regular stays as a fallback so an
	// older install (fonts dir from a previous version) still works.
	static constexpr std::array<const char*, 2> cjk_font_names = {
		"HarmonyOS_Sans_SC_Medium.ttf",
		"HarmonyOS_Sans_SC_Regular.ttf",
	};

	// Chinese UI font. Windows ships "Microsoft YaHei UI", which comes with
	// native hinting instructions: its stems snap to the pixel grid, which is
	// why Explorer, Notepad and ClassIsland look sharp at UI sizes. Prefer it,
	// and fall back to the bundled HarmonyOS Sans (auto-hinted by FreeType) on
	// systems that don't have it.
	struct CjkFontSource {
		std::filesystem::path path;
		int face_index = 0;
		bool native_hinting = false;
	};

	CjkFontSource cjk_font;
	{
		std::error_code ec;
		const char* windir = std::getenv("WINDIR");
		std::filesystem::path yahei =
			windir ? std::filesystem::path(windir) / "Fonts" / "msyh.ttc" : std::filesystem::path();

		if (!yahei.empty() && std::filesystem::exists(yahei, ec))
			cjk_font = { yahei, 1, true }; // face 1 is "Microsoft YaHei UI"
	}

	const std::filesystem::path fonts_dir = u::get_resources_path() / "fonts";
	if (cjk_font.path.empty()) {
		for (const auto name : cjk_font_names) {
			std::filesystem::path candidate = fonts_dir / name;
			std::error_code ec;
			if (std::filesystem::exists(candidate, ec)) {
				cjk_font = { candidate, 0, false };
				break;
			}
		}
	}

	// Sizes are in unscaled units; ui_scale() grows the whole interface (fonts,
	// paddings, window) together so the layout keeps its proportions. Chinese
	// glyphs also need a couple more pixels than latin ones to stay legible, so
	// the body font starts at 15 instead of the latin 13.
	const float ui_font_scale = ui_scale();

	// YaHei draws its ideographs smaller inside the em box than HarmonyOS Sans,
	// so request a slightly larger size to keep the same on-screen size. This
	// lands on a 17px em, which is what native Windows apps use at 100% scaling.
	const float cjk_size_boost = cjk_font.native_hinting ? 1.13f : 1.f;

	// Whole-pixel sizes: hinting aligns stems to the pixel grid, and a
	// fractional size defeats it (which is what made the text look soft).
	const float body_font_size = std::round(15.f * cjk_size_boost * ui_font_scale);
	const float smaller_header_font_size = std::round(20.f * cjk_size_boost * ui_font_scale);

	ImFontConfig cjk_font_cfg = font_cfg;
	cjk_font_cfg.FontNo = cjk_font.face_index;

	// Integer advances keep each glyph on the pixel grid instead of letting the
	// GPU resample it at a fractional offset.
	cjk_font_cfg.PixelSnapH = true;

	// ImGui always rasterises grayscale antialiasing, which reads thinner than
	// the ClearType-style rendering Windows apps use, so nudge coverage up.
	cjk_font_cfg.RasterizerMultiply = 1.08f;

	if (!cjk_font.native_hinting) {
		// HarmonyOS Sans has no hinting instructions of its own, so ask FreeType
		// for the auto-hinter (vertical-only, like ClearType).
		cjk_font_cfg.FontBuilderFlags = static_cast<unsigned int>(
			ImGuiFreeTypeBuilderFlags_LightHinting | ImGuiFreeTypeBuilderFlags_ForceAutoHint
		);
	}

	if (!cjk_font.path.empty()) {
		u::log(
			"ui: using CJK font '{}' face {} at {}px (heading {}px), window {}x{}, framebuffer {}x{}, scale {:.2f}, "
			"ui scale {:.2f}",
			cjk_font.path.filename().string(),
			cjk_font.face_index,
			body_font_size,
			smaller_header_font_size,
			window_w,
			window_h,
			pixel_w,
			pixel_h,
			framebuffer_scale,
			ui_font_scale
		);

		if (!fonts::dejavu.init_from_file(cjk_font.path, body_font_size, &cjk_font_cfg, cjk_ranges))
			return false;

		if (!fonts::smaller_header_font.init_from_file(
				cjk_font.path, smaller_header_font_size, &cjk_font_cfg, cjk_ranges
			))
			return false;
	}
	else {
		u::log_error(
			"CJK font not found in '{}' or the Windows font directory, falling back to the latin font",
			fonts_dir.string()
		);

		if (!fonts::dejavu.init(DEJAVU_SANS_COMPRESSED_DATA, body_font_size, &font_cfg, ranges))
			return false;

		if (!fonts::smaller_header_font.init(EB_GARAMOND_COMPRESSED_DATA, smaller_header_font_size, &font_cfg, ranges))
			return false;
	}

	if (!fonts::header_font.init(EB_GARAMOND_COMPRESSED_DATA, std::round(30.f * ui_font_scale), &font_cfg, ranges))
		return false;

	if (!fonts::icons.init(ICONS_COMPRESSED_DATA, std::round(15.f * ui_font_scale), &font_cfg, ranges))
		return false;

	return true;
}

void render::destroy() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
}

float render::ui_scale() {
	static const float scale = [] {
		float result = 1.3f;

		const char* env_scale = std::getenv("BLUR_UI_SCALE");
		if (!env_scale)
			env_scale = std::getenv("BLUR_UI_FONT_SCALE");

		if (env_scale) {
			const float parsed = std::strtof(env_scale, nullptr);
			if (parsed >= 0.5f && parsed <= 4.f)
				result = parsed;
		}

		return result;
	}();

	return scale;
}

int render::ui_px(int value) {
	return static_cast<int>((static_cast<float>(value) * ui_scale()) + 0.5f);
}

void render::update_window_size(SDL_Window* window) {
	SDL_GetWindowSize(window, &window_size.w, &window_size.h);
}

void render::ImGuiWrap::begin(SDL_Window* window) {
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	drawlist = ImGui::GetForegroundDrawList();

	// calculate frametime
	auto now = std::chrono::high_resolution_clock::now();
	static auto last = now;

	frametime = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(now - last).count() / 1000.f;

	last = now;
}

void render::ImGuiWrap::end(SDL_Window* window) { // NOLINT(readability-convert-member-functions-to-static)
	                                              // ^ yeah, but this is nicer to call
	static constexpr ImVec4 clear_colour = ImVec4(0.f, 0.f, 0.f, 1.f);

	for (auto& call : late_draw_calls) {
		call();
	}
	late_draw_calls.clear();

	ImGui::Render();

	static bool logged_font_atlas = false;
	if (!logged_font_atlas) {
		logged_font_atlas = true;
		const ImGuiIO& io = ImGui::GetIO();
		u::log(
			"ui: font atlas {}x{}, display {:.0f}x{:.0f} @ {:.2f}x",
			io.Fonts->TexWidth,
			io.Fonts->TexHeight,
			io.DisplaySize.x,
			io.DisplaySize.y,
			io.DisplayFramebufferScale.x
		);
	}

	int drawable_width = 0;
	int drawable_height = 0;
	SDL_GetWindowSizeInPixels(window, &drawable_width, &drawable_height);

	glViewport(0, 0, drawable_width, drawable_height);

	glClearColor(
		clear_colour.x * clear_colour.w,
		clear_colour.y * clear_colour.w,
		clear_colour.z * clear_colour.w,
		clear_colour.w
	);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(window);
}

void render::line(
	const gfx::Point& pos1, const gfx::Point& pos2, const gfx::Color& col, bool anti_aliased, float thickness
) {
	imgui.drawlist->AddLine(pos1, pos2, col.to_imgui(), thickness);
}

void render::rect_filled(const gfx::Rect& rect, const gfx::Color& col) {
	imgui.drawlist->AddRectFilled(rect.origin(), rect.max(), col.to_imgui());
}

void render::rect_stroke(const gfx::Rect& rect, const gfx::Color& col, float thickness) {
	imgui.drawlist->AddRect(rect.origin(), rect.max(), col.to_imgui(), 0.f, 0, thickness);
}

void render::rounded_rect_filled(
	const gfx::Rect& rect, const gfx::Color& col, float rounding, unsigned int rounding_flags
) {
	imgui.drawlist->AddRectFilled(rect.origin(), rect.max(), col.to_imgui(), rounding, rounding_flags);
}

void render::rounded_rect_stroke(
	const gfx::Rect& rect, const gfx::Color& col, float rounding, unsigned int rounding_flags, float thickness
) {
	imgui.drawlist->AddRect(rect.origin(), rect.max(), col.to_imgui(), rounding, rounding_flags, thickness);
}

void render::rect_filled_gradient(
	const gfx::Rect& rect,
	const std::vector<gfx::Point>& gradient_direction,
	const std::vector<gfx::Color>& colors,
	const std::vector<float>& positions
) {
	// Validate inputs
	if (colors.size() < 2 || colors.size() != positions.size() || gradient_direction.size() != 2) {
		return; // Invalid input parameters
	}

	// For custom gradient direction, we need to split the rectangle into multiple triangles
	ImVec2 origin = rect.origin();
	ImVec2 max = rect.max();

	// Calculate direction vector
	ImVec2 dir_vec(
		gradient_direction[1].x - gradient_direction[0].x, gradient_direction[1].y - gradient_direction[0].y
	);
	float dir_length = sqrtf((dir_vec.x * dir_vec.x) + (dir_vec.y * dir_vec.y));

	if (dir_length < 0.0001f) {
		return; // Invalid direction
	}

	// Normalize direction vector
	dir_vec.x /= dir_length;
	dir_vec.y /= dir_length;

	// Calculate perpendicular vector
	ImVec2 perp_vec(-dir_vec.y, dir_vec.x);

	// Create gradient mesh using triangles
	const int segments = 32; // Number of segments for smooth gradient

	for (int i = 0; i < segments; i++) {
		float t1 = static_cast<float>(i) / segments;
		float t2 = static_cast<float>(i + 1) / segments;

		// Find color at position t1 and t2
		ImU32 col1 = interpolate_color(colors, positions, t1).to_imgui();
		ImU32 col2 = interpolate_color(colors, positions, t2).to_imgui();

		// Calculate points for this segment
		ImVec2 p1 = ImVec2(
			origin.x + (dir_vec.x * t1 * (max.x - origin.x)) + (perp_vec.x * origin.y),
			origin.y + (dir_vec.y * t1 * (max.y - origin.y)) + (perp_vec.y * origin.x)
		);
		ImVec2 p2 = ImVec2(
			origin.x + (dir_vec.x * t2 * (max.x - origin.x)) + (perp_vec.x * origin.y),
			origin.y + (dir_vec.y * t2 * (max.y - origin.y)) + (perp_vec.y * origin.x)
		);
		ImVec2 p3 = ImVec2(
			max.x + (dir_vec.x * t1 * (max.x - origin.x)) + (perp_vec.x * max.y),
			max.y + (dir_vec.y * t1 * (max.y - origin.y)) + (perp_vec.y * max.x)
		);
		ImVec2 p4 = ImVec2(
			max.x + (dir_vec.x * t2 * (max.x - origin.x)) + (perp_vec.x * max.y),
			max.y + (dir_vec.y * t2 * (max.y - origin.y)) + (perp_vec.y * max.x)
		);

		// Draw triangles
		imgui.drawlist->AddTriangleFilled(p1, p2, p3, col1);
		imgui.drawlist->AddTriangleFilled(p2, p3, p4, col2);
	}
}

void render::rect_filled_gradient(
	const gfx::Rect& rect,
	GradientDirection direction,
	const std::vector<gfx::Color>& colors,
	const std::vector<float>& positions
) {
	// Validate inputs
	if (colors.size() < 2 || colors.size() != positions.size()) {
		return; // Invalid input parameters
	}

	ImVec2 origin = rect.origin();
	ImVec2 max = rect.max();

	// For predefined directions, we can use the ImGui built-in functions
	if (colors.size() == 2) {
		ImU32 col1 = colors[0].to_imgui();
		ImU32 col2 = colors[1].to_imgui();

		switch (direction) {
			case GradientDirection::GRADIENT_RIGHT:
				imgui.drawlist->AddRectFilledMultiColor(origin, max, col1, col2, col2, col1);
				break;
			case GradientDirection::GRADIENT_LEFT:
				imgui.drawlist->AddRectFilledMultiColor(origin, max, col2, col1, col1, col2);
				break;
			case GradientDirection::GRADIENT_DOWN:
				imgui.drawlist->AddRectFilledMultiColor(origin, max, col1, col1, col2, col2);
				break;
			case GradientDirection::GRADIENT_UP:
				imgui.drawlist->AddRectFilledMultiColor(origin, max, col2, col2, col1, col1);
				break;
			case GradientDirection::GRADIENT_DOWN_RIGHT:
				imgui.drawlist->AddRectFilledMultiColor(origin, max, col1, col2, col2, col1);
				break;
			case GradientDirection::GRADIENT_UP_LEFT:
				imgui.drawlist->AddRectFilledMultiColor(origin, max, col2, col1, col1, col2);
				break;
			case GradientDirection::GRADIENT_DOWN_LEFT:
				imgui.drawlist->AddRectFilledMultiColor(origin, max, col2, col1, col2, col1);
				break;
			case GradientDirection::GRADIENT_UP_RIGHT:
				imgui.drawlist->AddRectFilledMultiColor(origin, max, col1, col2, col1, col2);
				break;
		}
	}
	else {
		// For more than 2 colors, we need a more complex approach
		// Convert direction to points for the general implementation
		std::vector<gfx::Point> gradient_direction;

		switch (direction) {
			case GradientDirection::GRADIENT_RIGHT:
				gradient_direction = { gfx::Point(origin.x, origin.y), gfx::Point(max.x, origin.y) };
				break;
			case GradientDirection::GRADIENT_LEFT:
				gradient_direction = { gfx::Point(max.x, origin.y), gfx::Point(origin.x, origin.y) };
				break;
			case GradientDirection::GRADIENT_DOWN:
				gradient_direction = { gfx::Point(origin.x, origin.y), gfx::Point(origin.x, max.y) };
				break;
			case GradientDirection::GRADIENT_UP:
				gradient_direction = { gfx::Point(origin.x, max.y), gfx::Point(origin.x, origin.y) };
				break;
			case GradientDirection::GRADIENT_DOWN_RIGHT:
				gradient_direction = { gfx::Point(origin.x, origin.y), gfx::Point(max.x, max.y) };
				break;
			case GradientDirection::GRADIENT_UP_LEFT:
				gradient_direction = { gfx::Point(max.x, max.y), gfx::Point(origin.x, origin.y) };
				break;
			case GradientDirection::GRADIENT_DOWN_LEFT:
				gradient_direction = { gfx::Point(max.x, origin.y), gfx::Point(origin.x, max.y) };
				break;
			case GradientDirection::GRADIENT_UP_RIGHT:
				gradient_direction = { gfx::Point(origin.x, max.y), gfx::Point(max.x, origin.y) };
				break;
		}

		// Use the general implementation
		rect_filled_gradient(rect, gradient_direction, colors, positions);
	}
}

void render::rect_gradient_multi_filled(
	const gfx::Rect& rect,
	const gfx::Color& col_top_left,
	const gfx::Color& col_top_right,
	const gfx::Color& col_bottom_left,
	const gfx::Color& col_bottom_right
) {
	render::rect_stroke(rect, col_top_left);

	imgui.drawlist->AddRectFilledMultiColor(
		rect.top_left(),
		rect.bottom_right(),
		col_top_left.to_imgui(),
		col_top_right.to_imgui(),
		col_bottom_right.to_imgui(),
		col_bottom_left.to_imgui()
	);
}

void render::quadrilateral_filled(
	const gfx::Point& bottom_left,
	const gfx::Point& bottom_right,
	const gfx::Point& top_left,
	const gfx::Point& top_right,
	const gfx::Color& col
) {
	std::array<ImVec2, 5> positions = { top_left, bottom_left, bottom_right, top_right, top_left };

	imgui.drawlist->AddConvexPolyFilled(positions.data(), static_cast<int>(positions.size()), col.to_imgui());
}

void render::quadrilateral_stroke(
	const gfx::Point& bottom_left,
	const gfx::Point& bottom_right,
	const gfx::Point& top_left,
	const gfx::Point& top_right,
	const gfx::Color& col,
	float thickness
) {
	std::array<ImVec2, 5> positions = { top_left, bottom_left, bottom_right, top_right, top_left };

	imgui.drawlist->AddPolyline(
		positions.data(), static_cast<int>(positions.size()), col.to_imgui(), ImDrawFlags_Closed, thickness
	);
}

void render::triangle_filled(
	const gfx::Point& p1,
	const gfx::Point& p2,
	const gfx::Point& p3,
	const gfx::Color& col,
	float thickness,
	bool anti_aliased
) {
	imgui.drawlist->AddTriangleFilled(p1, p2, p3, col.to_imgui());
}

void render::triangle_stroke(
	const gfx::Point& p1,
	const gfx::Point& p2,
	const gfx::Point& p3,
	const gfx::Color& col,
	float thickness,
	bool anti_aliased
) {
	imgui.drawlist->AddTriangle(p1, p2, p3, col.to_imgui(), thickness);
}

void render::circle_filled(
	const gfx::Point& pos,
	float radius,
	const gfx::Color& colour,
	float thickness,
	int parts,
	float degrees,
	float start_degree,
	bool anti_aliased
) {
	imgui.drawlist->AddCircleFilled(pos, radius, colour.to_imgui(), parts);
}

void render::circle_stroke(
	const gfx::Point& pos,
	float radius,
	const gfx::Color& colour,
	float thickness,
	int parts,
	float degrees,
	float start_degree,
	bool anti_aliased
) {
	if (degrees != 360.f || start_degree != 0.f) {
		auto min_rad = u::deg_to_rad(start_degree);
		auto max_rad = u::deg_to_rad(degrees);
		imgui.drawlist->PathArcTo(pos, radius, min_rad, max_rad, parts);
		imgui.drawlist->PathStroke(colour.to_imgui(), 0, thickness);
		return;
	}

	imgui.drawlist->AddCircle(pos, radius, colour.to_imgui(), parts, thickness);
}

void render::text(
	gfx::Point pos,
	const gfx::Color& colour,
	const std::string& text,
	const Font& font,
	unsigned int flags,
	float rotation_deg,
	int rotation_pivot_y
) {
	if (!font)
		return;

	ImGui::PushFont(font.im_font());

	int vtx_idx_begin = imgui.drawlist->_VtxCurrentIdx;

	if (flags) {
		const auto size = font.calc_size(text);

		if (flags & FONT_CENTERED_X)
			pos.x -= int(size.w * 0.5f);

		if (flags & FONT_CENTERED_Y)
			pos.y -= int(size.h * 0.5f);

		if (flags & FONT_RIGHT_ALIGN)
			pos.x -= size.w;

		if (flags & FONT_BOTTOM_ALIGN)
			pos.y -= size.h;

		if (flags & FONT_OUTLINE) {
			const gfx::Color outline_colour(0, 0, 0, colour.a * 0.8f);
			static const std::array<gfx::Point, 4> offsets = {
				gfx::Point{ -1, 0 },
				gfx::Point{ 1, 0 },
				gfx::Point{ 0, -1 },
				gfx::Point{ 0, 1 },
			};

			for (const auto& offset : offsets) {
				imgui.drawlist->AddText(
					font.im_font(), font.size(), pos + offset, outline_colour.to_imgui(), text.data()
				);
			}
		}

		if (flags & FONT_DROPSHADOW) {
			const gfx::Color dropshadow_colour(0, 0, 0, colour.a * 0.6f);
			const int shift_amount = 1;

			imgui.drawlist->AddText(
				font.im_font(), font.size(), pos.offset(shift_amount), dropshadow_colour.to_imgui(), text.data()
			);
		}
	}

	imgui.drawlist->AddText(font.im_font(), font.size(), pos, colour.to_imgui(), text.data());

	if (rotation_deg != 0.f) {
		int vtx_idx_end = imgui.drawlist->_VtxCurrentIdx;

		auto text_size = font.calc_size(text);
		gfx::Rect text_rect(pos, gfx::Size(text_size.w, rotation_pivot_y));

		ImVec2 pivot_in = text_rect.center();
		ImVec2 pivot_out = text_rect.center();

		float ang = u::deg_to_rad(rotation_deg);
		ImGui::ShadeVertsTransformPos(
			imgui.drawlist, vtx_idx_begin, vtx_idx_end, pivot_in, std::cos(ang), std::sin(ang), pivot_out
		);
	}

	ImGui::PopFont();
}

render::Texture::~Texture() {
	destroy();
}

bool render::Texture::load_from_file(const std::string& path) {
	// Use SDL to load the image
	SDL_Surface* surface = IMG_Load(path.c_str());

	if (!surface) {
		u::log_error("Failed to load image: {}", SDL_GetError());
		return false;
	}

	// Convert to RGBA format if needed
	SDL_Surface* rgb_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
	SDL_DestroySurface(surface);

	if (!rgb_surface) {
		u::log_error("Failed to convert surface: {}", SDL_GetError());
		return false;
	}

	bool result = load_from_surface(rgb_surface);
	SDL_DestroySurface(rgb_surface);

	return result;
}

bool render::Texture::load_from_surface(SDL_Surface* surface) {
	if (!surface)
		return false;

	// Delete old texture if exists
	destroy();

	// Create a new texture
	glGenTextures(1, &m_id);
	glBindTexture(GL_TEXTURE_2D, m_id);

	// Setup texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Upload pixels into texture
	GLenum format = GL_RGBA;
	glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);

	m_width = surface->w;
	m_height = surface->h;

	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

void render::Texture::destroy() {
	if (m_id) {
		glDeleteTextures(1, &m_id);
		m_id = 0;
		m_width = 0;
		m_height = 0;
	}
}

void render::Texture::bind() const {
	glBindTexture(GL_TEXTURE_2D, m_id);
}

void render::Texture::unbind() {
	glBindTexture(GL_TEXTURE_2D, 0);
}

// Image rendering functions
void render::image(const gfx::Rect& rect, const Texture& texture, const gfx::Color& tint_color) {
	if (!texture.is_valid())
		return;

	imgui.drawlist->AddImage(
		texture.get_id(), rect.origin(), rect.max(), ImVec2(0, 0), ImVec2(1, 1), tint_color.to_imgui()
	);
}

void render::image_with_borders(
	const gfx::Rect& rect,
	const Texture& texture,
	const gfx::Color& border_color,
	const gfx::Color& inner_border_color,
	float border_thickness,
	const gfx::Color& tint_color
) {
	if (!texture.is_valid())
		return;

	image(rect.shrink(3), texture, tint_color);

	rect_stroke(rect.shrink(2), border_color, border_thickness);
	rect_stroke(rect.shrink(1), inner_border_color, border_thickness);
	rect_stroke(rect, border_color, border_thickness);
}

void render::image_rounded(
	const gfx::Rect& rect,
	const Texture& texture,
	float rounding,
	unsigned int rounding_flags,
	const gfx::Color& tint_color
) {
	if (!texture.is_valid())
		return;

	imgui.drawlist->AddImageRounded(
		texture.get_id(),
		rect.origin(),
		rect.max(),
		ImVec2(0, 0),
		ImVec2(1, 1),
		tint_color.to_imgui(),
		rounding,
		rounding_flags
	);
}

void render::rounded_image_with_borders(
	const gfx::Rect& rect,
	const Texture& texture,
	float rounding,
	const gfx::Color& border_color,
	const gfx::Color& inner_border_color,
	float border_thickness,
	unsigned int rounding_flags,
	const gfx::Color& tint_color
) {
	if (!texture.is_valid())
		return;

	image_rounded(rect.shrink(3), texture, rounding, rounding_flags, tint_color);

	rounded_rect_stroke(rect.shrink(2), border_color, rounding, rounding_flags, border_thickness);
	rounded_rect_stroke(rect.shrink(1), inner_border_color, rounding, rounding_flags, border_thickness);
	rounded_rect_stroke(rect, border_color, rounding, rounding_flags, border_thickness);
}

void render::push_clip_rect(const gfx::Rect& rect, bool intersect_clip_rect) {
	imgui.drawlist->PushClipRect(rect.origin(), rect.max(), intersect_clip_rect);
}

void render::push_clip_rect(int x1, int y1, int x2, int y2, bool intersect_clip_rect) {
	push_clip_rect(gfx::Rect(gfx::Point(x1, y1), gfx::Point(x2, y2)), intersect_clip_rect);
}

void render::push_fullscreen_clip_rect() {
	push_clip_rect(0, 0, window_size.w, window_size.h, false);
}

gfx::Rect render::pop_clip_rect() {
	ImVec2 min = imgui.drawlist->GetClipRectMin();
	ImVec2 max = imgui.drawlist->GetClipRectMax();

	imgui.drawlist->PopClipRect();

	return {
		gfx::Point(min.x, min.y),
		gfx::Point(max.x, max.y),
	};
}

gfx::Rect render::get_clip_rect() {
	ImVec2 min = imgui.drawlist->GetClipRectMin();
	ImVec2 max = imgui.drawlist->GetClipRectMax();

	return {
		gfx::Point(min.x, min.y),
		gfx::Point(max.x, max.y),
	};
}

bool render::clip_string(std::string& text, const Font& font, int max_width, int min_chars) {
	int size = font.calc_size(text).w;
	if (size <= max_width)
		return true;

	// bool start_right = size < max_width * 2;

	int dots_size = font.calc_size("...").w;

	const int num = static_cast<int>(text.size()) - min_chars;
	for (int i = 0; i < num; i++) {
		text.pop_back();

		if (font.calc_size(text).w + dots_size <= max_width) {
			text += "...";
			return true;
		}
	}

	text += "...";
	return false;
}

std::vector<std::string> render::wrap_text(
	const std::string& text, const gfx::Size& dimensions, const Font& font, int line_height
) {
	std::vector<std::string> lines;
	if (!font)
		return lines;

	std::istringstream iss(text);
	std::string word;
	std::string current_line;

	while (iss >> word) {
		std::string test_line = current_line;
		if (!test_line.empty())
			test_line += ' ';
		test_line += word;

		if (font.calc_size(test_line).w > dimensions.w) {
			if (!current_line.empty()) {
				lines.push_back(current_line);
				current_line = word;
			}
			else {
				// Word itself is too long, hard break
				std::string sub_word;
				for (char c : word) {
					sub_word += c;
					if (font.calc_size(sub_word).w > dimensions.w) {
						if (sub_word.length() > 1) {
							lines.push_back(sub_word.substr(0, sub_word.length() - 1));
							sub_word = sub_word.back();
						}
					}
				}
				current_line = sub_word;
			}
		}
		else {
			current_line = test_line;
		}
	}

	if (!current_line.empty()) {
		lines.push_back(current_line);
	}

	return lines;
}
