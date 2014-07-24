#include "DataFile.hpp"

DataFile::DataFile() {}

DataFile::~DataFile() {}

void DataFile::loadSpaceDelimited(const string& filename, trajectory_vec &trajectory, size_t framesize)
{

    string line;
    ifstream file(filename);

    boost::char_separator<char> spaceSeparator(" ");

    if (file.is_open())
    {

        while ( getline(file,line) )
        {

            // TODO: filter out empty lines and incorrect lines

            boost::tokenizer<boost::char_separator<char>> tokens(line, spaceSeparator);

            frame_vec frame;

            try
            {

                for (const auto &token : tokens)    // token content should be a double
                {

                    frame.push_back(boost::lexical_cast<double>(token));
                }
            }
            catch (std::exception& e)
            {
                cerr << "Cast to double in datafile failed, file not read: " << e.what();
                return;
            }

            if (frame.size() == framesize)  // TODO: rewrite this hack
            {
                trajectory.push_back(frame);    // TODO: is this efficient copywise?
            }
        }
        file.close();
    }

    else cerr << "Unable to open space delimited data file " << filename << endl;

}

double DataFile::roundToPrecision(double val)
{
    return floorf(val * roundingConstant + 0.5) / roundingConstant;
}

void DataFile::setPrecision(int p)
{
    precision = p;
    roundingConstant = pow(10, precision);
}
