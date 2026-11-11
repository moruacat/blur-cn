#include "font.h"

#include <fstream>
#include <iterator>
#include <vector>

#include <imgui.h>
#include <misc/freetype/imgui_freetype.h>

namespace {
	// ImGui 1.92 rasterises glyphs on demand and only stores the pointer passed to
	// AddFontFromMemoryTTF, so the TTF bytes must outlive the atlas. Keep one copy
	// per path (shared between the different font sizes) for the whole process.
	std::vector<unsigned char>& font_file_data(const std::filesystem::path& path) {
		static std::map<std::string, std::vector<unsigned char>> cache;

		const std::string key = path.string();
		auto it = cache.find(key);
		if (it != cache.end())
			return it->second;

		std::ifstream stream(path, std::ios::binary);
		if (!stream)
			return cache[key];

		std::vector<unsigned char> data{
			(std::istreambuf_iterator<char>(stream)), (std::istreambuf_iterator<char>())
		};

		return cache.emplace(key, std::move(data)).first->second;
	}
}

bool render::Font::init(
	std::span<const unsigned char> data, float size, ImFontConfig* font_cfg, const ImWchar* glyph_ranges
) {
	ImGuiIO* io = &ImGui::GetIO();
	m_size = size;

	ImFontConfig cfg = ImFontConfig();
	if (!font_cfg)
		font_cfg = &cfg;

	font_cfg->FontDataOwnedByAtlas = false;

	// Pass pointer and size from the span
	m_font = io->Fonts->AddFontFromMemoryCompressedTTF(data.data(), data.size(), m_size, font_cfg, glyph_ranges);

	m_height = calc_size("Q").h;

	m_initialised = m_font != nullptr;
	return m_initialised;
}

bool render::Font::init_from_file(
	const std::filesystem::path& path, float size, ImFontConfig* font_cfg, const ImWchar* glyph_ranges
) {
	// Read the file ourselves so non-ASCII paths work on Windows. ImGui's
	// AddFontFromFileTTF uses fopen and would fail on those.
	std::vector<unsigned char>& data = font_file_data(path);
	if (data.size() <= 100)
		return false;

	ImGuiIO* io = &ImGui::GetIO();
	m_size = size;

	ImFontConfig cfg = ImFontConfig();
	if (!font_cfg)
		font_cfg = &cfg;

	// The buffer is owned by the cache above, not by the atlas.
	font_cfg->FontDataOwnedByAtlas = false;

	m_font = io->Fonts->AddFontFromMemoryTTF(data.data(), (int)data.size(), m_size, font_cfg, glyph_ranges);

	m_initialised = m_font != nullptr;
	m_height = m_initialised ? calc_size("Q").h : 0;
	return m_initialised;
}

gfx::Size render::Font::calc_size(const std::string& text) const {
	auto size = m_font->CalcTextSizeA(m_size, FLT_MAX, 0.f, text.c_str());
	return { (int)size.x, (int)size.y };
}
