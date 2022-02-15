#ifndef COMMON_H
#define COMMON_H

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>


template <class T>
T read_numeric(std::string str)
{
    T value;

    try {
        boost::trim(str);
        value = boost::lexical_cast<T>(str);
    }
    catch (const boost::bad_lexical_cast &) {
        throw "Could not parse numeric value";
    }

    return value;
}


#endif // COMMON_H
