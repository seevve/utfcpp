#include "../../../source/utf8.h"
using namespace utf8;

// [ 2852872 ] invalid utf16 strings were parsed without any error
void id_2852872()
{
    const unsigned short two_trail_surrogates[] = {0xdd00, 0xdd01, 0};
    vector<char> utf8_result;
    try
    {
        utf8::utf16to8(two_trail_surrogates, two_trail_surrogates+2, back_inserter(utf8_result));
        // should throw in the previous line and never get here
        check(false);
    }
    catch(utf8::invalid_utf16&)
    {
        // this is what we expect
    }
    catch(...)
    {
        // an unexpected exception happened
        check(false);
    }
}
