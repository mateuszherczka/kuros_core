#include "KukaCommand.hpp"

KukaCommand::KukaCommand() {}

KukaCommand::~KukaCommand() {}

void KukaCommand::formatTrajectory(boost::asio::streambuf &message, const info_vec &info , const trajectory_vec &trajectory)
{

    // TODO: if this is a fact, something is crap further up the line
    //if (info[5] != trajectory.size()) {cerr << "Warning! Size of trajectory vector doesn't match framecount in info vector!" << endl; }

    std::ostream to_message_stream(&message);

    // doc tag
    to_message_stream   << "<ExternalData>\r\n";

    // info part
    to_message_stream   << "<RMode>" << info[0] << "</Mode>\r\n";
    to_message_stream   << "<RMs>" << info[1] << "</RMs>\r\n";
    to_message_stream   << "<Id>" << info[2] << "</Id>\r\n";
    to_message_stream   << "<Run>" << info[3] << "</Run>\r\n";
    to_message_stream   << "<Vel>" << info[4] << "</Vel>\r\n";
    to_message_stream   << "<Tol>" << info[5] << "</Tol>\r\n";
    to_message_stream   << "<FrameType>" << info[6] << "</FrameType>\r\n";
    to_message_stream   << "<FrameCount>" << info[7] << "</FrameCount>\r\n";

    // trajectory
    for (const frame_vec &frame : trajectory)
    {

        // a frame
        to_message_stream   << "<XFrame XPos=\"" << frame[0] << "\" "   // the attributes MUST be XPos YPos ZPos ARot BRot CRot
                            << "YPos=\"" << frame[1] << "\" "
                            << "ZPos=\"" << frame[2] << "\" "
                            << "ARot=\"" << frame[3] << "\" "
                            << "BRot=\"" << frame[4] << "\" "
                            << "CRot=\"" << frame[5] << "\" />\r\n";
    }

    to_message_stream   << "</ExternalData>\r\n";

}

/*
void KukaCommand::formatPose( boost::asio::streambuf &message, const info_vec &info , const frame_vec &frame)
{
    // TODO: rewrite this for new data structure - vector< framevector<double> >
    std::ostream to_message_stream(&message);


    to_message_stream   << "<ExternalData>\r\n";

    to_message_stream   << "<RMode>" << info[0] << "</RMode>\r\n";
    to_message_stream   << "<RMs>" << info[1] << "</RMs>\r\n";
    to_message_stream   << "<Id>" << info[2] << "</Id>\r\n";
    to_message_stream   << "<Run>" << info[3] << "</Run>\r\n";
    to_message_stream   << "<Vel>" << info[4] << "</Vel>\r\n";
    to_message_stream   << "<Tol>" << info[5] << "</Tol>\r\n";
    to_message_stream   << "<FrameType>" << info[6] << "</FrameType>\r\n";
    to_message_stream   << "<FrameCount>" << info[7] << "</FrameCount>\r\n";

    to_message_stream   << "<XFrame XPos=\"" << frame[0] << "\" "   // the attributes MUST be XPos YPos ZPos ARot BRot CRot
                        << "YPos=\"" << frame[1] << "\" "
                        << "ZPos=\"" << frame[2] << "\" "
                        << "ARot=\"" << frame[3] << "\" "
                        << "BRot=\"" << frame[4] << "\" "
                        << "CRot=\"" << frame[5] << "\" />\r\n";

    to_message_stream   << "</ExternalData>\r\n";

    //to_message_stream   << "\r\n";
}
*/
