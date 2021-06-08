#include <network.hpp>
#include <FileChecker.h>
#include <boost/lexical_cast.hpp>

int main(int argc, char* argv[])
{
    std::string catchmentDataFile;
    int num_partitions = 0;

    if( argc < 3 ){
        std::cout << "Missing required args:" << std::endl;
        std::cout << argv[0] << " <catchment_data_path> <number of partitions>" << std::endl;
    }
    else {
        bool error = false;
        if( !utils::FileChecker::file_is_readable(argv[1]) ) {
            std::cout<<"catchment data path "<<argv[1]<<" not readable"<<std::endl;
            error = true;
        }
        else{ catchmentDataFile = argv[1]; }
    
        try {
            num_partitions = boost::lexical_cast<int>(argv[2]);
            if(num_partitions < 0) throw boost::bad_lexical_cast();
        }
        catch(boost::bad_lexical_cast &e) {
            std::cout<<"number of partitions must be a postive integer."<<std::endl;
            error = true;
        }

        if(error) exit(-1);
    }

    //Get the feature collecion for the given hydrofabric
    geojson::GeoJSON catchment_collection = geojson::read(catchmentDataFile);
    std::string link_key = "toid";
  
    network::Network network(catchment_collection, &link_key);
    //Assumes dendridic, can add check in network if needed.
    int partition = 0;
    int counter = 0;
    int total = network.size()/2; //Note network.size is the number of catchments + nexuses.  This should be a rough count.
    int partition_size = total/num_partitions;
    std::vector<std::string> catchment_list, nexus_list;

    std::cout<<"in partition 0:"<<std::endl;
    for(const auto& catchment : network.filter("cat")){
            std::string nexus = network.get_destination_ids(catchment)[0];
            std::cout<<catchment<<" -> "<<nexus<<std::endl;
            //keep track of all the features in this partition
            catchment_list.push_back(catchment);
            nexus_list.push_back(nexus);
            counter++;
            if(counter == partition_size)
            {
                std::cout<<"nexus "<<nexus<<" is remote DOWN on partition "<<partition<<std::endl;
                partition++;
                counter = 0;
                std::cout<<"nexus "<<nexus<<" is remote UP on partition "<<partition<<std::endl;
                //Do stuff with partition lists
                //
                //
                //reset lists for next partition
                catchment_list.clear();
                nexus_list.clear();
                //this nexus overlaps partitions
                nexus_list.push_back(nexus);
                std::cout<<"in partition "<<partition<<":"<<std::endl;
            }
    }
    
    return 0;
}

