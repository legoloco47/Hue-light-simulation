/* 
 * HUE light simulator
 *
 * Last updated: 2/2/21
 * Author: Helen Edelson
 * Purpose: This is a console application to interface with the HUE-simulator for Josh.ai coding challenge
 */

#include <stdio.h>
#include <iostream>
#include <curl/curl.h>
#include <unistd.h>
#include "./inc/cmdparser.hpp"
#include "./inc/HUELightSimulator.h"

using namespace std;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

/**
 *	This function attempts a provided amount of connections to the server. If it fails after the nth time,
 *	  the server is assumed to be off and the program ends.  *
 *
 *
 * @param retryAttempts Number of times to retry the request
 * @param sleep   	Time in microseconds bewteen each GET request.
 * @param curl 		Handle to the easy Curl object we set up previously (trying to reconnect with)
 * @return Bool 	Success or failure of connecting to the server after retrying
 */
bool AttemptHTTPRequestRetry(int retryAttempts, int sleepTime, CURL *curl) {
  	CURLcode res;
	int attempt = 1;

  	// Try retryAttempts to reach the server
	while (attempt < retryAttempts) {

	    res = curl_easy_perform(curl);

	        if(res != CURLE_OK) {
	      		fprintf(stderr, "curl_easy_perform() failed attempt %d: %s\n", attempt, curl_easy_strerror(res));

	      		attempt++;
	        } else {
	        	// Revieved OK from server, continue processing requests
	        	return true;
	        }

		usleep(sleepTime);
	}

	return false;
}


/**
 * Creates the CURL handle with the setup parameters
 *
 *
 * @param urlString 	String to use for URL connection.
 * @param timeout 		Time in seconds before a timeout on the GET request.
 * @return CURL* 		Pointer to CURL handle to be used in future HTTP requests.
 */
CURL* CreateHTTPCurlHandle(char* url, int timeout, string* responseString) {
	CURL *curl;

    curl = curl_easy_init();

	// Provide the url to use in the request
	curl_easy_setopt(curl, CURLOPT_URL, url);

	// Specify the HTTP protocl version. The server runs on 1.1
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_1_1);

	// Set timeout field (seconds)
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);

	// Save the value returned into a json object
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);

    // The string to collect the response in
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, responseString);

    return curl;
}

/**
 *
 * This function compares and updates the new light situation to the light state in memory. We need a way to compare
 * the lights found in the most recent request to the ones we have saved from the last request. In this function we compare
 * the two vectors and make sure 
 *
 *
 * @param currentLightsState 	Vector of HueLight objects that were found on the server last request
 * @param newLights 			Vector of HueLight objects that were found on the server in the most recent request
 */
void CompareAndUpdateLightStates(vector<HueLight> &currentLightsState, vector<HueLight> newLights) {
	// First set the isValid on all of the currentLights to false. Then we will iterate over and mark each one
	//	as valid. This will show if any lights have gone offline since the last request.
	setIsValid(currentLightsState, false);

	for (auto light : newLights) {
		// Find the light in currentLightsState
		int lightId = light.id;
		int foundIt = false;
		int index = 0;

		// Check that all current lights are still valid, and if they are, check for changes
		for (auto existinglight : currentLightsState) {
			if (existinglight.id == light.id) {
				foundIt = true;
				currentLightsState.at(index).isValid = true;
				//For debugging: cout<<"For debugging: Found valid light "<< currentLightsState.at(index).id<< " updated isValid = "<<currentLightsState.at(index).isValid<<endl;

				// "brightness", "on", and "name" can change
				// Can two things change at once? yes --> do power then brightness
				if (light.on != existinglight.on) {
					json j = ordered_json{ {"id", light.id}, {"on", light.on}};

					cout<<j.dump(4)<<endl;

					// Update the curentLightState
					currentLightsState.at(index).on = light.on;
				}
				if (light.brightness != existinglight.brightness) {
					ordered_json j = ordered_json{ {"id", light.id}, {"brightness", light.brightness}};

					cout<<j.dump(4)<<endl;

					// Update the curentLightState
					currentLightsState.at(index).brightness = light.brightness;
				}
				if (light.name != existinglight.name) {
					ordered_json j = ordered_json{ {"id", light.id}, {"name", light.name}};

					cout<<j.dump(4)<<endl;

					// Update the curentLightState
					currentLightsState.at(index).name = light.name;
				}
				// Do not search anymore in the vector once you have found it
				continue;
			}

			index++;
		}
		if (!foundIt) {
			// What if new light has been added? --> if light in newLights does not exist in currentLightsState, then add it.
			cout<<"New light has been discovered id="<<light.id<<"\n"<<to_json(light).dump(4)<<endl;

			light.isValid = true;
			currentLightsState.push_back(light);
		}
	}

	for (int i = 0; i < currentLightsState.size(); i++) {
		//printf("Testing for validity ID %d isValid = %d\n", currentLightsState.at(i).id, currentLightsState.at(i).isValid);
		if (!currentLightsState.at(i).isValid) {
			// This means we did not detect a light that existed last pass through. It must have gone offline or had an error.
			// Remove it from the currentLightSet
			cout<<"No longer receiving communication from light ID: "<< currentLightsState.at(i).id<<". Removing it from known lights"<<endl;
			currentLightsState.erase(currentLightsState.begin() + i);
		}
	}
}

/**
 * Make the HTTP request via the CURL handle. The response string is saved into the preset string 
 * from the curl handle.
 *
 * @param curl 			Pointer to CURL handle to be used in HTTP requests.
 * @param sleep   		Time in microseconds bewteen each GET request.
 * @param retryAttempts Attemps to retry making a connection with the server before giving up. 
 * @return Bool 		Success or failure of request 		
 */
bool MakeHTTPRequest(CURL* curl, int sleep, int retryAttempts) {
  	CURLcode res;
	// Make the request
    res = curl_easy_perform(curl);

	// If we are unable to make a connection to the server, retry a set number of times
    if(res != CURLE_OK) {
  		fprintf(stderr, "Function MakeHTTPRequest: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

  		// If we cannot reach the server after retryAttempts, exit the program with error code
  		if (!AttemptHTTPRequestRetry(retryAttempts, sleep, curl)) {
  			char *url = NULL;
  			
  			curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);

  			printf("\nUnable to establish connection to server at %s.\n", url);
  			return false;
  		}
  		else {
    		printf("Able to re-establish connection to server. Proceed.\n");
  		}
    }

    return true;
}


/**
 * Get the individual Light objects from the server given the number of lights the server has running.
 *
 * @param url 		String url to connect to
 * @param timeout 	Time in seconds before a timeout on the GET request.
 * @param elements 	Number of elements found in the "Query all" GET request
 * @return vector<HueLight> Vector of individual HueLight objects that were found on the server
 */
vector<HueLight> GetLightObjects(string url, int timeout, int elements) {
	// For each light we found in the ALL request, request its specifics and return a vector of light objects
	vector<HueLight> lights;

    // Hardcode the retry information for individual light requests
    int lightRetryAttempts = 3;
    int lightSleep = 100;

	for (int i = 1; i <= elements; i++) {
  		string urlString = url + to_string(i);
		string responseString;

		// printf("For debugging: \tURL: [%s]\n", urlString.c_str());

		// Create char* url
		char* url = createURLFromString(urlString);

		CURL *curl = CreateHTTPCurlHandle(url, timeout, &responseString);

		if (!curl) {
			// Unable to create CURL object
			// cout<<"For debugging: Something went wrong creating curl object"<<endl;
			curl_easy_cleanup(curl);
			continue;
		}

		if (!MakeHTTPRequest(curl, lightSleep, lightRetryAttempts)) {
			// Something went wrong in the request, do not process responseString for JSON
			// cout<<"For debugging: Something went wrong in the HTTP request"<<endl;
			curl_easy_cleanup(curl);			
			continue;
		}

  		//cout<<"For debugging: \nResponse string: [[["<<responseString<<"]]]\n";

	    if (responseString == "") {
	    	//printf("There was no information for element with id = %d. It was not due to a failure on the server side. Assume light has gone offline. \n", i);
	    	curl_easy_cleanup(curl);
	    	continue;
    	}

  		// Validate the incoming JSON responseString --> make sure always have all fields correct
		// json library throws an error when there is illegal access:
		//	"accessing an invalid index (i.e., an index greater than or equal to the array size) or the passed object key is non-existing, an exception is thrown." (https://nlohmann.github.io/json/features/element_access/checked_access/)
		try {
			HueLight light;
			json j = json::parse(responseString);
			//cout<<"For debugging: j: "<<j.dump(4)<<endl;

			light.id = i;
			light.name = j.at("name");
	  		light.on = j.at("state").at("on");
			light.bri = j.at("state").at("bri");

	  		// https://developers.meethue.com/develop/hue-api/lights-api/
	  		// Note: Brightness of the light. This is a scale from the minimum brightness the light is capable of, 1, to the maximum capable brightness, 254.
			int bri = light.bri;
			if (bri > 254) bri = 254;
			if (bri < 1) bri = 1;
			light.brightness = (int) (100 * bri / 254);

			// If no error has been thrown, add the light to the lights vector
			lights.push_back(light);
		} catch (...) {
			printf("ERROR: Program is unable to parse JSON object for ID = %d.\n", i);
			//printf("This is most likely due to invalid JSON format in response string resulting in json.exception.out_of_range error.\n");
		}

		// Perform necessary clean up with CURL handle
		curl_easy_cleanup(curl);
	}

    return lights;
}

// Configure the parser to accept the correct commandline arguments
void configure_parser(cli::Parser& parser) {
	parser.set_optional<int>("t", "timeout", 10, "Integer timeout is the maximum time in seconds that you allow the HTTP request operation to take");
	parser.set_optional<int>("s", "samplesPerMinute", 60, "Integer samplesPerMinute is the number of HTTP requests in a minute. Default is 60 (1 request every second).");
	parser.set_optional<int>("r", "retryRequests", 10, "Integer retry requests is the number of retries to connect to the server before failure (program ends)");
	parser.set_optional<int>("p", "port", 80, "Integer port to connect to server on.");
	parser.set_optional<std::string>("n", "hostname", "localhost", "Hostname of server to connect to."); // h is reserved for "help"
}


//. Prints out changes.
/**
 * Updates the currentLightsState vector to have active lights from latest request. 
 * This function also prints out the state changes and initial state of the application (lights).
 *
 * @param url 		String url to connect to
 * @param timeout 	Time in seconds before a timeout on the GET request.
 * @param elements 	Number of elements found in the "Query all" GET request
 * @return vector<HueLight> Vector of individual HueLight objects that were found on the server
 */
void ProcessJSONLightsResonse(vector<HueLight> &currentLightsState, int elements, string url, int timeout, int runCount) {
	// For each light we find, we need to get its attributes 
	vector<HueLight> lights = GetLightObjects(url, timeout, elements);

	// Do the following for the first request being made
	if (runCount == 0) {
		// Perform deep copy of vector
	    copy(lights.begin(), lights.end(), back_inserter(currentLightsState)); 
	    
		//Print the json objects as dump
		ordered_json output = to_json_vector(lights);

		cout<<output.dump(4)<<endl;

		return;
	}

	// Need to compare the newly retrieved lights to the currentLightsState and print the differences.
	CompareAndUpdateLightStates(currentLightsState, lights);

	// cout<<"For debugging: "<<runCount<<": Current light vector\n"<<to_json_vector(currentLightsState).dump(4)<<endl;
}

/**
 * Run the simulation.
 *
 * This functions drives the program. It accepts as input the parameters retrieved as arguments (or defaults) and sets up 
 * the loop for the HTTP requests. It monitors the general connection to the server and requests "all" of the lights alive on it. It
 * will then pass the vector of lights to another function monitors. 
 *
 *
 * @param hostname 		Hostname to connect to.
 * @param portNumber 	Port to connect to.
 * @param timeout 		Time in seconds before a timeout on the GET request.
 * @param sleep   		Time in microseconds bewteen each GET request.
 * @param retryAttempts Attemps to retry making a connection with the server before giving up. 
 * @return Integer for success or failure.
 */
int RunProgram(string hostname, int portNumber, int timeout, int sleep, int retryAttempts) {
  	CURLcode res;
	vector<HueLight> currentLightsState;
	json j;
    int requestsMade = 0;
	int runCount = 0;
	int elements = 0;
    string responseString;
    string urlString;

	urlString = "http://"+hostname+":"+to_string(portNumber)+"/api/newdeveloper/lights/";
	char* url = createURLFromString(urlString);
	CURL *curl = CreateHTTPCurlHandle(url, timeout, &responseString);

	if (!curl) {
		// Unable to create CURL object
		curl_easy_cleanup(curl);
		return 1;
	}

	printf("Connecting to %s\n\n", urlString.c_str());

	while (curl) {
		// Clear the resonse string
		responseString.clear();

		// Attempt to make the HTTP request
		if (!MakeHTTPRequest(curl, sleep, retryAttempts)) {
			// Something went wrong in the request, do not process responseString for JSON
			printf("\nUnable to establish connection to server. Exiting program.\n");
			return 1;
		}
    	
    	// If there is no information to process in the response string, do not proceed
	    if (responseString == "") {
	    	continue;
    	}

		try {
			// Attempt to parse the json
			j= json::parse(responseString);
		} catch (...) {
			printf("ERROR: Program is unable to parse JSON object.\n");
			//printf("This is most likely due to invalid JSON format in response string resulting in json.exception.out_of_range error.\n");
			continue;
		}

		elements = 0;
		// Get the number of elements. TODO: There must be a better way to get the length of the json objects (i.e. for an iterable object)
		for (auto it = j.begin(); it != j.end(); ++it)
		{
			//For debugging: cout<<"it "<<it.key()<<j.at(it.key())<<endl;
			elements+=1;
		}

		// Updates the currentLightsState vector to have active lights from latest request. Prints out changes.
		ProcessJSONLightsResonse(currentLightsState, elements, urlString, timeout, runCount);	

		runCount++;
		
		usleep(sleep);
	}

    curl_easy_cleanup(curl);
    
    return 0;
}

/*
	Main function accepts paramters from the command line. the parameters indicate where the simulator is being run and request information.
	It calls the driving function RunProgram to begin executing the grunt of the application.
*/
int main(int argc, char *argv[]) {
	// Use cli::Parser to accept and sanitize the command line arguments
	cli::Parser parser(argc, argv);
	configure_parser(parser);
	parser.run_and_exit_if_error();

	// Retrieve command line arguments and sanitize input.
	int timeout = parser.get<int>("t");
	int samplesPerMinute = parser.get<int>("s");
	int retryAttempts = parser.get<int>("r");
	int portNumber = parser.get<int>("p");
	string hostname = parser.get<std::string>("n");

	double samplesPerSecond = samplesPerMinute / 60.0;
	// Sleep in microseconds between GET requests 
	int sleep = (int) (1000000 / samplesPerSecond);

	printf("\nWelcome to the Philips Hue Console Application. Connecting to server using the following parameters:\n\n");
	printf("Hostname:\t\t\t%s \n", hostname.c_str());
	printf("Port number:\t\t\t%d\n", portNumber);
	printf("Samples per minute:\t\t%d\n", samplesPerMinute);
	printf("Seconds between requests:\t%.2f\n", sleep/1000000.0);
	printf("Retry attempts: \t\t%d\n", retryAttempts);
	printf("Timeout (seconds):\t\t%d\n", timeout);
	printf("\nGet ready! Begin simulation!\n\n");

	return RunProgram(hostname, portNumber, timeout, sleep, retryAttempts);
}