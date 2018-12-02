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

using namespace std;

//Structure for hubs
struct hub{
    double lat;
    double lon;
    double fitness;
    int servicing;
};

//Structure for [STUFF]
struct collection{
    double fitness;
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
    int bestHub;
    int bestFit;
    int iterations;
    vector<hub> finals;
    collection addon;
};

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
    vector< vector <double> > places;
    vector< vector <string> > names;
    vector<string> raw;
    vector<double> data;
    int start, end, x;
    bool last=false;
    if(file.is_open()){
        cout << "Reading file...\n";
        //Opening file
        x = 0;
        while(!file.eof()){
            //Getting line
            getline(file, line);
            if(x!=0){
                //Start and end points of commas
                start = 0;
                end = 0;
                //Going through each line
                last=false;
                for(int i = 0; end != -1; i++){
                    //Finding next comma
                    end = (int)line.find(",", i);
                    //Checking validity
                    if(end - start > 0){
                        last=true;
                        //Getting number and adding to data
                        number = line.substr(start, end - start);
                        data.push_back(atof(number.c_str()));
                        raw.push_back(number);
                        //cout << atof(number.c_str()) << ", ";
                        
                    }else if(end == -1&&last){
                        //Hit the end of the line
                        end = (int)line.length();
                        //Adding to data
                        number = line.substr(start, end-start);
                        data.push_back(atof(number.c_str()));
                        raw.push_back(number);
                        //cout << atof(number.c_str());
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
                    //cout << "\n";
                }
            }
            x++;
        }
    }else{
        //Error message
        cout << fileName << " file didn't open!" << "\n";
        exit(1);
    }
    
    //Closing file
    file.close();
    cout << "Got all the places!\n";
    output.nums=places;
    output.names=names;
    return output;
}

//Getting lat/long bounds
bounds getBounds(vector< vector<double> > places){
    bounds newBounds;
    
    //Finding boundaries for lat/long
    double maxLat=-INFINITY;
    double minLat=INFINITY;
    double maxLong=-INFINITY;
    double minLong=INFINITY;
    for(int i=0;i<places.size();i++){
        //cout << places[i][3] << "\n";
        if(minLat>places[i][3]){
            minLat=places[i][3];
        }if(maxLat<places[i][3]){
            maxLat=places[i][3];
        }
        if(minLong>places[i][4]){
            minLong=places[i][4];
        }if(maxLong<places[i][4]){
            maxLong=places[i][4];
        }
    }
    newBounds.maxLat=maxLat;
    newBounds.minLat=minLat;
    newBounds.maxLong=maxLong;
    newBounds.minLong=minLong;
    return newBounds;
}

//Finding Weighted distances
double hDist(double startLat, double endLat, double startLong, double endLong, double weight){
    double convert=M_PI/180.0;
    double R=6371.01;
    double ela=endLat*convert;
    double sla=startLat*convert;
    double elo=endLong*convert;
    double slo=startLong*convert;
    
    double Dlat=(ela-sla);
    double Dlong=(elo-slo);
    
    double a = pow(sin(Dlat/2) , 2) + cos(sla)*cos(ela)*pow(sin(Dlong/2) , 2);
    
    double b = 2*atan2(sqrt(a) , sqrt(1.0-a));
    
    
    return R*b*weight;
}

//Finding the fitness of a single hub
double findFitness(double lat, double lon, vector< vector<double> > places){
    double fitness=0;
    for(int i=0;i<places.size();i++){
        fitness+=hDist(lat, places[i][3], lon, places[i][4], places[i][2]);
    }
    return fitness;
}

//Finding the fitness of multiple hubs, all connected to different places and those connections
collection findDualFitnesses(vector<hub> dualHubs, vector< vector<double> > places){
    double bestFitness;
    double fitness=0;
    vector<int> connections;
    double best=0;
    
    for(int i=0;i<places.size();i++){
        bestFitness=INFINITY;
        for(int j=0;j<dualHubs.size();j++){
            //cout << "Checking hub " << j << "(Fitness:"<<hDist(dualHubs[j].lat, places[i][3], dualHubs[j].lon, places[i][4], places[i][2])<<")\n";
            if(bestFitness>hDist(dualHubs[j].lat, places[i][3], dualHubs[j].lon, places[i][4], places[i][2])){
                //cout << "Accessed\n";
                bestFitness=hDist(dualHubs[j].lat, places[i][3], dualHubs[j].lon, places[i][4], places[i][2]);
                best=j;
            }
        }
        //cout << "Current Fitness=" << fitness << "\n";
        fitness+=bestFitness;
        connections.push_back(best);
    }
    collection col;
    col.connections=connections;
    col.fitness=fitness;
    return col;
}

//Optimising for _____single______ hub
opInfo optimise(vector< hub > hubs, vector< vector<double> > places){
    opInfo output;
    vector<hub> newHubs=hubs;
    
    //Relocating hubs
    int bestHub=0;
    double bestFitness=INFINITY;
    double search=0.001;
    double currentFit, testFit, dx, dy;
    bool changing=true;
    int iterations=0;
    
    //Finding minimum of hubs
    //cout << "Phase 1\n";
    while(changing){
        //cout << "Phase 2."<<iterations<<"\n";
        iterations++;
        changing=false;
        //Getting best hubs
        for(int i=0;i<newHubs.size();i++){
            if(bestFitness>newHubs[i].fitness){
                bestHub=i;
                bestFitness=newHubs[i].fitness;
            }
        }
        
        for(int k=0;k<newHubs.size();k++){
            dx=0;
            dy=0;
            
            currentFit=newHubs[k].fitness;
            for(int i=-10;i<=10;i++){
                for(int j=-10;j<=10;j++){
                    if(i!=0 || j!=0){
                        testFit=findFitness(newHubs[k].lat+i*search, newHubs[k].lon+j*search, places);
                        
                        if(testFit<currentFit){
                            changing=true;
                            dx=i*search;
                            
                            dy=j*search;
                            currentFit=testFit;
                            
                        }
                    }
                }
            }
            newHubs[k].lat+=dx;
            newHubs[k].lon+=dy;
            newHubs[k].fitness=currentFit;
        }
        
    }
    output.iterations=iterations;
    output.finals=newHubs;
    output.bestFit=bestFitness;
    output.bestHub=bestHub;
    output.iterations=iterations;
    return output;
}

//Optimising for _____multiple______ hubs
opInfo multiBALL(vector<hub> oldHubs, vector<vector<double> > places, int minMax){
    opInfo output;
    vector<hub> hubs = oldHubs;
    vector<hub> testHubs;
    collection standing;
    collection newStanding;
    int iterations=0;
    vector< vector<double> > subPlaces;
    standing=findDualFitnesses(hubs, places);
    newStanding=findDualFitnesses(hubs, places);
    
    bool testing=true;
    
    while (testing) {
        testing=false;
        iterations++;
        
        for(int i=0;i<hubs.size();i++){
            
            for(int j=0;j<standing.connections.size();j++){
                if(standing.connections[j]==i){
                    subPlaces.push_back(places[j]);
                    
                }
            }
            testHubs.push_back(hubs[i]);
            hubs[i]=optimise(testHubs, subPlaces).finals[0];
            testHubs.clear();
            subPlaces.clear();
        }
        newStanding=findDualFitnesses(hubs, places);
        
        if(newStanding.fitness<standing.fitness){
            testing=true;
            standing=findDualFitnesses(hubs, places);
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
        alpha.lat=distLat(generate);
        alpha.lon=distLong(generate);
        alpha.fitness=findFitness(alpha.lat, alpha.lon, places);
        alpha.servicing=0;
        //for(int j=0;j<places.size();j++){
        //    alpha.servicing+=places[j][2];
        //}
        hubs.push_back(alpha);
    }
    return hubs;
}

//Starting up program
int main(int argc, const char * argv[]) {
    
    string fileName="GBplaces.csv";
    //string fileName="Test.csv";
    //Places have only numbers in [0, 0, pop, lat, long]
    vector< vector <double> > places;
    vector< vector <string> > placeName;
    placesInfo information=getPlaces(fileName);
    places=information.nums;
    placeName=information.names;
    
    vector<hub> hubs;
    
    //Find boundaries
    bounds boundaries=getBounds(places);
    
    cout << "Min lat: " << boundaries.minLat << "; Max lat: " << boundaries.maxLat << "; Min long: " << boundaries.minLong << "; Max long: " << boundaries.maxLong << "\n";
    
    //Generating random hubs (nums)
    int nums = 5;
    hubs=getHubs(boundaries,5,places);
    
    
    //Optimising
    cout << "Finding best single hub...\n";
    opInfo data=optimise(hubs, places);
    hubs=data.finals;
    //Saying what the best single hub is=
    cout << "Best single hub found at: " << hubs[data.bestHub].lat << " , " << hubs[data.bestHub].lon << " , with a total node length of " << hubs[data.bestHub].fitness << ", after "<< data.iterations << " iterations\n";
    
    cout << "How many hubs would you like to place?\n";
    cout << ">>";
    cin >> nums;
    
    vector<hub> hubsA;
    vector<hub> best;
    double bestFit=INFINITY;
    opInfo moreData;
    opInfo bestData;
    int loops=20;
    
    for(int i=0;i<loops;i++){
        cout << "Observing possibility " << i << ", which has fitness ";
        hubsA=getHubs(boundaries, nums, places);
        moreData=multiBALL(hubsA, places, 10);
        cout << moreData.addon.fitness<< ", compared to the best fitness before this, "<< bestFit << "     \r";
        if(moreData.addon.fitness<bestFit){
            bestFit=moreData.addon.fitness;
            best=moreData.finals;
            bestData=moreData;
        }
    }
    cout << "Complete!\n\n";
    double servicing;
    for(int i=0;i<nums;i++){
        cout << "Hub found at: " << best[i].lat << " , " << best[i].lon << " , with a total node length of " << bestData.addon.fitness << ", after "<< bestData.iterations << " iterations\n";
        cout << "This hub is connected to:\n";
        servicing=0;
        for(int j=0;j<bestData.addon.connections.size();j++){
            if(bestData.addon.connections[j]==i){
                cout << placeName[j][0] << "\n";
                servicing+=places[j][2];
            }
        }
        cout << "Servicing " <<servicing<<"\n";
    }
    
    return 0;
}
