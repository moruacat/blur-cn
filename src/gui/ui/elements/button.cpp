#include "../ui.h"
#include "../../render/render.h"

#include "../keys.h"

// Sizes are computed on demand: they depend on the UI scale, which is only known
// after the OS display scale has been queried during startup.
namespace {
	gfx::Size button_padding() {
		return { render::ui_px(14), render::ui_px(9) };
	}

	float button_rounding() {
		return 7.f * render::ui_scale();
	}
}

void ui::render_button(const Container& container, const AnimatedElement& element) {
	const auto& button_data = std::get<ButtonElementData>(element.element->data);
	float anim = element.animations.at(hasher("main")).current;
	float hover_anim = element.animations.at(hasher("hover")).current;

	int shade = 17 + (20 * hover_anim);
	gfx::Color adjusted_color = gfx::Color(shade, shade, shade, anim * 255);
	gfx::Color adjusted_text_color = gfx::Color(255, 255, 255, anim * 255);

	// fill
	render::rounded_rect_filled(element.element->rect, adjusted_color, button_rounding());

	// border
	render::rounded_rect_stroke(element.element->rect, gfx::Color(100, 100, 100, anim * 255), button_rounding());

	render::text(
		element.element->rect.center(),
		adjusted_text_color,
		button_data.text,
		*button_data.font,
		FONT_CENTERED_X | FONT_CENTERED_Y
	);
}

bool ui::update_button(const Container& container, AnimatedElement& element) {
	const auto& button_data = std::get<ButtonElementData>(element.element->data);

	auto& anim = element.animations.at(hasher("hover"));

	bool hovered = element.element->rect.contains(keys::mouse_pos) && set_hovered_element(element);
	anim.set_goal(hovered ? 1.f : 0.f);

	if (hovered) {
		set_cursor(SDL_SYSTEM_CURSOR_POINTER);

		if (button_data.on_press) {
			if (keys::is_mouse_down()) {
				(*button_data.on_press)();
				keys::on_mouse_press_handled(SDL_BUTTON_LEFT);

				return true;
			}
		}
	}

	return false;
}

ui::AnimatedElement* ui::add_button(
	const std::string& id,
	Container& container,
	const std::string& text,
	const render::Font& font,
	std::optional<std::function<void()>> on_press
) {
	gfx::Size text_size = font.calc_size(text);

	gfx::Rect rect(
		container.current_position,
		gfx::Size(text_size.w + (button_padding().w * 2), font.height() + (button_padding().h * 2))
	);

	Element element(
		id,
		ElementType::BUTTON,
		rect,
		ButtonElementData{
			.text = text,
			.font = &font,
			.on_press = std::move(on_press),
		},
		render_button,
		update_button
	);

	return add_element(
		container,
		std::move(element),
		container.element_gap,
		{
			{ hasher("main"), AnimationState(25.f) },
			{ hasher("hover"), AnimationState(80.f) },
		}
	);
}
