#include "configs.h"

#include "../../ui/ui.h"
#include "../../render/render.h"

namespace configs = gui::components::configs;

void configs::option_information(ui::Container& container) {
	const static std::unordered_map<std::string, std::vector<std::string>> option_explanations = {
		// Blur settings
		// { "section blur checkbox",
		//   {
		// 	  "Enable motion blur",
		//   }, },
		{
			"blur amount",
			{
				"模糊强度",
				"（0 = 无模糊，1 = 完全混合所有帧，>1 = 混合额外帧（产生重影））",
			},
		},
		// { "output fps",
		//   {
		// 	  "FPS of the output video",
		//   }, },
		{
			"blur gamma",
			{
				"模糊前画面被压暗的程度，可让高光更突出",
			},
		},
		{
			"blur weighting gaussian std dev slider",
			{
				"高斯模糊权重的标准差",
			},
		},
		{
			"blur weighting triangle reverse checkbox",
			{
				"反转三角权重的方向",
			},
		},
		{
			"blur weighting bound input",
			{
				"权重边界，用于扩大权重分布范围",
			},
		},

		// Interpolation settings
		// { "section interpolation checkbox",
		//   {
		// 	  "Enable interpolation to a higher FPS before blurring",
		//   }, },
		{
			"interpolate scale checkbox",
			{
				"使用倍数而非固定帧率进行插帧",
			},
		},
		{
			"interpolated fps mult",
			{
				"插帧帧率倍数",
				"输入视频会先被插帧到该帧率（在模糊之前）",
			},
		},
		{
			"interpolated fps",
			{
				"将输入视频插帧到的帧率（在模糊之前）",
			},
		},
		{
			"interpolation method dropdown",
			{
				"质量：rife > svp",
				"速度：svp > rife",
			},
		},
		// pre-interp settings
		{
			"section pre-interpolation checkbox",
			{
				"在主插帧前使用更精确但更慢的 AI 模型进行预插帧",
			},
		},
		{
			"pre-interpolated fps mult",
			{
				"预插帧帧率倍数",
				"输入视频会先被插帧到该帧率（在主插帧和模糊之前）",
			},
		},
		{
			"pre-interpolated fps",
			{
				"将输入视频预插帧到的帧率（在模糊之前）",
			},
		},
		{
			"SVP interpolation preset dropdown",
			{
				"更多信息请查看 blur 的 GitHub",
			},
		},
		{
			"SVP interpolation algorithm dropdown",
			{
				"更多信息请查看 blur 的 GitHub",
			},
		},
		{
			"interpolation block size dropdown",
			{
				"插帧块大小",
				"（越高越快但精度越低；越低越精确但更慢）",
			},
		},
		{
			"interpolation mask area slider",
			{
				"插帧遮罩强度",
				"（提高可减少静态物体的模糊，但可能影响流畅度）",
			},
		},

		// Rendering settings
		{
			"deduplicate checkbox",
			{
				"移除重复帧并替换为插值帧",
				"（修复录像卡顿导致的输出不流畅）",
			},
		},
		{
			"deduplicate range",
			{
				"去重时在当前帧之后查找唯一帧的范围",
				"如果素材的实际帧率低于应有帧率（例如 120fps 的游戏画面被录制成 240fps），请调高此值",
				"如果模糊后的静态元素（如菜单界面）也开始模糊，请调低此值",
			},
		},
		{
			"deduplicate threshold input",
			{
				"触发去重的运动阈值",
				"在高级设置中开启调试并渲染视频，可在每帧中嵌入文字显示运动量",
			},
		},
		{
			"deduplicate method dropdown",
			{
				"质量：rife > svp",
				"速度：old > svp > rife",
			},
		},
		{
			"preview checkbox",
			{
				"渲染时显示预览",
			},
		},
		{
			"detailed filenames checkbox",
			{
				"在生成的文件名中加入模糊设置",
			},
		},

		// Timescale settings
		// {
		// 	"section timescale checkbox",
		// 	{
		// 		"Enable video timescale manipulation",
		// 	},
		// },
		// {
		// 	"input timescale",
		// 	{
		// 		"Timescale of the input video file",
		// 	},
		// },
		// {
		// 	"output timescale",
		// 	{
		// 		"Timescale of the output video file",
		// 	},
		// },
		{
			"adjust timescaled audio pitch checkbox",
			{
				"加速或减速视频时同步调整音频音调",
			},
		},

		// Filters
		// { "section filters checkbox", { "Enable video filters", }, },
		// { "brightness", { "Adjusts brightness of the output video", }, },
		// { "saturation", { "Adjusts saturation of the output video", }, },
		// { "contrast", { "Adjusts contrast of the output video", }, },

		// Advanced rendering
		// { "gpu interpolation checkbox", { "Uses GPU for interpolation", }, },
		// { "gpu encoding checkbox", { "Uses GPU for rendering", }, },
		// { "gpu encoding type dropdown", { "Select GPU type", }, },
		{
			"video container text input",
			{
				"输出视频的容器格式",
			},
		},
		{
			"custom ffmpeg filters text input",
			{
				"用于渲染的自定义 FFmpeg 滤镜",
				"（会覆盖 GPU 和质量选项）",
			},
		},
		// { "debug checkbox", { "Shows debug window and prints commands used by blur", } }
		{
			"copy dates checkbox",
			{
				"复制输入文件的修改日期",
			},
		},
	};

	std::string hovered = ui::get_hovered_id();

	if (hovered.empty())
		return;

	if (!option_explanations.contains(hovered))
		return;

	ui::add_hint("hovered option info", container, option_explanations.at(hovered), gfx::Color::white(), fonts::dejavu);
}
