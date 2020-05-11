#include <mpi.h>
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <algorithm>
#include <sys/time.h>

using namespace std;

enum tag {T_ONE=1,T_TWO=2,T_THREE};

void JoinTwoSortedArrays(unsigned char * first,unsigned  char * second,int count,unsigned char * result)
{
    int i = 0; // index to first array
    int j = 0; // index to second array
    int k = 0; // index to result array
    
    while (i < count && j < count) 
    { 
        if (first[i] < second[j]) 
            result[k++] = first[i++]; 
        else
            result[k++] = second[j++]; 
    } 
  
    // Store remaining elements of first array 
    while (i < count) 
        result[k++] = first[i++]; 
  
    // Store remaining elements of second array 
    while (j < count) 
        result[k++] = second[j++];
}

int main( int argc, char* argv[] ){
    char input_file_name[]= "numbers";
    int numprocs = 1;
    int input_size = 1;
    int input_desired_size=1;
    int leaf_numprocs=1;
    if(argc == 5)
    {
        input_size          = atol(argv[1]);
        input_desired_size  = atol(argv[2]);
        numprocs            = atol(argv[3]);
        leaf_numprocs       = atol(argv[4]);
    }
    
	int proceso_id;
	

	//parallel part
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &proceso_id);
	MPI_Status status;
	
	if(proceso_id == 0)// MAIN PROCESS
	{
		//reading input
		std::ifstream input(input_file_name, std::ios::binary);
        std::vector<unsigned char> array;

		while (true) {
            unsigned char x;
            input >> x;
            if( input.eof() )
                break;
            array.push_back(x);
        }
        
		input.close();
        //adding max_byte to the end of array
        for(int i=0; i < input_desired_size - input_size; ++i)
            array.push_back((unsigned char)-1);
        
        if(array.size() == 1)
        {
            std::cout << (int)array[0] << std::endl;
            std::cout << (int)array[0] << std::endl;
            MPI::Finalize();
            return 0;
        }
        else if(array.size() == 2)
        {
            int min = (int)(array[0] < array[1])?array[0]:array[1];
            int max = (int)(array[0] > array[1])?array[0]:array[1];
            std::cout << (int)array[0] << " " << (int)array[1] << std::endl;
            std::cout << min << std::endl;
            std::cout << max << std::endl;
            MPI::Finalize();
            return 0;
        }
        
        //get results
        unsigned char * result_left = new unsigned char [input_desired_size / 2];
        unsigned char * result_right = new unsigned char [input_desired_size / 2];
        unsigned char * result = new unsigned char [input_desired_size];
		
		//---### start ###---
		double starttime = MPI_Wtime();
		
		//distributing input to leafs		
        int i=0; // item index
        int j=-1; // leaf number
		for(std::vector<unsigned char>::iterator it = array.begin();
			it != array.end(); ++it)
		{
            unsigned char item = (*it);
            if(i % (input_desired_size / leaf_numprocs) == 0)
                ++j;
            
            int destination = numprocs - (leaf_numprocs - j);
            MPI_Send(&item, 1, MPI_UNSIGNED_CHAR, destination, T_ONE, MPI_COMM_WORLD);
            
			++i;
		}
        
        for(int i = 0; i < input_desired_size / 2; i++){
            unsigned char receivedElement; 
            MPI_Recv(&receivedElement, 1, MPI_UNSIGNED_CHAR, 1, T_THREE, MPI_COMM_WORLD, &status);
            result_left[i] = receivedElement;
            MPI_Recv(&receivedElement, 1, MPI_UNSIGNED_CHAR, 2, T_THREE, MPI_COMM_WORLD, &status);
            result_right[i] = receivedElement;
        }
        
        JoinTwoSortedArrays(result_left,result_right,input_desired_size / 2,result);
		
		//---### end ###---
		double endtime = MPI_Wtime();

//--------------------DEBUG----------------------
//printf("DURATION\t\t\t %f seconds \n", endtime-starttime);
//printf("N: %d \n", input_size);
//printf("P:  %d \n", numprocs);
//--------------------DEBUG----------------------

        //print results
        bool first = true;
        int index = 0;
        for(std::vector<unsigned char>::iterator it = array.begin();
			it != array.end(); ++it)
        {
            if(!first)
                std::cout << " ";
            else
                first = false;
            
            std::cout << (int)(*it);
            
            
            ++index;
            if(index == input_size)
                break;
        }
        std::cout << std::endl;
        
        for(int i=0; i < input_size; ++i)
            std::cout << (int)result[i] << std::endl;
        
        delete result;
        delete result_left;
        delete result_right;
	}
	else // CHILD PROCESS
	{
        if(proceso_id >= numprocs - leaf_numprocs){ //LEAF
            //local data
            int bucket_size =  input_desired_size / leaf_numprocs;
            unsigned char *local = new unsigned char [bucket_size];
            //get them
            for(int i = 0; i < bucket_size; i++){
                unsigned char receivedElement; 
                MPI_Recv(&receivedElement, 1, MPI_UNSIGNED_CHAR, 0, T_ONE, MPI_COMM_WORLD, &status);
                local[i] =  receivedElement;
            }
            
            //sort them
            std::sort(local, local+(bucket_size));
            
            int destination = (proceso_id + 1)/2 - 1;
            //pass up input size
            MPI_Send(&bucket_size, 1, MPI_INT, destination, T_TWO, MPI_COMM_WORLD);
            
            //pass them up
            for(int i = 0; i < bucket_size; i++)
            {
                MPI_Send(&local[i], 1, MPI_UNSIGNED_CHAR, destination, T_THREE, MPI_COMM_WORLD);
            }
            delete local;
        }
        else{ //TREE NODE
            //get data size
            int size;
            int node_left = proceso_id * 2 + 1 ;
            int node_right = node_left + 1;
                       
            MPI_Recv(&size, 1, MPI_INT, node_left, T_TWO, MPI_COMM_WORLD, &status);
            //local data
            unsigned char *local1 = new unsigned char [size];
            unsigned char *local2 = new unsigned char [size];
            unsigned char *local = new unsigned char [size*2];
            //get them
            unsigned char receivedElement;
            
            for(int i = 0; i < size; i++){
                MPI_Recv(&receivedElement, 1, MPI_UNSIGNED_CHAR, node_left, T_THREE, MPI_COMM_WORLD, &status);
                local1[i] =  receivedElement;
            }
            for(int i = 0; i < size; i++){
                MPI_Recv(&receivedElement, 1, MPI_UNSIGNED_CHAR, node_right, T_THREE, MPI_COMM_WORLD, &status);               
                local2[i] =  receivedElement;
            }
            
            JoinTwoSortedArrays(local1,local2,size,local);
            
            //pass result
            int future_size = size * 2;
            int destination = (proceso_id + 1)/2 - 1;
            
            MPI_Send(&future_size, 1, MPI_INT, destination, T_TWO, MPI_COMM_WORLD);
            for(int i = 0; i < size * 2; ++i)
            {
                MPI_Send(&local[i], 1, MPI_UNSIGNED_CHAR, destination, T_THREE, MPI_COMM_WORLD);            
            }
            
            delete local1;
            delete local2;
            delete local;
        }	
	}	
	MPI::Finalize();
	return 0;
}