#include "std_include.hpp"

namespace components
{
	constexpr auto INI_MAPNAME_ARG = 0;
	constexpr auto INI_FOG_DIST = 1;
	constexpr auto INI_FOG_COLOR_BEGIN = 2;
	constexpr auto INI_SUN_DIR_BEGIN = 5;
	constexpr auto INI_SUN_COLOR_BEGIN = 8;
	constexpr auto INI_SUN_INTENSITY = 11;
	constexpr auto INI_SKY_INDEX = 12;
	constexpr auto INI_ARGS_TOTAL = 13;


	void rtx_map_settings::set_settings_for_loaded_map(bool reload_settings)
	{
		DEBUG_PRINT("[DEBUG] # Function: map_settings::set_settings_for_loaded_map()\n");

		if ((m_settings.empty() || reload_settings) && !rtx_map_settings::load_settings())
		{
			return;
		}

		if (game::rgp->world && game::rgp->world->name)
		{
			std::string map_name = game::rgp->world->name;
			utils::replace_all(map_name, std::string("maps/mp/"), "");	// if mp map
			utils::replace_all(map_name, std::string("maps/"), "");		// if sp map
			utils::replace_all(map_name, std::string(".d3dbsp"), "");

			bool found = false;
			for (const auto& s : m_settings)
			{
				if (s.mapname == map_name)
				{
					m_loaded_map_settings = s;

					// 0-255 to 0-1 range
					utils::vector::scale3(m_loaded_map_settings.sun_color, (1.0f / 255.0f), m_loaded_map_settings.sun_color);

					rtx_gui::skysphere_spawn(s.sky_index);
					found = true;
					break;
				}
			}

			if (!found)
			{
				// grab map defaults
				if (game::gfx_world)
				{
					game::vec3_t sun_fwd = {};
					utils::vector::angle_vectors(game::gfx_world->sunParse.angles, sun_fwd, nullptr, nullptr);
					utils::vector::scale3(sun_fwd, -1.0f, sun_fwd);
					utils::vector::copy(sun_fwd, m_loaded_map_settings.sun_direction, 3);

					utils::vector::copy(game::gfx_world->sunParse.sunColor, m_loaded_map_settings.sun_color, 3);
					m_loaded_map_settings.sun_intensity = game::gfx_world->sunParse.sunLight;
				}

				if (!flags::has_flag("no_default_sky") && !rtx_gui::skysphere_is_valid())
				{
					rtx_gui::skysphere_spawn(rtx_gui::SKY::SUNSET);
				}
			}
		}
	}

	void rtx_map_settings::parse_culling()
	{
		// check if there are map settings
		bool map_settings_exist = false;
		map_settings_s* s = nullptr;

		// check if map settings exist
		for (auto& e : m_settings)
		{
			if (e.mapname._Equal(m_args[INI_MAPNAME_ARG]))
			{
				s = &e;
				map_settings_exist = true;
				break;
			}
		}

		// create defaults if not
		if (!map_settings_exist)
		{
			m_settings.push_back(map_settings_s(m_args[INI_MAPNAME_ARG]));
			s = &m_settings.back();
		}

		if (s)
		{
			// for each cell with its forced indices with format -> [cell](index index index)
			for (auto a = 1u; a < m_args.size(); a++)
			{
				const auto& str = m_args[a];

				if (str.empty())
				{
					// print msg here
					continue;
				}

				// which cell are we writing settings for?
				const auto cell_index = utils::try_stoi(utils::split_string_between_delims(str, '[', ']'), -1);
				if (cell_index != -1)
				{
					// check for duplicate cells
					bool ignore_current_cell = false;
					for (const auto& c : s->cell_settings)
					{
						if (cell_index == c.cell_index)
						{
							ignore_current_cell = true;
							break;
						}
					}

					// cell has been added already
					if (ignore_current_cell)
					{
						// print msg here
						continue;
					}

					// get inidices
					const auto indices_str = utils::split_string_between_delims(str, '(', ')');
					const auto split_indices = utils::split(indices_str, ' ');

					s->cell_settings.push_back(cell_settings_s(cell_index));
					const auto c = &s->cell_settings.back();

					// for each forced index
					for (const auto& i : split_indices)
					{
						c->forced_cell_indices.push_back(utils::try_stoi(i, -1));
					}
				}
			}
		}
	}

	void rtx_map_settings::parse_settings()
	{
		if (m_args.size() == INI_ARGS_TOTAL)
		{
			m_settings.push_back(
				{
					m_args[INI_MAPNAME_ARG],
					utils::try_stof(m_args[INI_FOG_DIST], 5000.0f),
					D3DCOLOR_XRGB
					(
						utils::try_stoi(m_args[INI_FOG_COLOR_BEGIN + 0], 255),
						utils::try_stoi(m_args[INI_FOG_COLOR_BEGIN + 1], 255),
						utils::try_stoi(m_args[INI_FOG_COLOR_BEGIN + 2], 255)
					),
					{
						utils::try_stof(m_args[INI_SUN_DIR_BEGIN + 0], 75.0f),
						utils::try_stof(m_args[INI_SUN_DIR_BEGIN + 1], -15.0f),
						utils::try_stof(m_args[INI_SUN_DIR_BEGIN + 2], -35.0f)
					},
					{
						utils::try_stof(m_args[INI_SUN_COLOR_BEGIN + 0], 255),
						utils::try_stof(m_args[INI_SUN_COLOR_BEGIN + 1], 255),
						utils::try_stof(m_args[INI_SUN_COLOR_BEGIN + 2], 255)
					},
					utils::try_stof(m_args[INI_SUN_INTENSITY], 1.0f),
					utils::try_stoi(m_args[INI_SKY_INDEX], 2)
				});
		}
	}

	bool rtx_map_settings::load_settings()
	{
		DEBUG_PRINT("[DEBUG] # Function: map_settings::load_settings()\n");

		m_settings.clear();
		m_settings.reserve(32);

		std::ifstream file;
		if (utils::fs::open_file_homepath("iw3xo\\rtx", "map_settings.ini", false, file))
		{
			std::string input;
			bool reading_cull_settings = false;

			// read line by line
			while (std::getline(file, input))
			{
				// ignore comment
				if (utils::starts_with(input, "//"))
				{
					continue;
				}

				if (!reading_cull_settings && utils::starts_with(input, "#CULL"))
				{
					reading_cull_settings = true;
					continue;
				}

				// split string on ','
				m_args = utils::split(input, ',');

				if (reading_cull_settings)
				{
					parse_culling();
				}
				else
				{
					parse_settings();
				}
			}

			file.close();
			return true;
		}

		return false;
	}

	rtx_map_settings::rtx_map_settings()
	{
		command::add("mapsettings_update", "", "reloads the map_settings.ini file in root/iw3xo/rtx", [this](const command::params&)
		{
			rtx_map_settings::set_settings_for_loaded_map(true);
		});

		command::add("mapsettings_get_defaults", "", "get sun direction, color and intensity in map_settings format based on the stock map settings", [this](const command::params&)
		{
			if (game::gfx_world)
			{
				game::vec3_t sun_fwd = {};
				utils::vector::angle_vectors(game::gfx_world->sunParse.angles, sun_fwd, nullptr, nullptr);
				utils::vector::scale3(sun_fwd, -1.0f, sun_fwd);

				game::vec3_t sun_color = {};
				utils::vector::scale3(game::gfx_world->sunParse.sunColor, 255.0f, sun_color);

				const auto str = utils::va("SunDir - SunColor - SunIntensity [%.3f,%.3f,%.3f,   %.1f,%.1f,%.1f,   %.2f]\n",
					sun_fwd[0], sun_fwd[1], sun_fwd[2], sun_color[0], sun_color[1], sun_color[2], game::gfx_world->sunParse.sunLight);

				game::Com_PrintMessage(0, str, 0);
				DEBUG_PRINT(str);
			}
			else
			{
				game::Com_PrintMessage(0, "No map loaded ..", 0);
			}
		});
	}
}