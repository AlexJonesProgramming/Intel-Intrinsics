#pragma once 

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstring>
//https://en.wikipedia.org/wiki/Truevision_TGA
//and 2018 ETEC 3702 06-mpi lab
template<typename T>
class Image{
  public:

    virtual ~Image(){}
    
    void* data(){
        return data_.data();
    }
    
    const void* data() const {
        return data_.data();
    }
  
    const T& header() const {
        return hdr;
    }
    
  protected:

    std::ifstream readHeader(std::string filename){
        std::ifstream in(filename,std::ios::binary);
        if( !in.good() )
            throw std::runtime_error("Cannot open file");
        in.read( (char*)&hdr, sizeof(hdr) );
        return in;
    }
    
    void readData(std::ifstream& in, unsigned sizeBytes){
        unsigned sizeXMM = sizeBytes / sizeof(XMM);
        if( sizeBytes % sizeof(XMM) )
            sizeXMM++;
        data_.resize(sizeXMM);
        in.read( (char*)data_.data(), sizeBytes );
    }
    T hdr;
    struct alignas(32) XMM{
        char xmm[32];
    };
    std::vector<XMM> data_;  
};

#pragma pack(push,1)
struct TargaHeader{
    //0...255: Size of ID field
    uint8_t idSize;
    
    //0=truecolor, 1=palette
    uint8_t colorType; 
    
    //1=palette, 2=truecolor, 3=greyscale
    //Add 8 if compressed (so 9, 10, or 11)     
    uint8_t compression;  
    
    //only useful if palette
    uint16_t firstColormapEntryInFile;
    uint16_t numberColormapEntriesInFile;
    uint8_t bitsPerColormapPixel;
    
    //tells if lower left or upper left for image origin
    //lower left: 0,0
    //upper left: 0,height
    uint16_t xOrigin, yOrigin;
    
    uint16_t width, height;
    
    //24=truecolor, 32=alpha
    uint8_t bitsPerPixel;   
    
    //     xxddaaaa: a=alpha channel size, d=direction, x=unused
    //0  = 00000000 = BGR, lower left origin
    //8  = 00001000 = BGRA, lower left origin
    //32 = 00100000 = BGR, upper left origin
    //40 = 00101000 = BGRA, upper left origin
    uint8_t descriptor;     
    
    //size is idSize
    //char comment[];
};
#pragma pack(pop)

#pragma pack(push,1)
struct BitmapHeader {

    //"BM"
    char sig[2];
    
    //size of whole bitmap file
    uint32_t size;           
    
    uint32_t dummy;

    //offset from file start to data. 54 if no extra fields and not RGBA
    uint32_t offsetToData;
    
    //amount of data in rest of header: sizeof(BitmapHeader)-14
    //if no extension data and not RGBA: 40
    uint32_t headerSize;
    
    uint32_t width;
    uint32_t height;
    
    //usually 1
    uint16_t planes;
    
    //24=true color (BGR), 32 if BGRx format
    uint16_t bitsPerPixel;
    
    //0=none, 3=file uses 32 bpp
    uint32_t compression;    
    
    //Number of bytes in image: w*h*bits_per_pixel/8
    uint32_t imgSize;       
    
    //Almost any value OK; no one really uses these
    uint32_t pixelsPerMeterX;
    uint32_t pixelsPerMeterY;

    //for truecolor, these two fields are zero
    uint32_t numColors;         
    uint32_t numImportantColors;
    
    //extra field: Only present if 32bpp file
    uint32_t colorMasks[4];
    
};
#pragma pack(pop)
    
    
class Targa : public Image<TargaHeader>{
  public:
    Targa(std::string filename) {
        auto in = readHeader(filename);
        in.seekg( hdr.idSize, std::ios::cur );
        readData(in,hdr.width*hdr.height*hdr.bitsPerPixel/8);
    }
};

class Bitmap : public Image<BitmapHeader>{
    public:
    Bitmap(std::string filename) {
        auto in = readHeader(filename);
        in.seekg( hdr.offsetToData );
        readData(in,hdr.width*hdr.height*hdr.bitsPerPixel/8);
    }
    
    //creates 24 bit bgr bitmap.
    Bitmap(int w, int h, const void* pixdata){
        if( (w*3) % 4 )
            throw std::runtime_error("We don't handle padding!");
            
        hdr.sig[0] = 'B';
        hdr.sig[1] = 'M';
        hdr.size = sizeof(hdr) - sizeof(hdr.colorMasks) + w*3*h;
        hdr.offsetToData = sizeof(hdr) - sizeof(hdr.colorMasks);
		hdr.headerSize = 40;// sizeof(hdr) - sizeof(hdr.colorMasks);
        hdr.width = w;
        hdr.height = h;
        hdr.planes = 1;
        hdr.bitsPerPixel = 24;
        hdr.compression = 0;   
        hdr.imgSize = w*h*3;
        hdr.pixelsPerMeterX = 720;
        hdr.pixelsPerMeterY = 720;
        hdr.numColors=0;
        hdr.numImportantColors=0;
        unsigned byteSize=w*h*3;
        unsigned xmmSize = byteSize / sizeof(XMM);
        if( byteSize % sizeof(XMM) )
            ++xmmSize;
        data_.resize( xmmSize );
        memcpy(data(), pixdata, w*h*3 );
    }
    
    void write( std::string fname ){
        std::ofstream o(fname,std::ios::binary);
        if(!o.good() )
            throw std::runtime_error("Cannot open output file");
        o.write( (char*) &hdr, sizeof(hdr)-sizeof(hdr.colorMasks) );
        o.write( (char*) data(), hdr.width*hdr.height*3 );
    }
};

