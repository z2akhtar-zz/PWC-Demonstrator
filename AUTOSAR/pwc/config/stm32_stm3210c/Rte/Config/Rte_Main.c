
#include <Rte_Internal.h>
#include <Rte_Fifo.h>
#include <Rte_Calprms.h>

extern boolean RteInitialized;

/** === Lifecycle API =============================================================================
 */
Std_ReturnType Rte_Start(void) {
    // Initialize calibration parameters
    Rte_Init_Calprms();

    // Initialize buffers
    Rte_Internal_Init_Buffers();

    // Initialize port status

    // Initialize mode machines

    RteInitialized = true;
    return RTE_E_OK;
}

Std_ReturnType Rte_Stop(void) {
    RteInitialized = false;
    return RTE_E_OK;
}

