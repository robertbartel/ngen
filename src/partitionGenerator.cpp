#include <network.hpp>
#include <FileChecker.h>
#include <boost/lexical_cast.hpp>
#include<string>

std::vector<std::string> list_item;
void write_part( int id,
                 std::unordered_map<std::string, std::vector<std::string>>& catchment_parts,
                 std::unordered_map<std::string, std::vector<std::string>>& nexus_parts,
                 std::vector<std::pair<std::string, std::string>>& remote_up,
                 std::vector<std::pair<std::string, std::string>>& remote_down, int num_part)
{
    // Write out catchment list
    std::cout<<"        {\"id\":"<<id<<", \"cat-ids\":[";
    for(auto const cat_id : catchment_parts)
        list_item = cat_id.second;
        for (std::vector<std::string>::const_iterator i = list_item.begin(); i != list_item.end(); ++i)   
            {
                if (i != (list_item.end()-1))
                    std::cout<< *i << ", ";
                else
                    std::cout<< *i;
            }
    std::cout<<"], ";

    // Write out nexus list
    std::cout<<"\"nex-ids\":[";
    for(auto const cat_id : nexus_parts)
        list_item = cat_id.second;
        for (std::vector<std::string>::const_iterator i = list_item.begin(); i != list_item.end(); ++i)
            {
                if (i != (list_item.end()-1))
                    std::cout<< *i << ", ";
                else
                    std::cout<< *i;
            }
    std::cout<<"], ";

    // Write out remote up 
    std::pair<std::string, std::string> remote_up_list;
    std::cout<<"\"remote-up\":[";
    for (std::vector<std::pair<std::string, std::string> >::const_iterator i = remote_up.begin(); i != remote_up.end(); ++i)
    {
        if (id != 0)
        {
            remote_up_list = *i;
            std::cout << "{" << "\"" << remote_up_list.first << "\"" << ":" << remote_up_list.second << ", ";
            ++i;
            remote_up_list = *i;
            std::cout << "\"" << remote_up_list.first << "\"" << ":" << "\"" << remote_up_list.second << "\"" << "}";
        }
    }
    std::cout<<"], ";

    // Write out remote down
    std::pair<std::string, std::string> remote_down_list;
    std::cout<<"\"remote-down\":[";
    for (std::vector<std::pair<std::string, std::string> >::const_iterator i = remote_down.begin(); i != remote_down.end(); ++i)
    {
        if (id != (num_part-1))
        {
            remote_down_list = *i;
            std::cout << "{" << "\"" << remote_down_list.first << "\"" << ":" << remote_down_list.second << ", ";
            ++i;
            remote_down_list = *i;
            std::cout << "\"" << remote_down_list.first << "\"" << ":" << "\"" << remote_down_list.second << "\"" << "}";
        }
    }
    std::cout<<"]";
    if (id != (num_part-1))
       std::cout<<"},";
    else
        std::cout<<"}";
}


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
    int partition_size = total/num_partitions+1;
    //std::cout << "num_partition:" << num_partitions << std::endl;
    //std::cout << "partition_size:" << partition_size << std::endl;
    std::vector<std::string> catchment_list, nexus_list;

    std::string id, partition_str, empty_up, empty_down;
    std::vector<std::string> empty_vec;
    std::unordered_map<std::string, int> this_part_id;
    std::unordered_map<std::string, std::vector<std::string> > this_catchment_part, this_nexus_part;
    std::vector<std::unordered_map<std::string, int> > part_ids;
    std::vector<std::unordered_map<std::string, std::vector<std::string> > > catchment_part, nexus_part;
    //std::unordered_map<std::string, std::string> remote_up_part, remote_down_part;
    std::vector<std::unordered_map<std::string, std::string> > remote_up_vec, remote_down_vec;

    std::pair<std::string, std::string> remote_up_id, remote_down_id, remote_up_part, remote_down_part;
    std::vector<std::pair<std::string, std::string> > remote_up, remote_down;

    std::cout<<"{"<<std::endl;
    std::cout<<"    \"partitions\":["<<std::endl;
    //std::cout<<"in partition 0:"<<std::endl;
    std::string up_nexus;
    std::string down_nexus;
    for(const auto& catchment : network.filter("cat")){
            std::string nexus = network.get_destination_ids(catchment)[0];
            //std::cout<<catchment<<" -> "<<nexus<<std::endl;

            //keep track of all the features in this partition
            catchment_list.push_back(catchment);
            nexus_list.push_back(nexus);
            counter++;
            if(counter == partition_size)
            {
                //std::cout<<"nexus "<<nexus<<" is remote DOWN on partition "<<partition<<std::endl;
                down_nexus = nexus;

                id = std::to_string(partition);
                this_part_id.emplace("id", partition);
                this_catchment_part.emplace("cat-ids", catchment_list);
                this_nexus_part.emplace("nex-ids", nexus_list);
                part_ids.push_back(this_part_id);
                catchment_part.push_back(this_catchment_part);
                nexus_part.push_back(this_nexus_part);

                if (partition == 0)
                {
                    remote_up_id = std::make_pair ("id", "\0");
                    remote_up_part = std::make_pair ("partition", "\0");
                    remote_up.push_back(remote_up_id);
                    remote_up.push_back(remote_up_part);
                }
                else
                {
                    partition_str = std::to_string(partition-1);
                    remote_up_id = std::make_pair ("id", up_nexus);
                    remote_up_part = std::make_pair ("partition", partition_str);
                    remote_up.push_back(remote_up_id);
                    remote_up.push_back(remote_up_part);
                }

                partition_str = std::to_string(partition+1);
                remote_down_id = std::make_pair ("id", down_nexus);
                remote_down_part = std::make_pair ("partition", partition_str);
                remote_down.push_back(remote_down_id);
                remote_down.push_back(remote_down_part);

                write_part(partition, this_catchment_part, this_nexus_part, remote_up, remote_down, num_partitions);
                std::cout << std::endl;

                // Clear unordered_map before next round of emplace
                this_part_id.clear();
                this_catchment_part.clear();
                this_nexus_part.clear();

                // Clear remote_up and remote_down vectors before next round
                remote_up.clear();
                remote_down.clear();

                partition++;
                counter = 0;
                //std::cout<<"\nnexus "<<nexus<<" is remote UP on partition "<<partition<<std::endl;

                catchment_list.clear();
                nexus_list.clear();

                //this nexus overlaps partitions
                nexus_list.push_back(nexus);
                up_nexus = nexus;
                //std::cout<<"\nin partition "<<partition<<":"<<std::endl;
            }
    }
    std::cout<<"    ]"<<std::endl;
    std::cout<<"}"<<std::endl;

    return 0;
}

