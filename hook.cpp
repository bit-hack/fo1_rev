#include <stdint.h>
#include <Windows.h>

void hook( void * pos, void * to ) {
    uint8_t * o1 = (uint8_t*) pos;
    uint8_t * o2 = (uint8_t*) to;
    DWORD old = 0;
    VirtualProtect(pos, 5, PAGE_EXECUTE_READWRITE, &old);
    o1[0] = 0xe9;
    *(uint32_t*)(o1+1) = o2 - (o1+5);
    VirtualProtect(pos, 5, old, &old);
}
