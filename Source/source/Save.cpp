#include <algorithm>
#include "Save.h"
#include "utils/string_utils.h"
#include "functions.h"
#include "load.h"
#include "vars.h"


void savemobs() {

	int lost_to_sunset = 0;
	data_node master("masternode", "");
	data_node* mobs_node = new data_node("mobs", "");
	data_node* onions_node = new data_node("onions", "");
	int sorcerer = 0;
	float warlock = 0;
	int id = 0;
	int lig = 0;
	vector<pair<int, int>> dill;
	for (size_t m = 0; m < mobs.size(); ++m) {
		mob* m_ptr = mobs[m];
		m_ptr->id = m;
	}
	for (size_t m = 0; m < mobs.size(); ++m) {
		mob* m_ptr = mobs[m];
		for (size_t l = 0; l < m_ptr->links.size(); ++l) {
			dill.push_back(make_pair(m, m_ptr->links[l]->id));

		}
	}
	for (size_t p = 0; p < pikmin_list.size(); ++p)
	{
		pikmin* p_ptr = pikmin_list[p];
		if (p_ptr->is_safe == false) {
			delete_mob((mob*)p_ptr);
			delete p_ptr;
			lost_to_sunset += 1;
		}
	}		for (size_t m = 0; m < mobs.size(); ++m) {
			mob* m_ptr = mobs[m];

			data_node* mob_node =
				new data_node(m_ptr->type->category->name, "");
			mobs_node->add(mob_node);
			if (m_ptr->type->category->name == "Ship") {
				ship* s_ptr = (ship*)m_ptr;
				warlock += s_ptr->pokos;
			}
			if (m_ptr->type->category->name == "Onion") {
				mob_node->add(new data_node(m_ptr->type->name, ""));
				onion* o = (onion*)m_ptr;
				data_node* o_node = new data_node(o->type->name, "");
				onions_node->add(o_node);
				sorcerer += + o->pikmin_inside[0] + o->pikmin_inside[1] + o->pikmin_inside[2];
				o_node->add(new data_node("Leaf_Pikmin_Inside",
					i2s(o->pikmin_inside[0])));
				o_node->add(new data_node("Bud_Pikmin_Inside",
					i2s(o->pikmin_inside[1])));
				o_node->add(new data_node("Flower_Pikmin_Inside",
					i2s(o->pikmin_inside[2])));
				o_node->add(new data_node("Fourth_Maturity",
					i2s(o->pikmin_inside[3])));
			}
			else if (m_ptr->type) {
				mob_node->add(
					new data_node("type", m_ptr->type->name)
				);
			}
			int ib = m;
			mob_node->add(new data_node("id", i2s(ib)));
			mob_node->add(
				new data_node(
					"p",
					f2s(m_ptr->pos.x) + " " + f2s(m_ptr->pos.y)
				)
			);
			if (m_ptr->angle != 0) {
				mob_node->add(
					new data_node("angle", f2s(m_ptr->angle))
				);
			}
			if (!m_ptr->vars.empty()) {
				string sing = "vars=";
				vector<string> names = m_ptr->varnames;
				for (size_t v = 0; v < m_ptr->varnames.size(); ++v) {
					string bing = names[v];
					string wing = m_ptr->vars.at(bing);
					sing += bing + "=" + wing;
					sing += " ";
				}
				mob_node->add(new data_node(sing, ""));
				string links = "";
				for (size_t l = 0; l < dill.size(); ++l) {
					if (dill[l].first == m) {
						links += i2s(dill[l].second) + " ";
					}
				}
				if(!m_ptr->links.empty())
				mob_node->add(new data_node("links", links));
			}

		}

		sorcerer += mobs_node->get_nr_of_children_by_name("Pikmin");
		scorer = "Pikmin With us:" + i2s(sorcerer) + "Pokos:" + f2s(warlock) + "Lost to sunset:" + i2s(lost_to_sunset);

		master.add(mobs_node);
		master.add(onions_node);

		string filessavename = AREAS_FOLDER_PATH + "/" + cur_area_data.name +
			"/Mobs_on_Day" + i2s(day) + ".txt";
		bool mob_save_ok = master.save_file(filessavename);
}
void loadmobs() {

	string saves = AREAS_FOLDER_PATH + "/" + cur_area_data.name +
		"/Mobs_on_Day" + i2s(day - 1) + ".txt";
	data_node save = load_data_file(saves);
	size_t n_mobs = save.get_child_by_name("mobs")->get_nr_of_children();
	data_node*	mobs_node = save.get_child_by_name("mobs");	
	vector<mob*> mobs_per_gen;
	vector<mob_gen*> mobs_in_gen;
	vector<pair<size_t, size_t> > mob_links_buffer;
	for (size_t n = 0; n < n_mobs; ++n) {
		bool link_signal = false;
		mob_gen* m_ptr = new mob_gen();
		data_node* mobe = mobs_node->get_child(n);
		if (mobe->get_nr_of_children_by_name("links") > 0) { link_signal = true; }

		if (mobe->name == "Pikmin"){
			m_ptr->category = mob_categories.get_from_name(mobe->name);
			string typame = mobe->get_child_by_name("type")->value;
			for (size_t o = 0; o < onions.size(); ++o) {
				if (typame == onions[o]->oni_type->pik_type->name) {
					m_ptr->pos = onions[o]->pos;
				}
			}
			m_ptr->type = m_ptr->category->get_type(typame);
			pikmin_type* type = (pikmin_type*)m_ptr->type;
			if (type->has_onion == false)m_ptr->pos = ships[0]->beam_final_pos;
			m_ptr->vars = mobe->get_child_by_name("vars")->value;
			m_ptr->angle = 0;
			mobs_per_gen.push_back(create_mob(m_ptr->category, m_ptr->pos, m_ptr->type, 0, m_ptr->vars));
			mobs_in_gen.push_back(m_ptr);
			vector<string> link_strs =
				split(mobe->get_child_by_name("links")->value);
			for (size_t l = 0; l < link_strs.size(); ++l) {
				mob_links_buffer.push_back(make_pair(n, s2i(link_strs[l])));
			}
		} else if (mobe->name == "Enemy") {
			m_ptr->category = mob_categories.get_from_name(mobe->name);
			string typame = mobe->get_child_by_name("type")->value;
			m_ptr->type = m_ptr->category->get_type(typame);
			m_ptr->pos = s2p(mobe->get_child_by_name("p")->value);
			m_ptr->vars = mobe->get_child_by_name("vars")->value;
			m_ptr->angle =
				s2f(
					mobe->get_child_by_name("angle")->get_value_or_default("0")
				);
			mobs_per_gen.push_back(create_mob(m_ptr->category, m_ptr->pos, m_ptr->type, m_ptr->angle, m_ptr->vars));
			mobs_in_gen.push_back(m_ptr);
			vector<string> link_strs =
				split(mobe->get_child_by_name("links")->value);
			for (size_t l = 0; l < link_strs.size(); ++l) {
				mob_links_buffer.push_back(make_pair(n, s2i(link_strs[l])));
			}
		} else if (mobe->name == "Leader") {
			m_ptr->category = mob_categories.get_from_name(mobe->name);
			string typame = mobe->get_child_by_name("type")->value;
			m_ptr->type = m_ptr->category->get_type(typame);
			m_ptr->pos = ships[0]->beam_final_pos;
			m_ptr->vars = mobe->get_child_by_name("vars")->value;
			m_ptr->angle =
				s2f(
					mobe->get_child_by_name("angle")->get_value_or_default("0")
				);
			mobs_per_gen.push_back(create_mob(m_ptr->category, m_ptr->pos, m_ptr->type, m_ptr->angle, m_ptr->vars));
			mobs_in_gen.push_back(m_ptr);
			vector<string> link_strs =
				split(mobe->get_child_by_name("links")->value);
			for (size_t l = 0; l < link_strs.size(); ++l) {
				mob_links_buffer.push_back(make_pair(n, s2i(link_strs[l])));
			}
		}
		else if (mobe->name == "Onion") {
			m_ptr->category = mob_categories.get_from_name("None");
			m_ptr->category->create_mob(point(0,0),m_ptr->category->get_type(""),0 );
			mobs_in_gen.push_back(m_ptr);
		}
		else if (mobe->name == "Ship") {
			m_ptr->category = mob_categories.get_from_name("None");
			m_ptr->category->create_mob(point(0, 0), m_ptr->category->get_type(""), 0);
			mobs_in_gen.push_back(m_ptr);
		}
		else if(mobe->name != "Pikmin" && mobe->name != "Enemy"){
			m_ptr->pos = s2p(mobe->get_child_by_name("p")->value);
			m_ptr->angle =
				s2f(
					mobe->get_child_by_name("angle")->get_value_or_default("0")
				);
			m_ptr->vars = mobe->get_child_by_name("vars")->value;

			m_ptr->category = mob_categories.get_from_name(mobe->name);
			string mt = mobe->get_child_by_name("type")->value;
			m_ptr->type = m_ptr->category->get_type(mt);

			vector<string> link_strs =
				split(mobe->get_child_by_name("links")->value);
			for (size_t l = 0; l < link_strs.size(); ++l) {
				mob_links_buffer.push_back(make_pair(n, s2i(link_strs[l])));
			}
			mobs_per_gen.push_back(create_mob(m_ptr->category, m_ptr->pos, m_ptr->type, m_ptr->angle, m_ptr->vars));
			mobs_in_gen.push_back(m_ptr);
		}
	}
	for (size_t m = 0; m < n_mobs; ++m) {
		mob_gen* m_ptr = mobs_in_gen[m];

		for (size_t l = 0; l < m_ptr->link_nrs.size(); ++l) {
			mobs_per_gen[m]->links.push_back(mobs_per_gen[m_ptr->link_nrs[l]]);
		}
	}
	mobs_per_gen.clear();
	size_t onion_n = save.get_child_by_name("onions")->get_nr_of_children();
	data_node* obion = save.get_child_by_name("onions");
	for (size_t o = 0; o < onions.size(); ++o) {
		onion* o_ptr = onions[o];
		string onion_name = obion->get_child_by_name(o_ptr->type->name)->name;
		if (onion_name != o_ptr->type->name) continue;
		o_ptr->pikmin_inside[1] = s2i(obion->get_child_by_name(onion_name)->get_child_by_name("Leaf_Pikmin_Inside")->value);
		o_ptr->pikmin_inside[2] = s2i(obion->get_child_by_name(onion_name)->get_child_by_name("Bud_Pikmin_Inside")->value);
		o_ptr->pikmin_inside[3] = s2i(obion->get_child_by_name(onion_name)->get_child_by_name("Flower_Pikmin_Inside")->value);

	}
}
void save_sectors() {
	data_node master("masternode", "");

	data_node* sectors_node = new data_node("sectors", "");

	master.add(sectors_node);

	for (size_t s = 0; s < cur_area_data.sectors.size(); ++s) {
		sector* s_ptr = cur_area_data.sectors[s];
		data_node* sector_node = new data_node("s", "");
		sectors_node->add(sector_node);

		if (s_ptr->type != SECTOR_TYPE_NORMAL) {
			sector_node->add(
				new data_node("type", sector_types.get_name(s_ptr->type))
			);
		}
		if (s_ptr->is_bottomless_pit) {
			sector_node->add(
				new data_node("is_bottomless_pit", "true")
			);
		}
		sector_node->add(new data_node("z", f2s(s_ptr->z)));
		if (s_ptr->brightness != DEF_SECTOR_BRIGHTNESS) {
			sector_node->add(
				new data_node("brightness", i2s(s_ptr->brightness))
			);
		}
		if (!s_ptr->tag.empty()) {
			sector_node->add(new data_node("tag", s_ptr->tag));
		}
		if (s_ptr->fade) {
			sector_node->add(new data_node("fade", b2s(s_ptr->fade)));
		}
		if (s_ptr->always_cast_shadow) {
			sector_node->add(
				new data_node(
					"always_cast_shadow",
					b2s(s_ptr->always_cast_shadow)
				)
			);
		}
		if (!s_ptr->hazards_str.empty()) {
			sector_node->add(new data_node("hazards", s_ptr->hazards_str));
			sector_node->add(
				new data_node(
					"hazards_floor",
					b2s(s_ptr->hazard_floor)
				)
			);
		}

		if (!s_ptr->texture_info.file_name.empty()) {
			sector_node->add(
				new data_node(
					"texture",
					s_ptr->texture_info.file_name
				)
			);
		}

		if (s_ptr->texture_info.rot != 0) {
			sector_node->add(
				new data_node(
					"texture_rotate",
					f2s(s_ptr->texture_info.rot)
				)
			);
		}
		if (
			s_ptr->texture_info.scale.x != 1 ||
			s_ptr->texture_info.scale.y != 1
			) {
			sector_node->add(
				new data_node(
					"texture_scale",
					f2s(s_ptr->texture_info.scale.x) + " " +
					f2s(s_ptr->texture_info.scale.y)
				)
			);
		}
		if (
			s_ptr->texture_info.translation.x != 0 ||
			s_ptr->texture_info.translation.y != 0
			) {
			sector_node->add(
				new data_node(
					"texture_trans",
					f2s(s_ptr->texture_info.translation.x) + " " +
					f2s(s_ptr->texture_info.translation.y)
				)
			);
		}
		if (
			s_ptr->texture_info.tint.r != 1.0 ||
			s_ptr->texture_info.tint.g != 1.0 ||
			s_ptr->texture_info.tint.b != 1.0 ||
			s_ptr->texture_info.tint.a != 1.0
			) {
			sector_node->add(
				new data_node("texture_tint", c2s(s_ptr->texture_info.tint))
			);
		}

	}
	bool mastersaveok = master.save_file(AREAS_FOLDER_PATH + "/" + cur_area_data.name +
		"/Sectors_on_day" + i2s(day) + ".txt");
}
void save_flags() {
	data_node master("masternode", "");
	data_node* meh = new data_node("Flags", "");
	master.add(meh);
	for (size_t f = 0; f < flagnames.size(); ++f) {
		meh->add( new data_node(flagnames[f], b2s(progressflags[flagnames[f]])));
	}
	bool master_save_ok = master.save_file(AREAS_FOLDER_PATH +"/"+ "Save" + i2s(day) + ".txt");
}