
// #ifndef HUE_LIGHT_SUMULATOR_H
// #define HUE_LIGHT_SUMULATOR_H
#include <string>
#include <map>
#include "nlohmann/json.hpp"

// Describes a HUE light
struct HueLight {
	std::string name;
	int id;
	bool on;		// Power state boolean
	int bri; 		// This is actual value retrieved from the API
	int brightness; // This is the % displayed to the user
	bool isValid;	// To check if light is still being heard from (alive) 
};

/**
 *
 * Function converts from a single HueLight object to a json
 *
 * @param HueLight 			HueLight object to convert
 * @return ordered_json 	Ordered Json converted from the HueLight
*/
nlohmann::ordered_json to_json(HueLight l) {
    return nlohmann::ordered_json{ {"name", l.name}, {"id", l.id}, {"on", l.on}, {"brightness", l.brightness}};
}

/**
 *
 * Function converts from a single json to a HueLight object
 *
 * @param j 			Json object to convert
 * @return HueLight 	HueLight Object converted from the JSON
*/
HueLight from_json(nlohmann::json j) {
	HueLight l;
    l.name = j.at("name");
    l.id = j.at("id");
    l.on = j.at("on");
    l.brightness = j.at("brightness");
    return l;
}

/**
 *
 * Function converts from a vector of HueLight objects to a single json (for printing)
 *
 * @param lights 		Vector of HueLight objects to convert
 * @return ordered_json JSON object created from a vector of lights
*/
nlohmann::ordered_json to_json_vector(std::vector<HueLight> lights) {
	nlohmann::ordered_json j = {};
	for (HueLight it : lights) { 
        j.push_back(to_json(it));
    } 
    return j;
}

/**
 *
 * Convert from a json of HueLight jsons to a vector of HueLight objects 
 *
 * @param j 				JSON to be converted (a light of json objects)
 * @return vector<HueLight> Vector of converted HueLight objects
*/
std::vector<HueLight> to_HueLight_vector(nlohmann::json j) {
	std::vector<HueLight> lights;

	for (auto it : j)
	{
		lights.push_back(from_json(it));
	}

    return lights;
}

/**
 *
 * This helper function sets all of the "valid" properties on a vector of HueLight objects to the boolean provided.
 *	It provides an easy way to change all of the lights at once.
 *
 * @param currentLightsState 	Vector of HueLight objects that were found on the server last request
 * @param newLights 			Vector of HueLight objects that were found on the server in the most recent request
 * @return Bool 	Success or failure of connecting to the server after retrying
*/
void setIsValid(std::vector<HueLight> &lights, bool b) {
	for (int i = 0; i < lights.size(); i++) {
		lights.at(i).isValid = b;
	}
}

/**
 *
 * This was taken from the curl API. It is how you read data in from a request (prototype).
*/
size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}



// #endif