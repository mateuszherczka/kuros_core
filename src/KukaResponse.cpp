#include "KukaResponse.hpp"

KukaResponse::KukaResponse() : info(3,0), frame(6,0) {}

KukaResponse::~KukaResponse() {}

void KukaResponse::parse(boost::asio::streambuf &message)
{

    // try to parse
    errorState = doc.Parse(streambufToPtr(message));

    if (errorState != 0)    // something wrong with xml
    {
        malformedXMLError("KukaResponse XMLDocument" );
        return;
    }

    // doc could parse message, try to get data
    valid = true;
    XMLHandle docHandle( &doc );
    XMLText *xmltext;

    // Status
    xmltext = docHandle.FirstChildElement( "Robot" )
              .FirstChildElement( "Status" )
              .FirstChild()   // this is a text, not an element
              .ToText();

    if( xmltext )
    {
        try
        {
            info[0] = boost::lexical_cast<int>(xmltext->Value());
        }
        catch (const boost::bad_lexical_cast &e)
        {
            badCast("Status");
        }
    }
    else
    {
        nodeNotFound("Status");
    }

    // Id
    xmltext = docHandle.FirstChildElement( "Robot" )
              .FirstChildElement( "Id" )
              .FirstChild()   // this is a text, not an element
              .ToText();

    if( xmltext )
    {
        try
        {
            info[1] = boost::lexical_cast<int>(xmltext->Value());
        }
        catch (const boost::bad_lexical_cast &e)
        {
            badCast("Id");
        }
    }
    else
    {
        nodeNotFound("Id");
    }

    // Tick
    xmltext = docHandle.FirstChildElement( "Robot" )
              .FirstChildElement( "Tick" )
              .FirstChild()   // this is a text, not an element
              .ToText();

    if( xmltext )
    {
        try
        {
            info[2] = boost::lexical_cast<int>(xmltext->Value());
        }
        catch (const boost::bad_lexical_cast &e)
        {
            badCast("Tick");
        }
    }
    else
    {
        nodeNotFound("Tick");
    }


    // ActPos
    XMLElement *xframe = docHandle.FirstChildElement( "Robot" ).
                         FirstChildElement( "ActPos" ).
                         ToElement();

    if ( xframe )      // check for nulpointer
    {

        int xmlerr = 0;

        xmlerr += xframe->QueryDoubleAttribute( "X",&frame[0] );    // errorcode 0 if ok

        xmlerr += xframe->QueryDoubleAttribute( "Y",&frame[1] );

        xmlerr += xframe->QueryDoubleAttribute( "Z",&frame[2] );

        xmlerr += xframe->QueryDoubleAttribute( "A",&frame[3] );    // errorcode 0 if ok

        xmlerr += xframe->QueryDoubleAttribute( "B",&frame[4] );

        xmlerr += xframe->QueryDoubleAttribute( "C",&frame[5] );

        if (xmlerr != 0)
        {
            malformedXMLError("ActPos Attributes");
        }
    }
    else
    {
        nodeNotFound("ActPos");
    }

    // Axis
    XMLElement *axframe = docHandle.FirstChildElement( "Robot" ).
                         FirstChildElement( "Axis" ).
                         ToElement();

    if ( axframe )      // check for nulpointer
    {

        int xmlerr = 0;

        xmlerr += axframe->QueryDoubleAttribute( "A1",&axis[0] );    // errorcode 0 if ok

        xmlerr += axframe->QueryDoubleAttribute( "A2",&axis[1] );

        xmlerr += axframe->QueryDoubleAttribute( "A3",&axis[2] );

        xmlerr += axframe->QueryDoubleAttribute( "A4",&axis[3] );    // errorcode 0 if ok

        xmlerr += axframe->QueryDoubleAttribute( "A5",&axis[4] );

        xmlerr += axframe->QueryDoubleAttribute( "A6",&axis[5] );

        if (xmlerr != 0)
        {
            malformedXMLError("Axis Attributes");
        }
    }
    else
    {
        nodeNotFound("Axis");
    }
}

/*
    Status Error Mode Tick Id
*/
std::vector<int> KukaResponse::getInfo()
{
    return info;
}

/*
    X Y Z A B C
*/
std::vector<double> KukaResponse::getFrame()
{
    return frame;
}

/*
    A1...A6
*/
std::vector<double> KukaResponse::getAxis()
{
    return axis;
}

void KukaResponse::printValues()
{

    cout << "Info: ";
    for (const auto &val : info)
    {
        cout << val << " ";
    }
    cout << endl;

    cout << "Frame: ";
    for (const auto &val : frame)
    {
        cout << val << " ";
    }
    cout << endl;

    cout << "Axis: ";
    for (const auto &val : axis)
    {
        cout << val << " ";
    }
    cout << endl;
}
