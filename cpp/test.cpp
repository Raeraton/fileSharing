/*

#include <iostream>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(sock);
        return 1;
    }

    std::cout << "Listening on UDP port 5000\n";






    while( 1 ){

        char buffer[1024] = {0};
        sockaddr_in target{};
        socklen_t target_len = sizeof(target);

        ssize_t recved = recvfrom(
            sock,
            buffer,
            1024,
            0,
            (sockaddr*)&target,
            &target_len
        );

        if( recved < 0 ){
            perror("recvfrom");
        }else{
            std::cout << "recved " << recved << " bytes: " << buffer << "\n";

            sendto( 
                sock,
                buffer,
                recved,
                0,
                (sockaddr*)&target,
                target_len
            );

        }

    }






    close(sock);
}


*/


#include "file_handler.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <vector>

#define min(x,y) (x<y?x:y)


long double create_file( size_t size ){

    std::ofstream file{"test.bin", std::ios_base::binary};
    if( ! file.is_open() )
        throw "create file";

    long double sum = 0;
    for( size_t i=0;  i<size;  i++ ){
        long double num = (long double)rand() / RAND_MAX - 0.5;
        sum += num;
        file.write( (char*)&num, sizeof(long double) );

    }
    
    return sum;

}

template<uint64_t bs, uint64_t cs>
long double load_file(){

    file_handler::IFile<bs, cs> file{ "test.bin" };

    long double sum = 0;

    for( size_t i=0;  i<file.size()/sizeof(long double);  i++ ){
        long double num;
        file.load_range( &num, i*sizeof(long double), sizeof(long double) );

        sum += num;


    }

    return sum;

}



int main(int argc, char** argv){

    srand(time(NULL));

    try{
        
        size_t size = 1000;
        if( argc >= 2 ){
            size = std::atoll(argv[1]);
        }

        auto rsum = create_file(size);
        auto tp = std::chrono::high_resolution_clock::now();
        auto fsum = load_file<64, 32>();
        auto dur = std::chrono::duration<long double>(std::chrono::high_resolution_clock::now()-tp).count();


        std::cout << "the real sum is: " << rsum << "\n";
        std::cout << "the sum read from file is: " << fsum << "\n";
        std::cout << "it took " << dur << " s\n";
    
    }catch( const char* err ){

        std::cerr << "error: " << err << "\n";

    }
}