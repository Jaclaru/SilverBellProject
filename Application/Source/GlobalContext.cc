#include "GlobalContext.hh"

using namespace SilverBell::Application;

GlobalContext& GlobalContext::Instance()
{
    static GlobalContext Instance;
    return Instance;
}