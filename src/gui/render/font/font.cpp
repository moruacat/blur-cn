#include "font.h"

#include <fstream>
#include <iterator>
#include <vector>

#include <imgui.h>
#include <misc/freetype/imgui_freetype.h>

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
	std::ifstream stream(path, std::ios::binary);
	if (!stream)
		return false;

	std::vector<unsigned char> data(
		(std::istreambuf_iterator<char>(stream)), (std::istreambuf_iterator<char>())
	);
	if (data.size() <= 100)
		return false;

	ImGuiIO* io = &ImGui::GetIO();
	m_size = size;

	ImFontConfig cfg = ImFontConfig();
	if (!font_cfg)
		font_cfg = &cfg;

	// ImGui copies the data into memory it owns when this is false.
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
