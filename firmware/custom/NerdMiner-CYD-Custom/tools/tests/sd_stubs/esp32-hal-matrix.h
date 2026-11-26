#pragma once
#include <map>
inline std::map<int,int> host_routes;
inline void pinMatrixOutAttach(int pin,int signal,bool,bool) { host_routes[pin]=signal; }
