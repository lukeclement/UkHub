//
//  main.cpp
//  Logistics Hub
//  A program to determine the location of location hub(s) to serve the UK
//  population.
//
//  Created by Luke Clement on 26/11/2018.
//  Copyright © 2018 Luke Clement. All rights reserved.
//

#include <iostream>
#include <cmath>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <random>
#include <thread>
#include <ctime>

using namespace std;

//Structure for hubs
struct hub{
    double lat;
    double lon;
    double fitness;
    int servicing;
};

//Structure for many hubs
struct collection{
    double fitness;
    //Hub connections
    vector<int> connections;
};
//Structure for boundaries
struct bounds{
    double maxLat;
    double maxLong;
    double minLat;
    double minLong;
};
//Structure containing data for the optimal hub
struct opInfo{
    int iterations;
    vector<hub> finals;
    collection addon;
};
//Structure to contain data on places
struct placesInfo{
    vector<vector<double> > nums;
    vector<vector<string> > names;
};

//Getting places (reading file)
placesInfo getPlaces(string fileName){
    placesInfo output;
    ifstream file(fileName);
    
    string line;
    string number;
    //Places have only numbers in [0, 0, pop, lat, long]
    //Names has raw string data in [name, type, pop, lat, long]
    vector< vector <double> > places;
    vector< vector <string> > names;
    vector<double> data;
    vector<string> raw;
    
    int start, end, x;
    bool last = false;
    
    if(file.is_open()){
        cout << "Reading file " << fileName << "...\n";
        //Opening file
        x = 0;
        while(!file.eof()){
            //Getting line
            getline(file, line);
            if(x != 0){
                //Start and end points of commas
                start = 0;
                end = 0;
                //Going through each line
                last = false;
                for(int i = 0; end != -1; i++){
                    //Finding next comma
                    end = (int)line.find(",", i);
                    //Checking validity
                    if(end - start > 0){
                        last = true;
                        //Getting number and adding to data
                        number = line.substr(start, end - start);
                        data.push_back(atof(number.c_str()));
                        raw.push_back(number);
                        
                    }else if(end == -1&&last){
                        //Hit the end of the line
                        end = (int)line.length();
                        //Adding to data
                        number = line.substr(start, end-start);
                        data.push_back(atof(number.c_str()));
                        raw.push_back(number);
                        //Setting end to -1 to end loop
                        end = -1;
                    }
                    //Resetting start
                    start = end + 1;
                }
                //Adding data to places and resetting data
                if(last){
                    places.push_back(data);
                    names.push_back(raw);
                    data.clear();
                    raw.clear();
                }
            }
            x++;
        }
    }else{
        //Error message
        cout << fileName << " didn't open!" << "\n";
        exit(1);
    }
    
    //Closing file
    file.close();
    cout << "Found all " << places.size() << " entries!\n";
    
    output.nums = places;
    output.names = names;
    return output;
}

//Getting lat/long bounds
bounds getBounds(vector< vector<double> > places){
    bounds newBounds;
    
    //Finding boundaries for lat/long
    double maxLat = -INFINITY;
    double minLat = INFINITY;
    double maxLong = -INFINITY;
    double minLong = INFINITY;
    //scanning through lat/long values
    for(int i = 0; i < places.size(); i++){
        if(minLat > places[i][3]){
            minLat = places[i][3];
        }if(maxLat < places[i][3]){
            maxLat = places[i][3];
        }
        if(minLong > places[i][4]){
            minLong = places[i][4];
        }if(maxLong < places[i][4]){
            maxLong = places[i][4];
        }
    }
    //Setting variable values
    newBounds.maxLat = maxLat;
    newBounds.minLat = minLat;
    newBounds.maxLong = maxLong;
    newBounds.minLong = minLong;
    return newBounds;
}

//Finding Weighted distances
double hDist(double startLat, double endLat, double startLong, double endLong, double weight){
    //Setting conversion constant and radius of Earth
    double convert = M_PI/180.0;
    double R = 6371.01;
    //Converting all the amounts
    double ela = endLat*convert;
    double sla = startLat*convert;
    double elo = endLong*convert;
    double slo = startLong*convert;
    //Actually calculating haversine distance
    double Dlat = (ela-sla);
    double Dlong = (elo-slo);
    
    double a = pow(sin(Dlat/2) , 2) + cos(sla)*cos(ela)*pow(sin(Dlong/2) , 2);
    
    double b = 2*atan2(sqrt(a) , sqrt(1.0-a));
    
    //Multipling distance by weight factor
    return R*b*weight;
}

//Finding the fitness of a single hub
double findFitness(double lat, double lon, vector< vector<double> > places){
    //Inital fitness is zero
    double fitness = 0;
    for(int i=0; i < places.size(); i++){
        //Add the weighted distance to each place
        fitness += hDist(lat, places[i][3], lon, places[i][4], places[i][2]);
    }
    return fitness;
}

//Finding the fitness of multiple hubs, all connected to different places and those connections
collection findDualFitnesses(vector< hub > dualHubs, vector< vector<double> > places){
    //setting variables
    collection col;
    
    vector< int > connections;
    
    double bestFitness;
    double fitness = 0;
    int best;
    //Starting the loop
    for(int i = 0; i < places.size(); i++){
        //Best fitness so far is unknown
        bestFitness = INFINITY;
        //Loop through all the hubs
        for(int j = 0; j < dualHubs.size(); j++){
            //Looking for best fitness
            if(bestFitness > hDist(dualHubs[j].lat, places[i][3], dualHubs[j].lon, places[i][4], places[i][2])){
                //letting the program know a best fitness so far has been found
                bestFitness = hDist(dualHubs[j].lat, places[i][3], dualHubs[j].lon, places[i][4], places[i][2]);
                best = j;
            }
        }
        //Adding on the best connection to the pile
        fitness += bestFitness;
        connections.push_back(best);
    }
    //Outputting results
    col.connections = connections;
    col.fitness = fitness;
    return col;
}

//Hill climb stuff
void threadStuff(hub &startHub, double search, vector<vector<double>> places, bool &changing){
    double currentFit, testFit;
    double dx=0;
    double dy=0;
    currentFit = startHub.fitness;
    for(int i = -10; i <= 10; i++){
        for(int j = -10; j <= 10; j++){
            if(i != 0 || j != 0){
                //Having a look around
                testFit = findFitness(startHub.lat + i*search, startHub.lon + j*search, places);
                //Seeing if found a better fit
                if(testFit < currentFit){
                    changing=true;
                    dx = i*search;
                    dy = j*search;
                    currentFit = testFit;
                }
            }
        }
    }
    //Editing hub
    startHub.lat += dx;
    startHub.lon += dy;
    startHub.fitness=currentFit;
    
}
//Optimising for _____single______ hub
opInfo optimise(vector< hub > hubs, vector< vector<double> > places){
    opInfo output;
    vector< hub > newHubs = hubs;
    
    //Relocating hubs
    double search = 0.001;
    //double currentFit, testFit, dx, dy;
    bool changing = true;
    int iterations = 0;
    
    //Starting the loop to find local minimum(s)
    while(changing){
        //Seeing how many iterations it took
        iterations++;
        changing = false;
        //Looping through the hubs
        vector<thread> threads;
        //time_t current_time;
        //a=time(NULL);
        for(int k = 0; k < newHubs.size(); k++){
            /*
             dx = 0;
             dy = 0;
             //Getting start fitness
             currentFit = newHubs[k].fitness;
             for(int i = -10; i <= 10; i++){
             for(int j = -10; j <= 10; j++){
             if(i != 0 || j != 0){
             //Having a look around
             testFit = findFitness(newHubs[k].lat + i*search, newHubs[k].lon + j*search, places);
             //Seeing if found a better fit
             if(testFit < currentFit){
             changing = true;
             dx = i*search;
             dy = j*search;
             currentFit = testFit;
             }
             }
             }
             }
             //Editing hub
             newHubs[k].lat += dx;
             newHubs[k].lon += dy;
             newHubs[k].fitness=currentFit;
             */
            //thread test(threadStuff, ref(newHubs[k]),search,places,ref(changing));
            threads.push_back(thread(threadStuff, ref(newHubs[k]),search,places,ref(changing)));
            //b=time(NULL)
            //test.join();
        }
        for(auto& th : threads){
            th.join();
        }
        
    }
    //Output results
    output.iterations = iterations;
    output.finals = newHubs;
    output.iterations = iterations;
    return output;
}

//Optimising for _____multiple______ hubs
opInfo multiBALL(vector< hub > oldHubs, vector< vector<double> > places, int minMax){
    //Setting variables
    opInfo output;
    collection standing;
    collection newStanding;
    
    vector<hub> hubs = oldHubs;
    vector<hub> testHubs;
    vector< vector<double> > subPlaces;
    
    int iterations = 0;
    bool testing = true;
    //Setting standing fitness and also starting new standing fitness
    standing = findDualFitnesses(hubs, places);
    newStanding = findDualFitnesses(hubs, places);
    //Staring loop
    while(testing) {
        testing = false;
        iterations++;
        //Looping through each hub
        for(int i = 0; i < hubs.size(); i++){
            //Getting conenctions, making each hub optimised for the cities it is connected to
            for(int j = 0; j < standing.connections.size(); j++){
                if(standing.connections[j] == i){
                    subPlaces.push_back(places[j]);
                    
                }
            }
            //Optimising each hub
            testHubs.push_back(hubs[i]);
            hubs[i] = optimise(testHubs, subPlaces).finals[0];
            testHubs.clear();
            subPlaces.clear();
        }
        //Making a new standing
        newStanding = findDualFitnesses(hubs, places);
        //Checking to see if new fitness is better than the old one
        if(newStanding.fitness<standing.fitness){
            testing = true;
            standing = findDualFitnesses(hubs, places);
        }
    }
    
    output.iterations=iterations;
    output.finals=hubs;
    output.addon=standing;
    return output;
}

//Generating random hubs
vector<hub> getHubs(bounds boundaries, int numOfHubs, vector<vector<double> > places){
    vector<hub> hubs;
    //Randomness generator
    random_device random;
    //Using Mersenne Twister engine (64 bit unsigned)
    mt19937_64 generate(random());
    uniform_real_distribution<> distLat(boundaries.minLat,boundaries.maxLat);
    uniform_real_distribution<> distLong(boundaries.minLong,boundaries.maxLong);
    hub alpha;
    //Setting random hubs
    for(int i = 0; i < numOfHubs; i++){
        alpha.lat = distLat(generate);
        alpha.lon = distLong(generate);
        alpha.fitness = findFitness(alpha.lat, alpha.lon, places);
        alpha.servicing = 0;
        //for(int j=0;j<places.size();j++){
        //    alpha.servicing+=places[j][2];
        //}
        hubs.push_back(alpha);
    }
    return hubs;
}

//Calculating possibilities concurrently
void possible(int i, bounds boundaries, int nums, vector<vector<double>> places, vector<opInfo> &data){
    vector<hub> hubsA;
    opInfo moreData;
    cout << "Starting thread " << i << "\n";
    hubsA = getHubs(boundaries, nums, places);
    moreData = multiBALL(hubsA, places, 10);
    
    hubsA = moreData.finals;
    cout << "Thread " << i << " has fitness " << moreData.addon.fitness <<"\n";
    data.push_back(moreData);
}

//Adjecency matrix
vector< vector< vector<double> > > adj(vector<vector<double>> places, opInfo hubData){
    vector< vector< vector<double> > > matricies;
    vector<double> dists;
    vector<vector<double> > adjs;
    vector<vector<double>> subPlaces;
    hub testHub;
    for(int i=0;i<hubData.finals.size();i++){
        testHub=hubData.finals[i];
        for(int j=0;j<hubData.addon.connections.size();j++){
            if(hubData.addon.connections[j]==i){
                subPlaces.push_back(places[j]);
            }
        }
        for(int j=0;j<=subPlaces.size();j++){
            for(int k=0;k<=subPlaces.size();k++){
                if(j+k==0){
                    dists.push_back(0);
                }else if(j==subPlaces.size()){
                    dists.push_back(hDist(hubData.finals[i].lat, subPlaces[k][3], hubData.finals[i].lon, subPlaces[k][4], subPlaces[k][2]));
                }else if(k==subPlaces.size()){
                    
                }else{
                    
                }
            }
            adjs.push_back(dists);
            dists.clear();
        }
    }
    
    return matricies;
}

//TSP
vector<collection> tsp(vector<vector<double>> places, opInfo hubData){
    vector<collection> output;
    
    
    return output;
}

//Starting up program
int main(int argc, const char * argv[]) {
    opInfo moreData;
    opInfo bestData;
    //Places have only numbers in [0, 0, pop, lat, long]
    vector< vector <double> > places;
    vector< vector <string> > placeName;
    vector<hub> hubsA;
    vector<hub> best;
    
    string fileName = "GBplaces.csv";
    //fileName="USplaces.csv";
    int nums, loops, q;
    double servicing;
    double bestFit = INFINITY;
    //Reading file
    placesInfo information = getPlaces(fileName);
    places = information.nums;
    placeName = information.names;
    
    vector<hub> hubs;
    
    //Find boundaries
    bounds boundaries = getBounds(places);
    
    cout << "Min lat: " << boundaries.minLat << "; Max lat: " << boundaries.maxLat << "; Min long: " << boundaries.minLong << "; Max long: " << boundaries.maxLong << "\n";
    //Requesting user input
    cout << "How many hubs would you like to place?\n";
    cout << ">>";
    cin >> nums;
    
    cout << "How many threads would you like to use to test? (Higher is more CPU intensive but can find better fits)\n";
    cout << ">>";
    cin >> loops;
    
    //Looking at each possibility
    vector<thread> threads;
    vector<opInfo> data;
    
    for(int i = 0; i < loops; i++){
        threads.push_back(thread(possible, i, boundaries, nums, places, ref(data)));
        /*cout << "Observing possibility " << i << ", which has fitness ";
        hubsA = getHubs(boundaries, nums, places);
        moreData = multiBALL(hubsA, places, 10);
        hubsA = moreData.finals;
        cout << moreData.addon.fitness<< ", compared to the best fitness before this, "<< bestFit << "\n";
        if(moreData.addon.fitness<bestFit){
            bestFit = moreData.addon.fitness;
            best = hubsA;
            bestData = moreData;
        }*/
    }
    for(int i=0;i< threads.size();i++){
        threads[i].join();
    }
    for(int i=0;i<data.size();i++){
        if(data[i].addon.fitness<bestFit){
            bestFit = data[i].addon.fitness;
            best = data[i].finals;
            bestData = data[i];
        }
    }
    cout << "Complete!\n\n";
    
    cout << "Found " << nums << " hubs, with a total node length of "<< bestData.addon.fitness << ", would you like to see:\n";
    cout << "[0]:   Hub locations\n";
    cout << "[1]:   Hub locations and connections\n";
    cout << "[2]:   Hub locations, connections and number of people hub is servicing\n";
    cout << "[3]:   Hub locations and number of people hub is servicing\n";
    cout << "[Else]:No output\n";
    cout << ">>";
    cin >> q;
    
    for(int i = 0; i < nums; i++){
        if(q >= 0 && q <= 3){
            cout << "Hub found at: " << best[i].lat << " , " << best[i].lon << "\n";
        }
        servicing = 0;
        if(q == 1 || q == 2 || q == 3){
            if(q != 3){
                cout << "This hub is connected to:\n";
            }
            for(int j = 0; j < bestData.addon.connections.size(); j++){
                if(bestData.addon.connections[j] == i){
                    if(q != 3){
                        cout << placeName[j][0] << "\n";
                    }
                    servicing += places[j][2];
                }
            }
        }
        if(q == 2 || q == 3){
            cout << "Servicing " << servicing << " people\n";
        }
    }
    
    return 0;
}
