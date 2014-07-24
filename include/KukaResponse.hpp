#ifndef KUKARESPONSE_H
#define KUKARESPONSE_H

#include <vector>

#include <XMLParser.hpp>

using boost::lexical_cast;
using boost::bad_lexical_cast;

class KukaResponse : public XMLParser
{
    public:

        // ---------------------------------------------------------------------------
        // Constructor / Destructor
        // ---------------------------------------------------------------------------

        KukaResponse();

        virtual ~KukaResponse();

        // ---------------------------------------------------------------------------
        // Public Methods
        // ---------------------------------------------------------------------------

        // parses and stores internally
        void parse(boost::asio::streambuf &message);

        /*
            Status Error Mode Tick Id
        */
        std::vector<int> getInfo();

        /*
            X Y Z A B C
        */
        std::vector<double> getFrame();

        void printValues();

    protected:

    private:

        // ---------------------------------------------------------------------------
        // Private Data
        // ---------------------------------------------------------------------------

        std::vector<int> info;
        std::vector<double> frame;

};

#endif // KUKARESPONSE_H


