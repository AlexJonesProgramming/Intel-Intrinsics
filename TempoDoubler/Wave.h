#pragma once
#include <fstream>
#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>
#include <iostream>

//http://wavefilegem.com/how_wave_files_work.html
 
//Each sample: One of unsigned 8 bit,
//signed 16/24/32 bit, or float 32 bit
//float has values from -1...1 
    
class Wave{
  public:
  
    enum FormatCode {
        PCM=1, ADPCM=2, FLOAT=3, MULAW=7, EXTENSIBLE=65534
    };
   
    #pragma pack(push,1)
    struct Format{
        uint16_t format;            //see FormatCode enum
        uint16_t numChannels;       //1=mono, 2=stereo
        uint32_t samplesPerSecond;  //ex: 44100 for CD-quality sound
        uint32_t bytesPerSecond;    //bytesPerFrame * samplesPerSecond
        uint16_t bytesPerFrame;     //ceil(numChannels * bitsPerSample/8)
        uint16_t bitsPerSample;     //usually 8, 16, 24, or 32
    };
    #pragma pack(pop)
    
    Format format;
    std::uint32_t numFrames; //calculated, not in the file
    
    /** Read a WAVE file from a file
        @param filename The name of the file to read
    */
    Wave(std::string filename){
        std::ifstream in;
        in.open(filename.c_str(),std::ios::binary);
        if(!in.good() )
            throw std::runtime_error("Cannot open file "+filename);
            
        char chunkID[4];
        in.read(chunkID,4);
        if( std::memcmp(chunkID,"RIFF",4) )
            throw std::runtime_error("Not a WAV file");
        in.seekg(4,std::ios::cur);       //skip chunk size
        
        char formatCode[4];
        in.read(formatCode,4);
        if( std::memcmp(formatCode,"WAVE",4) )
            throw std::runtime_error("Not a WAV code");

        while(true){
            in.read(chunkID,4);
            if( in.fail() )
                break;
            if( 0 == std::memcmp(chunkID, "fmt ",4) )
                readFmtChunk(in);
            else if( 0 == std::memcmp(chunkID,"data",4) )
                readDataChunk(in);
            else 
                readUnknownChunk(in);
            if( in.fail() )
                throw std::runtime_error("Bad wave file: Failed early");
        }
    }
       
    /**Allocate a new wave file with empty data
        @param fmt The data format
        @param numFrames The number of frames to allocate
    */
    Wave( Format fmt, uint32_t numFrames ){
        this->format = fmt;
        this->numFrames = numFrames;
        unsigned sizeInBytes =  fmt.bytesPerFrame * numFrames;
        unsigned sizeInQuanta = sizeInBytes/sizeof(Quantum);
        if( sizeInBytes % sizeof(Quantum) )
            sizeInQuanta++;
        datav.resize( sizeInQuanta );
    }
    
    /**Save this wave file
        @param filename the filename to use
    */
    void write(std::string filename){
        std::ofstream out(filename,std::ios::binary);
        out.write("RIFF",4);
        size_t sizeLoc = out.tellp();
        out.write("XXXX",4);    //placeholder for size
        out.write("WAVE",4);
        out.write("fmt ",4);
        uint32_t chunkSize = sizeof(this->format);
        if( format.format != 1 )
            chunkSize += 2;
        out.write((char*)&chunkSize,sizeof(chunkSize));
        out.write((char*)&this->format, sizeof(this->format));
        if( format.format != 1 ){
            uint16_t extensionSize=0;
            out.write((char*)&extensionSize,sizeof(extensionSize));
        }
        out.write("data",4);
        chunkSize = numFrames*format.bytesPerFrame;
        out.write( (char*) &chunkSize, sizeof(chunkSize) );
        out.write((char*)data(),chunkSize);
        if( chunkSize % 2 )
            out.write( (char*) &chunkSize,1); //padding, content is irrelevant
            
        uint32_t size = uint32_t(out.tellp());
        size -= 8;
        out.seekp(sizeLoc);
        out.write((char*)&size,sizeof(size));
    }
    
    void* data(){
        return (void*)datav.data();
    }


    //////////////////////////////////////////////////////
    //Nothing to see here; move along...
  private:
    struct alignas(32) Quantum{
        char data[32];
    };
    std::vector<Quantum> datav;
    
    void readUnknownChunk(std::ifstream& in){
        std::uint32_t chunkSize;
        in.read((char*)&chunkSize,4);
        in.seekg( chunkSize, std::ios::cur );
        if( in.fail() )
            throw std::runtime_error("Fail on read unknown chunk");
        if( chunkSize % 2 )
            in.seekg( 1, std::ios::cur );
    }
  
    void readDataChunk(std::ifstream& in){
        std::uint32_t chunkSize;
        in.read((char*)&chunkSize,4);
        unsigned sz = chunkSize / sizeof(Quantum);
        if( chunkSize % sizeof(Quantum) )
            ++sz;
        datav.resize(sz);
        in.read( (char*) data(), chunkSize);
        numFrames = chunkSize / format.bytesPerFrame;
        if( in.fail() )
            throw std::runtime_error("Fail on read data chunk");
        if( chunkSize % 2 )
            in.seekg( 1, std::ios::cur );
    }

    void readFmtChunk(std::ifstream& in){
        std::uint32_t chunkSize;
        in.read((char*)&chunkSize, 4);
        if( chunkSize < sizeof(format) )
            throw std::runtime_error("Corrupt format");
        in.read((char*)&format, sizeof(format));
        if( in.fail() )
            throw std::runtime_error("Fail on read fmt chunk");
        unsigned rest = chunkSize - sizeof(format);
        in.seekg( rest, std::ios::cur );
        if( chunkSize % 2 )
            in.seekg( 1, std::ios::cur  );
            
   }
};

inline
std::ostream& operator<<(std::ostream& o, const Wave& w){
    const Wave* p = (const Wave*)(&w);
    o << "{wave format=" << 
        p->format.format << " chan=" <<
        p->format.numChannels << " samp/sec=" << 
        p->format.samplesPerSecond << " bytes/sec=" << 
        p->format.bytesPerSecond << " bytes/frame=" << 
        p->format.bytesPerFrame << " bits/samp=" << 
        p->format.bitsPerSample << " numframe=" << 
        p->numFrames << "}";
    return o;
}
