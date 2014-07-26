#ifndef KUKACOMMAND_H
#define KUKACOMMAND_H

#include <types.hpp>
#include <constants.hpp>
#include <boost/asio.hpp>

class KukaCommand
{
    public:

        // ---------------------------------------------------------------------------
        // Constructor / Destructor
        // ---------------------------------------------------------------------------

        KukaCommand();

        virtual ~KukaCommand();

        // ---------------------------------------------------------------------------
        // Public methods
        // ---------------------------------------------------------------------------

        /*
            message: a streambuf to fill
            info:   <RMode RMs Id Run Vel FrameType FrameCount>
            trajectory:  vector of several frames <X Y Z A B C>
        */
        void formatTrajectory(boost::asio::streambuf &message, const info_vec &info , const trajectory_vec &trajectory);

    protected:

    private:

};

/*  deprecated
    message: a streambuf to fill
    info:   <RMode RMs Id Run Vel FrameType FrameCount>
    frame:  <X Y Z A B C>
*/
// void formatPose( boost::asio::streambuf &message, const info_vec &info , const frame_vec &frame);

#endif // KUKACOMMAND_H

