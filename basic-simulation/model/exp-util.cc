/**
 * Copyright (c) 2020 snkas
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include "exp-util.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include "cppjson2structure.hh"
/**
 * Trim from end of string.
 *
 * @param s     Original string
 *
 * @return Right-trimmed string
 */
std::string right_trim(std::string s) {
    s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
    return s;
}

/**
 * Trim from start of string.
 *
 * @param s     Original string
 *
 * @return Left-trimmed string
 */
std::string left_trim(std::string s) {
    s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
    return s;
}

/**
 * Trim from start and end of string.
 *
 * @param s     Original string
 *
 * @return Left-and-right-trimmed string
 */
std::string trim(std::string s) {
    return left_trim(right_trim(s));
}

/**
 * Check if the string ends with a suffix.
 *
 * @param str       String
 * @param prefix    Prefix
 *
 * @return True iff string ends with that suffix
 */
bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

/**
 * Check if the string starts with a prefix.
 *
 * @param str       String
 * @param prefix    Prefix
 *
 * @return True iff string starts with that prefix
 */
bool starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

/**
 * Remove the start and end double quote (") from a string if they are both present.
 *
 * @param s     String with quotes (e.g., "abc")
 *
 * @return String without quotes (e.g., abc)
 */
std::string remove_start_end_double_quote_if_present(std::string s) {
    if (s.size() >= 2 && starts_with(s, "\"") && ends_with(s, "\"")) {
        return s.substr(1, s.size() - 2);
    } else {
        return s;
    }
}

/**
 * Split a string by the delimiter(s).
 *
 * @param line          Line (e.g., "a->b->c")
 * @param delimiter     Delimiter string (e.g., "->")
 *
 * @return Split vector (e.g., [a, b, c])
 */
std::vector<std::string> split_string(const std::string line, const std::string delimiter) {
    std::vector<std::string> result;
    std::string remainder = line;
    size_t idx = remainder.find(delimiter);
    while (idx != std::string::npos) {
        result.push_back(remainder.substr(0, idx));
        remainder = remainder.substr(idx + delimiter.size(), remainder.size());
        idx = remainder.find(delimiter);
    }
    result.push_back(remainder);
    return result;
}

/**
 * Split a string by the delimiter(s) and check that the split size is of expected size.
 * If it is not of expected size, throw an exception.
 *
 * @param line          Line (e.g., "a->b->c")
 * @param delimiter     Delimiter string (e.g., "->")
 * @param expected      Expected number (e.g., 3)
 *
 * @return Split vector (e.g., [a, b, c])
 */
std::vector<std::string> split_string(const std::string line, const std::string delimiter, size_t expected) {

    std::vector<std::string> the_split = split_string(line, delimiter);

    // It must match the expected split length, else throw exception
    if (the_split.size() != expected) {
        throw std::invalid_argument(
                format_string(
                        "String %s has a %s-split of %zu != %zu",
                        line.c_str(),
                        delimiter.c_str(),
                        the_split.size(),
                        expected
                )
        );
    }

    return the_split;
}

/**
 * Parse string into an int64, or throw an exception.
 *
 * @param str  Input string
 *
 * @return Int64
 */
int64_t parse_int64(const std::string& str) {

    // Parse using stoll -- rethrow argument to be a bit easier debuggable
    int64_t val;
    size_t i;
    try {
        val = std::stoll(str, &i);
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument("Could not convert to int64: " + str);
    }

    // No remainder
    if (i != str.size()) {
        throw std::invalid_argument("Could not convert to int64: " + str);
    }

    return val;
}

/**
 * Parse string into a positive int64, or throw an exception.
 *
 * @param str  Input string
 *
 * @return Int64
 */
int64_t parse_positive_int64(const std::string& str) {
    int64_t val = parse_int64(str);
    if (val < 0) {
        throw std::invalid_argument(format_string("Negative int64 value not permitted: %" PRId64, val));
    }
    return val;
}

/**
 * Parse string into a int64 >= 1, or throw an exception.
 *
 * @param str  Input string
 *
 * @return Int64 >= 1
 */
int64_t parse_geq_one_int64(const std::string& str) {
    int64_t val = parse_positive_int64(str);
    if (val < 1) {
        throw std::invalid_argument(format_string("Value must be >= 1: %" PRId64, val));
    }
    return val;
}

/**
 * Parse string into a double, or throw an exception.
 *
 * @param str  Input string
 *
 * @return Double
 */
double parse_double(const std::string& str) {

    // Parse using stod -- rethrow argument to be a bit easier debuggable
    double val;
    size_t i;
    try {
        val = std::stod(str, &i);
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument("Could not convert to double: " + str);
    }

    // No remainder
    if (i != str.size()) {
        throw std::invalid_argument("Could not convert to double: " + str);
    }

    return val;
}

/**
 * Parse string into a positive double, or throw an exception.
 *
 * @param str  Input string
 *
 * @return Positive double
 */
double parse_positive_double(const std::string& str) {
    double val = parse_double(str);
    if (val < 0) {
        throw std::invalid_argument(format_string("Negative double value not permitted: %f", val));
    }
    return val;
}

/**
 * Parse string into a double in the range of [0.0, 1.0], or throw an exception.
 *
 * @param str  Input string
 *
 * @return Double in the range of [0.0, 1.0]
 */
double parse_double_between_zero_and_one(const std::string& str) {
    double val = parse_double(str);
    if (val < 0 || val > 1) {
        throw std::invalid_argument(format_string("Double value must be [0, 1]: %f", val));
    }
    return val;
}

/**
 * Parse string into a boolean, or throw an exception.
 *
 * @param str  Input string (e.g., "true", "false", "0", "1")
 *
 * @return Boolean
 */
bool parse_boolean(const std::string& str) {
    if (str == "true" || str == "1") {
        return true;
    } else if (str == "false" || str == "0") {
        return false;
    } else {
        throw std::invalid_argument(format_string("Value could not be converted to bool: %s", str.c_str()));
    }
}

/**
 * Parse set string (i.e., set(...)) into a real set without whitespace.
 * If it is incorrect format, throw an exception.
 *
 * @param str  Input string (e.g., "set(a,b,c)")
 *
 * @return String set (e.g., {a, b, c})
 */
std::set<std::string> parse_set_string(const std::string str) {

    // Check set(...)
    if (!starts_with(str, "set(") || !ends_with(str, ")")) {
        throw std::invalid_argument(format_string( "Set %s is not encased in set(...)", str.c_str()));
    }
    std::string only_inside = str.substr(4, str.size() - 5);
    if (trim(only_inside).empty()) {
        std::set<std::string> final;
        return final;
    }
    std::vector<std::string> final = split_string(only_inside, ",");
    std::set<std::string> final_set;
    for (std::string& s : final) {
        final_set.insert(trim(s));
    }
    if (final.size() != final_set.size()) {
        throw std::invalid_argument(
                format_string(
                        "Set %s contains duplicates",
                        str.c_str()
                )
        );
    }
    return final_set;
}

/**
 * Parse string into a set of positive int64, or throw an exception.
 *
 * @param str  Input string
 *
 * @return Set of int64s
 */
std::set<int64_t> parse_set_positive_int64(const std::string str) {
    std::set<std::string> string_set = parse_set_string(str);
    std::set<int64_t> int64_set;
    for (std::string s : string_set) {
        int64_set.insert(parse_positive_int64(s));
    }
    if (string_set.size() != int64_set.size()) {
        throw std::invalid_argument(
                format_string(
                        "Set %s contains int64 duplicates",
                        str.c_str()
                )
        );
    }
    return int64_set;
}

/**
 * Parse list string (i.e., list(...)) into a real list without whitespace.
 * If it is incorrect format, throw an exception.
 *
 * @param str  Input string (e.g., "list(a,b,c)")
 *
 * @return List of strings (e.g., [a, b, c])
 */
std::vector<std::string> parse_list_string(const std::string str) {

    // Check list(...)
    if (!starts_with(str, "list(") || !ends_with(str, ")")) {
        throw std::invalid_argument(format_string( "List %s is not encased in list(...)", str.c_str()));
    }
    std::string only_inside = str.substr(5, str.size() - 6);
    if (trim(only_inside).empty()) {
        std::vector<std::string> final;
        return final;
    }
    std::vector<std::string> prelim_list = split_string(only_inside, ",");
    std::vector<std::string> final_list;
    for (std::string& s : prelim_list) {
        final_list.push_back(trim(s));
    }
    return final_list;
}

/**
 * Parse string into a list of positive int64, or throw an exception.
 *
 * @param str  Input string
 *
 * @return List of int64s
 */
std::vector<int64_t> parse_list_positive_int64(const std::string str) {
    std::vector<std::string> string_list = parse_list_string(str);
    std::vector<int64_t> int64_list;
    for (std::string s : string_list) {
        int64_list.push_back(parse_positive_int64(s));
    }
    return int64_list;
}

/**
 * Parse map string (i.e., map(...)) into key-value pair list (which can be converted into a map later on).
 * If it is incorrect format, throw an exception.
 *
 * @param str  Input string (e.g., "map(a:b,c:d)"
 *
 * @return List of key-value pairs (e.g., [(a, b), (c, d))])
 */
std::vector<std::pair<std::string, std::string>> parse_map_string(const std::string str) {

    // Check for encasing map(...)
    if (!starts_with(str, "map(") || !ends_with(str, ")")) {
        throw std::invalid_argument(format_string( "Map %s is not encased in map(...)", str.c_str()));
    }
    std::string only_inside = str.substr(4, str.size() - 5);

    // If it is empty, just return an empty vector
    if (trim(only_inside).empty()) {
        std::vector<std::pair<std::string, std::string>> final;
        return final;
    }

    // Split by comma to find all elements
    std::vector<std::string> comma_split_list = split_string(only_inside, ",");

    // Add the elements one-by-one
    std::vector<std::pair<std::string, std::string>> result;
    std::set<std::string> key_set;
    for (std::string& s : comma_split_list) {

        // Split by colon and insert into result
        std::vector<std::string> colon_split_list = split_string(s, ":", 2);
        result.push_back(std::make_pair(trim(colon_split_list[0]), trim(colon_split_list[1])));

        // Check if it was a duplicate key
        key_set.insert(trim(colon_split_list[0]));
        if (result.size() != key_set.size()) {
            throw std::invalid_argument(format_string("Duplicate key: %s", trim(colon_split_list[0]).c_str()));
        }

    }

    return result;
}

/**
 * Throw an exception if not all items are less than a number.
 *
 * @param s         Set of int64s
 * @param number    Upper bound (non-inclusive) that all items must abide by.
 */
void all_items_are_less_than(const std::set<int64_t>& s, const int64_t number) {
    for (int64_t val : s) {
        if (val >= number) {
            throw std::invalid_argument(
                    format_string(
                            "Value %" PRIu64 " > %" PRIu64 "",
                    val,
                    number
            )
            );
        }
    }
}

/**
 * Intersection of two sets.
 *
 * @param s1    Set A
 * @param s2    Set B
 *
 * @return Intersection of set A and B
 */
std::set<int64_t> direct_set_intersection(const std::set<int64_t>& s1, const std::set<int64_t>& s2) {
    std::set<int64_t> intersect;
    set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(intersect, intersect.begin()));
    return intersect;
}

/**
 * Union of two sets.
 *
 * @param s1    Set A
 * @param s2    Set B
 *
 * @return Union of set A and B
 */
std::set<int64_t> direct_set_union(const std::set<int64_t>& s1, const std::set<int64_t>& s2) {
    std::set<int64_t> s_union;
    set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(s_union, s_union.begin()));
    return s_union;
}

/**
 * Read a config properties file into a mapping.
 *
 * @param   filename    File name of the config.ini
 *
 * @return Key-value map with the configuration
*/
void read_config(const std::string& run_dir, std::map<std::string, std::string>& config) {

	std::string filename;

	//<! basic_attribute.json
	filename = run_dir + "/basic_attribute.json";

    // Check that the file exists
    if (!file_exists(filename)) {
        throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
    }
    else{
    	ifstream jfile(filename);
    	if (jfile) {
    	    json j;
    		jfile >> j;
    		jsonns::basic_simulation_set_info vi = j.at("basic_simulation_set");
    		jsonns::basic_distributed_simulation_set_info vj = j.at("basic_distributed_simulation_set");

    		// Trim whitespace
    		std::string key, value;
    		key = trim("epoch");
    		value = remove_start_end_double_quote_if_present(trim(vi.epoch));
    		config[key] = value;
    		key = trim("simulation_end_time_ns");
    		value = remove_start_end_double_quote_if_present(trim(vi.simulation_end_time_ns));
    		config[key] = value;
    		key = trim("dynamic_state_update_interval_ns");
    		value = remove_start_end_double_quote_if_present(trim(vi.dynamic_state_update_interval_ns));
    		config[key] = value;
    		key = trim("simulation_seed");
    		value = remove_start_end_double_quote_if_present(trim(vi.simulation_seed));
    		config[key] = value;
    		key = trim("enable_sun_outage");
    		value = remove_start_end_double_quote_if_present(trim(vi.enable_sun_outage));
    		config[key] = value;

    		key = trim("enable_distributed");
    		value = remove_start_end_double_quote_if_present(trim(vj.enable_distributed));
    		config[key] = value;
    		if(j["basic_distributed_simulation_set"].count("enable_distributed_pre_process") > 0){
    			key = trim("enable_distributed_pre_process");
				value = remove_start_end_double_quote_if_present(trim(j["basic_distributed_simulation_set"]["enable_distributed_pre_process"].dump()));
				config[key] = value;
    		}
    		key = trim("distributed_simulator_implementation_type");
    		value = remove_start_end_double_quote_if_present(trim(vj.distributed_simulator_implementation_type));
    		config[key] = value;
    		key = trim("distributed_assign_algorithm");
    		value = remove_start_end_double_quote_if_present(trim(vj.distributed_assign_algorithm));
    		config[key] = value;

    //		// Check key does not exist yet
    //		if (config.find(key) != config.end()) {
    //			throw std::runtime_error(format_string("Duplicate property key: %s", key.c_str()));
    //		}
    		jfile.close();

    	}
    	else{
    		throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
    	}

    }



	//<! link_global_attribute.json
    filename = run_dir + "/config_protocol/link_global_attribute.json";

	// Check that the file exists
	if (!file_exists(filename)) {
		throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
	}
	else{
		ifstream jfile(filename);
		if (jfile) {
			json j;
			jfile >> j;
			jsonns::feeder_link_info vi = j.at("feeder_link");
			jsonns::intersatellite_link_info vj = j.at("intersatellite_link");

			// Trim whitespace
			std::string key, value;
			key = trim("enable_gsl_data_rate_fixed");
			value = remove_start_end_double_quote_if_present(trim(vi.enable_gsl_data_rate_fixed));
			config[key] = value;
			key = trim("gsl_data_rate_megabit_per_s");
			value = remove_start_end_double_quote_if_present(trim(vi.gsl_data_rate_megabit_per_s));
			config[key] = value;
			key = trim("gsl_max_queue_size_pkts");
			value = remove_start_end_double_quote_if_present(trim(vi.gsl_max_queue_size_pkts));
            config[key] = value;
            key = trim("gsl_error_rate_per_pkt");
            value = remove_start_end_double_quote_if_present(trim(vi.gsl_error_rate_per_pkt));
			config[key] = value;
			key = trim("enable_csma");
			value = remove_start_end_double_quote_if_present(trim(vi.csma.csma_enabled));
			config[key] = value;
			key = trim("enable_aloha");
			value = remove_start_end_double_quote_if_present(trim(vi.aloha.aloha_enabled));
			config[key] = value;
			key = trim("enable_fdma");
			value = remove_start_end_double_quote_if_present(trim(vi.fdma.fdma_enabled));
			config[key] = value;
			key = trim("maximum_feeder_link_number");
			value = remove_start_end_double_quote_if_present(trim(vi.maximum_feeder_link_number));
			config[key] = value;
			key = trim("enable_distance_nearest_first");
			value = remove_start_end_double_quote_if_present(trim(vi.enable_distance_nearest_first));
			config[key] = value;


			key = trim("enable_isl_data_rate_fixed");
			value = remove_start_end_double_quote_if_present(trim(vj.enable_isl_data_rate_fixed));
			config[key] = value;
			key = trim("isl_data_rate_megabit_per_s");
			value = remove_start_end_double_quote_if_present(trim(vj.isl_data_rate_megabit_per_s));
			config[key] = value;
			key = trim("isl_max_queue_size_pkts");
			value = remove_start_end_double_quote_if_present(trim(vj.isl_max_queue_size_pkts));
			config[key] = value;
            key = trim("isl_error_rate_per_pkt");
            value = remove_start_end_double_quote_if_present(trim(vj.isl_error_rate_per_pkt));
            config[key] = value;
			key = trim("enable_p2p_protocol");
			value = remove_start_end_double_quote_if_present(trim(vj.ppp.ppp_enabled));
			config[key] = value;
			key = trim("enable_hdlc_protocol");
			value = remove_start_end_double_quote_if_present(trim(vj.hdlc.hdlc_enabled));
			config[key] = value;
			key = trim("enable_grid_type_isl_establish");
			value = remove_start_end_double_quote_if_present(trim(vj.enable_grid_type_isl_establish));
			config[key] = value;

	//		// Check key does not exist yet
	//		if (config.find(key) != config.end()) {
	//			throw std::runtime_error(format_string("Duplicate property key: %s", key.c_str()));
	//		}
			jfile.close();

		}
		else{
			throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
		}

	}



	//<! ip_global_attribute.json
	filename = run_dir + "/config_protocol/ip_global_attribute.json";

	// Check that the file exists
	if (!file_exists(filename)) {
		throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
	}
	else{
		ifstream jfile(filename);
		if (jfile) {
			json j;
			jfile >> j;
			jsonns::ip_addressing vi = j.at("addressing");
			jsonns::ip_routing vj = j.at("routing");

			// Trim whitespace
			std::string key, value;
			key = trim("enable_ipv4_addressing_protocol");
			value = remove_start_end_double_quote_if_present(trim(vi.enable_ipv4_addressing_protocol));
			config[key] = value;
			key = trim("network_addressing_method");
			value = remove_start_end_double_quote_if_present(trim(vi.network_addressing_method));
			config[key] = value;
			key = trim("network_part");
			value = remove_start_end_double_quote_if_present(trim(vi.network_part));
			config[key] = value;
			key = trim("address_mask");
			value = remove_start_end_double_quote_if_present(trim(vi.address_mask));
			config[key] = value;
			key = trim("host_part_to_start_from");
			value = remove_start_end_double_quote_if_present(trim(vi.host_part_to_start_from));
			config[key] = value;


			key = trim("ip_export_routing_tables");
			value = remove_start_end_double_quote_if_present(trim(vj.export_routing_tables.enable_export_routing_tables));
			config[key] = value;
			key = trim("enable_open_shortest_path_first");
			value = remove_start_end_double_quote_if_present(trim(vj.ospf.ospf_enabled));
			config[key] = value;
			key = trim("enable_ad_hoc_on_demand_distance_vector");
			value = remove_start_end_double_quote_if_present(trim(vj.aodv.aodv_enabled));
			config[key] = value;
			key = trim("enable_minimum_hop_count_routing");
			value = remove_start_end_double_quote_if_present(trim(vj.minhop.minhop_enabled));
			config[key] = value;
			key = trim("enable_border_gateway_protocol");
			value = remove_start_end_double_quote_if_present(trim(vj.bgp.bgp_enabled));
			config[key] = value;
			key = trim("enable_satellite_to_ground_routing");
			value = remove_start_end_double_quote_if_present(trim(vj.satellite_to_ground_routing.satellite_to_ground_routing_enabled));
			config[key] = value;

		//		// Check key does not exist yet
		//		if (config.find(key) != config.end()) {
		//			throw std::runtime_error(format_string("Duplicate property key: %s", key.c_str()));
		//		}
			jfile.close();

		}
		else{
			throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
		}

	}


//<! physical_global_attribute.json
	filename = run_dir + "/config_protocol/physical_global_attribute.json";

	// Check that the file exists
	if (!file_exists(filename)) {
		throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
	}
	else{
		ifstream jfile(filename);
		if (jfile) {
			json j;
			jfile >> j;
			jsonns::antenna vi = j.at("antenna");

			// Trim whitespace
			std::string key, value;
			key = trim("minimum_elevation_angle_deg");
			value = to_string(vi.minimum_elevation_angle_deg);
			config[key] = value;


		//		// Check key does not exist yet
		//		if (config.find(key) != config.end()) {
		//			throw std::runtime_error(format_string("Duplicate property key: %s", key.c_str()));
		//		}
			jfile.close();

		}
		else{
			throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
		}

	}

////<! application_global_attribute.json
//	filename = run_dir + "/config_protocol/application_global_attribute.json";
//
//	// Check that the file exists
//	if (!file_exists(filename)) {
//		throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
//	}
//	else{
//		ifstream jfile(filename);
//		if (jfile) {
//			json j;
//			jfile >> j;
//			jsonns::application vi = j;
//
//			// Trim whitespace
//			std::string key, value;
//			key = trim("enable_sag_application_scheduler_udp");
//			value = remove_start_end_double_quote_if_present(trim(vi.enable_application_scheduler_udp));
//			config[key] = value;
//			key = trim("enable_sag_application_scheduler_tcp");
//			value = remove_start_end_double_quote_if_present(trim(vi.enable_application_scheduler_tcp));
//			config[key] = value;
//			key = trim("enable_sag_application_scheduler_rtp");
//			value = remove_start_end_double_quote_if_present(trim(vi.enable_application_scheduler_rtp));
//			config[key] = value;
//			key = trim("enable_sag_application_scheduler_3gpp_http");
//			value = remove_start_end_double_quote_if_present(trim(vi.enable_application_scheduler_3gpp_http));
//			config[key] = value;
//			key = trim("enable_sag_application_scheduler_ftp");
//			value = remove_start_end_double_quote_if_present(trim(vi.enable_application_scheduler_ftp));
//			config[key] = value;
//
//
//		//		// Check key does not exist yet
//		//		if (config.find(key) != config.end()) {
//		//			throw std::runtime_error(format_string("Duplicate property key: %s", key.c_str()));
//		//		}
//			jfile.close();
//
//		}
//		else{
//			throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
//		}
//
//	}



	//<! tracing.json
	filename = run_dir + "/config_analysis/tracing.json";

	// Check that the file exists
	if (!file_exists(filename)) {
		throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
	}
	else{
		ifstream jfile(filename);
		if (jfile) {
			json j;
			jfile >> j;
			jsonns::utilization vi = j.at("utilization");
			jsonns::wireshark vj = j.at("wireshark");

			// Trim whitespace
			std::string key, value;
			key = trim("enable_link_utilization_tracing");
			value = remove_start_end_double_quote_if_present(trim(vi.link_utilization_tracing_enabled));
			config[key] = value;
			key = trim("target_device_all");
			value = remove_start_end_double_quote_if_present(trim(vi.target_device_all));
			config[key] = value;
			key = trim("link_utilization_tracking_interval_ns");
			value = remove_start_end_double_quote_if_present(trim(vi.link_utilization_tracing_interval_ns));
			config[key] = value;
			key = trim("enable_trajectory_tracing");
			value = remove_start_end_double_quote_if_present(trim(vi.enable_trajectory_tracing));
			config[key] = value;

			key = trim("enable_pcap_tracing");
			value = remove_start_end_double_quote_if_present(trim(vj.wireshark_enabled));
			config[key] = value;
			key = trim("wireshark_target_device_all");
			value = remove_start_end_double_quote_if_present(trim(vj.wireshark_target_device_all));
			config[key] = value;


		//		// Check key does not exist yet
		//		if (config.find(key) != config.end()) {
		//			throw std::runtime_error(format_string("Duplicate property key: %s", key.c_str()));
		//		}
			jfile.close();

		}
		else{
			throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
		}

	}




}

/**
 * Get the parameter value or fail.
 *
 * @param param_key                 Parameter key
 * @param config                    Config parameters map
 *
 * @return Parameter value (if not present, an invalid_argument exception is thrown)
 */
std::string get_param_or_fail(const std::string& param_key, std::map<std::string, std::string>& config) {

    if (config.find(param_key) != config.end()) {
        return config[param_key];
    } else {
        throw std::invalid_argument(format_string("Necessary parameter '%s' is not set.", param_key.c_str()));
    }

}

/**
 * Get the parameter value or if not found, return default value.
 *
 * @param param_key                 Parameter key
 * @param default_value             Default value
 * @param config                    Config parameters map
 *
 * @return Parameter value (if not present, the default value is returned)
 */
std::string get_param_or_default(const std::string& param_key, std::string default_value, std::map<std::string, std::string>& config) {

    if (config.find(param_key) != config.end()) {
        return config[param_key];
    } else {
        return default_value;
    }

}

/**
 * Convert byte to megabit (Mbit).
 *
 * @param num_bytes     Number of bytes
 *
 * @return Number of megabit
 */
double byte_to_megabit(int64_t num_bytes) {
    return num_bytes * 8.0 / 1000.0 / 1000.0;
}

/**
 * Convert nanoseconds to seconds.
 *
 * @param num_nanosec   Number of nanoseconds
 *
 * @return Number of nanoseconds
 */
double nanosec_to_sec(int64_t num_nanosec) {
    return num_nanosec / 1e9;
}

/**
 * Convert nanoseconds to milliseconds.
 *
 * @param num_nanosec   Number of nanoseconds
 *
 * @return Number of milliseconds
 */
double nanosec_to_millisec(int64_t num_nanosec) {
    return num_nanosec / 1e6;
}

/**
 * Convert number of nanoseconds to microseconds.
 *
 * @param num_nanosec   Number of nanoseconds
 *
 * @return Number of nanoseconds
 */
double nanosec_to_microsec(int64_t num_nanosec) {
    return num_nanosec / 1e3;
}

/**
 * Check if a file exists.
 *
 * @param filename  Filename
 *
 * @return True iff the file exists
 */
bool file_exists(std::string filename) {
    struct stat st = {0};
    if (stat(filename.c_str(), &st) == 0) {
        return S_ISREG(st.st_mode);
    } else {
        return false;
    }
}

/**
 * Remove a file if it exists. If it exists but could not be removed, it throws an exception.
 *
 * @param filename  Filename to remove
 */
void remove_file_if_exists(std::string filename) {
    if (file_exists(filename)) {
        if (unlink(filename.c_str()) == -1) {
            throw std::runtime_error(format_string("File %s could not be removed.\n", filename.c_str()));
        }
    }
}

/**
 * Check if a directory exists.
 *
 * @param dirname  Directory name
 *
 * @return True iff the directory exists
 */
bool dir_exists(std::string dirname) {
    struct stat st = {0};
    if (stat(dirname.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    } else {
        return false;
    }
}

/**
 * Remove a directory if it exists.
 *
 * @param dirname   Directory name
 */
void remove_dir_if_exists(std::string dirname) {
    if (dir_exists(dirname)) {
        if (rmdir(dirname.c_str()) == -1) {
            // 或者通过检查 errno 的值执行特定的错误处理
            printf("errno=%u\n",errno);
            if (errno == ENOENT) {
                printf("Directory does not exist.\n");
            } else if (errno == EACCES) {
                printf("Permission denied.\n");
            } else if (errno == ENOTEMPTY) {
                printf("Directory is not empty.\n");
            } else {
                printf("Unknown error.\n");
            }   

            throw std::runtime_error(format_string("Directory %s could not be removed.\n", dirname.c_str()));
        }
    }
}

int recursiveDelete(const char *path) {
    DIR *dir;
    struct dirent *entry;

    // 打开目录
    dir = opendir(path);
    if (!dir) {
        perror("Error opening directory");
        return 1; // 返回错误码
    }

    // 逐个处理目录中的文件和子目录
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            // 忽略当前目录和上一级目录
            continue;
        }

        // 构建子文件或子目录的完整路径
        char entryPath[PATH_MAX];
        snprintf(entryPath, sizeof(entryPath), "%s/%s", path, entry->d_name);

        // 如果是子目录，递归删除
        if (entry->d_type == DT_DIR) {
            if (recursiveDelete(entryPath) != 0) {
                closedir(dir);
                return 2; // 返回错误码
            }
        } else {
            // 如果是文件，直接删除
            if (remove(entryPath) != 0) {
                perror("Error deleting file");
                closedir(dir);
                return 3; // 返回错误码
            }
        }
    }

    // 关闭目录流
    closedir(dir);

    // 删除空目录
    if (rmdir(path) != 0) {
        perror("Error deleting directory");
        return 4; // 返回错误码
    }

    return 0; // 返回成功
}

/**
 * Remove a directory and its subfiles if it exists.
 *
 * @param dirname   Directory name
 */
void remove_dir_and_subfile_if_exists(std::string dirname) {
    if (dir_exists(dirname)) {
        if (recursiveDelete(dirname.c_str()) != 0) {
            throw std::runtime_error(format_string("Directory %s could not be removed.\n", dirname.c_str()));
        }
    }
}

/**
 * Creates a directory if it does not exist.
 *
 * @param dirname  Directory name
 *
 * @return True iff the directory was created if it did not exist
 */
void mkdir_if_not_exists(std::string dirname) {
    if (!dir_exists(dirname)) {
        if (mkdir(dirname.c_str(), 0777) == -1) {
            // 或者通过检查 errno 的值执行特定的错误处理
            printf("errno=%u\n",errno);
            if (errno == ENOENT) {
                printf("Directory does not exist.\n");
            } else if (errno == EACCES) {
                printf("Permission denied.\n");
            } else if (errno == ENOTEMPTY) {
                printf("Directory is not empty.\n");
            } else {
                printf("Unknown error.\n");
            }   
            throw std::runtime_error(format_string("Directory \"%s\" could not be made.", dirname.c_str()));
        }
    }
}

int createDirectory(const std::string& path) {
    // 尝试创建目录
    if (mkdir(path.c_str(), 0777) == 0) {
        return 0; // 创建成功
    } else {
        if (errno == ENOENT) {
            // 如果目录不存在，递归创建上级目录
            size_t found = path.find_last_of('/');
            if (found != std::string::npos) {
                std::string parentPath = path.substr(0, found);
                if (createDirectory(parentPath)!=0) {
                    return 1; // 递归创建上级目录失败
                }
                // 再次尝试创建目标目录
                if(mkdir(path.c_str(), 0777) == 0){
                    return 0;
                }
            }
        }
        return 2; // 其他原因导致创建失败
    }
}

/**
 * Creates a directory and its parent directory if they do not exist.
 *
 * @param dirname  Directory name
 *
 * @return True iff the directory was created if it did not exist
 */
void mkdir_force_if_not_exists(std::string dirname) {
    if (!dir_exists(dirname)) {
        if (createDirectory(dirname) != 0) {
            throw std::runtime_error(format_string("Directory \"%s\" could not be made.", dirname.c_str()));
        }
    }
}

/**
 * Read the content of a file directly line-by-line into a vector of trimmed lines.
 *
 * @param   filename    File name
 *
 * @return Vector of trimmed lines
 */
std::vector<std::string> read_file_direct(const std::string& filename) {

    // Check that the file exists
    if (!file_exists(filename)) {
        throw std::runtime_error(format_string("File %s does not exist.", filename.c_str()));
    }

    // Storage
    std::vector<std::string> lines;

    // Open file
    std::string line;
    std::ifstream input_file(filename);
    if (input_file) {
        while (getline(input_file, line)) {
            lines.push_back(trim(line));
        }
        input_file.close();
    } else {
        throw std::runtime_error(format_string("File %s could not be read.", filename.c_str()));
    }

    return lines;

}
