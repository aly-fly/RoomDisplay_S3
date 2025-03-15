#include <Arduino.h>
#include <stdint.h>
#include <esp_heap_caps.h>

// https://github.com/espressif/esp-idf/blob/master/components/esp_common/include/esp_attr.h#L121
// create static buffer for reading stream from the server
// uint8_t gBuffer[3000] = { 0 }; // 3 kB
// EXT_RAM_BSS_ATTR uint8_t gBuffer[3000] = { 0 }; // 3 kB
__attribute__((section(".ext_ram.bss")))  uint8_t gBuffer[3000];
__attribute__((section(".ext_ram.bss")))  uint8_t GIFimage[100000]; // 100 kB, typ image is 50 kB


// use PSRAM
uint8_t *gBuffer1 = nullptr;

void globalVariablesInit(void)
{
    Serial.println("globalVariablesInit");
    // definitely allocate buffer in PSRAM
    // capabilities: 8/16/…-bit data accesses possible and in SPIRAM.
    gBuffer1 = (uint8_t *) heap_caps_malloc(3000, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    //ps_malloc // same as above
    //ps_calloc // same as above + init to zero
    assert(gBuffer1);
    Serial.printf(" Address static:  %p\r\n", (void *)gBuffer);
    Serial.printf(" Address dynamic: %p\r\n", (void *)gBuffer1);    
}

void globalVariablesFree(void)
{
    // if allocated with heaps_caps_malloc
    heap_caps_free(gBuffer1);
    // good practice
    gBuffer1 = nullptr;
}
