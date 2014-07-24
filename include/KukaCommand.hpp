#ifndef KUKACOMMAND_H
#define KUKACOMMAND_H

#include <types.hpp>
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
            info:   <Mode Tick Id Run Vel Frames>
            frame:  <X Y Z A B C>
        */
        void formatPose( boost::asio::streambuf &message, const info_vec &info , const frame_vec &frame);

        /*
            message: a streambuf to fill
            info:   <Mode Tick Id Run Vel Frames>
            trajectory:  vector of several frames <X Y Z A B C>
        */
        void formatTrajectory(boost::asio::streambuf &message, const info_vec &info , const trajectory_vec &trajectory);

    protected:

    private:

};

#endif // KUKACOMMAND_H

