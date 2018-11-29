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

struct hub{
    double lat;
    double lon;
    double fitness;
    int servicing;
};

struct collection{
    double fitness;
    vector<int> connections;
};

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
double findFitness(double lat, double lon, vector<vector<double>> places){
    double fitness=0;
    for(int i=0;i<places.size();i++){
        fitness+=hDist(lat, places[i][3], lon, places[i][4], places[i][2]);
    }
    return fitness;
}

//Finding the fitness of multiple hubs, all connected to different places and those connections
collection findDualFitnesses(vector<hub> dualHubs, vector<vector<double>> places){
    double bestFitness;
    double fitness=0;
    vector<int> connections;
    double best=0;
    
    for(int i=0;i<places.size();i++){
        bestFitness=INFINITY;
        for(int j=0;j<dualHubs.size();j++){
            if(bestFitness<hDist(dualHubs[j].lat, places[i][3], dualHubs[i].lon, places[i][4], places[i][2])){
                bestFitness=hDist(dualHubs[j].lat, places[i][3], dualHubs[i].lon, places[i][4], places[i][2]);
                best=j;
            }
        }
        fitness+=bestFitness;
        connections.push_back(best);
    }
    collection col;
    col.connections=connections;
    col.fitness=fitness;
    return col;
}

int main(int argc, const char * argv[]) {
    //
    string fileName="GBplaces.csv";
    
    ifstream file(fileName);
    
    string line;
    string number;
    //Places have only numbers in [0, 0, pop, lat, long]
    vector< vector <double> > places;
    vector<double> data;
    
    
    random_device random;
    //Using Mersenne Twister engine (64 bit unsigned)
    mt19937_64 generate(random());
    
    vector<hub> hubs;
    
    int start, end, x;
    
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
                for(int i = 0; end != -1; i++){
                    //Finding next comma
                    end = (int)line.find(",", i);
                    //Checking validity
                    if(end - start > 0){
                        //Getting number and adding to data
                        number = line.substr(start, end - start);
                        data.push_back(atof(number.c_str()));
                        //cout << atof(number.c_str()) << ", ";
                        
                    }else if(end == -1){
                        //Hit the end of the line
                        end = (int)line.length();
                        //Adding to data
                        number = line.substr(start, end-start);
                        data.push_back(atof(number.c_str()));
                        //cout << atof(number.c_str());
                        //Setting end to -1 to end loop
                        end = -1;
                    }
                    //Resetting start
                    start = end + 1;
                }
                //Adding data to places and resetting data
                places.push_back(data);
                data.clear();
                //cout << "\n";
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
    
    
    //Finding boundaries for lat/long
    double maxLat=-99;
    double minLat=99;
    double maxLong=-99;
    double minLong=99;
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
    
    cout << "Min lat: " << minLat << "; Max lat: " << maxLat << "; Min long: " << minLong << "; Max long: " << maxLong << "\n";
    
    //Random hubs
    uniform_real_distribution<> distLat(minLat,maxLat);
    uniform_real_distribution<> distLong(minLong,maxLong);
    
    
    int nums = 10;
    hub alpha;
    //Setting random hubs
    for(int i = 0; i < nums; i++){
        alpha.lat=distLat(generate);
        alpha.lon=distLong(generate);
        alpha.fitness=findFitness(alpha.lat, alpha.lon, places);
        alpha.servicing=0;
        for(int j=0;j<places.size();j++){
            alpha.servicing+=places[j][2];
        }
        hubs.push_back(alpha);
    }
    
    //Relocating hubs
    int bestHub=0;
    double bestFitness=INFINITY;
    double search=0.001;
    double currentFit, testFit;
    bool changing=true;
    int iterations=0;
    
    //Finding minimum of hubs
    while(changing){
        iterations++;
        changing=false;
        //Getting best hubs
        for(int i=0;i<hubs.size();i++){
            if(bestFitness>hubs[i].fitness){
                bestHub=i;
                bestFitness=hubs[i].fitness;
            }
        }
        //Looking at nearby areas
        for(int i=-1;i<=1;i++){
            for(int j=-1;j<=1;j++){
                if(i!=0 || j!=0){
                    for(int k=0;k<hubs.size();k++){
                        currentFit=hubs[k].fitness;
                        testFit=findFitness(hubs[k].lat+i*search, hubs[k].lon+j*search, places);
                        if(testFit<currentFit){
                            changing=true;
                            hubs[k].lat+=i*search;
                            hubs[k].lon+=j*search;
                            hubs[k].fitness=testFit;
                        }
                    }
                }
            }
        }
    }
    //Saying what the best single hub is=
    cout << "Best single hub found at: " << hubs[bestHub].lat << " , " << hubs[bestHub].lon << " , with a total node length of " << hubs[bestHub].fitness << ", after "<< iterations << " iterations\n";
    
    
    return 0;
}
