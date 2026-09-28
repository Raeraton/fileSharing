#include "file_handler.hpp"
#include "byteorder_stuffs.hpp"

#include <cmath>
#include <mutex>
#include <iostream>
#include <string>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


inline bool buffeqlen4str( const uint8_t* b, const char* s ){
    for(int i=0;  i<4;  i++){
        if( s[i] != b[i] ) return false;
    }
    return true;
}



template< uint64_t _bs, uint64_t _cs >
class Server{

    enum Request_type{
        SET_PACKAGE_SIZE,
        GET_FILE_SIZE,
        GET_PACKAGE_BY_IDX,
        GET_PACKAGE_BY_IDXS,
        GET_RANGE,
        FINISH_EXIT_CLOSE,
        UNKNOWN_REQUEST,
    };

    int sock = 0;

    uint16_t packet_size=0;

    std::mutex file_mutex;
    file_handler::IFile<_bs, _cs>& file_ref;
    inline void read_data_from_file( void* buffer, uint64_t start, uint64_t size ){
        file_mutex.lock();
        try{
            file_ref.load_range(buffer, start, size);
        }catch(const char* err){
            file_mutex.unlock();
            throw err;
        }catch(std::exception err){
            file_mutex.unlock();
            throw err;
        }catch(...){
            file_mutex.unlock();
            throw "Server::read_data_from_file unknown error with reading from file";
        }
        file_mutex.unlock();

    }
    inline uint64_t get_file_size(){
        file_mutex.lock();
        uint64_t out = file_ref.size();
        file_mutex.unlock();
        return out;
    }

    /// @brief dont write the req to the start
    /// @param packet_id 
    /// @param buffer 
    /// @param size 
    void get_response_block( uint32_t packet_id, uint8_t* buffer, uint64_t* size ){

        if( *size < 8 + packet_size )
            throw "Server::get_response_block buffer too small";

        *((uint32_t*)(buffer+4)) = to_big_endian(packet_id);

        uint8_t* data_buffer = buffer + 8;

        auto file_size = get_file_size();
        auto start = packet_id * packet_size;

        uint64_t data_size;
        if( start+packet_size > file_size ){
            data_size = file_size-start;
        }else{
            data_size = packet_size;
        }

        read_data_from_file(
            buffer+8,
            packet_id*packet_size,
            data_size
        );

        *size = data_size + 8;

    }

    Request_type get_request_type( uint8_t* buffer ){
        if( buffeqlen4str(buffer, "GPBI") )
            return GET_PACKAGE_BY_IDX;
        
        if( buffeqlen4str(buffer, "GBIS") )
            return GET_PACKAGE_BY_IDXS;
        
        if( buffeqlen4str(buffer, "GRNG") )
            return GET_RANGE;

        if( buffeqlen4str(buffer, "PKGS") )
            return SET_PACKAGE_SIZE;

        if( buffeqlen4str( buffer, "GFS_" ) )
            return GET_FILE_SIZE;
        
        if( buffeqlen4str( buffer, "FINH" ) )
            return FINISH_EXIT_CLOSE;

        return UNKNOWN_REQUEST;
    }

public:

    /// @brief 
    /// @param _file_ref 
    /// @param port - port in int
    Server( file_handler::IFile<_bs, _cs>& _file_ref, int port ) :
            file_ref{_file_ref} {

        sock = socket( AF_INET, SOCK_DGRAM, 0 );
        if( sock < 0 )
            throw "Server() creating socket";
        
        sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if( bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0 )
            throw "Server() binding";
    }

    ~Server(){
        if(sock){
            close(sock);
        }
    }

    void serve_one(){


        uint8_t req_buffer[4096];

        bool running = true;
        while( running ){

            sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);

            ssize_t recved = recvfrom(
                sock,
                req_buffer,
                4096,
                0,
                (sockaddr*)&addr,
                &addr_len
            );

            if( recved < 0 ){
                std::cerr << "request error: " << req_buffer << "\n";
                continue;
            }

            auto request_type = get_request_type( req_buffer );

            switch (request_type)
            {
            case GET_PACKAGE_BY_IDX:{
                uint32_t packet_id = from_big_endian( *((uint32_t*)(req_buffer+8)) );
                uint64_t resp_block_size = 4096-4;                

                get_response_block(packet_id, req_buffer+4, &resp_block_size);

                sendto(
                    sock,
                    req_buffer + 4,
                    resp_block_size,
                    0,
                    (sockaddr*)&addr,
                    addr_len
                );
            }break;
            case GET_PACKAGE_BY_IDXS:{
                // TODO
            }break;
            case GET_RANGE:{
                // TODO
            }break;
            case GET_FILE_SIZE:{
                *((uint64_t*)(req_buffer+8)) = to_big_endian(get_file_size());
                sendto(
                    sock,
                    req_buffer+4,
                    12,
                    0,
                    (sockaddr*)&addr,
                    addr_len
                );
            }break;
            case SET_PACKAGE_SIZE:{
                uint16_t _packet_size = *((uint16_t*)(req_buffer+8));
                _packet_size = from_big_endian(_packet_size);
                if( _packet_size >= 64 && packet_size == 0 ){
                    packet_size = _packet_size;
                    continue;
                }
                *((uint16_t*)(req_buffer+6)) = to_big_endian(packet_size);
                sendto(
                    sock,
                    req_buffer + 4,
                    6,
                    0,
                    (sockaddr*)&addr,
                    addr_len
                );
            }break;
            case FINISH_EXIT_CLOSE:{
                running = false;
            }
            default:
                break;
            }



        }


        // closing client
        packet_size = 0;
    }

};







int main(){


    file_handler::IFile<64, 8> file{"in"};

    std::cout << file.size() << '\n';

    Server<64, 8> server{file, 5000};

    server.serve_one();


}
