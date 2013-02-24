#include <assert.h>
#include "../src/utf8.h"
using namespace std;

int main()
{
// append
    {
	string s;
	utf8::append(U'\U00000448', s);
        assert (s.length() == 2 && s[0] == '\xd1' && s[1] == '\x88');

	s.erase();
	utf8::append(U'\U000065e5', s);
        assert (s.length() == 3 && s[0] == '\xe6' && s[1] == '\x97' && s[2] == '\xa5');
    }
    
}


