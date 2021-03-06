/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Data loading and unloading functions.
 */

#include <algorithm>

#include "load.h"

#include "const.h"
#include "drawing.h"
#include "editors/area_editor/editor.h"
#include "functions.h"
#include "init.h"
#include "utils/string_utils.h"
#include "vars.h"


/* ----------------------------------------------------------------------------
 * Loads an area into memory.
 * name:            Name of the area's folder.
 * load_for_editor: If true, skips loading some things that the area editor
 *   won't need.
 * from_backup:     If true, load from a backup, if any.
 */
void load_area(
    const string &name, const bool load_for_editor, const bool from_backup
) {
	if (load_for_editor == false && day != 1) {
		data_node file = load_data_file(AREAS_FOLDER_PATH + "/Save" + i2s(day - 1) + ".txt");
		size_t flag_amounts = file.get_child_by_name("Flags")->get_nr_of_children();
		for (size_t f = 0; f < flag_amounts; ++f) {
			data_node* flags = file.get_child_by_name("Flags")->get_child(f);
			flagnames.push_back(flags->name);
			if (flags->value == "true") progressflags[flags->name] = true;
			else progressflags[flags->name] = false;
		}
	}
    cur_area_data.clear();
    
    string geometry_file_name;
    string data_file_name;
    if(from_backup) {
        geometry_file_name =
            USER_AREA_DATA_FOLDER_PATH + "/" + name +
            "/Geometry_backup.txt";
        data_file_name =
            USER_AREA_DATA_FOLDER_PATH + "/" + name +
            "/Data_backup.txt";
    } else {
        geometry_file_name =
            AREAS_FOLDER_PATH + "/" + name + "/Geometry.txt";
        data_file_name =
            AREAS_FOLDER_PATH + "/" + name + "/Data.txt";
    }
    
    data_node data_file = load_data_file(data_file_name);
    
    cur_area_data.name =
        data_file.get_child_by_name("name")->get_value_or_default(name);
    cur_area_data.subtitle =
        data_file.get_child_by_name("subtitle")->value;
    cur_area_data.creator =
        data_file.get_child_by_name("creator")->value;
    cur_area_data.version =
        data_file.get_child_by_name("version")->value;
    cur_area_data.notes =
        data_file.get_child_by_name("notes")->value;
    cur_area_data.spray_amounts =
        data_file.get_child_by_name("spray_amounts")->value;
	VERSUS_ON = s2b(data_file.get_child_by_name("versus")->value);
	max_score = s2i(data_file.get_child_by_name("auto_win")->value);
    if(loading_text_bmp) al_destroy_bitmap(loading_text_bmp);
    if(loading_subtext_bmp) al_destroy_bitmap(loading_subtext_bmp);
    loading_text_bmp = NULL;
    loading_subtext_bmp = NULL;
    
    draw_loading_screen(cur_area_data.name, cur_area_data.subtitle, 1.0);
    al_flip_display();
    
    cur_area_data.weather_name = data_file.get_child_by_name("weather")->value;
    if(!load_for_editor) {
    
        if(cur_area_data.weather_name.empty()) {
            cur_area_data.weather_condition = weather();
            
        } else if(
            weather_conditions.find(cur_area_data.weather_name) ==
            weather_conditions.end()
        ) {
            log_error(
                "Area " + name +
                " refers to a non-existing weather condition, \"" +
                cur_area_data.weather_name + "\"!",
                &data_file
            );
            cur_area_data.weather_condition = weather();
            
        } else {
            cur_area_data.weather_condition =
                weather_conditions[cur_area_data.weather_name];
                
        }
    }
    
    cur_area_data.bg_bmp_file_name =
        data_file.get_child_by_name("bg_bmp")->value;
    if(!load_for_editor && !cur_area_data.bg_bmp_file_name.empty()) {
        cur_area_data.bg_bmp =
            textures.get(cur_area_data.bg_bmp_file_name, &data_file);
    }
    cur_area_data.bg_color =
        s2c(data_file.get_child_by_name("bg_color")->value);
    cur_area_data.bg_dist =
        s2f(data_file.get_child_by_name("bg_dist")->get_value_or_default("2"));
    cur_area_data.bg_bmp_zoom =
        s2f(data_file.get_child_by_name("bg_zoom")->get_value_or_default("1"));
        
        
    data_node geometry_file = load_data_file(geometry_file_name);
    
    //Vertexes.
    size_t n_vertexes =
        geometry_file.get_child_by_name(
            "vertexes"
        )->get_nr_of_children_by_name("v");
    for(size_t v = 0; v < n_vertexes; ++v) {
        data_node* vertex_data =
            geometry_file.get_child_by_name(
                "vertexes"
            )->get_child_by_name("v", v);
        vector<string> words = split(vertex_data->value);
        if(words.size() == 2) {
            cur_area_data.vertexes.push_back(
                new vertex(s2f(words[0]), s2f(words[1]))
            );
        }
    }
    
    //Edges.
    size_t n_edges =
        geometry_file.get_child_by_name(
            "edges"
        )->get_nr_of_children_by_name("e");
    for(size_t e = 0; e < n_edges; ++e) {
        data_node* edge_data =
            geometry_file.get_child_by_name(
                "edges"
            )->get_child_by_name("e", e);
        edge* new_edge = new edge();
        
        vector<string> s_nrs = split(edge_data->get_child_by_name("s")->value);
        if(s_nrs.size() < 2) s_nrs.insert(s_nrs.end(), 2, "-1");
        for(size_t s = 0; s < 2; ++s) {
            if(s_nrs[s] == "-1") new_edge->sector_nrs[s] = INVALID;
            else new_edge->sector_nrs[s] = s2i(s_nrs[s]);
        }
        
        vector<string> v_nrs = split(edge_data->get_child_by_name("v")->value);
        if(v_nrs.size() < 2) v_nrs.insert(v_nrs.end(), 2, "0");
        
        new_edge->vertex_nrs[0] = s2i(v_nrs[0]);
        new_edge->vertex_nrs[1] = s2i(v_nrs[1]);
        
        cur_area_data.edges.push_back(new_edge);
    }
    
    //Sectors.
	if (day != 1 && load_for_editor == false) {
		string geoetry_name = AREAS_FOLDER_PATH + "/" + cur_area_data.name + "/" + "Sectors_on_day" + i2s(day - 1) + ".txt";

		data_node geoetry_file = load_data_file(geoetry_name);

		size_t n_sectors =
			geoetry_file.get_child_by_name(
				"sectors"
			)->get_nr_of_children_by_name("s");
		for (size_t s = 0; s < n_sectors; ++s) {
			data_node* sector_data =
				geoetry_file.get_child_by_name(
					"sectors"
				)->get_child_by_name("s", s);
			sector* new_sector = new sector();

			new_sector->type =
				sector_types.get_nr(sector_data->get_child_by_name("type")->value);
			if (new_sector->type == 255) new_sector->type = SECTOR_TYPE_NORMAL;
			new_sector->is_bottomless_pit =
				s2b(
					sector_data->get_child_by_name(
						"is_bottomless_pit"
					)->get_value_or_default("false")
				);
			new_sector->brightness =
				s2f(
					sector_data->get_child_by_name(
						"brightness"
					)->get_value_or_default(i2s(DEF_SECTOR_BRIGHTNESS))
				);
			new_sector->tag = sector_data->get_child_by_name("tag")->value;
			new_sector->z = s2f(sector_data->get_child_by_name("z")->value);
			new_sector->fade = s2b(sector_data->get_child_by_name("fade")->value);
			new_sector->always_cast_shadow =
				s2b(
					sector_data->get_child_by_name("always_cast_shadow")->value
				);

			new_sector->texture_info.file_name =
				sector_data->get_child_by_name("texture")->value;
			new_sector->texture_info.rot =
				s2f(sector_data->get_child_by_name("texture_rotate")->value);

			vector<string> scales =
				split(sector_data->get_child_by_name("texture_scale")->value);
			if (scales.size() >= 2) {
				new_sector->texture_info.scale.x = s2f(scales[0]);
				new_sector->texture_info.scale.y = s2f(scales[1]);
			}
			vector<string> translations =
				split(sector_data->get_child_by_name("texture_trans")->value);
			if (translations.size() >= 2) {
				new_sector->texture_info.translation.x = s2f(translations[0]);
				new_sector->texture_info.translation.y = s2f(translations[1]);
			}
			new_sector->texture_info.tint =
				s2c(
					sector_data->get_child_by_name("texture_tint")->
					get_value_or_default("255 255 255")
				);

			if (!new_sector->fade && !new_sector->is_bottomless_pit) {
				new_sector->texture_info.bitmap =
					textures.get(new_sector->texture_info.file_name, NULL);
			}

			data_node* hazards_node = sector_data->get_child_by_name("hazards");
			vector<string> hazards_strs =
				semicolon_list_to_vector(hazards_node->value);
			for (size_t h = 0; h < hazards_strs.size(); ++h) {
				string hazard_name = hazards_strs[h];
				if (hazards.find(hazard_name) == hazards.end()) {
					log_error(
						"Unknown hazard \"" + hazard_name +
						"\"!", hazards_node
					);
				}
				else {
					new_sector->hazards.push_back(&(hazards[hazard_name]));
				}
			}
			new_sector->hazards_str = hazards_node->value;
			new_sector->hazard_floor =
				s2b(
					sector_data->get_child_by_name(
						"hazards_floor"
					)->get_value_or_default("true")
				);
			cur_area_data.sectors.push_back(new_sector);
		}
	}else {
		size_t n_sectors =
			geometry_file.get_child_by_name(
				"sectors"
			)->get_nr_of_children_by_name("s");
		for (size_t s = 0; s < n_sectors; ++s) {
			data_node* sector_data =
				geometry_file.get_child_by_name(
					"sectors"
				)->get_child_by_name("s", s);
			sector* new_sector = new sector();

			new_sector->type =
				sector_types.get_nr(sector_data->get_child_by_name("type")->value);
			if (new_sector->type == 255) new_sector->type = SECTOR_TYPE_NORMAL;
			new_sector->is_bottomless_pit =
				s2b(
					sector_data->get_child_by_name(
						"is_bottomless_pit"
					)->get_value_or_default("false")
				);
			new_sector->brightness =
				s2f(
					sector_data->get_child_by_name(
						"brightness"
					)->get_value_or_default(i2s(DEF_SECTOR_BRIGHTNESS))
				);
			new_sector->tag = sector_data->get_child_by_name("tag")->value;
			new_sector->z = s2f(sector_data->get_child_by_name("z")->value);
			new_sector->fade = s2b(sector_data->get_child_by_name("fade")->value);
			new_sector->always_cast_shadow =
				s2b(
					sector_data->get_child_by_name("always_cast_shadow")->value
				);

			new_sector->texture_info.file_name =
				sector_data->get_child_by_name("texture")->value;
			new_sector->texture_info.rot =
				s2f(sector_data->get_child_by_name("texture_rotate")->value);

			vector<string> scales =
				split(sector_data->get_child_by_name("texture_scale")->value);
			if (scales.size() >= 2) {
				new_sector->texture_info.scale.x = s2f(scales[0]);
				new_sector->texture_info.scale.y = s2f(scales[1]);
			}
			vector<string> translations =
				split(sector_data->get_child_by_name("texture_trans")->value);
			if (translations.size() >= 2) {
				new_sector->texture_info.translation.x = s2f(translations[0]);
				new_sector->texture_info.translation.y = s2f(translations[1]);
			}
			new_sector->texture_info.tint =
				s2c(
					sector_data->get_child_by_name("texture_tint")->
					get_value_or_default("255 255 255")
				);

			if (!new_sector->fade && !new_sector->is_bottomless_pit) {
				new_sector->texture_info.bitmap =
					textures.get(new_sector->texture_info.file_name, NULL);
			}

			data_node* hazards_node = sector_data->get_child_by_name("hazards");
			vector<string> hazards_strs =
				semicolon_list_to_vector(hazards_node->value);
			for (size_t h = 0; h < hazards_strs.size(); ++h) {
				string hazard_name = hazards_strs[h];
				if (hazards.find(hazard_name) == hazards.end()) {
					log_error(
						"Unknown hazard \"" + hazard_name +
						"\"!", hazards_node
					);
				}
				else {
					new_sector->hazards.push_back(&(hazards[hazard_name]));
				}
			}
			new_sector->hazards_str = hazards_node->value;
			new_sector->hazard_floor =
				s2b(
					sector_data->get_child_by_name(
						"hazards_floor"
					)->get_value_or_default("true")
				);

			cur_area_data.sectors.push_back(new_sector);
		}
	}
    //Mobs.
    vector<pair<size_t, size_t> > mob_links_buffer;

	if (day != 1 && load_for_editor == false) {
		load_day(name, true, false, i2s(day));
		for (size_t f = 0; f < flagnames.size(); ++f) {
			if (progressflags[flagnames[f]] == false) {
				load_mobs_from_flags(name, false, false, flagnames[f]);
			}
			else if(progressflags[flagnames[f]] == true){
				load_mobs_from_flags(name, false, true, flagnames[f]);

			}
		}
	}
	else if(day == 1){
		size_t n_mobs =
			geometry_file.get_child_by_name("mobs")->get_nr_of_children();

		for (size_t m = 0; m < n_mobs; ++m) {

			data_node* mob_node =
				geometry_file.get_child_by_name("mobs")->get_child(m);

			mob_gen* mob_ptr = new mob_gen();

			mob_ptr->pos = s2p(mob_node->get_child_by_name("p")->value);
			mob_ptr->angle =
				s2f(
					mob_node->get_child_by_name("angle")->get_value_or_default("0")
				);
			mob_ptr->vars = mob_node->get_child_by_name("vars")->value;

			mob_ptr->category = mob_categories.get_from_name(mob_node->name);
			if (!mob_ptr->category) continue;

			string mt = mob_node->get_child_by_name("type")->value;
			mob_ptr->type = mob_ptr->category->get_type(mt);

			vector<string> link_strs =
				split(mob_node->get_child_by_name("links")->value);
			for (size_t l = 0; l < link_strs.size(); ++l) {
				mob_links_buffer.push_back(make_pair(m, s2i(link_strs[l])));
			}

			bool problem = false;

			if (!mob_ptr->type && !load_for_editor) {
				//Error.
				log_error(
					"Unknown \"" + mob_ptr->category->name +
					"\" mob type \"" +
					mt + "\"!",
					mob_node
				);
				problem = true;
			}

			if (
				(
					mob_ptr->category->id == MOB_CATEGORY_NONE ||
					mob_ptr->category->id == INVALID
					) && !load_for_editor
				) {

				log_error(
					"Unknown mob category \"" + mob_node->name + "\"!", mob_node
				);
				mob_ptr->category = mob_categories.get(MOB_CATEGORY_NONE);
				problem = true;

			}

			if (!problem) {
				cur_area_data.mob_generators.push_back(mob_ptr);
			}
			else {
				delete mob_ptr;
			}
		}

		for (size_t l = 0; l < mob_links_buffer.size(); ++l) {
			size_t f = mob_links_buffer[l].first;
			size_t s = mob_links_buffer[l].second;
			cur_area_data.mob_generators[f]->links.push_back(
				cur_area_data.mob_generators[s]
			);
			cur_area_data.mob_generators[f]->link_nrs.push_back(s);
		}
	}
    //Path stops.
    size_t n_stops =
        geometry_file.get_child_by_name("path_stops")->get_nr_of_children();
    for(size_t s = 0; s < n_stops; ++s) {
    
        data_node* path_stop_node =
            geometry_file.get_child_by_name("path_stops")->get_child(s);
            
        path_stop* s_ptr = new path_stop();
        
        vector<string> words =
            split(path_stop_node->get_child_by_name("pos")->value);
        s_ptr->pos.x = (words.size() >= 1 ? s2f(words[0]) : 0);
        s_ptr->pos.y = (words.size() >= 2 ? s2f(words[1]) : 0);
        
        data_node* links_node = path_stop_node->get_child_by_name("links");
        size_t n_links = links_node->get_nr_of_children();
        
        for(size_t l = 0; l < n_links; ++l) {
        
            data_node* link_node = links_node->get_child(l);
            path_link l_struct(NULL, INVALID);
            
            l_struct.end_nr = s2i(link_node->value);
            
            s_ptr->links.push_back(l_struct);
            
        }
        
        cur_area_data.path_stops.push_back(s_ptr);
    }
    
    
    //Tree shadows.
    size_t n_shadows =
        geometry_file.get_child_by_name("tree_shadows")->get_nr_of_children();
    for(size_t s = 0; s < n_shadows; ++s) {
    
        data_node* shadow_node =
            geometry_file.get_child_by_name("tree_shadows")->get_child(s);
            
        tree_shadow* s_ptr = new tree_shadow();
        
        vector<string> words =
            split(shadow_node->get_child_by_name("pos")->value);
        s_ptr->center.x = (words.size() >= 1 ? s2f(words[0]) : 0);
        s_ptr->center.y = (words.size() >= 2 ? s2f(words[1]) : 0);
        
        words = split(shadow_node->get_child_by_name("size")->value);
        s_ptr->size.x = (words.size() >= 1 ? s2f(words[0]) : 0);
        s_ptr->size.y = (words.size() >= 2 ? s2f(words[1]) : 0);
        
        s_ptr->angle =
            s2f(
                shadow_node->get_child_by_name(
                    "angle"
                )->get_value_or_default("0")
            );
        s_ptr->alpha =
            s2i(
                shadow_node->get_child_by_name(
                    "alpha"
                )->get_value_or_default("255")
            );
        s_ptr->file_name = shadow_node->get_child_by_name("file")->value;
        s_ptr->bitmap = textures.get(s_ptr->file_name, NULL);
        
        words = split(shadow_node->get_child_by_name("sway")->value);
        s_ptr->sway.x = (words.size() >= 1 ? s2f(words[0]) : 0);
        s_ptr->sway.y = (words.size() >= 2 ? s2f(words[1]) : 0);
        
        if(s_ptr->bitmap == bmp_error && !load_for_editor) {
            log_error(
                "Unknown tree shadow texture \"" + s_ptr->file_name + "\"!",
                shadow_node
            );
        }
        
        cur_area_data.tree_shadows.push_back(s_ptr);
        
    }
    
    
    //Set up stuff.
    for(size_t e = 0; e < cur_area_data.edges.size(); ++e) {
        cur_area_data.fix_edge_pointers(cur_area_data.edges[e]);
    }
    for(size_t s = 0; s < cur_area_data.sectors.size(); ++s) {
        cur_area_data.connect_sector_edges(cur_area_data.sectors[s]);
    }
    for(size_t v = 0; v < cur_area_data.vertexes.size(); ++v) {
        cur_area_data.connect_vertex_edges(cur_area_data.vertexes[v]);
    }
    for(size_t s = 0; s < cur_area_data.path_stops.size(); ++s) {
        cur_area_data.fix_path_stop_pointers(cur_area_data.path_stops[s]);
    }
    for(size_t s = 0; s < cur_area_data.path_stops.size(); ++s) {
        cur_area_data.path_stops[s]->calculate_dists();
    }
    if(!load_for_editor) {
        //Fade sectors that also fade brightness should be
        //at midway between the two neighbors.
        for(size_t s = 0; s < cur_area_data.sectors.size(); ++s) {
            sector* s_ptr = cur_area_data.sectors[s];
            if(s_ptr->fade) {
                sector* n1 = NULL;
                sector* n2 = NULL;
                s_ptr->get_texture_merge_sectors(&n1, &n2);
                if(n1 && n2) {
                    s_ptr->brightness = (n1->brightness + n2->brightness) / 2;
                }
            }
        }
    }
    
    
    //Triangulate everything and save bounding boxes.
    for(size_t s = 0; s < cur_area_data.sectors.size(); ++s) {
        sector* s_ptr = cur_area_data.sectors[s];
        s_ptr->triangles.clear();
        //TODO report lone edges and such to the editor.
        triangulate(s_ptr, NULL, false, false);
        
        get_sector_bounding_box(s_ptr, &s_ptr->bbox[0], &s_ptr->bbox[1]);
    }
    
    if(!load_for_editor) cur_area_data.generate_blockmap();
}


/* ----------------------------------------------------------------------------
 * Loads the area's sector textures.
 */
void load_area_textures() {
    //TODO will this still be needed after area editor v2?
    for(size_t s = 0; s < cur_area_data.sectors.size(); ++s) {
        sector* s_ptr = cur_area_data.sectors[s];
        
        if(s_ptr->texture_info.file_name.empty()) {
            s_ptr->texture_info.bitmap = NULL;
        } else {
            s_ptr->texture_info.bitmap =
                textures.get(s_ptr->texture_info.file_name, NULL);
        }
    }
}


/* ----------------------------------------------------------------------------
 * Loads asset file names.
 */
void load_asset_file_names() {
    data_node file(SYSTEM_ASSET_FILE_NAMES_FILE_PATH);
    
    reader_setter rs(&file);
    
    rs.set("area_name_font", asset_file_names.area_name_font);
    rs.set("checkbox_check", asset_file_names.checkbox_check);
    rs.set("cursor", asset_file_names.cursor);
    rs.set("cursor_invalid", asset_file_names.cursor_invalid);
    rs.set("counter_font", asset_file_names.counter_font);
    rs.set("editor_icons", asset_file_names.editor_icons);
    rs.set("enemy_spirit", asset_file_names.enemy_spirit);
    rs.set("group_move_arrow", asset_file_names.group_move_arrow);
    rs.set("icon", asset_file_names.icon);
    rs.set("idle_glow", asset_file_names.idle_glow);
    rs.set("main_font", asset_file_names.main_font);
    rs.set("main_menu", asset_file_names.main_menu);
    rs.set("mouse_cursor", asset_file_names.mouse_cursor);
    rs.set("mouse_wheel_down_icon", asset_file_names.mouse_wd_icon);
    rs.set("mouse_wheel_up_icon", asset_file_names.mouse_wu_icon);
    rs.set("notification", asset_file_names.notification);
    rs.set("pikmin_silhouette", asset_file_names.pikmin_silhouette);
    rs.set("pikmin_spirit", asset_file_names.pikmin_spirit);
    rs.set("shadow", asset_file_names.shadow);
    rs.set("smack", asset_file_names.smack);
    rs.set("smoke", asset_file_names.smoke);
    rs.set("sparkle", asset_file_names.sparkle);
    rs.set("spotlight", asset_file_names.spotlight);
    rs.set("value_font", asset_file_names.value_font);
    rs.set("wave_ring", asset_file_names.wave_ring);
    
    for(unsigned char i = 0; i < 3; ++i) {
        rs.set(
            "mouse_button_" + i2s(i + 1) + "_icon",
            asset_file_names.mouse_button_icon[i]
        );
    }
    
}


/* ----------------------------------------------------------------------------
 * Loads a bitmap from the game's content.
 * file_name:          File name of the bitmap.
 * node:               If present, it will be used to report errors, if any.
 * report_error:       If false, omits error reporting.
 * error_bmp_on_error: If true, returns the error bitmap in the case of an
 *   error. Otherwise, returns NULL.
 * error_bmp_on_empty: If true, returns the error bitmap in the case of an
 *   empty file name. Otherwise, returns NULL.
 * path_from_root:     Normally, files are fetched from the images folder.
 *   If this parameter is true, the path starts from the game's root.
 */
ALLEGRO_BITMAP* load_bmp(
    const string &file_name, data_node* node,
    const bool report_error, const bool error_bmp_on_error,
    const bool error_bmp_on_empty, const bool path_from_root
) {
    if(file_name.empty()) {
        if(error_bmp_on_empty) {
            return bmp_error;
        } else {
            return NULL;
        }
    }
    
    string base_dir = (path_from_root ? "" : (GRAPHICS_FOLDER_PATH + "/"));
    ALLEGRO_BITMAP* b =
        al_load_bitmap((base_dir + file_name).c_str());
        
    if(!b) {
        if(report_error) {
            log_error("Could not open image " + file_name + "!", node);
        }
        if(error_bmp_on_error) {
            b = bmp_error;
        }
    }
    
    return b;
}


/* ----------------------------------------------------------------------------
 * Loads a game control.
 */
void load_control(
    const unsigned char action, const unsigned char player,
    const string &name, data_node &file, const string &def
) {
    string s =
        file.get_child_by_name(
            "p" + i2s((player + 1)) + "_" + name
        )->get_value_or_default((player == 0) ? def : "");
    vector<string> possible_controls = semicolon_list_to_vector(s);
    size_t n_possible_controls = possible_controls.size();
    
    for(size_t c = 0; c < n_possible_controls; ++c) {
        controls[player].push_back(control_info(action, possible_controls[c]));
    }
}


/* ----------------------------------------------------------------------------
 * Loads the creator tools from the tool config file.
 */
void load_creator_tools() {
    data_node file(CREATOR_TOOLS_FILE_PATH);
    
    if(!file.file_was_opened) return;
    
    creator_tools_enabled = s2b(file.get_child_by_name("enabled")->value);
    
    for(unsigned char k = 0; k < 20; k++) {
        string tool_name;
        if(k < 10) {
            //The first ten indexes are the F2 - F11 keys.
            tool_name = file.get_child_by_name("f" + i2s(k + 2))->value;
        } else {
            //The second ten indexes are the 0 - 9 keys.
            tool_name = file.get_child_by_name(i2s(k - 10))->value;
        }
        
        if(tool_name == "area_image") {
            creator_tool_keys[k] = CREATOR_TOOL_AREA_IMAGE;
        } else if(tool_name == "change_speed") {
            creator_tool_keys[k] = CREATOR_TOOL_CHANGE_SPEED;
        } else if(tool_name == "geometry_info") {
            creator_tool_keys[k] = CREATOR_TOOL_GEOMETRY_INFO;
        } else if(tool_name == "hitboxes") {
            creator_tool_keys[k] = CREATOR_TOOL_HITBOXES;
        } else if(tool_name == "hurt_mob") {
            creator_tool_keys[k] = CREATOR_TOOL_HURT_MOB;
        } else if(tool_name == "mob_info") {
            creator_tool_keys[k] = CREATOR_TOOL_MOB_INFO;
        } else if(tool_name == "new_pikmin") {
            creator_tool_keys[k] = CREATOR_TOOL_NEW_PIKMIN;
        } else if(tool_name == "teleport") {
            creator_tool_keys[k] = CREATOR_TOOL_TELEPORT;
        } else {
            creator_tool_keys[k] = CREATOR_TOOL_NONE;
        }
    }
    
    creator_tool_area_image_mobs =
        s2b(file.get_child_by_name("area_image_mobs")->value);
    creator_tool_area_image_shadows =
        s2b(file.get_child_by_name("area_image_shadows")->value);
    creator_tool_area_image_size =
        s2i(file.get_child_by_name("area_image_size")->value);
    creator_tool_change_speed_mult =
        s2f(file.get_child_by_name("change_speed_multiplier")->value);
    creator_tool_mob_hurting_ratio =
        s2f(file.get_child_by_name("mob_hurting_percentage")->value) / 100;
        
    creator_tool_auto_start_option =
        file.get_child_by_name("auto_start_option")->value;
    creator_tool_auto_start_mode =
        file.get_child_by_name("auto_start_mode")->value;
        
}


/* ----------------------------------------------------------------------------
 * Loads the user-made particle generators.
 */
void load_custom_particle_generators(const bool load_resources) {
    custom_particle_generators.clear();
    
    data_node file(PARTICLE_GENERATORS_FILE_PATH);
    
    size_t n_pg = file.get_nr_of_children();
    for(size_t pg = 0; pg < n_pg; ++pg) {
    
        data_node* pg_node = file.get_child(pg);
        data_node* p_node = pg_node->get_child_by_name("base");
        
        reader_setter grs(pg_node);
        reader_setter prs(p_node);
        
        float emission_interval;
        size_t number;
        string bitmap_name;
        particle base_p;
        base_p.priority = PARTICLE_PRIORITY_MEDIUM;
        
        grs.set("emission_interval", emission_interval);
        grs.set("number", number);
        
        prs.set("bitmap", bitmap_name);
        if(bitmap_name.empty()) {
            base_p.type = PARTICLE_TYPE_CIRCLE;
        } else {
            if(load_resources) {
                base_p.bitmap =
                    bitmaps.get(
                        bitmap_name, p_node->get_child_by_name("bitmap")
                    );
            }
            base_p.type = PARTICLE_TYPE_BITMAP;
        }
        prs.set("duration",        base_p.duration);
        prs.set("friction",        base_p.friction);
        prs.set("gravity",         base_p.gravity);
        prs.set("size_grow_speed", base_p.size_grow_speed);
        prs.set("size",            base_p.size);
        prs.set("speed",           base_p.speed);
        prs.set("color",           base_p.color);
        base_p.time = base_p.duration;
        
        particle_generator pg_struct(emission_interval, base_p, number);
        
        grs.set("number_deviation",      pg_struct.number_deviation);
        grs.set("duration_deviation",    pg_struct.duration_deviation);
        grs.set("friction_deviation",    pg_struct.friction_deviation);
        grs.set("gravity_deviation",     pg_struct.gravity_deviation);
        grs.set("size_deviation",        pg_struct.size_deviation);
        grs.set("pos_deviation",         pg_struct.pos_deviation);
        grs.set("speed_deviation",       pg_struct.speed_deviation);
        grs.set("angle",                 pg_struct.angle);
        grs.set("angle_deviation",       pg_struct.angle_deviation);
        grs.set("total_speed",           pg_struct.total_speed);
        grs.set("total_speed_deviation", pg_struct.total_speed_deviation);
        
        pg_struct.angle = deg_to_rad(pg_struct.angle);
        pg_struct.angle_deviation = deg_to_rad(pg_struct.angle_deviation);
        
        pg_struct.id = MOB_PARTICLE_GENERATOR_STATUS + pg;
        
        custom_particle_generators[pg_node->name] = pg_struct;
    }
}


/* ----------------------------------------------------------------------------
 * Loads a data file from the game's content.
 */
data_node load_data_file(const string &file_name) {
    data_node n = data_node(file_name);
    if(!n.file_was_opened) {
        log_error("Could not open data file " + file_name + "!");
    }
    
    return n;
}


/* ----------------------------------------------------------------------------
 * Loads the game's fonts.
 */
void load_fonts() {
    const int STANDARD_FONT_RANGES_SIZE = 2;
    int standard_font_ranges[STANDARD_FONT_RANGES_SIZE] = {
        0x0020, 0x007E, //ASCII
        /*0x00A0, 0x00A1, //Non-breaking space and inverted !
        0x00BF, 0x00FF, //Inverted ? and European vowels and such*/
    };
    
    const int COUNTER_FONT_RANGES_SIZE = 6;
    int counter_font_ranges[COUNTER_FONT_RANGES_SIZE] = {
        0x002D, 0x002D, //Dash
        0x002F, 0x0039, //Slash and numbers
        0x0078, 0x0078, //x
    };
    
    const int VALUE_FONT_RANGES_SIZE = 6;
    int value_font_ranges[VALUE_FONT_RANGES_SIZE] = {
        0x0024, 0x0024, //Dollar sign
        0x002D, 0x002D, //Dash
        0x0030, 0x0039, //Numbers
    };
    
    //We can't load the fonts directly because we want to set the ranges.
    //So we load them into bitmaps first.
    
    //Main font.
    ALLEGRO_BITMAP* temp_font_bmp = load_bmp(asset_file_names.main_font);
    if(temp_font_bmp) {
        font_main =
            al_grab_font_from_bitmap(
                temp_font_bmp,
                STANDARD_FONT_RANGES_SIZE / 2, standard_font_ranges
            );
    }
    al_destroy_bitmap(temp_font_bmp);
    
    //Area name font.
    temp_font_bmp = load_bmp(asset_file_names.area_name_font);
    if(temp_font_bmp) {
        font_area_name =
            al_grab_font_from_bitmap(
                temp_font_bmp,
                STANDARD_FONT_RANGES_SIZE / 2, standard_font_ranges
            );
    }
    al_destroy_bitmap(temp_font_bmp);
    
    //Counter font.
    temp_font_bmp = load_bmp(asset_file_names.counter_font);
    if(temp_font_bmp) {
        font_counter =
            al_grab_font_from_bitmap(
                temp_font_bmp,
                COUNTER_FONT_RANGES_SIZE / 2, counter_font_ranges
            );
    }
    al_destroy_bitmap(temp_font_bmp);
    
    //Value font.
    temp_font_bmp = load_bmp(asset_file_names.value_font);
    if(temp_font_bmp) {
        font_value =
            al_grab_font_from_bitmap(
                temp_font_bmp,
                VALUE_FONT_RANGES_SIZE / 2, value_font_ranges
            );
    }
    al_destroy_bitmap(temp_font_bmp);
    
    if(font_main) font_main_h = al_get_font_line_height(font_main);
    if(font_counter) font_counter_h = al_get_font_line_height(font_counter);
    
    font_builtin = al_create_builtin_font();
}


/* ----------------------------------------------------------------------------
 * Loads the game's configuration file.
 */
void load_game_config() {
    data_node file = load_data_file(CONFIG_FILE);
    
    reader_setter rs(&file);
    string pikmin_order_string;
    string leader_order_string;
    
    rs.set("game_name", game_name);
    rs.set("game_version", game_version);
    
    rs.set("carrying_color_move", carrying_color_move);
    rs.set("carrying_color_stop", carrying_color_stop);
    rs.set("carrying_speed_base_mult", carrying_speed_base_mult);
    rs.set("carrying_speed_max_mult", carrying_speed_max_mult);
    rs.set("carrying_speed_weight_mult", carrying_speed_weight_mult);
    
    rs.set("day_minutes_start", day_minutes_start);
    rs.set("day_minutes_end", day_minutes_end);
    rs.set("day_minutes_per_irl_sec", day_minutes_per_irl_sec);
    
    rs.set("pikmin_order", pikmin_order_string);
    rs.set("standard_pikmin_height", standard_pikmin_height);
    rs.set("standard_pikmin_radius", standard_pikmin_radius);
    
    rs.set("leader_order", leader_order_string);
    
    rs.set("idle_task_range", idle_task_range);
    rs.set("group_move_task_range", group_move_task_range);
    rs.set("pikmin_chase_range", pikmin_chase_range);
    rs.set("max_pikmin_in_field", max_pikmin_in_field);
    rs.set("maturity_power_mult", maturity_power_mult);
    rs.set("maturity_speed_mult", maturity_speed_mult);
    
    rs.set("can_throw_leaders", can_throw_leaders);
    rs.set("cursor_max_dist", cursor_max_dist);
    rs.set("cursor_spin_speed", cursor_spin_speed);
    rs.set("next_pluck_range", next_pluck_range);
    rs.set("onion_open_range", onion_open_range);
    rs.set("pikmin_grab_range", pikmin_grab_range);
    rs.set("pluck_range", pluck_range);
    rs.set("whistle_growth_speed", whistle_growth_speed);
    
    rs.set("message_char_interval", message_char_interval);
    rs.set("zoom_max_level", zoom_max_level);
    rs.set("zoom_min_level", zoom_min_level);
    
    al_set_window_title(display, game_name.c_str());
    
    pikmin_order_strings = semicolon_list_to_vector(pikmin_order_string);
    leader_order_strings = semicolon_list_to_vector(leader_order_string);
    cursor_spin_speed = deg_to_rad(cursor_spin_speed);
	size_t flag_amounts = file.get_child_by_name("Flags")->get_nr_of_children();
	for (size_t f = 0; f < flag_amounts; ++f) {
		data_node* flags = file.get_child_by_name("Flags")->get_child(f);
		flagnames.push_back(flags->name);
		if (flags->value == "true") progressflags[flags->name] = true;
		else progressflags[flags->name] = false;
	}
	day = 1;
}



/* ----------------------------------------------------------------------------
 * Loads the hazards from the game data.
 */
void load_hazards() {
    data_node file = load_data_file(MISC_FOLDER_PATH + "/Hazards.txt");
    if(!file.file_was_opened) return;
    
    size_t n_hazards = file.get_nr_of_children();
    for(size_t h = 0; h < n_hazards; ++h) {
        data_node* h_node = file.get_child(h);
        hazard h_struct;
        
        h_struct.name = h_node->name;
        
        data_node* effects_node = h_node->get_child_by_name("effects");
        vector<string> effects_strs =
            semicolon_list_to_vector(effects_node->value);
        for(size_t e = 0; e < effects_strs.size(); ++e) {
            string effect_name = effects_strs[e];
            if(status_types.find(effect_name) == status_types.end()) {
                log_error(
                    "Unknown status effect \"" + effect_name + "\"!",
                    effects_node
                );
            } else {
                h_struct.effects.push_back(&(status_types[effect_name]));
            }
        }
        data_node* l_node = h_node->get_child_by_name("liquid");
        if(!l_node->value.empty()) {
            if(liquids.find(l_node->value) == liquids.end()) {
                log_error(
                    "Liquid \"" + l_node->value + "\" not found!",
                    l_node
                );
            } else {
                h_struct.associated_liquid = &(liquids[l_node->value]);
            }
        }
        
        reader_setter(h_node).set("color", h_struct.main_color);
        
        hazards[h_node->name] = h_struct;
    }
}


/* ----------------------------------------------------------------------------
 * Loads the liquids from the game data.
 */
void load_liquids(const bool load_resources) {
    data_node file = load_data_file(MISC_FOLDER_PATH + "/Liquids.txt");
    if(!file.file_was_opened) return;
    
    map<string, data_node*> nodes;
    
    size_t n_liquids = file.get_nr_of_children();
    for(size_t l = 0; l < n_liquids; ++l) {
        data_node* l_node = file.get_child(l);
        liquid l_struct;
        
        l_struct.name = l_node->name;
        reader_setter rs(l_node);
        rs.set("color", l_struct.main_color);
        rs.set("surface_1_speed", l_struct.surface_speed[0]);
        rs.set("surface_2_speed", l_struct.surface_speed[0]);
        rs.set("surface_alpha", l_struct.surface_alpha);
        
        liquids[l_node->name] = l_struct;
        nodes[l_node->name] = l_node;
    }
    
    if(load_resources) {
        for(auto l = liquids.begin(); l != liquids.end(); ++l) {
            data_node anim_file =
                load_data_file(
                    ANIMATIONS_FOLDER_PATH + "/" +
                    nodes[l->first]->get_child_by_name("animation")->value
                );
            l->second.anim_db =
                load_animation_database_from_file(&anim_file);
            if(!l->second.anim_db.animations.empty()) {
                l->second.anim_instance =
                    animation_instance(&l->second.anim_db);
                l->second.anim_instance.cur_anim =
                    l->second.anim_db.animations[0];
                l->second.anim_instance.start();
            }
        }
    }
}


/* ----------------------------------------------------------------------------
 * Loads miscellaneous fixed graphics.
 */
void load_misc_graphics() {
    //Icon.
    bmp_icon = load_bmp(asset_file_names.icon);
    al_set_display_icon(display, bmp_icon);
    
    //Graphics.
    bmp_checkbox_check = load_bmp(   asset_file_names.checkbox_check);
    bmp_cursor = load_bmp(           asset_file_names.cursor);
    bmp_cursor_invalid = load_bmp(   asset_file_names.cursor_invalid);
    bmp_enemy_spirit = load_bmp(     asset_file_names.enemy_spirit);
    bmp_idle_glow = load_bmp(        asset_file_names.idle_glow);
    bmp_mouse_cursor = load_bmp(     asset_file_names.mouse_cursor);
    bmp_mouse_wd_icon = load_bmp(    asset_file_names.mouse_wd_icon);
    bmp_mouse_wu_icon = load_bmp(    asset_file_names.mouse_wu_icon);
    bmp_notification = load_bmp(     asset_file_names.notification);
    bmp_group_move_arrow = load_bmp( asset_file_names.group_move_arrow);
    bmp_pikmin_silhouette = load_bmp(asset_file_names.pikmin_silhouette);
    bmp_pikmin_spirit = load_bmp(    asset_file_names.pikmin_spirit);
    bmp_rock = load_bmp(             asset_file_names.rock);
    bmp_shadow = load_bmp(           asset_file_names.shadow);
    bmp_smack = load_bmp(            asset_file_names.smack);
    bmp_smoke = load_bmp(            asset_file_names.smoke);
    bmp_sparkle = load_bmp(          asset_file_names.sparkle);
    bmp_spotlight = load_bmp(        asset_file_names.spotlight);
    bmp_wave_ring = load_bmp(        asset_file_names.wave_ring);
    for(unsigned char i = 0; i < 3; ++i) {
        bmp_mouse_button_icon[i] =
            load_bmp(asset_file_names.mouse_button_icon[i]);
    }
}


/* ----------------------------------------------------------------------------
 * Loads miscellaneous fixed sound effects.
 */
void load_misc_sounds() {
    //Sound effects.
    voice =
        al_create_voice(
            44100, ALLEGRO_AUDIO_DEPTH_INT16,   ALLEGRO_CHANNEL_CONF_2
        );
    mixer =
        al_create_mixer(
            44100, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2
        );
    al_attach_mixer_to_voice(mixer, voice);
    sfx_attack = load_sample(              "Attack.ogg",               mixer);
    sfx_pikmin_attack = load_sample(       "Pikmin_attack.ogg",        mixer);
    sfx_pikmin_carrying = load_sample(     "Pikmin_carrying.ogg",      mixer);
    sfx_pikmin_carrying_grab = load_sample("Pikmin_carrying_grab.ogg", mixer);
    sfx_pikmin_caught = load_sample(       "Pikmin_caught.ogg",        mixer);
    sfx_pikmin_dying = load_sample(        "Pikmin_dying.ogg",         mixer);
    sfx_pikmin_held = load_sample(         "Pikmin_held.ogg",          mixer);
    sfx_pikmin_idle = load_sample(         "Pikmin_idle.ogg",          mixer);
    sfx_pikmin_thrown = load_sample(       "Pikmin_thrown.ogg",        mixer);
    sfx_pikmin_plucked = load_sample(      "Pikmin_plucked.ogg",       mixer);
    sfx_pikmin_called = load_sample(       "Pikmin_called.ogg",        mixer);
    sfx_olimar_whistle = load_sample(      "Olimar_whistle.ogg",       mixer);
    sfx_louie_whistle = load_sample(       "Louie_whistle.ogg",        mixer);
    sfx_president_whistle = load_sample(   "President_whistle.ogg",    mixer);
    sfx_olimar_name_call = load_sample(    "Olimar_name_call.ogg",     mixer);
    sfx_louie_name_call = load_sample(     "Louie_name_call.ogg",      mixer);
    sfx_president_name_call = load_sample( "President_name_call.ogg",  mixer);
    sfx_pluck = load_sample(               "Pluck.ogg",                mixer);
    sfx_throw = load_sample(               "Throw.ogg",                mixer);
    sfx_switch_pikmin = load_sample(       "Switch_Pikmin.ogg",        mixer);
    sfx_camera = load_sample(              "Camera.ogg",               mixer);
}


/* ----------------------------------------------------------------------------
 * Loads the player's options.
 */
void load_options() {
    for(size_t h = 0; h < ANIMATION_EDITOR_HISTORY_SIZE; ++h) {
        animation_editor_history.push_back("");
    }
	
    data_node file = data_node(OPTIONS_FILE_PATH);
    if(!file.file_was_opened) return;
	string vuil;

	max_players =
		s2i(
			file.get_child_by_name(
				"playernumber"
			)->value
	);
    //Init joysticks.
    joystick_numbers.clear();
    int n_joysticks = al_get_num_joysticks();
    for(int j = 0; j < n_joysticks; ++j) {
        joystick_numbers[al_get_joystick(j)] = j;
    }
    
    /* Load controls.
     * Format of a control:
     * "p<player>_<action>=<possible control 1>,<possible control 2>,<...>"
     * Format of a possible control:
     * "<input method>_<parameters, underscore separated>"
     * Input methods:
     * "k" (keyboard key), "mb" (mouse button),
     * "mwu" (mouse wheel up), "mwd" (down),
     * "mwl" (left), "mwr" (right), "jb" (joystick button),
     * "jap" (joystick axis, positive), "jan" (joystick axis, negative).
     * The parameters are the key/button number, joystick number,
     * joystick stick and axis, etc.
     * Check the constructor of control_info for more information.
     */
    for(unsigned char p = 0; p < max_players; ++p) {
        controls[p].clear();
        for(size_t b = 0; b < N_BUTTONS; ++b) {
            string option_name = buttons.list[b].option_name;
            if(option_name.empty()) continue;
            load_control(buttons.list[b].id, p, option_name, file);
        }
    }
    
    //Weed out controls that didn't parse correctly.
    for(size_t p = 0; p < max_players; p++) {
        size_t n_controls = controls[p].size();
        for(size_t c = 0; c < n_controls; ) {
            if(controls[p][c].action == BUTTON_NONE) {
                controls[p].erase(controls[p].begin() + c);
            } else {
                c++;
            }
        }
    }
    
    for(unsigned char p = 0; p < max_players; ++p) {
        mouse_moves_cursor[p] =
            s2b(
                file.get_child_by_name(
                    "p" + i2s((p + 1)) + "_mouse_moves_cursor"
                )->value
            );
    }
    
    //Other options.
    reader_setter rs(&file);
    string resolution_str;
    rs.set("area_editor_backup_interval", area_editor_backup_interval);
    rs.set("area_editor_grid_interval", area_editor_grid_interval);
    rs.set("area_editor_show_edge_length", area_editor_show_edge_length);
    rs.set("area_editor_undo_limit", area_editor_undo_limit);
    rs.set("area_editor_view_mode", area_editor_view_mode);
    rs.set("draw_cursor_trail", draw_cursor_trail);
    rs.set("editor_mmb_pan", editor_mmb_pan);
    rs.set("editor_mouse_drag_threshold", editor_mouse_drag_threshold);
    rs.set("fps", game_fps);
    rs.set("fullscreen", scr_fullscreen);
    rs.set("joystick_min_deadzone", joystick_min_deadzone);
    rs.set("joystick_max_deadzone", joystick_max_deadzone);
    rs.set("max_particles", max_particles);
    rs.set("middle_zoom_level", zoom_mid_level);
    rs.set("mipmaps", mipmaps_enabled);
    rs.set("pretty_whistle", pretty_whistle);
    rs.set("resolution", resolution_str);
    rs.set("smooth_scaling", smooth_scaling);
    rs.set("window_position_hack", window_position_hack);
    
    game_fps = max(1, game_fps);
    joystick_min_deadzone = clamp(joystick_min_deadzone, 0.0f, 1.0f);
    joystick_max_deadzone = clamp(joystick_max_deadzone, 0.0f, 1.0f);
    if(joystick_min_deadzone > joystick_max_deadzone) {
        swap(joystick_min_deadzone, joystick_max_deadzone);
    }
    
    vector<string> resolution_parts = split(resolution_str);
    if(resolution_parts.size() >= 2) {
        scr_w = max(1, s2i(resolution_parts[0]));
        scr_h = max(1, s2i(resolution_parts[1]));
    }
    
    for(size_t h = 0; h < ANIMATION_EDITOR_HISTORY_SIZE; ++h) {
        rs.set(
            "animation_editor_history_" + i2s(h + 1),
            animation_editor_history[h]
        );
    }
    
    intended_scr_fullscreen = scr_fullscreen;
    intended_scr_w = scr_w;
    intended_scr_h = scr_h;
    
}


/* ----------------------------------------------------------------------------
 * Loads an audio sample from the game's content.
 */
sample_struct load_sample(
    const string &file_name, ALLEGRO_MIXER* const mixer
) {
    ALLEGRO_SAMPLE* sample =
        al_load_sample((AUDIO_FOLDER_PATH + "/" + file_name).c_str());
    if(!sample) {
        log_error("Could not open audio sample " + file_name + "!");
    }
    
    return sample_struct(sample, mixer);
}


/* ----------------------------------------------------------------------------
 * Loads the spike damage types available.
 */
void load_spike_damage_types() {
    data_node types_file = load_data_file(SPIKE_DAMAGE_TYPES_FILE_PATH);
    size_t n_types =
        types_file.get_nr_of_children();
        
    for(size_t t = 0; t < n_types; ++t) {
        data_node* type_node = types_file.get_child(t);
        spike_damage_type s_type;
        
        s_type.name = type_node->name;
        s_type.damage =
            s2f(type_node->get_child_by_name("damage")->value);
        s_type.ingestion_only =
            s2b(type_node->get_child_by_name("ingestion_only")->value);
        s_type.is_damage_ratio =
            s2b(type_node->get_child_by_name("is_damage_ratio")->value);
            
        data_node* pg_node = type_node->get_child_by_name("particle_generator");
        string pg_name = pg_node->value;
        if(!pg_name.empty()) {
            if(
                custom_particle_generators.find(pg_name) ==
                custom_particle_generators.end()
            ) {
                log_error(
                    "Unknown particle generator \"" +
                    pg_name + "\"!", pg_node
                );
            } else {
                s_type.particle_gen = &custom_particle_generators[pg_name];
                s_type.particle_offset_pos =
                    s2p(
                        type_node->get_child_by_name("particle_offset")->value,
                        &s_type.particle_offset_z
                    );
            }
        }
        
        if(s_type.damage == 0) {
            log_error(
                "Spike damage type \"" + s_type.name +
                "\" needs a damage number!",
                type_node
            );
        }
        
        spike_damage_types[s_type.name] = s_type;
    }
}


/* ----------------------------------------------------------------------------
 * Loads spray types from the game data.
 */
void load_spray_types(const bool load_resources) {
    data_node file = data_node(MISC_FOLDER_PATH + "/Sprays.txt");
    if(!file.file_was_opened) return;
    
    size_t n_sprays = file.get_nr_of_children();
    for(size_t s = 0; s < n_sprays; ++s) {
        data_node* s_node = file.get_child(s);
        spray_type st;
        
        st.name = s_node->name;
        
        data_node* effects_node = s_node->get_child_by_name("effects");
        vector<string> effects_strs =
            semicolon_list_to_vector(effects_node->value);
        for(size_t e = 0; e < effects_strs.size(); ++e) {
            string effect_name = effects_strs[e];
            if(status_types.find(effect_name) == status_types.end()) {
                log_error(
                    "Unknown status effect \"" + effect_name + "\"!",
                    effects_node
                );
            } else {
                st.effects.push_back(&(status_types[effect_name]));
            }
        }
        
        reader_setter rs(s_node);
        rs.set("group", st.group);
        rs.set("angle", st.angle);
        rs.set("distance_range", st.distance_range);
        rs.set("angle_range", st.angle_range);
        rs.set("color", st.main_color);
        rs.set("ingredients_needed", st.ingredients_needed);
        rs.set("buries_pikmin", st.buries_pikmin);
        
        st.angle = deg_to_rad(st.angle);
        st.angle_range = deg_to_rad(st.angle_range);
        
        if(load_resources) {
            data_node* icon_node = s_node->get_child_by_name("icon");
            st.bmp_spray = bitmaps.get(icon_node->value, icon_node);
        }
        
        spray_types.push_back(st);
    }
}


/* ----------------------------------------------------------------------------
 * Loads status effect types from the game data.
 */
void load_status_types(const bool load_resources) {
    data_node file = data_node(MISC_FOLDER_PATH + "/Statuses.txt");
    if(!file.file_was_opened) return;
    
    size_t n_statuses = file.get_nr_of_children();
    for(size_t s = 0; s < n_statuses; ++s) {
        data_node* s_node = file.get_child(s);
        status_type st;
        
        st.name = s_node->name;
        
        string particle_offset_str;
        
        reader_setter rs(s_node);
        rs.set("color",                   st.color);
        rs.set("tint",                    st.tint);
        rs.set("glow",                    st.glow);
        rs.set("removable_with_whistle",  st.removable_with_whistle);
        rs.set("auto_remove_time",        st.auto_remove_time);
        rs.set("health_change_ratio",     st.health_change_ratio);
        rs.set("causes_disable",          st.causes_disable);
        rs.set("causes_flailing",         st.causes_flailing);
        rs.set("causes_panic",            st.causes_panic);
        rs.set("disabled_state_inedible", st.disabled_state_inedible);
        rs.set("speed_multiplier",        st.speed_multiplier);
        rs.set("attack_multiplier",       st.attack_multiplier);
        rs.set("defense_multiplier",      st.defense_multiplier);
        rs.set("maturity_change_amount",  st.maturity_change_amount);
        rs.set("disables_attack",         st.disables_attack);
        rs.set("turns_invisible",         st.turns_invisible);
        rs.set("anim_speed_multiplier",   st.anim_speed_multiplier);
        rs.set("animation",               st.animation_name);
        rs.set("animation_mob_scale",     st.animation_mob_scale);
        rs.set("particle_offset",         particle_offset_str);
        
        st.affects = 0;
        if(s2b(s_node->get_child_by_name("affects_pikmin")->value)) {
            st.affects |= STATUS_AFFECTS_PIKMIN;
        }
        if(s2b(s_node->get_child_by_name("affects_leaders")->value)) {
            st.affects |= STATUS_AFFECTS_LEADERS;
        }
        if(s2b(s_node->get_child_by_name("affects_enemies")->value)) {
            st.affects |= STATUS_AFFECTS_ENEMIES;
        }
        if(s2b(s_node->get_child_by_name("affects_others")->value)) {
            st.affects |= STATUS_AFFECTS_OTHERS;
        }
        
        data_node* pg_node = s_node->get_child_by_name("particle_generator");
        string pg_name = pg_node->value;
        if(!pg_name.empty()) {
            if(
                custom_particle_generators.find(pg_name) ==
                custom_particle_generators.end()
            ) {
                log_error(
                    "Unknown particle generator \"" +
                    pg_name + "\"!", pg_node
                );
            } else {
                st.generates_particles = true;
                st.particle_gen = &custom_particle_generators[pg_name];
                st.particle_offset_pos =
                    s2p(particle_offset_str, &st.particle_offset_z);
            }
        }
        
        status_types[st.name] = st;
    }
    
    if(load_resources) {
        for(auto s = status_types.begin(); s != status_types.end(); ++s) {
            if(s->second.animation_name.empty()) continue;
            data_node anim_file =
                load_data_file(
                    ANIMATIONS_FOLDER_PATH + "/" + s->second.animation_name
                );
            s->second.anim_db = load_animation_database_from_file(&anim_file);
            if(!s->second.anim_db.animations.empty()) {
                s->second.anim_instance =
                    animation_instance(&s->second.anim_db);
                s->second.anim_instance.cur_anim =
                    s->second.anim_db.animations[0];
                s->second.anim_instance.start();
            }
        }
    }
}


/* ----------------------------------------------------------------------------
 * Loads the animations that are used system-wide.
 */
void load_system_animations() {
    data_node system_animations_file =
        load_data_file(SYSTEM_ANIMATIONS_FILE_PATH);
        
    init_single_animation(
        &system_animations_file, "leader_damage_sparks", spark_animation
    );
}


/* ----------------------------------------------------------------------------
 * Loads the weather conditions available.
 */
void load_weather() {
    data_node weather_file = load_data_file(WEATHER_FILE_PATH);
    size_t n_weather_conditions =
        weather_file.get_nr_of_children();
        
    for(size_t wc = 0; wc < n_weather_conditions; ++wc) {
        data_node* weather_node = weather_file.get_child(wc);
        weather weather_struct;
        
        weather_struct.name = weather_node->name;
        
        //Lighting.
        vector<pair<size_t, string> > lighting_table =
            get_weather_table(weather_node->get_child_by_name("lighting"));
            
        for(size_t p = 0; p < lighting_table.size(); ++p) {
            weather_struct.daylight.push_back(
                make_pair(
                    lighting_table[p].first,
                    s2c(lighting_table[p].second)
                )
            );
        }
        
        //Sun's strength.
        vector<pair<size_t, string> > sun_strength_table =
            get_weather_table(weather_node->get_child_by_name("sun_strength"));
            
        for(size_t p = 0; p < sun_strength_table.size(); ++p) {
            weather_struct.sun_strength.push_back(
                make_pair(
                    sun_strength_table[p].first,
                    s2i(sun_strength_table[p].second)
                )
            );
        }
        
        //Blackout effect's strength.
        vector<pair<size_t, string> > blackout_strength_table =
            get_weather_table(
                weather_node->get_child_by_name("blackout_strength")
            );
            
        for(size_t p = 0; p < blackout_strength_table.size(); ++p) {
            weather_struct.blackout_strength.push_back(
                make_pair(
                    blackout_strength_table[p].first,
                    s2i(blackout_strength_table[p].second)
                )
            );
        }
        
        //Fog.
        weather_struct.fog_near =
            s2f(weather_node->get_child_by_name("fog_near")->value);
        weather_struct.fog_far =
            s2f(weather_node->get_child_by_name("fog_far")->value);
        weather_struct.fog_near = max(weather_struct.fog_near, 0.0f);
        weather_struct.fog_far =
            max(weather_struct.fog_far, weather_struct.fog_near);
            
        vector<pair<size_t, string> > fog_color_table =
            get_weather_table(
                weather_node->get_child_by_name("fog_color")
            );
        for(size_t p = 0; p < fog_color_table.size(); ++p) {
            weather_struct.fog_color.push_back(
                make_pair(
                    fog_color_table[p].first,
                    s2c(fog_color_table[p].second)
                )
            );
        }
        
        //Precipitation.
        weather_struct.precipitation_type =
            s2i(
                weather_node->get_child_by_name(
                    "precipitation_type"
                )->get_value_or_default(i2s(PRECIPITATION_TYPE_NONE))
            );
        weather_struct.precipitation_frequency =
            interval(
                weather_node->get_child_by_name(
                    "precipitation_frequency"
                )->value
            );
        weather_struct.precipitation_speed =
            interval(
                weather_node->get_child_by_name(
                    "precipitation_speed"
                )->value
            );
        weather_struct.precipitation_angle =
            interval(
                weather_node->get_child_by_name(
                    "precipitation_angle"
                )->get_value_or_default(f2s((TAU * 0.75)))
            );
            
        //Save it in the map.
        weather_conditions[weather_struct.name] = weather_struct;
    }
}


/* ----------------------------------------------------------------------------
 * Unloads the loaded area from memory.
 */
void unload_area() {
    cur_area_data.clear();
}


/* ----------------------------------------------------------------------------
 * Unloads the loaded area's sector textures from memory.
 */
void unload_area_textures() {
    for(size_t s = 0; s < cur_area_data.sectors.size(); ++s) {
        sector* s_ptr = cur_area_data.sectors[s];
        
        if(s_ptr->texture_info.file_name.empty()) continue;
        
        textures.detach(s_ptr->texture_info.file_name);
        s_ptr->texture_info.file_name.clear();
        s_ptr->texture_info.bitmap = NULL;
    }
}


/* ----------------------------------------------------------------------------
 * Unloads custom particle generators loaded from memory.
 */
void unload_custom_particle_generators() {
    for(
        auto g = custom_particle_generators.begin();
        g != custom_particle_generators.end();
        ++g
    ) {
        bitmaps.detach(g->second.base_particle.bitmap);
    }
    custom_particle_generators.clear();
}


/* ----------------------------------------------------------------------------
 * Unloads hazards loaded in memory.
 */
void unload_hazards() {
    hazards.clear();
}


/* ----------------------------------------------------------------------------
 * Unloads loaded liquids from memory.
 */
void unload_liquids() {
    for(auto l = liquids.begin(); l != liquids.end(); ++l) {
        l->second.anim_db.destroy();
    }
    liquids.clear();
}


/* ----------------------------------------------------------------------------
 * Unloads miscellaneous graphics, sounds, and other resources.
 */
void unload_misc_resources() {
    al_destroy_bitmap(bmp_checkbox_check);
    al_destroy_bitmap(bmp_cursor);
    al_destroy_bitmap(bmp_cursor_invalid);
    al_destroy_bitmap(bmp_enemy_spirit);
    al_destroy_bitmap(bmp_icon);
    al_destroy_bitmap(bmp_idle_glow);
    al_destroy_bitmap(bmp_mouse_wd_icon);
    al_destroy_bitmap(bmp_mouse_wu_icon);
    al_destroy_bitmap(bmp_mouse_cursor);
    al_destroy_bitmap(bmp_notification);
    al_destroy_bitmap(bmp_group_move_arrow);
    al_destroy_bitmap(bmp_pikmin_spirit);
    al_destroy_bitmap(bmp_rock);
    al_destroy_bitmap(bmp_shadow);
    al_destroy_bitmap(bmp_smack);
    al_destroy_bitmap(bmp_smoke);
    al_destroy_bitmap(bmp_sparkle);
    for(unsigned char i = 0; i < 3; ++i) {
        bitmaps.detach(bmp_mouse_button_icon[i]);
    }
    
    sfx_attack.destroy();
    sfx_pikmin_attack.destroy();
    sfx_pikmin_carrying.destroy();
    sfx_pikmin_carrying_grab.destroy();
    sfx_pikmin_caught.destroy();
    sfx_pikmin_dying.destroy();
    sfx_pikmin_held.destroy();
    sfx_pikmin_idle.destroy();
    sfx_pikmin_thrown.destroy();
    sfx_pikmin_plucked.destroy();
    sfx_pikmin_called.destroy();
    sfx_olimar_whistle.destroy();
    sfx_louie_whistle.destroy();
    sfx_president_whistle.destroy();
    sfx_olimar_name_call.destroy();
    sfx_louie_name_call.destroy();
    sfx_president_name_call.destroy();
    sfx_throw.destroy();
    sfx_switch_pikmin.destroy();
    sfx_camera.destroy();
}


/* ----------------------------------------------------------------------------
 * Unloads spike damage types loaded in memory.
 */
void unload_spike_damage_types() {
    spike_damage_types.clear();
}


/* ----------------------------------------------------------------------------
 * Unloads loaded spray types from memory.
 */
void unload_spray_types() {
    for(size_t s = 0; s < spray_types.size(); ++s) {
        bitmaps.detach(spray_types[s].bmp_spray);
    }
    spray_types.clear();
}


/* ----------------------------------------------------------------------------
 * Unloads loaded status effect types from memory.
 */
void unload_status_types(const bool unload_resources) {

    if(unload_resources) {
        for(auto s = status_types.begin(); s != status_types.end(); ++s) {
            s->second.anim_db.destroy();
        }
    }
    status_types.clear();
}

void load_day( const string &name , const bool load_for_editor, const bool from_backup, const string &day){
if(load_for_editor == true) cur_area_data.mob_generators.clear();
    string gen_file_name;
	if (!from_backup) {
		gen_file_name =
			AREAS_FOLDER_PATH + "/" + name + "/" + day + "/this.txt";
	} else gen_file_name =
		USER_AREA_DATA_FOLDER_PATH + "/" + name + "/" + day + "/this_backup.txt";

data_node gen_file = load_data_file(gen_file_name);
vector<pair<size_t, size_t> > mob_links_buffer;
    size_t n_mobs =
        gen_file.get_child_by_name("mobs")->get_nr_of_children();
        
    for(size_t m = 0; m < n_mobs; ++m) {
    
        data_node* mob_node =
            gen_file.get_child_by_name("mobs")->get_child(m);
            
        mob_gen* mob_ptr = new mob_gen();
        
        mob_ptr->pos = s2p(mob_node->get_child_by_name("p")->value);
        mob_ptr->angle =
            s2f(
                mob_node->get_child_by_name("angle")->get_value_or_default("0")
            );
        mob_ptr->vars = mob_node->get_child_by_name("vars")->value;
        
        mob_ptr->category = mob_categories.get_from_name(mob_node->name);
        if(!mob_ptr->category) continue;
        
        string mt = mob_node->get_child_by_name("type")->value;
        mob_ptr->type = mob_ptr->category->get_type(mt);
        vector<string> link_strs =
            split(mob_node->get_child_by_name("links")->value);
        for(size_t l = 0; l < link_strs.size(); ++l) {
            mob_links_buffer.push_back(make_pair(m, s2i(link_strs[l])));
        }
        
        bool problem = false;
        
        if(!mob_ptr->type && !load_for_editor) {
            //Error.
            log_error(
                "Unknown \"" + mob_ptr->category->name +
                "\" mob type \"" +
                mt + "\"!",
                mob_node
            );
            problem = true;
        }
        
        if(
            (
                mob_ptr->category->id == MOB_CATEGORY_NONE ||
                mob_ptr->category->id == INVALID
            ) && !load_for_editor
        ) {
        
            log_error(
                "Unknown mob category \"" + mob_node->name + "\"!", mob_node
            );
            mob_ptr->category = mob_categories.get(MOB_CATEGORY_NONE);
            problem = true;
            
        }
        
        if(!problem) {
            cur_area_data.mob_generators.push_back(mob_ptr);
        } else {
            delete mob_ptr;
        }
    }
    
    for(size_t l = 0; l < mob_links_buffer.size(); ++l) {
        size_t f = mob_links_buffer[l].first;
        size_t s = mob_links_buffer[l].second;
        cur_area_data.mob_generators[f]->links.push_back(
            cur_area_data.mob_generators[s]
        );
        cur_area_data.mob_generators[f]->link_nrs.push_back(s);
    }
 }
/*load mobs based on level flags*/
void load_mobs_from_flags(const string &name, const bool load_for_editor,
	const bool from_backup, const string &flag) {
	if (load_for_editor == true) cur_area_data.mob_generators.clear();
	string gen_file_name;
	if (!from_backup) {
		gen_file_name =
			AREAS_FOLDER_PATH + "/" + name + "/flagdata/" + "this.txt";
	}
	else gen_file_name =
	AREAS_FOLDER_PATH + "/" + name + "/flagdata_false/" + "this.txt";
	data_node* gen_file;
	data_node geb_file = load_data_file(gen_file_name);
	if (geb_file.get_child_by_name(flag)->name != flag) {
		return;
	}
	else gen_file = geb_file.get_child_by_name(flag);
	vector<pair<size_t, size_t> > mob_links_buffer;
	size_t n_mobs =
		gen_file->get_child_by_name("mobs")->get_nr_of_children();

	for (size_t m = 0; m < n_mobs; ++m) {

		data_node* mob_node =
			gen_file->get_child_by_name("mobs")->get_child(m);

		mob_gen* mob_ptr = new mob_gen();

		mob_ptr->pos = s2p(mob_node->get_child_by_name("p")->value);
		mob_ptr->angle =
			s2f(
				mob_node->get_child_by_name("angle")->get_value_or_default("0")
			);
		mob_ptr->vars = mob_node->get_child_by_name("vars")->value;

		mob_ptr->category = mob_categories.get_from_name(mob_node->name);
		if (!mob_ptr->category) continue;

		string mt = mob_node->get_child_by_name("type")->value;
		mob_ptr->type = mob_ptr->category->get_type(mt);

		vector<string> link_strs =
			split(mob_node->get_child_by_name("links")->value);
		for (size_t l = 0; l < link_strs.size(); ++l) {
			mob_links_buffer.push_back(make_pair(m, s2i(link_strs[l])));
		}

		bool problem = false;

		if (!mob_ptr->type && !load_for_editor) {
			//Error.
			log_error(
				"Unknown \"" + mob_ptr->category->name +
				"\" mob type \"" +
				mt + "\"!",
				mob_node
			);
			problem = true;
		}

		if (
			(
				mob_ptr->category->id == MOB_CATEGORY_NONE ||
				mob_ptr->category->id == INVALID
				) && !load_for_editor
			) {

			log_error(
				"Unknown mob category \"" + mob_node->name + "\"!", mob_node
			);
			mob_ptr->category = mob_categories.get(MOB_CATEGORY_NONE);
			problem = true;

		}

		if (!problem) {
			cur_area_data.mob_generators.push_back(mob_ptr);
		}
		else {
			delete mob_ptr;
		}
	}

	for (size_t l = 0; l < mob_links_buffer.size(); ++l) {
		size_t f = mob_links_buffer[l].first;
		size_t s = mob_links_buffer[l].second;
		cur_area_data.mob_generators[f]->links.push_back(
			cur_area_data.mob_generators[s]
		);
		cur_area_data.mob_generators[f]->link_nrs.push_back(s);
	}
}
void load_sectors_by_day() {}
	