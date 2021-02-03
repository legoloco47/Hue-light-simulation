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
#include "HUELightSimulator.h"

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
bool SuccessfulRetryPhase(int retryAttempts, int sleepTime, CURL *curl) {
	int attempt = 1;
  	CURLcode res;

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
 *
 * This function compares and updates the new light situation to the light state in memory. We need a way to compare
 * the lights found in the most recent request to the ones we have saved from the last request. In this function we compare
 * the two vectors and make sure 
 *
 *
 * @param currentLightsState 	Vector of HueLight objects that were found on the server last request
 * @param newLights 			Vector of HueLight objects that were found on the server in the most recent request
 */
void compareAndUpdateLightStates(vector<HueLight> &currentLightsState, vector<HueLight> newLights) {
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
			// What if new light has been added?
			//	if light in newLights does not exist in currentLightsState, then add it.
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
 * Get the individual Light objects from the server given the number of lights the server has running.
 *
 * @param url 		String url to connect to
 * @param timeout 	Time in seconds before a timeout on the GET request.
 * @param sleep   	Time in microseconds bewteen each GET request.
 * @param elements 	Number of elements found in the "Query all" GET request
 * @return vector<HueLight> Vector of individual HueLight objects that were found on the server
 */
vector<HueLight> GetLightObjects(string url, int timeout, int sleep, int elements) {
	// For each light we found in the ALL request, request its specifics and return a vector of light objects
	vector<HueLight> lights;
	CURL *curl;
  	CURLcode res;
    curl = curl_easy_init();
    bool allGood = true;

  	if (curl) {
  		for (int i = 1; i <= elements; i++) {
	  		// Provide the url to use in the request
	  		string urlString = url + to_string(i);
	  		char* url = new char[urlString.length() + 1];
	  		strcpy(url, urlString.c_str());

  		  	//For debugging: printf("For debugging: \nURL: [%s]\n", url);
  		  	// URL to connect to
	  		curl_easy_setopt(curl, CURLOPT_URL, url);

	  		// specify the HTTP protocl version. The server runs on 1.1
	    	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_1_1);

		    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);

	    	// Save the value returned into a json object
	        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);

	        std::string response_string;
	        // The string to collect the response in
		    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    		// Make the request
		    res = curl_easy_perform(curl);

			// If we are unable to make a connection to the server, retry a set number of times
	        if(res != CURLE_OK) {
	      		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

	      		// If we cannot reach the server after retryAttempts, exit the program with error code
	      		if (!SuccessfulRetryPhase(3, 100, curl)) {
	      			// Perform necessary cleanup
	      			printf("\nUnable to establish connection to light %d. Assuming it has gone offline.\n", i);
	      			continue;
	      		}
	      		else {
	        		printf("Able to re-establish connection to server and light %d. Proceed.\n", i);
	      		}
	        }
	        if (response_string == "") {
	        	//printf("There was no information for element with id = %d. It was not due to a failure on the server side. Assume light has gone offline. \n", i);
	        	continue;
	        }
	  		//cout<<"For debugging: \nResponse string: [[["<<response_string<<"]]]\n";

			json j = json::parse(response_string);
			//For debugging: cout<<"j: "<<j.dump(4)<<endl;

	  		// Validate the incoming JSON response_string --> make sure always have all fields correct
			// json library throws an error when there is illegal access:
			//	"accessing an invalid index (i.e., an index greater than or equal to the array size) or the passed object key is non-existing, an exception is thrown." (https://nlohmann.github.io/json/features/element_access/checked_access/)
			try {
				HueLight light;
				light.name = j.at("name");
		  		light.id = i;
		  		light.on = j.at("state").at("on");
				int bri = j.at("state").at("bri");
				light.bri = bri;
		  		// https://developers.meethue.com/develop/hue-api/lights-api/
		  		// Note: Brightness of the light. This is a scale from the minimum brightness the light is capable of, 1, to the maximum capable brightness, 254.
				if (bri > 254) bri = 254;
				if (bri < 1) bri = 1;
				light.brightness = (int) (100 * bri / 254);

				// If no error has been thrown, add the light to the lights vector
				lights.push_back(light);
			} catch (...) {
				printf("ERROR: Program is unable to parse JSON object for ID = %d.\n", i);
				//printf("This is most likely due to invalid JSON format in response string resulting in json.exception.out_of_range error.\n");
			}
		}
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
	CURL *curl;
  	CURLcode res;
    curl = curl_easy_init();
    bool allGood = true;
    bool initialRun = true;
	vector<HueLight> currentLightsState;

	string urlString = "http://"+hostname+":"+to_string(portNumber)+"/api/newdeveloper/lights/";
	char* url = new char[urlString.length() + 1];
	strcpy(url, urlString.c_str());

  	if (curl) {
  		printf("Connecting to %s\n\n", url);
  		// Provide the url to use in the request
  		curl_easy_setopt(curl, CURLOPT_URL, url);

  		// Specify the HTTP protocl version. The server runs on 1.1
    	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_1_1);

    	// Set timeout field (seconds)
	    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);

    	// Save the value returned into a json object
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);

    	// Perform blocking transfer 
    	while (allGood) {
	        // The string to collect the response in
	        std::string response_string;
		    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    		// Make the request
		    res = curl_easy_perform(curl);

		    // If we are unable to make a connection to the server, retry a set number of times
	        if(res != CURLE_OK) {
	      		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

	      		// If we cannot reach the server after retryAttempts, exit the program with error code
	      		if (!SuccessfulRetryPhase(retryAttempts, sleep, curl)) {
      				printf("\nUnable to establish connection to server. Exiting program.\n");

	      			// Perform necessary cleanup
	      			curl_easy_cleanup(curl);
	      			return 1;
	      		}
	      		else {
	        		printf("Able to establish connection to server. Proceed with application.\n");
	      		}
	        }

			//Process the response to collect the number of lights 
			int elements = 0;
			json j = json::parse(response_string);

			// Get the IDs of the lights found 
			for (auto it = j.begin(); it != j.end(); ++it)
			{
				//For debugging: cout<<"it "<<it.key()<<j.at(it.key())<<endl;
				elements+=1;
			}

			// For each light we find, we need to get its attributes 
			vector<HueLight> lights = GetLightObjects(urlString, timeout, sleep, elements);

			if (initialRun) {
				// Perform deep copy of vector
			    copy(lights.begin(), lights.end(), back_inserter(currentLightsState)); 
			    
				//Print the json files as dump
				ordered_json output = to_json_vector(lights);
				cout<<output.dump(4)<<endl;
				initialRun = false;
			}
			else {
				// Need to compare the newly retrieved lights to the currentLightsState and print the differences.
				compareAndUpdateLightStates(currentLightsState, lights);
				//For debugging: cout<<"Current light vector\n"<<to_json_vector(currentLightsState).dump(4)<<endl;
			}

			usleep(sleep);
		}

	    curl_easy_cleanup(curl);
  	}
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