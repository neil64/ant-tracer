/*
 *  Macros to convert NRF error codes to sexy text.
 */

#include "defs.h"
#include "types.h"


const char *
NRFError(u16 code)
{
    switch (code)
    {
    case NRF_SUCCESS:
        return "NRF_SUCCESS";
    case NRF_ERROR_SVC_HANDLER_MISSING:
        return "NRF_ERROR_SVC_HANDLER_MISSING";
    case NRF_ERROR_SOFTDEVICE_NOT_ENABLED:
        return "NRF_ERROR_SOFTDEVICE_NOT_ENABLED";
    case NRF_ERROR_INTERNAL:
        return "NRF_ERROR_INTERNAL";
    case NRF_ERROR_NO_MEM:
        return "NRF_ERROR_NO_MEM";
    case NRF_ERROR_NOT_FOUND:
        return "NRF_ERROR_NOT_FOUND";
    case NRF_ERROR_NOT_SUPPORTED:
        return "NRF_ERROR_NOT_SUPPORTED";
    case NRF_ERROR_INVALID_PARAM:
        return "NRF_ERROR_INVALID_PARAM";
    case NRF_ERROR_INVALID_STATE:
        return "NRF_ERROR_INVALID_STATE";
    case NRF_ERROR_INVALID_LENGTH:
        return "NRF_ERROR_INVALID_LENGTH";
    case NRF_ERROR_INVALID_FLAGS:
        return "NRF_ERROR_INVALID_FLAGS";
    case NRF_ERROR_INVALID_DATA:
        return "NRF_ERROR_INVALID_DATA";
    case NRF_ERROR_DATA_SIZE:
        return "NRF_ERROR_DATA_SIZE";
    case NRF_ERROR_TIMEOUT:
        return "NRF_ERROR_TIMEOUT";
    case NRF_ERROR_NULL:
        return "NRF_ERROR_NULL";
    case NRF_ERROR_FORBIDDEN:
        return "NRF_ERROR_FORBIDDEN";
    case NRF_ERROR_INVALID_ADDR:
        return "NRF_ERROR_INVALID_ADDR";
    case NRF_ERROR_BUSY:
        return "NRF_ERROR_BUSY";
    case NRF_ERROR_CONN_COUNT:
        return "NRF_ERROR_CONN_COUNT";
    case NRF_ERROR_RESOURCES:
        return "NRF_ERROR_RESOURCES";
    case NRF_ERROR_INVALID_LICENSE_KEY:
        return "NRF_ERROR_INVALID_LICENSE_KEY";

    case NRF_ANT_ERROR_CHANNEL_IN_WRONG_STATE:
        return "NRF_ANT_ERROR_CHANNEL_IN_WRONG_STATE";
    case NRF_ANT_ERROR_CHANNEL_NOT_OPENED:
        return "NRF_ANT_ERROR_CHANNEL_NOT_OPENED";
    case NRF_ANT_ERROR_CHANNEL_ID_NOT_SET:
        return "NRF_ANT_ERROR_CHANNEL_ID_NOT_SET";
    case NRF_ANT_ERROR_CLOSE_ALL_CHANNELS:
        return "NRF_ANT_ERROR_CLOSE_ALL_CHANNELS";
    case NRF_ANT_ERROR_TRANSFER_IN_PROGRESS:
        return "NRF_ANT_ERROR_TRANSFER_IN_PROGRESS";
    case NRF_ANT_ERROR_TRANSFER_SEQUENCE_NUMBER_ERROR:
        return "NRF_ANT_ERROR_TRANSFER_SEQUENCE_NUMBER_ERROR";
    case NRF_ANT_ERROR_TRANSFER_IN_ERROR:
        return "NRF_ANT_ERROR_TRANSFER_IN_ERROR";
    case NRF_ANT_ERROR_TRANSFER_BUSY:
        return "NRF_ANT_ERROR_TRANSFER_BUSY";
    case NRF_ANT_ERROR_MESSAGE_SIZE_EXCEEDS_LIMIT:
        return "NRF_ANT_ERROR_MESSAGE_SIZE_EXCEEDS_LIMIT";
    case NRF_ANT_ERROR_INVALID_MESSAGE:
        return "NRF_ANT_ERROR_INVALID_MESSAGE";
    case NRF_ANT_ERROR_INVALID_NETWORK_NUMBER:
        return "NRF_ANT_ERROR_INVALID_NETWORK_NUMBER";
    case NRF_ANT_ERROR_INVALID_LIST_ID:
        return "NRF_ANT_ERROR_INVALID_LIST_ID";
    case NRF_ANT_ERROR_INVALID_SCAN_TX_CHANNEL:
        return "NRF_ANT_ERROR_INVALID_SCAN_TX_CHANNEL";
    case NRF_ANT_ERROR_INVALID_PARAMETER_PROVIDED:
        return "NRF_ANT_ERROR_INVALID_PARAMETER_PROVIDED";
    }
    return "UNKNOWN_ERR_CODE";
}

/****************************************************************************/

const char *
NRFANTEvent(u16 code)
{
    switch (code)
    {
    case RESPONSE_NO_ERROR:
        return "RESPONSE_NO_ERROR";
    case EVENT_RX_SEARCH_TIMEOUT:
        return "EVENT_RX_SEARCH_TIMEOUT";
    case EVENT_RX_FAIL:
        return "EVENT_RX_FAIL";
    case EVENT_TX:
        return "EVENT_TX";
    case EVENT_TRANSFER_RX_FAILED:
        return "EVENT_TRANSFER_RX_FAILED";
    case EVENT_TRANSFER_TX_COMPLETED:
        return "EVENT_TRANSFER_TX_COMPLETED";
    case EVENT_TRANSFER_TX_FAILED:
        return "EVENT_TRANSFER_TX_FAILED";
    case EVENT_CHANNEL_CLOSED:
        return "EVENT_CHANNEL_CLOSED";
    case EVENT_RX_FAIL_GO_TO_SEARCH:
        return "EVENT_RX_FAIL_GO_TO_SEARCH";
    case EVENT_CHANNEL_COLLISION:
        return "EVENT_CHANNEL_COLLISION";
    case EVENT_TRANSFER_TX_START:
        return "EVENT_TRANSFER_TX_START";
    case EVENT_TRANSFER_NEXT_DATA_BLOCK:
        return "EVENT_TRANSFER_NEXT_DATA_BLOCK";
    case CHANNEL_IN_WRONG_STATE:
        return "CHANNEL_IN_WRONG_STATE";
    case CHANNEL_NOT_OPENED:
        return "CHANNEL_NOT_OPENED";
    case CHANNEL_ID_NOT_SET:
        return "CHANNEL_ID_NOT_SET";
    case CLOSE_ALL_CHANNELS:
        return "CLOSE_ALL_CHANNELS";
    case TRANSFER_IN_PROGRESS:
        return "TRANSFER_IN_PROGRESS";
    case TRANSFER_SEQUENCE_NUMBER_ERROR:
        return "TRANSFER_SEQUENCE_NUMBER_ERROR";
    case TRANSFER_IN_ERROR:
        return "TRANSFER_IN_ERROR";
    case TRANSFER_BUSY:
        return "TRANSFER_BUSY";
    case MESSAGE_SIZE_EXCEEDS_LIMIT:
        return "MESSAGE_SIZE_EXCEEDS_LIMIT";
    case INVALID_MESSAGE:
        return "INVALID_MESSAGE";
    case INVALID_NETWORK_NUMBER:
        return "INVALID_NETWORK_NUMBER";
    case INVALID_LIST_ID:
        return "INVALID_LIST_ID";
    case INVALID_SCAN_TX_CHANNEL:
        return "INVALID_SCAN_TX_CHANNEL";
    case INVALID_PARAMETER_PROVIDED:
        return "INVALID_PARAMETER_PROVIDED";
    case EVENT_QUE_OVERFLOW:
        return "EVENT_QUE_OVERFLOW";
    case EVENT_ENCRYPT_NEGOTIATION_SUCCESS:
        return "EVENT_ENCRYPT_NEGOTIATION_SUCCESS";
    case EVENT_ENCRYPT_NEGOTIATION_FAIL:
        return "EVENT_ENCRYPT_NEGOTIATION_FAIL";
    case EVENT_RFACTIVE_NOTIFICATION:
        return "EVENT_RFACTIVE_NOTIFICATION";
    case EVENT_CONNECTION_START:
        return "EVENT_CONNECTION_START";
    case EVENT_CONNECTION_SUCCESS:
        return "EVENT_CONNECTION_SUCCESS";
    case EVENT_CONNECTION_FAIL:
        return "EVENT_CONNECTION_FAIL";
    case EVENT_CONNECTION_TIMEOUT:
        return "EVENT_CONNECTION_TIMEOUT";
    case EVENT_CONNECTION_UPDATE:
        return "EVENT_CONNECTION_UPDATE";
    case NO_RESPONSE_MESSAGE:
        return "NO_RESPONSE_MESSAGE";
    case EVENT_RX:
        return "EVENT_RX";
    case EVENT_BLOCKED:
        return "EVENT_BLOCKED";
    }
    return "UNKNOWN_EVT_CODE";
}

/****************************************************************************/

#if WE_HAVE_BLE
#    include "ble.h"

const char *
NRFBLEEvent(u16 code)
{
    switch (code)
    {
    case BLE_EVT_TX_COMPLETE:
        return "BLE_EVT_TX_COMPLETE";
    case BLE_EVT_USER_MEM_REQUEST:
        return "BLE_EVT_USER_MEM_REQUEST";
    case BLE_EVT_USER_MEM_RELEASE:
        return "BLE_EVT_USER_MEM_RELEASE";
    case BLE_GAP_EVT_CONNECTED:
        return "BLE_GAP_EVT_CONNECTED";
    case BLE_GAP_EVT_DISCONNECTED:
        return "BLE_GAP_EVT_DISCONNECTED";
    case BLE_GAP_EVT_CONN_PARAM_UPDATE:
        return "BLE_GAP_EVT_CONN_PARAM_UPDATE";
    case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
        return "BLE_GAP_EVT_SEC_PARAMS_REQUEST";
    case BLE_GAP_EVT_SEC_INFO_REQUEST:
        return "BLE_GAP_EVT_SEC_INFO_REQUEST";
    case BLE_GAP_EVT_PASSKEY_DISPLAY:
        return "BLE_GAP_EVT_PASSKEY_DISPLAY";
    case BLE_GAP_EVT_KEY_PRESSED:
        return "BLE_GAP_EVT_KEY_PRESSED";
    case BLE_GAP_EVT_AUTH_KEY_REQUEST:
        return "BLE_GAP_EVT_AUTH_KEY_REQUEST";
    case BLE_GAP_EVT_LESC_DHKEY_REQUEST:
        return "BLE_GAP_EVT_LESC_DHKEY_REQUEST";
    case BLE_GAP_EVT_AUTH_STATUS:
        return "BLE_GAP_EVT_AUTH_STATUS";
    case BLE_GAP_EVT_CONN_SEC_UPDATE:
        return "BLE_GAP_EVT_CONN_SEC_UPDATE";
    case BLE_GAP_EVT_TIMEOUT:
        return "BLE_GAP_EVT_TIMEOUT";
    case BLE_GAP_EVT_RSSI_CHANGED:
        return "BLE_GAP_EVT_RSSI_CHANGED";
    case BLE_GAP_EVT_ADV_REPORT:
        return "BLE_GAP_EVT_ADV_REPORT";
    case BLE_GAP_EVT_SEC_REQUEST:
        return "BLE_GAP_EVT_SEC_REQUEST";
    case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
        return "BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST";
    case BLE_GAP_EVT_SCAN_REQ_REPORT:
        return "BLE_GAP_EVT_SCAN_REQ_REPORT";
    case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP:
        return "BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP";
    case BLE_GATTC_EVT_REL_DISC_RSP:
        return "BLE_GATTC_EVT_REL_DISC_RSP";
    case BLE_GATTC_EVT_CHAR_DISC_RSP:
        return "BLE_GATTC_EVT_CHAR_DISC_RSP";
    case BLE_GATTC_EVT_DESC_DISC_RSP:
        return "BLE_GATTC_EVT_DESC_DISC_RSP";
    case BLE_GATTC_EVT_CHAR_VAL_BY_UUID_READ_RSP:
        return "BLE_GATTC_EVT_CHAR_VAL_BY_UUID_READ_RSP";
    case BLE_GATTC_EVT_READ_RSP:
        return "BLE_GATTC_EVT_READ_RSP";
    case BLE_GATTC_EVT_CHAR_VALS_READ_RSP:
        return "BLE_GATTC_EVT_CHAR_VALS_READ_RSP";
    case BLE_GATTC_EVT_WRITE_RSP:
        return "BLE_GATTC_EVT_WRITE_RSP";
    case BLE_GATTC_EVT_HVX:
        return "BLE_GATTC_EVT_HVX";
    case BLE_GATTC_EVT_TIMEOUT:
        return "BLE_GATTC_EVT_TIMEOUT";
    case BLE_GATTS_EVT_WRITE:
        return "BLE_GATTS_EVT_WRITE";
    case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
        return "BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST";
    case BLE_GATTS_EVT_SYS_ATTR_MISSING:
        return "BLE_GATTS_EVT_SYS_ATTR_MISSING";
    case BLE_GATTS_EVT_HVC:
        return "BLE_GATTS_EVT_HVC";
    case BLE_GATTS_EVT_SC_CONFIRM:
        return "BLE_GATTS_EVT_SC_CONFIRM";
    case BLE_GATTS_EVT_TIMEOUT:
        return "BLE_GATTS_EVT_TIMEOUT";
        return "BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST";
    case BLE_L2CAP_EVT_RX:
        return "BLE_L2CAP_EVT_RX";
    }
    return "UNKNOWN_EVT_CODE";
}
#endif // WE_HAVE_BLE

/**********************************************************************/
