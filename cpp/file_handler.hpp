#ifndef FILE_HANDLER_HPP
#define FILE_HANDLER_HPP


#include <fstream>
#include <cstdint>
#include <string>
#include <cmath>




namespace file_handler{




template<uint64_t block_size=1024, uint32_t cache_size=32>
class IFile{

    struct Block{
        uint8_t data[block_size];
        uint32_t age;
        uint64_t in_file_idx;
    };

    uint64_t file_size;
    uint64_t current_block_idx;
    std::ifstream file;
    Block blocks[cache_size];


    void load_block(uint64_t i){
        if( i == blocks[current_block_idx].in_file_idx ) return;

        uint64_t file_size_in_block = file_size/block_size + (file_size%block_size!=0);
        if( i >= file_size_in_block ) throw "IFile::load_block i out of range";

        bool already_loaded = false;
        uint64_t idx = 0;
        for( uint64_t j=0;  j<cache_size;  j++ ){
            if( blocks[j].in_file_idx == i ){
                already_loaded = true;
                idx = j;
                break;
            }
        }


        if( already_loaded ){ // ha a cache-ben van, akkor öregítés
            
            if( blocks[idx].age ){
                for( uint64_t j=0; j<cache_size; j++ ){
                    if( blocks[j].age < blocks[idx].age ){
                        blocks[j].age += 1;
                    }
                }
                blocks[idx].age = 0;
                current_block_idx = idx;
            }


        }else{ // ha nincs a cache-ben, akkor a legöregebb helyére betöltjük
            
            uint64_t oldest_idx = 0;
            for( uint64_t j=0; j<cache_size; j++ ){
                if( blocks[j].age > blocks[oldest_idx].age ) oldest_idx = j;
                blocks[j].age += 1;
            }

            blocks[oldest_idx].age = 0;
            blocks[oldest_idx].in_file_idx = i;

            file.seekg( i*block_size, std::ios_base::beg );

            uint64_t _size = file_size-(i*block_size);
            if( _size > block_size ) _size = block_size;

            file.read((char*)&blocks[oldest_idx].data, _size);

            current_block_idx = oldest_idx;

        }
    }

    uint8_t get_byte_by_idx( uint64_t idx ){
        if( idx >= file_size )
            throw "IFile::get_by_by_idx idx out of range";

        uint64_t block_idx = idx / block_size;
        uint64_t in_block_idx = idx % block_size;

        load_block( block_idx );

        return blocks[current_block_idx].data[in_block_idx];

    }

public:

    IFile() = delete;
    IFile( const std::string& path ) {
        file = std::ifstream{ path, std::ios::binary | std::ios::in };
        if( ! file.is_open() )
            throw "IFile(path) path cannot be opened";

        file.seekg(0, std::ios_base::end);
        file_size = file.tellg();
        file.seekg(0, std::ios_base::beg);

        for( uint32_t i=0; i<cache_size; i++ ){
            blocks[i].in_file_idx = (uint64_t)-1;
            blocks[i].age = (uint32_t)-1;
        }
        current_block_idx = (uint64_t)-1;
    }


    void load_range( void* _buffer, uint64_t start, uint64_t size ){

        if( start >= file_size || start+size > file_size )
            throw "IFile::load_range range out of range";

        uint8_t* buffer = (uint8_t*)_buffer;
        uint64_t buffer_idx = 0;

        for( uint64_t i=0;  i<size; i++ ){
            buffer[i] = get_byte_by_idx(start + i);
        }

    }

    inline uint64_t size() const { return file_size; }

};



}






#endif