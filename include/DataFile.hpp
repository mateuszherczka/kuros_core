#ifndef DATAFILE_H
#define DATAFILE_H

#include <iostream>
#include <string>
#include <fstream>
#include <math.h>

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include <types.hpp>

using std::string;
using std::ifstream;

/*
Helper class for loading space-delimited files generated in matlab etc.
*/
class DataFile
{
    public:
        DataFile();
        virtual ~DataFile();

        void loadSpaceDelimited(const string& filename, trajectory_vec &trajectory, size_t framesize = 6);

        double roundToPrecision(double val);

        void setPrecision(int p);

    protected:
    private:

        int precision = 8;
        double roundingConstant = pow(10, precision);


};


#endif // DATAFILE_H
