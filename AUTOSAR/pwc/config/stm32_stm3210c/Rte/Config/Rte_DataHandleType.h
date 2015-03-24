/**
 * Data Handle Types Header File
 *
 * @req SWS_Rte_07920
 * @req SWS_Rte_07921
 * @req SWS_Rte_07922
 * @req SWS_Rte_07923
 * @req SWS_Rte_07136
 */

#ifndef RTE_DATAHANDLETYPE_H_
#define RTE_DATAHANDLETYPE_H_

#include <Rte_Type.h>

/** --- Data Element without Status ---------------------------------------------------------------
 *  @req SWS_Rte_01363
 *  @req SWS_Rte_01364
 *  @req SWS_Rte_02607
 */

typedef boolean myBoolean;

typedef enum {
	request_neutral = 0 ,
	request_basic_down = 1,
	request_basic_up = 2,
	request_express_down = 3,
	request_express_up = 4
} requestType ;

typedef enum {
	command_neutral = 0,
	command_down = 1,
	command_up = 2
} commandType;

typedef struct {
    commandType value;
} Rte_DE_commandType;
typedef struct {
    requestType value;
} Rte_DE_requestType;
typedef struct {
    myBoolean value;
} Rte_DE_myBoolean;

/** --- Data Element with Status ------------------------------------------------------------------
 *  @req SWS_Rte_01365
 *  @req SWS_Rte_01366
 *  @req SWS_Rte_03734
 *  @req SWS_Rte_02666
 *  @req SWS_Rte_02589
 *  @req SWS_Rte_02590
 *  @req SWS_Rte_02609
 *  @req SWS_Rte_03836
 */

#endif /* RTE_DATAHANDLETYPE_H_ */
