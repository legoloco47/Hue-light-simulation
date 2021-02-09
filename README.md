# Hue Light Simulation Coding Challenge
This project was written by Helen Edelson in C++ 11 to implement the coding challenge for Josh.ai.

## To build the project:
Run the makefile
```
make 
```

then run the newly created executable
```
./HUELightSimulation 
```
with any of the following command line parameters. 

### Flags: 
|arg| arg string   | default 	 | type 		| Definition	|
|---|:------------:|:-------------:|:---------:|:-------------:|
| -t| --timeout   	|	10		| Integer |The maximum time in seconds that you allow the HTTP request operation to take|
| -s|--samplesPerMinute|  60 	| Integer | The number of HTTP requests in a minute.|
| -r|--retryRequests|  10		| Integer | Number of retries to connect to the server before failure (program ends)|
| -p|--port 		| 	80 		| Integer |Port to connect to server on.|
| -n|--hostname 	|localhost| String | Hostname of server to connect to.|

#### Example:
```
./HUELightSimulation

./HUELightSimulation --hostname localhost -s 700 -p 8080

./HUELightSimulation --samplesPerMinute 30 -r 5
```

Note: The simulator server can be started a few different ways that needed to be accounted for in the argument handling above. For proper results, please ensure the port and hostname match for the console application and server.

```
# start the simulator on localhost:80 
sudo hue-simulator

# start the simulator on localhost:8080 
hue-simulator --port=8080
 
# start the simulator on 127.0.3.1:80 
sudo ifconfig lo0 alias 127.0.3.1
sudo hue-simulator --hostname=127.0.3.1
```

## Program Requirements
 1. When the program starts, it should print out all lights and their state (on/off, brightness, name and ID). 
 2. After the initial print, the program should only output changes to the light states. These state changes include:
 	1. On/off
	1. Brightness (as a % (minimum 1 and max value being 254))
	1. Name


## Assumptions
1. ID as a primary key

	According to the Philips Hue documentation, the MAC address is the only unique key. However, for this application, I am using the ID as the primary key.

2. Brightness changes

	The specs request that brightness is reported in a percentage. The brightness has a valid range from 1 to 254. However, it can be set to any integer. In the program, I use “bri” to represent actual set value and “brightness” to represent the percentage reported out. So, if the bri is set to 500, the brightness=100%. If the "bri" value changes from 500 to 400 (or from -50 to -100), the actual brightness percentage does not change (i.e. 100% to 100%, 0% to 0%). Thus, I am not reporting these changes in the "state change" section of the code. 

3. Order of handling changes

	If the light has multiple state changes in a single request (i.e. a name change, a change to "off" and a brightness change from 200 to 50), I report the power change (on or off), then the brightness change, then the name change in that order. 




## Libraries
#### HTTP library:
I used libcurl (https://curl.se/) to process HTTP requests. I chose this one because it had good documentation, it looked easy to use, and it came highly recommended.
#### JSON library:
I used nlohmann json to handle the JSON objects.
#### Argument Parser:
I used the recommended cmdparser to parse the arguments. I did this part last and was pleasantly surprised with how easy, straightforward and complete the library was.


## Next steps in application lifecycle
1. Create test suite 
	See tests below
2. Create PR and await feedback!
3. Start thinking about security. Potentially start with sanitizing input received from the server (especially on "name" changes and string input).



## Future Test Suite to Implement
1. No response from server (HUE simulator is down)
>Solution: Set number of retry attempts. Verify correct number of attempts before failure. Verify successful reconnect. Verify failure to reconnect has graceful exit.

2. Server response time out
>Solution: Use Curllib's timeout functionality to set a timeout value. Verify timeout value is upheld.

3. Empty JSON server response
>Solution: Ignore empty responses from server. Verify empty JSON is handled gracefully.

4. Add/remove light(s) on the server
>Solution: compare all recently discovered lights with the current list of lights. any new lights are added to the current list. any lights on the current list that are not in the recently discovered are removed. Verify new and removed lights are handled properly.

6. Invalid JSON response from server (ex: return an integer when expecting a bool)
>Solution: Catch invalid JSON responses from server. Verify bad JSON is handled gracefully.

7. Change "name", "brightness", and "on" in various combinations and expect correct results
>Verify correct results and reporting of state changes from different combinations of changes.

### Additional tests after PR:
8. Send back non-JSON response strings from server
> Verify program correctly handles strings that cause JSON parsing errors


## Reflection:
The most difficult part of this project was learning to use HTTP and JSON with C++. I have done it in other languages (JS, C, etc.), but never with C++. Making the HTTP requests and the JSON objects function properly (including understand the API and research) occupied the majority of my development time. Overall, this challenge was a fun use of real-world IOT APIs and allowed me to see a bit into Josh.ai's domain and future work.

### Time Allocation
| ~ Hours | Work 												|
| ----- |:-------------------------------------------------:|
| 1		| Understanding project requirements and planning	|
| 2 	| Understanding & implementing C++ HTTP requests    |
| 2		| Understanding & implementing C++ JSON objects   	|
| 3.5  | Satisfying requirements, debugging, testing, etc. 	|
| 2		| Code clean-up, documenting, writing Readme, etc.|


 ### Total: 10.5 Hours


